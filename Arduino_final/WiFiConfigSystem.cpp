#include "WiFiConfigSystem.h"
#include "Config.h"
#include "Utils.h"
#include "EEPROMManager.h"

/**
 * @brief Konstruktor WiFiConfigSystem
 * @param emgSys Reference na EMG systém
 * @param lcd Pointer na LCD displej (volitelný)
 */
WiFiConfigSystem::WiFiConfigSystem(EMGSystem &emgSys, LCDDisplay *lcd) : server(httpPort), isAPMode(false), initialized(false), emgSystem(emgSys), lcdDisplay(lcd) {}

/**
 * @brief Pokusí se připojit k WiFi síti
 * @return True pokud se připojení podařilo
 */
bool WiFiConfigSystem::connectToWiFi()
{
    if (wifiSSID.length() == 0 || wifiPass.length() == 0)
        return false;

    printIfPinLow("Zkouším připojení k WiFi...", debugPin);
    printIfPinLow(("SSID: " + wifiSSID).c_str(), debugPin);
    printIfPinLow(("Pass length: " + String(wifiPass.length())).c_str(), debugPin);

    // Update LCD
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->clear();
        lcdDisplay->setBacklightColor(255, 165, 0); // Oranžová pro připojování
        lcdDisplay->printAt(0, 0, "Pripojovani WiFi");
        lcdDisplay->printAt(0, 1, wifiSSID.substring(0, 16));
    }

    // Disconnect any previous connection
    WiFi.disconnect();
    delay(100);

    // Začni připojení
    WiFi.begin(wifiSSID.c_str(), wifiPass.c_str());

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
        printIfPinLow("", debugPin);
        printIfPinLow("WiFi připojeno!", debugPin);
        IPAddress ip = WiFi.localIP();
        String ipMsg = "IP adresa: " + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
        ;
        printIfPinLow(ipMsg.c_str(), debugPin);
        String rssiMsg = "Síla signálu: " + String(WiFi.RSSI());
        printIfPinLow(rssiMsg.c_str(), debugPin);

        // Update LCD with success
        if (lcdDisplay && lcdDisplay->isReady())
        {
            lcdDisplay->setBacklightColor(0, 255, 0); // Zelená pro úspěch
            lcdDisplay->clear();
            lcdDisplay->printAt(0, 0, "WiFi pripojeno!");
            String ipStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
            lcdDisplay->printAt(0, 1, ipStr.substring(0, 16));
            delay(2000);
        }

        return true;
    }

    printIfPinLow("", debugPin);
    printIfPinLow(("Připojení k WiFi se nezdařilo. Status: " + String(WiFi.status())).c_str(), debugPin);
    return false;
}

/**
 * @brief Spustí Access Point režim
 * @return True pokud se AP spustil úspěšně
 */
bool WiFiConfigSystem::startAccessPoint()
{
    printIfPinLow("Spouštím Access Point...", debugPin);

    // Update LCD
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->clear();
        lcdDisplay->setBacklightColor(255, 255, 0); // Žlutá pro AP režim
        lcdDisplay->printAt(0, 0, "Access Point");
        lcdDisplay->printAt(0, 1, "Spousteni...");
    }

    if (WiFi.beginAP(apSSID, apPass) != WL_AP_LISTENING)
    {
        printIfPinLow("Chyba při spouštění AP", debugPin);

        // Update LCD with error
        if (lcdDisplay && lcdDisplay->isReady())
        {
            lcdDisplay->setBacklightColor(255, 0, 0); // Červená pro chybu
            lcdDisplay->clear();
            lcdDisplay->printAt(0, 0, "Chyba AP!");
            delay(2000);
        }

        return false;
    }

    delay(apStabilizationMs); // nutné pro stabilizaci

    IPAddress ip = WiFi.localIP();
    String ipMsg = "IP adresa AP: " + String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
    ;
    printIfPinLow(ipMsg.c_str(), debugPin);

    // Update LCD with AP info
    if (lcdDisplay && lcdDisplay->isReady())
    {
        lcdDisplay->setBacklightColor(255, 255, 0); // Žlutá pro AP režim
        lcdDisplay->clear();
        lcdDisplay->printAt(0, 0, "AP: EMG_Config");
        String ipStr = String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
        lcdDisplay->printAt(0, 1, ipStr.substring(0, 16));
    }

    // Spusť web server pouze v AP režimu
    server.begin();
    printIfPinLow("Web server spuštěn v AP režimu", debugPin);
    return true;
}

