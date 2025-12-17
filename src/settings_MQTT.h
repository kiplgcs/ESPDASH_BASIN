#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#include "wifi_manager.h"
#include "fs_utils.h"

inline WiFiClient mqttWifiClient;
inline PubSubClient mqttClient(mqttWifiClient);

inline const char* mqttConfigPath = "/mqtt.json";
inline String mqttHost = "broker.emqx.io";
inline uint16_t mqttPort = 1883;
inline String mqttUsername = "";
inline String mqttPassword = "";
inline bool mqttEnabled = false;
inline unsigned long mqttPublishInterval = 10000;
inline unsigned long mqttLastPublish = 0;

inline void persistMqttSettings(){
  if(!spiffsMounted) return;
  StaticJsonDocument<256> doc;
  doc["host"] = mqttHost;
  doc["port"] = mqttPort;
  doc["user"] = mqttUsername;
  doc["pass"] = mqttPassword;
  doc["interval"] = mqttPublishInterval;
  doc["enabled"] = mqttEnabled;

  File file = SPIFFS.open(mqttConfigPath, FILE_WRITE);
  if(!file) return;
  serializeJson(doc, file);
  file.close();
}

inline void loadMqttSettings(){
  if(spiffsMounted && SPIFFS.exists(mqttConfigPath)){
    File file = SPIFFS.open(mqttConfigPath, FILE_READ);
    if(file){
      StaticJsonDocument<256> doc;
      DeserializationError err = deserializeJson(doc, file);
      if(!err){
        mqttHost = doc["host"] | mqttHost;
        mqttPort = static_cast<uint16_t>(doc["port"] | mqttPort);
        mqttUsername = doc["user"] | mqttUsername;
        mqttPassword = doc["pass"] | mqttPassword;
        mqttPublishInterval = doc["interval"] | mqttPublishInterval;
        mqttEnabled = doc["enabled"] | mqttEnabled;
      }
      file.close();
    }
  } else {
    // fallback to legacy preferences if present
    mqttHost = loadValue<String>("mqttHost", mqttHost);
    mqttPort = static_cast<uint16_t>(loadValue<int>("mqttPort", mqttPort));
    mqttUsername = loadValue<String>("mqttUser", mqttUsername);
    mqttPassword = loadValue<String>("mqttPass", mqttPassword);
    mqttEnabled = loadValue<int>("mqttEnabled", mqttEnabled ? 1 : 0) == 1;
    persistMqttSettings();
  }
}

inline void saveMqttSettings(){
  persistMqttSettings();
}

inline void configureMqttServer(){
  mqttClient.setServer(mqttHost.c_str(), mqttPort);
}

inline void connectMqtt(){
  if(!mqttEnabled) return;
  if(WiFi.status() != WL_CONNECTED) return;

  if(!mqttClient.connected()){
    String clientId = "espdash-" + WiFi.macAddress();
    bool connected = false;
    if(mqttUsername.length()) connected = mqttClient.connect(clientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str());
    else connected = mqttClient.connect(clientId.c_str());


    if(connected){
      mqttClient.subscribe("home/esp32/tempSet", 0);
    }
  }
}

inline void stopMqttService(){
  mqttClient.disconnect();
  mqttLastPublish = 0;
}

inline void applyMqttState(){
  stopMqttService();
  configureMqttServer();
  if(mqttEnabled){
    mqttLastPublish = 0;
    connectMqtt();
  }
}


inline void handleMqttLoop(){
  if(!mqttClient.connected()) connectMqtt();

  if(!mqttEnabled){
    if(mqttClient.connected()) stopMqttService();
    return;
  }

  mqttClient.loop();

  unsigned long now = millis();
  if(now - mqttLastPublish >= mqttPublishInterval){
    mqttLastPublish = now;
    if(mqttClient.connected()){
      String payload = String("ESP32 uptime: ") + String(now / 1000) + "s";
      mqttClient.publish("home/esp32/status", 0, false, payload.c_str());
    }
  }
}
