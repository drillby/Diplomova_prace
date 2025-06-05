#include <SPI.h>
#include <WiFiNINA.h>

/**
 * @brief WiFi konfigurace a TCP port
 */
const char ssid[] = "Zizice_doma";  // SSID WiFi sítě
const char pass[] = "Pavel_Olinka"; // Heslo k WiFi síti
const int tcpPort = 8888;           // TCP port pro server

/**
 * @brief Parametry systému a pinů
 */
const int refreshRateHz = 1000;                   // Frekvence aktualizace v Hz
const int refreshRate = 1000 / refreshRateHz;     // Perioda smyčky v ms
const int debugPin = 7;                           // Pin pro výpis debug informací
const int serialPrintPin = 6;                     // Pin pro výpis dat přes Serial
const int serialBaudRate = 9600;                  // Baud rate pro Serial
const int maxSensors = 4;                         // Maximální počet podporovaných senzorů
const int emgPins[maxSensors] = {A0, A1, A2, A3}; // Analogové piny EMG senzorů
const uint16_t aliveIntervalMs = 2000;            // Interval mezi "ALIVE" zprávami

/**
 * @struct CommandEntry
 * @brief Struktura příkazu s kódem a textovým popisem
 */
struct CommandEntry
{
    const uint8_t code; // Číselný kód příkazu
    const char *label;  // Textový popis příkazu
};

/**
 * @brief Tabulka dostupných příkazů
 */
const CommandEntry commandTable[] = {
    {1, "LEFT"},
    {2, "RIGHT"},
    {3, "UP"},
    {4, "DOWN"},
    {5, "FORWARD"},
    {6, "BACKWARD"},
    {7, "OPEN"},
    {8, "CLOSE"}};

/**
 * @brief Vrátí textový název příkazu na základě jeho kódu
 * @param code Číselný kód příkazu
 * @return Textový popis nebo "UNKNOWN" při neznámém kódu
 */
const char *getCommandLabel(uint8_t code)
{
    for (const auto &entry : commandTable)
    {
        if (entry.code == code)
            return entry.label;
    }
    return "UNKNOWN";
}

/**
 * @brief Vytiskne zprávu přes Serial pokud je daný pin LOW
 * @param message Zpráva k výpisu
 * @param pin Pin, který musí být LOW
 */
