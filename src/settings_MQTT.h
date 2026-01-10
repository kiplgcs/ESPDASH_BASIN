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
inline bool mqttEnabled = true; // флаг включения MQTT
inline bool mqttIsConnected = false; // флаг подключения MQTT
inline unsigned long mqttPublishInterval = 10000; // интервал публикации
inline unsigned long mqttLastPublish = 0; // время последней публикации
inline const char* mqttAvailabilityTopic = "home/esp32/availability"; // топик доступности устройства
inline const char* mqttDiscoveryPrefix = "homeassistant"; // префикс MQTT Discovery
inline bool mqttDiscoveryPending = false; // публикация после первого успешного MQTT loop
// mqttDiscovery публикуется один раз после успешного подключения и availability=online.

extern float DS1; // температура воды
extern float PH; // pH воды
extern float ppmCl; // уровень хлора
extern int corrected_ORP_Eh_mV; // ORP, мВ
extern String OverlayFilterState; // состояние фильтрации (строка)
extern bool Power_H2O2; // состояние насоса NaOCl
extern bool Power_ACO; // состояние насоса ACO
extern bool Power_Heat; // состояние нагрева
extern bool Power_Topping_State; // состояние соленоида долива
extern bool Power_Filtr; // ручная фильтрация
extern bool Pow_Ul_light; // ручное освещение
extern bool Activation_Heat; // управление нагревом
extern String SetLamp; // режим лампы
extern String SetRGB; // режим RGB

inline String mqttDiscoveryDeviceId(){ // идентификатор устройства для Discovery
  String id = WiFi.macAddress(); // MAC-адрес
  id.replace(":", ""); // удаление двоеточий
  id.toLowerCase(); // приведение к нижнему регистру
  return "espdash_" + id; // итоговый id
}

inline String mqttDiscoveryDeviceName(){ // имя устройства для Discovery
  return "ESP32 " + WiFi.macAddress(); // имя устройства
}

inline void publishMqttAvailability(const char* payload, bool retain = true){ // публикация доступности
  if(!mqttClient.connected()) return; // выход если нет подключения
  mqttClient.publish(mqttAvailabilityTopic, payload, retain); // публикация статуса
}

