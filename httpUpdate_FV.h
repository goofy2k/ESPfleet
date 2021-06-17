/**
//derived from: 

   httpUpdate.ino

    Created on: 27.11.2015

*/

#include <HTTPUpdate.h>
#include "my_secrets.h"

void dummy_httpUpdate(const String& host, const String& path){
}


void exec_httpUpdate(const String& host, const String& path){


    //String host = "192.168.2.200";
    //String path = "ESP32_firmware/BP_LORA32_AsyncMQTT_V2.5.ino.esp32.bin";

    
    WiFiClient client;

    // The line below is optional. It can be used to blink the LED on the board during flashing
    // The LED will be on during download of one buffer of data from the network. The LED will
    // be off during writing that buffer to flash
    // On a good connection the LED should flash regularly. On a bad connection the LED will be
    // on much longer than it will be off. Other pins than LED_BUILTIN may be used. The second
    // value is used to put the LED on. If the LED is on with HIGH, that value should be passed
    // httpUpdate.setLedPin(LED_BUILTIN, LOW);
    /*
     char updateurl[50] = "";
     strcpy(updateurl,"");
     strcat(updateurl,"http://");
     strcat(updateurl,host.c_str());
     strcat(updateurl,"/");
     strcat(updateurl,path.c_str());
     */

      
     
    //Serial.print("STARTING UPDATE ");Serial.print(" updatehost: ");Serial.print(SYNO_HOST);Serial.print(" updatepath: ");Serial.println(UPDATE_PATH); 
    Serial.print("STARTING UPDATE ");Serial.print(" updatehost: ");Serial.print(host);Serial.print(" updatepath: ");Serial.println(path); 
    //Serial.print("STARTING UPDATE ");Serial.print(" updateurl: ");Serial.print(updateurl);
      
      //>>>>>>>>  String myString = String(null_terminated_string); <<<<<<<<
    
      //Serial.print("STARTING DOWNLOAD OF FIRMWARE UPDATE with FIXED URL: ");Serial.print("http://192.168.2.200/ESP32_firmware/B.bin");
      Serial.println("---  DO NOT INTERRUPT --- (or do so to test if it is harmful......)");
      //t_httpUpdate_return ret = httpUpdate.update(client,"http://192.168.2.200/ESP32_firmware/B.bin");   //OK
      //t_httpUpdate_return ret = httpUpdate.update(client, "192.168.2.200", 80, "/ESP32_firmware/B.bin"); //OK
      //t_httpUpdate_return ret = httpUpdate.update(client, "SYNO_HOST", 80, "/UPDATE_PATH"); //NOK with #define UPDATE_PATH "ESP32_firmware/B.bin"
                                                                                              // and #define SYNO_HOST "192.168.2.200"

     t_httpUpdate_return ret = httpUpdate.update(client, host, 80, path); //with 192.168.2.200  /ESP32_firmware/B.bin as String //OK


    //https://drive.google.com/file/d/0B6vadbGV25HATWF0TUJZSmxJSTg/view?usp=sharing
    //t_httpUpdate_return ret = httpUpdate.update(client, "https://drive.google.com/file/d/1yoO17u8L-qmQd93lPiEJg2Hy27Fe77wG/view?usp=sharing");
 


    //SET OTAsucces and OTAcounter in Preferences
    // <code here>

  
    //t_httpUpdate_return ret = httpUpdate.update(client, "SYNO_HOST", 80, "/UPDATE_PATH");
    //t_httpUpdate_return ret = httpUpdate.update(client,"http://SYNO_HOST/UPDATE_PATH");

    //RESET 
    //t_httpUpdate_return ret = httpUpdate.update(client,updateurl);
    // t_httpUpdate_return ret = httpUpdate.update(client, host, 80, path);


    //Serial.println("THIS CODE IS NEVER REACHED unless update failed"); //unless update failed?
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;

      default:
        Serial.print("result code: "); Serial.println(ret);
        break;    
    }


    Serial.println("THIS CODE IS NEVER REACHED 2");
   
  
  };//httpUpdate
