/**
 * NAME: IotDisplay.h
 * DESCRIPTION: Header file for the Utility library to support the LCD Display.
 * 
 * AUTHOR: Professor Mark Reha
 * VERSION: 1.0.0   Initial release
 * COPYRIGHT: On The Edge Software Consulting Services 2019.  All rights reserved.
 * 
 */

#ifndef IOTDISPLAY_h
#define IOTDISPLAY_h

// Adjust for Wifi
#define SECRET_SSID "ReplaceME"
#define SECRET_PASS "ReplaceME"

extern void initializeDisplay(int ledSize, int ledBorder, bool landscape);
extern void displayWelcomeMessage(String msg1, String msg2);
extern void displayMessage(int row, int column, String msg, int fontSize, int color);
extern void clearDisplay();
extern int getNumberLEDColumns();
extern int getNumberLEDRows();
extern void displayLED(int x, int y, int color);

#endif
