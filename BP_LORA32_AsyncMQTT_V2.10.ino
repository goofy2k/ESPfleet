/* Code by FV: June 2021
 * See README.MD for more information 
 */
//handling of null-terminated strings:
//see: http://www.cs.ecu.edu/karl/3300/spr14/Notes/C/String/nullterm.html
//conversion:
// char* mychararray = myString.c_str()  
// String myString = String(mychararray);

#define VERSIONSTR "V2.10 build 1" //this represents the file and folder name
//#define BUILDSTR   "21";     //this represents the build nr (compiled firmware nr of same source code folder) 

#include "Arduino.h"

#include <Preferences.h>  //not required in Arduino IDE ( NOT TRUE !)
//https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
//also see info on how to remove a namespace
Preferences preferences;     //create an instance (preferences) of Preferences
#define PREFERENCES_READ_ONLY = false //use preferences in R/W mode
#define MAXOTATRIALS  5
 
#include <Wire.h>
//#include "heltec.h"         //study conditional includes/ compilation
//#include "esp_wifi.h"
//#include <WiFi.h>
#include "wifi_connect.h"
//#include <ESP8266WiFi.h>
//#include <PubSubClient.h>   //Knoleary MQTT module
#include <ArduinoJson.h> 

extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}

//prepare for AsyncElegantOTA
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>

#include <HTTPClient.h>     //for downloading files from server
#include "httpUpdate_FV.h"  //simple OTA derived from httpUpdate.ino example

unsigned int payload_count =0;

//AsyncWebServer server(80);


/*
AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
*/
//TimerHandle_t wifiReconnectTimer;

int sleeptime;
bool lowpowermode;
bool autoupdate;
bool updatefirmware;
bool blocked;
bool loggingMode;
bool OTAsuccess;
unsigned int OTAcounter;
String updatehostStr;
String updatepathStr;
bool proceed = false;

#define uS_TO_S_FACTOR 1000000  /* Conversion factor for micro seconds to seconds */
#define DEFAULT_SLEEPTIME 60    /* Time ESP32 will go to sleep (in seconds) */
/*
 preferences.putBool("lowpowermode",lowpowermode); 
  preferences.putUInt("sleeptime",sleeptime);
  preferences.putBool("autoupdate",autoupdate); 
  preferences.putBool("updatefirmware",updatefirmware); 
  preferences.putBool("blocked",blocked); 
*/




const char* updatehost;
const char* updatepath;
bool globalupdate;

const char * VersionStr = VERSIONSTR;
//hardware version 
//firmware / build version
unsigned long update_time ;           //last update time  timestamp milliseconds 

//initialize cycle timing parameters
RTC_DATA_ATTR int wakeupCount = 0;    //store in ULP, keep value while sleeping
unsigned long wakeupTime;             //milliseconds;
time_t cycleStart;                    //for compatibility with on board RTC
RTC_DATA_ATTR time_t prevcycleStart;  //milliseconds;
unsigned long cycleDuration;          //milliseconds;

time_t now;

unsigned long sleepTime;              //
unsigned long connectStart;           //
unsigned long connectSuccess;         //

//for identification of the board, e.g. via MQTT topics
char deviceId[23] = "";
char deviceType[10] = "";
char deviceTopic[50] = "";

