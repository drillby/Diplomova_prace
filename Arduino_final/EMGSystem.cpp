#include "EMGSystem.h"
#include "Config.h"
#include "Utils.h"
#include "CommandTable.h"

/**
 * @brief Konstruktor EMGSystemu
 * @param port TCP port serveru
 */
EMGSystem::EMGSystem(int port) : server(port) {}

/**
 * @brief Spustí TCP server pro EMG systém
 */
void EMGSystem::beginServer()
{
    server.begin();
    printIfPinLow("EMG TCP server spuštěn", debugPin);
}

/**
 * @brief Zpracuje zprávy od klienta
 */
void EMGSystem::handleClientMessages()
{
    if (client.available())
    {
        String message = client.readStringUntil('\n');
        message.trim();
        message.toUpperCase();

        if (message == "DISCONNECT")
        {
            printIfPinLow("DISCONNECT příkaz přijat. Ukončuji spojení...", debugPin);
            cleanupClient();
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
        client.print("ALIVE\n");
        printIfPinLow("Odesláno: ALIVE", debugPin);
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
    if (sendZeroPending && now >= zeroSendTime)
    {
        client.print("0\n");
        printIfPinLow("0", debugPin);
        sendZeroPending = false;
    }

    if (emg1Active && !emg1LastActive && (now - lastCycleTime >= cooldown))
    {
        cycledValue++;
        if (cycledValue > sizeof(commandTable) / sizeof(CommandEntry))
        {
            cycledValue = 1;
        }

        lastCycleTime = now;

        String msg = "Navoleno: " + String(cycledValue) + " (" + String(getCommandLabel(cycledValue)) + ")";
        printIfPinLow(msg.c_str(), debugPin);
    }

    if (emg2Active && !emg2LastActive && (now - lastSendTime >= cooldown))
    {
        if (cycledValue == 0)
        {
            printIfPinLow("Žádný příkaz nenavolen.", debugPin);
            return;
        }
        String msg = String(cycledValue) + "\n";
        client.print(msg);
        printIfPinLow(msg.c_str(), debugPin);
        lastSendTime = now;

        // Schedule "0" to be sent after 0.5 seconds
        zeroSendTime = now + sendZeroDelay;
        sendZeroPending = true;
    }

    emg1LastActive = emg1Active;
    emg2LastActive = emg2Active;

    if (digitalRead(serialPrintPin) == LOW)
    {
        String serialMsg = String(sensors[0]->getEnvelope(), 4) + "," + String(sensors[1]->getEnvelope(), 4);
        printIfPinLow(serialMsg.c_str(), serialPrintPin);
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
    printIfPinLow("Systém inicializován pro 2 EMG senzory.", debugPin);
}

/**
 * @brief Zpracuje nového klienta
 */
void EMGSystem::handleNewClient()
{
    client = server.available();
    if (!client)
        return;
    printIfPinLow("Klient připojen - inicializuji senzory", debugPin);
    initSensors();
    wasClientConnected = true;
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
    printIfPinLow("Klient odpojen a systém resetován.", debugPin);
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
            printIfPinLow("Klient ztratil spojení.", debugPin);
            cleanupClient();
        }

        if (!noClientPrinted)
        {
            printIfPinLow("Žádný klient není připojen.", debugPin);
            noClientPrinted = true;
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
            printIfPinLow("Senzory nejsou inicializovány.", debugPin);
            notInitializedPrinted = true;
        }
        return;
    }
    else
        notInitializedPrinted = false;

    handleLogic();
    handleClientMessages();
    sendAliveIfNeeded();
}

/**
 * @brief Vrací příznak inicializace EMG systému
 */
bool EMGSystem::isInitialized() const
{
    return initialized;
}
