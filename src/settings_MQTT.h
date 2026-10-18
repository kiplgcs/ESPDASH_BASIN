#pragma once // защита от повторного включения файла

#include <Arduino.h> // базовые функции Arduino
#include <WiFi.h> // работа с Wi-Fi
#include <SPIFFS.h> // файловая система SPIFFS
#include <PubSubClient.h> // MQTT клиент
#include <ArduinoJson.h> // работа с JSON
#include <map>

#include "wifi_manager.h" // менеджер Wi-Fi
#include "fs_utils.h" // утилиты файловой системы

inline WiFiClient mqttWifiClient; // WiFi-клиент для MQTT
inline PubSubClient mqttClient(mqttWifiClient); // MQTT-клиент поверх WiFi

inline const char* mqttConfigPath = "/mqtt.json"; // путь к файлу настроек MQTT
inline String mqttHost = "192.168.0.100"; //"broker.emqx.io"; // адрес брокера
inline uint16_t mqttPort = 1883; // порт брокера
inline String mqttUsername = ""; // имя пользователя MQTT
inline String mqttPassword = ""; // пароль MQTT
inline String mqttBaseTopic = "home/esp32"; // базовый MQTT топик
inline bool mqttEnabled = true; // флаг включения MQTT
inline bool mqttIsConnected = false; // флаг подключения MQTT
inline unsigned long mqttPublishInterval = 10000; // интервал публикации
inline unsigned long mqttLastPublish = 0; // время последней публикации
inline const char* mqttAvailabilityTopic = "home/esp32/availability"; // топик доступности устройства
inline std::map<String, String> mqttLastPublishedPayloads; // Последние отправленные payload по topic, чтобы не слать одно и то же.
inline bool mqttForcePublishAllStates = true; // После старта/переподключения один раз отправляем полный снимок.

inline String mqttNormalizeBaseTopic(String topic){
  topic.trim();
  topic.replace("\\", "/");
  while(topic.startsWith("/")) topic.remove(0, 1);
  while(topic.endsWith("/") && topic.length() > 0) topic.remove(topic.length() - 1);
  if(topic.length() == 0) topic = "home/esp32";
  return topic;
}

inline void mqttApplyBaseTopic(String topic){
  mqttBaseTopic = mqttNormalizeBaseTopic(topic);
}

inline String mqttTopicFromLegacy(const String &topic){
  const String legacyBase = "home/esp32";
  String resolved = topic;
  resolved.trim();
  String base = mqttNormalizeBaseTopic(mqttBaseTopic);
  if(resolved == legacyBase) return base;
  if(resolved.startsWith(legacyBase + "/")){
    return base + resolved.substring(legacyBase.length());
  }
  return resolved;
}

inline String mqttLegacyTopicFromConfigured(const String &topic){
  const String legacyBase = "home/esp32";
  String incoming = topic;
  incoming.trim();
  String base = mqttNormalizeBaseTopic(mqttBaseTopic);
  if(incoming == base) return legacyBase;
  if(incoming.startsWith(base + "/")){
    return legacyBase + incoming.substring(base.length());
  }
  return incoming;
}

inline String mqttAvailabilityTopicString(){
  return mqttTopicFromLegacy(String(mqttAvailabilityTopic));
}

#if 1 // MQTT Discovery включен
inline const char* mqttDiscoveryPrefix = "homeassistant"; // префикс MQTT Discovery
inline bool mqttDiscoveryPending = false; // публикация после первого успешного MQTT loop

inline unsigned long mqttDiscoveryLastAttempt = 0; // время последней попытки discovery
inline const unsigned long mqttDiscoveryInterval = 250; // интервал между пакетами discovery
inline const uint8_t mqttDiscoveryBatchSize = 4; // максимум сущностей за loop
inline const uint8_t mqttDiscoveryMaxRetries = 3; // максимум попыток публикации на сущность
inline bool mqttDiscoveryFullDevicePublished = false; // полный device блок опубликован
inline size_t mqttDiscoveryLastMaxPayload = 0; // максимум payload discovery
inline size_t mqttDiscoveryRetryIndex = 0; // индекс повторной публикации
inline uint8_t mqttDiscoveryRetryCount = 0; // число повторов на сущность
inline bool mqttDiscoveryLegacyCleanupDone = false; // очистка удалённых legacy-сущностей
#endif

// inline void publishMqttAvailability(const char* payload, bool retain = true){ // публикация доступности
//   if(!mqttClient.connected()) return; // выход если нет подключения
//   mqttClient.publish(mqttAvailabilityTopic, payload, retain); // публикация статуса
// }

inline unsigned long mqttLastConnectAttempt = 0; // время последней попытки подключения
inline const unsigned long mqttConnectInterval = 5000; // интервал попыток подключения
inline unsigned long mqttReconnectBlockedUntil = 0; // После ошибок держим паузу, чтобы MQTT не подвешивал сетевую задачу частыми connect().
inline uint8_t mqttConsecutiveFailures = 0; // Счетчик подряд идущих ошибок подключения для увеличения паузы.
inline int mqttLastConnectState = MQTT_DISCONNECTED; // Последний код PubSubClient::state() для диагностики отказов MQTT.
inline unsigned long mqttLastResolveAttempt = 0; // время последней попытки DNS
inline const unsigned long mqttResolveInterval = 30000; // интервал резолва хоста
inline bool mqttHasResolvedIp = false; // флаг успешного резолва
inline IPAddress mqttResolvedIp; // резолвенный IP адрес
inline String mqttResolvedHostName; // хост для которого выполнен резолв


#if 1 // MQTT Discovery включен
enum DiscoveryStage {
  DISCOVERY_NONE,
  DISCOVERY_MAIN_ENTITIES,
  DISCOVERY_DONE
};

inline DiscoveryStage mqttDiscoveryStage = DISCOVERY_NONE; // этап discovery
inline size_t mqttDiscoveryIndex = 0; // индекс публикации сущностей

// mqttDiscovery публикуется один раз после успешного подключения и availability=online.
#endif

extern float DS1; // температура воды
extern float DS2; // температура после нагревателя
extern bool DS2Available; // достоверность температуры после нагревателя
extern float PH; // pH воды
extern float ppmCl; // уровень хлора
extern int corrected_ORP_Eh_mV; // ORP, мВ
extern String OverlayPoolTemp; // температура воды (оверлей)
extern String OverlayHeaterTemp; // температура после нагревателя (оверлей)
extern String OverlayLevelUpper; // верхний уровень (оверлей)
extern String OverlayLevelLower; // нижний уровень (оверлей)
extern String OverlayPh; // pH (оверлей)
extern String OverlayChlorine; // хлор (оверлей)
extern String OverlayFilterState; // состояние фильтрации (строка)
extern String OverlayDrainPit; // состояние сливной ямы (оверлей)
extern String OverlayLight; // освещенность (оверлей)
extern String InfoString2; // строка статуса лампы
extern String InfoStringDIN; // строка статуса DIN
extern String ThemeColor; // цвет темы
extern int MotorSpeedSetting; // скорость мотора
extern int RangeMin; // минимум диапазона
extern int RangeMax; // максимум диапазона
extern int IntInput; // целочисленный ввод
extern float FloatInput; // ввод float
extern String Timer1; // время таймера
extern String Comment; // комментарий
extern int RandomVal; // случайное значение
extern int button1; // кнопка 1
extern int button2; // кнопка 2
extern bool Power_H2O2; // состояние насоса NaOCl
extern bool Power_ACO; // состояние насоса ACO
extern bool Power_Heat; // состояние нагрева
extern bool Power_Topping_State; // состояние соленоида долива
extern bool Power_Topping; // ручной долив
extern bool Power_Drain_State; // состояние режима слива (насос включен)
extern bool Power_Drain; // ручной слив
extern bool Power_Filtr; // ручная фильтрация
extern bool Filtr_Time1; // таймер фильтрации №1
extern bool Filtr_Time2; // таймер фильтрации №2
extern bool Filtr_Time3; // таймер фильтрации №3
extern bool Power_Clean; // промывка фильтра
extern bool Clean_Time1; // таймер промывки
extern bool Pow_Ul_light; // ручное освещение
extern bool Pow_Yard_light; // освещение во дворе
extern bool Ul_light_Time; // таймер уличного освещения
extern bool UlLight_autosvet; // уличное освещение по датчику
extern bool UlLight_sensorTimerOff; // улица: датчик включает, таймер отключает
extern bool Activation_Heat; // управление нагревом
extern bool Activation_Water_Level; // контроль уровня воды
extern bool WaterLevelSensorUpper; // датчик верхнего уровня
extern bool WaterLevelSensorLower; // датчик нижнего уровня
extern bool WaterLevelSensorDrain; // датчик уровня для остановки слива
extern float PH_Lower; // нижний предел гистерезиса pH
extern float PH_setting; // верхняя уставка pH
extern float PH_Upper; // верхний предел гистерезиса pH
extern bool PH_Control_ACO; // контроль pH ACO
extern int ACO_Work; // период дозирования ACO
extern float PH1; // нижняя граница pH
extern float PH2; // верхняя граница pH
extern float PH1_CAL; // калибровка pH1
extern float PH2_CAL; // калибровка pH2
extern float Temper_Reference; // температура референса
extern float Temper_PH; // температура компенсации pH
extern int analogValuePH_Comp; // значение АЦП PH
extern bool NaOCl_H2O2_Control; // контроль хлора
extern float CL_Lower; // нижний предел гистерезиса CL
extern float CL_Upper; // верхний предел гистерезиса CL
extern int ORP_setting; // уставка ORP
extern int H2O2_Work; // период дозирования NaOCl
extern int CalRastvor256mV; // калибровочный раствор
extern int Calibration_ORP_mV; // калибровочный коэффициент ORP
extern int Lumen_Ul; // освещенность на улице
extern bool Pow_WS2815; // RGB лента (ручная)
extern bool WS2815_Time1; // таймер RGB
extern String LEDColor; // цвет RGB
extern String LedColorMode; // режим цвета
extern int LedBrightness; // яркость
extern String LedPattern; // режим подсветки
extern int LedAutoplayDuration; // период автосмены
extern bool LedAutoplay; // автосмена
extern String LedColorOrder; // порядок цветов
extern int LampTimerON; // таймер лампы ON
extern int LampTimerOFF; // таймер лампы OFF
extern int RgbTimerON; // таймер RGB ON
extern int RgbTimerOFF; // таймер RGB OFF
extern int UlLightTimerON; // таймер уличного освещения ON
extern int UlLightTimerOFF; // таймер уличного освещения OFF
extern int YardLightTimerON; // таймер освещения во дворе ON
extern int YardLightTimerOFF; // таймер освещения во дворе OFF
extern int Sider_heat; // уставка нагрева
extern bool RoomTemper; // контроль температуры в помещении
extern float RoomTempOn; // граница включения отопления
extern float RoomTempOff; // граница выключения отопления
extern bool Power_Warm_floor_heating; // обогрев пола
extern String SetLamp; // режим лампы
extern String SetRGB; // режим RGB
extern String SetUlLight; // режим уличного освещения
extern String SetYardLight; // режим освещения во дворе
extern bool Lamp; // состояние лампы
extern bool Lamp_autosvet; // авто-режим лампы
extern bool Power_Time1; // режим таймера лампы
extern bool Pow_WS2815_autosvet; // авто-режим RGB
extern String DaysSelect; // выбранные дни промывки
extern bool ManualPulse_H2O2_Active; // фактический ручной импульс насоса NaOCl
extern bool ManualPulse_ACO_Active; // фактический ручной импульс насоса ACO
extern bool ReadRelayArray[16]; // фактические состояния реле Modbus
extern bool ReadInputArray[16]; // фактические дискретные входы Modbus
extern bool AktualReadRelay; // признак актуального ответа блока реле
extern bool AktualReadInput; // признак актуального ответа блока входов
extern bool Rs485Enabled;
extern int Rs485BaudRate;
extern String Rs485UartMode;
extern int Rs485SlaveId;
extern int Rs485PollIntervalMs;
extern bool PoolLowerLevelLowConfirmed; // подтвержденный нижний уровень бассейна
extern bool PoolUpperLevelReachedConfirmed; // подтвержденный верхний уровень бассейна
extern bool DrainPitFullConfirmed; // подтвержденное заполнение сливной ямы
extern int LampLightOffThreshold; // порог включения лампы в темноте
extern int LampLightOnThreshold; // порог выключения лампы при свете
extern int RgbLightOffThreshold; // порог включения RGB в темноте
extern int RgbLightOnThreshold; // порог выключения RGB при свете
extern int UlLightOffThreshold; // порог включения улицы в темноте
extern int UlLightOnThreshold; // порог выключения улицы при свете
extern int YardLightOffThreshold; // порог включения двора в темноте
extern int YardLightOnThreshold; // порог выключения двора при свете
extern String LightSensorLiveInfo; // диагностическая строка датчика освещенности
extern String LampLightAutoState; // состояние автоматики лампы
extern String RgbLightAutoState; // состояние автоматики RGB
extern String UlLightAutoState; // состояние автоматики улицы
extern String YardLightAutoState; // состояние автоматики освещения во дворе
extern String PoolWaterLevelStageInfo; // этап автоматики уровня бассейна
extern String DrainPitStageInfo; // этап автоматики слива
extern int ToppingWorkMinutes; // время работы долива
extern int ToppingPauseMinutes; // пауза долива
extern int DrainWorkMinutes; // время порции слива
extern int DrainPauseMinutes; // пауза слива
extern int DrainCyclesTarget; // заданное число порций слива
extern int DrainCyclesDone; // выполненное число порций слива
extern bool AirPump; // ручной компрессор промывки
extern bool AirPumpAuto; // автоматический компрессор промывки
extern bool SolValveFilBack; // ручной клапан промывки
extern bool ValveBackwashAuto; // автоматический клапан промывки
extern bool SolValveFiltration; // ручной клапан фильтрации
extern bool ValveFiltrationAuto; // автоматический клапан фильтрации
extern bool SolSandDump; // ручной сброс песка
extern bool SolSandDumpAuto; // автоматический сброс песка
extern bool CleanSequenceActive; // активная последовательность промывки
extern int TimerAirSetting; // время компрессора промывки
extern int TimerValveSetting; // время переключения клапанов
extern int TimerBackwashSetting; // время обратной промывки
extern int TimerSolSandDump; // время сброса песка
extern String CurrentTime; // текущее локальное время
extern int gmtOffset_correct; // текущий часовой пояс
class UIRegistry; // forward declaration
extern UIRegistry ui; // доступ к UI-реестру таймеров
void syncCleanDaysFromSelection(); // синхронизация дней промывки
String uiValueForId(const String &id); // получение значения UI по id
bool uiApplyValueForId(const String &id, const String &value); // применение значения UI по id
bool applyRs485ConfigRequest(const String &key, const String &value);

