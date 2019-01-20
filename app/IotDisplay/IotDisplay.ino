#include "IotDisplay.h"

#include <CytronWiFiShield.h>
#include <CytronWiFiClient.h>
#include <CytronWiFiServer.h>
#include <SoftwareSerial.h>
#include <ArduinoHttpClient.h>

// Adjust for Wifi
#define SECRET_SSID "StarBase III"
#define SECRET_PASS "Brianna3325"

// Adjust these settings for desired Display Setup
#define LED_SIZE  20
#define LED_BORDER 2

// Adjust this flag to true to connect to remote Arduino
#define HAS_IOT true

// Program Contants
String appTitle = "IoT Weather Application";
String appVersion = "Version 0.1";


#define BLACK       0x0000      /*   0,   0,   0 */
#define PURPLE      0x780F      /* 128,   0, 128 */
#define RED         0xF800      /* 255,   0,   0 */
#define YELLOW      0xFFE0      /* 255, 255,   0 */
#define WHITE       0xFFFF      /* 255, 255, 255 */

#define PORTRAIT  false
#define LANDSCAPE true

int ledX, ledY = 0;
ESP8266Server server(8081);

void setup() 
{
  // Initilaie the Serial Port
  Serial.begin(9600);
  while(!Serial);

  // Initialize and connect to WiFi module
  if(!wifi.begin(12, 13))
  {
    Serial.println("Failed to connect to Wifi shield\n");
    while(1);
  }
  else
  {
    Serial.println("Starting Wifi connection......\n");
    if(!wifi.connectAP(SECRET_SSID, SECRET_PASS))
    {
      Serial.println("Failed to connect to Wifi\n");
      while(1);
    }
    Serial.print("Wifi connected to: ");Serial.println(wifi.SSID());
    Serial.print("IP Address: ");Serial.println(wifi.localIP());
    wifi.updateStatus();
    Serial.print("Wifi status is : ");Serial.println(wifi.status());   //2- wifi connected with ip, 3- got connection with servers or clients, 4- disconnect with clients or servers, 5- no wifi
    server.begin();
  }
  
  // Initialize the LED Display
  initializeDisplay(LED_SIZE, LED_BORDER, LANDSCAPE);
  displayWelcomeMessage(appTitle, appVersion);
  delay(2000);

  // Clear the LED Display and wait for Commands from the Remote IoT Arduino
  clearDisplay();
 }

void loop() 
{
#if HAS_IOT
  ESP8266Client client = server.available();
  if(client)
    Serial.println("Valid Client");
  else
    Serial.println("No Client");
      
  // Wait for a Command from the Remote IoT Arduino
  while(client.connected())
  {
    Serial.println("Waiting for command.....");
    while(client.available() > 0)
    {
      // Get LED Display Command and Data
      String req = client.readStringUntil('\n');
      client.find("\n");     
      Serial.print("Processing command.....");Serial.println(req);
      char* cmd = strtok(req.c_str(), "=");
      char* data = strtok(NULL, "=");

      // Switch on Command
      //  LED Display with a desired Color
      if(strcmp(cmd, "LED") == 0)
      {
        int color = BLACK;
        if(strcmp(data, "PURPLE") == 0)
          color = PURPLE;
        else if(strcmp(data, "WHITE") == 0)
          color = WHITE;
        else if(strcmp(data, "YELLOW") == 0)
          color = YELLOW;
        else if(strcmp(data, "RED") == 0)
          color = RED;
        else
          color = BLACK;
        displayLED(ledX, ledY, color);
        calculateNextLED();
      }
    }
  }
  delay(10000);
#else
  // Standalone LED Display Demo
  int color = calculateLEDColor();
  displayLED(ledX, ledY, color);
  delay(250);
  calculateNextLED();
#endif
}

int calculateLEDColor()
{
  int color = (ledY & 1) ? (ledX & 1 ? PURPLE : WHITE) : (ledX & 1 ? WHITE : PURPLE);
  if(ledX == 5 && ledY == 1)
    color = YELLOW;
  if(ledX == 6 && ledY == 4)
    color = RED;
  return color; 
}

void calculateNextLED()
{
  ++ledX;
  if(ledX == getNumberLEDColumns())
  {
    ledX = 0;
    ++ledY;
  }
  if(ledY == getNumberLEDRows())
  {
    ledX = 0;
    ledY = 0;
  }
}
