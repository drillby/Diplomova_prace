#include "EEPROMManager.h"
#include "Config.h"

/**
 * @brief Inicializuje EEPROM
 */
void EEPROMManager::begin()
{
    EEPROM.begin();
}

/**
 * @brief Zapíše řetězec do EEPROM na specifikovanou adresu
 * @param startAddr Počáteční adresa v EEPROM
 * @param data Řetězec k zápisu (C-string)
 */
void EEPROMManager::writeString(int startAddr, const char *data)
{
    int len = strlen(data);
    len = len > maxStringLength ? maxStringLength : len; // max 31 znaků + 1 bajt na délku

    // Čtení aktuální délky
    int storedLen = EEPROM.read(startAddr);
    if (storedLen != len)
    {
        EEPROM.write(startAddr, len);
    }

    // Zápis jednotlivých znaků jen pokud se liší
    for (int i = 0; i < len; i++)
    {
        char c = data[i];
        if (EEPROM.read(startAddr + 1 + i) != c)
        {
            EEPROM.write(startAddr + 1 + i, c);
        }
    }

    // Vymazání zbytků staršího řetězce, pokud nový je kratší
    for (int i = len; i < storedLen; i++)
    {
        EEPROM.write(startAddr + 1 + i, 0);
    }
}

/**
 * @brief Načte řetězec z EEPROM ze specifikované adresy
 * @param startAddr Počáteční adresa v EEPROM
 * @param buffer Buffer pro načtený řetězec
 * @param bufferSize Velikost bufferu
 * @return Počet načtených znaků nebo -1 při chybě
 */
int EEPROMManager::readString(int startAddr, char *buffer, int bufferSize)
{
    int len = EEPROM.read(startAddr);
    if (len < 0 || len > maxStringLength || len >= bufferSize)
        return -1;

    for (int i = 0; i < len; i++)
    {
        buffer[i] = EEPROM.read(startAddr + 1 + i);
    }
    buffer[len] = '\0'; // Null terminator
    return len;
}
