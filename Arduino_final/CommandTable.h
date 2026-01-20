#ifndef COMMAND_TABLE_H
#define COMMAND_TABLE_H

#include <Arduino.h>

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
extern const CommandEntry commandTable[9];

/**
 * @brief Vrátí textový název příkazu na základě jeho kódu
 * @param code Číselný kód příkazu
 * @return Textový popis nebo "UNKNOWN" při neznámém kódu
 */
const char *getCommandLabel(uint8_t code);

#endif // COMMAND_TABLE_H
