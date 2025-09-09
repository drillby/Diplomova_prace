#include "LCDDisplay.h"
#include "Config.h"
#include "Utils.h"

/**
 * @brief Konstruktor LCDDisplay
 */
LCDDisplay::LCDDisplay() : isInitialized(false), currentRow(0), currentCol(0), displayCols(16), displayRows(2)
{
}

/**
 * @brief Destruktor LCDDisplay
 */
LCDDisplay::~LCDDisplay()
{
    // Vyčištění a vypnutí displeje při destrukci
    if (isInitialized)
    {
        clear();
        displayOff();
    }
}

/**
 * @brief Inicializuje LCD displej
 * @param cols Počet sloupců (default 16)
 * @param rows Počet řádků (default 2)
 * @return True pokud byla inicializace úspěšná
 */
bool LCDDisplay::begin(int cols, int rows)
{
    displayCols = cols;
    displayRows = rows;

    // Inicializace I2C komunikace
    Wire.begin();

    // Inicializace LCD displeje
    lcd.begin(displayCols, displayRows);

    // Nastavení základního stavu
    clear();
    home();
    displayOn();
    hideCursor();
    noBlink();

    isInitialized = true;

    printIfPinLow(F("LCD displej inicializován úspěšně"), debugPin);
    return true;
}

/**
 * @brief Vypíše text na displej s možností formátování (C-string)
 * @param text Text k zobrazení
 */
void LCDDisplay::print(const char *text)
{
    if (!isInitialized)
        return;
    lcd.print(text);
}

/**
 * @brief Vypíše text na displej s možností formátování (F macro support)
 * @param text Text k zobrazení (flash string)
 */
void LCDDisplay::print(const __FlashStringHelper *text)
{
    if (!isInitialized)
        return;
    lcd.print(text);
}

/**
 * @brief Vypíše číslo na displej
 * @param number Číslo k zobrazení
 */
void LCDDisplay::print(int number)
{
    if (!isInitialized)
        return;
    lcd.print(number);
}

/**
 * @brief Vypíše float číslo na displej
 * @param number Float číslo k zobrazení
 * @param decimals Počet desetinných míst (default 2)
 */
void LCDDisplay::print(float number, int decimals)
{
    if (!isInitialized)
        return;
    lcd.print(number, decimals);
}

/**
 * @brief Vypíše text na displej a přejde na nový řádek (C-string)
 * @param text Text k zobrazení
 */
void LCDDisplay::println(const char *text)
{
    if (!isInitialized)
        return;

    print(text);

    // Přesun na další řádek
    currentRow++;
    if (currentRow >= displayRows)
    {
        currentRow = 0; // Wrap na první řádek
    }
    setCursor(0, currentRow);
}

/**
 * @brief Vypíše text na displej a přejde na nový řádek (F macro support)
 * @param text Text k zobrazení (flash string)
 */
void LCDDisplay::println(const __FlashStringHelper *text)
{
    if (!isInitialized)
        return;

    print(text);

    // Přesun na další řádek
    currentRow++;
    if (currentRow >= displayRows)
    {
        currentRow = 0; // Wrap na první řádek
    }
    setCursor(0, currentRow);
}

/**
 * @brief Nastaví pozici kurzoru
 * @param col Sloupec (0-15)
 * @param row Řádek (0-1)
 */
void LCDDisplay::setCursor(int col, int row)
{
    if (!isInitialized)
        return;

    // Omezení na platné rozsahy
    col = constrain(col, 0, displayCols - 1);
    row = constrain(row, 0, displayRows - 1);

    currentCol = col;
    currentRow = row;

    lcd.setCursor(col, row);
}

/**
 * @brief Vymaže obsah displeje
 */
void LCDDisplay::clear()
{
    if (!isInitialized)
        return;

    lcd.clear();
    currentCol = 0;
    currentRow = 0;
}

/**
 * @brief Vrátí kurzor na domovskou pozici (0,0)
 */
void LCDDisplay::home()
{
    if (!isInitialized)
        return;

    lcd.home();
    currentCol = 0;
    currentRow = 0;
}

/**
 * @brief Zapne zobrazování kurzoru
 */
void LCDDisplay::showCursor()
{
    if (!isInitialized)
        return;
    lcd.cursor();
}

/**
 * @brief Vypne zobrazování kurzoru
 */
void LCDDisplay::hideCursor()
{
    if (!isInitialized)
        return;
    lcd.noCursor();
}

/**
 * @brief Zapne blikání kurzoru
 */
void LCDDisplay::blinkCursor()
{
    if (!isInitialized)
        return;
    lcd.blink();
}

/**
 * @brief Vypne blikání kurzoru
 */
void LCDDisplay::noBlink()
{
    if (!isInitialized)
        return;
    lcd.noBlink();
}

/**
 * @brief Zapne displej
 */
void LCDDisplay::displayOn()
{
    if (!isInitialized)
        return;
    lcd.display();
}

/**
 * @brief Vypne displej (ale ponechá podsvícení)
 */
void LCDDisplay::displayOff()
{
    if (!isInitialized)
        return;
    lcd.noDisplay();
}

/**
 * @brief Posune obsah displeje doleva
 */
void LCDDisplay::scrollLeft()
{
    if (!isInitialized)
        return;
    lcd.scrollDisplayLeft();
}

/**
 * @brief Posune obsah displeje doprava
 */
void LCDDisplay::scrollRight()
{
    if (!isInitialized)
        return;
    lcd.scrollDisplayRight();
}

/**
 * @brief Vypíše text na konkrétní pozici (C-string)
 * @param col Sloupec (0-15)
 * @param row Řádek (0-1)
 * @param text Text k zobrazení
 */
void LCDDisplay::printAt(int col, int row, const char *text)
{
    if (!isInitialized)
        return;

    setCursor(col, row);
    print(text);
}

/**
 * @brief Vypíše text na konkrétní pozici (F macro support)
 * @param col Sloupec (0-15)
 * @param row Řádek (0-1)
 * @param text Text k zobrazení (flash string)
 */
void LCDDisplay::printAt(int col, int row, const __FlashStringHelper *text)
{
    if (!isInitialized)
        return;

    setCursor(col, row);
    print(text);
}

/**
 * @brief Kontroluje, zda je displej inicializovaný
 * @return True pokud je inicializovaný
 */
bool LCDDisplay::isReady() const
{
    return isInitialized;
}

/**
 * @brief Vrací aktuální pozici kurzoru
 * @param col Reference na proměnnou pro sloupec
 * @param row Reference na proměnnou pro řádek
 */
void LCDDisplay::getCursorPosition(int &col, int &row) const
{
    col = currentCol;
    row = currentRow;
}

/**
 * @brief Vrací rozměry displeje
 * @param cols Reference na proměnnou pro počet sloupců
 * @param rows Reference na proměnnou pro počet řádků
 */
void LCDDisplay::getDisplaySize(int &cols, int &rows) const
{
    cols = displayCols;
    rows = displayRows;
}
