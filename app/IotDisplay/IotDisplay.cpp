/**
 * NAME: IotDisplay.c
 * DESCRIPTION: Utility library to support the LCD Display.
 * 
 * AUTHOR: Professor Mark Reha
 * VERSION: 1.0.0   Initial release
 * COPYRIGHT: On The Edge Software Consulting Services 2019.  All rights reserved.
 * 
 */
 
// Adjust these flags to reduce program size
#define USE_FONTS 0
#define TRIM_APP false

#include <Adafruit_GFX.h>;
#include <MCUFRIEND_kbv.h>;
#if USE_FONTS == 1
  #include <Fonts/FreeSansBold9pt7b.h>
  #include <Fonts/FreeSansBold24pt7b.h>
#endif

int LED_SIZE = 0;
int LED_BORDER = 0;
bool IS_LANDSCAPE = true;

MCUFRIEND_kbv tft;
int screenWidth, screenHeight;

/**
 * NAME: initializeDisplay()
 * DESCRIPTION: Initialize the LCD Display.
 * PROCESS:       Resets and initializes the LCD Display (reset, screen rotation, clear screen)
 *                Initalizes runtime variables (LED Size, Landscape Mode, and Screen Size)
 * INPUTS: 
 *      ledSize Size in square pixels of a LED
 *      ledBorder Width in pixels of the border around a LED
 *      landscape True if landscape mode else False for portrait mode
 * OUTPUTS: 
 *      None
 * 
 */
void initializeDisplay(int ledSize, int ledBorder, bool landscape)
{
  // Initialize the LCD Display
  tft.reset();
  tft.begin(tft.readID());
  tft.setRotation(landscape);
  tft.fillScreen(TFT_BLACK);

  // Initialize runtime variables
  LED_SIZE = ledSize;
  LED_BORDER = ledBorder;
  IS_LANDSCAPE = landscape;
  screenWidth = tft.width();
  screenHeight = tft.height();
}

/**
 * NAME: cleareDisplay()
 * DESCRIPTION: Clear the LCD Display.
 * PROCESS:       Fills the screen with black pixels
 * INPUTS: 
 *      None
 * OUTPUTS: 
 *      None
 * 
 */
void clearDisplay()
{
  // Clear Display by filling with black
  tft.fillScreen(TFT_BLACK);
}

/**
 * NAME: getNumberLEDColumns()
 * DESCRIPTION: Calculated the number LED Columns give the LED Size and Screen Size.
 * PROCESS:       Calulation screenWidth/LED_SIZE
 * INPUTS: 
 *      None
 * OUTPUTS: 
 *      Number LED Columns that can fit on the screen
 * 
 */
int getNumberLEDColumns()
{
  // Return number of Columns in the Display Grid
  return screenWidth/LED_SIZE;
}

/**
 * NAME: getNumberLEDRows()
 * DESCRIPTION: Calculated the number LED Rows give the LED Size and Screen Size.
 * PROCESS:       Calulation screenHeight/LED_SIZE
 * INPUTS: 
 *      None
 * OUTPUTS: 
 *      Number LED Rows that can fit on the screen
 * 
 */
int getNumberLEDRows()
{
  // Return number of Rows in the Display Grid
  return screenHeight/LED_SIZE;
}

/**
 * NAME: displayWelcomeMessage()
 * DESCRIPTION: Displays a desired 2 line Welcome Message on the screen.
 * PROCESS:       Clear the screen
 *                Sets the Font, Text Color, and Location and display Welcome Message Line 1
 *                Sets the Font, Text Color, and Location and display Welcome Message Line 2
 *                Displays the GCU Banner centered on the screen
 * INPUTS: 
 *      msg1  Line 1 of the Welcome Message
 *      msg2  Line 2 of the Welcome Message
 * OUTPUTS: 
 *      None
 * 
 */
void displayWelcomeMessage(String msg1, String msg2)
{
  // Clear the Display
  clearDisplay();
  
  // Display Welcome Message
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 20);
  tft.setTextColor(TFT_PURPLE);
#if USE_FONTS == 1
  tft.setFont(&FreeSansBold9pt7b);
#else
  tft.setTextSize(IS_LANDSCAPE ? 3 : 2);
#endif
  tft.println(msg1);
  tft.println("");
  tft.setTextColor(TFT_WHITE);
#if USE_FONTS == 1
   tft.setFont(&FreeSansBold9pt7b);
#else
  tft.setTextSize(IS_LANDSCAPE ? 2 : 1);
#endif
  tft.println(msg2);
  delay(1000);

  // Display GCU Banner centered on the Display
#if USE_FONTS == 1
  int x1, y1, w, h;
  tft.setFont(&FreeSansBold24pt7b);
  tft.getTextBounds("GCU", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((screenWidth/2) - w/2, (screenHeight/2) + h/2);
#else
  int x1, y1, w, h;
  tft.setTextSize(8);
  tft.getTextBounds("GCU", 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((screenWidth/2) - w/2, (screenHeight/2) - h/2);
#endif
  tft.setTextColor(TFT_PURPLE);
  tft.print("G");
  delay(1000);
  tft.setTextColor(TFT_WHITE);
  tft.print("C");
  delay(1000);
  tft.setTextColor(TFT_PURPLE);
  tft.print("U");
  delay(1000);
}

/**
 * NAME: displayMessage()
 * DESCRIPTION: Displays a desired Message on the screen at a location, font, and color.
 * PROCESS:       Sets the Font, Text Color, and Location and display Message
 * INPUTS: 
 *      row  Pixel location X for the Message
 *      column  Pixel location Y for the Message
 *      msg The Message to display
 *      fontSize  The font size for the Message
 *      color The color for the Message
 * OUTPUTS: 
 *      None
 * 
 */
void displayMessage(int row, int column, String msg, int fontSize, int color)
{
  int x1, y1, w, h;

  tft.setTextSize(fontSize);
  tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor(w * column, h * row);
  tft.setTextColor(TFT_WHITE);
  tft.print(msg); 
}

/**
 * NAME: displayLED()
 * DESCRIPTION: Displays a LED on the screen at a location, and color.
 * PROCESS:       If x and y are 0 then clear the display
 *                Draw a rectangle on the screen in the specified Color and Location (using the calculated LED Size)
 * INPUTS: 
 *      x  Row location for the LED
 *      y  Column location for the LED
 *      color The color for the LED
 * OUTPUTS: 
 *      None
 * 
 */
void displayLED(int x, int y, int color)
{
  // Clear the Display if at the top of the Grid
  if(x == 0 && y == 0)
    clearDisplay();

  // Display Rectangle with desired size, border, and color
  tft.fillRect((x * LED_SIZE) + LED_BORDER, (y * LED_SIZE) + LED_BORDER, LED_SIZE - LED_BORDER, LED_SIZE - LED_BORDER, color); 
}
