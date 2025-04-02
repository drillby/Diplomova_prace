#include <SPI.h>
#include <WiFiNINA.h>

char ssid[] = "Zizice_doma";
char pass[] = "Pavel_Olinka";

int status = WL_IDLE_STATUS;
WiFiServer server(8888);
WiFiClient client;

void setup()
{
    Serial.begin(9600);
    while (!Serial)
        ;

    if (WiFi.status() == WL_NO_MODULE)
    {
        Serial.println("WiFi modul nebyl nalezen.");
        while (true)
            ;
    }

    while (status != WL_CONNECTED)
    {
        Serial.print("Připojuji se k síti: ");
        Serial.println(ssid);
        status = WiFi.begin(ssid, pass);
        delay(5000);
    }

    Serial.println("Připojeno k WiFi!");
    printWiFiStatus();

    server.begin();
    Serial.println("TCP server běží...");
}

void loop()
{
    if (!client || !client.connected())
    {
        client = server.available();
        if (client)
        {
            Serial.println("Klient připojen");
        }
    }

    if (client && client.connected())
    {
        int fakeEMG = random(0, 1023); // simulace EMG signálu

        if (client.println(fakeEMG) == 0)
        {
            Serial.println("Odeslání selhalo, klient odpojen.");
            client.stop();
        }
        else
        {
            Serial.print("Odesláno klientovi: ");
            Serial.println(fakeEMG);
        }

        delay(100);
    }
}

void printWiFiStatus()
{
    Serial.print("IP adresa: ");
    Serial.println(WiFi.localIP());
}
