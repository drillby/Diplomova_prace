#include "WiFiConfigSystem.h"
#include "Config.h"
#include "Utils.h"
#include "EEPROMManager.h"

/**
 * @brief Konstruktor WiFiConfigSystem
 * @param emgSys Reference na EMG systém
 * @param lcd Pointer na LCD displej (volitelný)
 */
WiFiConfigSystem::WiFiConfigSystem(EMGSystem &emgSys, LCDDisplay *lcd) : server(httpPort), isAPMode(false), initialized(false), emgSystem(emgSys), lcdDisplay(lcd)
{
    // Initialize char arrays
    wifiSSID[0] = '\0';
    wifiPass[0] = '\0';
}

/**
 * @brief Pokusí se připojit k WiFi síti
 * @return True pokud se připojení podařilo
 */
bool WiFiConfigSystem::connectToWiFi()
{
    if (strlen(wifiSSID) == 0 || strlen(wifiPass) == 0)
        return false;

    printIfPinLow(F("Zkouším připojení k WiFi..."), debugPin);
    printIfPinLow(wifiSSID, debugPin);

    printIfPinLow(F("Pass length set"), debugPin);

    // Update LCD
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->clear();
        lcdDisplay->printAt(0, 0, F("Pripojovani WiFi"));
        // Show first 16 characters of SSID
        char ssidDisplay[17];
        strncpy(ssidDisplay, wifiSSID, 16);
        ssidDisplay[16] = '\0';
        lcdDisplay->printAt(0, 1, ssidDisplay);
    }

    // Disconnect any previous connection
    WiFi.disconnect();
    delay(100);

    // Začni připojení
    WiFi.begin(wifiSSID, wifiPass);

    // Čekej na připojení (max wifiTimeoutMs)
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < wifiTimeoutMs)
    {
        delay(500);
        Serial.print(".");
        Serial.print(WiFi.status());
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        printIfPinLow(F("WiFi připojeno!"), debugPin);
        IPAddress ip = WiFi.localIP();
        char ipStr[17];
        snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        printIfPinLow(F("IP získána:"), debugPin);
        printIfPinLow(ipStr, debugPin);
        Serial.println(ipStr);

        printIfPinLow(F("Signál dobrý"), debugPin);

        // Update LCD with success
        if (lcdDisplay && lcdDisplay->isReady())
        {
            lcdDisplay->clear();
            lcdDisplay->printAt(0, 0, F("WiFi pripojeno!"));
            char ipStr[17];
            snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
            lcdDisplay->printAt(0, 1, ipStr);
            delay(2000);
        }

        return true;
    }

    printIfPinLow(F("WiFi připojení selhalo"), debugPin);
    return false;
}

/**
 * @brief Spustí Access Point režim
 * @return True pokud se AP spustil úspěšně
 */
bool WiFiConfigSystem::startAccessPoint()
{
    printIfPinLow(F("Spouštím Access Point..."), debugPin);

    // Update LCD
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->clear();
        lcdDisplay->printAt(0, 0, F("Access Point"));
        lcdDisplay->printAt(0, 1, F("Spousteni..."));
    }

    if (WiFi.beginAP(apSSID, apPass) != WL_AP_LISTENING)
    {
        printIfPinLow(F("Chyba při spouštění AP"), debugPin);

        // Update LCD with error
        if (lcdDisplay && lcdDisplay->isReady())
        {
            lcdDisplay->clear();
            lcdDisplay->printAt(0, 0, F("Chyba AP!"));
            delay(2000);
        }

        return false;
    }

    delay(apStabilizationMs); // nutné pro stabilizaci

    IPAddress ip = WiFi.localIP();
    printIfPinLow(F("AP IP získána"), debugPin);

    // Update LCD with AP info
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->clear();
        lcdDisplay->printAt(0, 0, F("AP: EMG_Config"));
        char ipStr[17];
        snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
        lcdDisplay->printAt(0, 1, ipStr);
    }

    // Spusť web server pouze v AP režimu
    server.begin();
    printIfPinLow(F("Web server spuštěn v AP režimu"), debugPin);
    return true;
}

/**
 * @brief Zpracuje HTTP požadavek s WiFi údaji
 * @param reqLine Řádek HTTP požadavku
 * @param client WiFi klient pro odpověď
 * @return True pokud byl požadavek zpracován
 */
