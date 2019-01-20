// Adjust these flags to reduce program size
#define USE_FONTS 0
#define TRIM_APP true

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

void clearDisplay()
{
  // Clear Display by filling with black
  tft.fillScreen(TFT_BLACK);
}

int getNumberLEDColumns()
{
  // Return number of Columns in the Display Grid
  return screenWidth/LED_SIZE;
}

int getNumberLEDRows()
{
  // Return number of Rows in the Display Grid
  return screenHeight/LED_SIZE;
}

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
 #if TRIM_APP == false  
   int x1, y1, w, h;
   tft.setTextSize(8);
   tft.getTextBounds("GCU", 0, 0, &x1, &y1, &w, &h);
   tft.setCursor((screenWidth/2) - w/2, (screenHeight/2) - h/2);
 #else
  tft.setTextSize(8);
  if(IS_LANDSCAPE)
      tft.setCursor(168, 128);
   else
     tft.setCursor(88, 208);
 #endif
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

void displayLED(int x, int y, int color)
{
  // Clear the Display if at the top of the Grid
  if(x == 0 && y == 0)
    clearDisplay();

  // Display Rectangle with desired size, border, and color
  tft.fillRect((x * LED_SIZE) + LED_BORDER, (y * LED_SIZE) + LED_BORDER, LED_SIZE - LED_BORDER, LED_SIZE - LED_BORDER, color); 
}
