/* 
 * File:   LCD.h
 * Author: ccrouch
 *
 * Created on March 14, 2018, 3:40 PM
 */

#ifndef LCD_H
#define	LCD_H

#ifdef	__cplusplus
extern "C" {
#endif

    void LCD_ClearCommand();
    void LCD_Init();
    void LCD_SetDisplayAddressCommand(int address);
    void LCD_DisplayOptions(int options);
    void LCD_PrintString(char *str);
    void LCD_PrintChar(char ch);
    void LCD_PrintInteger(int num);
    void LCD_PrintFloat(float num);

#ifdef	__cplusplus
}
#endif

#endif	/* LCD_H */

