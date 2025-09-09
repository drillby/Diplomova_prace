#include <SPI.h>
#include <WiFiNINA.h>
#include "Config.h"
#include "Utils.h"
#include "CommandTable.h"
#include "EEPROMManager.h"
#include "EMGSensor.h"
#include "EMGSystem.h"
#include "WiFiConfigSystem.h"
#include "LCDDisplay.h"

/**
 * @brief Globální instance WiFi konfiguračního systému, EMG systému a LCD displeje
 */
LCDDisplay display;
EMGSystem emgSystem(tcpPort);
WiFiConfigSystem wifiConfig(emgSystem, &display);

/**
 * @brief Inicializační funkce Arduino
 */
void setup()
{
    pinMode(debugPin, INPUT_PULLUP);
    pinMode(serialPrintPin, INPUT_PULLUP);
    pinMode(resetNetworkCreds, INPUT_PULLUP);
    Serial.begin(serialBaudRate);
    while (!Serial)
        ;

    // Kontrola reset síťových přihlašovacích údajů
    if (digitalRead(debugPin) == LOW && digitalRead(resetNetworkCreds) == LOW)
    {
        Serial.println(F("RESET AKTIVOVÁN!"));
        Serial.println(F("Ukládání..."));

        resetNetworkCredentials();

        Serial.println(F("Reset dokončen."));
        Serial.print(F("SSID: "));
        Serial.println(placeholderSSID);
        Serial.print(F("Heslo: "));
        Serial.println(placeholderPass);
        Serial.println(F(""));
        Serial.println(F("!!! RESTART !!!"));
        Serial.println(F("Stiskněte reset tlačítko."));

        // Nekonečná smyčka - zastavení Arduino
        while (true)
        {
            Serial.println(F("Čekání..."));
            delay(2000);
        }
    }

    printIfPinLow(F("Start systému..."), debugPin);

    // Inicializace LCD displeje
    if (display.begin(16, 2))
    {
        display.printAt(0, 0, F("EMG System"));
        display.printAt(0, 1, F("Inicializace..."));
        delay(1000);
    }

    // Předání LCD displeje do systémů
    emgSystem.setLCDDisplay(&display);

    wifiConfig.begin();
    printIfPinLow(F("Systém připraven."), debugPin);

    while (digitalRead(debugPin) == LOW && digitalRead(serialPrintPin) == LOW)
    {
        Serial.print(F("Použij jen jeden pin: debug ("));
        Serial.print(debugPin);
        Serial.print(F(") nebo serial ("));
        Serial.print(serialPrintPin);
        Serial.println(F("). NE OBA!"));
        delay(1000);
    }
}

/**
 * @brief Hlavní smyčka Arduino
 */
void loop()
{
    int timeStart = millis();
    wifiConfig.update();
    int timeEnd = millis();
    delay(max(0, refreshRate - (timeEnd - timeStart)));
}
