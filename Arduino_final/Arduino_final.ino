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
        Serial.println("RESET SÍŤOVÝCH PŘIHLAŠOVACÍCH ÚDAJŮ AKTIVOVÁN!");
        Serial.println("Ukládání placeholder hodnot do EEPROM...");

        resetNetworkCredentials();

        Serial.println("Síťové přihlašovací údaje byly resetovány na placeholder hodnoty.");
        Serial.println("SSID: " + String(placeholderSSID));
        Serial.println("Heslo: " + String(placeholderPass));
        Serial.println("");
        Serial.println("!!! RESTARTUJTE ARDUINO !!!");
        Serial.println("Pro restart stiskněte reset tlačítko nebo odpojte a znovu připojte napájení.");

        // Nekonečná smyčka - zastavení Arduino
        while (true)
        {
            Serial.println("Čekání na restart...");
            delay(2000);
        }
    }

    printIfPinLow("Start systému...", debugPin);

    // Inicializace LCD displeje
    if (display.begin(16, 2))
    {
        display.setBacklightColor(0, 100, 255); // Modrá barva pro start
        display.printAt(0, 0, "EMG System");
        display.printAt(0, 1, "Inicializace...");
        delay(1000);
    }

    // Předání LCD displeje do systémů
    emgSystem.setLCDDisplay(&display);

    wifiConfig.begin();
    printIfPinLow("Systém připraven.", debugPin);

    while (digitalRead(debugPin) == LOW && digitalRead(serialPrintPin) == LOW)
    {
        Serial.println("Pro správné zasílání dat přes serial je potřeba mít zapojen debug pin (pin " + String(debugPin) + "), nebo serial print pin (pin " + String(serialPrintPin) + "). NE OBOJÍ!");
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
