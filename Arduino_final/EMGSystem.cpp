#include "EMGSystem.h"
#include "Config.h"
#include "Utils.h"
#include "CommandTable.h"

/**
 * @brief Konstruktor EMGSystemu
 * @param port TCP port serveru
 */
EMGSystem::EMGSystem(int port) : server(port), lcdDisplay(nullptr) {}

/**
 * @brief Spustí TCP server pro EMG systém
 */
void EMGSystem::beginServer()
{
    server.begin();
    printIfPinLow(F("EMG TCP server spuštěn"), debugPin);
}

/**
 * @brief Zpracuje zprávy od klienta
 */
void EMGSystem::handleClientMessages()
{
    if (client.available())
    {
        char message[64];
        int msgLen = client.readBytesUntil('\n', message, sizeof(message) - 1);
        if (msgLen > 0)
        {
            message[msgLen] = '\0';

            // Trim whitespace and convert to uppercase
            while (msgLen > 0 && (message[msgLen - 1] == ' ' || message[msgLen - 1] == '\r'))
            {
                message[--msgLen] = '\0';
            }

            // Simple uppercase conversion for ASCII
            for (int i = 0; i < msgLen; i++)
            {
                if (message[i] >= 'a' && message[i] <= 'z')
                    message[i] = message[i] - 'a' + 'A';
            }

            if (strcmp(message, "DISCONNECT") == 0)
            {
                printIfPinLow(F("DISCONNECT příkaz přijat. Ukončuji spojení..."), debugPin);
                cleanupClient();
            }
        }
    }
}

/**
 * @brief Odesílá ALIVE zprávu v nastaveném intervalu
 */
void EMGSystem::sendAliveIfNeeded()
{
    unsigned long now = millis();
    if (now - lastAliveTime >= aliveIntervalMs)
    {
        client.print(F("ALIVE\n"));
        printIfPinLow(F("Odesláno: ALIVE"), debugPin);
        lastAliveTime = now;
    }
}

/**
 * @brief Hlavní logika zpracování EMG signálů a komunikace
 */
void EMGSystem::handleLogic()
{
    static unsigned long zeroSendTime = 0;
    static bool sendZeroPending = false;
    static const unsigned int sendZeroDelay = 500; // 0.5 sekund pro odeslání "0"

    sensors[0]->updateEnvelope();
    sensors[1]->updateEnvelope();

    bool emg1Active = sensors[0]->isActive();
    bool emg2Active = sensors[1]->isActive();
    unsigned long now = millis();

    // Check if we need to send "0" (non-blocking, checked every loop iteration)
    /*
    if (sendZeroPending && now >= zeroSendTime)
    {
        client.print(F("0\n"));
        printIfPinLow(F("0"), debugPin);
        sendZeroPending = false;
    }
    */

    if (emg1Active && !emg1LastActive && (now - lastCycleTime >= cooldown))
    {
        cycledValue++;
        if (cycledValue >= sizeof(commandTable) / sizeof(CommandEntry))
        {
            // cycledValue = 1;
            cycledValue = 0;
        }

        lastCycleTime = now;

        // printIfPinLow(F("Command selected"), debugPin);
        printIfPinLow(F("Aktuální příkaz:"), debugPin);
        char msg[32];
        snprintf(msg, sizeof(msg), "%d - %s", cycledValue, getCommandLabel(cycledValue));
        printIfPinLow(msg, debugPin);
        const char *commandLabel = getCommandLabel(cycledValue);

        // Update LCD with selected command
        if (lcdDisplay && lcdDisplay->isReady() && client && client.connected())
        {
            lcdDisplay->clear();
            char commandStr[32];
            snprintf(commandStr, sizeof(commandStr), "Prikaz %d", cycledValue);
            lcdDisplay->printAt(0, 0, commandStr);

            // Show first 16 characters of command label
            char commandLabel[17];
            const char *fullLabel = getCommandLabel(cycledValue);
            strncpy(commandLabel, fullLabel, 16);
            commandLabel[16] = '\0';
            lcdDisplay->printAt(0, 1, commandLabel);
        }
    }

    if (emg2Active && !emg2LastActive && (now - lastSendTime >= cooldown))
    {
        /*
        if (cycledValue == 0)
        {
            printIfPinLow(F("Žádný příkaz nenavolen."), debugPin);
            return;
        }
        */
        char msg[8];
        snprintf(msg, sizeof(msg), "%d\n", cycledValue);
        client.print(msg);
        printIfPinLow(msg, debugPin);
        lastSendTime = now;
        cycledValue = 0; // po odeslání příkazu chceme mít možnost hned zastavit chod robota

        // Schedule "0" to be sent after 0.5 seconds
        /*
        zeroSendTime = now + sendZeroDelay;
        sendZeroPending = true;
        */
    }

    emg1LastActive = emg1Active;
    emg2LastActive = emg2Active;

    if (digitalRead(serialPrintPin) == LOW)
    {
        char serialMsg[24];
        snprintf(serialMsg, sizeof(serialMsg), "%.4f,%.4f", sensors[0]->getEnvelope(), sensors[1]->getEnvelope());
        printIfPinLow(serialMsg, serialPrintPin);
    }
}

/**
 * @brief Kalibruje oba senzory
 */
