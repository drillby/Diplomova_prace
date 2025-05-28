#include <Arduino.h>

// --- Parametry ---
const int refreshRateHz = 100;
const int refreshRate = 1000 / refreshRateHz;

const int emgPin = A0;
const float alpha = 0.4;
float envelope = 0.0;

// --- Inicializace ---
void setup()
{
    Serial.begin(9600);
    pinMode(emgPin, INPUT);
}

// --- Funkce pro čtení a zpracování signálu ---
float readVoltage(float referenceVoltage = 5.0, int adcResolution = 1023)
{
    return analogRead(emgPin) * (referenceVoltage / adcResolution);
}

float updateEnvelope(float referenceVoltage = 5.0)
{
    float voltage = readVoltage(referenceVoltage);
    float centered = voltage - (referenceVoltage / 2.0);
    float rectified = fabs(centered);
    envelope = alpha * rectified + (1 - alpha) * envelope;
    return envelope;
}

// --- Hlavní smyčka ---
void loop()
{
    float value = updateEnvelope();
    Serial.println(value); // Odeslání na Serial s 4 desetinnými místy
    delay(refreshRate);
}
