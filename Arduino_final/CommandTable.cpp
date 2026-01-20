#include "CommandTable.h"

/**
 * @brief Tabulka dostupných příkazů
 */
const CommandEntry commandTable[9] = {
    {0, "DO_NOTHING"},
    {1, "LEFT"},
    {2, "RIGHT"},
    {3, "UP"},
    {4, "DOWN"},
    {5, "FORWARD"},
    {6, "BACKWARD"},
    {7, "OPEN"},
    {8, "CLOSE"}};

/**
 * @brief Vrátí textový název příkazu na základě jeho kódu
 * @param code Číselný kód příkazu
 * @return Textový popis nebo "UNKNOWN" při neznámém kódu
 */
const char *getCommandLabel(uint8_t code)
{
    for (const auto &entry : commandTable)
    {
        if (entry.code == code)
            return entry.label;
    }
    return "UNKNOWN";
}
