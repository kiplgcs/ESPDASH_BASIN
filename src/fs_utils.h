//fs_utils.h - Работа с памятью SPIFFS
#pragma once

#include <Arduino.h>
#include <SPIFFS.h>

inline bool spiffsMounted = false;

inline void initFileSystem(){
  if(SPIFFS.begin(true)){
    spiffsMounted = true;
    Serial.println("SPIFFS mounted OK");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while(file){
      Serial.print("SPIFFS file: ");
      Serial.print(file.name());
      Serial.print(" size: ");
      Serial.println(file.size());
      file = root.openNextFile();
    }
  } else {
    Serial.println("SPIFFS mount failed!");
  }
}