/**
 * @brief Zpracuje HTTP požadavek s WiFi údaji
 * @param reqLine Řádek HTTP požadavku
 * @param client WiFi klient pro odpověď
 * @return True pokud byl požadavek zpracován
 */
bool WiFiConfigSystem::handleWiFiConfig(const String &reqLine, WiFiClient &client)
{
    int i1 = reqLine.indexOf("input1=");
    int i2 = reqLine.indexOf("&input2=");
    int i3 = reqLine.indexOf(" ", i2);

    if (i1 > 0 && i2 > 0 && i3 > 0)
    {
        wifiSSID = reqLine.substring(i1 + 7, i2);
        wifiPass = reqLine.substring(i2 + 8, i3);

        // URL decode
        wifiSSID = urlDecode(wifiSSID);
        wifiPass = urlDecode(wifiPass);

        printIfPinLow("Decoded values:", debugPin);
        printIfPinLow(("SSID = " + wifiSSID).c_str(), debugPin);
        printIfPinLow(("Pass length = " + String(wifiPass.length())).c_str(), debugPin);

        EEPROMManager::writeString(EEPROM_ADDR_WIFISSID, wifiSSID);
        EEPROMManager::writeString(EEPROM_ADDR_WIFIPASS, wifiPass);

        printIfPinLow("Uloženo do EEPROM:", debugPin);
        printIfPinLow(("SSID = " + wifiSSID).c_str(), debugPin);
        printIfPinLow(("Pass = " + wifiPass).c_str(), debugPin);

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
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=UTF-8");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Arduino EMG Systém - Konfigurace uložena</title>");
    client.println("<style>");
    client.println("body{font-family:'Segoe UI',Arial,sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#333;min-height:100vh}");
    client.println(".container{max-width:500px;margin:50px auto;background:white;border-radius:15px;box-shadow:0 8px 32px rgba(0,0,0,0.1);padding:40px;text-align:center;backdrop-filter:blur(10px)}");
    client.println("h1{color:#27ae60;margin-bottom:20px;font-size:2em;font-weight:300}");
    client.println(".success-icon{font-size:80px;color:#27ae60;margin-bottom:25px;animation:bounce 1s ease-in-out}");
    client.println("@keyframes bounce{0%,20%,50%,80%,100%{transform:translateY(0)}40%{transform:translateY(-20px)}60%{transform:translateY(-10px)}}");
    client.println("p{color:#7f8c8d;margin-bottom:20px;line-height:1.6;font-size:1.1em}");
    client.println("button{background:linear-gradient(135deg,#3498db,#2980b9);color:white;padding:15px 30px;border:none;border-radius:8px;font-size:16px;cursor:pointer;margin:10px;text-decoration:none;display:inline-block;transition:all 0.3s ease;font-weight:600}");
    client.println("button:hover{background:linear-gradient(135deg,#2980b9,#1f5f99);transform:translateY(-2px);box-shadow:0 5px 15px rgba(0,0,0,0.2)}");
    client.println(".btn-secondary{background:linear-gradient(135deg,#95a5a6,#7f8c8d)}");
    client.println(".btn-secondary:hover{background:linear-gradient(135deg,#7f8c8d,#6c7b7d)}");
    client.println(".config-info{background:linear-gradient(135deg,#e8f4f8,#d1ecf1);padding:25px;border-radius:12px;margin:25px 0;text-align:left;border:2px solid #bee5eb}");
    client.println(".info-label{font-weight:bold;color:#2c3e50}");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<div class='container'>");
    client.println("<div class='success-icon'>✅</div>");
    client.println("<h1>Konfigurace uložena!</h1>");
    client.println("<p>WiFi údaje byly úspěšně uloženy do EEPROM paměti Arduino.</p>");

    client.println("<div class='config-info'>");
    client.println("<div style='margin-bottom:10px'>");
    client.println("<span class='info-label'>Uložené SSID:</span> ");
    client.print(wifiSSID);
    client.println("</div>");
    client.println("<div>");
    client.println("<span class='info-label'>Heslo:</span> Uloženo (skryto)");
    client.println("</div>");
    client.println("</div>");

    client.println("<p>Arduino se nyní pokusí připojit k nové WiFi síti. Pokud se připojení nezdaří, automaticky se spustí režim Access Point pro další konfiguraci.</p>");
    client.println("<p><strong>Systém se restartuje za 5 sekund...</strong></p>");

    client.println("<button onclick=\"location.href='/restart'\">🔄 Restartovat nyní</button>");
    client.println("<button onclick=\"location.href='/'\" class='btn-secondary'>← Zpět na hlavní stránku</button>");

    client.println("</div>");
    client.println("<script>");
    client.println("setTimeout(function(){location.href='/restart';}, 5000);");
    client.println("</script>");
    client.println("</body>");
    client.println("</html>");
}