inline void publishHomeAssistantDiscovery(){ // публикация MQTT Discovery
  if(!mqttClient.connected()) return; // выход если нет подключения

  const String deviceId = mqttDiscoveryDeviceId(); // id устройства
  const String deviceName = mqttDiscoveryDeviceName(); // имя устройства

  bool publishedAll = true; // флаг успешной публикации всех сущностей

  auto publishEntity = [&](const char* component,
                           const char* id,
                           const char* name,
                           const char* stateTopic,
                           const char* commandTopic,
                           const char* deviceClass,
                           const char* unit,
                           const char* stateClass,
                           const char* valueTemplate,
                           const char* payloadOn,
                           const char* payloadOff) -> bool {
    StaticJsonDocument<256> doc; // JSON-документ
    const String uniqueId = deviceId + "_" + id; // уникальный id
    const String topic = String(mqttDiscoveryPrefix) + "/" + component + "/" + uniqueId + "/config"; // топик config

    doc["unique_id"] = uniqueId; // unique_id
    doc["name"] = name; // name
    doc["availability_topic"] = mqttAvailabilityTopic; // availability_topic
    doc["payload_available"] = "online"; // payload available
    doc["payload_not_available"] = "offline"; // payload not available
    if(stateTopic) doc["state_topic"] = stateTopic; // state_topic
    if(commandTopic) doc["command_topic"] = commandTopic; // command_topic
    if(deviceClass) doc["device_class"] = deviceClass; // device_class
    if(unit) doc["unit_of_measurement"] = unit; // unit_of_measurement
    if(stateClass) doc["state_class"] = stateClass; // state_class
    if(valueTemplate) doc["value_template"] = valueTemplate; // value_template
    if(payloadOn) doc["payload_on"] = payloadOn; // payload_on
    if(payloadOff) doc["payload_off"] = payloadOff; // payload_off

    JsonObject device = doc.createNestedObject("device"); // объект устройства
    JsonArray identifiers = device.createNestedArray("identifiers"); // идентификаторы
    identifiers.add(deviceId); // добавление id
    device["name"] = deviceName; // имя устройства
    device["model"] = "ESP32-S3"; // модель
    device["manufacturer"] = "Espressif"; // производитель

    String payload; // строка JSON
    serializeJson(doc, payload); // сериализация JSON
    return mqttClient.publish(topic.c_str(), payload.c_str(), true); // публикация config с retain
  };

  publishedAll = publishEntity(
    "sensor",
    "status",
    "ESP32 Uptime",
    "home/esp32/status",
    nullptr,
    "duration",
    "s",
    "measurement",
    "{{ value | replace('ESP32 uptime: ', '') | replace('s','') }}",
    nullptr,
    nullptr
  ) && publishedAll;

  publishedAll = publishEntity("sensor", "DS1", "Pool Water Temperature", "home/esp32/DS1", nullptr, "temperature", "°C", "measurement", nullptr, nullptr, nullptr) && publishedAll;
  publishedAll = publishEntity("sensor", "RoomTemp", "Room Temperature", "home/esp32/RoomTemp", nullptr, "temperature", "°C", "measurement", nullptr, nullptr, nullptr) && publishedAll;
  publishedAll = publishEntity("sensor", "PH", "Pool pH", "home/esp32/PH", nullptr, nullptr, "pH", "measurement", nullptr, nullptr, nullptr) && publishedAll;
  publishedAll = publishEntity("sensor", "corrected_ORP_Eh_mV", "ORP", "home/esp32/corrected_ORP_Eh_mV", nullptr, nullptr, "mV", "measurement", nullptr, nullptr, nullptr) && publishedAll;
  publishedAll = publishEntity("sensor", "ppmCl", "Free Chlorine", "home/esp32/ppmCl", nullptr, nullptr, "mg/L", "measurement", nullptr, nullptr, nullptr) && publishedAll;
  publishedAll = publishEntity("sensor", "OverlayFilterState", "Filter State", "home/esp32/OverlayFilterState", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr) && publishedAll;

  publishedAll = publishEntity("binary_sensor", "Power_H2O2", "NaOCl Pump State", "home/esp32/Power_H2O2", nullptr, nullptr, nullptr, nullptr, nullptr, "1", "0") && publishedAll;
  publishedAll = publishEntity("binary_sensor", "Power_ACO", "ACO Pump State", "home/esp32/Power_ACO", nullptr, nullptr, nullptr, nullptr, nullptr, "1", "0") && publishedAll;
  publishedAll = publishEntity("binary_sensor", "Power_Heat", "Heating State", "home/esp32/Power_Heat", nullptr, nullptr, nullptr, nullptr, nullptr, "1", "0") && publishedAll;
  publishedAll = publishEntity("binary_sensor", "Power_Topping_State", "Water Top Up State", "home/esp32/Power_Topping_State", nullptr, nullptr, nullptr, nullptr, nullptr, "1", "0") && publishedAll;

  publishedAll = publishEntity("switch", "Power_Filtr", "Pool Filter (Manual)", "home/esp32/Power_Filtr", "home/esp32/Power_Filtr/set", nullptr, nullptr, nullptr, nullptr, "1", "0") && publishedAll;
  publishedAll = publishEntity("switch", "Pow_Ul_light", "Outdoor Light (Manual)", "home/esp32/Pow_Ul_light", "home/esp32/Pow_Ul_light/set", nullptr, nullptr, nullptr, nullptr, "1", "0") && publishedAll;
  publishedAll = publishEntity("switch", "Activation_Heat", "Heating Control", "home/esp32/Activation_Heat", "home/esp32/Activation_Heat/set", nullptr, nullptr, nullptr, nullptr, "1", "0") && publishedAll;

  {
    StaticJsonDocument<256> doc; // JSON-документ
    const String uniqueId = deviceId + "_SetLamp"; // уникальный id
    const String topic = String(mqttDiscoveryPrefix) + "/select/" + uniqueId + "/config"; // топик config
    doc["unique_id"] = uniqueId; // unique_id
    doc["name"] = "Lamp Mode"; // name
    doc["availability_topic"] = mqttAvailabilityTopic; // availability_topic
    doc["payload_available"] = "online"; // payload available
    doc["payload_not_available"] = "offline"; // payload not available
    doc["state_topic"] = "home/esp32/SetLamp"; // state_topic
    doc["command_topic"] = "home/esp32/SetLamp/set"; // command_topic
    JsonArray options = doc.createNestedArray("options"); // options
    options.add("off");
    options.add("on");
    options.add("auto");
    options.add("timer");
    JsonObject device = doc.createNestedObject("device"); // объект устройства
    JsonArray identifiers = device.createNestedArray("identifiers"); // идентификаторы
    identifiers.add(deviceId); // добавление id
    device["name"] = deviceName; // имя устройства
    device["model"] = "ESP32-S3"; // модель
    device["manufacturer"] = "Espressif"; // производитель
    String payload; // строка JSON
    serializeJson(doc, payload); // сериализация JSON
    publishedAll = mqttClient.publish(topic.c_str(), payload.c_str(), true) && publishedAll; // публикация config с retain
  }

  {
    StaticJsonDocument<256> doc; // JSON-документ
    const String uniqueId = deviceId + "_SetRGB"; // уникальный id
    const String topic = String(mqttDiscoveryPrefix) + "/select/" + uniqueId + "/config"; // топик config
    doc["unique_id"] = uniqueId; // unique_id
    doc["name"] = "RGB Mode"; // name
    doc["availability_topic"] = mqttAvailabilityTopic; // availability_topic
    doc["payload_available"] = "online"; // payload available
    doc["payload_not_available"] = "offline"; // payload not available
    doc["state_topic"] = "home/esp32/SetRGB"; // state_topic
    doc["command_topic"] = "home/esp32/SetRGB/set"; // command_topic
    JsonArray options = doc.createNestedArray("options"); // options
    options.add("off");
    options.add("on");
    options.add("auto");
    options.add("timer");
    JsonObject device = doc.createNestedObject("device"); // объект устройства
    JsonArray identifiers = device.createNestedArray("identifiers"); // идентификаторы
    identifiers.add(deviceId); // добавление id
    device["name"] = deviceName; // имя устройства
    device["model"] = "ESP32-S3"; // модель
    device["manufacturer"] = "Espressif"; // производитель
    String payload; // строка JSON
    serializeJson(doc, payload); // сериализация JSON
    publishedAll = mqttClient.publish(topic.c_str(), payload.c_str(), true) && publishedAll; // публикация config с retain
  }

  if(publishedAll){
    mqttDiscoveryPending = false; // сброс ожидания только при успехе публикации
  }
}

