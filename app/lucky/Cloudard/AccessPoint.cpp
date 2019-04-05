#include "AccessPoint.h"
#include <WiFiNINA.h>
#include <ArduinoLog.h>

char ssidAP[] = "IoTAccessPoint";        
WiFiServer serverAP(80);
String configuredSSID = "";
String configuredSSID_PW = "";
String configuredDisplay_IP = "";

bool accessPoint()
{
  WiFiClient client;
  int status = WL_IDLE_STATUS;
  
  // Create Access Point
  Log.verbose(F("Access Point Web Server\n"));
  if (WiFi.status() == WL_NO_SHIELD)
  {
    Log.verbose(F("WiFi shield not present\n"));
    return false;
  }
  Log.verbose(F("Creating Access Point named: %s\n"), ssidAP);
  status = WiFi.beginAP(ssidAP);
  if (status != WL_AP_LISTENING) 
  {
    Log.verbose(F("Creating Access Point failed"));
    return false;
  }

  // Start Server with Access Point and print connectivity information
  delay(5000);
  serverAP.begin();
  Log.verbose(F("Connect your Wifi to this Access Point SSID: %s\n"), WiFi.SSID());
  do
  {
    status = WiFi.status();
  }while (status != WL_AP_CONNECTED);
  Log.verbose(F("Connected to Access Point\n"));
  Log.verbose(F("Open a browser to http://%d.%d.%d.%d\n"), WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  
  // Listen for incoming requests and once connected display the Configuration Page
  bool done = false;
  while(!done)
  {
    do
    {
      client = serverAP.available();
    }while (!client);
    Log.verbose(F("Connected to Access Point\n"));
    String currentLine = "";
    bool processFormPost = false;
    while (client.connected()) 
    {
      // If bytes available to read then check for GET request, POST request, and Form Content Length
      if (client.available()) 
      {             
        char c = client.read(); 
        if (c == '\n') 
        {
          // Check for GET and then display Form, check for POST then process form data, check for Content-Length then get form data
          if (currentLine.startsWith("GET")) 
          {
            // Send HTTP headers: HTTP/1.1 200 OK and Content Type, end with a blank line
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();
  
            // Print out the Configuration Page in HTML
            client.print("<html>");
            client.print("<body>");
            client.print("<h2>IoT Weather Station Setup</h2><br/>");
            client.print("<form action=\"\\\" method=\"POST\">");
            client.print("  SSID: <input name=\"ssid\" type=\"text\" maxlength=\"25\" placeholder=\"Enter SSID\" required><br/>");
            client.print("  Password: <input name=\"password\" type=\"password\" maxlength=\"25\" placeholder=\"Enter SSID Password\"><br/>");
            client.print("  Display IP Address: <input name=\"ipaddress\" type=\"text\" maxlength=\"15\" placeholder=\"Enter Display IP Address\" pattern=\"\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\" required><br/>");
            client.print("  <input type=\"submit\" value=\"Submit\">");
            client.print("</form>");
            client.print("</body>");
            client.print("</html>");
  
            // End the HTTP response with another blank line and break out of this while loop
            client.println();
            break;
          }
          else if (currentLine.startsWith("POST"))
          {
            // Indicate we are processing a Form POST
            processFormPost = true;
            currentLine = "";            
          }
          else if(processFormPost && currentLine.startsWith("Content-Length:"))
          {
            // Get the Posted Form Data, Parse it, and set in local variables
            String temp = currentLine.substring(currentLine.indexOf("Content-Length:") + 15);
            temp.trim();
            int length = temp.toInt();   
            length = length + 22;     // TOTAL HACK: Add the Accepted-Language header to the count
            currentLine = "";            
            while(length)
            {
              char c = client.read();
              if(c != '\r' && c != '\n')
              {
                currentLine += c;
                --length;    
              }
            }

            // Parse the Form Data
            int n1 = currentLine.indexOf('&');
            String s1 = currentLine.substring(0, n1);
            int n2 = currentLine.lastIndexOf('&');
            String s2 = currentLine.substring(n1+1, n2);
            String s3 = currentLine.substring(n2+1);
            configuredSSID = s1.substring(s1.indexOf('=')+1);
            configuredSSID_PW = s2.substring(s2.indexOf('=')+1);
            configuredDisplay_IP = s3.substring(s3.indexOf('=')+1);
            
            configuredSSID.replace('+', ' ');
            configuredSSID.trim();
            configuredSSID_PW.trim();
            configuredDisplay_IP.trim();             
            done = true;  
            break;
          }
          else 
          { 
            // If we got a newline then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r') 
        {
          // Wait until we get a carriage return character and add characters to the end of the currentLine
          currentLine += c; 
        }       
      }
    }
       
    // close the connection:
    client.stop();
    Log.verbose(F("Access Point disconnected\n"));
  }
  Log.verbose(F("Done with Access Point\n"));
  return true;
}

String getConfiguredSSID()
{
  return configuredSSID;
}

String getConfiguredPW()
{
  return configuredSSID_PW;
}

String getConfiguredDisplayIP()
{
  return configuredDisplay_IP;
}