/**
 * @brief Odešle HTML stránku pro restart
 * @param client WiFi klient pro odpověď
 */
void WiFiConfigSystem::sendRestartPage(WiFiClient &client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=UTF-8");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Arduino EMG Systém - Restartování</title>");
    client.println("<meta http-equiv='refresh' content='10;url=/'>");
    client.println("<style>");
    client.println("body{font-family:'Segoe UI',Arial,sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#333;text-align:center;min-height:100vh}");
    client.println(".container{max-width:500px;margin:50px auto;background:white;border-radius:15px;box-shadow:0 8px 32px rgba(0,0,0,0.1);padding:40px;backdrop-filter:blur(10px)}");
    client.println("h1{color:#e67e22;margin-bottom:20px;font-size:2em;font-weight:300}");
    client.println(".restart-icon{font-size:80px;color:#e67e22;margin-bottom:25px;animation:spin 2s linear infinite}");
    client.println("@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}");
    client.println("p{color:#7f8c8d;margin-bottom:15px;line-height:1.6;font-size:1.1em}");
    client.println(".progress{background:#ecf0f1;border-radius:15px;height:25px;margin:25px 0;overflow:hidden;box-shadow:inset 0 2px 4px rgba(0,0,0,0.1)}");
    client.println(".progress-bar{background:linear-gradient(135deg,#3498db,#2980b9);height:100%;width:0%;animation:progress 8s linear forwards;border-radius:15px}");
    client.println("@keyframes progress{0%{width:0%}100%{width:100%}}");
    client.println(".info-box{background:linear-gradient(135deg,#e8f4f8,#d1ecf1);border:2px solid #bee5eb;padding:25px;border-radius:12px;margin:25px 0;text-align:left}");
    client.println(".info-label{font-weight:bold;color:#2c3e50}");
    client.println("ul{margin:0;padding-left:20px;color:#7f8c8d}");
    client.println("li{margin-bottom:8px}");
    client.println("strong{color:#2c3e50}");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<div class='container'>");
    client.println("<div class='restart-icon'>🔄</div>");
    client.println("<h1>Restartování systému...</h1>");
    client.println("<p>Arduino EMG systém se restartuje. Tento proces může trvat několik sekund.</p>");

    client.println("<div class='progress'>");
    client.println("<div class='progress-bar'></div>");
    client.println("</div>");

    client.println("<div class='info-box'>");
    client.println("<div style='margin-bottom:10px'>");
    client.println("<span class='info-label'>Co se děje:</span>");
    client.println("</div>");
    client.println("<ul style='margin:0;padding-left:20px;color:#7f8c8d'>");
    client.println("<li>Ukládání konfigurace do paměti</li>");
    client.println("<li>Restart mikrokontroléru</li>");
    if (wifiSSID.length() > 0)
    {
        client.println("<li>Pokus o připojení k WiFi síti: <strong>");
        client.print(wifiSSID);
        client.println("</strong></li>");
        client.println("<li>Spuštění EMG TCP serveru na portu ");
        client.print(tcpPort);
        client.println("</li>");
    }
    else
    {
        client.println("<li>Spuštění Access Point režimu</li>");
    }
    client.println("</ul>");
    client.println("</div>");

    client.println("<p><strong>Stránka se automaticky obnoví za 10 sekund.</strong></p>");
    client.println("<p style='font-size:12px;color:#95a5a6'>Pokud se stránka neobnoví, zkuste ji načíst ručně nebo se připojte k Access Point síti Arduino.</p>");

    client.println("</div>");
    client.println("</body>");
    client.println("</html>");
}