//get unique deviceId, it contains the MAC address in reverse order
//later also get the device type and Rev
void get_deviceId() {
  uint64_t chipid = ESP.getEfuseMac(); // The chip ID is essentially its MAC address(length: 6 bytes).
  snprintf(deviceId, 23, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
}

void get_deviceType() {
  snprintf(deviceType, 23, "ESP32", "", "");
}




void wake_up_from_deep_sleep(){
  //measure cycleDuration in time units that persist over wake-ups
  time(&now); //get RTC time and assign to now variable  
              //this value is NOT reset by waking up, but only at reboot   millis() starts at zero each time afte wake-up
  cycleStart = now;
  cycleDuration = cycleStart - prevcycleStart;
  wakeupTime = millis();

  
  
  Serial.begin(115200);
   
  delay(10); //Take some time to open up the Serial Monitor //was 1000

  
  //Method to print the reason by which ESP32 has been awaken from sleep
  print_wakeup_reason();

  Serial.println("now "+String(now));
  Serial.println("wakeupTime " + String(wakeupTime));
  Serial.println("cycleStart " + String(cycleStart));
  Serial.println("prevcycleStart " + String(prevcycleStart));
  Serial.println("cycleDuration "+ String(cycleDuration));  
  prevcycleStart = cycleStart; //store (in ULP)
  
  // Open Preferences with my-app namespace. Each application module, library, etc
  // has to use a namespace name to prevent key name collisions. We will open storage in
  // RW-mode (second parameter has to be false).
  // Note: Namespace name is limited to 15 chars.
  preferences.begin("my-app", false);

  // Remove all preferences under the opened namespace
  //preferences.clear();

  // Or remove the counter key only
  //preferences.remove("counter");

  // Get the counter value, if the key does not exist, return a default value of 0
  // Note: Key name is limited to 15 chars.
  unsigned int wakeup_counter = preferences.getUInt("wakeup_counter", 0);
  // Increase wakeup_counter by 1
  wakeup_counter++;
  // Store the wakeup_counter to the Preferences
  preferences.putUInt("wakeup_counter", wakeup_counter);
  // Print the wakeup_counter to Serial Monitor
  Serial.printf("Total nr of wakeups: %u\n", wakeup_counter);


  loggingMode =  preferences.getBool("loggingMode",true);   //must persist 
  lowpowermode = preferences.getBool("lowpowermode",false); 
  sleeptime = preferences.getUInt("sleeptime",DEFAULT_SLEEPTIME);
  blocked = preferences.getBool("blocked",false); 
  autoupdate = preferences.getBool("autoupdate",false); 
  updatefirmware = preferences.getBool("updatefirmware",false);
  updatehostStr = preferences.getString("updatehostStr","192.168.2.200");   //updatehost as String must be converted upon input
  updatepathStr = preferences.getString("updatepathStr","/ESP32_firmware/BP_LORA32_AsyncMQTT_V2.9.ino.esp32.bin");   //updatehost as String must be converted upon input
  OTAsuccess = preferences.getBool("OTAsuccess",false); 
  OTAcounter = preferences.getUInt("OTAcounter",0);
  
  Serial.println(">>>>> ON WAKE UP <<<<<");
  Serial.println("lowpowermode: "+String(lowpowermode));
  Serial.println("sleeptime: "+String(sleeptime) + " seconds");
  Serial.println("autoupdate: "+String(autoupdate));
  Serial.println("updatefirmware: "+String(updatefirmware));
  Serial.println("updatehostStr: "+updatehostStr);
  Serial.println("updatepathStr: "+updatepathStr);
  updatehost = updatehostStr.c_str();
  updatepath = updatepathStr.c_str();
  Serial.println("updatehost: "+String(updatehost));
  Serial.println("updatepath: "+String(updatepath));
  Serial.print("OTAsuccess: "); Serial.println(OTAsuccess);
  Serial.print("OTAcounter: "); Serial.println(OTAcounter);
  Serial.println("blocked: "+String(blocked));
  Serial.println("loggingMode: " + String(loggingMode));
  Serial.println(">>>>>          <<<<<");

  //previous update was succesfull, reset the updatefirmware trigger to prevent update going on and on and on 
  if (OTAsuccess){updatefirmware = false;};

  
//  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());   //ESP.getChipModel() 
//  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
//  Serial.print("Chip ID: "); Serial.println(chipId2);
get_deviceId();
Serial.println(deviceId);
get_deviceType();
Serial.println(deviceType);
//get device Ver ????

  // Close the Preferences
  preferences.end();

  //Increment boot number and print it every reboot
  ++wakeupCount;
  Serial.println("----- wakeupCount since last reboot: " + String(wakeupCount)+" -----");

  }



void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


StaticJsonDocument<512> deviceData_doc;

  void prepare_and_go_for_sleep(){

    
  /*
  First we configure the wake up source
  We set our ESP32 to wake up every 5 seconds
  */
  esp_sleep_enable_timer_wakeup(sleeptime * uS_TO_S_FACTOR);
  Serial.println("Prepare ESP32 to sleep for " + String(sleeptime) +
  " Seconds");

  /*
  Next we decide what all peripherals to shut down/keep on
  By default, ESP32 will automatically power down the peripherals
  not needed by the wakeup source, but if you want to be a poweruser
  this is for you. Read in detail at the API docs
  http://esp-idf.readthedocs.io/en/latest/api-reference/system/deep_sleep.html
  Left the line commented as an example of how to configure peripherals.
  The line below turns off all RTC peripherals in deep sleep.
  */
  //esp_deep_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_OFF);
  //Serial.println("Configured all RTC Peripherals to be powered down in sleep");

  /*
  Now that we have setup a wake cause and if needed setup the
  peripherals state in deep sleep, we can now start going to
  deep sleep.
  In the case that no wake up sources were provided but deep
  sleep was started, it will sleep forever unless hardware
  reset occurs.
  */

  Serial.println("Collecting and reporting connection timing data");
  //esp_wifi_disconnect();
  unsigned long disconnectTime = millis();
  sleepTime = millis();

  //currently each data-element is sent in a separate message. The value is in payload. The "key" is in the 
  //message topic. It is more efficient (on the device side AND on the cloud service side) to create ONE 
  //messag with all data-elements in the payload. This requires formation of a deviceData object and conversion
  //to a JSON string, that will be sent via MQTT.

// Follow this instruction: http://www.steves-internet-guide.com/arduino-sending-receiving-json-mqtt/
// For version 6: On the contrary, I recommend declaring a short-lived JsonDocument that you use only in your serialization functions.
// Use a StaticJsonDocument to store in the stack (recommended for documents smaller than 1KB)
// Here are the four techniques that you can use to determine the capacity of your JsonDocument. The technique you choose depends on the amount of knowledge you have on your document and your platform.
// See: https://arduinojson.org/v6/how-to/determine-the-capacity-of-the-jsondocument/



//StaticJsonDocument<256> deviceData_doc;
deviceData_doc.clear();
deviceData_doc["deviceId"] = deviceId;
deviceData_doc["blocked"] = blocked;
deviceData_doc["IP"] = WiFi.localIP();
deviceData_doc["lowpowermode"] = lowpowermode;
deviceData_doc["sleeptime"] = sleeptime;
deviceData_doc["VersionStr"] = VersionStr;
deviceData_doc["autoupdate"] = autoupdate;
deviceData_doc["lastupdate"] = "OTAstr"; //to be impemented with OTA
/* ADD THESE
  preferences.putBool("OTAsuccess",OTAsuccess); 
  preferences.putUInt("OTAcounter",OTAcounter);
*/
// Generate the minified JSON and send it to the Serial port.
//
char out[256];
int b =serializeJson(deviceData_doc, out);
Serial.print("JSON bytes = ");
Serial.println(b,DEC);

//create topic
 strcpy(deviceTopic,"");
 strcat(deviceTopic,"fleet/");
 strcat(deviceTopic,deviceId);
 strcat(deviceTopic,"/deviceData"); 

uint16_t packetIdPubD = mqttClient.publish(deviceTopic, 1, false, out);  
Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", deviceTopic, packetIdPubD);


    
unsigned long awakeDuration = sleepTime - wakeupTime;  
Serial.println("<><><> awakeDuration " + String(awakeDuration));
//create topic
 strcpy(deviceTopic,"");
 strcat(deviceTopic,"fleet/");
 strcat(deviceTopic,deviceType);
 strcat(deviceTopic,"/"); 
 strcat(deviceTopic,deviceId);
 strcat(deviceTopic,"/timers/awakeDuration"); 
// publish for AsyncMqtt
  uint16_t packetIdPub1 = mqttClient.publish(deviceTopic, 1, false, String(awakeDuration).c_str());                        
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", deviceTopic, packetIdPub1);
  //Serial.printf("Message: %.2f \n", String(awakeDuration).c_str());

  Serial.println("<><><> cycleDuration " + String(cycleDuration));
//create topic
 strcpy(deviceTopic,"");
 strcat(deviceTopic,"fleet/");
 strcat(deviceTopic,deviceType);
 strcat(deviceTopic,"/"); 
 strcat(deviceTopic,deviceId);
 strcat(deviceTopic,"/cycleDuration");   
// publish for AsyncMqtt
  uint16_t packetIdPub2 = mqttClient.publish(deviceTopic, 1, false, String(cycleDuration).c_str());                           
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", deviceTopic, packetIdPub2);
  //Serial.printf("Message: %.2f \n", String(cycleDuration).c_str());


  Serial.println("<><><> wakeupCount " + String(wakeupCount));
//create topic
 strcpy(deviceTopic,"");
 strcat(deviceTopic,"fleet/");
 strcat(deviceTopic,deviceType);
 strcat(deviceTopic,"/"); 
 strcat(deviceTopic,deviceId);
 strcat(deviceTopic,"/counters/wakeupCount");
// publish for AsyncMqtt
  uint16_t packetIdPub3 = mqttClient.publish(deviceTopic, 1, false, String(wakeupCount).c_str());                           
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", deviceTopic, packetIdPub2);
  //Serial.printf("Message: %.2f \n", String(wakeupCount).c_str());


unsigned long connectDuration = disconnectTime - connectStart; 
Serial.println("<><><> connectDuration " + String(connectDuration));
 strcpy(deviceTopic,"");
 strcat(deviceTopic,"fleet/");
 strcat(deviceTopic,deviceType);
 strcat(deviceTopic,"/"); 
 strcat(deviceTopic,deviceId);
 strcat(deviceTopic,"/timers/connectDuration");   
// publish for AsyncMqtt
  uint16_t packetIdPub5 = mqttClient.publish(deviceTopic, 1, false, String(wakeupCount).c_str());                           
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", deviceTopic, packetIdPub5);
  //Serial.printf("Message: %.2f \n", String(connectDuration).c_str());

unsigned long successDuration = connectSuccess - connectStart;
Serial.println("<><><> successDuration " + String(successDuration));
strcpy(deviceTopic,"");
strcat(deviceTopic,"fleet/");
strcat(deviceTopic,deviceType);
strcat(deviceTopic,"/"); 
strcat(deviceTopic,deviceId);
strcat(deviceTopic,"/timers/successDuration"); 
// publish for AsyncMqtt
  uint16_t packetIdPub4 = mqttClient.publish(deviceTopic, 1, false, String(successDuration).c_str());                           
  Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", deviceTopic, packetIdPub4);
  //Serial.printf("Message: %.2f \n", String(successDuration).c_str());

  
  Serial.println("----- Waiting 15 seconds for going to sleep -----");
   delay(15000); 
//save relevant parameters in settings
//they will be retrieved after wake up
  Serial.println(">>>>> ON SLEEP <<<<<");
  Serial.println("lowpowermode: "+String(lowpowermode));
  Serial.println("sleeptime: "+String(sleeptime) + " seconds");
  Serial.println("autoupdate: "+String(autoupdate));
  Serial.println("updatefirmware: "+String(updatefirmware));
  Serial.print("updatehost: "); Serial.println(updatehostStr);
  Serial.print("updatepath: "); Serial.println(updatepathStr);
  Serial.print("OTAsuccess: "); Serial.println(OTAsuccess);
  Serial.print("OTAcounter: "); Serial.println(OTAcounter);
  Serial.println("blocked: "+String(blocked));
  Serial.println("loggingMode: " + String(loggingMode));
  Serial.println(">>>>>          <<<<<");

  //save in preferences
  preferences.begin("my-app", false);
  preferences.putBool("loggingMode",loggingMode); 
  preferences.putBool("lowpowermode",lowpowermode); 
  preferences.putUInt("sleeptime",sleeptime);
  preferences.putBool("autoupdate",autoupdate);
  preferences.putBool("blocked",blocked); 
  preferences.putBool("updatefirmware",updatefirmware); 
  //THESE MUST BE SAVED AFTER RECEIVING THEM, as this code is not reached after an update....
  //updatehost as String
  //updatehost as bytes
  //updatepath as String
  //updatepath as bytes  
  preferences.putBool("OTAsuccess",OTAsuccess); 
  preferences.putUInt("OTAcounter",OTAcounter); 
  preferences.end();
  //MUST ALSO SAVE UPDATEPATH AND UPDATEHOST
  //NO LARGE DELAY AFTER SAVING PREFERENCES TO MINIMIZE THE RISK OF CHANGING SOME (updatefirmware) AFTER SAVING
Serial.println("----- Going to sleep now -----");
Serial.println();
delay(10);
Serial.flush(); 
esp_deep_sleep_start();
Serial.println("This will never be printed"); //unreachable code
}



//battery parameters
#define Fbattery    3700  //The default battery is 3700mv when the battery is fully charged.

float XS = 0.00225;      //The returned reading is multiplied by this XS to get the battery voltage.
uint16_t MUL = 1000;
uint16_t MMUL = 100;


/*
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];
//int value = 0;
*/

/*
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);

  switch (event) {
    case SYSTEM_EVENT_WIFI_READY: 
      Serial.println("WiFi interface ready");
      break;
    case SYSTEM_EVENT_SCAN_DONE:
      Serial.println("Completed scan for access points");
      break;
    case SYSTEM_EVENT_STA_START:
      Serial.println("WiFi client started");
      break;
    case SYSTEM_EVENT_STA_STOP:
      Serial.println("WiFi clients stopped");
      break;
    case SYSTEM_EVENT_STA_CONNECTED:
      Serial.println("Connected to access point");
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
    case SYSTEM_EVENT_STA_AUTHMODE_CHANGE:
      Serial.println("Authentication mode of access point has changed");
      break;
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("Obtained IP address: ");
      //Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_LOST_IP:
      Serial.println("Lost IP address and IP address is reset to 0");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_SUCCESS:
      Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_FAILED:
      Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_TIMEOUT:
      Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
      break;
    case SYSTEM_EVENT_STA_WPS_ER_PIN:
      Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
      break;
    case SYSTEM_EVENT_AP_START:
      Serial.println("WiFi access point started");
      break;
    case SYSTEM_EVENT_AP_STOP:
      Serial.println("WiFi access point  stopped");
      break;
    case SYSTEM_EVENT_AP_STACONNECTED:
      Serial.println("Client connected");
      break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
      Serial.println("Client disconnected");
      break;
    case SYSTEM_EVENT_AP_STAIPASSIGNED:
      Serial.println("Assigned IP address to client");
      break;
    case SYSTEM_EVENT_AP_PROBEREQRECVED:
      Serial.println("Received probe request");
      break;
    case SYSTEM_EVENT_GOT_IP6:
      Serial.println("IPv6 is preferred");
      break;
    case SYSTEM_EVENT_ETH_START:
      Serial.println("Ethernet started");
      break;
    case SYSTEM_EVENT_ETH_STOP:
      Serial.println("Ethernet stopped");
      break;
    case SYSTEM_EVENT_ETH_CONNECTED:
      Serial.println("Ethernet connected");
      break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
      Serial.println("Ethernet disconnected");
      break;
    case SYSTEM_EVENT_ETH_GOT_IP:
      Serial.println("Obtained IP address");
      break;
    default: break;
}}
*/
/*
//NEW, later extend with the above cases, useful for debugging only
void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      Serial.println(WiFi.localIP());
      connectToMqtt();
      break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
      xTimerStart(wifiReconnectTimer, 0);
      break;
  }
}
*/

void onMqttConnect(bool sessionPresent) {
  connectSuccess = millis();    //record success time
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  //subscribe to relevant topics
  
  /*
  uint16_t packetIdSub = mqttClient.subscribe("test/lol", 2);
  Serial.print("Subscribing at QoS 2, packetId: ");
  Serial.println(packetIdSub);
  */
  
  uint16_t packetIdSubcross = mqttClient.unsubscribe("#");
  Serial.print("unsubscribing Topic: #, packetId: ");
  Serial.println(packetIdSubcross);
  
//replace this by 1 signgle subscription to fleet/command/#
//handle the different commands in a command API handler

//be careful with channels for the whole fleet, prevent filtering on the device side. Do this on the server side 
/*
  uint16_t packetIdSub = mqttClient.subscribe("fleet/command/#", 1);
  Serial.print("Subscribing at QoS 1,Topic: fleet/command/set_t, packetId: ");
  Serial.println(packetIdSub);
*/
  
  uint16_t packetIdSubcross2 = mqttClient.unsubscribe("fleet/command/#");
  Serial.print("Subscribing at QoS 1, Topic: fleet/command/#, packetId: ");
  Serial.println(packetIdSubcross2);
  
//create topic

   strcpy(deviceTopic,"");
   strcat(deviceTopic,"fleet/command/updatedevice/");
   strcat(deviceTopic,deviceId);

   
   //mqtt_client.publish(deviceTopic, msg);
//for device specific messages, put deviceId in the substription. This prevents the need for filtering overhead by the board)
  uint16_t packetIdSubu = mqttClient.subscribe(deviceTopic, 1);
  Serial.print("Subscribing at QoS 1,Topic: ");Serial.print(deviceTopic); Serial.print(", packetId: ");
  Serial.println(packetIdSubu);


  uint16_t packetIdtest = mqttClient.subscribe("testMQTT", 1);
  Serial.print("Subscribing at QoS 1,Topic: ");Serial.print("testMQTT"); Serial.print(", packetId: ");
  Serial.println(packetIdtest);

  
  /*
  uint16_t packetIdSub = mqttClient.subscribe("fleet/command/set_t", 1);
  Serial.print("Subscribing at QoS 1,Topic: fleet/command/set_t, packetId: ");
  Serial.println(packetIdSub);
  uint16_t packetIdSub = mqttClient.subscribe("fleet/command/manualOTA", 1);  //  
  Serial.print("Subscribing at QoS 1,Topic: fleet/command/set_t, packetId: ");
  Serial.println(packetIdSub);
   uint16_t packetIdSubs = mqttClient.subscribe("fleet/command/set_sleepmode", 1);
  Serial.print("Subscribing at QoS 1,Topic: fleet/command/set_t, packetId: ");
  Serial.println(packetIdSubs);
   uint16_t packetIdSubb = mqttClient.subscribe("fleet/command/set_blocking", 1);
  Serial.print("Subscribing at QoS 1,Topic: fleet/command/set_t, packetId: ");
  Serial.println(packetIdSubb);
  */
  
  //when connected, prepare for sleep already !!!
  //if (lowpowermode) {prepare_and_go_for_sleep();}
  
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.print("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.print(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.print("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void set_t(int time_to_sleep){
  Serial.print("Setting time_to_sleep to "); Serial.print(time_to_sleep); Serial.println(" seconds");     
  //save in preferences
  preferences.begin("my-app", false);
  preferences.putInt("time_to_sleep",time_to_sleep);
  preferences.end();
  }
 
void command_set_t(char* payload , unsigned int length){
  Serial.print("New sleep time: "); Serial.println(atoi(payload));
  // Convert the payload
  char format[16];
  snprintf(format, sizeof format, "%%%ud", length);  
  int payload_value = 0;
  if (sscanf((const char *) payload, format, &payload_value) == 1){  //???????
    //Serial.println();
    set_t(payload_value);
  }
  else{
    Serial.println("command_set_t  Conversion error occurred"); // Conversion error occurred
    }; 
}

void command_API(char* topic, size_t topic_len, char* payload,size_t payload_len){
  Serial.println("Command API call. Action required");
  //strip of "fleet/command/" and implement a switch / case despatcher on the command....
  //sscanf() itself has some pretty strong pattern matching capabilities. I don't know how efficient they are
  
  Serial.print("topic "); Serial.println(topic);
  char * subStr = "fleet/command/";
  char  *newtopic =  &topic[strlen(subStr)];
  //consider to construct a way based on char* strstr(const char* haystack, const char* needle)
  //...Return a pointer to the first occurrence of substring needle in string haystack, or return NULL if there is none. Both haystack and needle are null-terminated strings.
 
  Serial.print("despatcher newtopic "); Serial.println(newtopic); 
  
  //must trim payload using payload_len
  Serial.print("despatcher payload "); Serial.println(payload);
  
  if (String(topic).indexOf("set_t") > 0) {
    Serial.println("despatcher Received command: set_t");
    command_set_t(payload, payload_len);
    }
  else if (String(topic).indexOf("manualOTA") > 0) {      //this tests if topic CONTAINS the substring,  it should be equal...howto
    Serial.println("despatcher Received command: manualOTA");
    }

  else if (String(topic).indexOf("confirmation") > 0) {      //this tests if topic CONTAINS the substring,  it should be equal...howto
    Serial.println("despatcher Received command: confirmation");
    deserializeJson(deviceData_doc,payload);
    Serial.print("confirmed update: ");Serial.println(payload);
    if (String(payload).indexOf("Yes") > 0) 
    {proceed = true;} else {proceed = false;};
    }
    
    else if (String(topic).indexOf("updatedevice") > 0) {      //this tests if topic CONTAINS the substring,  it should be equal...howto
      Serial.println("despatcher Received command: updatedevice");
      Serial.println("now going to update parameters based on the content of payload");
      // info on callback in http://www.steves-internet-guide.com/arduino-sending-receiving-json-mqtt/
    
      //StaticJsonDocument <256> deviceData_doc;
      deserializeJson(deviceData_doc,payload);
      // deserializeJson(doc,str); can use string instead of payload
      
      //blocked
      blocked = deviceData_doc["blocked"];
      Serial.print("blocked: "); Serial.println(blocked);      
      //autoupdate
      autoupdate = deviceData_doc["autoupdate"];
      Serial.print("autoupdate: "); Serial.println(autoupdate);        
      //sleeptime
      sleeptime = deviceData_doc["sleeptime"];  //update of this is also performed in command_set_t(), look at that
      set_t(sleeptime);
      Serial.print("sleeptime: "); Serial.println(sleeptime);  
      //lowpowermode
      lowpowermode = deviceData_doc["lowpowermode"];  
      Serial.print("lowpowermode: "); Serial.println(lowpowermode);  
      //updatefirmware
      updatefirmware = deviceData_doc["updatefirmware"];
      OTAsuccess = !updatefirmware;  //remove previous succes  
       
      Serial.print("updatefirmware: "); Serial.println(updatefirmware); 
      //updatehost
      updatehost = deviceData_doc["updatehost"]; 
        String convertedhost = String(updatehost);
  
      Serial.print("updatehost: "); Serial.print(updatehost);Serial.print("   ");Serial.println(convertedhost);
      //updatepath
      updatepath = deviceData_doc["updatepath"];  
      String convertedpath = String(updatepath); 
      Serial.print("updatepath: "); Serial.print(updatepath);Serial.print("   ");Serial.println(convertedpath);
        //save in preferences
  preferences.begin("my-app", false);
 
  //THESE MUST BE SAVED AFTER RECEIVING THEM, as this code is not reached after an update....
  preferences.putString("updatehostStr",String(updatehost));   //updatehost as String


  //preferences.putString("updatehostStr","testhost");   //updatehost as String
  //updatehost as bytes
  preferences.putString("updatepathStr",String(updatepath));   //updatepath as String
  //preferences.putString("updatepathStr","testpath");   //updatepath as String
  //updatepath as bytes
  preferences.end();
      //globalupdate
      
      globalupdate = deviceData_doc["globalupdate"];  
      Serial.print("globalupdate :"); Serial.println(globalupdate); 
    }  
  else {Serial.println("despatcher Received unrecognized command");
  }
  
  };

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  Serial.println("Publish received.");
  Serial.print("Meta topic: ");
  Serial.println(topic);
  Serial.print("Meta qos: ");
  Serial.println(properties.qos);
  Serial.print("Meta dup: ");
  Serial.println(properties.dup);
  Serial.print("Meta retain: ");
  Serial.println(properties.retain);
  Serial.print("Meta len: ");
  Serial.println(len);
  Serial.print("Meta index: ");
  Serial.println(index);
  Serial.print("Meta total: ");
  Serial.println(total);
  Serial.print("Meta payload: ");
  Serial.println(payload);
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
/*
  //copy payload buffer into null terminated string
  //this prevents that the payload will be lost due to overwriting by a subsequent message
  Serial.println("***3");
  char payload_str[len+1];
  Serial.println("***4");
  unsigned int topiclen = strlen(topic);
  Serial.println("***5");
  char topic_str[topiclen+1];
  Serial.println("***6");

  //see: http://www.cs.ecu.edu/karl/3300/spr14/Notes/C/String/nullterm.html
  strcpy(payload_str,payload); //replaces the byte-wise copy in Steve's ???
  Serial.println("***7");
  strcpy(topic_str,topic);
  Serial.println("***8");
  Serial.println();
  size_t payload_len =  strlen(payload_str);
  size_t topic_len =  strlen(topic_str);
  Serial.print("payload len: ");Serial.print(len);Serial.print(" payload_str length: ");Serial.println(payload_len);
  Serial.print("topic_str length: ");Serial.println(topic_len);

*/

  // act on individual topics
  // if (strcmp(topic, "fleet/command/set_t")==0) {
  // if(content.indexOf("Teststring") > 0)

     //strip off substring
     const char* substring = "fleet/command/";
     char * p;
     p = strstr (topic, substring);
/*  
  if (String(topic) == "fleet/command/set_t") {
    Serial.print("Received command: set_t");
    command_set_t(payload, len);
    }
*/
    
    if (p) { 
      // else if (String(topic).indexOf("fleet/command/") > 0) {    
      Serial.println("Received message. Action required");
   
//limit the actions to setting flags, for actions to be executed just before going to sleep
Serial.println("Calling command");    
//command_API(topic_str, topic_len, payload_str,len);
unsigned int topic_len = 0;
command_API(topic, topic_len, payload,len);
Serial.println("Returned from command_API");  
    }
  else {Serial.println("Received message. No action required");}  

}

void onMqttPublish(uint16_t packetId) {
  Serial.print("\nPublish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("Connected to AP successfully!");
}

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(IPAddress(info.got_ip.ip_info.ip.addr));
}

unsigned long previousMillis = 0;
unsigned long interval = 30000;

/*
void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}
*/

void prepare_MQTT(){
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.onMessage(onMqttMessage);
  //mqttClient.onMessage(onMqttMessageTest);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCleanSession(false);  //to receive messages that were sent to the broker while board was asleep
  //If your broker requires authentication (username and password), set them below
  mqttClient.setCredentials(MQTT_USER, MQTT_PASSWORD);
  };//setup_MQTT

 //cannot compile this for non Heltec boards
 /*
   void measure_Heltec_voltages() {
   
   //prepare ADC for battery voltage measurement
   //analogSetClockDiv(255); // 1338mS
   analogSetCycles(8);                   // Set number of cycles per sample, default is 8 and provides an optimal result, range is 1 - 255
   analogSetSamples(1);                  // Set number of samples in the range, default is 1, it has an effect on sensitivity has been multiplied
   analogSetClockDiv(1);                 // Set the divider for the ADC clock, default is 1, range is 1 - 255
   analogSetAttenuation(ADC_11db);       // Sets the input attenuation for ALL ADC inputs, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
   analogSetPinAttenuation(36,ADC_11db); // Sets the input attenuation, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
   analogSetPinAttenuation(37,ADC_11db);
                                        // ADC_0db provides no attenuation so IN/OUT = 1 / 1 an input of 3 volts remains at 3 volts before ADC measurement
                                        // ADC_2_5db provides an attenuation so that IN/OUT = 1 / 1.34 an input of 3 volts is reduced to 2.238 volts before ADC measurement
                                        // ADC_6db provides an attenuation so that IN/OUT = 1 / 2 an input of 3 volts is reduced to 1.500 volts before ADC measurement
                                        // ADC_11db provides an attenuation so that IN/OUT = 1 / 3.6 an input of 3 volts is reduced to 0.833 volts before ADC measurement
//   adcAttachPin(VP);                     // Attach a pin to ADC (also clears any other analog mode that could be on), returns TRUE/FALSE result 
//   adcStart(VP);                         // Starts an ADC conversion on attached pin's bus
//   adcBusy(VP);                          // Check if conversion on the pin's ADC bus is currently running, returns TRUE/FALSE result 
//   adcEnd(VP);
   
   adcAttachPin(36);
   adcAttachPin(37);

   //WiFi LoRa 32        -- hardare versrion ≥ 2.3
   //WiFi Kit 32         -- hardare versrion ≥ 2
   //Wireless Stick      -- hardare versrion ≥ 2.3
   //Wireless Stick Lite -- hardare versrion ≥ 2.3
   //Battery voltage read pin changed from GPIO13 to GPI37

   //measure battery voltage on pin 37
   adcStart(37);
   while(adcBusy(37));
   uint16_t c  =  analogRead(37)*XS*MUL;
   adcEnd(37);

   //publish battery voltage
   Serial.println(analogRead(37));
   snprintf (msg, MSG_BUFFER_SIZE, "%s",(String)c);
   Serial.print("Publish message Vbat: ");
   Serial.println(msg);

   strcpy(deviceTopic,"");
   strcat(deviceTopic,"fleet/");
   strcat(deviceTopic,deviceType);
   strcat(deviceTopic,"/"); 
   strcat(deviceTopic,deviceId);
   strcat(deviceTopic,"/Vbat");
   
   mqtt_client.publish(deviceTopic, msg);

   delay(100);

   //measure ????? voltage on pin 36
   adcStart(36);
   while(adcBusy(36));

   //publish ????? voltage

   uint16_t c2  =  analogRead(36)*0.769 + 150;
   adcEnd(36);
   Serial.printf("voltage input on GPIO 36: ");
   Serial.println(analogRead(36));
   snprintf (msg, MSG_BUFFER_SIZE, "%s",(String)c2);
   strcpy(deviceTopic,"");
   strcat(deviceTopic,"fleet/");
   strcat(deviceTopic,deviceType);
   strcat(deviceTopic,"/"); 
   strcat(deviceTopic,deviceId);
   strcat(deviceTopic,"/Vin"); 
   
   mqtt_client.publish(deviceTopic, msg);
   Serial.println("-------------");


   Heltec.display->drawString(0, 0, "Remaining battery still has:");
   Heltec.display->drawString(0, 10, "Vbat =");
   Heltec.display->drawString(35, 10, (String)c);
   Heltec.display->drawString(60, 10, "(mV)");
   
   Heltec.display->drawString(0, 20, "Vin   = ");
   Heltec.display->drawString(33, 20, (String)c2);
   Heltec.display->drawString(60, 20, "(mV)");

   Heltec.display->display();
   }
*/

/*
  void init_Heltec_display(){
  //Absence of Heltec.begin affects battery voltage readings 
  Heltec.begin(true, //DisplayEnable Enable, 
               false,//LoRa Enable
               false //Serial Enable);

  Heltec.display->init();
  Heltec.display->flipScreenVertically();
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->drawString(0, 0, "WAKE UP");
  Heltec.display->display();
  delay(1000);
  Heltec.display->clear();
  }  
*/


void setup(){

  wake_up_from_deep_sleep();



  //Heltec board only
  //init_Heltec_display();


  if (wakeupCount == 1) {    
  Serial.println("setting up WiFi");
  prepare_MQTT();
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  prepare_and_connect_wifi();
  connectStart = millis();
  }
  else {
    Serial.println("alternative WiFi connect");
    prepare_MQTT();
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    prepare_and_connect_wifi();
    connectStart = millis();
    }

  // DO SOMETHING USEFUL in setup
  Serial.println("something useful in setup()");
//  measure_Heltec_voltages();  //Heltec board only
  Serial.println("wait for 10 seconds to establish connections and communicate via MQTT");
  //delay for receiving commands over MQTT etc
  delay(10000);

  if (updatefirmware) {

/*
  //create a timer here
  //allow the user to answer, if he does not answer within ... seconds this is interpreted as proceed = false  
  //if sleep_mode is off, the webserver is started
  
  proceed = false;
  //request for confirmation to proceed
  //subscribe to channel for confirmation
  //create topic
   strcpy(deviceTopic,"");
   strcat(deviceTopic,"fleet/command/confirmation/");
   strcat(deviceTopic,deviceId);
   uint16_t packetIdSubc = mqttClient.subscribe(deviceTopic, 1);

   Serial.print("Subscribing at QoS 1,Topic: ");Serial.print(deviceTopic); Serial.print(", packetId: ");
   Serial.println(packetIdSubc);
   //publish request for confirmation
   //create topic
 strcpy(deviceTopic,"");
 strcat(deviceTopic,"fleet/");
 strcat(deviceTopic,deviceId);
 strcat(deviceTopic,"/reqconfirmation"); 

uint16_t packetIdPubX = mqttClient.publish(deviceTopic, 1, false, "ARE YOU SURE YOU WANT TO UPDATE?");  
Serial.printf("Publishing on topic %s at QoS 1, packetId: %i", deviceTopic, packetIdPubX);
*/
/*
  Serial.println("Waiting for confirmation");
  while (!proceed ){} //wait until proceed set to true  //and timer not expired 
  Serial.println("Waiting for confirmation");
*/ 
  proceed = true;
  // this code is executed when proceed == true OR timer expired
  if (proceed) {
  
  Serial.println("Starting update trials");
  int OTAcounter = 0;
  bool OTAsuccess = true;
  //updatefirmware = false;   //later do this after succesful update
  preferences.begin("my-app", false);
  preferences.putBool("updatefirmware",updatefirmware); 
  preferences.putBool("OTAsuccess",OTAsuccess);
  preferences.end();

  while (OTAcounter < MAXOTATRIALS) {
    OTAcounter += 1;  
    preferences.begin("my-app", false);
    preferences.putUInt("OTAcounter",OTAcounter);
    preferences.end();
     
    Serial.print("Going to call exec_httpUpdate with updatehost: ");Serial.print(updatehost);Serial.print(" updatepath: ");Serial.println(updatepath);
    //exec_httpUpdate(String(updatehost), String(updatepath));      //OK
    //Serial.print("Going to call exec_httpUpdate with updatehost: ");Serial.print(updatehostStr);Serial.print(" updatepath: ");Serial.println(updatepathStr);
    exec_httpUpdate(updatehostStr, updatepathStr);      //OK
    //exec_httpUpdate("192.168.2.200", "/ESP32_firmware/B.bin");  //OK
    Serial.print("Firmware update not successfull after "); Serial.print(OTAcounter);Serial.println(" trials");
    delay(5000);
  } //while
  
  Serial.println("OTA trials exceeded maximum allowed");
  OTAsuccess = false;
  preferences.begin("my-app", false);
  preferences.putBool("OTAsuccess",OTAsuccess);
  preferences.end();

  //communicate this to the dashboard
  //<code here>
  


} else //timer expired or cancelled

{  // send a warning that timer has expired
   // go to sleep
   Serial.println("void");
  }
}
 if (lowpowermode) {prepare_and_go_for_sleep(); }
  } //setup  


void loop(){
  //This is not going to be called when going to sleep in setup()
  // DO SOMETHING USEFUL in loop
    Serial.println("something useful in loop()");
    //exec_http_Update(int *client);
    
   // exec_httpUpdate();
 
    //if (lowpowermode) {prepare_and_go_for_sleep();}
    

}
