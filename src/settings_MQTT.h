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

#if 0 // MQTT Discovery отключен (слишком много кода и не нужен)
inline const char* mqttDiscoveryPrefix = "homeassistant"; // префикс MQTT Discovery
inline bool mqttDiscoveryPending = false; // публикация после первого успешного MQTT loop

inline unsigned long mqttDiscoveryLastAttempt = 0; // время последней попытки discovery
inline const unsigned long mqttDiscoveryInterval = 250; // интервал между пакетами discovery
inline const uint8_t mqttDiscoveryBatchSize = 4; // максимум сущностей за loop
#endif

inline void publishMqttAvailability(const char* payload, bool retain = true){ // публикация доступности
  if(!mqttClient.connected()) return; // выход если нет подключения
  mqttClient.publish(mqttAvailabilityTopic, payload, retain); // публикация статуса
}
inline unsigned long mqttLastConnectAttempt = 0; // время последней попытки подключения
inline const unsigned long mqttConnectInterval = 5000; // интервал попыток подключения
inline unsigned long mqttLastResolveAttempt = 0; // время последней попытки DNS
inline const unsigned long mqttResolveInterval = 30000; // интервал резолва хоста
inline bool mqttHasResolvedIp = false; // флаг успешного резолва
inline IPAddress mqttResolvedIp; // резолвенный IP адрес
inline String mqttResolvedHostName; // хост для которого выполнен резолв


#if 0 // MQTT Discovery отключен (слишком много кода и не нужен)
enum DiscoveryStage {
  DISCOVERY_NONE,
  DISCOVERY_TEST_SENSOR,
  DISCOVERY_MAIN_ENTITIES,
  DISCOVERY_DONE
};

inline DiscoveryStage mqttDiscoveryStage = DISCOVERY_NONE; // этап discovery
inline size_t mqttDiscoveryIndex = 0; // индекс публикации сущностей

// mqttDiscovery публикуется один раз после успешного подключения и availability=online.
#endif

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
extern bool Filtr_Time1; // таймер фильтрации №1
extern bool Filtr_Time2; // таймер фильтрации №2
extern bool Filtr_Time3; // таймер фильтрации №3
extern bool Power_Clean; // промывка фильтра
extern bool Clean_Time1; // таймер промывки
extern bool Pow_Ul_light; // ручное освещение
extern bool Activation_Heat; // управление нагревом
extern String SetLamp; // режим лампы
extern String SetRGB; // режим RGB
extern String DaysSelect; // выбранные дни промывки
class UIRegistry; // forward declaration
extern UIRegistry ui; // доступ к UI-реестру таймеров
void syncCleanDaysFromSelection(); // синхронизация дней промывки

String formatMinutesToTime(uint16_t minutes); // форматирование минут в HH:MM
bool mqttApplyTimerField(const String &fieldId, const String &value); // применить значение таймера
uint16_t mqttTimerOnMinutes(const String &id); // время включения таймера
uint16_t mqttTimerOffMinutes(const String &id); // время выключения таймера
void mqttApplyDaysSelect(const String &value); // применить выбор дней промывки

// MQTT-помощники для таймеров/дней (перенесены сюда, чтобы MQTT-логика была в одном файле)
inline bool mqttApplyTimerField(const String &fieldId, const String &value){
  return ui.updateTimerField(fieldId, value);
}

inline uint16_t mqttTimerOnMinutes(const String &id){
  return ui.timer(id).on;
}

inline uint16_t mqttTimerOffMinutes(const String &id){
  return ui.timer(id).off;
}

inline void mqttApplyDaysSelect(const String &value){
  DaysSelect = value;
  syncCleanDaysFromSelection();
  saveValue<String>("DaysSelect", DaysSelect);
}


inline void publishMqttStateString(const char* topic, const String &value); // forward declaration
inline void publishMqttStateBool(const char* topic, bool value); // forward declaration

