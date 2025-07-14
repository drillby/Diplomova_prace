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
 * @param data Řetězec k zápisu
 */
void EEPROMManager::writeString(int startAddr, const String &data)
{
    int len = data.length();
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
 * @return Načtený řetězec nebo prázdný řetězec při chybě
 */
String EEPROMManager::readString(int startAddr)
{
    int len = EEPROM.read(startAddr);
    if (len < 0 || len > maxStringLength)
        return "";
    String result = "";
    for (int i = 0; i < len; i++)
    {
        char c = EEPROM.read(startAddr + 1 + i);
        result += c;
    }
    return result;
}
