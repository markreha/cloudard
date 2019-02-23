/**
 * NAME: Cloudard.c
 * DESCRIPTION: Arduino Uno Wifi Rev2 and Luck Shield application to demonstrate an IoT Device 
 *              capturing Weather Data from a Lucky Shield and posting the sensor data to a 
 *              backend REST API.
 * 
 * AUTHOR: Professor Mark Reha
 * VERSION: 1.0.0   Initial release
 * COPYRIGHT: On The Edge Software Consulting Services 2018.  All rights reserved.
 * 
 */
#include "Lucky.h"
#include <WiFiNINA.h>
#include <HttpClient.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#include "AccessPoint.h"
#include "Cloudard.h"

// Set this to true if using remote LED Dispaly over Wifi
#define HAS_LCD true

// Set this to true to send REST API request to local development server
#define DEV_ENV false

// Set this to the number of seconds that the Watch Dog will use before reseting the Arduino
#define WATCH_DOG_SECONDS 600

WiFiClient wifi;
WiFiSSLClient wifiSecure;
WiFiClient remoteLedClient;
char ssid[] = SECRET_SSID;        
char pass[] = SECRET_PASS;    
int postCount = 0;   
char ledDisplayAddress[] = "10.0.1.154";
int ledDisplayPort = 8081;
bool wdEnable = true;
volatile int wdSecCount = WATCH_DOG_SECONDS;

/**
 * NAME: setup()
 * DESCRIPTION: Arduino Entry Point for setting up the application:
 * PROCESS:       Initialize Luck Shield
 *                Display Welcome Message
 *                Initialize Logger
 *                Initialize the LED Display
 *                Connect to the Wifi Network
 * INPUTS: None
 * OUTPUTS: None
 * 
 */
void setup() 
{
  // Initialize the System
  lucky.begin();
  Serial.begin(9600);
  while(!Serial);

  // Initialize the Logger
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  // Display application startup message
  Log.notice(F("IoT Weather Station v0.1\n\n"));

  // Clear LED display
  postCount = 0;
  displayLED(postCount);

  // Read Configuration Settings from EEPROM
  bool ok = readConfiguration();

  // Check for Network Configuration Page
  configurationStartupCheck(ok ? false : true);

  // Initialize and connect to WiFi module
  connectToWifi();

  // Initialize the RTC and internal Watch Dog counter
  wdEnable = true;
  wdSecCount = WATCH_DOG_SECONDS;
  initRTC();
}

/**
 * NAME: loop()
 * DESCRIPTION: Arduino Entry Point for the application:
 * PROCESS:       Loop Forever
 *                  Get the temperature, pressure, and humidity sensor data
 *                  Convert the sensor data to JSON
 *                  Log the sensor data   
 *                  POST the sensor data to the REST endpoints
 *                  Update the LED Display
 *                  Sleep
 * INPUTS: None
 * OUTPUTS: None
 * 
 */
void loop() 
{
  // Enable and reset Watch Dog while we are in the processing loop (minus the wait())
  wdEnable = true;
  wdSecCount = WATCH_DOG_SECONDS;
  
  // For debugging display Free RAM
  Log.verbose(F("Free RAM is %d\n"), freeRam());          
  
  // Get current temperature, pressure, and humidity
  float temperature = (lucky.environment().temperature() * 9/5) + 32;
  float pressure = (lucky.environment().pressure() / 100.0F) / 33.8638F;
  float humidity = lucky.environment().humidity();

  // Convert sensor data to JSON
  String json = createJSON(temperature, pressure, humidity);

  // Print sensor data as JSON to the Verbose Logger
  Log.verbose(F("Generated JSON sensor data: %s\n"), json.c_str());

  // Make sure we are still connected to the Wifi network and if not connect back to the Wifi network
  if(wifi.status() == WL_CONNECTION_LOST || wifi.status() == WL_DISCONNECTED)
  {
    Log.verbose(F("Lost connection to Wifi"));
    connectToWifi();  
  }

  // POST the sensor data to all REST Endpoints
  int errorCount = 0;
  int status = 0;
  #if DEV_ENV == true
    String serverAddress = "10.0.1.101";
    testEndpoint(serverAddress, "/cloudservices/rest/weather/get/1/6", 8080);
    status = postToEndpoint(serverAddress, "/cloudservices/rest/weather/save", 8080, json);
    if(status != 200)
      ++errorCount;
  #else
    String serverAddress = "";
    // Post to Heroku
    serverAddress = "mark-servicesapp.herokuapp.com";
    status = postToEndpoint(serverAddress, "/rest/weather/save", 80, json);
    if(status != 200)
      ++errorCount;
    // Post to Azure
    serverAddress = "markwsserve2.azurewebsites.net";
    status = postToEndpoint(serverAddress, "/cloudservices/rest/weather/save", 80, json);
    if(status != 200)
      ++errorCount;
    // Post to AWS
    serverAddress = "services-app.us-east-2.elasticbeanstalk.com";
    status = postToEndpoint(serverAddress, "/rest/weather/save", 80, json);
    if(status != 200)
      ++errorCount;
    // Post to Google
    serverAddress = "cloud-workshop-services.appspot.com";
    status = postToEndpoint(serverAddress, "/rest/weather/save", 80, json);
    if(status != 200)
      ++errorCount;
  #endif

  // Display POST Count on the LED's
  ++postCount;
  displayLED(postCount);

  // Display Status on Remot LED Display
#if HAS_LCD == true
  String color = "";
  if(errorCount != 0)
  {
    color = "YELLOW";
  }
  else
  {
    if((postCount & 0x01) == 1)
      color = "PURPLE";
    else
      color = "WHITE";    
  }
  displayRemoteLED("LED", color);
#endif

  // Disable the Watch Dog while we are sleeping in wait()
  wdEnable = false;

  // Sleep until we need to read the sensors again
  wait(SAMPLE_TIME_SECS * 1000UL);
}