inline bool mqttPayloadIsOn(String payload){ // проверка включения
  payload.trim(); // очистка
  payload.toLowerCase(); // нижний регистр
  return payload == "1" || payload == "on" || payload == "true"; // ON
}

inline bool mqttPayloadIsOff(String payload){ // проверка выключения
  payload.trim(); // очистка
  payload.toLowerCase(); // нижний регистр
  return payload == "0" || payload == "off" || payload == "false"; // OFF
}

inline bool mqttIsAllowedMode(const String &value){ // проверка режима
  return value == "off" || value == "on" || value == "auto" || value == "timer"; // допустимые режимы
}

inline void handleMqttCommandMessage(char* topic, byte* payload, unsigned int length){ // обработка MQTT команд
  String topicStr(topic); // топик
  String message; // payload строкой
  message.reserve(length); // резерв
  for(unsigned int i = 0; i < length; ++i){
    message += static_cast<char>(payload[i]); // сбор payload
  }
  message.trim(); // очистка

  if(topicStr == "home/esp32/Power_Filtr/set"){
    Power_Filtr = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Power_Filtr", Power_Filtr ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Power_Filtr", Power_Filtr); // публикация
    return;
  }

  if(topicStr == "home/esp32/Filtr_Time1/set"){
    Filtr_Time1 = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Filtr_Time1", Filtr_Time1 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Filtr_Time1", Filtr_Time1); // публикация
    return;
  }

  if(topicStr == "home/esp32/Filtr_Time2/set"){
    Filtr_Time2 = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Filtr_Time2", Filtr_Time2 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Filtr_Time2", Filtr_Time2); // публикация
    return;
  }

  if(topicStr == "home/esp32/Filtr_Time3/set"){
    Filtr_Time3 = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Filtr_Time3", Filtr_Time3 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Filtr_Time3", Filtr_Time3); // публикация
    return;
  }

  if(topicStr == "home/esp32/Power_Clean/set"){
    Power_Clean = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Power_Clean", Power_Clean ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Power_Clean", Power_Clean); // публикация
    return;
  }

  if(topicStr == "home/esp32/Clean_Time1/set"){
    Clean_Time1 = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Clean_Time1", Clean_Time1 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Clean_Time1", Clean_Time1); // публикация
    return;
  }

  if(topicStr == "home/esp32/DaysSelect/set"){
    mqttApplyDaysSelect(message); // обновление строки дней
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect); // публикация
    return;
  }

  if(topicStr == "home/esp32/FiltrTimer1_ON/set"){
    if(mqttApplyTimerField("FiltrTimer1_ON", message)){
      publishMqttStateString("home/esp32/FiltrTimer1_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer1")));
    }
    return;
  }

  if(topicStr == "home/esp32/FiltrTimer1_OFF/set"){
    if(mqttApplyTimerField("FiltrTimer1_OFF", message)){
      publishMqttStateString("home/esp32/FiltrTimer1_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer1")));
    }
    return;
  }

  if(topicStr == "home/esp32/FiltrTimer2_ON/set"){
    if(mqttApplyTimerField("FiltrTimer2_ON", message)){
      publishMqttStateString("home/esp32/FiltrTimer2_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer2")));
    }
    return;
  }

  if(topicStr == "home/esp32/FiltrTimer2_OFF/set"){
    if(mqttApplyTimerField("FiltrTimer2_OFF", message)){
      publishMqttStateString("home/esp32/FiltrTimer2_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer2")));
    }
    return;
  }

  if(topicStr == "home/esp32/FiltrTimer3_ON/set"){
    if(mqttApplyTimerField("FiltrTimer3_ON", message)){
      publishMqttStateString("home/esp32/FiltrTimer3_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer3")));
    }
    return;
  }

  if(topicStr == "home/esp32/FiltrTimer3_OFF/set"){
    if(mqttApplyTimerField("FiltrTimer3_OFF", message)){
      publishMqttStateString("home/esp32/FiltrTimer3_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer3")));
    }
    return;
  }

  if(topicStr == "home/esp32/CleanTimer1_ON/set"){
    if(mqttApplyTimerField("CleanTimer1_ON", message)){
      publishMqttStateString("home/esp32/CleanTimer1_ON", formatMinutesToTime(mqttTimerOnMinutes("CleanTimer1")));
    }
    return;
  }

  if(topicStr == "home/esp32/CleanTimer1_OFF/set"){
    if(mqttApplyTimerField("CleanTimer1_OFF", message)){
      publishMqttStateString("home/esp32/CleanTimer1_OFF", formatMinutesToTime(mqttTimerOffMinutes("CleanTimer1")));
    }
    return;
  }


  if(topicStr == "home/esp32/Pow_Ul_light/set"){
    Pow_Ul_light = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Pow_Ul_light", Pow_Ul_light ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Pow_Ul_light", Pow_Ul_light); // публикация
    return;
  }

  if(topicStr == "home/esp32/Activation_Heat/set"){
    Activation_Heat = mqttPayloadIsOn(message); // обновление состояния
    saveValue<int>("Activation_Heat", Activation_Heat ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Activation_Heat", Activation_Heat); // публикация
    return;
  }

  if(topicStr == "home/esp32/SetLamp/set"){
    if(mqttIsAllowedMode(message)){
      SetLamp = message; // обновление режима
      saveValue<String>("SetLamp", SetLamp); // сохранение
      publishMqttStateString("home/esp32/SetLamp", SetLamp); // публикация
    }
    return;
  }

  if(topicStr == "home/esp32/SetRGB/set"){
    if(mqttIsAllowedMode(message)){
      SetRGB = message; // обновление режима
      saveValue<String>("SetRGB", SetRGB); // сохранение
      publishMqttStateString("home/esp32/SetRGB", SetRGB); // публикация
    }
    return;
  }

  if(topicStr == "home/esp32/button/restart/set"){
    ESP.restart(); // перезапуск
    return;
  }
}