String formatMinutesToTime(uint16_t minutes); // форматирование минут в HH:MM
bool mqttApplyTimerField(const String &fieldId, const String &value); // применить значение таймера
uint16_t mqttTimerOnMinutes(const String &id); // время включения таймера
uint16_t mqttTimerOffMinutes(const String &id); // время выключения таймера
void mqttApplyDaysSelect(const String &value); // применить выбор дней промывки
bool mqttDaysSelectContains(const char* dayToken); // проверить включен ли день в DaysSelect

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
  String next = value;
  next.trim();

  if(next.startsWith("toggle:") || next.startsWith("add:") || next.startsWith("remove:")){
    bool removeDay = next.startsWith("remove:");
    const int prefixLen = next.startsWith("toggle:") ? 7 : (removeDay ? 7 : 4);
    const String day = next.substring(prefixLen);
    String marked = "," + DaysSelect + ",";
    const String token = day + ",";
    const bool hasDay = marked.indexOf(token) >= 0;
    if((next.startsWith("toggle:") && hasDay) || (removeDay && hasDay)){
      marked.replace(token, ",");
    } else if((next.startsWith("toggle:") && !hasDay) || (!removeDay && !next.startsWith("toggle:") && !hasDay)){
      marked += day + ",";
    }
    while(marked.indexOf(",,") >= 0) marked.replace(",,", ",");
    if(marked.startsWith(",")) marked.remove(0, 1);
    if(marked.endsWith(",")) marked.remove(marked.length() - 1);
    next = marked;
  }

  DaysSelect = next;
  syncCleanDaysFromSelection();
  saveValue<String>("DaysSelect", DaysSelect);
}

inline bool mqttDaysSelectContains(const char* dayToken){
  if(dayToken == nullptr || dayToken[0] == '\0') return false;
  const String marked = "," + DaysSelect + ",";
  const String token = String(dayToken) + ",";
  return marked.indexOf(token) >= 0;
}

inline bool mqttApplyDualRangeInt(const String &payload, int &minRef, int &maxRef,
                                  const char* minKey, const char* maxKey){
  int sep = payload.indexOf('-');
  if(sep < 0) return false;
  int nextMin = payload.substring(0, sep).toInt();
  int nextMax = payload.substring(sep + 1).toInt();
  if(nextMin > nextMax){
    int temp = nextMin;
    nextMin = nextMax;
    nextMax = temp;
  }
  minRef = nextMin;
  maxRef = nextMax;
  saveValue<int>(minKey, minRef);
  saveValue<int>(maxKey, maxRef);
  return true;
}

inline bool mqttApplyDualRangeFloat(const String &payload, float &minRef, float &maxRef,
                                    const char* minKey, const char* maxKey){
  int sep = payload.indexOf('-');
  if(sep < 0) return false;
  float nextMin = payload.substring(0, sep).toFloat();
  float nextMax = payload.substring(sep + 1).toFloat();
  if(nextMin > nextMax){
    float temp = nextMin;
    nextMin = nextMax;
    nextMax = temp;
  }
  minRef = nextMin;
  maxRef = nextMax;
  saveValue<float>(minKey, minRef);
  saveValue<float>(maxKey, maxRef);
  return true;
}


inline void publishMqttStateString(const char* topic, const String &value); // forward declaration
inline void publishMqttStateBool(const char* topic, bool value); // forward declaration
inline void publishMqttStateFloat(const char* topic, float value); // forward declaration
inline void publishMqttStateInt(const char* topic, int value); // forward declaration

inline bool mqttParseBooleanPayload(String payload, bool &value){ // принимает только явные MQTT boolean, не превращая мусор в OFF
  payload.trim();
  payload.toLowerCase();
  if(payload == "1" || payload == "on" || payload == "true"){
    value = true;
    return true;
  }
  if(payload == "0" || payload == "off" || payload == "false"){
    value = false;
    return true;
  }
  return false;
}

inline bool mqttParseFiniteFloat(String payload, float &value){ // строгий парсер числовых настроек
  payload.trim();
  if(payload.length() == 0) return false;
  char *end = nullptr;
  const float parsed = strtof(payload.c_str(), &end);
  if(end == payload.c_str() || end == nullptr || *end != '\0' || !isfinite(parsed)) return false;
  value = parsed;
  return true;
}

inline bool mqttParseInteger(String payload, int &value){ // строгий парсер целых настроек
  payload.trim();
  if(payload.length() == 0) return false;
  char *end = nullptr;
  const long parsed = strtol(payload.c_str(), &end, 10);
  if(end == payload.c_str() || end == nullptr || *end != '\0') return false;
  value = static_cast<int>(parsed);
  return true;
}

inline bool mqttValidateDualFloatPayload(const String &payload){ // проверка формата min-max до передачи UI-валидатору
  const int separator = payload.indexOf('-');
  if(separator <= 0 || separator >= static_cast<int>(payload.length()) - 1) return false;
  float first = 0.0f;
  float second = 0.0f;
  return mqttParseFiniteFloat(payload.substring(0, separator), first) &&
         mqttParseFiniteFloat(payload.substring(separator + 1), second);
}

inline void mqttBeginHomeAssistantReplay(bool cleanupLegacy){ // discovery, затем принудительный полный state-снимок
  (void)cleanupLegacy; // Устаревшие retained config очищаются один раз за boot; reconnect не повторяет лишний burst.
  mqttForcePublishAllStates = true;
  mqttLastPublish = 0;
#if 1 // MQTT Discovery включен
  mqttDiscoveryStage = DISCOVERY_NONE;
  mqttDiscoveryIndex = 0;
  mqttDiscoveryLastAttempt = 0;
  mqttDiscoveryPending = true;
  mqttDiscoveryFullDevicePublished = false;
  mqttDiscoveryLastMaxPayload = 0;
  mqttDiscoveryRetryIndex = 0;
  mqttDiscoveryRetryCount = 0;
#endif
}

inline void mqttPublishWaterControlStates(){ // публикует итог после применения взаимных блокировок воды
  publishMqttStateBool("home/esp32/Activation_Water_Level", Activation_Water_Level);
  publishMqttStateBool("home/esp32/Power_Topping", Power_Topping);
  publishMqttStateBool("home/esp32/Power_Drain", Power_Drain);
}

inline void mqttPublishChemistryLimits(){ // один предел может нормализовать соседний, поэтому отправляем весь блок
  publishMqttStateFloat("home/esp32/PH_Lower", PH_Lower);
  publishMqttStateFloat("home/esp32/PH_Upper", PH_Upper);
  publishMqttStateFloat("home/esp32/PH_setting", PH_Upper); // совместимый старый state topic
  publishMqttStateFloat("home/esp32/CL_Lower", CL_Lower);
  publishMqttStateFloat("home/esp32/CL_Upper", CL_Upper);
}

inline void mqttPublishLightThresholds(){ // нормализация одной границы может изменить вторую
  publishMqttStateInt("home/esp32/LampLightOffThreshold", LampLightOffThreshold);
  publishMqttStateInt("home/esp32/LampLightOnThreshold", LampLightOnThreshold);
  publishMqttStateInt("home/esp32/RgbLightOffThreshold", RgbLightOffThreshold);
  publishMqttStateInt("home/esp32/RgbLightOnThreshold", RgbLightOnThreshold);
  publishMqttStateInt("home/esp32/UlLightOffThreshold", UlLightOffThreshold);
  publishMqttStateInt("home/esp32/UlLightOnThreshold", UlLightOnThreshold);
  publishMqttStateInt("home/esp32/YardLightOffThreshold", YardLightOffThreshold);
  publishMqttStateInt("home/esp32/YardLightOnThreshold", YardLightOnThreshold);
}

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

inline bool mqttIsAllowedMode(String value){ // проверка режима
  value.trim();
  value.toLowerCase();
  return value == "off" || value == "on" || value == "auto" || value == "auto_timer_off" || value == "timer"; // допустимые режимы
}

inline String mqttNormalizedMode(String value){
  value.trim();
  value.toLowerCase();
  return value;
}

inline void mqttApplyLampMode(const String &rawMode){
  applyLampControlMode(rawMode);
}

inline void mqttApplyRgbMode(const String &rawMode){
  applyRgbControlMode(rawMode);
}

inline void mqttApplyUlLightMode(const String &rawMode){
  applyUlLightControlMode(rawMode);
}

inline void mqttApplyYardLightMode(const String &rawMode){
  applyYardLightControlMode(rawMode);
}

inline String mqttLedAutoplayState(){
  return LedAutoplay ? "Автоматически" : "Вручную";
}

inline bool mqttParseLedAutoplay(const String &rawValue, bool &parsedValue){
  String value = rawValue;
  value.trim();
  value.toLowerCase();

  if(value == "1" || value == "true" || value == "on" || value == "auto" || value == "автоматически" || value == "автомат"){
    parsedValue = true;
    return true;
  }

  if(value == "0" || value == "false" || value == "off" || value == "manual" || value == "вручную" || value == "ручной"){
    parsedValue = false;
    return true;
  }

  return false;
}

