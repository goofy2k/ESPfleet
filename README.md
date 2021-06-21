# ESPfleet
 Manage fleet of ESP32/ESP8266 boards with Nodered
 
## Contents

1. Goal
2. About this code
3. Currently running board fleet hardware
4. Fleet management dashboard (Nodered)
    - Fleet mgr
    - Device editor
5. TODO
6. Version history
7. References / Further reading  
 
 
## 1. Goal
 
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
 
## 2. About this code 

The ESP firmware is written in the Arduino IDE. Development Notes have been kept in a README.md.ino file.
The .ino extension of this file is necessary to open it for editing in a tab in the Arduino IDE 
Versions have until now been managed by creation of new folders, accessible by the Arduino IDE. It is yet
unknown to me how to handle versioning with Github / Arduino IDE.
 
## 3. Currently running board fleet hardware

    - 1x Heltec LORA32 .....
    - 2x TTGO LORA32 ....
    
    power supply: 1000 mAh Lithium Rechargeable Battery (MakerFocus)
    
## 4. Fleet management dashboard (Nodered)

The management dashboard is implemented in Nodered. In this repository only the main functionality of the dashboard is described. 

The dashboard consists of 2 screens:

1. The ***Fleet mgr***, with an overview of devices in the fleet.
2. A ***Device Editor*** for setting parameters of a device and announcing/triggering a firmware update


