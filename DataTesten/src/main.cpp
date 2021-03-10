#include "IMU.h"
#include "Broker.h"

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;

  // initialize the IMU
  status1 = imu.begin();
  if (status1 < 0)
  {
    Serial.println("Failed to initialize IMU!");
    while (1)
      ;
  }

  /* Set sample rate divider for 50 Hz */
  bool status = imu.setSrd(19);
  if (!status)
  {
    // ERROR
  }

  // get the TFL representation of the model byte array
  tflModel = tflite::GetModel(model);
  if (tflModel->version() != TFLITE_SCHEMA_VERSION)
  {
    Serial.println("Model schema mismatch!");
    while (1)
      ;
  }

  tflOpsResolver.AddBuiltin(
      tflite::BuiltinOperator_DEPTHWISE_CONV_2D,
      tflite::ops::micro::Register_DEPTHWISE_CONV_2D());
  tflOpsResolver.AddBuiltin(
      tflite::BuiltinOperator_MAX_POOL_2D,
      tflite::ops::micro::Register_MAX_POOL_2D());
  tflOpsResolver.AddBuiltin(tflite::BuiltinOperator_CONV_2D,
                            tflite::ops::micro::Register_CONV_2D());
  tflOpsResolver.AddBuiltin(
      tflite::BuiltinOperator_FULLY_CONNECTED,
      tflite::ops::micro::Register_FULLY_CONNECTED());
  tflOpsResolver.AddBuiltin(tflite::BuiltinOperator_SOFTMAX,
                            tflite::ops::micro::Register_SOFTMAX());

  // Create an interpreter to run the model
  tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

  // Allocate memory for the model's input and output tensors
  tflInterpreter->AllocateTensors();

  // Get pointers for the model's input and output tensors
  tflInputTensor = tflInterpreter->input(0);
  tflOutputTensor = tflInterpreter->output(0);

  // output declareren
  pinMode(SpeakerPin, OUTPUT);
  pinMode(PushButton, INPUT_PULLUP);

  // Attach an interrupt to the ISR vector
  attachInterrupt(15, pin_ISR, CHANGE);

  // testen
  Aantalwaves = 0;
  Aantalpunches = 0;

  // wifi setup
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);
  oefReeks = esp_random() % 2 + 1;
  Serial.println("De setup is klaar.");
  Serial.print("Oefeningenreeks: ");
  Serial.println(oefReeks);
  Serial.println("");
}

void loop()
{
  if (!client.connected())
  {
    reconnect(oefReeks);
  }
  client.loop();

  delay(500);

  if (buttonPressed == true)
  {
    geenFouten = true;
    client.publish("esp32/fitness/LCDmessage", "meten");
    switch (oefReeks)
    {
    case 1:
      while (Aantalwaves < 10 && geenFouten == true)
      {
        fitnessTracken();
        if (Aantalpunches > 0)
        {
          geenFouten = false;
        }
        else
        {
          client.publish("esp32/fitness/OKmessage", "oefOK");
        }
      }
      break;

    case 2:
      while (Aantalpunches < 10 && geenFouten == true)
      {
        fitnessTracken();
        if (Aantalwaves > 0)
        {
          geenFouten = false;
        }
        else
        {
          client.publish("esp32/fitness/OKmessage", "oefOK");
        }
      }
      break;

    default:
      break;
    }
    if (geenFouten == true)
    {
      Serial.println("De reeks juist uitgevoerd.");
      client.publish("esp32/fitness/LCDmessage", "juist");
    }
    else
    {
      Serial.println("De reeks was FOUT!!");
      client.publish("esp32/fitness/LCDmessage", "fout");
    }
    delay(100);
    Serial.print("Aantal waves: ");
    Serial.println(Aantalwaves);
    Serial.print("Aantal punches: ");
    Serial.println(Aantalpunches);
    Serial.println("");

    Aantalwaves = 0;
    Aantalpunches = 0;
    buttonPressed = false;
  }
}