#include "mcc_generated_files/mcc.h"
#define FCY 4000000UL //FCY = FOSC / 2 unsigned long (UL) 
// PC24FJ128GA204 = 8MHz internal oscillator
// must be defined before importing <libpic30.h>
#include <libpic30.h>
#include "LCD.h"
#include "xc.h"

void updateDisplayAfterKeypress(char keypress);
bool updateTensPlace(int keyValue);
bool updateOnesPlace(int keyValue);
void displayData();
void errorMessage();
void initializeKeypadRowPins();

int actualTemp = 36;
int batteryChargeStatus = 56;
int userDesiredTemp = 37;
bool isCursorInTensPlace = true;
// Vdd curiosity board = 3.305V

#define TempSensorCorrection 7
#define TEMPERATURE_CHANNEL 4
#define BATTERY_CHANNEL 5
#define SetADCForExternalReference() AD1CON2 = 0x6000; // for temperature
#define SetADCForInternalReference() AD1CON2 = 0x0000; // for battery level

int main(void) {
    // initialize the device
    SYSTEM_Initialize();
    LCD_Init();
    LCD_ClearCommand();
    initializeKeypadRowPins();
    initializeADCExternalReferenceVoltage();
    float tempRef = 1.72;
//    float batteryRef = 3.305;
    int ADCvalue = 0;
    float Vout = 0.0;
    float batteryPercentage;

    while (1) {

        SetADCForExternalReference();
        // set output high to generate Temp reference voltage
        LATBbits.LATB14 = 1;
        ADCvalue = ADC1_GetConversion(TEMPERATURE_CHANNEL);
        Vout = ADCvalue * (tempRef / 4096);
        Vout = ((Vout / 0.01) - 50) * 1.8 + 32;
        actualTemp = Vout;
        LATBbits.LATB14 = 0;

        SetADCForInternalReference();
        ADCvalue = ADC1_GetConversion(BATTERY_CHANNEL);
        // To scale from 3.3 - 2.5 as 100% to 0% respectively,
        // - OR 0x0FFF - x where x is the bit reading when power supply is at 13.5 V
        // - take (ADCValue - x as defined above) * 100
        batteryPercentage = (ADCvalue / (float) 0x0FFF) * 100;
        batteryChargeStatus = batteryPercentage;
        LCD_ClearCommand();
        displayData();
        __delay_ms(1000);
    }

    return -1;
}

void initializeADCExternalReferenceVoltage() {
    // init port 14 to be digital output for generating
    // temperature reference 
    TRISBbits.TRISB14 = 0;
    ANSBbits.ANSB14 = 0;
    LATBbits.LATB14 = 0;
}

void initializeKeypadRowPins() {

    /*
     * Digital/Analog (ANSx) Output/Input (TRISx)
     *  set in pin_manager.c
     * TRISx: '0' = output
     * ANSx: '0' = digital
     */

    // initially set keypad row pins to HIGH
    LATBbits.LATB10 = 1;
    LATBbits.LATB11 = 1;
    LATBbits.LATB12 = 1;
    LATBbits.LATB13 = 1;
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