/**
 * NAME: readConfiguration()
 * DESCRIPTION: Read the Configuration Settings from the EEPROM.
 * PROCESS:   Read 4 null terminated strings: [0] = Configuration Status, [1] = SSID, [2] = SSID Password, [3] = Display IP Address
 * 
 * INPUTS:
 *    None
 * OUTPUTS:
 *    True if Configuration read else return false
 *    
 */
bool readConfiguration()
{
  int tokens = 0;
  int address = 0;
  byte value;
  int index = 0;
  char buffer[50];
  String flag = "";

  // Read all 4 Configuration Tokens
  Log.verbose(F("Reading Configuration Settings from EEPROM\n"));
  while(tokens < 4)
  {
    // Get value from EEPROM
    value = EEPROM.read(address);

    // Check if the Configuration Flag is not set and if not set just return so we can display the Configuration Page
    if(tokens == 0 && address == 0 && value != 1)
      return false;

    // If at end of a Token then end buffer and save value in String
    // Else save EEPROM value in buffer
    if(value == 0)
    {
      buffer[index++] = '\0';
      if(tokens == 0)
      {
          flag = String(buffer);
          ++tokens;
          index = 0;
      }
      else if(tokens == 1)
      {
          String s1 = String(buffer);
          s1.toCharArray(ssid, s1.length()+1);
          ++tokens;
          index = 0;
      }
      else if(tokens == 2)
      {
          String s2 = String(buffer);
          s2.toCharArray(pass, s2.length()+1);
         ++tokens;
          index = 0;
      }
      else if(tokens == 3)
      {
          String s3 = String(buffer);
          s3.toCharArray(ledDisplayAddress, s3.length()+1);
          ++tokens;
          index = 0;
      }
    }
    else
    {
      buffer[index++] = value;
    }
    ++address;
  }

  // Print out results and return OK
  Log.verbose(F("EEPROM Configuration Values are: \n"));
  Log.verbose("%s\n", ssid);
  Log.verbose("%s\n", pass);
  Log.verbose("%s\n", ledDisplayAddress);
  return true;
}

/**
 * NAME: writeConfiguration()
 * DESCRIPTION: Write the Configuration Settings from the EEPROM.
 * PROCESS:   Write 4 null terminated strings: [0] = Configuration Status, [1] = SSID, [2] = SSID Password, [3] = Display IP Address
 * 
 * INPUTS:
 *    SSID, SSID Password, and Display IP Address
 * OUTPUTS:
 *    None
 *    
 */
void writeConfiguration(String ssid, String password, String displayIp)
{
  int address = 0;

  // Clear out EEPROM
  Log.verbose(F("Writing Configuration Settings to EEPROM\n"));
  for (int i = 0;i < EEPROM.length();++i)
    EEPROM.write(i, 0xFF);
  // Write Configuration Set flag
  EEPROM.write(address++, 1);
  EEPROM.write(address++, 0); 
  // Write SSID
  for (int i = 0;i < ssid.length();++i)
    EEPROM.write(address++, ssid[i]);
  EEPROM.write(address++, 0); 
  // Write SSID Password
  for (int i = 0;i < password.length();++i)
    EEPROM.write(address++, password[i]);
  EEPROM.write(address++, 0); 
  // Write Display IP Address
  for (int i = 0;i < displayIp.length();++i)
    EEPROM.write(address++, displayIp[i]);
  EEPROM.write(address++, 0);
}

