#include <SPI.h>
#include <WiFiNINA.h>

// --- Konstanty ---
const char ssid[] = "Zizice_doma";            /// @brief SSID WiFi sítě
const char pass[] = "Pavel_Olinka";           /// @brief Heslo k WiFi síti
const int tcpPort = 8888;                     /// @brief TCP port pro server
const int refreshRateHz = 100;                /// @brief Frekvence aktualizace v Hz
const int refreshRate = 1000 / refreshRateHz; /// @brief Doba mezi aktualizacemi v ms
const int debugPin = 7;                       /// @brief Pin pro aktivaci debug výpisu

const int maxSensors = 4;                         /// @brief Maximální počet podporovaných EMG senzorů
const int emgPins[maxSensors] = {A0, A1, A2, A3}; /// @brief Analogové piny pro senzory

const uint16_t aliveIntervalMs = 1500; /// @brief Interval mezi ALIVE zprávami v ms

/// @brief Vypíše debug zprávu do Serial monitoru, pokud je debugPin v LOW.
/// @param message Zpráva k vypsání
void debugPrint(const char *message)
{
    if (digitalRead(debugPin) == LOW)
    {
        Serial.println(message);
    }
}

/// @brief Třída pro jeden EMG senzor, včetně výpočtu obálky a kalibrace
class EMGSensor
{
private:
    int pin;
    float alpha = 0.4;
    float envelope = 0.0;
    float threshold = 0.2;

public:
    /// @brief Konstruktor EMG senzoru
    /// @param analogPin Analogový pin, ke kterému je senzor připojen
    EMGSensor(int analogPin) : pin(analogPin)
    {
        pinMode(pin, INPUT);
    }

    /// @brief Čtení napětí ze senzoru
    /// @param referenceVoltage Referenční napětí
    /// @param adcResolution Rozlišení ADC převodníku
    /// @return Napětí v rozsahu 0 - referenceVoltage
    float readVoltage(float referenceVoltage = 5.0, int adcResolution = 1023)
    {
        return analogRead(pin) * (referenceVoltage / adcResolution);
    }

    /// @brief Aktualizace obálky signálu pomocí EMA filtru
    /// @param referenceVoltage Referenční napětí
    /// @return Nová hodnota obálky
    float updateEnvelope(float referenceVoltage = 5.0)
    {
        float voltage = readVoltage(referenceVoltage);
        float centered = voltage - (referenceVoltage / 2.0);
        float rectified = fabs(centered);
        envelope = alpha * rectified + (1 - alpha) * envelope;
        return envelope;
    }

    /// @brief Zjištění, zda je aktivita nad prahem
    /// @return True, pokud obálka překročila práh
    bool isActive() const { return envelope > threshold; }

    /// @brief Kalibrace senzoru založená na průměru a směrodatné odchylce
    /// @param durationMs Doba kalibrace v milisekundách
    void calibrate(unsigned long durationMs = 3000)
    {
        const int samplingRate = 100;
        const int sampleInterval = 1000 / samplingRate;
        const int maxSamples = durationMs / sampleInterval;

        float samples[maxSamples];
        int count = 0;

        for (unsigned long tStart = millis(); millis() - tStart < durationMs; delay(sampleInterval))
        {
            updateEnvelope();
            if (count < maxSamples)
            {
                samples[count++] = envelope;
            }
        }

        float sum = 0;
        for (int i = 0; i < count; i++)
            sum += samples[i];
        float mean = sum / count;

        float varSum = 0;
        for (int i = 0; i < count; i++)
            varSum += pow(samples[i] - mean, 2);
        float stdDev = sqrt(varSum / count);

        threshold = mean + 1.5 * stdDev;

        debugPrint("Kalibrace:");

        char buf[50];
        char floatStr[20];

        dtostrf(mean, 7, 4, floatStr); // šířka 7 znaků, 4 desetinná místa
        snprintf(buf, sizeof(buf), "Průměr: %s", floatStr);
        debugPrint(buf);

        dtostrf(stdDev, 7, 4, floatStr);
        snprintf(buf, sizeof(buf), "Směrodatná odchylka: %s", floatStr);
        debugPrint(buf);

        dtostrf(threshold, 7, 4, floatStr);
        snprintf(buf, sizeof(buf), "Nastaven prah: %s", floatStr);
        debugPrint(buf);
    }

    /// @brief Vrací aktuální obálku signálu
    /// @return Poslední hodnota obálky
    float getEnvelope() const
    {
        return envelope;
    }

    /// @brief Nastaví nový práh pro detekci aktivity
    /// @param t Nová hodnota prahu
    void setThreshold(float t)
    {
        threshold = t;
    }
};