#if 0 // MQTT Discovery отключен (слишком много кода и не нужен)
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

inline void publishDiscoveryDeviceBlock(JsonDocument &doc, const String &deviceId, const String &deviceName){ // блок device
  JsonObject device = doc["device"].to<JsonObject>(); // объект устройства
  JsonArray identifiers = device["identifiers"].to<JsonArray>(); // идентификаторы
  identifiers.add(deviceId); // добавление id
  device["name"] = deviceName; // имя устройства
  device["model"] = "ESP32-S3"; // модель
  device["manufacturer"] = "Espressif"; // производитель
}

struct MqttDiscoveryEntity {
  const char* component;
  const char* id;
  const char* name;
  const char* stateTopic;
  const char* commandTopic;
  const char* deviceClass;
  const char* unit;
  const char* stateClass;
  const char* valueTemplate;
  const char* payloadOn;
  const char* payloadOff;
};

inline bool publishMqttDiscoveryEntity(const MqttDiscoveryEntity &entity,
                                       const String &deviceId,
                                       const String &deviceName){ // публикация сущности
  JsonDocument doc; // JSON-документ
  const String uniqueId = deviceId + "_" + entity.id; // уникальный id
  const String topic = String(mqttDiscoveryPrefix) + "/" + entity.component + "/" + uniqueId + "/config"; // топик config

  doc["unique_id"] = uniqueId; // unique_id
  doc["name"] = entity.name; // name
  doc["availability_topic"] = mqttAvailabilityTopic; // availability_topic
  doc["payload_available"] = "online"; // payload available
  doc["payload_not_available"] = "offline"; // payload not available
  if(entity.stateTopic) doc["state_topic"] = entity.stateTopic; // state_topic
  if(entity.commandTopic) doc["command_topic"] = entity.commandTopic; // command_topic
  if(entity.deviceClass) doc["device_class"] = entity.deviceClass; // device_class
  if(entity.unit) doc["unit_of_measurement"] = entity.unit; // unit_of_measurement
  if(entity.stateClass) doc["state_class"] = entity.stateClass; // state_class
  if(entity.valueTemplate) doc["value_template"] = entity.valueTemplate; // value_template
  if(entity.payloadOn) doc["payload_on"] = entity.payloadOn; // payload_on
  if(entity.payloadOff) doc["payload_off"] = entity.payloadOff; // payload_off

  publishDiscoveryDeviceBlock(doc, deviceId, deviceName); // блок device

  String payload; // строка JSON
  serializeJson(doc, payload); // сериализация JSON
  return mqttClient.publish(topic.c_str(), payload.c_str(), true); // публикация config с retain
}

