#include "EMGSensor.h"
#include "Config.h"
#include "Utils.h"

/**
 * @brief Konstruktor EMGSensoru
 * @param analogPin Analogový pin senzoru
 */
EMGSensor::EMGSensor(int analogPin) : pin(analogPin)
{
    pinMode(pin, INPUT);
}

/**
 * @brief Načte napětí ze senzoru
 * @param referenceVoltage Referenční napětí (default 5.0V)
 * @param adcResolution Rozlišení ADC (default 1023)
 * @return Naměřené napětí
 */
float EMGSensor::readVoltage(float referenceVoltage, int adcResolution)
{
    return analogRead(pin) * (referenceVoltage / adcResolution);
}

/**
 * @brief Aktualizuje obálku signálu
 * @param referenceVoltage Referenční napětí (default 5.0V)
 * @return Nová hodnota obálky
 */
float EMGSensor::updateEnvelope(float referenceVoltage)
{
    float voltage = readVoltage(referenceVoltage);
    float centered = voltage - (referenceVoltage / 3.0);
    float rectified = fabs(centered);
    envelope = alpha * rectified + (1 - alpha) * envelope;
    return envelope;
}

/**
 * @brief Zjistí, zda je senzor aktivní (nad prahem)
 * @return True pokud je aktivní
 */
bool EMGSensor::isActive() const
{
    return envelope > thresholdUpper;
}

/**
 * @brief Kalibruje senzor po zadanou dobu
 * @param durationMs Doba kalibrace v ms (default 3000)
 */
void EMGSensor::calibrate(unsigned long durationMs)
{
    const int maxSamples = 500;
    float samples[maxSamples];
    int count = 0;

    unsigned long tStart = millis();
    while (millis() - tStart < durationMs && count < maxSamples)
    {
        updateEnvelope();
        samples[count++] = envelope;
        delay(refreshRate);
    }

    float sum = 0;
    for (int i = 0; i < count; i++)
        sum += samples[i];
    mean = sum / count;

    float varSum = 0;
    for (int i = 0; i < count; i++)
        varSum += pow(samples[i] - mean, 2);
    float stdDev = sqrt(varSum / count);

    thresholdUpper = mean + 3.0 * stdDev;
    thresholdLower = mean - 3.0 * stdDev;

    printIfPinLow(F("Kalibrace:"), debugPin);

    char msg[64];

    snprintf(msg, sizeof(msg), "Průměr: %.4f", mean);
    printIfPinLow(msg, debugPin);

    snprintf(msg, sizeof(msg), "Směrodatná odchylka: %.6f", stdDev);
    printIfPinLow(msg, debugPin);

    snprintf(msg, sizeof(msg), "Nastaven prah upper: %.6f", thresholdUpper);
    printIfPinLow(msg, debugPin);

    snprintf(msg, sizeof(msg), "Nastaven prah lower: %.6f", thresholdLower);
    printIfPinLow(msg, debugPin);
}

/**
 * @brief Vrací aktuální hodnotu obálky
 * @return Hodnota obálky
 */
float EMGSensor::getEnvelope() const
{
    return envelope;
}