void printIfPinLow(const char *message, int pin)
{
    if (digitalRead(pin) == LOW)
    {
        Serial.println(message);
    }
}

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
    EMGSensor(int analogPin) : pin(analogPin)
    {
        pinMode(pin, INPUT);
    }

    /**
     * @brief Načte napětí ze senzoru
     * @param referenceVoltage Referenční napětí (default 5.0V)
     * @param adcResolution Rozlišení ADC (default 1023)
     * @return Naměřené napětí
     */
    float readVoltage(float referenceVoltage = 5.0, int adcResolution = 1023)
    {
        return analogRead(pin) * (referenceVoltage / adcResolution);
    }

    /**
     * @brief Aktualizuje obálku signálu
     * @param referenceVoltage Referenční napětí (default 5.0V)
     * @return Nová hodnota obálky
     */
    float updateEnvelope(float referenceVoltage = 5.0)
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
    bool isActive() const { return envelope > thresholdUpper; }

    /**
     * @brief Kalibruje senzor po zadanou dobu
     * @param durationMs Doba kalibrace v ms (default 3000)
     */
    void calibrate(unsigned long durationMs = 3000)
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

        printIfPinLow("Kalibrace:", debugPin);

        String msg;

        msg = "Průměr: " + String(mean, 4);
        printIfPinLow(msg.c_str(), debugPin);

        msg = "Směrodatná odchylka: " + String(stdDev, 4);
        printIfPinLow(msg.c_str(), debugPin);

        msg = "Nastaven prah upper: " + String(thresholdUpper, 4);
        printIfPinLow(msg.c_str(), debugPin);

        msg = "Nastaven prah lower: " + String(thresholdLower, 4);
        printIfPinLow(msg.c_str(), debugPin);
    }

    /**
     * @brief Vrací aktuální hodnotu obálky
     * @return Hodnota obálky
     */
    float getEnvelope() const { return envelope; }
};

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

    /**
     * @brief Zpracuje zprávy od klienta
     */
    void handleClientMessages()
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
    void sendAliveIfNeeded()
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
    void handleLogic()
    {
        static unsigned long zeroSendTime = 0;
        static bool sendZeroPending = false;
        static const unsigned int sendZeroDelay = 200; // Cooldown pro odeslání "0"

        sensors[0]->updateEnvelope();
        sensors[1]->updateEnvelope();

        bool emg1Active = sensors[0]->isActive();
        bool emg2Active = sensors[1]->isActive();
        unsigned long now = millis();

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

            zeroSendTime = millis() + sendZeroDelay;

            sendZeroPending = true;

            if (sendZeroPending && millis() >= zeroSendTime)
            {
                client.print("0\n");
                printIfPinLow("0", debugPin);
                sendZeroPending = false;
            }
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
    void calibrateSensors()
    {
        for (int i = 0; i < 2; i++)
            sensors[i]->calibrate();
        initialized = true;
    }

    /**
     * @brief Inicializuje senzory
     */
    void initSensors()
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
    void handleNewClient()
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
    void cleanupSensors()
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
    void cleanupClient()
    {
        client.stop();
        cleanupSensors();
        printIfPinLow("Klient odpojen a systém resetován.", debugPin);
        wasClientConnected = false;
    }

public:
    /**
     * @brief Konstruktor EMGSystemu
     * @param port TCP port serveru
     */
    EMGSystem(int port) : server(port) {}

    /**
     * @brief Inicializuje WiFi připojení a server
     */
    void beginWiFi()
    {
        int status = WL_IDLE_STATUS;
        while (status != WL_CONNECTED)
        {
            String debugMessage = String("Připojování k SSID: ") + ssid;
            printIfPinLow(debugMessage.c_str(), debugPin);
            status = WiFi.begin(ssid, pass);
            delay(5000);
        }
        printIfPinLow("WiFi připojeno", debugPin);
        server.begin();
    }

    /**
     * @brief Hlavní aktualizační metoda systému (volat v loop)
     */
    void update()
    {
        static bool wifiDisconnectedPrinted = false;
        static bool noClientPrinted = false;
        static bool notInitializedPrinted = false;

        if (WiFi.status() != WL_CONNECTED)
        {
            if (!wifiDisconnectedPrinted)
            {
                printIfPinLow("WiFi odpojeno, připojuji znovu...", debugPin);
                wifiDisconnectedPrinted = true;
            }
            beginWiFi();
            return;
        }
        else
            wifiDisconnectedPrinted = false;

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
};

/**
 * @brief Globální instance EMG systému
 */
EMGSystem emgSystem(tcpPort);

/**
 * @brief Inicializační funkce Arduino
 */
void setup()
{
    pinMode(debugPin, INPUT_PULLUP);
    pinMode(serialPrintPin, INPUT_PULLUP);
    Serial.begin(serialBaudRate);
    while (!Serial)
        ;

    printIfPinLow("Start systému...", debugPin);
    emgSystem.beginWiFi();

    IPAddress ip = WiFi.localIP();
    String ipStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    String msg = "IP adresa: " + ipStr;
    Serial.println(msg);
    printIfPinLow("Systém připraven, čekám na klienta...", debugPin);

    while (digitalRead(debugPin) == LOW && digitalRead(serialPrintPin) == LOW)
    {
        Serial.println("Pro správné zasílání dat přes serial je potřeba mít zapojen debug pin (pin 7), nebo serial print pin (pin 6). NE OBOJÍ!");
        delay(1000);
    }
}

/**
 * @brief Hlavní smyčka Arduino
 */
void loop()
{
    int timeStart = millis();
    emgSystem.update();
    int timeEnd = millis();
    delay(max(0, refreshRate - (timeEnd - timeStart)));
}