inline void handleMqttCommandMessage(char* topic, byte* payload, unsigned int length){ // обработка MQTT команд
  String topicStr = mqttLegacyTopicFromConfigured(String(topic)); // входящие команды приводим к старой схеме home/esp32/...
  String message; // payload строкой
  message.reserve(length); // резерв
  for(unsigned int i = 0; i < length; ++i){
    message += static_cast<char>(payload[i]); // сбор payload
  }
  message.trim(); // очистка

  if(topicStr == "homeassistant/status"){ // HA birth: retained discovery уже есть, но state topics могли потеряться при рестарте HA
    String status = message;
    status.toLowerCase();
    if(status == "online") mqttBeginHomeAssistantReplay(false);
    return;
  }

  if(topicStr == "home/esp32/Power_Filtr/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    Power_Filtr = requestedState; // обновление состояния
    saveValue<int>("Power_Filtr", Power_Filtr ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Power_Filtr", Power_Filtr || (rs485RelayDataFresh() && ReadRelayArray[8])); // stale readback не маскирует фактическую потерю связи
    return;
  }

  if(topicStr == "home/esp32/Filtr_Time1/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    Filtr_Time1 = requestedState; // обновление состояния
    saveValue<int>("Filtr_Time1", Filtr_Time1 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Filtr_Time1", Filtr_Time1); // публикация
    return;
  }

  if(topicStr == "home/esp32/Filtr_Time2/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    Filtr_Time2 = requestedState; // обновление состояния
    saveValue<int>("Filtr_Time2", Filtr_Time2 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Filtr_Time2", Filtr_Time2); // публикация
    return;
  }

  if(topicStr == "home/esp32/Filtr_Time3/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    Filtr_Time3 = requestedState; // обновление состояния
    saveValue<int>("Filtr_Time3", Filtr_Time3 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Filtr_Time3", Filtr_Time3); // публикация
    return;
  }

  if(topicStr == "home/esp32/Power_Clean/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    Power_Clean = requestedState; // обновление состояния
    saveValue<int>("Power_Clean", Power_Clean ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Power_Clean", Power_Clean || CleanSequenceActive); // Реле 4 является свободным каналом, а не статусом промывки.
    return;
  }

  if(topicStr == "home/esp32/Power_Drain/set" ||
     topicStr == "home/esp32/Power_Topping/set" ||
     topicStr == "home/esp32/Activation_Water_Level/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    String controlId;
    if(topicStr == "home/esp32/Power_Drain/set") controlId = "Power_Drain";
    else if(topicStr == "home/esp32/Power_Topping/set") controlId = "Power_Topping";
    else controlId = "Activation_Water_Level";
    if(applyWaterControlRequest(controlId, requestedState)) mqttPublishWaterControlStates();
    return;
  }

  if(topicStr == "home/esp32/Rs485Enabled/set" ||
     topicStr == "home/esp32/Rs485BaudRate/set" ||
     topicStr == "home/esp32/Rs485UartMode/set" ||
     topicStr == "home/esp32/Rs485SlaveId/set" ||
     topicStr == "home/esp32/Rs485PollIntervalMs/set"){
    const String prefix = "home/esp32/";
    const String settingId = topicStr.substring(prefix.length(), topicStr.length() - 4);
    String normalizedValue = message;
    if(settingId == "Rs485Enabled"){
      bool enabled = false;
      if(!mqttParseBooleanPayload(message, enabled)) return;
      if(!enabled){ // Нельзя останавливать шину из MQTT: физические реле могли остаться включенными без возможности отправить OFF.
        publishMqttStateBool("home/esp32/Rs485Enabled", Rs485Enabled);
        return;
      }
      normalizedValue = enabled ? "1" : "0";
    } else if(settingId == "Rs485BaudRate"){
      int baud = 0;
      if(!mqttParseInteger(message, baud) ||
         !(baud == 1200 || baud == 2400 || baud == 4800 || baud == 9600 ||
           baud == 19200 || baud == 38400 || baud == 57600 || baud == 115200)) return;
    } else if(settingId == "Rs485UartMode"){
      if(!(message == "8N1" || message == "8N2" || message == "8O1" || message == "8E1")) return;
    } else {
      int parsed = 0;
      if(!mqttParseInteger(message, parsed)) return;
    }
    if(!applyRs485ConfigRequest(settingId, normalizedValue)) return;
    if(settingId == "Rs485Enabled") publishMqttStateBool("home/esp32/Rs485Enabled", Rs485Enabled);
    else if(settingId == "Rs485BaudRate") publishMqttStateInt("home/esp32/Rs485BaudRate", Rs485ActiveBaudRate);
    else if(settingId == "Rs485UartMode") publishMqttStateString("home/esp32/Rs485UartMode", Rs485ActiveUartMode);
    else if(settingId == "Rs485SlaveId") publishMqttStateInt("home/esp32/Rs485SlaveId", Rs485ActiveSlaveId);
    else publishMqttStateInt("home/esp32/Rs485PollIntervalMs", Rs485PollIntervalMs);
    return;
  }


  if(topicStr == "home/esp32/Clean_Time1/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    Clean_Time1 = requestedState; // обновление состояния
    saveValue<int>("Clean_Time1", Clean_Time1 ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Clean_Time1", Clean_Time1); // публикация
    return;
  }

  if(topicStr == "home/esp32/DaysSelect/set"){
    mqttApplyDaysSelect(message); // обновление строки дней
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect); // публикация
     publishMqttStateBool("home/esp32/DaysMonToggle", mqttDaysSelectContains("Mon"));
    publishMqttStateBool("home/esp32/DaysTueToggle", mqttDaysSelectContains("Tue"));
    publishMqttStateBool("home/esp32/DaysWedToggle", mqttDaysSelectContains("Wed"));
    publishMqttStateBool("home/esp32/DaysThuToggle", mqttDaysSelectContains("Thu"));
    publishMqttStateBool("home/esp32/DaysFriToggle", mqttDaysSelectContains("Fri"));
    publishMqttStateBool("home/esp32/DaysSatToggle", mqttDaysSelectContains("Sat"));
    publishMqttStateBool("home/esp32/DaysSunToggle", mqttDaysSelectContains("Sun"));
    return;
  }

  if(topicStr == "home/esp32/DaysMonToggle/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyDaysSelect(requestedState ? "add:Mon" : "remove:Mon");
    publishMqttStateBool("home/esp32/DaysMonToggle", mqttDaysSelectContains("Mon"));
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
    return;
  }

  if(topicStr == "home/esp32/DaysTueToggle/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyDaysSelect(requestedState ? "add:Tue" : "remove:Tue");
    publishMqttStateBool("home/esp32/DaysTueToggle", mqttDaysSelectContains("Tue"));
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
    return;
  }

  if(topicStr == "home/esp32/DaysWedToggle/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyDaysSelect(requestedState ? "add:Wed" : "remove:Wed");
    publishMqttStateBool("home/esp32/DaysWedToggle", mqttDaysSelectContains("Wed"));
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
    return;
  }

  if(topicStr == "home/esp32/DaysThuToggle/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyDaysSelect(requestedState ? "add:Thu" : "remove:Thu");
    publishMqttStateBool("home/esp32/DaysThuToggle", mqttDaysSelectContains("Thu"));
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
    return;
  }

  if(topicStr == "home/esp32/DaysFriToggle/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyDaysSelect(requestedState ? "add:Fri" : "remove:Fri");
    publishMqttStateBool("home/esp32/DaysFriToggle", mqttDaysSelectContains("Fri"));
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
    return;
  }

  if(topicStr == "home/esp32/DaysSatToggle/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyDaysSelect(requestedState ? "add:Sat" : "remove:Sat");
    publishMqttStateBool("home/esp32/DaysSatToggle", mqttDaysSelectContains("Sat"));
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
    return;
  }

  if(topicStr == "home/esp32/DaysSunToggle/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyDaysSelect(requestedState ? "add:Sun" : "remove:Sun");
    publishMqttStateBool("home/esp32/DaysSunToggle", mqttDaysSelectContains("Sun"));
    publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
    return;
  }


  if(topicStr == "home/esp32/RoomTempRange/set"){
    if(mqttValidateDualFloatPayload(message) && uiApplyValueForId("RoomTempRange", message))
      publishMqttStateString("home/esp32/RoomTempRange", String(RoomTempOn, 1) + "-" + String(RoomTempOff, 1));
    return;
  }

  if(topicStr == "home/esp32/Float_PH_Slider/set") {
    mqttApplyDualRangeFloat(message, PH1, PH2, "PH1_MIN", "PH2_MAX");
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

 if(topicStr == "home/esp32/LampTimer_ON/set"){
    if(mqttApplyTimerField("LampTimer_ON", message)){
      publishMqttStateString("home/esp32/LampTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("LampTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/LampTimer_OFF/set"){
    if(mqttApplyTimerField("LampTimer_OFF", message)){
      publishMqttStateString("home/esp32/LampTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("LampTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/RgbTimer_ON/set"){
    if(mqttApplyTimerField("RgbTimer_ON", message)){
      publishMqttStateString("home/esp32/RgbTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("RgbTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/RgbTimer_OFF/set"){
    if(mqttApplyTimerField("RgbTimer_OFF", message)){
      publishMqttStateString("home/esp32/RgbTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("RgbTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/UlLightTimer_ON/set"){
    if(mqttApplyTimerField("UlLightTimer_ON", message)){
      publishMqttStateString("home/esp32/UlLightTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("UlLightTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/UlLightTimer_OFF/set"){
    if(mqttApplyTimerField("UlLightTimer_OFF", message)){
      publishMqttStateString("home/esp32/UlLightTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("UlLightTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/YardLightTimer_ON/set"){
    if(mqttApplyTimerField("YardLightTimer_ON", message)){
      publishMqttStateString("home/esp32/YardLightTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("YardLightTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/YardLightTimer_OFF/set"){
    if(mqttApplyTimerField("YardLightTimer_OFF", message)){
      publishMqttStateString("home/esp32/YardLightTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("YardLightTimer")));
    }
    return;
  }

  if(topicStr == "home/esp32/Pow_Ul_light/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyUlLightMode(requestedState ? "on" : "off");
    publishMqttStateBool("home/esp32/Pow_Ul_light", Pow_Ul_light || (rs485RelayDataFresh() && ReadRelayArray[15])); // публикация фактического состояния
    publishMqttStateString("home/esp32/SetUlLight", SetUlLight);
    publishMqttStateBool("home/esp32/Ul_light_Time", Ul_light_Time);
    return;
  }

  if(topicStr == "home/esp32/Pow_Yard_light/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyYardLightMode(requestedState ? "on" : "off");
    const bool yardLightActual = rs485RelayDataFresh() ? ReadRelayArray[7] : Pow_Yard_light;
    publishMqttStateBool("home/esp32/Pow_Yard_light", yardLightActual);
    publishMqttStateBool("home/esp32/YardLightActual", yardLightActual);
    publishMqttStateString("home/esp32/SetYardLight", SetYardLight);
    return;
  }

  if(topicStr == "home/esp32/Pow_WS2815/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    mqttApplyRgbMode(requestedState ? "on" : "off"); // совместимый switch меняет весь режим, а не один внутренний флаг
    publishMqttStateString("home/esp32/SetRGB", SetRGB);
    publishMqttStateBool("home/esp32/Pow_WS2815", Pow_WS2815 || (rs485RelayDataFresh() && ReadRelayArray[1]));
    publishMqttStateBool("home/esp32/WS2815_Time1", WS2815_Time1);
    return;
  }

  if(topicStr == "home/esp32/WS2815_Time1/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    if(requestedState) mqttApplyRgbMode("timer");
    else if(SetRGB == "timer") mqttApplyRgbMode("off");
    publishMqttStateString("home/esp32/SetRGB", SetRGB);
    publishMqttStateBool("home/esp32/Pow_WS2815", Pow_WS2815 || (rs485RelayDataFresh() && ReadRelayArray[1]));
    publishMqttStateBool("home/esp32/WS2815_Time1", WS2815_Time1);
    return;
  }

  if(topicStr == "home/esp32/Ul_light_Time/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    if(requestedState) mqttApplyUlLightMode("timer");
    else if(SetUlLight == "timer") mqttApplyUlLightMode("off");
    publishMqttStateString("home/esp32/SetUlLight", SetUlLight);
    publishMqttStateBool("home/esp32/Pow_Ul_light", Pow_Ul_light || (rs485RelayDataFresh() && ReadRelayArray[15]));
    publishMqttStateBool("home/esp32/Ul_light_Time", Ul_light_Time);
    return;
  }

  if(topicStr == "home/esp32/Activation_Heat/set"){
    bool requestedState = false;
    if(!mqttParseBooleanPayload(message, requestedState)) return;
    Activation_Heat = requestedState; // обновление состояния
    saveValue<int>("Activation_Heat", Activation_Heat ? 1 : 0); // сохранение
    publishMqttStateBool("home/esp32/Activation_Heat", Activation_Heat); // публикация
    return;
  }

  if(topicStr == "home/esp32/SetLamp/set"){
    if(mqttIsAllowedMode(message)){
      mqttApplyLampMode(message);
      publishMqttStateString("home/esp32/SetLamp", SetLamp); // публикация
    }
    return;
  }

  if(topicStr == "home/esp32/SetRGB/set"){
    if(mqttIsAllowedMode(message)){
      mqttApplyRgbMode(message);
      publishMqttStateString("home/esp32/SetRGB", SetRGB); // публикация
      publishMqttStateBool("home/esp32/Pow_WS2815", Pow_WS2815 || (rs485RelayDataFresh() && ReadRelayArray[1]));
      publishMqttStateBool("home/esp32/WS2815_Time1", WS2815_Time1);
    }
    return;
  }

  if(topicStr == "home/esp32/SetUlLight/set"){
    if(mqttIsAllowedMode(message)){
      mqttApplyUlLightMode(message);
      publishMqttStateString("home/esp32/SetUlLight", SetUlLight);
      publishMqttStateBool("home/esp32/Pow_Ul_light", Pow_Ul_light || (rs485RelayDataFresh() && ReadRelayArray[15]));
      publishMqttStateBool("home/esp32/Ul_light_Time", Ul_light_Time);
    }
    return;
  }

  if(topicStr == "home/esp32/SetYardLight/set"){
    if(mqttIsAllowedMode(message)){
      mqttApplyYardLightMode(message);
      publishMqttStateString("home/esp32/SetYardLight", SetYardLight);
      const bool yardLightActual = rs485RelayDataFresh() ? ReadRelayArray[7] : Pow_Yard_light;
      publishMqttStateBool("home/esp32/Pow_Yard_light", yardLightActual);
      publishMqttStateBool("home/esp32/YardLightActual", yardLightActual);
    }
    return;
  }

  if(topicStr == "home/esp32/PH_Lower/set" || topicStr == "home/esp32/PH_Upper/set" ||
     topicStr == "home/esp32/PH_setting/set" || topicStr == "home/esp32/CL_Lower/set" ||
     topicStr == "home/esp32/CL_Upper/set"){
    float parsed = 0.0f;
    if(!mqttParseFiniteFloat(message, parsed)) return;
    (void)parsed;
    const String prefix = "home/esp32/";
    const String settingId = topicStr.substring(prefix.length(), topicStr.length() - 4);
    if(applyChemistrySettingRequest(settingId, message)) mqttPublishChemistryLimits();
    return;
  }

  if(topicStr == "home/esp32/LampLightOffThreshold/set" || topicStr == "home/esp32/LampLightOnThreshold/set" ||
     topicStr == "home/esp32/RgbLightOffThreshold/set" || topicStr == "home/esp32/RgbLightOnThreshold/set" ||
     topicStr == "home/esp32/UlLightOffThreshold/set" || topicStr == "home/esp32/UlLightOnThreshold/set" ||
     topicStr == "home/esp32/YardLightOffThreshold/set" || topicStr == "home/esp32/YardLightOnThreshold/set"){
    int parsed = 0;
    if(!mqttParseInteger(message, parsed)) return;
    (void)parsed;
    const String prefix = "home/esp32/";
    const String settingId = topicStr.substring(prefix.length(), topicStr.length() - 4);
    if(applyLightThresholdSettingRequest(settingId, message)) mqttPublishLightThresholds();
    return;
  }

  if(topicStr == "home/esp32/ClockSelect/set"){
    int offset = 0;
    if(!mqttParseInteger(message, offset) || !isValidGmtOffsetValue(offset)) return;
    applyGmtOffsetFromEsp(offset, true);
    syncNextionRtcFromEpoch(getCurrentEpoch());
    publishMqttStateInt("home/esp32/ClockSelect", gmtOffset_correct);
    return;
  }

    if(topicStr == "home/esp32/LedAutoplay/set"){
    bool autoplayState = LedAutoplay;
    if(mqttParseLedAutoplay(message, autoplayState)){
      LedAutoplay = autoplayState;
      saveValue<int>("LedAutoplay", LedAutoplay ? 1 : 0);
      publishMqttStateString("home/esp32/LedAutoplay", mqttLedAutoplayState());
    }
    return;
  }

  if(topicStr == "home/esp32/button/restart/set"){
    // Не перезагружаемся из MQTT callback. Retained/пустое сообщение брокера
    // иначе создаёт бесконечный цикл connect -> restart. Перезапуск устройства
    // остаётся доступен только через авторизованный Web endpoint /restart.
    Serial.printf("[MQTT] Restart command ignored (payload=%s)\n", message.c_str());
    return;
  }
  
  if(topicStr.startsWith("home/esp32/") && topicStr.endsWith("/set")){
    const String basePrefix = "home/esp32/";
    const size_t prefixLen = basePrefix.length();
    const size_t suffixLen = 4; // "/set"
    if(topicStr.length() > prefixLen + suffixLen){
      String id = topicStr.substring(prefixLen, topicStr.length() - suffixLen);
        if(id == "button1" || id == "button2" || id == "MotorSpeed" ||
         id == "IntInput" || id == "FloatInput" || id == "RangeSlider"){
        return;
      }

            if(id == "Temper_PH" || id == "Temper_Reference" ||
         id == "Float_PH_Slider" || id == "PH2_CAL" ||
         id == "Power_H2O2_Button" || id == "Calibration_ORP_mV" ||
         id == "CalRastvor256mV" || id == "Power_ACO_Button" ||
         id == "ORP_setting"){
        return;
      }

      if(uiApplyValueForId(id, message)){
        publishMqttStateString((basePrefix + id).c_str(), uiValueForId(id));
      }
    }
    return;
  }
}

#if 1 // MQTT Discovery включен
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
  String availabilityTopic = mqttAvailabilityTopicString();
  mqttClient.publish(availabilityTopic.c_str(), payload, retain); // публикация статуса
}

inline void publishDiscoveryDeviceBlock(JsonDocument &doc, const String &deviceId, const String &deviceName){ // блок device
  JsonObject device = doc["device"].to<JsonObject>(); // объект устройства
  JsonArray identifiers = device["identifiers"].to<JsonArray>(); // идентификаторы
  identifiers.add(deviceId); // добавление id
  device["name"] = deviceName; // имя устройства
  device["model"] = "ESP32-S3"; // модель
  device["manufacturer"] = "Espressif"; // производитель
}

inline void publishDiscoveryDeviceBlockMinimal(JsonDocument &doc, const String &deviceId){ // минимальный блок device
  JsonObject device = doc["device"].to<JsonObject>(); // объект устройства
  JsonArray identifiers = device["identifiers"].to<JsonArray>(); // идентификаторы
  identifiers.add(deviceId); // добавление id
}

struct MqttDiscoveryPublishResult {
  bool published;
  size_t payloadLength;
  String topic;
};

inline bool mqttDiscoveryPayloadFits(const String &topic, size_t payloadLength){ // проверка размера
  const size_t bufferSize = mqttClient.getBufferSize();
  const size_t reserved = 12; // запас на заголовки
  if(bufferSize <= reserved) return false;
  return (topic.length() + payloadLength + reserved) <= bufferSize;
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
  const char* payloadPress = nullptr;
  const char* minValue = nullptr;
  const char* maxValue = nullptr;
  const char* stepValue = nullptr;
};

enum MqttDiscoveryGroup {
  DISCOVERY_GROUP_OVERVIEW,
  DISCOVERY_GROUP_CONTROLS,
  DISCOVERY_GROUP_FILTRATION,
  DISCOVERY_GROUP_BACKWASH,
  DISCOVERY_GROUP_LAMP,
  DISCOVERY_GROUP_RGB,
  DISCOVERY_GROUP_WATER_LEVEL,
  DISCOVERY_GROUP_DRAIN,
  DISCOVERY_GROUP_POOL_TEMPERATURE,
  DISCOVERY_GROUP_PH_ACO,
  DISCOVERY_GROUP_CHLORINE_NAOCL,
  DISCOVERY_GROUP_ROOM_TEMPERATURE,
  DISCOVERY_GROUP_OUTDOOR_LIGHTING,
  DISCOVERY_GROUP_TIME,
  DISCOVERY_GROUP_RS485
};

inline MqttDiscoveryGroup mqttDiscoveryGroupForEntityId(const char* id){
  String entityId = id ? String(id) : String();
  entityId.trim();

  if(entityId == "OverlayPoolTemp" || entityId == "OverlayHeaterTemp" ||
     entityId == "OverlayLevelUpper" || entityId == "OverlayLevelLower" ||
     entityId == "OverlayPh" || entityId == "OverlayChlorine" ||
     entityId == "OverlayFilterState" || entityId == "OverlayDrainPit" ||
     entityId == "OverlayLight") return DISCOVERY_GROUP_OVERVIEW;

  if(entityId == "Power_Filtr" || entityId == "Filtr_Time1" ||
     entityId == "Filtr_Time2" || entityId == "Filtr_Time3" ||
     entityId == "FiltrTimer1_ON" || entityId == "FiltrTimer1_OFF" ||
     entityId == "FiltrTimer2_ON" || entityId == "FiltrTimer2_OFF" ||
     entityId == "FiltrTimer3_ON" || entityId == "FiltrTimer3_OFF") return DISCOVERY_GROUP_FILTRATION;

  if(entityId == "Power_Clean" || entityId == "Clean_Time1" || entityId == "Timer1" ||
     entityId == "CleanTimer1_ON" || entityId == "CleanTimer1_OFF" ||
     entityId == "DaysMonToggle" || entityId == "DaysTueToggle" ||
     entityId == "DaysWedToggle" || entityId == "DaysThuToggle" ||
     entityId == "DaysFriToggle" || entityId == "DaysSatToggle" ||
     entityId == "DaysSunToggle" || entityId == "AirPump" ||
     entityId == "SolValveFilBack" || entityId == "SolValveFiltration" ||
     entityId == "SolSandDump" || entityId == "T_air_pump" ||
     entityId == "T_Valve_Switch" || entityId == "T_backwash" ||
     entityId == "T_SolSandDump") return DISCOVERY_GROUP_BACKWASH;

  if(entityId == "InfoString2" || entityId == "SetLamp" || entityId == "LampActual" ||
     entityId == "Lumen_Ul" || entityId == "LampTimer_ON" ||
     entityId == "LampTimer_OFF" || entityId == "LightSensorLiveInfo" ||
     entityId == "LampLightAutoState" || entityId == "LampLightOffThreshold" ||
     entityId == "LampLightOnThreshold") return DISCOVERY_GROUP_LAMP;

  if(entityId == "WS2815_Time1" || entityId == "Pow_WS2815" || entityId == "RgbActual" ||
     entityId == "SetRGB" || entityId == "LEDColor" || entityId == "LedBrightness" ||
    entityId == "LED Brightness" || entityId.equalsIgnoreCase("ledbrightness") ||
     entityId == "LedColorMode" || entityId == "LedPattern" ||
     entityId == "LedAutoplay" || entityId == "LedAutoplayDuration" ||
     entityId == "LedColorOrder" || entityId == "RgbTimer_ON" || entityId == "RgbTimer_OFF" ||
     entityId == "RgbLightAutoState" || entityId == "RgbLightOffThreshold" ||
     entityId == "RgbLightOnThreshold") return DISCOVERY_GROUP_RGB;

  if(entityId == "Activation_Water_Level" || entityId == "WaterLevelSensorUpper" ||
     entityId == "WaterLevelSensorLower" || entityId == "Power_Topping_State" ||
     entityId == "Power_Topping" ||
     entityId == "PoolWaterLevelStageInfo" || entityId == "ToppingWorkMinutes" ||
     entityId == "ToppingPauseMinutes") return DISCOVERY_GROUP_WATER_LEVEL;

  if(entityId == "WaterLevelSensorDrain" || entityId == "Power_Drain_State" ||
     entityId == "Power_Drain" || entityId == "DrainPitStageInfo" ||
     entityId == "DrainWorkMinutes" || entityId == "DrainPauseMinutes" ||
     entityId == "DrainCyclesTarget" || entityId == "DrainCyclesDone") return DISCOVERY_GROUP_DRAIN;

  if(entityId == "DS1" || entityId == "DS2" || entityId == "DS2Available" || entityId == "Sider_heat" ||
     entityId == "Activation_Heat" || entityId == "Power_Heat") return DISCOVERY_GROUP_POOL_TEMPERATURE;

  if(entityId == "PH" || entityId == "PH_Control_ACO" || entityId == "PH_Lower" ||
     entityId == "PH_Upper" || entityId == "PH_setting" || entityId == "analogValuePH" ||
     entityId == "Float_PH_Slider" || entityId == "PH2_CAL" ||
     entityId == "Temper_Reference" || entityId == "Temper_PH" ||
     entityId == "Power_ACO_Button" || entityId == "Power_ACO" ||
     entityId == "ACO_Work") return DISCOVERY_GROUP_PH_ACO;

  if(entityId == "ppmCl" || entityId == "corrected_ORP_Eh_mV" ||
     entityId == "Power_H2O2" || entityId == "CL_Lower" || entityId == "CL_Upper" ||
     entityId == "NaOCl_H2O2_Control" || entityId == "ORP_setting" ||
     entityId == "CalRastvor256mV" || entityId == "Calibration_ORP_mV" ||
     entityId == "Power_H2O2_Button" ||
     entityId == "H2O2_Work") return DISCOVERY_GROUP_CHLORINE_NAOCL;

  if(entityId == "RoomTemp" || entityId == "RoomTempRange" ||
     entityId == "RoomTemper" || entityId == "Power_Warm_floor_heating") return DISCOVERY_GROUP_ROOM_TEMPERATURE;

  if(entityId == "Pow_Ul_light" || entityId == "OutdoorLightActual" || entityId == "SetUlLight" ||
     entityId == "Ul_light_Time" || entityId == "UlLightTimer_ON" || entityId == "UlLightTimer_OFF" ||
     entityId == "UlLightAutoState" || entityId == "UlLightOffThreshold" ||
     entityId == "UlLightOnThreshold" || entityId == "Pow_Yard_light" ||
     entityId == "YardLightActual" || entityId == "SetYardLight" ||
     entityId == "YardLightTimer_ON" || entityId == "YardLightTimer_OFF" ||
     entityId == "YardLightAutoState" || entityId == "YardLightOffThreshold" ||
     entityId == "YardLightOnThreshold") return DISCOVERY_GROUP_OUTDOOR_LIGHTING;

  if(entityId == "CurrentTime" || entityId == "ClockSelect") return DISCOVERY_GROUP_TIME;

  if(entityId == "InfoStringDIN" || entityId.startsWith("Rs485")) return DISCOVERY_GROUP_RS485;

  return DISCOVERY_GROUP_OVERVIEW;
}

inline const char* mqttDiscoveryGroupSuffix(MqttDiscoveryGroup group){
  switch(group){
    case DISCOVERY_GROUP_CONTROLS: return "controls";
    case DISCOVERY_GROUP_FILTRATION: return "filtration";
    case DISCOVERY_GROUP_BACKWASH: return "backwash";
    case DISCOVERY_GROUP_LAMP: return "lamp";
    case DISCOVERY_GROUP_RGB: return "rgb";
    case DISCOVERY_GROUP_WATER_LEVEL: return "water_level";
    case DISCOVERY_GROUP_DRAIN: return "drain";
    case DISCOVERY_GROUP_POOL_TEMPERATURE: return "pool_temperature";
    case DISCOVERY_GROUP_PH_ACO: return "ph_aco";
    case DISCOVERY_GROUP_CHLORINE_NAOCL: return "chlorine_naocl";
    case DISCOVERY_GROUP_ROOM_TEMPERATURE: return "room_temperature";
    case DISCOVERY_GROUP_OUTDOOR_LIGHTING: return "outdoor_lighting";
    case DISCOVERY_GROUP_TIME: return "time";
    case DISCOVERY_GROUP_RS485: return "rs485";
    case DISCOVERY_GROUP_OVERVIEW:
    default: return "overview";
  }
}

inline const char* mqttDiscoveryGroupName(MqttDiscoveryGroup group){
  switch(group){
    case DISCOVERY_GROUP_CONTROLS: return "Controls";
    case DISCOVERY_GROUP_FILTRATION: return "Настройка фильтрации";
    case DISCOVERY_GROUP_BACKWASH: return "Настройка промывки фильтра";
    case DISCOVERY_GROUP_LAMP: return "Управление лампой";
    case DISCOVERY_GROUP_RGB: return "Управление RGB подсветкой";
    case DISCOVERY_GROUP_WATER_LEVEL: return "Контроль уровня воды";
    case DISCOVERY_GROUP_DRAIN: return "Слив воды";
    case DISCOVERY_GROUP_POOL_TEMPERATURE: return "Контроль температуры";
    case DISCOVERY_GROUP_PH_ACO: return "Контроль pH (ACO)";
    case DISCOVERY_GROUP_CHLORINE_NAOCL: return "Контроль хлора CL (NaOCl)";
    case DISCOVERY_GROUP_ROOM_TEMPERATURE: return "Контроль температуры в помещении";
    case DISCOVERY_GROUP_OUTDOOR_LIGHTING: return "Уличное освещение";
    case DISCOVERY_GROUP_TIME: return "Дата и время";
    case DISCOVERY_GROUP_RS485: return "RS485 Modbus I/O";
    case DISCOVERY_GROUP_OVERVIEW:
    default: return "Общая информация по бассейну";
  }
}

inline void publishDiscoveryDeviceBlockGrouped(JsonDocument &doc,
                                               const String &rootDeviceId,
                                               const String &rootDeviceName,
                                               MqttDiscoveryGroup group){
  JsonObject device = doc["device"].to<JsonObject>();
  JsonArray identifiers = device["identifiers"].to<JsonArray>();
  const String groupDeviceId = group == DISCOVERY_GROUP_OVERVIEW
    ? rootDeviceId
    : rootDeviceId + "_" + mqttDiscoveryGroupSuffix(group);
  identifiers.add(groupDeviceId);
  String compactRootName = rootDeviceName;
  if(compactRootName.startsWith("ESP32 ") && compactRootName.length() > 5){
    compactRootName = "ESP32 " + compactRootName.substring(compactRootName.length() - 5);
  }
  device["name"] = compactRootName + " • " + mqttDiscoveryGroupName(group);
  device["model"] = "ESP32-S3";
  device["manufacturer"] = "Espressif";
  if(group != DISCOVERY_GROUP_OVERVIEW) device["via_device"] = rootDeviceId;
}

inline MqttDiscoveryPublishResult publishMqttDiscoveryEntity(const MqttDiscoveryEntity &entity,
                                       const String &deviceId,
                                       const String &deviceName,
                                       bool minimalDevice){ // публикация сущности
                                          (void)minimalDevice;
  JsonDocument doc; // JSON-документ
  const String uniqueId = deviceId + "_" + entity.id; // уникальный id
  const String topic = String(mqttDiscoveryPrefix) + "/" + entity.component + "/" + uniqueId + "/config"; // топик config

  doc["unique_id"] = uniqueId; // unique_id
  doc["name"] = entity.name; // name
  doc["availability_topic"] = mqttAvailabilityTopicString(); // availability_topic
  doc["payload_available"] = "online"; // payload available
  doc["payload_not_available"] = "offline"; // payload not available
  if(entity.stateTopic) doc["state_topic"] = mqttTopicFromLegacy(entity.stateTopic); // state_topic
  if(entity.commandTopic) doc["command_topic"] = mqttTopicFromLegacy(entity.commandTopic); // command_topic
  if(entity.deviceClass) doc["device_class"] = entity.deviceClass; // device_class
  if(entity.unit) doc["unit_of_measurement"] = entity.unit; // unit_of_measurement
  if(entity.stateClass) doc["state_class"] = entity.stateClass; // state_class
  if(entity.valueTemplate) doc["value_template"] = entity.valueTemplate; // value_template
  if(entity.payloadOn) doc["payload_on"] = entity.payloadOn; // payload_on
  if(entity.payloadOff) doc["payload_off"] = entity.payloadOff; // payload_off
  if(entity.payloadPress) doc["payload_press"] = entity.payloadPress; // payload_press
  if(entity.minValue) doc["min"] = atof(entity.minValue); // min (number entities)
  if(entity.maxValue) doc["max"] = atof(entity.maxValue); // max (number entities)
  if(entity.stepValue) doc["step"] = atof(entity.stepValue); // step (number entities)

    publishDiscoveryDeviceBlockGrouped(doc, deviceId, deviceName, mqttDiscoveryGroupForEntityId(entity.id));

  String payload; // строка JSON
  serializeJson(doc, payload); // сериализация JSON
  bool published = mqttClient.publish(topic.c_str(), payload.c_str(), true); // публикация config с retain
  return {published, payload.length(), topic};
}

inline MqttDiscoveryPublishResult publishMqttDiscoverySelect(const char* id,
                                       const char* name,
                                       const char* stateTopic,
                                       const char* commandTopic,
                                       const char* const* options,
                                       size_t optionsCount,
                                       const String &deviceId,
                                       const String &deviceName,
                                       bool minimalDevice,
                                       bool includeOptions){ // публикация select
                                          (void)minimalDevice;
                                          (void)includeOptions;
  JsonDocument doc; // JSON-документ
  const String uniqueId = deviceId + "_" + id; // уникальный id
  const String topic = String(mqttDiscoveryPrefix) + "/select/" + uniqueId + "/config"; // топик config

  doc["unique_id"] = uniqueId; // unique_id
  doc["name"] = name; // name
  doc["availability_topic"] = mqttAvailabilityTopicString(); // availability_topic
  doc["payload_available"] = "online"; // payload available
  doc["payload_not_available"] = "offline"; // payload not available
  doc["state_topic"] = mqttTopicFromLegacy(stateTopic); // state_topic
  doc["command_topic"] = mqttTopicFromLegacy(commandTopic); // command_topic
  JsonArray optionsArray = doc["options"].to<JsonArray>(); // MQTT Select без options невалиден для HA
  for(size_t i = 0; i < optionsCount; ++i){
    optionsArray.add(options[i]);
  }

  publishDiscoveryDeviceBlockGrouped(doc, deviceId, deviceName, mqttDiscoveryGroupForEntityId(id));

  String payload; // строка JSON
  serializeJson(doc, payload); // сериализация JSON
  bool published = mqttClient.publish(topic.c_str(), payload.c_str(), true); // публикация config с retain
  return {published, payload.length(), topic};
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
  
   if(!mqttDiscoveryLegacyCleanupDone){
    struct LegacyEntityConfig { const char* component; const char* id; };
    static const LegacyEntityConfig legacyEntities[] = {
      {"text", "DaysSelect"},
      {"button", "DaysMonToggle"},
      {"button", "DaysTueToggle"},
      {"button", "DaysWedToggle"},
      {"button", "DaysThuToggle"},
      {"button", "DaysFriToggle"},
      {"button", "DaysSatToggle"},
      {"button", "DaysSunToggle"},
      {"sensor", "LedBrightness"},
      {"switch", "SetRGB"},
      {"switch", "SetLamp"},
      {"switch", "SetUlLight"},
      {"sensor", "status"},
      {"sensor", "alive"},
      {"text", "Comment"},
      {"text", "ThemeColor"},
      {"button", "restart"}
    };

    bool cleanupOk = true;
    for(const auto &legacy : legacyEntities){
      const String uniqueId = deviceId + "_" + legacy.id;
      const String topic = String(mqttDiscoveryPrefix) + "/" + legacy.component + "/" + uniqueId + "/config";
      if(!mqttClient.publish(topic.c_str(), "", true)){
        cleanupOk = false;
      }
    }

    if(!cleanupOk) return; // повторим очистку в следующем цикле
    mqttDiscoveryLegacyCleanupDone = true;
  }


  static const MqttDiscoveryEntity baseEntities[] = {
    {"sensor", "OverlayPoolTemp", "Overlay Pool Temp", "home/esp32/OverlayPoolTemp", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}, // первым создаем root device для via_device групп
    {"sensor", "DS1", "Pool Water Temperature", "home/esp32/DS1", nullptr, "temperature", "°C", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "DS2", "Temperature After Heater", "home/esp32/DS2", nullptr, "temperature", "°C", "measurement", nullptr, nullptr, nullptr},
    {"binary_sensor", "DS2Available", "Heater Temperature Sensor Available", "home/esp32/DS2Available", nullptr, "connectivity", nullptr, nullptr, nullptr, "1", "0"},
    {"sensor", "RoomTemp", "Room Temperature", "home/esp32/RoomTemp", nullptr, "temperature", "°C", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "PH", "Pool pH", "home/esp32/PH", nullptr, nullptr, "pH", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "corrected_ORP_Eh_mV", "ORP", "home/esp32/corrected_ORP_Eh_mV", nullptr, nullptr, "mV", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "ppmCl", "Free Chlorine", "home/esp32/ppmCl", nullptr, nullptr, "mg/L", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "OverlayHeaterTemp", "Overlay Heater Temp", "home/esp32/OverlayHeaterTemp", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "OverlayLevelUpper", "Overlay Level Upper", "home/esp32/OverlayLevelUpper", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "OverlayLevelLower", "Overlay Level Lower", "home/esp32/OverlayLevelLower", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "OverlayPh", "Overlay pH", "home/esp32/OverlayPh", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "OverlayChlorine", "Overlay Chlorine", "home/esp32/OverlayChlorine", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "OverlayFilterState", "Filter State", "home/esp32/OverlayFilterState", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "OverlayDrainPit", "Overlay Drain Pit", "home/esp32/OverlayDrainPit", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "OverlayLight", "Overlay Outdoor Light", "home/esp32/OverlayLight", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "InfoString2", "Lamp Info", "home/esp32/InfoString2", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "InfoStringDIN", "DIN State", "home/esp32/InfoStringDIN", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "Lumen_Ul", "Outdoor Luminance", "home/esp32/Lumen_Ul", nullptr, nullptr, "%", "measurement", nullptr, nullptr, nullptr},
    {"sensor", "analogValuePH", "PH ADC Value", "home/esp32/analogValuePH", nullptr, nullptr, nullptr, "measurement", nullptr, nullptr, nullptr},
    {"binary_sensor", "LampActual", "Pool Lamp Actual State", "home/esp32/LampActual", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "RgbActual", "RGB Actual State", "home/esp32/RgbActual", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "OutdoorLightActual", "Outdoor Light Actual State", "home/esp32/OutdoorLightActual", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "YardLightActual", "Courtyard Light Actual State", "home/esp32/YardLightActual", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"sensor", "LightSensorLiveInfo", "Outdoor Light Sensor Status", "home/esp32/LightSensorLiveInfo", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "LampLightAutoState", "Pool Lamp Automation", "home/esp32/LampLightAutoState", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "RgbLightAutoState", "RGB Automation", "home/esp32/RgbLightAutoState", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "UlLightAutoState", "Outdoor Light Automation", "home/esp32/UlLightAutoState", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "YardLightAutoState", "Courtyard Light Automation", "home/esp32/YardLightAutoState", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"binary_sensor", "Power_H2O2", "NaOCl Pump State", "home/esp32/Power_H2O2", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_ACO", "ACO Pump State", "home/esp32/Power_ACO", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_Heat", "Heating State", "home/esp32/Power_Heat", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_Topping_State", "Water Top Up State", "home/esp32/Power_Topping_State", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "WaterLevelSensorUpper", "Water Level Upper", "home/esp32/WaterLevelSensorUpper", nullptr, "moisture", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "WaterLevelSensorLower", "Water Level Lower", "home/esp32/WaterLevelSensorLower", nullptr, "moisture", nullptr, nullptr, nullptr, "1", "0"},
      {"binary_sensor", "WaterLevelSensorDrain", "Drain Pit Level (Input 3)", "home/esp32/WaterLevelSensorDrain", nullptr, "moisture", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_Drain_State", "Water Drain Mode State", "home/esp32/Power_Drain_State", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Power_Warm_floor_heating", "Floor Heating State", "home/esp32/Power_Warm_floor_heating", nullptr, "heat", nullptr, nullptr, nullptr, "1", "0"},
    {"sensor", "PoolWaterLevelStageInfo", "Water Level Automation Stage", "home/esp32/PoolWaterLevelStageInfo", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"number", "ToppingWorkMinutes", "Top Up Work Time", "home/esp32/ToppingWorkMinutes", "home/esp32/ToppingWorkMinutes/set", nullptr, "min", nullptr, nullptr, nullptr, nullptr, nullptr, "5", "60", "1"},
    {"number", "ToppingPauseMinutes", "Top Up Pause Time", "home/esp32/ToppingPauseMinutes", "home/esp32/ToppingPauseMinutes/set", nullptr, "min", nullptr, nullptr, nullptr, nullptr, nullptr, "5", "60", "1"},
    {"sensor", "DrainPitStageInfo", "Drain Automation Stage", "home/esp32/DrainPitStageInfo", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"number", "DrainWorkMinutes", "Drain Portion Work Time", "home/esp32/DrainWorkMinutes", "home/esp32/DrainWorkMinutes/set", nullptr, "min", nullptr, nullptr, nullptr, nullptr, nullptr, "1", "30", "1"},
    {"number", "DrainPauseMinutes", "Drain Portion Pause Time", "home/esp32/DrainPauseMinutes", "home/esp32/DrainPauseMinutes/set", nullptr, "min", nullptr, nullptr, nullptr, nullptr, nullptr, "1", "30", "1"},
    {"number", "DrainCyclesTarget", "Drain Portions Target", "home/esp32/DrainCyclesTarget", "home/esp32/DrainCyclesTarget/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "1", "30", "1"},
    {"sensor", "DrainCyclesDone", "Drain Portions Done", "home/esp32/DrainCyclesDone", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"switch", "Power_Filtr", "01 🧽 Фильтрация (вручную)", "home/esp32/Power_Filtr", "home/esp32/Power_Filtr/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Filtr_Time1", "02 ⏱️ Таймер включения №1", "home/esp32/Filtr_Time1", "home/esp32/Filtr_Time1/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"text", "FiltrTimer1_ON", "03 🟢 Время вкл. по таймеру №1", "home/esp32/FiltrTimer1_ON", "home/esp32/FiltrTimer1_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "FiltrTimer1_OFF", "04 🔴 Время откл. по таймеру №1", "home/esp32/FiltrTimer1_OFF", "home/esp32/FiltrTimer1_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"switch", "Filtr_Time2", "05 ⏱️ Таймер включения №2", "home/esp32/Filtr_Time2", "home/esp32/Filtr_Time2/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"text", "FiltrTimer2_ON", "06 🟢 Время вкл. по таймеру №2", "home/esp32/FiltrTimer2_ON", "home/esp32/FiltrTimer2_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "FiltrTimer2_OFF", "07 🔴 Время откл. по таймеру №2", "home/esp32/FiltrTimer2_OFF", "home/esp32/FiltrTimer2_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"switch", "Filtr_Time3", "08 ⏱️ Таймер включения №3", "home/esp32/Filtr_Time3", "home/esp32/Filtr_Time3/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"text", "FiltrTimer3_ON", "09 🟢 Время вкл. по таймеру №3", "home/esp32/FiltrTimer3_ON", "home/esp32/FiltrTimer3_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "FiltrTimer3_OFF", "10 🔴 Время откл. по таймеру №3", "home/esp32/FiltrTimer3_OFF", "home/esp32/FiltrTimer3_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"switch", "Power_Clean", "🧼 Промывка (ручной)", "home/esp32/Power_Clean", "home/esp32/Power_Clean/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Clean_Time1", "🗓️ Промывка по времени", "home/esp32/Clean_Time1", "home/esp32/Clean_Time1/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "DaysMonToggle", "01 ПН", "home/esp32/DaysMonToggle", "home/esp32/DaysMonToggle/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "DaysTueToggle", "02 ВТ", "home/esp32/DaysTueToggle", "home/esp32/DaysTueToggle/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "DaysWedToggle", "03 СР", "home/esp32/DaysWedToggle", "home/esp32/DaysWedToggle/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "DaysThuToggle", "04 ЧТ", "home/esp32/DaysThuToggle", "home/esp32/DaysThuToggle/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "DaysFriToggle", "05 ПТ", "home/esp32/DaysFriToggle", "home/esp32/DaysFriToggle/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "DaysSatToggle", "06 СБ", "home/esp32/DaysSatToggle", "home/esp32/DaysSatToggle/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "DaysSunToggle", "07 ВС", "home/esp32/DaysSunToggle", "home/esp32/DaysSunToggle/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"text", "Timer1", "⏰ Старт промывки", "home/esp32/Timer1", "home/esp32/Timer1/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "CleanTimer1_ON", "🟢 Промывка ВКЛ", "home/esp32/CleanTimer1_ON", "home/esp32/CleanTimer1_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "CleanTimer1_OFF", "🔴 Промывка ВЫКЛ", "home/esp32/CleanTimer1_OFF", "home/esp32/CleanTimer1_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"binary_sensor", "AirPump", "Backwash Air Compressor", "home/esp32/AirPump", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "SolValveFilBack", "Backwash Valve", "home/esp32/SolValveFilBack", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "SolValveFiltration", "Filtration Valve", "home/esp32/SolValveFiltration", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "SolSandDump", "Sand Dump Valve", "home/esp32/SolSandDump", nullptr, "power", nullptr, nullptr, nullptr, "1", "0"},
    {"number", "T_air_pump", "Air Compressor Time", "home/esp32/T_air_pump", "home/esp32/T_air_pump/set", nullptr, "s", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "120", "1"},
    {"number", "T_Valve_Switch", "Valve Switching Time", "home/esp32/T_Valve_Switch", "home/esp32/T_Valve_Switch/set", nullptr, "s", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "60", "1"},
    {"number", "T_backwash", "Backwash Time", "home/esp32/T_backwash", "home/esp32/T_backwash/set", nullptr, "s", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "300", "1"},
    {"number", "T_SolSandDump", "Sand Dump Time", "home/esp32/T_SolSandDump", "home/esp32/T_SolSandDump/set", nullptr, "s", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "60", "1"},
     {"text", "LampTimer_ON", "02 🟢 Время вкл. лампы по таймеру", "home/esp32/LampTimer_ON", "home/esp32/LampTimer_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "LampTimer_OFF", "03 🔴 Время откл. лампы по таймеру", "home/esp32/LampTimer_OFF", "home/esp32/LampTimer_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"switch", "WS2815_Time1", "01 ⏱️ Активация таймера RGB", "home/esp32/WS2815_Time1", "home/esp32/WS2815_Time1/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"text", "RgbTimer_ON", "02 🟢 Время вкл. RGB по таймеру", "home/esp32/RgbTimer_ON", "home/esp32/RgbTimer_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "RgbTimer_OFF", "03 🔴 Время откл. RGB по таймеру", "home/esp32/RgbTimer_OFF", "home/esp32/RgbTimer_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"switch", "Pow_WS2815", "🌈 RGB лента", "home/esp32/Pow_WS2815", "home/esp32/Pow_WS2815/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Activation_Heat", "🔥 Контроль нагрева", "home/esp32/Activation_Heat", "home/esp32/Activation_Heat/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Activation_Water_Level", "✅ Контроль уровня воды", "home/esp32/Activation_Water_Level", "home/esp32/Activation_Water_Level/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Power_Topping", "🚰 Включить/Отключить соленоид долива воды", "home/esp32/Power_Topping", "home/esp32/Power_Topping/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
       {"switch", "Power_Drain", "🧯 СЛИВ ВОДЫ ИЗ БАССЕЙНИА", "home/esp32/Power_Drain", "home/esp32/Power_Drain/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Pow_Ul_light", "🚏 Свет улицы (ручной)", "home/esp32/Pow_Ul_light", "home/esp32/Pow_Ul_light/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "Ul_light_Time", "01 ⏱️ Активация таймера улицы", "home/esp32/Ul_light_Time", "home/esp32/Ul_light_Time/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"text", "UlLightTimer_ON", "02 🟢 Время вкл. улицы по таймеру", "home/esp32/UlLightTimer_ON", "home/esp32/UlLightTimer_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "UlLightTimer_OFF", "03 🔴 Время откл. улицы по таймеру", "home/esp32/UlLightTimer_OFF", "home/esp32/UlLightTimer_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"switch", "Pow_Yard_light", "🏡 Освещение во дворе (ручной)", "home/esp32/Pow_Yard_light", "home/esp32/Pow_Yard_light/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"text", "YardLightTimer_ON", "🟢 Время вкл. двора по таймеру", "home/esp32/YardLightTimer_ON", "home/esp32/YardLightTimer_ON/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "YardLightTimer_OFF", "🔴 Время откл. двора по таймеру", "home/esp32/YardLightTimer_OFF", "home/esp32/YardLightTimer_OFF/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
     {"switch", "PH_Control_ACO", "🧪 Контроль pH (ACO)", "home/esp32/PH_Control_ACO", "home/esp32/PH_Control_ACO/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "NaOCl_H2O2_Control", "🧪 Контроль хлора (NaOCl)", "home/esp32/NaOCl_H2O2_Control", "home/esp32/NaOCl_H2O2_Control/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
    {"switch", "RoomTemper", "Контроль температуры в помещении", "home/esp32/RoomTemper", "home/esp32/RoomTemper/set", nullptr, nullptr, nullptr, nullptr, "1", "0"},
     {"number", "Sider_heat", "🎯 Уставка нагрева", "home/esp32/Sider_heat", "home/esp32/Sider_heat/set", nullptr, "°C", nullptr, nullptr, nullptr, nullptr, nullptr, "5", "30", "1"},
    {"number", "LampLightOffThreshold", "Pool Lamp Dark ON Threshold", "home/esp32/LampLightOffThreshold", "home/esp32/LampLightOffThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "LampLightOnThreshold", "Pool Lamp Bright OFF Threshold", "home/esp32/LampLightOnThreshold", "home/esp32/LampLightOnThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "RgbLightOffThreshold", "RGB Dark ON Threshold", "home/esp32/RgbLightOffThreshold", "home/esp32/RgbLightOffThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "RgbLightOnThreshold", "RGB Bright OFF Threshold", "home/esp32/RgbLightOnThreshold", "home/esp32/RgbLightOnThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "UlLightOffThreshold", "Outdoor Dark ON Threshold", "home/esp32/UlLightOffThreshold", "home/esp32/UlLightOffThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "UlLightOnThreshold", "Outdoor Bright OFF Threshold", "home/esp32/UlLightOnThreshold", "home/esp32/UlLightOnThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "YardLightOffThreshold", "Courtyard Dark ON Threshold", "home/esp32/YardLightOffThreshold", "home/esp32/YardLightOffThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "YardLightOnThreshold", "Courtyard Bright OFF Threshold", "home/esp32/YardLightOnThreshold", "home/esp32/YardLightOnThreshold/set", nullptr, "%", nullptr, nullptr, nullptr, nullptr, nullptr, "0", "100", "1"},
    {"number", "PH_Lower", "pH Lower Limit", "home/esp32/PH_Lower", "home/esp32/PH_Lower/set", nullptr, "pH", nullptr, nullptr, nullptr, nullptr, nullptr, "6", "9", "0.1"},
    {"number", "PH_Upper", "pH Upper Limit", "home/esp32/PH_Upper", "home/esp32/PH_Upper/set", nullptr, "pH", nullptr, nullptr, nullptr, nullptr, nullptr, "6", "9", "0.1"},
    {"number", "CL_Lower", "Free Chlorine Lower Limit", "home/esp32/CL_Lower", "home/esp32/CL_Lower/set", nullptr, "mg/L", nullptr, nullptr, nullptr, nullptr, nullptr, "0.2", "2.5", "0.1"},
    {"number", "CL_Upper", "Free Chlorine Upper Limit", "home/esp32/CL_Upper", "home/esp32/CL_Upper/set", nullptr, "mg/L", nullptr, nullptr, nullptr, nullptr, nullptr, "0.2", "2.5", "0.1"},
    {"number", "PH_setting", "pH Upper Limit (Legacy)", "home/esp32/PH_setting", "home/esp32/PH_setting/set", nullptr, "pH", nullptr, nullptr, nullptr, nullptr, nullptr, "6", "9", "0.1"},
      {"number", "LedBrightness", "🔆 Яркость", "home/esp32/LedBrightness", "home/esp32/LedBrightness/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, "10", "255", "1"},
    {"number", "LedAutoplayDuration", "⏳ Смена режима (сек)", "home/esp32/LedAutoplayDuration", "home/esp32/LedAutoplayDuration/set", nullptr, "s", nullptr, nullptr, nullptr, nullptr, nullptr, "5", "180", "5"},
    {"text", "LEDColor", "LED Color", "home/esp32/LEDColor", "home/esp32/LEDColor/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"text", "RoomTempRange", "🎚️ Включение/выключение обогрева, °C", "home/esp32/RoomTempRange", "home/esp32/RoomTempRange/set", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"binary_sensor", "Rs485Enabled", "RS485 Communication Enabled", "home/esp32/Rs485Enabled", nullptr, "connectivity", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Rs485RelayDataValid", "RS485 Relay Data Valid", "home/esp32/Rs485RelayDataValid", nullptr, "connectivity", nullptr, nullptr, nullptr, "1", "0"},
    {"binary_sensor", "Rs485InputDataValid", "RS485 Input Data Valid", "home/esp32/Rs485InputDataValid", nullptr, "connectivity", nullptr, nullptr, nullptr, "1", "0"},
    {"sensor", "Rs485BaudRate", "RS485 Active Baud Rate", "home/esp32/Rs485BaudRate", nullptr, nullptr, "baud", nullptr, nullptr, nullptr, nullptr},
    {"sensor", "Rs485UartMode", "RS485 Active UART Mode", "home/esp32/Rs485UartMode", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"sensor", "Rs485SlaveId", "RS485 Active Modbus Slave ID", "home/esp32/Rs485SlaveId", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"number", "Rs485PollIntervalMs", "RS485 Poll Interval", "home/esp32/Rs485PollIntervalMs", "home/esp32/Rs485PollIntervalMs/set", nullptr, "ms", nullptr, nullptr, nullptr, nullptr, nullptr, "200", "5000", "100"},
    {"sensor", "CurrentTime", "Device Local Time", "home/esp32/CurrentTime", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
    {"number", "ClockSelect", "GMT Offset", "home/esp32/ClockSelect", "home/esp32/ClockSelect/set", nullptr, "h", nullptr, nullptr, nullptr, nullptr, nullptr, "-12", "14", "1"},
  };

  static const char* const selectOptions[] = {"off", "on", "auto", "auto_timer_off", "timer"}; // варианты для select
  static const char* const ledColorModeOptions[] = {"auto", "manual"};
  static const char* const ledAutoplayOptions[] = {"Автоматически", "Вручную"};
  static const char* const ledPatternOptions[] = {
    "rainbow", "pulse", "chase", "comet", "color_wipe", "theater_chase", "scanner", "sparkle",
    "twinkle", "confetti", "waves", "breathe", "firefly", "ripple", "dots", "gradient",
    "meteor", "juggle", "aurora", "candy", "twirl", "sparkle_trails", "neon_flow", "calm_sea"
  };
  static const char* const ledColorOrderOptions[] = {"GRB", "RGB", "GBR", "RBG", "BRG", "BGR"};
  static const char* const dosingOptions[] = {"1", "2", "14", "15", "16", "3", "4", "5", "6", "8", "9", "10", "11", "12", "13", "7"};
  const size_t baseCount = sizeof(baseEntities) / sizeof(baseEntities[0]); // количество base сущностей
  const size_t selectCount = 10;
  const size_t rs485EntityCount = 32; // Только безопасное отображение: 16 readback + 16 дискретных входов.
  const size_t totalCount = baseCount + selectCount + rs485EntityCount;

  if(mqttDiscoveryStage == DISCOVERY_NONE){
    mqttDiscoveryStage = DISCOVERY_MAIN_ENTITIES; // старт этапа
  }

if(mqttDiscoveryStage == DISCOVERY_MAIN_ENTITIES){
    uint8_t publishedCount = 0; // опубликовано в этом loop
    while(publishedCount < mqttDiscoveryBatchSize && mqttDiscoveryIndex < totalCount){
      bool published = false;
      MqttDiscoveryPublishResult result = {false, 0, ""};
      if(mqttDiscoveryIndex < baseCount){
        result = publishMqttDiscoveryEntity(baseEntities[mqttDiscoveryIndex], deviceId, deviceName, true);
      } else if(mqttDiscoveryIndex == baseCount){
          result = publishMqttDiscoverySelect("SetLamp", "💡 Режим света", "home/esp32/SetLamp", "home/esp32/SetLamp/set", selectOptions, 5, deviceId, deviceName, true, true);
      } else if(mqttDiscoveryIndex == baseCount + 1){
          result = publishMqttDiscoverySelect("SetRGB", "🎛️ Режим управления RGB подсветкой", "home/esp32/SetRGB", "home/esp32/SetRGB/set", selectOptions, 5, deviceId, deviceName, true, true);
        } else if(mqttDiscoveryIndex == baseCount + 2){
          result = publishMqttDiscoverySelect("SetUlLight", "🚏 Режим уличного освещения", "home/esp32/SetUlLight", "home/esp32/SetUlLight/set", selectOptions, 5, deviceId, deviceName, true, true);
        } else if(mqttDiscoveryIndex == baseCount + 3){
          result = publishMqttDiscoverySelect("SetYardLight", "🏡 Режим освещения во дворе", "home/esp32/SetYardLight", "home/esp32/SetYardLight/set", selectOptions, 5, deviceId, deviceName, true, true);
      } else if(mqttDiscoveryIndex == baseCount + 4){
         result = publishMqttDiscoverySelect("LedColorMode", "🎨 Режим цвета", "home/esp32/LedColorMode", "home/esp32/LedColorMode/set", ledColorModeOptions, 2, deviceId, deviceName, true, true);
      } else if(mqttDiscoveryIndex == baseCount + 5){
          result = publishMqttDiscoverySelect("LedPattern", "✨ Режим подсветки", "home/esp32/LedPattern", "home/esp32/LedPattern/set", ledPatternOptions, 24, deviceId, deviceName, true, true);
      } else if(mqttDiscoveryIndex == baseCount + 6){
         result = publishMqttDiscoverySelect("LedAutoplay", "🔁 Автосмена", "home/esp32/LedAutoplay", "home/esp32/LedAutoplay/set", ledAutoplayOptions, 2, deviceId, deviceName, true, true);
      } else if(mqttDiscoveryIndex == baseCount + 7){
          result = publishMqttDiscoverySelect("LedColorOrder", "🎚️ Порядок цветов ленты", "home/esp32/LedColorOrder", "home/esp32/LedColorOrder/set", ledColorOrderOptions, 6, deviceId, deviceName, true, true);
      } else if(mqttDiscoveryIndex == baseCount + 8){
          result = publishMqttDiscoverySelect("ACO_Work", "⏳ Период дозирования ACO", "home/esp32/ACO_Work", "home/esp32/ACO_Work/set", dosingOptions, 16, deviceId, deviceName, true, true);
      } else if(mqttDiscoveryIndex == baseCount + 9){
         result = publishMqttDiscoverySelect("H2O2_Work", "⏳ Период дозирования NaOCl", "home/esp32/H2O2_Work", "home/esp32/H2O2_Work/set", dosingOptions, 16, deviceId, deviceName, true, true);
      } else {
        const size_t dynamicIndex = mqttDiscoveryIndex - baseCount - selectCount;
        const bool isReadback = dynamicIndex < 16;
        const uint8_t channel = static_cast<uint8_t>(dynamicIndex % 16);
        String id = isReadback ? "Rs485RelayReadback" : "Rs485Input";
        id += String(channel);
        String name = isReadback ? "Relay Readback " : "Digital Input ";
        name += String(channel + 1);
        String stateTopic = "home/esp32/" + id;
        MqttDiscoveryEntity dynamicEntity = {
          "binary_sensor",
          id.c_str(), name.c_str(), stateTopic.c_str(), nullptr,
          isReadback ? "power" : nullptr, nullptr, nullptr, nullptr,
          "1", "0"
        };
        result = publishMqttDiscoveryEntity(dynamicEntity, deviceId, deviceName, true);
      }

 if(!result.published){
        if(mqttDiscoveryRetryIndex != mqttDiscoveryIndex){
          mqttDiscoveryRetryIndex = mqttDiscoveryIndex;
          mqttDiscoveryRetryCount = 0;
        }
        mqttDiscoveryRetryCount++;
        Serial.printf("[MQTT] Discovery publish failed idx=%u retries=%u topic=%s payload=%u buffer=%u\n",
                      static_cast<unsigned int>(mqttDiscoveryIndex),
                      mqttDiscoveryRetryCount,
                      result.topic.c_str(),
                      static_cast<unsigned int>(result.payloadLength),
                      static_cast<unsigned int>(mqttClient.getBufferSize()));
        if(!mqttDiscoveryPayloadFits(result.topic, result.payloadLength)){
          Serial.printf("[MQTT] Discovery payload oversized idx=%u topic=%s payload=%u buffer=%u\n",
                        static_cast<unsigned int>(mqttDiscoveryIndex),
                        result.topic.c_str(),
                        static_cast<unsigned int>(result.payloadLength),
                        static_cast<unsigned int>(mqttClient.getBufferSize()));
        }

        if(mqttDiscoveryRetryCount <= mqttDiscoveryMaxRetries){
          break; // повторим в следующем loop
        }
        Serial.printf("[MQTT] Discovery skip idx=%u topic=%s payload=%u\n",
                      static_cast<unsigned int>(mqttDiscoveryIndex),
                      result.topic.c_str(),
                      static_cast<unsigned int>(result.payloadLength));
        mqttDiscoveryRetryCount = 0;
        mqttDiscoveryIndex++;
        publishedCount++;
        continue;
      }

      mqttDiscoveryLastMaxPayload = max(mqttDiscoveryLastMaxPayload, result.payloadLength);
      mqttDiscoveryIndex++;
      publishedCount++;
    }

    if(mqttDiscoveryIndex >= totalCount){
      mqttDiscoveryStage = DISCOVERY_DONE; // завершено
      mqttDiscoveryPending = false; // сброс ожидания
            Serial.printf("[MQTT] Discovery done. Max payload=%u buffer=%u\n",
                    static_cast<unsigned int>(mqttDiscoveryLastMaxPayload),
                    static_cast<unsigned int>(mqttClient.getBufferSize()));
    }
  }
}
#endif

inline bool publishMqttPayloadIfChanged(const char* topic, const String &payload){ // Отправляем MQTT state только при изменении или полном sync.
  if(!mqttClient.connected()) return false;
  String topicKey = mqttTopicFromLegacy(String(topic));
  auto it = mqttLastPublishedPayloads.find(topicKey);
  if(!mqttForcePublishAllStates && it != mqttLastPublishedPayloads.end() && it->second == payload) return false;
  bool published = mqttClient.publish(topicKey.c_str(), payload.c_str());
  if(published) mqttLastPublishedPayloads[topicKey] = payload;
  return published;
}

inline void publishMqttStateString(const char* topic, const String &value){ // публикация строкового состояния
  publishMqttPayloadIfChanged(topic, value); // повтор одинакового значения не отправляем
}

inline void publishMqttStateBool(const char* topic, bool value){ // публикация bool состояния
  publishMqttPayloadIfChanged(topic, value ? String("1") : String("0")); // публикация 1/0 только при изменении
}

inline void publishMqttStateFloat(const char* topic, float value){ // публикация float состояния
  if(!isfinite(value)){
    publishMqttPayloadIfChanged(topic, String("unknown")); // неисправный датчик не маскируем ложным нулем
    return;
  }
  String payload = String(value); // формирование строки
  publishMqttPayloadIfChanged(topic, payload); // публикация значения только при изменении
}

inline void publishMqttStateInt(const char* topic, int value){ // публикация int состояния
  String payload = String(value); // формирование строки
  publishMqttPayloadIfChanged(topic, payload); // публикация значения только при изменении
}


inline void persistMqttSettings(){ // сохранение настроек MQTT
  mqttApplyBaseTopic(mqttBaseTopic);
  saveValue<String>("mqttHost", mqttHost); // Дублируем MQTT host в NVS, чтобы не терять настройки при проблемах SPIFFS.
  saveValue<int>("mqttPort", mqttPort); // Дублируем порт MQTT в NVS.
  saveValue<String>("mqttUser", mqttUsername); // Дублируем логин MQTT в NVS.
  saveValue<String>("mqttPass", mqttPassword); // Дублируем пароль MQTT в NVS.
  saveValue<String>("mqttBaseTopic", mqttBaseTopic); // Дублируем базовый MQTT топик в NVS.
  saveValue<int>("mqttEnabled", mqttEnabled ? 1 : 0); // Дублируем флаг включения MQTT в NVS.
  return; // NVS is authoritative; never write SPIFFS from an AsyncWebServer callback.
  if(!spiffsMounted) return; // Legacy mqtt.json writer below stays unreachable.
  JsonDocument doc; // JSON-документ
  doc["host"] = mqttHost; // адрес брокера
  doc["port"] = mqttPort; // порт брокера
  doc["user"] = mqttUsername; // пользователь
  doc["pass"] = mqttPassword; // пароль
  doc["baseTopic"] = mqttBaseTopic; // базовый топик
  doc["interval"] = mqttPublishInterval; // интервал
  doc["enabled"] = mqttEnabled; // флаг включения

  File file = SPIFFS.open(mqttConfigPath, FILE_WRITE); // открытие файла
  if(!file) return; // выход если файл не открыт
  serializeJson(doc, file); // запись JSON в файл
  file.close(); // закрытие файла
}

inline void loadMqttSettings(){ // загрузка настроек MQTT
  String nvsHost = loadValue<String>("mqttHost", mqttHost); // NVS считается основным надежным хранилищем.
  uint16_t nvsPort = static_cast<uint16_t>(loadValue<int>("mqttPort", mqttPort));
  String nvsUser = loadValue<String>("mqttUser", mqttUsername);
  String nvsPass = loadValue<String>("mqttPass", mqttPassword);
  String nvsBaseTopic = loadValue<String>("mqttBaseTopic", mqttBaseTopic);
  bool nvsEnabled = loadValue<int>("mqttEnabled", mqttEnabled ? 1 : 0) == 1;

  mqttHost = nvsHost.length() ? nvsHost : mqttHost;
  mqttPort = nvsPort > 0 ? nvsPort : mqttPort;
  mqttUsername = nvsUser;
  mqttPassword = nvsPass;
  mqttApplyBaseTopic(nvsBaseTopic);
  mqttEnabled = nvsEnabled;

  if(spiffsMounted && SPIFFS.exists(mqttConfigPath)){ // SPIFFS используем как совместимый дубль настроек.
    File file = SPIFFS.open(mqttConfigPath, FILE_READ); // открытие файла
    if(file){ // если файл открыт
      JsonDocument doc; // JSON-документ
      DeserializationError err = deserializeJson(doc, file); // парсинг JSON
      if(!err){ // если без ошибок
        String fileHost = doc["host"] | String("");
        if(!mqttHost.length() && fileHost.length()) mqttHost = fileHost; // Не даем пустому файлу затереть NVS.
        mqttPort = static_cast<uint16_t>(doc["port"] | mqttPort); // порт
        mqttUsername = doc["user"] | mqttUsername; // пользователь
        mqttPassword = doc["pass"] | mqttPassword; // пароль
        String fileBaseTopic = doc["baseTopic"] | String("");
        if(fileBaseTopic.length()) mqttApplyBaseTopic(fileBaseTopic);
        mqttPublishInterval = doc["interval"] | mqttPublishInterval; // интервал
        mqttEnabled = doc["enabled"] | mqttEnabled; // флаг
      }
      file.close(); // закрытие файла
    }
  }
  if(!mqttHost.length()) mqttEnabled = false;
  if(mqttPublishInterval < 1000UL) mqttPublishInterval = 1000UL; // поврежденная настройка не должна раскрутить MQTT в tight loop
  if(mqttPublishInterval > 3600000UL) mqttPublishInterval = 3600000UL;
  mqttApplyBaseTopic(mqttBaseTopic);
  persistMqttSettings(); // После чтения синхронизируем оба хранилища.
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
  if(!mqttClient.setBufferSize(4096)){ // увеличиваем буфер для крупных MQTT Discovery payload
    mqttClient.setBufferSize(2048); // fallback, не ниже 2048
  }
  mqttClient.setSocketTimeout(1); // Короткий таймаут: не даем MQTT connect()/loop() надолго занять сетевую задачу.
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

inline bool mqttSubscribeTopic(const char* topic, uint8_t qos = 0){
  String resolvedTopic = mqttTopicFromLegacy(String(topic));
  return mqttClient.subscribe(resolvedTopic.c_str(), qos);
}

inline void mqttSubscribeRequired(const char* topic, uint8_t qos = 0){ // подписки discovery-команд не должны молча теряться
  if(!mqttSubscribeTopic(topic, qos)){
    const String resolvedTopic = mqttTopicFromLegacy(String(topic));
    Serial.printf("[MQTT] Subscribe failed: %s\n", resolvedTopic.c_str());
  }
}

inline void connectMqtt(){ // подключение к MQTT
  if(!mqttEnabled) return; // выход если MQTT выключен
  if(mqttHost.length() == 0) return; // выход если host пустой
  if(WiFi.status() != WL_CONNECTED) return; // выход если нет Wi-Fi
if(!resolveMqttHost()) return; // хост не резолвится
  const unsigned long now = millis(); // текущее время
  if(mqttReconnectBlockedUntil != 0 && static_cast<long>(now - mqttReconnectBlockedUntil) < 0) return; // После отказа брокера ждем backoff-окно.
  if(now - mqttLastConnectAttempt < mqttConnectInterval) return; // защита от частых попыток
  mqttLastConnectAttempt = now; // фиксация попытки подключения

  if(!mqttClient.connected()){ // если не подключены
    String clientId = "espdash-" + WiFi.macAddress(); // уникальный clientId
    const bool useMqttCredentials = mqttUsername.length() > 0 || mqttPassword.length() > 0; // Пустые логин/пароль не отправляем как credentials.
    String availabilityTopic = mqttAvailabilityTopicString();
    bool connected = useMqttCredentials
      ? mqttClient.connect(
          clientId.c_str(),
          mqttUsername.c_str(),
          mqttPassword.c_str(),
          availabilityTopic.c_str(),
          0,
          true,
          "offline")
      : mqttClient.connect(
          clientId.c_str(),
          availabilityTopic.c_str(),
          0,
          true,
          "offline");

    if(connected){ // если подключение успешно
      mqttIsConnected = true; // обновление флага
      mqttConsecutiveFailures = 0; // Успешное подключение сбрасывает счетчик ошибок.
      mqttReconnectBlockedUntil = 0; // Снимаем MQTT backoff после успешного подключения.
      mqttLastConnectState = MQTT_CONNECTED; // Фиксируем успешное состояние клиента.
      publishMqttAvailability("online", true); // публикация доступности
      mqttBeginHomeAssistantReplay(true); // при reconnect сначала обновляем discovery, затем полный state snapshot
#if 1
      publishHomeAssistantDiscovery(); // попытка публикации сразу после подключения
#endif

      mqttSubscribeRequired("homeassistant/status", 0); // HA birth/restart replay
      mqttSubscribeRequired("home/esp32/+/set", 0); // все одноуровневые discovery command topics без дублей
      // MQTT restart намеренно не подписываем: retained command не должен
      // когда-либо перезагружать контроллер при подключении к брокеру.
    
    } else { // если не удалось подключиться
      mqttIsConnected = false; // сброс флага
      mqttLastConnectState = mqttClient.state(); // Запоминаем код отказа PubSubClient для выбора паузы.
      if(mqttConsecutiveFailures < 10) mqttConsecutiveFailures++; // Ограничиваем счетчик, чтобы backoff не переполнился.

      unsigned long retryDelay = mqttConnectInterval; // Базовая пауза между попытками MQTT.
      if(mqttLastConnectState == MQTT_CONNECT_BAD_CREDENTIALS || mqttLastConnectState == MQTT_CONNECT_UNAUTHORIZED){
        retryDelay = 60000UL; // Ошибка логина/пароля: не дергаем брокер и сетевую задачу каждые 5 секунд.
      } else {
        uint8_t shift = mqttConsecutiveFailures > 1 ? mqttConsecutiveFailures - 1 : 0;
        if(shift > 3) shift = 3;
        retryDelay = mqttConnectInterval * (1UL << shift);
        if(retryDelay > 30000UL) retryDelay = 30000UL;
      }
      mqttReconnectBlockedUntil = now + retryDelay; // Следующая попытка будет только после backoff-окна.
      mqttClient.disconnect(); // Закрываем неудачную попытку, чтобы не держать зависший socket.
    }
  }
}

inline void stopMqttService(){ // остановка MQTT
    if(mqttClient.connected()) publishMqttAvailability("offline", true); // публикация offline
  mqttClient.disconnect(); // отключение от брокера
  mqttIsConnected = false; // сброс флага подключения
  mqttLastPublish = 0; // сброс таймера публикаций
  mqttLastPublishedPayloads.clear(); // при смене настроек/топика следующий connect отправит полный снимок заново
  mqttForcePublishAllStates = true; // Следующее подключение снова отправит полный снимок состояний.
  mqttLastConnectAttempt = 0; // сброс таймера подключений
  mqttReconnectBlockedUntil = 0; // При ручном применении настроек снимаем паузу повторного подключения.
  mqttConsecutiveFailures = 0; // Новые настройки должны стартовать без старого счетчика ошибок.
  mqttLastConnectState = MQTT_DISCONNECTED; // Сбрасываем диагностический код MQTT.
  mqttLastResolveAttempt = 0; // сброс таймера резолва

  #if 1 // MQTT Discovery включен
  mqttDiscoveryPending = false; // сброс discovery
  mqttDiscoveryStage = DISCOVERY_NONE; // сброс этапа
  mqttDiscoveryIndex = 0; // сброс индекса
  // mqttDiscoveryLegacyCleanupDone намеренно не сбрасываем: очистка legacy-конфигов нужна один раз за boot.
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

inline volatile bool mqttApplyRequested = false;

inline void requestMqttApply(){
  mqttApplyRequested = true; // HTTP меняет только RAM; сеть применяется из WebMqtt task.
}

inline void handleMqttLoop(){ // основной цикл MQTT
  if(mqttApplyRequested){
    mqttApplyRequested = false;
    applyMqttState();
  }
  if(!mqttClient.connected()) connectMqtt(); // переподключение
  mqttIsConnected = mqttClient.connected(); // обновление флага

  if(!mqttEnabled || mqttHost.length() == 0){ // если MQTT выключен или host пустой
    if(mqttClient.connected()) stopMqttService(); // отключение
    return; // выход
  }

  mqttClient.loop(); // обработка MQTT

  #if 1 // MQTT Discovery включен
    if(mqttDiscoveryPending) publishHomeAssistantDiscovery(); // публикация после первого loop
  #endif

  unsigned long now = millis(); // текущее время
  if(!mqttDiscoveryPending && (mqttForcePublishAllStates || now - mqttLastPublish >= mqttPublishInterval)){ // state replay идет только после discovery
    mqttLastPublish = now; // обновление времени
    if(mqttClient.connected()){ // если подключены
      publishMqttStateFloat("home/esp32/DS1", DS1);
      publishMqttStateString("home/esp32/DS2", DS2Available ? String(DS2) : String("unknown"));
      publishMqttStateBool("home/esp32/DS2Available", DS2Available);
      publishMqttStateFloat("home/esp32/RoomTemp", DS1); // RoomTemp использует DS1 в UI
      publishMqttStateFloat("home/esp32/PH", PH);
      publishMqttStateInt("home/esp32/corrected_ORP_Eh_mV", corrected_ORP_Eh_mV);
      publishMqttStateFloat("home/esp32/ppmCl", ppmCl);
      publishMqttStateString("home/esp32/OverlayPoolTemp", OverlayPoolTemp);
      publishMqttStateString("home/esp32/OverlayHeaterTemp", OverlayHeaterTemp);
      publishMqttStateString("home/esp32/OverlayLevelUpper", OverlayLevelUpper);
      publishMqttStateString("home/esp32/OverlayLevelLower", OverlayLevelLower);
      publishMqttStateString("home/esp32/OverlayPh", OverlayPh);
      publishMqttStateString("home/esp32/OverlayChlorine", OverlayChlorine);
      publishMqttStateString("home/esp32/OverlayFilterState", OverlayFilterState);
      publishMqttStateString("home/esp32/OverlayDrainPit", OverlayDrainPit);
      publishMqttStateString("home/esp32/OverlayLight", OverlayLight);
      publishMqttStateString("home/esp32/InfoString2", InfoString2);
      publishMqttStateString("home/esp32/InfoStringDIN", InfoStringDIN);
      publishMqttStateInt("home/esp32/Lumen_Ul", Lumen_Ul);
      publishMqttStateInt("home/esp32/analogValuePH", analogValuePH_Comp);

      const bool rs485RelayFresh = rs485RelayDataFresh();
      const bool rs485InputFresh = rs485InputDataFresh();
      publishMqttStateBool("home/esp32/LampActual", Lamp || (rs485RelayFresh && ReadRelayArray[0]));
      publishMqttStateBool("home/esp32/RgbActual", Pow_WS2815 || (rs485RelayFresh && ReadRelayArray[1]));
      publishMqttStateBool("home/esp32/OutdoorLightActual", Pow_Ul_light || (rs485RelayFresh && ReadRelayArray[15]));
      const bool yardLightActual = rs485RelayFresh ? ReadRelayArray[7] : Pow_Yard_light;
      publishMqttStateBool("home/esp32/YardLightActual", yardLightActual);
      publishMqttStateString("home/esp32/LightSensorLiveInfo", uiValueForId("LightSensorLiveInfo"));
      publishMqttStateString("home/esp32/LampLightAutoState", uiValueForId("LampLightAutoState"));
      publishMqttStateString("home/esp32/RgbLightAutoState", uiValueForId("RgbLightAutoState"));
      publishMqttStateString("home/esp32/UlLightAutoState", uiValueForId("UlLightAutoState"));
      publishMqttStateString("home/esp32/YardLightAutoState", uiValueForId("YardLightAutoState"));

      publishMqttStateBool("home/esp32/Rs485Enabled", Rs485Enabled);
      publishMqttStateInt("home/esp32/Rs485BaudRate", Rs485ActiveBaudRate);
      publishMqttStateString("home/esp32/Rs485UartMode", Rs485ActiveUartMode);
      publishMqttStateInt("home/esp32/Rs485SlaveId", Rs485ActiveSlaveId);
      publishMqttStateInt("home/esp32/Rs485PollIntervalMs", Rs485PollIntervalMs);
      publishMqttStateBool("home/esp32/Rs485RelayDataValid", rs485RelayFresh);
      publishMqttStateBool("home/esp32/Rs485InputDataValid", rs485InputFresh);
      for(uint8_t channel = 0; channel < 16; ++channel){
        const String readbackTopic = "home/esp32/Rs485RelayReadback" + String(channel);
        const String inputTopic = "home/esp32/Rs485Input" + String(channel);
        if(rs485RelayFresh) publishMqttStateBool(readbackTopic.c_str(), ReadRelayArray[channel]);
        else publishMqttStateString(readbackTopic.c_str(), "unknown");
        if(rs485InputFresh) publishMqttStateBool(inputTopic.c_str(), ReadInputArray[channel]);
        else publishMqttStateString(inputTopic.c_str(), "unknown");
      }

      publishMqttStateBool("home/esp32/Power_H2O2", Power_H2O2 || ManualPulse_H2O2_Active || (rs485RelayFresh && ReadRelayArray[5]));
      publishMqttStateBool("home/esp32/Power_ACO", Power_ACO || ManualPulse_ACO_Active || (rs485RelayFresh && ReadRelayArray[6]));
      publishMqttStateBool("home/esp32/Power_Heat", Power_Heat || (rs485RelayFresh && ReadRelayArray[4]));
      publishMqttStateBool("home/esp32/Power_Topping_State", Power_Topping_State || (rs485RelayFresh && ReadRelayArray[13]));
      publishMqttStateBool("home/esp32/WaterLevelSensorUpper", PoolUpperLevelReachedConfirmed);
      publishMqttStateBool("home/esp32/WaterLevelSensorLower", PoolLowerLevelLowConfirmed);
      publishMqttStateBool("home/esp32/WaterLevelSensorDrain", DrainPitFullConfirmed);
      publishMqttStateBool("home/esp32/Power_Drain_State", Power_Drain_State);
      publishMqttStateBool("home/esp32/Power_Warm_floor_heating", Power_Warm_floor_heating || (rs485RelayFresh && ReadRelayArray[14]));

      publishMqttStateString("home/esp32/PoolWaterLevelStageInfo", buildPoolWaterLevelStageInfo());
      publishMqttStateInt("home/esp32/ToppingWorkMinutes", ToppingWorkMinutes);
      publishMqttStateInt("home/esp32/ToppingPauseMinutes", ToppingPauseMinutes);
      publishMqttStateString("home/esp32/DrainPitStageInfo", buildDrainPitStageInfo());
      publishMqttStateInt("home/esp32/DrainWorkMinutes", DrainWorkMinutes);
      publishMqttStateInt("home/esp32/DrainPauseMinutes", DrainPauseMinutes);
      publishMqttStateInt("home/esp32/DrainCyclesTarget", DrainCyclesTarget);
      publishMqttStateInt("home/esp32/DrainCyclesDone", DrainCyclesDone);

      publishMqttStateBool("home/esp32/Power_Filtr", Power_Filtr || (rs485RelayFresh && ReadRelayArray[8]));
      publishMqttStateBool("home/esp32/Filtr_Time1", Filtr_Time1);
      publishMqttStateBool("home/esp32/Filtr_Time2", Filtr_Time2);
      publishMqttStateBool("home/esp32/Filtr_Time3", Filtr_Time3);
      publishMqttStateBool("home/esp32/Power_Clean", Power_Clean || CleanSequenceActive);
      publishMqttStateBool("home/esp32/Clean_Time1", Clean_Time1);
      publishMqttStateBool("home/esp32/AirPump", AirPump || AirPumpAuto || (rs485RelayFresh && ReadRelayArray[9]));
      publishMqttStateBool("home/esp32/SolValveFilBack", SolValveFilBack || ValveBackwashAuto || (rs485RelayFresh && ReadRelayArray[11]));
      publishMqttStateBool("home/esp32/SolValveFiltration", SolValveFiltration || ValveFiltrationAuto || (rs485RelayFresh && ReadRelayArray[10]));
      publishMqttStateBool("home/esp32/SolSandDump", SolSandDump || SolSandDumpAuto || (rs485RelayFresh && ReadRelayArray[12]));
      publishMqttStateInt("home/esp32/T_air_pump", TimerAirSetting);
      publishMqttStateInt("home/esp32/T_Valve_Switch", TimerValveSetting);
      publishMqttStateInt("home/esp32/T_backwash", TimerBackwashSetting);
      publishMqttStateInt("home/esp32/T_SolSandDump", TimerSolSandDump);
      publishMqttStateBool("home/esp32/Pow_WS2815", Pow_WS2815 || (rs485RelayFresh && ReadRelayArray[1]));
      publishMqttStateBool("home/esp32/WS2815_Time1", WS2815_Time1);
      publishMqttStateBool("home/esp32/Activation_Water_Level", Activation_Water_Level);
      publishMqttStateBool("home/esp32/Power_Topping", Power_Topping);
      publishMqttStateBool("home/esp32/Power_Drain", Power_Drain);
      publishMqttStateBool("home/esp32/PH_Control_ACO", PH_Control_ACO);
      publishMqttStateBool("home/esp32/NaOCl_H2O2_Control", NaOCl_H2O2_Control);
      publishMqttStateBool("home/esp32/RoomTemper", RoomTemper);
      publishMqttStateBool("home/esp32/Ul_light_Time", Ul_light_Time);
      publishMqttStateString("home/esp32/RoomTempRange", String(RoomTempOn, 1) + "-" + String(RoomTempOff, 1));
      publishMqttStateString("home/esp32/Timer1", Timer1);
      publishMqttStateString("home/esp32/LEDColor", LEDColor);
      publishMqttStateString("home/esp32/LedColorMode", LedColorMode);
      publishMqttStateInt("home/esp32/LedBrightness", LedBrightness);
      publishMqttStateString("home/esp32/LedPattern", LedPattern);
      publishMqttStateInt("home/esp32/LedAutoplayDuration", LedAutoplayDuration);
      publishMqttStateString("home/esp32/LedAutoplay", mqttLedAutoplayState());
      publishMqttStateString("home/esp32/LedColorOrder", LedColorOrder);
      publishMqttStateInt("home/esp32/Sider_heat", Sider_heat);
      mqttPublishChemistryLimits();
      mqttPublishLightThresholds();
      publishMqttStateInt("home/esp32/ACO_Work", ACO_Work);
      publishMqttStateInt("home/esp32/H2O2_Work", H2O2_Work);
      publishMqttStateString("home/esp32/DaysSelect", DaysSelect);
      publishMqttStateBool("home/esp32/DaysMonToggle", mqttDaysSelectContains("Mon"));
      publishMqttStateBool("home/esp32/DaysTueToggle", mqttDaysSelectContains("Tue"));
      publishMqttStateBool("home/esp32/DaysWedToggle", mqttDaysSelectContains("Wed"));
      publishMqttStateBool("home/esp32/DaysThuToggle", mqttDaysSelectContains("Thu"));
      publishMqttStateBool("home/esp32/DaysFriToggle", mqttDaysSelectContains("Fri"));
      publishMqttStateBool("home/esp32/DaysSatToggle", mqttDaysSelectContains("Sat"));
      publishMqttStateBool("home/esp32/DaysSunToggle", mqttDaysSelectContains("Sun"));
      publishMqttStateString("home/esp32/FiltrTimer1_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer1")));
      publishMqttStateString("home/esp32/FiltrTimer1_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer1")));
      publishMqttStateString("home/esp32/FiltrTimer2_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer2")));
      publishMqttStateString("home/esp32/FiltrTimer2_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer2")));
      publishMqttStateString("home/esp32/FiltrTimer3_ON", formatMinutesToTime(mqttTimerOnMinutes("FiltrTimer3")));
      publishMqttStateString("home/esp32/FiltrTimer3_OFF", formatMinutesToTime(mqttTimerOffMinutes("FiltrTimer3")));
      publishMqttStateString("home/esp32/CleanTimer1_ON", formatMinutesToTime(mqttTimerOnMinutes("CleanTimer1")));
      publishMqttStateString("home/esp32/CleanTimer1_OFF", formatMinutesToTime(mqttTimerOffMinutes("CleanTimer1")));
      publishMqttStateString("home/esp32/LampTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("LampTimer")));
      publishMqttStateString("home/esp32/LampTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("LampTimer")));
      publishMqttStateString("home/esp32/RgbTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("RgbTimer")));
      publishMqttStateString("home/esp32/RgbTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("RgbTimer")));
      publishMqttStateString("home/esp32/UlLightTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("UlLightTimer")));
      publishMqttStateString("home/esp32/UlLightTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("UlLightTimer")));
      publishMqttStateBool("home/esp32/Pow_Ul_light", Pow_Ul_light || (rs485RelayFresh && ReadRelayArray[15]));
      publishMqttStateString("home/esp32/YardLightTimer_ON", formatMinutesToTime(mqttTimerOnMinutes("YardLightTimer")));
      publishMqttStateString("home/esp32/YardLightTimer_OFF", formatMinutesToTime(mqttTimerOffMinutes("YardLightTimer")));
      publishMqttStateBool("home/esp32/Pow_Yard_light", yardLightActual);
      publishMqttStateBool("home/esp32/Activation_Heat", Activation_Heat);

      publishMqttStateString("home/esp32/SetLamp", SetLamp);
      publishMqttStateString("home/esp32/SetRGB", SetRGB);
      publishMqttStateString("home/esp32/SetUlLight", SetUlLight);
      publishMqttStateString("home/esp32/SetYardLight", SetYardLight);
      publishMqttStateString("home/esp32/CurrentTime", CurrentTime);
      publishMqttStateInt("home/esp32/ClockSelect", gmtOffset_correct);
      mqttForcePublishAllStates = false; // Полный снимок отправлен, дальше state-публикации идут только при изменении.
    
    }
  }
}