![alt text](https://github.com/goofy2k/ESPfleet/blob/main/media/Screenshot_Fleet_Mgr.jpg?raw=true)

For each board that contacts the MQTT broker for the first time the properties are collected in a table in the Fleet mgr. The table is updated, every time that a board finishes a wake-up deepsleep cycle.  With the ***clear fleet*** button, the table is erased. The ***input field*** and  ***refresh table*** button are obsolete.

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

The ***cancel*** and ***stop*** buttons are obsolete.

## 5. TODO

* TODO  
*   - must report back if update fails
*   - re-introduce reconnect to wifi. It was lost somehow. See: https://github.com/marvinroger/async-mqtt-client/blob/master/examples/FullyFeatured-ESP8266/FullyFeatured-ESP8266.ino  
*   - move your self-created re-usable header files to a central folder, e.g. My Documents\Arduino\libraries\
*   - Find a way to let the boards know which updates are available.
*   - Introduce autoupdate, switch it on only for the alfa/beta tester. Put it on for the others when it is safe to release   
*     This implies that an owner of a device must have a means to allow or forbid autoupdates. He must have a dashboard to control his personal
*     fleet. Start with creating pseudo code for autoupdate.
*
* >>> TODO <<<
*   - Autodetection and subsequent handling of attached sensors / hardware (e.g. T-sensor, battery voltage)
*   - Make it an option to send the edited deviceData to all devices (= bulk update).  
*   - On implementing the above: make all decisions on the server side. Do not bother the devices with that.
*     Do not send MQTT messages to a device when it does not need to take an action.
*   -  Repair the Serial.print  for despatcher payload
*   - Logging tasks
*     * check if usage of WiFiEvents is optimal
*     * DONE introduce a way to monitor duty cycle for optimization towards low power applications. This requires recording of duration between wake ups.
*     * do analysis and (possibly) statistics in cloud service
*     * find solution for phase shift of 1 cycle for the cycleDuration 
*   - implement MQTT command to trigger and control updates and other tasks
*     * introduce command to change logging mode
*   - compatibility with fleet: adapt the app such that it can be used by other boards  
*     *remove dependency on heltec.h (remove interaction with display, ot use general display driver)
*     *distinguish between cloud tasks for board owners and task for fleet manager  
*/


## 6. Version history 

/*
* V1.0
* WiFi and MQTT operational 
* Battery voltage readout sometimes gives high values 
*   - are the used pins in the combined sketches compatible?
*   - does introduction of WiFi interfere with the battery measurement?
*   - is the combination of the two loop delay methods sloppy?
*   - is it lack of conversion time for the ADC?
*   
* V1.1 added some extra code with settings to do ADC conversion properly  
*
* V1.2 adding deep sleep to save battery power in idle time
*   - https://randomnerdtutorials.com/esp32-timer-wake-up-deep-sleep/
*   - https://raw.githubusercontent.com/RuiSantosdotme/ESP32-Course/master/code/DeepSleep/TimerWakeUp/TimerWakeUp.ino
*   - need to move the includes to the top of the code, to prevent an error when compiling RTC_DATA_ATTR int wakeupCount = 0;
*   - BUG: after waking up, connecting to the WiFi network does not work.
*          connection is OK after pressing the RST button. 
* V1.3 created more reliable WiFi reconnect using WiFi events         
*   - BUG: delivery of data stops, reason unknown. Battery voltage at the moment of "crash" was high enough.
*   - introduce tools for debugging:
*         * introduce logging, while not connected to serial port
*         * introduce ESP side logging (Use different levels. Set level in Preferences)  
*         * introduce logging on cloud server (a.o. battery voltage, timestamp, deviceId)
*   - SOLVED: not all info printed to Serial, solved by moving display initialization 
*   - added timers for optimization / rationalization of delays      
*   - BUG: awakeDuration should be the longest duration of all, It isn't.
*   - introduce reporting of timers in wireless / battery mode (MQTT)
* V1.4 solve V1.3 issues   
*   - must be able to use MQTT client in any scope
*   - Optimization of delays and uptime for battery life
*     - successDuration takes most of the awake time. Room for improvement in speed of making connection with WiFi 

* V1.4.1 introduce WiFiEvents for more reliable connection   
*   - reverted to PubSubClient (synchronous), because AsyncMQTT client does not 
*     properly handle QoS > 0 and cleanSession.
*   - asynchronous method of connecting to Wifi in V1.5 was elegant. Less spread in connection time.  
*   - added deviceId, a string representing the board's MAC adress in reverse order
*   - added MQTT topics containing the unique deviceId
*     * prevent using String variables. See
*   - a logging flag is present, which persists over reboots by storing it into flash memory
*     * further logging methods to be implemented
*     
* V2.0 proceed on branch with PubSubClient   
*    - This version properly receives retained messages from the broker
*      This could be a work-around in case persistent session / clean session and QoS 1 or 2 does not work
*      to receive messages that were sent to the broker while the board was sleeping
*    - Non-retained messages with QoS > 0 are now received when they were sent while the recipient was asleep.  
*      This was done by using a FIXED clientId (essential) and a mqtt_disconnect() just before going to sleep
*      It appears that a disconnect is not necessary for receipt of these messages
*                   2) how to prevent that a retained message overrides the info in an other message 
*                      test for retained flag, how?
*                   3) is re-subscription neccesary after wake up?   
*    - Introduced reporting of the number of wake-ups (wakeupCount)               
*    - Introduced timer for real cycle length and report that to the cloud server
*    - It looks like not all wake-ups generate data. Detect missed cycles in Nodered by calculating interval between receipts of messages 
*      This number is a multiple of the cycle length if reports are missed
*    - Introduced timers for connection / disconnection. This is for evaluation of connection success and power consumption.
*    - Moved useful payload (battery voltage measurement) to separate routine outside setup()
*    -Introduced a command via MQTT for setting sleep time      
*    
* V2.1   
*   - cycles are very stable when voltage measurements are absent. Are these a cause of timing differences in voltage measurements?
*   - By commenting out Heltec dependencies, the code also runs on the TTGO Lora32 board (provided that compilation takes place for ESP 32 Dev Module)
*   - Must provide command to set sleep time with proper device code 
*     
*   - How to re-introduce display settings?
*   - How to disable/enable voltage measurements per board?
*   - This introduces FLEET DEPENDENT issues:
*       * How to send the same command to the whole fleet? Use smart topic wildcards 
*       * DONE Reception interval measurement i Nodered must be done per device. Can be done in interval node (check "by topic")
*         If not: then intervals represent phase shifts, showing the differences in interval lengths
*       * How to introduce board dependent code? E.g. display drivers. (Conditional compile?)
*       * OTA updates must be done with board dependent bin files
*       * DONE How to maintain a list of fleet members and their activity in Nodered?
*       * Boards from the same manufacturer have the last 4 bytes in common in the deviceId. Consider to reverse the order of the deviceId
*         to enable sorting per manufacturer.
* V2.1.1      
*   - Version branching from the PubSub version, but now based on AsyncMQTT (master)
*   - After successful introduction of AsyncMQTT, consider to switch to the dev version of it for automatic disconnect after all messages have been sent.
*   - Introduced AsyncElegant OTA (including AsyncWebServer) TO BE TESTED
*   - Allow user to switch sleep mode (temporarily) off and switch it ON again. This helps to perform OTA with user interaction via a browser
*   - Allow sending of all device dat in 1 MQTT message. This enables efficiently showing more data in a table row of the fleet manager
*     * DONE Introduce formation of a deviceData object and conversion to a JSON string, that will be sent via MQTT.
*     * DONE Add "sleepmode" and firmware version info to the deviceData*   

*     
* V2.2.1    
*   - TO BE DONE when updating over OTA is activated, sleepmode must be temporarily swithched off. This holds for autoupdate or update over web server.  
*     * TO BE DONE When sleepmode is de-activated, and webserver for ElegantAsyncOTA is started, device must send a message to the dashboard that user must connect
*     * to http\\<IP>/update (this is the same condition as sleepmode = false. If you leave than condition live too long, it will drain the battery
*     
*   - introduce extended firmware versioning      
*     * DONE contains the code version (e.g. V2.1.1)
*     * TO BE DONE  contains the compilation method (board type), injected automatically at compilation time
*     * TO BE DONE upon first boot of new firmware, the device must store the update datetime (last update)
*     * TO BE DONE the device reports all of the above to the fleet server
*   - introduce "clean" MQTT payload:  you must use the len parameter oft the onMessage callback to trim the payload buffer
*       see: http://www.steves-internet-guide.com/arduino-sending-receiving-json-mqtt/
*     * TO BE DONE: make sure, e.g. with a time-out, that the update loop does not take too long. 
*       practice with a loop without updating
*       
**************************************************************************************************************************       
* REMARK: the Nodered dashboard is essentially single user. It might not be very useful for interaction of individual users with
*         their device or with their device's data. Implementation of such an interface on the device itself may be a too heavy load.
*         Simple interactions can be done via a small server / interface on the board. Others tasks (data analysis / inspection) may 
*         have to be implemented in a separate user application that does not or very limited contact the device but only with the cloud 
*         data. SO: do not invest too much in end-user interaction via Nodered!
* **************************************************************************************************************************************
* 
* V2.2.2
*   - laid the foundations for handling the firmware update:  must implement timeout befofore proceeding
*   - ElegantAsyncOTA can update with files that are on the same machineas the ??? How about remote update files on e.g. github or via a remote FTP
*     connection with a NAS
* V2.3 -V2.4  tested updating over the air (while wired, while un-wired)     
*   -DONE if received, persist updatefirmware in Preferences, such that it will be available in the next wake-up,  
* 
* V2.5   
*   - After connect, actively unsubscribe from all channels (#). Otherwise old, unwanted subscriptions can still be active.
*   - Do NOT subscribe to channels for all devices. Only subscribe to channels for this device. This saves decision making at the device side.
*     Let the server make the decisions and send only device-speficic messages.
*   - Introduced announcement of update by filename (fixed filename and path in this version) 
*   
* V2.6
*   - Exported the comments and TODO's to this separate README.MD.ino file
*   - If failed, and update trigger persisted in Preferences, the device will try in the next wake-up 
*     count the trials. Report the counters to the cloud service and track them for reliability assessment. 
*     If more than 5 (N), stop trying. 
*     
* V2.7.1a     
*   - implementd code for persisting the first payload in an array of payload_str
*   - BUG in V2.6? It looks like a new sleeptime is not taken ove rin the next cycle, but in the one after that.
*   - Have a look at missed commands. Prevent that by storing that in Preferences until it has been executed? (NOTE: commands come in
*     asynchronously and during sleep).
*     * when a command comes in, store it in a command stack.
*       * lead: "We then convert the received jSON string into a doc object and access the values using the same syntax we used 
*         for create them and print out a few just to confirm that it all works. "
*         see: http://www.steves-internet-guide.com/arduino-sending-receiving-json-mqtt/
*     * take care: a command comes in in the deviceData_doc. Is it OK to store more than one of those???
*     
*     * execute commands in the stack just before going to sleep.
*     * do this in a while loop over the individual commands, this makes sure that all received commands have been handled befor going to sleep
*     * be careful with time-consuming commands.... These my be divided into smaller subtasks (LOW PRIORITY)
*     
* V2.7.2a    
*     * ABANDONED: last version of branch with persisted MQTT payloads in a stack of payloads
*     * too complex 
*     * opened a new branch:  handle the MQTT message immediately on receipt, but limit the handling to setting relevant flags, handle the consequens 
*       of flags just before going to sleep. This removes the need to persist the MQTT payloads.
* V2.7.3a      
*     *reverted back to direct storage of flags in the onMQTTmessage handler
*     
* V2.7      
*     *First version with OTA with a flexible host / path
*     
* V2.8
*     * made sure that triggering an OTA update is succesful in three phases of the WAKE-UP / SLEEP cycle
*       - during sleeptime. This update is done at the end of the next cycle.
*       - awake while waiting for connections. This update is done at the end of the current cycle. 
*       - awake while waiting before going asleep. This update is done at the end of the next cycle.
*       
* V2.9  - In retrieval from Preferences at waking up: Set default values for updatehost and update path to a stable version that 
* 
*         can be updated over OTA >>>>>> BP_LORA32_AsyncMQTT_V2.9.ino.esp32.bin
* V2.10    

## 7. References / Further reading 

1. https://blog.arduino.cc/2021/06/18/14-awesome-arduino-cloud-features-you-never-knew-existed/
  
2. https://esphome.io/
   Offers a system for generating and maintaining firmware for ESP32/ESP8266
   An interesting feature is the automatic generation of C++ source code with a .yaml file as input

3. https://www.home-assistant.io/
   Home Assistant can be used to deal with the data generated by the boards that are maintained with ESPHome.
 
4.   This sketch shows the WiFi event usage - Example from WiFi > WiFiClientEvents
   Complete details at https://RandomNerdTutorials.com/esp32-useful-wi-fi-functions-arduino/
 
5.  Simple Deep Sleep with Timer Wake Up
=====================================
ESP32 offers a deep sleep mode for effective power
saving as power is an important factor for IoT
applications. In this mode CPUs, most of the RAM,
and all the digital peripherals which are clocked
from APB_CLK are powered off. The only parts of
the chip which can still be powered on are:
RTC controller, RTC peripherals ,and RTC memories

This code displays the most basic deep sleep with
a timer to wake it up and how to store data in
RTC memory to use it over reboots

This code is under Public Domain License.

Author:
Pranav Cherukupalli <cherukupallip@gmail.com>
 
 6. 
/* 
 * HelTec Automation(TM) Electricity detection example.
 *
 * Function summary:
 *
 * - Vext connected to 3.3V via a MOS-FET, the gate pin connected to GPIO21;  //BUT WHERE IS THE CODE THAT DOES THAT?
 *
 * - Battery power detection is achieved by detecting the voltage of GPIO13;  //SHOULD BE GPIO37    what is the meaning of GPIO36 in this sketch?
 *
 * - OLED display and PE4259(RF switch) use Vext as power supply;
 *
 * - WIFI Kit series V1 don't have Vext control function;
 *
 * HelTec AutoMation, Chengdu, China.
 * ??????????????
 * https://heltec.org
 * support@heltec.cn
 *
 * this project also release in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * 
*/ 
 
 7.
 
 /* Code by FV for WIFI Lora 32(V2) board, derived from File / Examples / Heltec ESP32 Dev-Boards / ESP32 / ADC_Read_Voltage / Battery_power 
 * The original example gives a correct battery voltage readout on the display and a raw value in the Serial output .
 * The aim is to modify the code to send MQTT readouts to a web-service 
 * This requires establishing a Wifi connection (station mode) and introduction of an MQTT client
 * 
*/  