/**
 * @brief Odešle hlavní konfigurační HTML stránku
 * @param client WiFi klient pro odpověď
 */
void WiFiConfigSystem::sendConfigPage(WiFiClient &client)
{
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html; charset=UTF-8");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Arduino EMG Systém - WiFi Konfigurace</title>");
    client.println("<style>");
    client.println("body{font-family:'Segoe UI',Arial,sans-serif;margin:0;padding:20px;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#333;min-height:100vh}");
    client.println(".container{max-width:600px;margin:0 auto;background:white;border-radius:15px;box-shadow:0 8px 32px rgba(0,0,0,0.1);padding:30px;backdrop-filter:blur(10px)}");
    client.println("h1{color:#2c3e50;text-align:center;margin-bottom:10px;font-size:2.2em;font-weight:300}");
    client.println("h1::before{content:'🔌';font-size:0.8em;margin-right:10px}");
    client.println(".subtitle{text-align:center;color:#7f8c8d;margin-bottom:30px;font-style:italic;border-bottom:2px solid #ecf0f1;padding-bottom:20px}");
    client.println("h2{color:#34495e;margin-top:30px;margin-bottom:15px;font-size:1.4em;border-left:4px solid #3498db;padding-left:15px}");
    client.println(".status{padding:15px;border-radius:10px;margin-bottom:25px;font-weight:bold;text-align:center;position:relative;overflow:hidden}");
    client.println(".status::before{content:'';position:absolute;top:0;left:-100%;width:100%;height:100%;background:linear-gradient(90deg,transparent,rgba(255,255,255,0.3),transparent);animation:shimmer 2s infinite}");
    client.println("@keyframes shimmer{0%{left:-100%}100%{left:100%}}");
    client.println(".status.ap{background:linear-gradient(135deg,#ffeaa7,#fdcb6e);border:2px solid #e17055;color:#2d3436}");
    client.println(".status.connected{background:linear-gradient(135deg,#00b894,#00cec9);border:2px solid #00a085;color:white}");
    client.println(".form-group{margin-bottom:25px}");
    client.println("label{display:block;margin-bottom:8px;font-weight:600;color:#2c3e50;font-size:1.1em}");
    client.println("input[type='text'],input[type='password']{width:100%;padding:15px;border:2px solid #ddd;border-radius:8px;font-size:16px;box-sizing:border-box;transition:all 0.3s ease}");
    client.println("input[type='text']:focus,input[type='password']:focus{outline:none;border-color:#3498db;box-shadow:0 0 10px rgba(52,152,219,0.3);transform:translateY(-2px)}");
    client.println("button,input[type='submit']{background:linear-gradient(135deg,#3498db,#2980b9);color:white;padding:15px 30px;border:none;border-radius:8px;font-size:16px;cursor:pointer;margin-right:10px;text-decoration:none;display:inline-block;transition:all 0.3s ease;font-weight:600}");
    client.println("button:hover,input[type='submit']:hover{background:linear-gradient(135deg,#2980b9,#1f5f99);transform:translateY(-2px);box-shadow:0 5px 15px rgba(0,0,0,0.2)}");
    client.println(".btn-secondary{background:linear-gradient(135deg,#95a5a6,#7f8c8d)}");
    client.println(".btn-secondary:hover{background:linear-gradient(135deg,#7f8c8d,#6c7b7d)}");
    client.println(".info-box{background:linear-gradient(135deg,#f8f9fa,#e9ecef);padding:25px;border-radius:12px;margin:25px 0;border:1px solid #dee2e6}");
    client.println(".info-item{margin-bottom:12px;padding:8px 0;border-bottom:1px solid #ecf0f1}");
    client.println(".info-item:last-child{border-bottom:none}");
    client.println(".info-label{font-weight:bold;color:#2c3e50;display:inline-block;min-width:150px}");
    client.println(".system-info{background:linear-gradient(135deg,#e8f4f8,#d1ecf1);border:2px solid #bee5eb;padding:25px;border-radius:12px;margin-top:25px}");
    client.println("@media (max-width:768px){.container{margin:10px;padding:20px}.info-label{min-width:auto;display:block;margin-bottom:5px}}");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<div class='container'>");
    client.println("<h1>Arduino EMG Systém</h1>");
    client.println("<p class='subtitle'>Elektromyografický systém pro snímání a zpracování svalových signálů</p>");

    if (isAPMode)
    {
        client.println("<div class='status ap'>⚠️ Režim Access Point aktivní - nepodařilo se připojit k WiFi síti</div>");
    }
    else
    {
        client.println("<div class='status connected'>✅ Úspěšně připojeno k WiFi síti</div>");
    }

    client.println("<h2>📶 Konfigurace WiFi připojení</h2>");
    client.println("<form method='GET'>");
    client.println("<div class='form-group'>");
    client.println("<label for='input1'>Název WiFi sítě (SSID):</label>");
    client.println("<input type='text' id='input1' name='input1' placeholder='Zadejte název WiFi sítě' required>");
    client.println("</div>");
    client.println("<div class='form-group'>");
    client.println("<label for='input2'>Heslo WiFi sítě:</label>");
    client.println("<input type='password' id='input2' name='input2' placeholder='Zadejte heslo WiFi sítě'>");
    client.println("</div>");
    client.println("<input type='submit' value='💾 Uložit a restartovat'>");
    client.println("</form>");

    client.println("<div class='info-box'>");
    client.println("<h2>📋 Aktuální nastavení</h2>");
    client.println("<div class='info-item'>");
    client.println("<span class='info-label'>SSID:</span> ");
    client.print(wifiSSID.length() > 0 ? wifiSSID : "Nenastaveno");
    client.println("</div>");
    client.println("<div class='info-item'>");
    client.println("<span class='info-label'>Heslo:</span> ");
    client.print(wifiPass.length() > 0 ? "••••••••" : "Nenastaveno");
    client.println("</div>");

    if (!isAPMode)
    {
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>IP adresa:</span> ");
        client.print(WiFi.localIP());
        client.println("</div>");
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>EMG TCP port:</span> ");
        client.print(tcpPort);
        client.println("</div>");
    }
    client.println("</div>");

    client.println("<div class='system-info'>");
    client.println("<h2>ℹ️ Systémové informace</h2>");
    client.println("<div class='info-item'>");
    client.println("<span class='info-label'>Stav systému:</span> ");
    if (isAPMode)
    {
        client.println("Konfigurační režim (Access Point)");
    }
    else
    {
        client.println("EMG režim - připraven na příjem TCP spojení");
    }
    client.println("</div>");
    client.println("<div class='info-item'>");
    client.println("<span class='info-label'>Verze firmware:</span> Arduino EMG System v1.0");
    client.println("</div>");
    client.println("<div class='info-item'>");
    client.println("<span class='info-label'>Komunikační protokol:</span> TCP/IP s ALIVE keepalive");
    client.println("</div>");
    client.println("<div class='info-item'>");
    client.println("<span class='info-label'>Podporované senzory:</span> 2x EMG (A0, A1)");
    client.println("</div>");
    client.println("<div class='info-item'>");
    client.println("<span class='info-label'>Aktualizační frekvence:</span> ");
    client.print(refreshRateHz);
    client.println(" Hz");
    client.println("</div>");

    if (isAPMode)
    {
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>Access Point:</span> ");
        client.print(apSSID);
        client.println("</div>");
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>AP heslo:</span> ");
        client.print(apPass);
        client.println("</div>");
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>AP IP adresa:</span> ");
        client.print(WiFi.localIP());
        client.println("</div>");
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>Návod k připojení:</span> Připojte se k WiFi síti \"");
        client.print(apSSID);
        client.println("\" a otevřete tuto stránku v prohlížeči");
        client.println("</div>");
    }
    else
    {
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>ALIVE interval:</span> ");
        client.print(aliveIntervalMs / 1000);
        client.println(" sekund");
        client.println("</div>");
        client.println("<div class='info-item'>");
        client.println("<span class='info-label'>Síla WiFi signálu:</span> ");
        client.print(WiFi.RSSI());
        client.println(" dBm");
        client.println("</div>");
    }
    client.println("</div>");

    if (!isAPMode)
    {
        client.println("<div style='margin-top:30px;text-align:center'>");
        client.println("<button onclick=\"location.href='/restart'\" class='btn-secondary'>🔄 Restartovat systém</button>");
        client.println("</div>");
    }

    client.println("</div>");
    client.println("</body>");
    client.println("</html>");
}