inline bool publishMqttDiscoverySelect(const char* id,
                                       const char* name,
                                       const char* stateTopic,
                                       const char* commandTopic,
                                       const char* const* options,
                                       size_t optionsCount,
                                       const String &deviceId,
                                       const String &deviceName){ // публикация select
  JsonDocument doc; // JSON-документ
  const String uniqueId = deviceId + "_" + id; // уникальный id
  const String topic = String(mqttDiscoveryPrefix) + "/select/" + uniqueId + "/config"; // топик config

  doc["unique_id"] = uniqueId; // unique_id
  doc["name"] = name; // name
  doc["availability_topic"] = mqttAvailabilityTopic; // availability_topic
  doc["payload_available"] = "online"; // payload available
  doc["payload_not_available"] = "offline"; // payload not available
  doc["state_topic"] = stateTopic; // state_topic
  doc["command_topic"] = commandTopic; // command_topic
  JsonArray optionsArray = doc["options"].to<JsonArray>(); // options
  for(size_t i = 0; i < optionsCount; ++i){
    optionsArray.add(options[i]);
  }

  publishDiscoveryDeviceBlock(doc, deviceId, deviceName); // блок device

  String payload; // строка JSON
  serializeJson(doc, payload); // сериализация JSON
  return mqttClient.publish(topic.c_str(), payload.c_str(), true); // публикация config с retain
}

