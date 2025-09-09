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
 * @brief Vytiskne zprávu přes Serial pokud je daný pin LOW (F macro support)
 * @param message Zpráva k výpisu (flash string)
 * @param pin Pin, který musí být LOW
 */
void printIfPinLow(const __FlashStringHelper *message, int pin)
{
    if (digitalRead(pin) == LOW)
    {
        Serial.println(message);
    }
}

/**
 * @brief Utility funkce pro práci s řetězci a URL dekódování
 * @param str Řetězec k dekódování (C-string)
 * @param buffer Buffer pro dekódovaný řetězec
 * @param bufferSize Velikost bufferu
 * @return Počet dekódovaných znaků nebo -1 při chybě
 */
int urlDecode(const char *str, char *buffer, int bufferSize)
{
    if (!str || !buffer || bufferSize <= 0)
        return -1;

    int srcLen = strlen(str);
    int destIndex = 0;

    for (int i = 0; i < srcLen && destIndex < bufferSize - 1; i++)
    {
        if (str[i] == '+')
        {
            buffer[destIndex++] = ' ';
        }
        else if (str[i] == '%' && i + 2 < srcLen)
        {
            // Convert hex to number more efficiently
            char c1 = str[i + 1];
            char c2 = str[i + 2];

            // Simple hex decode for most common cases
            if (c1 == '2')
            {
                switch (c2)
                {
                case '0':
                    buffer[destIndex++] = ' ';
                    break;
                case '1':
                    buffer[destIndex++] = '!';
                    break;
                case '2':
                    buffer[destIndex++] = '"';
                    break;
                case '3':
                    buffer[destIndex++] = '#';
                    break;
                case '4':
                    buffer[destIndex++] = '$';
                    break;
                case '5':
                    buffer[destIndex++] = '%';
                    break;
                case '6':
                    buffer[destIndex++] = '&';
                    break;
                case '7':
                    buffer[destIndex++] = '\'';
                    break;
                case '8':
                    buffer[destIndex++] = '(';
                    break;
                case '9':
                    buffer[destIndex++] = ')';
                    break;
                case 'A':
                case 'a':
                    buffer[destIndex++] = '*';
                    break;
                case 'B':
                case 'b':
                    buffer[destIndex++] = '+';
                    break;
                case 'C':
                case 'c':
                    buffer[destIndex++] = ',';
                    break;
                case 'D':
                case 'd':
                    buffer[destIndex++] = '-';
                    break;
                case 'E':
                case 'e':
                    buffer[destIndex++] = '.';
                    break;
                case 'F':
                case 'f':
                    buffer[destIndex++] = '/';
                    break;
                default:
                    buffer[destIndex++] = str[i];
                    if (destIndex < bufferSize - 1)
                        buffer[destIndex++] = c1;
                    if (destIndex < bufferSize - 1)
                        buffer[destIndex++] = c2;
                    break;
                }
            }
            else if (c1 == '3')
            {
                switch (c2)
                {
                case 'A':
                case 'a':
                    buffer[destIndex++] = ':';
                    break;
                case 'B':
                case 'b':
                    buffer[destIndex++] = ';';
                    break;
                case 'C':
                case 'c':
                    buffer[destIndex++] = '<';
                    break;
                case 'D':
                case 'd':
                    buffer[destIndex++] = '=';
                    break;
                case 'E':
                case 'e':
                    buffer[destIndex++] = '>';
                    break;
                case 'F':
                case 'f':
                    buffer[destIndex++] = '?';
                    break;
                default:
                    buffer[destIndex++] = str[i];
                    if (destIndex < bufferSize - 1)
                        buffer[destIndex++] = c1;
                    if (destIndex < bufferSize - 1)
                        buffer[destIndex++] = c2;
                    break;
                }
            }
            else
            {
                // For other hex codes, just copy original characters
                buffer[destIndex++] = str[i];
                if (destIndex < bufferSize - 1)
                    buffer[destIndex++] = c1;
                if (destIndex < bufferSize - 1)
                    buffer[destIndex++] = c2;
            }
            i += 2; // Skip the next two characters
        }
        else
        {
            buffer[destIndex++] = str[i];
        }
    }

    buffer[destIndex] = '\0';
    return destIndex;
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
