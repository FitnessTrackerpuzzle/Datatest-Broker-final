#include "Arduino.h"
#include "Wire.h"
// library from https://github.com/bolderflight/MPU9250
#include "MPU9250.h"
#include "Button.h"
#include "TensorFlowLite_ESP32.h"
#include "tensorflow/lite/experimental/micro/kernels/micro_ops.h"
#include "tensorflow/lite/experimental/micro/micro_error_reporter.h"
#include "tensorflow/lite/experimental/micro/micro_interpreter.h"
#include "tensorflow/lite/experimental/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"
#include "model.h"

#define NUM_AXES 6
#define ACCEL_THRESHOLD 5
#define INTERVAL 10
#define NUM_SAMPLES 30
// sometimes you may get "spikes" in the readings
// set a sensible value to truncate too large values
#define TRUNCATE_AT 20

/// nodige variabelen om te werken met tensorflow ///
// global variables used for TensorFlow Lite (Micro)
tflite::MicroErrorReporter tflErrorReporter;

// pull in all the TFLM ops, you can remove this line and
// only pull in the TFLM ops you need, if would like to reduce
// the compiled size of the sketch.
static tflite::MicroMutableOpResolver tflOpsResolver;

const tflite::Model *tflModel = nullptr;
tflite::MicroInterpreter *tflInterpreter = nullptr;
TfLiteTensor *tflInputTensor = nullptr;
TfLiteTensor *tflOutputTensor = nullptr;

// Create a static memory buffer for TFLM, the size may need to
// be adjusted based on the model you are using
constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];

// array to map gesture index to a name
const char *GESTURES[] = {
    "Punch",
    "Wave"};

#define NUM_GESTURES (sizeof(GESTURES) / sizeof(GESTURES[0]))
/// einde tensorflow ///

MPU9250 imu(Wire, 0x68);

// 0 = wave
// 1 = punch
int motion;

// de sequentie van oefeningen die je moet uitvoeren
// 1 = 10 keer waven
// 2 = 10 keer punchen
int oefReeks;

// algemene variabelen
const float accelerationThreshold = 25; // threshold of significant in G's
const int numSamples = 95;
int samplesRead = numSamples;
const int SpeakerPin = 5;
int Aantalwaves;
int Aantalpunches;
int status1;
bool geenFouten;

void imu_setup()
{
    Wire.begin();
    imu.begin();
}

void imu_read(float *ax, float *ay, float *az, float *gX, float *gY, float *gZ)
{
    imu.readSensor();

    *ax = imu.getAccelX_mss();
    *ay = imu.getAccelY_mss();
    *az = imu.getAccelZ_mss();
    *gX = imu.getGyroX_rads();
    *gY = imu.getGyroY_rads();
    *gZ = imu.getGyroZ_rads();
}

void checkMotion(int motion)
{
    switch (motion)
    {
    case 0: // wave
        Aantalwaves++;
        break;
    case 1: // punch
        Aantalpunches++;
        break;

    default:
        break;
    }
}

void fitnessTracken()
{
    digitalWrite(SpeakerPin, HIGH);
    delay(100);
    digitalWrite(SpeakerPin, LOW);
    float aX, aY, aZ, gX, gY, gZ;

    // wait for significant motion
    while (samplesRead == numSamples)
    {
        // read the acceleration data
        imu_read(&aX, &aY, &aZ, &gX, &gY, &gZ);

        // sum up the absolutes
        float aSum = fabs(aX) + fabs(aY) + fabs(aZ);

        // check if it's above the threshold
        if (aSum >= accelerationThreshold)
        {
            // reset the sample read count
            samplesRead = 0;
            break;
        }
    }

    // check if the all the required samples have been read since
    // the last time the significant motion was detected
    while (samplesRead < numSamples)
    {
        // check if new acceleration AND gyroscope data is available
        // read the acceleration and gyroscope data
        imu_read(&aX, &aY, &aZ, &gX, &gY, &gZ);

        // normalize the IMU data between 0 to 1 and store in the model's
        // input tensor
        tflInputTensor->data.f[samplesRead * 6 + 0] = (aX + 16.0) / 32.0;
        tflInputTensor->data.f[samplesRead * 6 + 1] = (aY + 16.0) / 32.0;
        tflInputTensor->data.f[samplesRead * 6 + 2] = (aZ + 16.0) / 32.0;
        tflInputTensor->data.f[samplesRead * 6 + 3] = (gX + 2000.0) / 4000.0;
        tflInputTensor->data.f[samplesRead * 6 + 4] = (gY + 2000.0) / 4000.0;
        tflInputTensor->data.f[samplesRead * 6 + 5] = (gZ + 2000.0) / 4000.0;

        samplesRead++;

        if (samplesRead == numSamples)
        {
            // Run inferencing
            TfLiteStatus invokeStatus = tflInterpreter->Invoke();
            if (invokeStatus != kTfLiteOk)
            {
                Serial.println("Invoke failed!");
                while (1)
                    ;
                return;
            }

            // Loop through the output tensor values from the model
            // for (int i = 0; i < NUM_GESTURES; i++)
            // {
            //   Serial.print(GESTURES[i]);
            //   Serial.print(": ");
            //   Serial.println(tflOutputTensor->data.f[i], 6);
            // }

            if (tflOutputTensor->data.f[0] < tflOutputTensor->data.f[1])
            {
                motion = 0;
            }
            else
            {
                motion = 1;
            }
            checkMotion(motion);
        }
        delay(20);
    }
}
