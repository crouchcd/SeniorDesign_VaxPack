/* 
 * File:   main.h
 * Author: ccrouch
 *
 * Created on April 18, 2018, 11:48 PM
 */

#ifndef MAIN_H
#define	MAIN_H

#ifdef	__cplusplus
extern "C" {
#endif

void updateDisplayAfterKeypress(char keypress);
bool updateTensPlace(int keyValue);
bool updateOnesPlace(int keyValue);
void displayData();
void errorMessage();
void initializeKeypadRowPins();
void initRelayTriggers();
void getTargetTemperature();

int actualTemp = 36;
int batteryChargeStatus = 56;
int userDesiredTemp = 0;
bool isCursorInTensPlace = true;
bool userTempSelected = false;

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


#ifdef	__cplusplus
}
#endif

#endif	/* MAIN_H */