/**
 * NAME: configurationStartupCheck()
 * DESCRIPTION: Method to interface to display Configuration Page thru a Wifi Access Point to get SSID, Password, and Display IP Address from a User.
 * PROCESS:   Sound the Buzzer Twice
 *            If the Joy Stick Button is Pressed then display Configuration page and get all the network connectivity information from the user.
 * 
 * INPUTS:
 *    ignoreButtonCheck Set to true to bypass Joy Stick Button press check
 * OUTPUTS:
 *    None
 *    
 */
void configurationStartupCheck(bool ignoreButtonCheck)
{
  // Check if Joy Stick Button is pressed
  int buttonState = 1;
  if(!ignoreButtonCheck)
    buttonState = lucky.gpio().digitalRead(JOYC);
  if(buttonState)
  {
    // Sound Buzzer twice on startup
    tone(5, 0); tone(5, 300); delay(500); noTone(5); delay(500); tone(5, 300); delay(500); noTone(5);

    // Display Configuration Page using Wifi Access Point
    bool ok = accessPoint();
    if(ok)
    {
      //  If OK sound Buzzer twice
      tone(5, 0); tone(5, 300); delay(500); noTone(5); delay(500); tone(5, 300); delay(500); noTone(5);

      // Save all the network configuration info
      String s1 = getConfiguredSSID();
      String s2 = getConfiguredPW();
      String s3 = getConfiguredDisplayIP();
      if(s1.length() == 0 || s2.length() == 0 || s3.length() == 0)
      {
        // Error sound Buzzer three times
        tone(5, 0); tone(5, 500); delay(500); noTone(5); delay(500); tone(5, 500); delay(500); noTone(5); tone(5, 500); delay(500); noTone(5);
      }
      else
      {
        // Save the Configuration in the global variables
        s1.toCharArray(ssid, s1.length()+1);        
        Log.verbose(F("SSID Configured to %s\n"), s1.c_str());
        s2.toCharArray(pass, s2.length()+1);
        Log.verbose(F("SSID Password Configured to %s\n"), s2.c_str());
        s3.toCharArray(ledDisplayAddress, s3.length()+1);
        Log.verbose(F("Display IP Address Configured to %s\n"), s3.c_str());

        // Save the Configuration in EEPROM
        writeConfiguration(ssid, pass, ledDisplayAddress);
      }
    }
    else
    {
      // Else Error sound Buzzer three times
      tone(5, 0); tone(5, 500); delay(500); noTone(5); delay(500); tone(5, 500); delay(500); noTone(5); tone(5, 500); delay(500); noTone(5);
    }
  }
}

/**
 * NAME: freeRam()
 * DESCRIPTION: Utility method to get Free RAM
 * 
 * INPUTS:
 *    None
 * OUTPUTS:
 *    Available Free RAM in bytes
 *    
 */
int freeRam() 
{
  extern int __heap_start, *__brkval;
  int v;
  return (int)&v - (__brkval == 0 ? (int)&__heap_start : (int)__brkval);
}

/**
 * NAME: connectToWifi()
 * DESCRIPTION: Connect to the Wifi Network using global SSID, Username, and Password.
 * 
 * INPUTS:
 *    None
 * OUTPUTS:
 *    None
 *    
 */