void EMGSystem::calibrateSensors()
{
    for (int i = 0; i < 2; i++)
        sensors[i]->calibrate();
    initialized = true;
}

/**
 * @brief Inicializuje senzory
 */
void EMGSystem::initSensors()
{
    cleanupSensors();
    for (int i = 0; i < 2; i++)
        sensors[i] = new EMGSensor(emgPins[i]);
    calibrateSensors();
    cycledValue = 1;
    printIfPinLow(F("Systém inicializován pro 2 EMG senzory."), debugPin);
}

/**
 * @brief Zpracuje nového klienta
 */
void EMGSystem::handleNewClient()
{
    client = server.available();
    if (!client)
        return;
    printIfPinLow(F("Klient připojen - inicializuji senzory"), debugPin);

    // Update LCD with client connected
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->clear();
        lcdDisplay->printAt(0, 0, F("Klient pripojen"));
        lcdDisplay->printAt(0, 1, F("Kalibrace..."));
    }

    initSensors();
    wasClientConnected = true;

    // After calibration, show initial command
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->clear();
        char commandStr[32];
        snprintf(commandStr, sizeof(commandStr), "Prikaz %d", cycledValue);
        lcdDisplay->printAt(0, 0, commandStr);

        // Show first 16 characters of command label
        char commandLabel[17];
        const char *fullLabel = getCommandLabel(cycledValue);
        strncpy(commandLabel, fullLabel, 16);
        commandLabel[16] = '\0';
        lcdDisplay->printAt(0, 1, commandLabel);
    }
}

/**
 * @brief Uvolní paměť senzorů
 */
void EMGSystem::cleanupSensors()
{
    for (int i = 0; i < 2; i++)
    {
        if (sensors[i] != nullptr)
        {
            delete sensors[i];
            sensors[i] = nullptr;
        }
    }
    initialized = false;
}

/**
 * @brief Odpojí klienta a resetuje systém
 */
void EMGSystem::cleanupClient()
{
    client.stop();
    cleanupSensors();
    printIfPinLow(F("Klient odpojen a systém resetován."), debugPin);
    wasClientConnected = false;
}

/**
 * @brief Hlavní aktualizační metoda systému (volat v loop)
 */
void EMGSystem::update()
{
    static bool noClientPrinted = false;
    static bool notInitializedPrinted = false;

    if (!client || !client.connected())
    {
        if (wasClientConnected)
        {
            printIfPinLow(F("Klient ztratil spojení."), debugPin);
            cleanupClient();

            // Update LCD when client disconnects
            if (lcdDisplay && lcdDisplay->isReady())
            {
                lcdDisplay->clear();
                lcdDisplay->printAt(0, 0, F("Cekam na klienta"));
                IPAddress ip = WiFi.localIP();
                char ipStr[17];
                snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                lcdDisplay->printAt(0, 1, ipStr);
            }
        }

        if (!noClientPrinted)
        {
            printIfPinLow(F("Žádný klient není připojen."), debugPin);
            noClientPrinted = true;

            // Show server IP when waiting for client
            if (lcdDisplay && lcdDisplay->isReady())
            {
                lcdDisplay->clear();
                lcdDisplay->printAt(0, 0, F("Cekam na klienta"));
                IPAddress ip = WiFi.localIP();
                char ipStr[17];
                snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
                lcdDisplay->printAt(0, 1, ipStr);
            }
        }

        handleNewClient();
        return;
    }
    else
        noClientPrinted = false;

    if (!initialized)
    {
        if (!notInitializedPrinted)
        {
            printIfPinLow(F("Senzory nejsou inicializovány."), debugPin);
            notInitializedPrinted = true;
        }
        return;
    }
    else
        notInitializedPrinted = false;

    handleLogic();
    handleClientMessages();
    // sendAliveIfNeeded();
}

/**
 * @brief Vrací příznak inicializace EMG systému
 */
bool EMGSystem::isInitialized() const
{
    return initialized;
}

/**
 * @brief Nastaví pointer na LCD displej
 * @param lcd Pointer na LCD displej
 */
void EMGSystem::setLCDDisplay(LCDDisplay *lcd)
{
    lcdDisplay = lcd;
}

/**
 * @brief Vrací aktuálně vybraný příkaz
 * @return Číselný kód aktuálního příkazu
 */
int EMGSystem::getCurrentCommand() const
{
    return cycledValue;
}

/**
 * @brief Odešle aktuálně vybraný příkaz (simuluje akci EMG2)
 * @return True pokud byl příkaz odeslán úspěšně
 */
bool EMGSystem::sendCurrentCommand()
{
    if (!initialized || !client || !client.connected() || cycledValue == 0)
    {
        return false;
    }

    unsigned long now = millis();
    if (now - lastSendTime < cooldown)
    {
        return false; // Still in cooldown period
    }

    // Send the command
    char msg[8];
    snprintf(msg, sizeof(msg), "%d\n", cycledValue);
    client.print(msg);
    printIfPinLow(F("API: Command sent to TCP client"), debugPin);
    printIfPinLow(msg, debugPin);
    lastSendTime = now;

    // Schedule "0" to be sent after 0.5 seconds (same as EMG2 behavior)
    static unsigned long zeroSendTime = now + 500;
    // Note: The zero sending logic is handled in handleLogic(),
    // but we need to ensure it gets triggered

    return true;
}