inline void publishMqttStateString(const char* topic, const String &value){ // публикация строкового состояния
  mqttClient.publish(topic, value.c_str()); // публикация значения (retain не используется)
}

inline void publishMqttStateBool(const char* topic, bool value){ // публикация bool состояния
  mqttClient.publish(topic, value ? "1" : "0"); // публикация 1/0
}

inline void publishMqttStateFloat(const char* topic, float value){ // публикация float состояния
  if(isnan(value)){
    mqttClient.publish(topic, "0"); // защита от NaN
    return;
  }
  String payload = String(value); // формирование строки
  mqttClient.publish(topic, payload.c_str()); // публикация значения
}

inline void publishMqttStateInt(const char* topic, int value){ // публикация int состояния
  String payload = String(value); // формирование строки
  mqttClient.publish(topic, payload.c_str()); // публикация значения
}


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
   String clientId = mqttDiscoveryDeviceId(); // уникальный clientId
    bool connected = false; // флаг подключения
   if(mqttUsername.length()){
      connected = mqttClient.connect( // подключение с логином и LWT
        clientId.c_str(),
        mqttUsername.c_str(),
        mqttPassword.c_str(),
        mqttAvailabilityTopic,
        0,
        true,
        "offline"
      );
    } else {
      connected = mqttClient.connect( // подключение без логина и LWT
        clientId.c_str(),
        mqttAvailabilityTopic,
        0,
        true,
        "offline"
      );
    }


    if(connected){ // если подключение успешно
      mqttIsConnected = true; // обновление флага
            publishMqttAvailability("online", true); // публикация доступности
      mqttDiscoveryPending = true; // публикация MQTT Discovery после первого loop

      mqttClient.subscribe("home/esp32/tempSet", 0); // подписка на топик
    } else { // если не удалось подключиться
      mqttIsConnected = false; // сброс флага
    }
  }
}

inline void stopMqttService(){ // остановка MQTT
    if(mqttClient.connected()) publishMqttAvailability("offline", true); // публикация offline
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
    if(mqttDiscoveryPending) publishHomeAssistantDiscovery(); // публикация после первого loop

  unsigned long now = millis(); // текущее время
  if(now - mqttLastPublish >= mqttPublishInterval){ // проверка интервала
    mqttLastPublish = now; // обновление времени
    if(mqttClient.connected()){ // если подключены
      String payload = String("ESP32 uptime: ") + String(now / 1000) + "s"; // сообщение
      // mqttClient.publish("home/esp32/status", 0, false, payload.c_str());
      mqttClient.publish("home/esp32/status", payload.c_str(), true); // публикация
    
      publishMqttStateFloat("home/esp32/DS1", DS1);
      publishMqttStateFloat("home/esp32/RoomTemp", DS1); // RoomTemp использует DS1 в UI
      publishMqttStateFloat("home/esp32/PH", PH);
      publishMqttStateInt("home/esp32/corrected_ORP_Eh_mV", corrected_ORP_Eh_mV);
      publishMqttStateFloat("home/esp32/ppmCl", ppmCl);
      publishMqttStateString("home/esp32/OverlayFilterState", OverlayFilterState);

      publishMqttStateBool("home/esp32/Power_H2O2", Power_H2O2);
      publishMqttStateBool("home/esp32/Power_ACO", Power_ACO);
      publishMqttStateBool("home/esp32/Power_Heat", Power_Heat);
      publishMqttStateBool("home/esp32/Power_Topping_State", Power_Topping_State);

      publishMqttStateBool("home/esp32/Power_Filtr", Power_Filtr);
      publishMqttStateBool("home/esp32/Pow_Ul_light", Pow_Ul_light);
      publishMqttStateBool("home/esp32/Activation_Heat", Activation_Heat);

      publishMqttStateString("home/esp32/SetLamp", SetLamp);
      publishMqttStateString("home/esp32/SetRGB", SetRGB);
    
    }
  }
}
