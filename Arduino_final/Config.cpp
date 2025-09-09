#include "Config.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "EEPROMManager.h"

/**
 * @brief WiFi konfigurace a systémové parametry
 */
const char apSSID[] = "ArduinoAP";        // SSID Access Pointu
const char apPass[] = "12345678";         // Heslo Access Pointu
const int httpPort = 80;                  // HTTP port pro web server
const int tcpPort = 8888;                 // TCP port pro EMG server
const int restApiPort = 8080;             // REST API port
const int serialBaudRate = 9600;          // Baud rate pro Serial komunikaci
const uint16_t wifiTimeoutMs = 20000;     // Timeout pro WiFi připojení v ms
const uint16_t apStabilizationMs = 10000; // Čas pro stabilizaci AP v ms

/**
 * @brief EEPROM konfigurace
 */
const int EEPROM_ADDR_WIFISSID = 0;  // EEPROM adresa pro WiFi SSID
const int EEPROM_ADDR_WIFIPASS = 40; // EEPROM adresa pro WiFi heslo (odděleno dostatečně)
const int maxStringLength = 31;      // Maximální délka řetězce v EEPROM

/**
 * @brief Placeholder hodnoty pro reset síťových přihlašovacích údajů
 */
const char placeholderSSID[] = "PLACEHOLDER"; // Placeholder SSID
const char placeholderPass[] = "PLACEHOLDER"; // Placeholder heslo

/**
 * @brief EMG systém parametry
 */
const int refreshRateHz = 1000;                   // Frekvence aktualizace v Hz
const int refreshRate = 1000 / refreshRateHz;     // Perioda smyčky v ms
const int debugPin = 7;                           // Pin pro výpis debug informací
const int serialPrintPin = 6;                     // Pin pro výpis dat přes Serial
const int resetNetworkCreds = 5;                  // Pin pro reset síťových přihlašovacích údajů
const int maxSensors = 4;                         // Maximální počet podporovaných senzorů
const int emgPins[maxSensors] = {A0, A1, A2, A3}; // Analogové piny EMG senzorů
const uint16_t aliveIntervalMs = 10000;           // Interval mezi "ALIVE" zprávami

/**
 * @brief Resetuje síťové přihlašovací údaje na placeholder hodnoty
 */
void resetNetworkCredentials()
{
    // Zahájení EEPROM
    EEPROMManager::begin();

    // Zápis placeholder hodnot pomocí EEPROMManager
    EEPROMManager::writeString(EEPROM_ADDR_WIFISSID, placeholderSSID);
    EEPROMManager::writeString(EEPROM_ADDR_WIFIPASS, placeholderPass);
}