bool WiFiConfigSystem::handleWiFiConfig(const char *reqLine, WiFiClient &client)
{
    // Find positions of parameters in the request line
    const char *ssidStart = strstr(reqLine, "input1=");
    const char *passStart = strstr(reqLine, "&input2=");
    const char *passEnd = strchr(passStart ? passStart : reqLine, ' ');

    if (ssidStart && passStart && passEnd && ssidStart < passStart)
    {
        // Extract SSID
        ssidStart += 7; // Skip "input1="
        int ssidLen = passStart - ssidStart;
        if (ssidLen >= sizeof(wifiSSID))
            ssidLen = sizeof(wifiSSID) - 1;

        char tempSSID[32];
        strncpy(tempSSID, ssidStart, ssidLen);
        tempSSID[ssidLen] = '\0';

        // Extract password
        passStart += 8; // Skip "&input2="
        int passLen = passEnd - passStart;
        if (passLen >= sizeof(wifiPass))
            passLen = sizeof(wifiPass) - 1;

        char tempPass[32];
        strncpy(tempPass, passStart, passLen);
        tempPass[passLen] = '\0';

        // URL decode
        urlDecode(tempSSID, wifiSSID, sizeof(wifiSSID));
        urlDecode(tempPass, wifiPass, sizeof(wifiPass));

        printIfPinLow(F("Decoded values:"), debugPin);
        printIfPinLow(wifiSSID, debugPin);

        printIfPinLow(F("Pass decoded"), debugPin);

        EEPROMManager::writeString(EEPROM_ADDR_WIFISSID, wifiSSID);
        EEPROMManager::writeString(EEPROM_ADDR_WIFIPASS, wifiPass);

        printIfPinLow(F("Uloženo do EEPROM:"), debugPin);
        printIfPinLow(wifiSSID, debugPin);

        sendSuccessPage(client);
        return true;
    }
    return false;
}

/**
 * @brief Odešle HTML stránku s potvrzením úspěšného uložení
 * @param client WiFi klient pro odpověď
 */
