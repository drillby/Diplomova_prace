#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

/**
 * @brief WiFi konfigurace a systémové parametry
 */
extern const char apSSID[];              // SSID Access Pointu
extern const char apPass[];              // Heslo Access Pointu
extern const int httpPort;               // HTTP port pro web server
extern const int tcpPort;                // TCP port pro EMG server
extern const int serialBaudRate;         // Baud rate pro Serial komunikaci
extern const uint16_t wifiTimeoutMs;     // Timeout pro WiFi připojení v ms
extern const uint16_t apStabilizationMs; // Čas pro stabilizaci AP v ms

/**
 * @brief EEPROM konfigurace
 */
extern const int EEPROM_ADDR_WIFISSID; // EEPROM adresa pro WiFi SSID
extern const int EEPROM_ADDR_WIFIPASS; // EEPROM adresa pro WiFi heslo
extern const int maxStringLength;      // Maximální délka řetězce v EEPROM

/**
 * @brief Placeholder hodnoty pro reset síťových přihlašovacích údajů
 */
extern const char placeholderSSID[]; // Placeholder SSID
extern const char placeholderPass[]; // Placeholder heslo

/**
 * @brief Funkce pro reset síťových přihlašovacích údajů
 */
void resetNetworkCredentials();

/**
 * @brief EMG systém parametry
 */
extern const int refreshRateHz;        // Frekvence aktualizace v Hz
extern const int refreshRate;          // Perioda smyčky v ms
extern const int debugPin;             // Pin pro výpis debug informací
extern const int serialPrintPin;       // Pin pro výpis dat přes Serial
extern const int resetNetworkCreds;    // Pin pro reset síťových přihlašovacích údajů
extern const int maxSensors;           // Maximální počet podporovaných senzorů
extern const int emgPins[];            // Analogové piny EMG senzorů
extern const uint16_t aliveIntervalMs; // Interval mezi "ALIVE" zprávami

#endif // CONFIG_H