inline void publishHomeAssistantDiscovery(){ // публикация MQTT Discovery
  if(!mqttClient.connected()) return; // выход если нет подключения
  if(mqttDiscoveryStage == DISCOVERY_DONE){ // завершено
    mqttDiscoveryPending = false; // сброс ожидания
    return;
  }

  const unsigned long now = millis(); // текущее время
  if(now - mqttDiscoveryLastAttempt < mqttDiscoveryInterval) return; // интервал между пакетами
  mqttDiscoveryLastAttempt = now; // обновление таймера

  const String deviceId = mqttDiscoveryDeviceId(); // id устройства
  const String deviceName = mqttDiscoveryDeviceName(); // имя устройства

static const MqttDiscoveryEntity baseEntities[] = {
    {"sensor", "status", "ESP32 Uptime", "home/esp32/status", nullptr, "duration", "s", "measurement", "{{ value | replace('ESP32 uptime: ', '') | replace('s','') }}", nullptr, nullptr},
    {"sensor", "test", "ESP32 Test Sensor", "home/esp32/test", nullptr, nullptr, nullptr, "measurement", nullptr, nullptr, nullptr},
    {"sensor", "DS1", "Pool Water Temperature", "home/esp32/DS1", nullptr, "temperature", "°C", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "RoomTemp", "Room Temperature", "home/esp32/RoomTemp", nullptr, "temperature", "°C", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "PH", "Pool pH", "home/esp32/PH", nullptr, nullptr, "pH", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "corrected_ORP_Eh_mV", "ORP", "home/esp32/corrected_ORP_Eh_mV", nullptr, nullptr, "mV", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "ppmCl", "Free Chlorine", "home/esp32/ppmCl", nullptr, nullptr, "mg/L", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "OverlayFilterState", "Filter State", "home/esp32/OverlayFilterState", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"binary_sensor", "Power_H2O2", "NaOCl Pump State", "home/esp32/Power_H2O2", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_ACO", "ACO Pump State", "home/esp32/Power_ACO", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_Heat", "Heating State", "home/esp32/Power_Heat", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_Topping_State", "Water Top Up State", "home/esp32/Power_Topping_State", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Power_Filtr", "Pool Filter (Manual)", "home/esp32/Power_Filtr", "home/esp32/Power_Filtr/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Pow_Ul_light", "Outdoor Light (Manual)", "home/esp32/Pow_Ul_light", "home/esp32/Pow_Ul_light/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Activation_Heat", "Heating Control", "home/esp32/Activation_Heat", "home/esp32/Activation_Heat/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"button", "restart", "ESP32 Restart", nullptr, "home/esp32/button/restart/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}
  };

  static const char* const selectOptions[] = {"off", "on", "auto", "timer"}; // варианты для select
  const size_t baseCount = sizeof(baseEntities) / sizeof(baseEntities[0]); // количество base сущностей
  const size_t totalCount = baseCount + 2; // +2 select

  if(mqttDiscoveryStage == DISCOVERY_NONE){
    mqttDiscoveryStage = DISCOVERY_TEST_SENSOR; // старт этапа
  }

if(mqttDiscoveryStage == DISCOVERY_TEST_SENSOR){
    const MqttDiscoveryEntity testSensor = {"sensor", "alive", "ESP32 Alive", "home/esp32/alive", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    if(publishMqttDiscoveryEntity(testSensor, deviceId, deviceName)){
      mqttClient.publish("home/esp32/alive", "1", true); // якорный сенсор
      mqttDiscoveryStage = DISCOVERY_MAIN_ENTITIES; // переход к основным сущностям
      mqttDiscoveryIndex = 0; // сброс индекса
    }
    return;
  }

if(mqttDiscoveryStage == DISCOVERY_MAIN_ENTITIES){
    uint8_t publishedCount = 0; // опубликовано в этом loop
    while(publishedCount < mqttDiscoveryBatchSize && mqttDiscoveryIndex < totalCount){
      bool published = false;
      if(mqttDiscoveryIndex < baseCount){
        published = publishMqttDiscoveryEntity(baseEntities[mqttDiscoveryIndex], deviceId, deviceName);
      } else if(mqttDiscoveryIndex == baseCount){
        published = publishMqttDiscoverySelect("SetLamp", "Lamp Mode", "home/esp32/SetLamp", "home/esp32/SetLamp/set", selectOptions, 4, deviceId, deviceName);
      } else if(mqttDiscoveryIndex == baseCount + 1){
        published = publishMqttDiscoverySelect("SetRGB", "RGB Mode", "home/esp32/SetRGB", "home/esp32/SetRGB/set", selectOptions, 4, deviceId, deviceName);
      }

      if(!published){
        break; // повторим в следующем loop
      }

      mqttDiscoveryIndex++;
      publishedCount++;
    }

    if(mqttDiscoveryIndex >= totalCount){
      mqttDiscoveryStage = DISCOVERY_DONE; // завершено
      mqttDiscoveryPending = false; // сброс ожидания
    }
  }
}
#endif

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
  JsonDocument doc; // JSON-документ
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
      JsonDocument doc; // JSON-документ
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
 if(mqttResolvedHostName != mqttHost){
    mqttResolvedHostName = mqttHost; // обновление хоста
    mqttHasResolvedIp = false; // сброс IP
    mqttLastResolveAttempt = 0; // сброс таймера резолва
  }
  if(!mqttHasResolvedIp){
    IPAddress ip;
    if(ip.fromString(mqttHost)){
      mqttResolvedIp = ip; // фиксируем IP
      mqttHasResolvedIp = true;
    }
  }
  if(mqttHasResolvedIp){
    mqttClient.setServer(mqttResolvedIp, mqttPort); // установка IP и port
  } else {
    mqttClient.setServer(mqttHost.c_str(), mqttPort); // установка host и port
  }
  mqttClient.setCallback(handleMqttCommandMessage); // обработчик входящих команд
mqttClient.setSocketTimeout(2); // быстрый таймаут сетевых операций
  mqttClient.setKeepAlive(30); // keep-alive для снижения задержек
}

