#include "mcc_generated_files/mcc.h"
#define FCY 4000000UL //FCY = FOSC / 2 unsigned long (UL) 
// PC24FJ128GA204 = 8MHz internal oscillator
// must be defined before importing <libpic30.h>
#include <libpic30.h>
#include "LCD.h"
#include "xc.h"

#define CORRECTION 3

void updateDisplayAfterKeypress(char keypress);
bool updateTensPlace(int keyValue);
bool updateOnesPlace(int keyValue);
void displayData();
void errorMessage();

int actualTemp = 36;
int batteryChargeStatus = 56;
int userDesiredTemp = 37;
bool isCursorInTensPlace = true;

int main(void) {
    // initialize the device
    SYSTEM_Initialize();
    LCD_Init();
    LCD_ClearCommand();
    // initially set keypad row pins to HIGH
    LATAbits.LATA0 = 1;
    LATAbits.LATA1 = 1;
    LATAbits.LATA7 = 1;
    LATAbits.LATA10 = 1;
    int temps[5] = {0, 0, 0, 0, 0};
    float ref = 1.65;
    int ADCvalue = 0;
    float Vout = 0.0;
    int count = 0;
    int avgTemp;

    while (1) {

        for (; count < 5; count++) {
            ADCvalue = ADC1_GetConversion(4);
            LCD_ClearCommand();
            Vout = ADCvalue * (ref / 4096);
            Vout = Vout - 0.3;
            int temp = (Vout / 0.01) * 1.8 + 32 + CORRECTION; // T(°C) × 1.8 + 32
            temp = temp - 10;
            temps[count] = temp;
        }

        count = 0;

        for (; count < 5; count++) {
            avgTemp += temps[count];
        }

        avgTemp = avgTemp / 5;

        actualTemp = avgTemp;

        displayData();
        count = 0;
        __delay_ms(500);
    }

    return -1;
}

void updateDisplayAfterKeypress(char keypress) {
    // convert char to int
    int keyValue = keypress - '0';
    LCD_ClearCommand();

    if (keyValue >= 0 && keyValue <= 9) {
        bool isUpdateSuccessful = false;

        // update variable in memory
        if (isCursorInTensPlace) {
            isUpdateSuccessful = updateTensPlace(keyValue);
        } else {
            isUpdateSuccessful = updateOnesPlace(keyValue);
        }

        // display if the keyValue was accepted
        if (isUpdateSuccessful) {
            // move cursor position
            isCursorInTensPlace = !isCursorInTensPlace;
            displayData();
        }

    } else {
        errorMessage();
    }
}

bool updateTensPlace(int keyValue) {
    bool isUpdateSuccessful = false;
    if (keyValue > 4 || keyValue < 3) {
        errorMessage();
    } else {
        int onesPlace = userDesiredTemp % 10;
        if ((keyValue == 4 && onesPlace > 2) || (keyValue == 3 && onesPlace < 2)) {
            userDesiredTemp = keyValue * 10 + 2; // automatically set the ones place to 2 if
                                                 // it would be out of range
        } else {
            userDesiredTemp = keyValue * 10 + onesPlace;
        }
        isUpdateSuccessful = true;
    }
    return isUpdateSuccessful;
}

bool updateOnesPlace(int keyValue) {
    bool isUpdateSuccessful = false;
    if ((userDesiredTemp / 10) == 3 && keyValue < 2) {
        errorMessage();
    } else if ((userDesiredTemp / 10) == 4 && keyValue > 2) {
        errorMessage();
    } else {
        userDesiredTemp = (userDesiredTemp / 10) * 10; // e.g. 32 => 30
        userDesiredTemp = userDesiredTemp + keyValue;
        isUpdateSuccessful = true;
    }
    return isUpdateSuccessful;
}

void displayData() {
    LCD_PrintString("ACT:");
    LCD_PrintInteger(actualTemp);

    LCD_PrintString("+ BAT:");
    LCD_PrintInteger(batteryChargeStatus);

    LCD_PrintString("%");

    LCD_SetDisplayAddressCommand(0x0040);
    LCD_PrintString("DES:");

    LCD_PrintInteger(userDesiredTemp);
    LCD_PrintString("+F");

    if (isCursorInTensPlace) {
        LCD_SetDisplayAddressCommand(0x0044);
    } else {
        LCD_SetDisplayAddressCommand(0x0045);
    }
}

void errorMessage() {
    LCD_PrintString("Invalid Key");
    LCD_SetDisplayAddressCommand(0x0040);
    LCD_PrintString("range: 32 - 42+F");
}