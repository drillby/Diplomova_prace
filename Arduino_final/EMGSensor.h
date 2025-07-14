#ifndef EMG_SENSOR_H
#define EMG_SENSOR_H

#include <Arduino.h>

/**
 * @class EMGSensor
 * @brief Třída reprezentující jeden EMG senzor
 */
class EMGSensor
{
private:
    const int pin;               // Analogový pin senzoru
    float alpha = 0.6;           // Koeficient pro exponenciální vyhlazování
    float envelope = 0.0;        // Aktuální hodnota obálky signálu
    float thresholdUpper = 0.2;  // Horní práh pro detekci aktivity
    float thresholdLower = 0.05; // Dolní práh pro detekci aktivity
    float mean = 0.0;            // Průměrná hodnota signálu

public:
    /**
     * @brief Konstruktor EMGSensoru
     * @param analogPin Analogový pin senzoru
     */
    EMGSensor(int analogPin);

    /**
     * @brief Načte napětí ze senzoru
     * @param referenceVoltage Referenční napětí (default 5.0V)
     * @param adcResolution Rozlišení ADC (default 1023)
     * @return Naměřené napětí
     */
    float readVoltage(float referenceVoltage = 5.0, int adcResolution = 1023);

    /**
     * @brief Aktualizuje obálku signálu
     * @param referenceVoltage Referenční napětí (default 5.0V)
     * @return Nová hodnota obálky
     */
    float updateEnvelope(float referenceVoltage = 5.0);

    /**
     * @brief Zjistí, zda je senzor aktivní (nad prahem)
     * @return True pokud je aktivní
     */
    bool isActive() const;

    /**
     * @brief Kalibruje senzor po zadanou dobu
     * @param durationMs Doba kalibrace v ms (default 3000)
     */
    void calibrate(unsigned long durationMs = 3000);

    /**
     * @brief Vrací aktuální hodnotu obálky
     * @return Hodnota obálky
     */
    float getEnvelope() const;
};

#endif // EMG_SENSOR_H
