#include "Lucky.h"

#include <Wire.h>
#include <SPI.h>
#include <WiFiNINA.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>

#define DEV_ENV 0
#define SAMPLE_TIME_SECS  7200

WiFiClient wifi;
WiFiSSLClient wifiSecure;
char ssid[] = "StarBase III";        
char pass[] = "Brianna3325";    
int postCount = 0;   

void setup() 
{
  // Initialize the System
  lucky.begin();
  Serial.begin(9600);
  while(!Serial);

  // Initialize Logger
  Log.begin(LOG_LEVEL_VERBOSE, &Serial);

  // Display application startup message
  Log.notice("IoT Weather Application v0.1\n\n");

  // Clear LED display
  postCount = 0;
  displayLED(postCount);

  // Initialize and connect to WiFi module
  int status = WL_IDLE_STATUS;
  while ( status != WL_CONNECTED) 
  {
    Log.verbose(F("Attempting to connect to Network named: %s\n"), ssid);
    status = WiFi.begin(ssid, pass);
    Log.verbose("You're connected to the network\n");
    Log.verbose(F("SSID: %s\n"), WiFi.SSID());
    Log.verbose(F("IP Address: %d.%d.%d.%d\n"), WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  }
}

void loop() 
{
  // Get current temp, pressure, and humidity
  float temperature = (lucky.environment().temperature() * 9/5) + 32;
  float pressure = (lucky.environment().pressure() / 100.0F) / 33.8638F;
  float humidity = lucky.environment().humidity();

  // Convert sensor data to JSON
  String json = createJSON(temperature, pressure, humidity);

  // Print sensor data as JSON to the Verbose Logger
  Log.verbose(F("Generated JSON sensor data: %s\n\n"), json.c_str());

  // POST the sensor data to all REST Endpoints
  #if DEV_ENV
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

String createJSON(float temperature, float pressure, float humidity)
{
  // Round everything to just 2 decimal places
  temperature = roundf((temperature * 100 + .5))/100;
  pressure = roundf((pressure * 100 + .5))/100;
  humidity = roundf((humidity * 100 + .5))/100;
     
  // Convert sensor data to JSON
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

void postToEndpoint(String serverAddress, String uri, int port, String json)
{
  // Send HTTP POST Request to the Server for the Save REST API
  Log.verbose(F("Making POST request with HTTP basic authentication to %s\n"), serverAddress.c_str());
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
  Log.verbose(F("Return Status code: %d\n"), statusCode);
  Log.verbose(F("Return Response: %s\n"), response.c_str());
}
