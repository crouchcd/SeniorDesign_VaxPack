#include "mcc_generated_files/mcc.h"
#define FCY 4000000UL //FCY = FOSC / 2 unsigned long (UL) 
/* PC24FJ128GA204 = 8MHz internal oscillator
 * must be defined before importing <libpic30.h> */
#include <libpic30.h>
#include "LCD.h"
#include "xc.h"

void updateDisplayAfterKeypress(char keypress);
bool updateTensPlace(int keyValue);
bool updateOnesPlace(int keyValue);
void displayData();
void errorMessage();
void initializeKeypadRowPins();
void initRelayTriggers();

int actualTemp = 36;
int batteryChargeStatus = 56;
int userDesiredTemp = 37;
bool isCursorInTensPlace = true;

#define TEMPERATURE_CHANNEL 4 // RB2
#define BATTERY_CHANNEL 5 // RB3
#define SetADCForExternalReference() AD1CON2 = 0x6000; // for temperature
#define SetADCForInternalReference() AD1CON2 = 0x0000; // for battery level
#define ARRAY_SIZE 15
#define TEMP_REF_HI 1.75
#define TEMP_REF_LOW 0.1
#define Vdd 3.118
#define BAT_REF_HI 3.118 /* 100% battery charge level - look at Vscaled when supply is 16.8V
                            should be as close to Vdd as possible */ 
#define BAT_REF_LOW 2.6 // 0% battery charge level - look at Vscaled when supply is 14V
#define TEMP_CORRECTION 3 // temp seems to be off by a few degrees
#define BAT_CORRECTION 0 // off by +5 to +7 degrees (power suppply) (0 for battery)
#define COOLING_RELAY LATBbits.LATB14
#define TEC_RELAY LATAbits.LATA7
#define COOLDOWN_TIME 500 // approx 1.50 seconds

int main(void) {
    // initialize the device
    SYSTEM_Initialize();
    LCD_Init();
    LCD_ClearCommand();
    initializeKeypadRowPins();
    float tempRef = TEMP_REF_HI - TEMP_REF_LOW;
    int ADCvalue = 0;
    float Vout = 0.0;
    float batteryScaled;
    short arrayElemCounter = 0;
    int temps[ARRAY_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int battLevels[ARRAY_SIZE] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    bool isTEC_off = true;
    int cooldownTimer = 0;
    initRelayTriggers();

    while (1) {

        SetADCForExternalReference();
        ADCvalue = ADC1_GetConversion(TEMPERATURE_CHANNEL);
        Vout = ADCvalue * (tempRef / 4095);
        /*Vout is originally in centigrade, 50 is subtracted to
         * get the reading as its centigrade value, then convert to F */
        Vout = ((Vout / 0.01) - 50) * 1.8 + 32;
        actualTemp = Vout;

        SetADCForInternalReference();
        ADCvalue = ADC1_GetConversion(BATTERY_CHANNEL);
        batteryScaled = ADCvalue * (Vdd / 4095);
        batteryChargeStatus = ((batteryScaled - BAT_REF_LOW) / (BAT_REF_HI - BAT_REF_LOW)) * 100;

        // run the cool-down system for X number of seconds after the TEC turns off
        if (isTEC_off) {
            if (cooldownTimer > COOLDOWN_TIME) {
                if (COOLING_RELAY == 1) COOLING_RELAY = 0;
            } else {
                cooldownTimer++;
            }
        }

        if (arrayElemCounter < ARRAY_SIZE) {
            // add temp and battery readings to an array
            temps[arrayElemCounter] = actualTemp;
            battLevels[arrayElemCounter] = batteryChargeStatus;
            arrayElemCounter++;
            __delay_ms(100);
        } else {
            // average temperatures and bat. levels
            short i = 0;
            int tempSum = 0, battSum = 0;
            for (; i < ARRAY_SIZE; i++) {
                tempSum += temps[i];
                battSum += battLevels[i];
            }
            actualTemp = (tempSum / ARRAY_SIZE) + TEMP_CORRECTION;
            batteryChargeStatus = (battSum / ARRAY_SIZE) - BAT_CORRECTION;
            if (batteryChargeStatus < 0) batteryChargeStatus = 0;

            // determine whether to keep the TEC/Cool-down system on/off
            if (actualTemp >= (userDesiredTemp + 5)) {
                // if actual temp gets 5 or more above target, power TEC and COOLING
                if (COOLING_RELAY == 0) COOLING_RELAY = 1;
                if (TEC_RELAY == 0) TEC_RELAY = 1;
                isTEC_off = false;
                cooldownTimer = 0;
            } else if (actualTemp <= (userDesiredTemp - 5)) {
                // if actual temp gets 5 or less below target, cut TEC power
                // ... and start the cool-down timer
                if (TEC_RELAY == 1) TEC_RELAY = 0;
                isTEC_off = true;
            }
            // else {
            // do nothing, leave TEC and COOLING systems on
            // }
            LCD_ClearCommand();
            displayData();
            arrayElemCounter = 0;
        }
    }
    return -1;
}

void initRelayTriggers() {
    // initialize RA7 and RB14 for relays (RB15 pin has a bad connection)
    TRISBbits.TRISB14 = 0; // for Cool-down
    ANSBbits.ANSB14 = 0;
    TRISAbits.TRISA7 = 0; // for TEC
    COOLING_RELAY = 0; 
    TEC_RELAY = 0;
}

void initializeKeypadRowPins() {

    /*
     * Digital/Analog (ANSx) Output/Input (TRISx)
     *  set in pin_manager.c
     * TRISx: '0' = output
     * ANSx: '0' = digital
     */

    // initially set keypad row pins to HIGH
    LATBbits.LATB10 = 1; // ROW0
    LATBbits.LATB11 = 1;
    LATBbits.LATB12 = 1;
    LATBbits.LATB13 = 1; // ROW3
}

void updateDisplayAfterKeypress(char keypress) {
    // convert char to int
    int keyValue = keypress - '0';

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
            LCD_ClearCommand();
            displayData();
        }

    } else {
//        errorMessage();
    }
}

bool updateTensPlace(int keyValue) {
    bool isUpdateSuccessful = false;
    if (keyValue > 4 || keyValue < 3) {
//        errorMessage();
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
//        errorMessage();
    } else if ((userDesiredTemp / 10) == 4 && keyValue > 5) {
//        errorMessage();
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
    LCD_PrintString("TARGET:");

    LCD_PrintInteger(userDesiredTemp);
    LCD_PrintString("+F");

    if (isCursorInTensPlace) {
        LCD_SetDisplayAddressCommand(0x0047);
    } else {
        LCD_SetDisplayAddressCommand(0x0048);
    }
}

void errorMessage() {
    LCD_PrintString("Invalid Key");
    LCD_SetDisplayAddressCommand(0x0040);
    LCD_PrintString("range: 32 - 45+F");
}