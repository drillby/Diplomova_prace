#include "Utils.h"
#include <avr/wdt.h>

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
 * @brief Utility funkce pro práci s řetězci a URL dekódování
 * @param str Řetězec k dekódování
 * @return Dekódovaný řetězec
 */
String urlDecode(const String &str)
{
    String decoded = str;
    decoded.replace("+", " ");
    decoded.replace("%20", " ");
    decoded.replace("%21", "!");
    decoded.replace("%22", "\"");
    decoded.replace("%23", "#");
    decoded.replace("%24", "$");
    decoded.replace("%25", "%");
    decoded.replace("%26", "&");
    decoded.replace("%27", "'");
    decoded.replace("%28", "(");
    decoded.replace("%29", ")");
    decoded.replace("%2A", "*");
    decoded.replace("%2B", "+");
    decoded.replace("%2C", ",");
    decoded.replace("%2D", "-");
    decoded.replace("%2E", ".");
    decoded.replace("%2F", "/");
    decoded.replace("%3A", ":");
    decoded.replace("%3B", ";");
    decoded.replace("%3C", "<");
    decoded.replace("%3D", "=");
    decoded.replace("%3E", ">");
    decoded.replace("%3F", "?");
    decoded.replace("%40", "@");
    decoded.replace("%5B", "[");
    decoded.replace("%5C", "\\");
    decoded.replace("%5D", "]");
    decoded.replace("%5E", "^");
    decoded.replace("%5F", "_");
    decoded.replace("%60", "`");
    decoded.replace("%7B", "{");
    decoded.replace("%7C", "|");
    decoded.replace("%7D", "}");
    decoded.replace("%7E", "~");
    return decoded;
}

/**
 * @brief Restartuje Arduino pomocí watchdog timeru
 */
void reboot()
{
    // Zakážeme všechny přerušení
    cli();

    // Vypneme watchdog timer
    wdt_disable();

    // Počkáme chvilku pro ukončení všech operací
    delay(100);

    // Nastavíme watchdog timer na nejkratší možný čas (15ms)
    wdt_enable(WDTO_15MS);

    // Nekonečná smyčka - watchdog timer způsobí restart
    while (true)
    {
        // Prázdná smyčka - čekáme na watchdog reset
        ;
    }
}
