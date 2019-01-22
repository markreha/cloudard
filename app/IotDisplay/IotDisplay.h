#ifndef IOTDISPLAY_h
#define IOTDISPLAY_h

// Adjust for Wifi
#define SECRET_SSID "ReplaceME"
#define SECRET_PASS "ReplaceME"

extern void initializeDisplay(int ledSize, int ledBorder, bool landscape);
extern void displayWelcomeMessage(String msg1, String msg2);
extern void clearDisplay();
extern int getNumberLEDColumns();
extern int getNumberLEDRows();
extern void displayLED(int x, int y, int color);

#endif
