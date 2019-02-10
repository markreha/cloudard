/**
 * NAME: IotDisplay.c
 * DESCRIPTION: Utility library to support the LCD Display.
 * 
 * AUTHOR: Professor Mark Reha
 * VERSION: 1.0.0   Initial release
 * COPYRIGHT: On The Edge Software Consulting Services 2019.  All rights reserved.
 * 
 */

#include "IotDisplay.h"

#include <CytronWiFiShield.h>
#include <CytronWiFiClient.h>
#include <CytronWiFiServer.h>
#include <SoftwareSerial.h>
#include <ArduinoHttpClient.h>

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
bool hasConnectedToClient = false;
IPAddress ipAddress;
ESP8266Server server(8081);

/**
 * NAME: setup()
 * DESCRIPTION: Arduino Entry Point for setting up the application:
 * PROCESS:       Initialize Serial Port
 *                Connect to the Wifi Network
 *                Initialize LCD Display, display Welcome Message, and display Connectivity IP Address Message
 *                Clear the LCD Display
  * INPUTS: None
 * OUTPUTS: None
 * 
 */
void setup() 
{
  // Initilaie the Serial Port
  Serial.begin(9600);
  while(!Serial);

  // Initialize and connect to WiFi module
  if(!wifi.begin(10, 11))
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
    ipAddress = wifi.localIP();
    Serial.print("Wifi connected to: ");Serial.println(wifi.SSID());
    Serial.print("IP Address: ");Serial.println(ipAddress);
    wifi.updateStatus();
    Serial.print("Wifi status is: ");Serial.println(wifi.status());   //2- wifi connected with ip, 3- got connection with servers or clients, 4- disconnect with clients or servers, 5- no wifi
    hasConnectedToClient = false;
    server.begin();
  }
  
  // Initialize the LED Display
  initializeDisplay(LED_SIZE, LED_BORDER, LANDSCAPE);
  displayWelcomeMessage(appTitle, appVersion);
  delay(2000);

  // Clear the LED Display and wait for Commands from the Remote IoT Arduino
  clearDisplay();
}

/**
 * NAME: loop()
 * DESCRIPTION: Arduino Entry Point for the application:
 * PROCESS:       Loop Forever
 *                  If not connected to remote Client then print IP Address and waiting Message to Serial Port
 *                  Check if connected to remote Client and if connected wait until a Display Command is received
 *                    Once Display Command is received then swith on Command: LED=[PURPLE | WHITE | YELLOW | RED]   
 *                    Update the LCD Display
 *                  Sleep
 * INPUTS: None
 * OUTPUTS: None
 * 
 */
void loop() 
{
#if HAS_IOT
  // Display the Wifi IP Address that the IoT Device can connect to and once first connection has been made clear the screen and display LED squares
  if(!hasConnectedToClient)
  {
    String address = String("Connect your IoT Device to ") + ipAddress[0] + String(".") + ipAddress[1] + String(".") + ipAddress[2] + String(".") + ipAddress[3];
    displayMessage(0, 0, address, 2, 0);
    displayMessage(2, 0, "  Waiting to connect to IoT Device...", 2, 0);
  }
  ESP8266Client client = server.available();
  if(client)
  {
    Serial.println("Valid Client");
    if(!hasConnectedToClient)
    {
      hasConnectedToClient = true;
      clearDisplay();
    }
  }
  else
  {
    Serial.println("No Client");
  }
      
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

/**
 * NAME: calculateLEDColor()
 * DESCRIPTION: Utility test method to cycle thru colors based on current X and Y LED screen location.
 * INPUTS: None
 * OUTPUTS: LED Color Value
 * 
 */
int calculateLEDColor()
{
  int color = (ledY & 1) ? (ledX & 1 ? PURPLE : WHITE) : (ledX & 1 ? WHITE : PURPLE);
  if(ledX == 5 && ledY == 1)
    color = YELLOW;
  if(ledX == 6 && ledY == 4)
    color = RED;
  return color; 
}

/**
 * NAME: calculateNextLED()
 * DESCRIPTION: Utility method to calculate the next X and Y LED screen location.
 * INPUTS: None
 * OUTPUTS: None (sets global variables ledX and ledY)
 * 
 */
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
