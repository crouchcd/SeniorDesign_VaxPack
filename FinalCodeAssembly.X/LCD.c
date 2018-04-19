#include "xc.h"
#define FCY 4000000UL //FCY = FOSC / 2 unsigned long (UL) 
// PC24FJ128GA204 = 8MHz internal oscillator
// must be defined before importing <libpic30.h>
#include <libpic30.h>
#include <string.h>
#define DEGREE_ALIAS 0x002B // is the '+' sign acting in place of the ° symbol

#include "LCD.h"

void LCD_ClearCommand() {
    /*
     * This function clears the RC8 bit for Command Mode, Sets E (RC9), and Sets RC0
     * to complete the clear command
     */

    //clear display
    LATC = 0x0201; //0000 0010 0000 0001
    __delay_ms(1);
    LATCbits.LATC9 = 0; // toggle E pin
    __delay_ms(1);
}

//address can only be a number between $00 - $0F OR $40 - $4F

void LCD_SetDisplayAddressCommand(int address) {
    /*
     * This function clears the RC8 bit for Command Mode & Sets E (RC9)
     * RC7 must be set; RC0 - RC6 is used to set the address to a value between
     * $00-0F OR $40-$4F
     * @param: address: must be between $00-0F or $40-4F
     */

    LATC = address; // sets the LCD display address
    LATC |= 0x0280; // 0000 0010 1000 0000 Required configuration
    __delay_ms(1);
    LATCbits.LATC9 = 0; // toggle E pin
    __delay_ms(1);
}

void LCD_Init() {
    /*
     * This function clears the RC8 bit for Command Mode & Sets E (RC9), Reference the LCD
     * documentation for the listing of commands
     */

    //display on command, underline cursor, blink cursor
    LCD_DisplayOptions(0x000F); // ... 0000 1111

    //set function command
    LATC = 0x023F; // 0000 0010 0011 1111
    __delay_ms(1);
    LATCbits.LATC9 = 0; // toggle E pin
    __delay_ms(1);
}

void LCD_DisplayOptions(int options) {
    /*
     * With RS - RC8 and E - RC9 - 
     * @param: options declares the display/cursor options must be between
     * x08 and x0F
     */
    if (options < 0x0008 || options > 0x000F) return;
    LATC = 0x0200 | options; // 0000 0010 0000 1DUB
     __delay_ms(1);
    LATCbits.LATC9 = 0; // toggle E pin
    __delay_ms(1);
}

void LCD_PrintString(char *str) {
    /*
     * This function takes a char[] (null terminated) and outputs each char in
     * the array to the LCD controller. Special characters require a different
     * method. RS needs to be set for data transfer
     * @param: str: the string to be output on the LCD 
     */
    char ch;
    int i = 0;

    for (; i < strlen(str); i++) {
        ch = str[i];
        if (ch == DEGREE_ALIAS) {
            //it is the ° alias symbol, so print it manually
            LCD_PrintChar(ch);
        } else {
            LATC = ch;
            LATC |= 0x0300; // 0000 0011 ---- ---- RS set for Data Mode
            __delay_ms(1);
            LATCbits.LATC9 = 0; // toggle E pin
            // delay?
        }
    }
}

void LCD_PrintChar(char ch) {
    /*
     * This function takes a char (mainly special chars like the degree symbol)
     * and outputs it to the LCD controller. This has to be done by hard-coding
     * the hex value of the char to the LCD data bus
     * RS needs to be set for data transfer
     * @param: ch: the char to be output to the LCD
     */

    int charHexValue;

    //degree symbol
    if (ch == DEGREE_ALIAS) {
        charHexValue = 0x00DF;
    } else {
        // or something to make sure the value has a valid hex value
        charHexValue = ch;
    }

    LATC = charHexValue;
    LATC |= 0x0300; // 0000 0011 ---- ---- RS and E set
    __delay_ms(1);
    LATCbits.LATC9 = 0; // toggle E
    __delay_ms(1);
}

void LCD_PrintInteger(int num) {
    if (num / 1000 >= 1) {
        switch (num / 1000) {
            case 1:
                LCD_PrintChar('1');
                break;
            case 2:
                LCD_PrintChar('2');
                break;
            case 3:
                LCD_PrintChar('3');
                break;
            case 4:
                LCD_PrintChar('4');
                break;
            case 5:
                LCD_PrintChar('5');
                break;
            case 6:
                LCD_PrintChar('6');
                break;
            case 7:
                LCD_PrintChar('7');
                break;
            case 8:
                LCD_PrintChar('8');
                break;
            case 9:
                LCD_PrintChar('9');
                break;
        }
        if ((num / 100) % 10 == 0)
            LCD_PrintChar('0');
        if ((num / 10) % 100 == 0)
            LCD_PrintChar('0');
        LCD_PrintInteger(num % 1000);
    } else if (num / 100 >= 1) {
        switch (num / 100) {
            case 1:
                LCD_PrintChar('1');
                break;
            case 2:
                LCD_PrintChar('2');
                break;
            case 3:
                LCD_PrintChar('3');
                break;
            case 4:
                LCD_PrintChar('4');
                break;
            case 5:
                LCD_PrintChar('5');
                break;
            case 6:
                LCD_PrintChar('6');
                break;
            case 7:
                LCD_PrintChar('7');
                break;
            case 8:
                LCD_PrintChar('8');
                break;
            case 9:
                LCD_PrintChar('9');
                break;
        }
        if ((num / 10) % 10 == 0)
            LCD_PrintChar('0');
        LCD_PrintInteger(num % 100);
    } else if (num / 10 >= 1) {
        switch (num / 10) {
            case 1:
                LCD_PrintChar('1');
                break;
            case 2:
                LCD_PrintChar('2');
                break;
            case 3:
                LCD_PrintChar('3');
                break;
            case 4:
                LCD_PrintChar('4');
                break;
            case 5:
                LCD_PrintChar('5');
                break;
            case 6:
                LCD_PrintChar('6');
                break;
            case 7:
                LCD_PrintChar('7');
                break;
            case 8:
                LCD_PrintChar('8');
                break;
            case 9:
                LCD_PrintChar('9');
                break;
        }
        LCD_PrintInteger(num % 10);
    } else {
        switch (num) {
            case 0:
                LCD_PrintChar('0');
                break;
            case 1:
                LCD_PrintChar('1');
                break;
            case 2:
                LCD_PrintChar('2');
                break;
            case 3:
                LCD_PrintChar('3');
                break;
            case 4:
                LCD_PrintChar('4');
                break;
            case 5:
                LCD_PrintChar('5');
                break;
            case 6:
                LCD_PrintChar('6');
                break;
            case 7:
                LCD_PrintChar('7');
                break;
            case 8:
                LCD_PrintChar('8');
                break;
            case 9:
                LCD_PrintChar('9');
                break;
        }
    }
}

void LCD_PrintFloat(float num) {
    LCD_PrintInteger(num);
    LCD_PrintChar('.');
    int floatingPoint = ((num - (int)num) * 100);
    LCD_PrintInteger(floatingPoint);
}
