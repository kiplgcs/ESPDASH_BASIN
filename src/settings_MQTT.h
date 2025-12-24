#pragma once // защита от повторного включения файла

#include <Arduino.h> // базовые функции Arduino
#include <WiFi.h> // работа с Wi-Fi
#include <SPIFFS.h> // файловая система SPIFFS
#include <PubSubClient.h> // MQTT клиент
#include <ArduinoJson.h> // работа с JSON

#include "wifi_manager.h" // менеджер Wi-Fi
#include "fs_utils.h" // утилиты файловой системы

inline WiFiClient mqttWifiClient; // WiFi-клиент для MQTT
inline PubSubClient mqttClient(mqttWifiClient); // MQTT-клиент поверх WiFi

inline const char* mqttConfigPath = "/mqtt.json"; // путь к файлу настроек MQTT
inline String mqttHost = "192.168.0.100"; //"broker.emqx.io"; // адрес брокера
inline uint16_t mqttPort = 1883; // порт брокера
inline String mqttUsername = ""; // имя пользователя MQTT
inline String mqttPassword = ""; // пароль MQTT
inline bool mqttEnabled = false; // флаг включения MQTT
inline bool mqttIsConnected = false; // флаг подключения MQTT
inline unsigned long mqttPublishInterval = 10000; // интервал публикации
inline unsigned long mqttLastPublish = 0; // время последней публикации

inline void persistMqttSettings(){ // сохранение настроек MQTT
  if(!spiffsMounted) return; // выход если SPIFFS не смонтирован
  StaticJsonDocument<256> doc; // JSON-документ
  doc["host"] = mqttHost; // адрес брокера
  doc["port"] = mqttPort; // порт брокера
  doc["user"] = mqttUsername; // пользователь
  doc["pass"] = mqttPassword; // пароль
  doc["interval"] = mqttPublishInterval; // интервал
  doc["enabled"] = mqttEnabled; // флаг включения

  File file = SPIFFS.open(mqttConfigPath, FILE_WRITE); // открытие файла
  if(!file) return; // выход если файл не открыт
  serializeJson(doc, file); // запись JSON в файл
  file.close(); // закрытие файла
}

inline void loadMqttSettings(){ // загрузка настроек MQTT
  if(spiffsMounted && SPIFFS.exists(mqttConfigPath)){ // если файл существует
    File file = SPIFFS.open(mqttConfigPath, FILE_READ); // открытие файла
    if(file){ // если файл открыт
      StaticJsonDocument<256> doc; // JSON-документ
      DeserializationError err = deserializeJson(doc, file); // парсинг JSON
      if(!err){ // если без ошибок
        mqttHost = doc["host"] | mqttHost; // адрес брокера
        mqttPort = static_cast<uint16_t>(doc["port"] | mqttPort); // порт
        mqttUsername = doc["user"] | mqttUsername; // пользователь
        mqttPassword = doc["pass"] | mqttPassword; // пароль
        mqttPublishInterval = doc["interval"] | mqttPublishInterval; // интервал
        mqttEnabled = doc["enabled"] | mqttEnabled; // флаг
      }
      file.close(); // закрытие файла
    }
  } else { // если файл отсутствует
    // fallback to legacy preferences if present
    mqttHost = loadValue<String>("mqttHost", mqttHost); // старый host
    mqttPort = static_cast<uint16_t>(loadValue<int>("mqttPort", mqttPort)); // старый порт
    mqttUsername = loadValue<String>("mqttUser", mqttUsername); // старый пользователь
    mqttPassword = loadValue<String>("mqttPass", mqttPassword); // старый пароль
    mqttEnabled = loadValue<int>("mqttEnabled", mqttEnabled ? 1 : 0) == 1; // старый флаг
    persistMqttSettings(); // сохранение в новом формате
  }
}

inline void saveMqttSettings(){ // сохранение настроек MQTT
  persistMqttSettings(); // запись в SPIFFS
}

inline void configureMqttServer(){ // настройка сервера MQTT
  mqttClient.setServer(mqttHost.c_str(), mqttPort); // установка host и port
}

inline void connectMqtt(){ // подключение к MQTT
  if(!mqttEnabled) return; // выход если MQTT выключен
  if(mqttHost.length() == 0) return; // выход если host пустой
  if(WiFi.status() != WL_CONNECTED) return; // выход если нет Wi-Fi

  if(!mqttClient.connected()){ // если не подключены
    String clientId = "espdash-" + WiFi.macAddress(); // уникальный clientId
    bool connected = false; // флаг подключения
    if(mqttUsername.length()) connected = mqttClient.connect(clientId.c_str(), mqttUsername.c_str(), mqttPassword.c_str()); // подключение с логином
    else connected = mqttClient.connect(clientId.c_str()); // подключение без логина

    if(connected){ // если подключение успешно
      mqttIsConnected = true; // обновление флага
      mqttClient.subscribe("home/esp32/tempSet", 0); // подписка на топик
    } else { // если не удалось подключиться
      mqttIsConnected = false; // сброс флага
    }
  }
}

inline void stopMqttService(){ // остановка MQTT
  mqttClient.disconnect(); // отключение от брокера
  mqttIsConnected = false; // сброс флага подключения
  mqttLastPublish = 0; // сброс таймера публикаций
}

inline void applyMqttState(){ // применение состояния MQTT
  stopMqttService(); // остановка текущего подключения
  configureMqttServer(); // настройка сервера
  if(mqttEnabled){ // если MQTT включен
    mqttLastPublish = 0; // сброс таймера
    connectMqtt(); // подключение
  }
}

inline void handleMqttLoop(){ // основной цикл MQTT
  if(!mqttClient.connected()) connectMqtt(); // переподключение
  mqttIsConnected = mqttClient.connected(); // обновление флага

  if(!mqttEnabled || mqttHost.length() == 0){ // если MQTT выключен или host пустой
    if(mqttClient.connected()) stopMqttService(); // отключение
    return; // выход
  }

  mqttClient.loop(); // обработка MQTT

  unsigned long now = millis(); // текущее время
  if(now - mqttLastPublish >= mqttPublishInterval){ // проверка интервала
    mqttLastPublish = now; // обновление времени
    if(mqttClient.connected()){ // если подключены
      String payload = String("ESP32 uptime: ") + String(now / 1000) + "s"; // сообщение
      // mqttClient.publish("home/esp32/status", 0, false, payload.c_str());
      mqttClient.publish("home/esp32/status", payload.c_str(), true); // публикация
    }
  }
}