/// @brief Třída reprezentující celý systém EMG senzorů a TCP komunikace
class EMGSystem
{
private:
    EMGSensor *sensors[maxSensors];
    int sensorCount;
    WiFiServer server;
    WiFiClient client;
    bool initialized = false;
    long lastAliveTime = 0;
    int lastSentValue = -1;
    int currentWindowValue = 0;
    unsigned long windowStartTime = 0;
    const unsigned long sendDelayMs = 1500;

    /// @brief Přijímá nového klienta a inicializuje senzory podle vstupu
    void handleNewClient()
    {
        client = server.available();
        if (!client)
            return;

        debugPrint("Klient připojen, čekám na počet senzorů...");

        unsigned long connectTime = millis(); // čas připojení
        char input[10] = {0};

        // Čekej max. 3 sekundy na vstup
        while (millis() - connectTime < 3000)
        {
            if (readClientInput(input, sizeof(input)))
            {
                int count = atoi(input);
                if (count >= 1 && count <= maxSensors)
                {
                    initSensors(count);
                    return;
                }
                else
                {
                    debugPrint("Neplatný vstup od klienta.");
                    client.stop();
                    return;
                }
            }
        }

        // Po 3 vteřinách bez vstupu
        debugPrint("Klient neposlal data do 3 vteřin - odpojuji.");
        client.stop();
    }

    /// @brief Načte vstup od klienta do bufferu
    /// @param buffer Výstupní buffer
    /// @param bufferSize Velikost bufferu
    /// @param timeoutMs Timeout pro čekání na vstup
    /// @return True pokud byl vstup úspěšně načten
    bool readClientInput(char *buffer, size_t bufferSize, uint16_t timeoutMs = 3000)
    {
        size_t index = 0;
        unsigned long timeout = millis() + timeoutMs;
        while (millis() < timeout)
        {
            if (client.available())
            {
                char c = client.read();
                if (c == '\n' || index >= bufferSize - 1)
                {
                    buffer[index] = '\0';
                    return true;
                }
                buffer[index++] = c;
            }
        }
        buffer[0] = '\0';
        return false;
    }

    /// @brief Odesílá zakódovaná data klientovi, včetně ALIVE zpráv
    void sendDataToClient()
    {
        if (!client || !client.connected())
        {
            debugPrint("Klient odpojen během odesílání dat.");
            cleanupClient();
            return;
        }

        if (client.available())
        {
            const int bufferSize = 64;
            char inputBuffer[bufferSize] = {0};
            int index = 0;
            while (client.available() && index < bufferSize - 1)
            {
                char c = client.read();
                inputBuffer[index++] = c;
            }
            inputBuffer[index] = '\0';
            if (strstr(inputBuffer, "DISCONNECT") != nullptr)
            {
                debugPrint("Přijato 'DISCONNECT' od klienta.");
                cleanupClient();
                return;
            }
        }

        int sensorData[sensorCount];
        for (int i = 0; i < sensorCount; i++)
        {
            sensors[i]->updateEnvelope();
            sensorData[i] = sensors[i]->isActive() ? 1 : 0;
        }

        int newValue = 0;
        for (int i = 0; i < sensorCount; i++)
        {
            newValue += sensorData[i] << (sensorCount - 1 - i);
        }

        unsigned long now = millis();

        if (windowStartTime == 0)
        {
            windowStartTime = now;
            currentWindowValue = newValue;
        }

        if (newValue > currentWindowValue)
        {
            currentWindowValue = newValue;
        }

        if (now - windowStartTime >= sendDelayMs)
        {
            if (currentWindowValue != lastSentValue)
            {
                lastSentValue = currentWindowValue;
                char msg[20];
                snprintf(msg, sizeof(msg), "%d\n", currentWindowValue);
                client.print(msg);
                debugPrint(msg);
                debugPrint("Data odeslána klientovi.");
            }

            windowStartTime = 0;
            currentWindowValue = 0;
        }

        if (now - lastAliveTime >= aliveIntervalMs)
        {
            client.print("ALIVE\n");
            debugPrint("Odesláno: ALIVE");
            lastAliveTime = now;
        }

        if (!client.connected())
        {
            debugPrint("Klient odpojen po odeslání dat.");
            cleanupClient();
        }
    }

    /// @brief Vyčištění senzorů
    void cleanupSensors()
    {
        for (int i = 0; i < maxSensors; i++)
        {
            if (sensors[i] != nullptr)
            {
                delete sensors[i];
                sensors[i] = nullptr;
            }
        }
        sensorCount = 0;
        initialized = false;
        debugPrint("Senzory byly vyčištěny.");
    }

    /// @brief Ukončení spojení s klientem a reset systému
    void cleanupClient()
    {
        client.stop();
        cleanupSensors();
        lastSentValue = -1;
        debugPrint("Klient odpojen a systém resetován.");
    }

