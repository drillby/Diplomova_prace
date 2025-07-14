#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

/**
 * @brief Vytiskne zprávu přes Serial pokud je daný pin LOW
 * @param message Zpráva k výpisu
 * @param pin Pin, který musí být LOW
 */
void printIfPinLow(const char *message, int pin);

/**
 * @brief Utility funkce pro práci s řetězci a URL dekódování
 * @param str Řetězec k dekódování
 * @return Dekódovaný řetězec
 */
String urlDecode(const String &str);

/**
 * @brief Restartuje Arduino pomocí watchdog timeru
 */
void reboot();

#endif // UTILS_H
