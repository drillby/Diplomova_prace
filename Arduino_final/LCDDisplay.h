#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include "rgb_lcd.h"

/**
 * @class LCDDisplay
 * @brief Třída pro ovládání Grove-16x2 LCD displeje
 */
class LCDDisplay
{
private:
    rgb_lcd lcd;        // Instance LCD objektu
    bool isInitialized; // Stav inicializace displeje
    int currentRow;     // Aktuální řádek kurzoru
    int currentCol;     // Aktuální sloupec kurzoru
    int displayCols;    // Počet sloupců displeje
    int displayRows;    // Počet řádků displeje

public:
    /**
     * @brief Konstruktor LCDDisplay
     */
    LCDDisplay();

    /**
     * @brief Destruktor LCDDisplay
     */
    ~LCDDisplay();

    /**
     * @brief Inicializuje LCD displej
     * @param cols Počet sloupců (default 16)
     * @param rows Počet řádků (default 2)
     * @return True pokud byla inicializace úspěšná
     */
    bool begin(int cols = 16, int rows = 2);

    /**
     * @brief Vypíše text na displej s možností formátování
     * @param text Text k zobrazení (C-string)
     */
    void print(const char *text);

    /**
     * @brief Vypíše text na displej s možností formátování (F macro support)
     * @param text Text k zobrazení (flash string)
     */
    void print(const __FlashStringHelper *text);

    /**
     * @brief Vypíše číslo na displej
     * @param number Číslo k zobrazení
     */
    void print(int number);

    /**
     * @brief Vypíše float číslo na displej
     * @param number Float číslo k zobrazení
     * @param decimals Počet desetinných míst (default 2)
     */
    void print(float number, int decimals = 2);

    /**
     * @brief Vypíše text na displej a přejde na nový řádek (C-string)
     * @param text Text k zobrazení
     */
    void println(const char *text);

    /**
     * @brief Vypíše text na displej a přejde na nový řádek (F macro support)
     * @param text Text k zobrazení (flash string)
     */
    void println(const __FlashStringHelper *text);

    /**
     * @brief Nastaví pozici kurzoru
     * @param col Sloupec (0-15)
     * @param row Řádek (0-1)
     */
    void setCursor(int col, int row);

    /**
     * @brief Vymaže obsah displeje
     */
    void clear();

    /**
     * @brief Vrátí kurzor na domovskou pozici (0,0)
     */
    void home();

    /**
     * @brief Zapne zobrazování kurzoru
     */
    void showCursor();

    /**
     * @brief Vypne zobrazování kurzoru
     */
    void hideCursor();

    /**
     * @brief Zapne blikání kurzoru
     */
    void blinkCursor();

    /**
     * @brief Vypne blikání kurzoru
     */
    void noBlink();

    /**
     * @brief Zapne displej
     */
    void displayOn();

    /**
     * @brief Vypne displej (ale ponechá podsvícení)
     */
    void displayOff();

    /**
     * @brief Posune obsah displeje doleva
     */
    void scrollLeft();

    /**
     * @brief Posune obsah displeje doprava
     */
    void scrollRight();

    /**
     * @brief Vypíše text na konkrétní pozici (C-string)
     * @param col Sloupec (0-15)
     * @param row Řádek (0-1)
     * @param text Text k zobrazení
     */
    void printAt(int col, int row, const char *text);

    /**
     * @brief Vypíše text na konkrétní pozici (F macro support)
     * @param col Sloupec (0-15)
     * @param row Řádek (0-1)
     * @param text Text k zobrazení (flash string)
     */
    void printAt(int col, int row, const __FlashStringHelper *text);

    /**
     * @brief Kontroluje, zda je displej inicializovaný
     * @return True pokud je inicializovaný
     */
    bool isReady() const;

    /**
     * @brief Vrací aktuální pozici kurzoru
     * @param col Reference na proměnnou pro sloupec
     * @param row Reference na proměnnou pro řádek
     */
    void getCursorPosition(int &col, int &row) const;

    /**
     * @brief Vrací rozměry displeje
     * @param cols Reference na proměnnou pro počet sloupců
     * @param rows Reference na proměnnou pro počet řádků
     */
    void getDisplaySize(int &cols, int &rows) const;
};

#endif // LCD_DISPLAY_H
