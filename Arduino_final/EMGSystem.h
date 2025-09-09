#ifndef EMG_SYSTEM_H
#define EMG_SYSTEM_H

#include <Arduino.h>
#include <WiFiNINA.h>
#include "EMGSensor.h"
#include "LCDDisplay.h"

/**
 * @class EMGSystem
 * @brief Třída pro správu EMG systému a komunikace
 */
class EMGSystem
{
private:
    EMGSensor *sensors[2];               // Pole dvou EMG senzorů
    WiFiServer server;                   // TCP server
    WiFiClient client;                   // TCP klient
    bool initialized = false;            // Příznak inicializace systému
    long lastAliveTime = 0;              // Čas poslední ALIVE zprávy
    int cycledValue = 0;                 // Aktuálně zvolená hodnota příkazu
    bool emg1LastActive = false;         // Stav aktivity EMG1 v předchozím cyklu
    bool emg2LastActive = false;         // Stav aktivity EMG2 v předchozím cyklu
    unsigned long lastCycleTime = 0;     // Čas posledního cyklu volby
    unsigned long lastSendTime = 0;      // Čas posledního odeslání
    const unsigned long cooldown = 1000; // Cooldown mezi akcemi v ms
    bool wasClientConnected = false;     // Příznak předchozího připojení klienta
    LCDDisplay *lcdDisplay;              // Pointer na LCD displej

    /**
     * @brief Zpracuje zprávy od klienta
     */
    void handleClientMessages();

    /**
     * @brief Odesílá ALIVE zprávu v nastaveném intervalu
     */
    void sendAliveIfNeeded();

    /**
     * @brief Hlavní logika zpracování EMG signálů a komunikace
     */
    void handleLogic();

    /**
     * @brief Kalibruje oba senzory
     */
    void calibrateSensors();

    /**
     * @brief Inicializuje senzory
     */
    void initSensors();

    /**
     * @brief Zpracuje nového klienta
     */
    void handleNewClient();

    /**
     * @brief Uvolní paměť senzorů
     */
    void cleanupSensors();

    /**
     * @brief Odpojí klienta a resetuje systém
     */
    void cleanupClient();

public:
    /**
     * @brief Konstruktor EMGSystemu
     * @param port TCP port serveru
     */
    EMGSystem(int port);

    /**
     * @brief Spustí TCP server pro EMG systém
     */
    void beginServer();

    /**
     * @brief Hlavní aktualizační metoda systému (volat v loop)
     */
    void update();

    /**
     * @brief Vrací příznak inicializace EMG systému
     */
    bool isInitialized() const;

    /**
     * @brief Nastaví pointer na LCD displej
     * @param lcd Pointer na LCD displej
     */
    void setLCDDisplay(LCDDisplay *lcd);

    /**
     * @brief Vrací aktuálně vybraný příkaz
     * @return Číselný kód aktuálního příkazu
     */
    int getCurrentCommand() const;

    /**
     * @brief Odešle aktuálně vybraný příkaz (simuluje akci EMG2)
     * @return True pokud byl příkaz odeslán úspěšně
     */
    bool sendCurrentCommand();
};

#endif // EMG_SYSTEM_H
