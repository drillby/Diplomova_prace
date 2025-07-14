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
     * @param data Řetězec k zápisu
     */
    static void writeString(int startAddr, const String &data);

    /**
     * @brief Načte řetězec z EEPROM ze specifikované adresy
     * @param startAddr Počáteční adresa v EEPROM
     * @return Načtený řetězec nebo prázdný řetězec při chybě
     */
    static String readString(int startAddr);
};

#endif // EEPROM_MANAGER_H
