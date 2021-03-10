#include "WiFi.h"
#include "PubSubClient.h"

#define SSID "NETGEAR68"
#define PWD "excitedtuba713"

#define MQTT_SERVER "192.168.1.2" // could change if the setup is moved
#define MQTT_PORT 1883

#define LED_PIN 2

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;
const char *nmr;

void setup_wifi()
{
    delay(10);
    Serial.println("Connecting to WiFi..");

    WiFi.begin(SSID, PWD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

// callback function, only used when receiving messages
void callback(char *topic, byte *message, unsigned int length)
{
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(". Message: ");
    String messageTemp;

    for (int i = 0; i < length; i++)
    {
        Serial.print((char)message[i]);
        messageTemp += (char)message[i];
    }
    Serial.println();

    if (String(topic) == "esp32/fitness")
    {
        Serial.print("Changing state to ");
        if (messageTemp == "start")
        {
            Serial.println("start");
            digitalWrite(LED_PIN, HIGH);
        }
        else if (messageTemp == "stop")
        {
            Serial.println("stop");
            digitalWrite(LED_PIN, LOW);
        }
        else if (messageTemp == "reset")
        {
            Serial.println("reset");
            setup();
        }
    }
    if (String(topic) == "esp32/fitness/control")
    {
        if (messageTemp == "0")
        {
            Serial.println("reset");
            setup();
        }
    }
    if (String(topic) == "esp32/fitness/nmr")
    {
        if (messageTemp == "nmr")
        {
            int i = messageTemp.toInt();
            Serial.println(i);
        }
    }
}

// function to establish MQTT connection
void reconnect(int oefReeks)
{
    delay(10);
    // Loop until we're reconnected
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect("ESP8266Client"))
        {
            Serial.println("connected");
            // Publish
            client.publish("esp32/fitness/OKmessage", "OK");
            // ... and resubscribe
            delay(5000);

            String nummer = (String)oefReeks;
            nmr = nummer.c_str();
            client.publish("esp32/fitness/nmrOef", nmr);
            client.subscribe("esp32/fitness");
            client.subscribe("esp32/fitness/nmrOef");
            client.subscribe("esp32/fitness/control");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }
}