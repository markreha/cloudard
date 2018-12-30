**IoT Device Reference Application**
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
The IoT Device Reference application logic, as illustrated in the flow chart below, primary functionality includes sitting in loop reading the Sense HAT IoT data, posting this data to the IoT Services application using a REST API, and then sleeping for a specified period of time. The current IoT Device Reference application leverages the LED display and the Temperature, Humidity, and Barometric pressure sensors in its implementation. The application uses the Logging Framework built into the Python 3 libraries. The application could be extended in the future to leverage other features of the Sense HAT.

It should be noted that the IoT Device Reference application contains additional logic so that the Raspberry Pi and Sense HAT could be shared across multiple back end IoT Services applications with the assumption that each back end adheres to the REST API as defined in this SDK. The REST API endpoints are configured in the Config.py code module.

![IoT Device Flow Chart Diagram](https://github.com/markreha/cloudworkshop/blob/master/sdk/docs/architecture/images/iotflowchart.png)

Repository Contents
----------
This repository contains code to support the Arduino Uno Wifi Rev2 and Lucky Shield. The IoT Device Reference application can be used as a starting point to monitor Weather IoT data.

 - ***app/lucky***: this folder contains the C code for the Reference IoT Device application using a Lucky Shield.

[Back to Top](#iot-device-reference-application)