/**
 * @brief Inicializuje WiFi systém a načte konfiguraci z EEPROM
 */
void WiFiConfigSystem::begin()
{
    // Initialize WiFi module
    if (WiFi.status() == WL_NO_MODULE)
    {
        printIfPinLow("Nepodařilo se připojit k WiFi", debugPin);
        while (true)
            ;
    }

    EEPROMManager::begin();

    // Načti z EEPROM
    wifiSSID = EEPROMManager::readString(EEPROM_ADDR_WIFISSID);
    wifiPass = EEPROMManager::readString(EEPROM_ADDR_WIFIPASS);

    printIfPinLow(("Načteno z EEPROM - SSID: " + wifiSSID).c_str(), debugPin);
    printIfPinLow(("Načteno z EEPROM - Pass: " + wifiPass).c_str(), debugPin);

    // Pokus o připojení k WiFi, pokud máme uložené údaje
    if (connectToWiFi())
    {
        isAPMode = false;
        printIfPinLow("WiFi připojeno - spouštím EMG systém", debugPin);
        emgSystem.beginServer();
    }
    else
    {
        printIfPinLow("Spouštím AP režim...", debugPin);
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

        printIfPinLow("Klient připojen", debugPin);

        String reqLine = "";
        unsigned long timeout = millis();
        while (client.connected() && millis() - timeout < 2000)
        {
            if (client.available())
            {
                char c = client.read();
                reqLine += c;
                if (c == '\n')
                    break;
            }
        }

        printIfPinLow(("Request: " + reqLine).c_str(), debugPin);

        // Zpracování GET parametrů
        if (reqLine.startsWith("GET /?"))
        {
            if (handleWiFiConfig(reqLine, client))
            {
                delay(50);
                client.stop();
                printIfPinLow("Klient odpojen po uložení konfigurace", debugPin);

                // Krátká pauza před restartem
                delay(2000);

                // Ukončíme WiFi spojení
                WiFi.disconnect();
                delay(500);

                // Restartujeme systém
                printIfPinLow("Restartování po uložení nové konfigurace...", debugPin);
                reboot();
                return;
            }
        }

        // Zpracování restart požadavku
        if (reqLine.startsWith("GET /restart"))
        {
            sendRestartPage(client);
            delay(50);
            client.stop();
            printIfPinLow("Restartování Arduino...", debugPin);

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
        printIfPinLow("Klient odpojen", debugPin);
    }
    else
    {
        // V WiFi režimu zpracovávej EMG systém
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
 * @return WiFi SSID řetězec
 */
String WiFiConfigSystem::getWiFiSSID() const
{
    return wifiSSID;
}

/**
 * @brief Vrací informaci o délce WiFi hesla
 * @return True pokud je heslo nastaveno
 */
bool WiFiConfigSystem::hasWiFiPassword() const
{
    return wifiPass.length() > 0;
}

/**
 * @brief Nastaví pointer na LCD displej
 * @param lcd Pointer na LCD displej
 */
void WiFiConfigSystem::setLCDDisplay(LCDDisplay *lcd)
{
    lcdDisplay = lcd;
}
