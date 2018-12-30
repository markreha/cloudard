**Arduino IoT Device Reference Application**
==================
The example IoT Device Reference application implemented in an Arduino Uno Wifi Rev2 and Lucky Shield reads IoT data from the Sense HAT and posts this data to the IoT Services Reference application using its published REST API's. These applications in combination demonstrate a simple, scalable, Cloud based IoT application.

<p align="center">
	<img src="https://github.com/markreha/cloudworkshop/blob/master/sdk/docs/architecture/images/iotdevice1.png" alt="IoT Device"/>
</p>

Architecture & Technologies
--------
 The IoT Device Reference application is designed and implemented in C using the Lucky Shield support libaries. 

The Lucky Shield supports the following features:

 - Temperature, Humidity, and Barometric pressure 
 - Gyroscope Accelerometer, and Magnetometer
 - LED display 
 - Joystick 
 
Basic Application Functionality
--------
The IoT Device Reference application logic, as illustrated in the flow chart below, primary functionality includes sitting in loop reading the Lucky Shield IoT data, posting this data to the IoT Services application using a REST API, and then sleeping for a specified period of time. The current IoT Device Reference application leverages the LED display and the Temperature, Humidity, and Barometric pressure sensors in its implementation. The application could be extended in the future to leverage other features of the Lucky Shield.

![IoT Device Flow Chart Diagram](https://github.com/markreha/cloudworkshop/blob/master/sdk/docs/architecture/images/iotflowchart1.png)

Repository Contents
----------
This repository contains code to support the Arduino Uno Wifi Rev2 and Lucky Shield. The IoT Device Reference application can be used as a starting point to monitor Weather IoT data. I order to build the Arduino application requires the following libraries: WiFiNINA, ArduinoHttpClient, ArduinoJson, and ArduinoLog.

 - ***app/lucky***: this folder contains the C code for the Reference IoT Device application using an Arduino Uno Wifi Rev2 and a Lucky Shield.

[Back to Top](#iot-device-reference-application)
