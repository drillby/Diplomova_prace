// screen / dev / ttyACM0 9600

#include <Arduino.h>

const int refreshRateHz = 100;                // Refresh rate in Hz
const int refreshRate = 1000 / refreshRateHz; // Convert Hz to milliseconds

void setup()
{
    Serial.begin(9600);
    pinMode(A0, INPUT); // EMG senzor vstup
    pinMode(9, OUTPUT); // PWM výstup (např. LED, motor, DAC filtr)
}

void loop()
{
    int emgValue = analogRead(A0); // Hodnota z EMG
    Serial.println(emgValue);
    delay(refreshRate); // Use the calculated refresh rate
}
