#include "mcc_generated_files/mcc.h"
#define FCY 4000000UL //FCY = FOSC / 2 unsigned long (UL) 
/* PC24FJ128GA204 = 8MHz internal oscillator
 * must be defined before importing <libpic30.h> */
#include <libpic30.h>
#include "LCD.h"
#include "xc.h"
#include "main.h"

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
    getTargetTemperature();

    while (1) {
        if (userTempSelected) {
            LCD_DisplayOptions(0x000C); //display on, underline off, blink off
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
    }
    return -1;
}

void getTargetTemperature() {
    LCD_DisplayOptions(0x000F); // everything on
    LCD_SetDisplayAddressCommand(0x0040);
    LCD_PrintString("(32 to 45+F)");
    LCD_SetDisplayAddressCommand(0x0000);
    LCD_PrintString("Enter Temp: ");
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
    /*
     * the userTempSelected bool determines when the user has entered ONE
     * valid, two digit temperature. After that point, normal execution flows. If
     * ONE valid temp has already been entered, the user must reinitialize the
     * program in order to change their temperature
     */
    if (!userTempSelected) {
        // convert char to int
        int keyValue = keypress - '0';

        if (keyValue >= 0 && keyValue <= 9) {
            bool isUpdateSuccessful = false;
            // update variable in memory
            if (isCursorInTensPlace) {
                isUpdateSuccessful = updateTensPlace(keyValue);
                // if update successful, move cursor to ones place
                if (isUpdateSuccessful) isCursorInTensPlace = false;
            } else {
                isUpdateSuccessful = updateOnesPlace(keyValue);
                // if update successful, then the user has entered a valid temp
                if (isUpdateSuccessful) userTempSelected = true;
            }

            // display if the keyValue was accepted
            if (isUpdateSuccessful) {
                // if not a completely valid temp, output the first value
                if (!userTempSelected) LCD_PrintInteger(userDesiredTemp);
                    // if a valid temp, output the one's place
                else LCD_PrintInteger(keyValue);
            }

        }
    }
}

bool updateTensPlace(int keyValue) {
    bool isUpdateSuccessful = false;
    if (keyValue <= 4 && keyValue >= 3) {
        // only tens place values from 3 to 4 are acceptable
        userDesiredTemp = keyValue;
        isUpdateSuccessful = true;
    }
    return isUpdateSuccessful;
}

bool updateOnesPlace(int keyValue) {
    bool isUpdateSuccessful = false;
    if ((userDesiredTemp == 3 && keyValue >= 2) || (userDesiredTemp == 4 && keyValue <= 5)) {
        // only 32 to 45 are acceptable
        userDesiredTemp = userDesiredTemp * 10; // e.g. 3 => 30
        userDesiredTemp = userDesiredTemp + keyValue; // e.g. 30 + 2 => 32
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
}