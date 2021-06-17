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

For each board that contacts the MQTT broker for the first time the properties are collected in a table in the Fleet mgr. The table is updated, every time that a board finishes a wake-up deepsleep cycle.  With the clear fleet button, the table is erased. The input field and  "refresh table" button are obsolete.

Above the table the properties are listed of the device that was the last to send an update.

All information in the table are received from the devices over MQTT/WiFi.

- The devices are listed in the order of first contact after the last "clear fleet" action.
- The unique deviceId contains it's MAC address in reverse order. The last 4 hex digits in this code are representative for the board type (e.g. Heltec LORA32 (01E6E2E0), TTGO LORA32 (FAC40A24)
- A device can be blocked from the services (see device Editor). This is not yet implemented. 
- Currently the devices acquire a dynamic IP address on the local network. WiFi credentials (ssid and password) are currently fixed. It requires a way of working with the board acting as an access point to allow end-users to set their own parameters. Static IP adressess are possible in principle, but the details are not yet implemented. 
- lowpowermode is in principle on. This means that, in order to save battery power, after performing it's tasks and reporting it's status the device is going into deepsleep mode.
- sleeptime is the duration of the deepsleep period in seconds. Note that this is NOT the same as the cycle time, which is the period between two subseqent wake ups an is equal to sum of the time required to execute the tasks (awake time) and the sleeptime. The awaketime depends on the tasks and delays in the firmware. Some of these taks may have a variable duration (e.g. connection to the network).  
-  versionStr is the version information that is present in the firmware
-  autoupdate can be used to prevent automatic download of firmware. Currently firmware is auto-updated when availability is announced to a board (see device editor)
-  firscontact is the time of first contact of a board (after the last "clear table" action) 
-  lastcontact is the time of last contact and can be used detect if a board is still actively excuting cycles.


![alt text](https://github.com/goofy2k/ESPfleet/blob/main/media/Screenshot_Device_Editor.jpg?raw=true)

A number of the entries should be clear from the description of the fleet manager.

In the right part of the device editor a device can be selected with a dropdown menu. Currently this is via it's index number in the Fleet mgr table. The active settings for the selected device are loaded into the form on the left side, wher they can be edited. 
When the update firmware switch is set, the selected device will download the binary file from the server (host) at the indicated location (path). This will always be done after execution of the main tasks, just before going to sleep. After a succesfull download, the device will reboot and start the new firmware immediately. The (possibly updated) tasks will be executed again before the board goes to sleep again. 

Commands by the Fleet mgr are sent to the MQTT broker by pressing the Submit button. The broker sends the message when the board is awake or stores it and sends it when the board wakes up (QoS = 1). 

The update all devices is not active yet. It can be used e.g. to sent an "update available" message to all active device instead of to only the edited device.

The cancel and stop buttons are obsolete.