void connectToWifi()
{
  int status = WL_IDLE_STATUS;
  while (status != WL_CONNECTED) 
  {
    Log.verbose(F("Attempting to connect to Network named: %s\n"), ssid);
    status = WiFi.begin(ssid, pass);
    Log.verbose(F("You're connected to the network\n"));
    Log.verbose(F("SSID: %s\n"), WiFi.SSID());
    Log.verbose(F("IP Address: %d.%d.%d.%d\n"), WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  }  
}

/**
 * NAME: wait()
 * DESCRIPTION: Utility method to accurately wait the Sample Time.
 * 
 * INPUTS:
 *    waitTime  Time to wait in milliseconds
 * OUTPUTS:
 *    None
 *    
 */
void wait(unsigned long waitTime)
{
  // Hang until the Sample Time has expired
  unsigned long currentMillis = millis();
  unsigned long previousMillis = currentMillis;
  do
  {
    currentMillis = millis();
  }while (currentMillis - previousMillis < waitTime);
}
 
/**
 * NAME: createJSON()
 * DESCRIPTION: Utility method to convert sensor data to JSON.
 * 
 * INPUTS:
 *    float temperature   The temperature read from the sensor
 *    float pressure      The pressure read from the sensor
 *    float humidity      The humidity read from the sensor
 * OUTPUTS:
 *    JSON formatted sensor data per the REST API specification
 *    
 */
String createJSON(float temperature, float pressure, float humidity)
{
  // Round everything to just 2 decimal places
  temperature = roundf((temperature * 100 + .5))/100;
  pressure = roundf((pressure * 100 + .5))/100;
  humidity = roundf((humidity * 100 + .5))/100;

  // Convert sensor values to JSON using the JSON Library
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["deviceID"] = 1;
  root["temperature"] = temperature;
  root["pressure"] = pressure;
  root["humidity"] = humidity;
  
  // Return JSON as a string
  String json;
  root.printTo(json);
  return json;
}

/**
 * NAME: displayLED()
 * DESCRIPTION: Utility method to update the LED Display.
 * PROCESS:   Bit 0 of input value is displayed on LED1
 *            Bit 1 of input value is displayed on LED2
 * 
 * INPUTS:
 *    int value   The count to take 2 bits from for the LED Display
 * OUTPUTS:
 *    None
 *    
 */
void displayLED(int value)
{
  // Display values as a 2 bit binary value on LED1 and LED2
  if(value & 0x01)
    lucky.gpio().digitalWrite(LED1,HIGH);
  else
    lucky.gpio().digitalWrite(LED1,LOW);    
  if(value & 0x02)
    lucky.gpio().digitalWrite(LED2,HIGH);
  else
    lucky.gpio().digitalWrite(LED2,LOW);    
}

/**
 * NAME: displayRemoteLED()
 * DESCRIPTION: Utility method to send a Command and Data to a Remote LCD Display over a Wifi Connection.
 * PROCESS:   Connect to remote LED Display Arduino
 *            Send Command and Data
 * 
 * INPUTS:
 *    String cmd    The Command for the remote LCD Display (LED and MESSAGE)
 *    String data   The Command Data for the remote LED Display (LCD color and MESSAGE msg)
 * OUTPUTS:
 *    None
 *    
 */
void displayRemoteLED(String cmd, String data)
{
  Log.verbose(F("Sending Message to Remote LED Display........"));
  if (remoteLedClient.connect(ledDisplayAddress, ledDisplayPort)) 
  {
    Log.verbose(F("connected to Remote LED Display....."));
    remoteLedClient.print(cmd + "=" + data + "\n");
    Log.verbose(F("message sent\n\n"));
  }
  else
  {
    Log.verbose(F("no Connection to Remote LED Display\n\n"));
  }
}

/**
 * NAME: testEndpoint()
 * DESCRIPTION: Utility method to access the Test API from the REST Endpoint (only used for basic testing during development).
 * PROCESS:   Log the GET Request parameters
 *            Create a HTTP Client Connection
 *            Make a HTTP GET Request with Basic HTTP Authentication Headers set
 *            Log the Status and Response back from the HTTP GET Request            
 * 
 * INPUTS:
 *    String serverAddress    The REST API Endpoint server domain address
 *    String uri              The REST API Endpoint server URI
 *    int port                The REST API Endpoint server Port
 * OUTPUTS:
 *    None
 *    
 */
void testEndpoint(String serverAddress, String uri, int port)
{
  // Send HTTP GET Request to the Server for the Test REST API
  Log.verbose(F("Making GET request with HTTP basic authentication to %s\n"), serverAddress.c_str());
  HttpClient client = HttpClient(port == 443 ? wifiSecure : wifi, serverAddress, port);
  client.beginRequest();
  client.get(uri);
  client.sendBasicAuth("CloudWorkshop", "dGVzdHRlc3Q=");
  client.endRequest();

  // Read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  // Print status and response to the Verbose Logger
  Log.verbose(F("Return Status code: %d\n"), statusCode);
  Log.verbose(F("Return Response: %s\n"), response.c_str());
}

/**
 * NAME: postToEndpoint()
 * DESCRIPTION: Utility method to access the Save API from the REST Endpoint.
 * PROCESS:   Log the POST Request parameters
 *            Create a HTTP Client Connection
 *            Make a HTTP POST Request with Basic HTTP Authentication Headers set and JSON payload
 *            Log the Status and Response back from the HTTP GET Request            
 * 
 * INPUTS:
 *    String serverAddress    The REST API Endpoint server domain address
 *    String uri              The REST API Endpoint server URI
 *    int port                The REST API Endpoint server Port
 *    int json                The JSON to send to the Endpoint server
 * OUTPUTS:
 *    HTTP Status Code
 *    
 */
int postToEndpoint(String serverAddress, String uri, int port, String json)
{
  // Send HTTP POST Request to the Server for the Save REST API
  Log.verbose(F("Making POST request with HTTP basic authentication to %s\n"), serverAddress.c_str());
  HttpClient client = HttpClient(wifi, serverAddress, port);
  client.setHttpResponseTimeout(60 * 1000);
  client.beginRequest();
  client.post(uri);
  client.sendBasicAuth("CloudWorkshop", "dGVzdHRlc3Q=");
  client.sendHeader("Content-Type", "application/json");
  client.sendHeader("Content-Length", json.length());
  client.beginBody();
  client.print(json);
  client.endRequest();

  // Read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  // Print status and response to the Verbose Logger
  Log.verbose(F("Return Status code: %d\n"), statusCode);
  Log.verbose(F("Return Response: %s\n"), response.c_str());

  // Return HTTP Status Code
  return statusCode;
}


/**
 * NAME: initRTC()
 * DESCRIPTION: Utility method to initialize the Real Time Clock.
 * 
 * INPUTS:
 *    None
 * OUTPUTS:
 *    None
 *    
 */
void initRTC()
{
   // Disable RTC interrupts
  Log.verbose(F("Initializing the RTC and internal Watch Dog....."));   
  cli();
   
  // Initialize 32.768kHz Oscillator and disable the oscillator
  uint8_t temp = CLKCTRL.XOSC32KCTRLA;
  temp &= ~CLKCTRL_ENABLE_bm;
  
  // Enable writing to protected register and wait until XOSC32KS becomes 0
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.XOSC32KCTRLA = temp;
  while(CLKCTRL.MCLKSTATUS & CLKCTRL_XOSC32KS_bm);
    
  // SEL = 0 (Use External Crystal) and enable writing to protected register
  temp = CLKCTRL.XOSC32KCTRLA;
  temp &= ~CLKCTRL_SEL_bm;
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.XOSC32KCTRLA = temp;
    
  // Enable oscillator and Enable writing to protected register
  temp = CLKCTRL.XOSC32KCTRLA;
  temp |= CLKCTRL_ENABLE_bm;
  CPU_CCP = CCP_IOREG_gc;
  CLKCTRL.XOSC32KCTRLA = temp;
    
  //Initialize RTC and wait for all register to be synchronized
  while (RTC.STATUS > 0);

  // Set 32.768kHz External Crystal Oscillator (XOSC32K), enable Periodic Interrupts, at RTC Clock Cycles 32768 (1 second)
  RTC.CLKSEL = RTC_CLKSEL_TOSC32K_gc;
  RTC.DBGCTRL = RTC_DBGRUN_bm;
  RTC.PITINTCTRL = RTC_PI_bm; 
  RTC.PITCTRLA = RTC_PERIOD_CYC32768_gc | RTC_PITEN_bm;

  // Enable RTC interrupts
  sei();
  Log.verbose(F("complete\n"));   
}

/**
 * NAME: ISR()
 * DESCRIPTION: Interrupt Service Routine for RTC.
 * PROCESS:   Decrement applications Watch Dog counter.
 *            If applications Watch Dog counter hits 0 then use real Watch Dog Timer to reset the Arduino (the Application will reset the applications Watch Dog counter in loop()). 
 * 
 * INPUTS:
 *    None
 * OUTPUTS:
 *    None
 *    
 */
ISR(RTC_PIT_vect)
{
  // Clear interrupt flag by writing '1'
  RTC.PITINTFLAGS = RTC_PI_bm;

  // If Watch Dog enabled then decrement Watch Dog count and if we hit 0 then assume Arduino hung so use the actual Watch Dog Timer to reset the Arduino
  if(wdEnable)
  {
    --wdSecCount;
    if(wdSecCount == 0)
    {
      Serial.println("Watch Dog expired and going to reset the Arduino in 2 seconds");
      wdEnable = false;
      wdt_enable(WDTO_2S);
    }
  }
 }
