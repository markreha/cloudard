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

// Adjust these flags to reduce program size
#define HAS_LCD true
#define TRIM_APP true

// Set this to true to send REST API request to local development server
#define DEV_ENV false

#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#if TRIM_APP == false
  #include <ArduinoJson.h>
#endif
#if HAS_LCD == false
  #include <ArduinoLog.h>
#endif
#include "Cloudard.h"

WiFiClient wifi;
WiFiSSLClient wifiSecure;
char ssid[] = SECRET_SSID;        
char pass[] = SECRET_PASS;    
int postCount = 0;   

/**
 * NAME: setup()
 * DESXRIPTION: Arduino Entry Point for setting up the application:
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
#if HAS_LCD == false
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);
#endif

  // Display application startup message
#if HAS_LCD == false
  Log.notice("IoT Weather Application v0.1\n\n");
#endif

  // Clear LED display
  postCount = 0;
  displayLED(postCount);

  // Initialize and connect to WiFi module
  int status = WL_IDLE_STATUS;
  while ( status != WL_CONNECTED) 
  {
#if HAS_LCD == false
   Log.verbose(F("Attempting to connect to Network named: %s\n"), ssid);
#endif
  status = WiFi.begin(ssid, pass);
#if HAS_LCD == false
   Log.verbose("You're connected to the network\n");
   Log.verbose(F("SSID: %s\n"), WiFi.SSID());
   Log.verbose(F("IP Address: %d.%d.%d.%d\n"), WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
#endif
  }
}

/**
 * NAME: loop()
 * DESXRIPTION: Arduino Entry Point for the application:
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
  // Get current temperature, pressure, and humidity
  float temperature = (lucky.environment().temperature() * 9/5) + 32;
  float pressure = (lucky.environment().pressure() / 100.0F) / 33.8638F;
  float humidity = lucky.environment().humidity();

  // Convert sensor data to JSON
  String json = createJSON(temperature, pressure, humidity);

  // Print sensor data as JSON to the Verbose Logger
#if HAS_LCD == false
  Log.verbose(F("Generated JSON sensor data: %s\n\n"), json.c_str());
#endif

  // POST the sensor data to all REST Endpoints
  #if DEV_ENV == true
    String serverAddress = "10.0.1.101";
    testEndpoint(serverAddress, "/cloudservices/rest/weather/get/1/6", 8080);
    postToEndpoint(serverAddress, "/cloudservices/rest/weather/save", 8080, json);
  #else
    String serverAddress = "";
    // Post to Azure
    serverAddress = "markwsserve2.azurewebsites.net";
    postToEndpoint(serverAddress, "/cloudservices/rest/weather/save", 80, json);
    // Post to Heroku
    serverAddress = "mark-servicesapp.herokuapp.com";
    postToEndpoint(serverAddress, "/rest/weather/save", 80, json);
    // Post to AWS
    serverAddress = "services-app.us-east-2.elasticbeanstalk.com";
    postToEndpoint(serverAddress, "/rest/weather/save", 80, json);
    // Post to Google
    serverAddress = "cloud-workshop-services.appspot.com";
    postToEndpoint(serverAddress, "/rest/weather/save", 80, json);
  #endif

  // Display POST Count on the LED's
  ++postCount;
  displayLED(postCount);

  // Sleep until we need to read the sensors again
  delay(SAMPLE_TIME_SECS * 1000);
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
  // Convert sensor data to JSON 
#if TRIM_APP == false
  // Round everything to just 2 decimal places
  temperature = roundf((temperature * 100 + .5))/100;
  pressure = roundf((pressure * 100 + .5))/100;
  humidity = roundf((humidity * 100 + .5))/100;

  // Use JSON Library if we can afford the space
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
#else
  // Use String.concat() and optimized floatToString() which is a bit smaller implementation than using JSON Library
  char str_temp1[6], str_temp2[6], str_temp3[6];
  floatToString(str_temp1, temperature);
  floatToString(str_temp2, pressure);
  floatToString(str_temp3, humidity);
  String buffer ="";
  buffer.concat("{\"deviceID\":");
  buffer.concat(1);
  buffer.concat(",\"temperature\":");
  buffer.concat(str_temp1);
  buffer.concat(",\"humidity\":");
  buffer.concat(str_temp2);
  buffer.concat(",\"humidity\":");
  buffer.concat(str_temp3);
  buffer.concat("}");

  // Return JSON as a string
  return String(buffer);
#endif
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
#if HAS_LCD == false
  Log.verbose(F("Making GET request with HTTP basic authentication to %s\n"), serverAddress.c_str());
#endif
  HttpClient client = HttpClient(port == 443 ? wifiSecure : wifi, serverAddress, port);
  client.beginRequest();
  client.get(uri);
  client.sendBasicAuth("CloudWorkshop", "dGVzdHRlc3Q=");
  client.endRequest();

  // Read the status code and body of the response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  // Print status and response to the Verbose Logger
#if HAS_LCD == false
  Log.verbose(F("Return Status code: %d\n"), statusCode);
  Log.verbose(F("Return Response: %s\n"), response.c_str());
#endif
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
 * OUTPUTS:
 *    HTTP Status Code
 *    
 */
int postToEndpoint(String serverAddress, String uri, int port, String json)
{
  // Send HTTP POST Request to the Server for the Save REST API
#if HAS_LCD == false
  Log.verbose(F("Making POST request with HTTP basic authentication to %s\n"), serverAddress.c_str());
#endif
  HttpClient client = HttpClient(wifi, serverAddress, port);
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
#if HAS_LCD == false
  Log.verbose(F("Return Status code: %d\n"), statusCode);
  Log.verbose(F("Return Response: %s\n"), response.c_str());
#endif

  // Return HTTP Status Code
  return statusCode;
}

/**
 * NAME: floatToString()
 * DESCRIPTION: Utility method that is highly optimized to convert a float to a string.
 * PROCESS:   Truncate the float to get the whole number
 *            Remove the whole part of float and shift 2 places over
 *            Truncate the fractional part from the new whole part
 * 
 * INPUTS:
 *    char* str   The output string buffer
 *    float flt   The input floating point number to convert
 * OUTPUTS:
 *    NONE (output is copied to input str buffer)
 */
void floatToString(char* str, float flt)
{
  int whole, fraction;

  //Get whole and fractional part of input floating point number
  whole = (int)flt;
  fraction = ((flt + .005) - whole) * 100;

  // Convert whole to character string (up to 3 digits) and fraction to character string (up to 2 digits)
  int index = 0;
  if((whole - 100) > 0)
  {
    str[index++] = 0x30 + (whole/100);
    whole = whole - 100;     
  }
  if((whole - 10) > 0)
  {
    str[index++] = 0x30 + (whole/10);
    whole = whole - ((whole/10) * 10);     
  }
  str[index++] = 0x30 + whole;
  str[index++] = '.';
  if((fraction - 10) > 0)
  {
    str[index++] = 0x30 + (fraction/10);
    fraction = fraction - ((fraction/10) * 10);     
  }
  else
  {
    str[index++] = 0x30;
  }
  str[index++] = 0x30 + fraction;
  str[index++] = '\0';
}
