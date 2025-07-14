#ifndef WIFI_CONFIG_SYSTEM_H
#define WIFI_CONFIG_SYSTEM_H

#include <Arduino.h>
#include <WiFiNINA.h>
#include "EMGSystem.h"

/**
 * @class WiFiConfigSystem
 * @brief Třída pro správu WiFi konfigurace a web serveru
 */
class WiFiConfigSystem
{
private:
    WiFiServer server;    // HTTP server instance
    String wifiSSID;      // Uložené WiFi SSID
    String wifiPass;      // Uložené WiFi heslo
    bool isAPMode;        // Příznak režimu Access Point
    bool initialized;     // Příznak inicializace systému
    EMGSystem &emgSystem; // Reference na EMG systém

    /**
     * @brief Pokusí se připojit k WiFi síti
     * @return True pokud se připojení podařilo
     */
    bool connectToWiFi();

    /**
     * @brief Spustí Access Point režim
     * @return True pokud se AP spustil úspěšně
     */
    bool startAccessPoint();

    /**
     * @brief Zpracuje HTTP požadavek s WiFi údaji
     * @param reqLine Řádek HTTP požadavku
     * @param client WiFi klient pro odpověď
     * @return True pokud byl požadavek zpracován
     */
    bool handleWiFiConfig(const String &reqLine, WiFiClient &client);

    /**
     * @brief Odešle HTML stránku s potvrzením úspěšného uložení
     * @param client WiFi klient pro odpověď
     */
    void sendSuccessPage(WiFiClient &client);

    /**
     * @brief Odešle HTML stránku pro restart
     * @param client WiFi klient pro odpověď
     */
    void sendRestartPage(WiFiClient &client);

    /**
     * @brief Odešle hlavní konfigurační HTML stránku
     * @param client WiFi klient pro odpověď
     */
    void sendConfigPage(WiFiClient &client);

public:
    /**
     * @brief Konstruktor WiFiConfigSystem
     * @param emgSys Reference na EMG systém
     */
    WiFiConfigSystem(EMGSystem &emgSys);

    /**
     * @brief Inicializuje WiFi systém a načte konfiguraci z EEPROM
     */
    void begin();

    /**
     * @brief Hlavní aktualizační metoda (volat v loop)
     */
    void update();

    /**
     * @brief Vrací příznak režimu Access Point
     * @return True pokud je systém v AP režimu
     */
    bool isInAPMode() const;

    /**
     * @brief Vrací aktuální WiFi SSID
     * @return WiFi SSID řetězec
     */
    String getWiFiSSID() const;

    /**
     * @brief Vrací informaci o délce WiFi hesla
     * @return True pokud je heslo nastaveno
     */
    bool hasWiFiPassword() const;
};

#endif // WIFI_CONFIG_SYSTEM_H
