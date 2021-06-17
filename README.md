# ESPfleet
 Manage fleet of ESP32/ESP8266 boards with Nodered
 
## Goal
 
The objective of this project is to generate a system for maintaining a "fleet" of ESP microcontroller boards, including (automatic) firmware updates.
 
The boards connect to local WiFi and and an MQTT broker. 
Two Nodered flows communicate with the boards via MQTT. This is done via the command_API() routine.

1. Overview and current status of all connected boards
2. Editor for changing parameters per board
 
This code is Version 2.10 of the firmware for ESP32 boards. It enables over the air (OTA) updates of the
firmware (.bin) that has been created with the Arduino IDE.

The firmware is available for download by the boards from a web-server (OTA). The Nodered fleet-manager 
supplies the boards (or a selection of them) with the name and location of the firmware. The boards automatically
download and install the update.

The boards execute an awake - deep sleep cycle with configurable sleep time. This enables long battery life. 
 
## About this code 

The ESP firmware is written in the Arduino IDE. Development Notes have been kept in a README.md.ino file.
The .ino extension of this file is necessary to open it for editing in a tab in the Arduino IDE 
Versions have until now been managed by creation of new folders, accessible by the Arduino IDE. It is yet
unknown to me how to handle versioning with Github / Arduino IDE.
 
## Currently running board fleet hardware

    - 1x Heltec LORA32 .....
    - 2x TTGO LORA32 ....
    
    power supply: 1000 mAh Lithium Rechargeable Battery (MakerFocus)
    
## Fleet management dashboard (Nodered)

The management dashboard is implemented in Nodered. In this repository only the main functionality of the dashboard is described. 
The dashboard consists of 2 screens:

1. The Fleet mgr, with an overview of devices in the fleet.
2. A device editor for setting parameters of a device and triggering a firmware update


![alt text](https://github.com/goofy2k/ESPfleet/blob/main/media/Screenshot_Fleet_Mgr.jpg?raw=true)

When a board sends contacts the MQTT broker for the first time it's properties are shown in a table in the Fleet mgr. The devices are listed in the order of first contact. The unique deviceId contains it's MAC address in reverse order. The last digits in the code are representative for the board type (e.g. Heltec LORA32 ....., TTGO LORA32 ......
A device can be blocked from the services (see device Editor). This is not yet implemented. Currently the devices acquire a dynamic IP address on the local network. WiFi credentials (ssid and password) are currently fixed. It requires a construction with the board acting as an access point to allow end-users to set their own parameters. Static adressess are also possible, but the details are not yet implemented.

![alt text](https://github.com/goofy2k/ESPfleet/blob/main/media/Screenshot_Device_Editor.jpg?raw=true)

bla bla
