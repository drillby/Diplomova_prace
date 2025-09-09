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
 * @brief Vytiskne zprávu přes Serial pokud je daný pin LOW (F macro support)
 * @param message Zpráva k výpisu (flash string)
 * @param pin Pin, který musí být LOW
 */
void printIfPinLow(const __FlashStringHelper *message, int pin);

/**
 * @brief Utility funkce pro práci s řetězci a URL dekódování
 * @param str Řetězec k dekódování (C-string)
 * @param buffer Buffer pro dekódovaný řetězec
 * @param bufferSize Velikost bufferu
 * @return Počet dekódovaných znaků nebo -1 při chybě
 */
int urlDecode(const char *str, char *buffer, int bufferSize);

/**
 * @brief Restartuje Arduino pomocí watchdog timeru
 */
void reboot();

#endif // UTILS_H