    /// @brief Inicializace senzorů dle zadaného počtu a provedení kalibrace
    /// @param count Počet senzorů
    void initSensors(int count)
    {
        cleanupSensors();

        if (count < 1 || count > maxSensors)
        {
            debugPrint("Neplatný počet senzorů, inicializace selhala.");
            return;
        }

        sensorCount = count;
        for (int i = 0; i < sensorCount; i++)
        {
            sensors[i] = new EMGSensor(emgPins[i]);
        }

        lastAliveTime = millis();

        debugPrint("Spouštím paralelní kalibraci senzorů...");

        const int samplingRate = 100;
        const int sampleInterval = 1000 / samplingRate;
        const int maxSamples = 3000 / sampleInterval;

        float samples[maxSensors][maxSamples] = {0};
        int counts[maxSensors] = {0};
        unsigned long tStart = millis();

        while (millis() - tStart < 3000)
        {
            for (int i = 0; i < sensorCount; i++)
            {
                sensors[i]->updateEnvelope();
                if (counts[i] < maxSamples)
                {
                    samples[i][counts[i]++] = sensors[i]->getEnvelope();
                }
            }
            delay(sampleInterval);
        }

        for (int i = 0; i < sensorCount; i++)
        {
            float sum = 0;
            for (int j = 0; j < counts[i]; j++)
                sum += samples[i][j];
            float mean = sum / counts[i];

            float varSum = 0;
            for (int j = 0; j < counts[i]; j++)
                varSum += pow(samples[i][j] - mean, 2);
            float stdDev = sqrt(varSum / counts[i]);

            sensors[i]->setThreshold(mean + 1.5 * stdDev);

            char buf[80];
            char meanStr[10], stddevStr[10], thresholdStr[10];
            dtostrf(mean, 6, 4, meanStr);
            dtostrf(stdDev, 6, 4, stddevStr);
            dtostrf(mean + 1.5 * stdDev, 6, 4, thresholdStr);

            snprintf(buf, sizeof(buf), "Senzor %d - průměr: %s, směr. odch.: %s, prah: %s",
                     i, meanStr, stddevStr, thresholdStr);
            debugPrint(buf);
        }
        debugPrint("Paralelní kalibrace dokončena.");

        char msg[50];
        snprintf(msg, sizeof(msg), "Inicializováno %d EMG senzor(ů).", sensorCount);
        debugPrint(msg);
        initialized = true;
    }

public:
    /// @brief Konstruktor EMG systému
    /// @param port TCP port pro server
    EMGSystem(int port) : server(port), sensorCount(0) {}

    /// @brief Zahájení WiFi komunikace a spuštění serveru
    void beginWiFi()
    {
        int status = WL_IDLE_STATUS;
        while (status != WL_CONNECTED)
        {
            char debugMessage[60];
            snprintf(debugMessage, sizeof(debugMessage), "Připojování k SSID: %s", ssid);
            debugPrint(debugMessage);
            status = WiFi.begin(ssid, pass);
            delay(5000);
        }
        debugPrint("WiFi připojeno");
        server.begin();
    }

    /// @brief Hlavní smyčka pro obsluhu EMG dat a komunikace
    void update()
    {
        static bool wifiDisconnectedPrinted = false;
        static bool noClientPrinted = false;
        static bool notInitializedPrinted = false;

        if (WiFi.status() != WL_CONNECTED)
        {
            if (!wifiDisconnectedPrinted)
            {
                debugPrint("WiFi odpojeno, připojuji znovu...");
                wifiDisconnectedPrinted = true;
            }
            beginWiFi();
            return;
        }
        else
        {
            wifiDisconnectedPrinted = false;
        }

        if (!client || !client.connected())
        {
            if (!noClientPrinted)
            {
                debugPrint("Žádný klient není připojen.");
                noClientPrinted = true;
            }
            handleNewClient();
            return;
        }
        else
        {
            noClientPrinted = false;
        }

        if (!initialized)
        {
            if (!notInitializedPrinted)
            {
                debugPrint("Senzory nejsou inicializovány.");
                notInitializedPrinted = true;
            }
            return;
        }
        else
        {
            notInitializedPrinted = false;
        }

        sendDataToClient();
    }
};

EMGSystem emgSystem(tcpPort);

/// @brief Inicializace systému při startu
void setup()
{
    pinMode(debugPin, INPUT_PULLUP);
    Serial.begin(9600);
    while (!Serial)
        ;
    debugPrint("Start systému...");
    emgSystem.beginWiFi();

    IPAddress ip = WiFi.localIP();
    char ipStr[16];
    snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    char msg[40];
    snprintf(msg, sizeof(msg), "IP adresa: %s", ipStr);
    debugPrint(msg);
    debugPrint("Systém připraven, čekám na klienta...");
}

/// @brief Hlavní smyčka programu
void loop()
{
    int timeStart = millis();
    emgSystem.update();
    int timeEnd = millis();
    delay(max(0, refreshRate - (timeEnd - timeStart)));
}
