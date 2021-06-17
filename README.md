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


![alt text](https://github.com/[Goofy2k]/ESPfleet/blob/main/media/Screenshot_Device_Editor.jpg?raw=true)
