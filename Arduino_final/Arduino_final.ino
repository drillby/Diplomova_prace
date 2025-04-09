#include <SPI.h>
#include <WiFiNINA.h>

// --- Konstanty ---
const char ssid[] = "Zizice_doma";
const char pass[] = "Pavel_Olinka";
const int tcpPort = 8888;
const int refreshRateHz = 100;
const int refreshRate = 1000 / refreshRateHz;
const int debugPin = 7;

const int maxSensors = 4;
const int emgPins[maxSensors] = {A0, A1, A2, A3};

/// @brief Vypíše debug zprávu do Serial monitoru, pokud je debugPin v LOW.
/// @param message Zpráva k vypsání.
void debugPrint(const char *message)
{
    if (digitalRead(debugPin) == LOW)
    {
        Serial.println(message);
    }
}

/// @brief Třída reprezentující jeden EMG senzor.
class EMGSensor
{
private:
    int pin;

public:
    EMGSensor(int analogPin) : pin(analogPin)
    {
        pinMode(pin, INPUT);
    }

    float readVoltage(float referenceVoltage = 5.0, int adcResolution = 1023)
    {
        return analogRead(pin) * (referenceVoltage / adcResolution);
    }
};

/// @brief Třída pro správu všech EMG senzorů a TCP komunikace.
class EMGSystem
{
private:
    EMGSensor *sensors[maxSensors];
    int sensorCount;
    WiFiServer server;
    WiFiClient client;
    bool initialized = false;
    long lastTimeSent = 0;
    int lastSentValue = -1;

    /// @brief Handles a new client connection and initializes sensors based on client input.
    void handleNewClient()
    {
        client = server.available();
        if (!client)
            return;

        debugPrint("Klient připojen, čekám na počet senzorů...");

        char input[10] = {0};
        if (readClientInput(input, sizeof(input)))
        {
            int count = atoi(input);
            if (count >= 1 && count <= maxSensors)
            {
                initSensors(count);
            }
            else
            {
                debugPrint("Neplatný vstup od klienta.");
                client.stop();
            }
        }
        else
        {
            debugPrint("Časový limit pro vstup klienta vypršel.");
            client.stop();
        }
    }

    /// @brief Reads input from the client with a timeout.
    /// @param buffer The buffer to store the input.
    /// @param bufferSize The size of the buffer.
    /// @return True if input was successfully read, false otherwise.
    bool readClientInput(char *buffer, size_t bufferSize)
    {
        size_t index = 0;
        unsigned long timeout = millis() + 3000;
        while (millis() < timeout)
        {
            while (client.available())
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

    /// @brief Sends sensor data to the connected client.
    void sendDataToClient()
    {
        if (!client || !client.connected())
        {
            debugPrint("Klient odpojen během odesílání dat.");
            cleanupClient();
            return;
        }

        // Check for "DISCONNECT" command from the client
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
            inputBuffer[index] = '\0'; // ukonči string

            if (strstr(inputBuffer, "DISCONNECT") != nullptr)
            {
                debugPrint("Přijato 'DISCONNECT' od klienta.");
                cleanupClient();
                return;
            }
        }

        // save sensors data to array
        int sensorData[sensorCount];
        for (int i = 0; i < sensorCount; i++)
        {
            sensorData[i] = sensors[i]->readVoltage() > 1.6 ? 1 : 0;
        }
        // convert from binary to decimal
        int decimalValue = 0;
        for (int i = 0; i < sensorCount; i++)
        {
            decimalValue += sensorData[i] << (sensorCount - 1 - i);
        }

        // pošle pouze pokud se změnila hodnota
        if (decimalValue != lastSentValue)
        {
            lastSentValue = decimalValue;

            char msg[20];
            snprintf(msg, sizeof(msg), "%d\n", decimalValue);
            client.print(msg);
            debugPrint(msg);
        }

        // check if client is still connected
        if (!client.connected())
        {
            debugPrint("Klient odpojen po odeslání dat.");
            cleanupClient();
        }
        else
        {
            debugPrint("Data odeslána klientovi.");
        }
    }

    /// @brief Cleans up all sensors and resets the system state.
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

    /// @brief Cleans up client connection and resets the system state.
    void cleanupClient()
    {
        client.stop();
        cleanupSensors();
        lastSentValue = -1;
        debugPrint("Klient odpojen a systém resetován.");
    }

    /// @brief Inicializuje senzory podle přijatého počtu.
    void initSensors(int count)
    {
        cleanupSensors(); // Ensure no leftover sensors from previous initialization

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

        char msg[50];
        snprintf(msg, sizeof(msg), "Inicializováno %d EMG senzor(ů).", sensorCount);
        debugPrint(msg);
        initialized = true;
    }

public:
    /// @brief Constructs an EMGSystem object.
    /// @param port The TCP port for the WiFi server.
    EMGSystem(int port) : server(port), sensorCount(0) {}

    /// @brief Připojí se k WiFi síti.
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

    /// @brief Aktualizuje komunikaci s klientem – inicializace + odesílání dat.
    void update()
    {
        if (WiFi.status() != WL_CONNECTED)
        {
            debugPrint("WiFi odpojeno, připojuji znovu...");
            beginWiFi();
            return;
        }

        if (!client || !client.connected())
        {
            debugPrint("Žádný klient není připojen.");
            handleNewClient();
            return;
        }

        if (!initialized)
        {
            debugPrint("Senzory nejsou inicializovány.");
            return;
        }

        long currentTime = millis();
        if (currentTime - lastTimeSent >= 1000)
        {
            lastTimeSent = currentTime;
            sendDataToClient();
        }
    }
};

EMGSystem emgSystem(tcpPort);

void setup()
{
    pinMode(debugPin, INPUT_PULLUP);
    Serial.begin(9600);
    while (!Serial)
        ;
    debugPrint("Start systému...");
    emgSystem.beginWiFi();
}

void loop()
{
    int timeStart = millis();
    emgSystem.update();
    int timeEnd = millis();
    delay(max(0, refreshRate - (timeEnd - timeStart)));
}
