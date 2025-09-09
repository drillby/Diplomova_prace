#ifndef EEPROM_MANAGER_H
#define EEPROM_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

/**
 * @class EEPROMManager
 * @brief Třída pro správu dat v EEPROM paměti
 */
class EEPROMManager
{
public:
    /**
     * @brief Inicializuje EEPROM
     */
    static void begin();

    /**
     * @brief Zapíše řetězec do EEPROM na specifikovanou adresu
     * @param startAddr Počáteční adresa v EEPROM
     * @param data Řetězec k zápisu (C-string)
     */
    static void writeString(int startAddr, const char *data);

    /**
     * @brief Načte řetězec z EEPROM ze specifikované adresy
     * @param startAddr Počáteční adresa v EEPROM
     * @param buffer Buffer pro načtený řetězec
     * @param bufferSize Velikost bufferu
     * @return Počet načtených znaků nebo -1 při chybě
     */
    static int readString(int startAddr, char *buffer, int bufferSize);
};

#endif // EEPROM_MANAGER_H