void WiFiConfigSystem::sendSuccessPage(WiFiClient &client)
{
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: text/html; charset=UTF-8"));
    client.println(F("Connection: close"));
    client.println();
    client.println(F("<!DOCTYPE html><html><head>"));
    client.println(F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"));
    client.println(F("<title>EMG - Uloženo</title>"));
    client.println(F("<style>body{font-family:Arial,sans-serif;margin:20px;background:#667eea;color:#333}"));
    client.println(F(".box{max-width:500px;margin:50px auto;background:white;border-radius:10px;padding:30px;text-align:center}"));
    client.println(F("h1{color:#27ae60;margin-bottom:20px}.success{font-size:60px;color:#27ae60;margin:20px 0}"));
    client.println(F("button{background:#3498db;color:white;padding:12px 25px;border:none;border-radius:5px;font-size:14px;cursor:pointer;margin:8px}"));
    client.println(F("button:hover{background:#2980b9}.btn-sec{background:#95a5a6}.btn-sec:hover{background:#7f8c8d}"));
    client.println(F(".info{background:#e8f4f8;padding:15px;border-radius:8px;margin:20px 0;text-align:left}"));
    client.println(F("</style></head><body><div class='box'>"));
    client.println(F("<div class='success'>✅</div><h1>Konfigurace uložena!</h1>"));
    client.println(F("<p>WiFi údaje byly uloženy do EEPROM.</p>"));

    client.println(F("<div class='info'><b>SSID:</b> "));
    client.print(wifiSSID);
    client.println(F("<br><b>Heslo:</b> Uloženo</div>"));

    client.println(F("<p>Arduino se pokusí připojit k WiFi. Při neúspěchu se spustí AP režim.</p>"));
    client.println(F("<p><b>Restart za 5 sekund...</b></p>"));

    client.println(F("<button onclick=\"location.href='/restart'\">🔄 Restart</button>"));
    client.println(F("<button onclick=\"location.href='/'\" class='btn-sec'>← Zpět</button>"));

    client.println(F("</div><script>setTimeout(function(){location.href='/restart';},5000);</script>"));
    client.println(F("</body></html>"));
}

/**
 * @brief Odešle HTML stránku pro restart
 * @param client WiFi klient pro odpověď
 */
void WiFiConfigSystem::sendRestartPage(WiFiClient &client)
{
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: text/html; charset=UTF-8"));
    client.println(F("Connection: close"));
    client.println();
    client.println(F("<!DOCTYPE html><html><head>"));
    client.println(F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"));
    client.println(F("<title>EMG - Restart</title><meta http-equiv='refresh' content='10;url=/'/>"));
    client.println(F("<style>body{font-family:Arial,sans-serif;margin:20px;background:#667eea;color:#333;text-align:center}"));
    client.println(F(".box{max-width:500px;margin:50px auto;background:white;border-radius:10px;padding:30px}"));
    client.println(F("h1{color:#e67e22;margin-bottom:20px}.restart{font-size:60px;color:#e67e22;margin:20px 0;animation:spin 2s linear infinite}"));
    client.println(F("@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}"));
    client.println(F(".progress{background:#ecf0f1;border-radius:15px;height:20px;margin:20px 0;overflow:hidden}"));
    client.println(F(".bar{background:#3498db;height:100%;width:0%;animation:progress 8s linear forwards}"));
    client.println(F("@keyframes progress{0%{width:0%}100%{width:100%}}"));
    client.println(F(".info{background:#e8f4f8;padding:15px;border-radius:8px;margin:20px 0;text-align:left}"));
    client.println(F("</style></head><body><div class='box'>"));
    client.println(F("<div class='restart'>🔄</div><h1>Restartování...</h1>"));
    client.println(F("<p>EMG systém se restartuje.</p><div class='progress'><div class='bar'></div></div>"));

    client.println(F("<div class='info'><b>Probíhá:</b><br>"));
    client.println(F("• Ukládání konfigurace<br>• Restart mikrokontroléru<br>"));
    if (strlen(wifiSSID) > 0)
    {
        client.println(F("• Připojení k WiFi: <b>"));
        client.print(wifiSSID);
        client.println(F("</b><br>• Spuštění EMG serveru na portu "));
        client.print(tcpPort);
        client.println(F("<br>"));
    }
    else
    {
        client.println(F("• Spuštění Access Point<br>"));
    }
    client.println(F("</div><p><b>Automatické obnovení za 10 sekund.</b></p>"));
    client.println(F("</div></body></html>"));
}

/**
 * @brief Odešle hlavní konfigurační HTML stránku
 * @param client WiFi klient pro odpověď
 */
void WiFiConfigSystem::sendConfigPage(WiFiClient &client)
{
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: text/html; charset=UTF-8"));
    client.println(F("Connection: close"));
    client.println();
    client.println(F("<!DOCTYPE html><html><head>"));
    client.println(F("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>"));
    client.println(F("<title>Arduino EMG - WiFi Config</title>"));
    client.println(F("<style>body{font-family:Arial,sans-serif;margin:20px;background:#667eea;color:#333}"));
    client.println(F(".container{max-width:600px;margin:0 auto;background:white;border-radius:10px;padding:25px}"));
    client.println(F("h1{color:#2c3e50;text-align:center;margin-bottom:10px;font-size:2em}h1::before{content:'🔌';margin-right:10px}"));
    client.println(F(".subtitle{text-align:center;color:#7f8c8d;margin-bottom:25px;font-style:italic;border-bottom:2px solid #ecf0f1;padding-bottom:15px}"));
    client.println(F("h2{color:#34495e;margin:25px 0 15px;font-size:1.3em;border-left:4px solid #3498db;padding-left:15px}"));
    client.println(F(".status{padding:12px;border-radius:8px;margin-bottom:20px;font-weight:bold;text-align:center}"));
    client.println(F(".status.ap{background:#ffeaa7;border:2px solid #e17055;color:#2d3436}"));
    client.println(F(".status.connected{background:#00b894;border:2px solid #00a085;color:white}"));
    client.println(F(".form-group{margin-bottom:20px}label{display:block;margin-bottom:5px;font-weight:600;color:#2c3e50}"));
    client.println(F("input[type='text'],input[type='password']{width:100%;padding:12px;border:2px solid #ddd;border-radius:5px;font-size:14px;box-sizing:border-box}"));
    client.println(F("input:focus{outline:none;border-color:#3498db}"));
    client.println(F("button,input[type='submit']{background:#3498db;color:white;padding:12px 25px;border:none;border-radius:5px;font-size:14px;cursor:pointer;margin-right:8px;font-weight:600}"));
    client.println(F("button:hover,input[type='submit']:hover{background:#2980b9}.btn-sec{background:#95a5a6}.btn-sec:hover{background:#7f8c8d}"));
    client.println(F(".info-box{background:#f8f9fa;padding:20px;border-radius:8px;margin:20px 0;border:1px solid #dee2e6}"));
    client.println(F(".info-item{margin-bottom:10px;padding:5px 0;border-bottom:1px solid #ecf0f1}.info-item:last-child{border-bottom:none}"));
    client.println(F(".info-label{font-weight:bold;color:#2c3e50;display:inline-block;min-width:130px}"));
    client.println(F("@media (max-width:768px){.container{margin:10px;padding:15px}.info-label{min-width:auto;display:block;margin-bottom:5px}}"));
    client.println(F("</style></head><body><div class='container'>"));
    client.println(F("<h1>Arduino EMG Systém</h1>"));
    client.println(F("<p class='subtitle'>Elektromyografický systém pro snímání svalových signálů</p>"));

    if (isAPMode)
    {
        client.println(F("<div class='status ap'>⚠️ Access Point - nepodařilo se připojit k WiFi</div>"));
    }
    else
    {
        client.println(F("<div class='status connected'>✅ Připojeno k WiFi síti</div>"));
    }

    client.println(F("<h2>📶 WiFi Konfigurace</h2>"));
    client.println(F("<form method='GET'><div class='form-group'>"));
    client.println(F("<label for='input1'>WiFi síť (SSID):</label>"));
    client.println(F("<input type='text' id='input1' name='input1' placeholder='Název WiFi sítě' required></div>"));
    client.println(F("<div class='form-group'><label for='input2'>Heslo:</label>"));
    client.println(F("<input type='password' id='input2' name='input2' placeholder='WiFi heslo'></div>"));
    client.println(F("<input type='submit' value='💾 Uložit a restartovat'></form>"));

    client.println(F("<div class='info-box'><h2>📋 Současné nastavení</h2>"));
    client.println(F("<div class='info-item'><span class='info-label'>SSID:</span> "));
    client.print(strlen(wifiSSID) > 0 ? wifiSSID : "Nenastaveno");
    client.println(F("</div><div class='info-item'><span class='info-label'>Heslo:</span> "));
    client.print(strlen(wifiPass) > 0 ? "••••••••" : "Nenastaveno");
    client.println(F("</div>"));

    if (!isAPMode)
    {
        client.println(F("<div class='info-item'><span class='info-label'>IP adresa:</span> "));
        client.print(WiFi.localIP());
        client.println(F("</div><div class='info-item'><span class='info-label'>EMG TCP port:</span> "));
        client.print(tcpPort);
        client.println(F("</div>"));
    }
    client.println(F("</div>"));

    client.println(F("<div class='info-box'><h2>ℹ️ Systém</h2>"));
    client.println(F("<div class='info-item'><span class='info-label'>Stav:</span> "));
    client.print(isAPMode ? F("Konfigurační režim") : F("EMG režim - TCP server aktivní"));
    client.println(F("</div><div class='info-item'><span class='info-label'>Verze:</span> EMG System v1.0</div>"));
    client.println(F("<div class='info-item'><span class='info-label'>Protokol:</span> TCP/IP s ALIVE keepalive</div>"));
    client.println(F("<div class='info-item'><span class='info-label'>REST API:</span> GET /status, POST /send-command</div>"));
    client.println(F("<div class='info-item'><span class='info-label'>Senzory:</span> 2x EMG (A0, A1)</div>"));
    client.println(F("<div class='info-item'><span class='info-label'>Frekvence:</span> "));
    client.print(refreshRateHz);
    client.println(F(" Hz</div>"));

    if (isAPMode)
    {
        client.println(F("<div class='info-item'><span class='info-label'>Access Point:</span> "));
        client.print(apSSID);
        client.println(F("</div><div class='info-item'><span class='info-label'>AP heslo:</span> "));
        client.print(apPass);
        client.println(F("</div><div class='info-item'><span class='info-label'>AP IP:</span> "));
        client.print(WiFi.localIP());
        client.println(F("</div>"));
    }
    else
    {
        client.println(F("<div class='info-item'><span class='info-label'>ALIVE interval:</span> "));
        client.print(aliveIntervalMs / 1000);
        client.println(F(" s</div><div class='info-item'><span class='info-label'>WiFi signál:</span> "));
        client.print(WiFi.RSSI());
        client.println(F(" dBm</div>"));
    }
    client.println(F("</div>"));

    if (!isAPMode)
    {
        client.println(F("<div style='margin-top:25px;text-align:center'>"));
        client.println(F("<button onclick=\"location.href='/restart'\" class='btn-sec'>🔄 Restart</button></div>"));
    }

    client.println(F("</div></body></html>"));
}

/**
 * @brief Inicializuje WiFi systém a načte konfiguraci z EEPROM
 */
void WiFiConfigSystem::begin()
{
    // Initialize WiFi module
    if (WiFi.status() == WL_NO_MODULE)
    {
        printIfPinLow(F("Nepodařilo se připojit k WiFi"), debugPin);
        while (true)
            ;
    }

    EEPROMManager::begin();

    // Načti z EEPROM
    EEPROMManager::readString(EEPROM_ADDR_WIFISSID, wifiSSID, sizeof(wifiSSID));
    EEPROMManager::readString(EEPROM_ADDR_WIFIPASS, wifiPass, sizeof(wifiPass));

    printIfPinLow(F("EEPROM data načtena"), debugPin);

    // Pokus o připojení k WiFi, pokud máme uložené údaje
    if (connectToWiFi())
    {
        isAPMode = false;
        printIfPinLow(F("WiFi připojeno - spouštím EMG systém"), debugPin);
        emgSystem.beginServer();
    }
    else
    {
        printIfPinLow(F("Spouštím AP režim..."), debugPin);
        isAPMode = true;
        if (!startAccessPoint())
        {
            while (true)
                ; // Zastavení při chybě AP
        }
    }

    initialized = true;
}

/**
 * @brief Hlavní aktualizační metoda (volat v loop)
 */
void WiFiConfigSystem::update()
{
    if (!initialized)
        return;

    // V AP režimu zpracovávej web konfiguraci
    if (isAPMode)
    {
        WiFiClient client = server.available();
        if (!client)
            return;

        printIfPinLow(F("Klient připojen"), debugPin);

        char reqLine[256] = "";
        int reqIndex = 0;
        unsigned long timeout = millis();
        while (client.connected() && millis() - timeout < 2000)
        {
            if (client.available())
            {
                char c = client.read();
                if (reqIndex < sizeof(reqLine) - 1)
                {
                    reqLine[reqIndex++] = c;
                    reqLine[reqIndex] = '\0';
                }
                if (c == '\n')
                    break;
            }
        }

        printIfPinLow(reqLine, debugPin);

        // Zpracování GET parametrů
        if (strncmp(reqLine, "GET /?", 6) == 0)
        {
            if (handleWiFiConfig(reqLine, client))
            {
                delay(50);
                client.stop();
                printIfPinLow(F("Klient odpojen po uložení konfigurace"), debugPin);

                // Krátká pauza před restartem
                delay(2000);

                // Ukončíme WiFi spojení
                WiFi.disconnect();
                delay(500);

                // Restartujeme systém
                printIfPinLow(F("Restartování po uložení nové konfigurace..."), debugPin);
                reboot();
                return;
            }
        }

        // Zpracování restart požadavku
        if (strncmp(reqLine, "GET /restart", 12) == 0)
        {
            sendRestartPage(client);
            delay(50);
            client.stop();
            printIfPinLow(F("Restartování Arduino..."), debugPin);

            // Ukončíme WiFi spojení
            WiFi.disconnect();

            // Počkáme na dokončení všech operací
            delay(1000);

            // Restartujeme systém
            reboot();
            return;
        }

        // HTML odpověď - hlavní stránka
        sendConfigPage(client);

        delay(10);
        client.stop();
        printIfPinLow(F("Klient odpojen"), debugPin);
    }
    else
    {
        // V WiFi režimu zpracovávej EMG systém a REST API
        emgSystem.update();
    }
}

/**
 * @brief Vrací příznak režimu Access Point
 * @return True pokud je systém v AP režimu
 */
bool WiFiConfigSystem::isInAPMode() const
{
    return isAPMode;
}

/**
 * @brief Vrací aktuální WiFi SSID
 * @param buffer Buffer pro SSID řetězec
 * @param bufferSize Velikost bufferu
 * @return Počet zkopírovaných znaků
 */
int WiFiConfigSystem::getWiFiSSID(char *buffer, int bufferSize) const
{
    if (!buffer || bufferSize <= 0)
        return 0;

    int len = strlen(wifiSSID);
    if (len >= bufferSize)
        len = bufferSize - 1;

    strncpy(buffer, wifiSSID, len);
    buffer[len] = '\0';
    return len;
}

/**
 * @brief Vrací informaci o délce WiFi hesla
 * @return True pokud je heslo nastaveno
 */
bool WiFiConfigSystem::hasWiFiPassword() const
{
    return strlen(wifiPass) > 0;
}

/**
 * @brief Nastaví pointer na LCD displej
 * @param lcd Pointer na LCD displej
 */
void WiFiConfigSystem::setLCDDisplay(LCDDisplay *lcd)
{
    lcdDisplay = lcd;
}