inline bool resolveMqttHost(){ // резолв MQTT хоста без блокировки
  if(mqttHost.length() == 0) return false; // пустой host
  if(mqttHasResolvedIp && mqttResolvedHostName == mqttHost) return true; // уже есть IP
  IPAddress ip;
  if(ip.fromString(mqttHost)){
    mqttResolvedIp = ip;
    mqttHasResolvedIp = true;
    mqttResolvedHostName = mqttHost;
    return true;
  }
  const unsigned long now = millis();
  if(now - mqttLastResolveAttempt < mqttResolveInterval){
    return mqttHasResolvedIp; // ждём следующего окна резолва
  }
  mqttLastResolveAttempt = now;
  if(WiFi.status() != WL_CONNECTED) return mqttHasResolvedIp; // нет Wi-Fi
  if(WiFi.hostByName(mqttHost.c_str(), ip)){
    mqttResolvedIp = ip;
    mqttHasResolvedIp = true;
    mqttResolvedHostName = mqttHost;
    return true;
  }
  return mqttHasResolvedIp; // используем предыдущий IP если был
}

inline void connectMqtt(){ // подключение к MQTT
  if(!mqttEnabled) return; // выход если MQTT выключен
  if(mqttHost.length() == 0) return; // выход если host пустой
  if(WiFi.status() != WL_CONNECTED) return; // выход если нет Wi-Fi
if(!resolveMqttHost()) return; // хост не резолвится
  const unsigned long now = millis(); // текущее время
  if(now - mqttLastConnectAttempt < mqttConnectInterval) return; // защита от частых попыток
  mqttLastConnectAttempt = now; // фиксация попытки подключения

  if(!mqttClient.connected()){ // если не подключены
    String clientId = "espdash-" + WiFi.macAddress(); // уникальный clientId
bool connected = mqttClient.connect( // подключение с логином и LWT
      clientId.c_str(),
      mqttUsername.c_str(),
      mqttPassword.c_str(),
      mqttAvailabilityTopic,
      0,
      true,
      "offline"
    );

    if(connected){ // если подключение успешно
      mqttIsConnected = true; // обновление флага
       publishMqttAvailability("online", true); // публикация доступности
 #if 0 // MQTT Discovery отключен
      mqttDiscoveryStage = DISCOVERY_NONE; // сброс этапа discovery
      mqttDiscoveryIndex = 0; // сброс индекса
      mqttDiscoveryLastAttempt = 0; // сброс таймера
            mqttDiscoveryPending = true; // публикация MQTT Discovery после первого loop
      publishHomeAssistantDiscovery(); // попытка публикации сразу после подключения
      #endif

      mqttClient.subscribe("home/esp32/tempSet", 0); // подписка на топик
      mqttClient.subscribe("home/esp32/Power_Filtr/set", 0); // команда фильтра
      mqttClient.subscribe("home/esp32/Filtr_Time1/set", 0); // таймер фильтрации №1
      mqttClient.subscribe("home/esp32/Filtr_Time2/set", 0); // таймер фильтрации №2
      mqttClient.subscribe("home/esp32/Filtr_Time3/set", 0); // таймер фильтрации №3
      mqttClient.subscribe("home/esp32/Power_Clean/set", 0); // промывка фильтра
      mqttClient.subscribe("home/esp32/Clean_Time1/set", 0); // таймер промывки
      mqttClient.subscribe("home/esp32/DaysSelect/set", 0); // дни промывки
      mqttClient.subscribe("home/esp32/FiltrTimer1_ON/set", 0); // время фильтрации №1 ON
      mqttClient.subscribe("home/esp32/FiltrTimer1_OFF/set", 0); // время фильтрации №1 OFF
      mqttClient.subscribe("home/esp32/FiltrTimer2_ON/set", 0); // время фильтрации №2 ON
      mqttClient.subscribe("home/esp32/FiltrTimer2_OFF/set", 0); // время фильтрации №2 OFF
      mqttClient.subscribe("home/esp32/FiltrTimer3_ON/set", 0); // время фильтрации №3 ON
      mqttClient.subscribe("home/esp32/FiltrTimer3_OFF/set", 0); // время фильтрации №3 OFF
      mqttClient.subscribe("home/esp32/CleanTimer1_ON/set", 0); // время промывки ON
      mqttClient.subscribe("home/esp32/CleanTimer1_OFF/set", 0); // время промывки OFF
      mqttClient.subscribe("home/esp32/Pow_Ul_light/set", 0); // команда освещения
      mqttClient.subscribe("home/esp32/Activation_Heat/set", 0); // команда нагрева
      mqttClient.subscribe("home/esp32/SetLamp/set", 0); // команда режима лампы
      mqttClient.subscribe("home/esp32/SetRGB/set", 0); // команда режима RGB
      mqttClient.subscribe("home/esp32/button/restart/set", 0); // команда перезапуска
    
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
  mqttLastConnectAttempt = 0; // сброс таймера подключений
  mqttLastResolveAttempt = 0; // сброс таймера резолва

  #if 0 // MQTT Discovery отключен
  mqttDiscoveryPending = false; // сброс discovery
  mqttDiscoveryStage = DISCOVERY_NONE; // сброс этапа
  mqttDiscoveryIndex = 0; // сброс индекса
  #endif
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

  #if 0 // MQTT Discovery отключен
    if(mqttDiscoveryPending) publishHomeAssistantDiscovery(); // публикация после первого loop
#endif

  unsigned long now = millis(); // текущее время
  if(now - mqttLastPublish >= mqttPublishInterval){ // проверка интервала
    mqttLastPublish = now; // обновление времени
    if(mqttClient.connected()){ // если подключены
      String payload = String("ESP32 uptime: ") + String(now / 1000) + "s"; // сообщение
      
      mqttClient.publish("home/esp32/status", payload.c_str(), true); // публикация
    
      publishMqttStateString("home/esp32/test", "123");
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
      publishMqttStateBool("home/esp32/Filtr_Time1", Filtr_Time1);
      publishMqttStateBool("home/esp32/Filtr_Time2", Filtr_Time2);
      publishMqttStateBool("home/esp32/Filtr_Time3", Filtr_Time3);
      publishMqttStateBool("home/esp32/Power_Clean", Power_Clean);
      publishMqttStateBool("home/esp32/Clean_Time1", Clean_Time1);
      publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
      publishMqttStateString("home/esp32/FiltrTimer1_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer1")));
      publishMqttStateString("home/esp32/FiltrTimer1_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer1")));
      publishMqttStateString("home/esp32/FiltrTimer2_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer2")));
      publishMqttStateString("home/esp32/FiltrTimer2_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer2")));
      publishMqttStateString("home/esp32/FiltrTimer3_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer3")));
      publishMqttStateString("home/esp32/FiltrTimer3_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer3")));
      publishMqttStateString("home/esp32/CleanTimer1_ON", formatMinutesToTime(mqttTimerOnMinutes("CleanTimer1")));
      publishMqttStateString("home/esp32/CleanTimer1_OFF", formatMinutesToTime(mqttTimerOffMinutes("CleanTimer1")));
      publishMqttStateBool("home/esp32/Pow_Ul_light", Pow_Ul_light);
      publishMqttStateBool("home/esp32/Activation_Heat", Activation_Heat);

      publishMqttStateString("home/esp32/SetLamp", SetLamp);
      publishMqttStateString("home/esp32/SetRGB", SetRGB);
    
    }
  }
}
