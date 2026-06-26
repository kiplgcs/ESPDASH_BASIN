// ------------------- web.h - веб-интерфейс и HTTP -------------------
#pragma once                    // Защищает от повторного включения этого заголовочного файла

#include <Arduino.h>             // Основная библиотека Arduino для ESP32
#include <AsyncTCP.h>            // Асинхронный TCP для ESP32 (не блокирующий)
#include <ESPAsyncWebServer.h>   // Асинхронный веб-сервер для ESP32
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <vector>                // STL вектор для хранения элементов UI
#include <functional>
#include <map>                   // контейнер для провайдеров значений UI
#include <memory>                // shared_ptr для безопасной жизни HTML-буфера в chunk-callback
#include <esp_chip_info.h>
#include <esp_efuse.h>
#ifdef ARDUINO_ARCH_ESP32
#include <esp_partition.h>
#include <esp_ota_ops.h>
#endif
#include "graph.h"               // Пользовательские графики (кастомные)
#include "fs_utils.h"            // Вспомогательные функции для работы с файловой системой
#include "wifi_manager.h"                // Логика Wi-Fi и хранение параметров
#include "NPT_Time.h"            // Настройка времени и часового пояса
#include <ArduinoJson.h>

using std::vector;              // Используем vector без указания std:: каждый раз

constexpr bool kWebVerboseSerial = false; // Подробный WEB-лог выключен, чтобы Serial не тормозил загрузку сайта.

inline const char* webResetReasonToText(esp_reset_reason_t reason){ // Переводим код причины ресета в понятный текст для Serial-диагностики
  switch(reason){ // Нормализуем все важные типы перезапуска, чтобы быстро видеть источник проблемы
    case ESP_RST_POWERON: return "Power On"; // Питание подано заново (обычный старт)
    case ESP_RST_EXT: return "External Reset"; // Внешний reset-пин или внешний источник сброса
    case ESP_RST_SW: return "Software Reset"; // Программный перезапуск из кода
    case ESP_RST_PANIC: return "Panic"; // Авария/исключение ядра (panic)
    case ESP_RST_INT_WDT: return "Interrupt Watchdog"; // Сработал watchdog прерываний
    case ESP_RST_TASK_WDT: return "Task Watchdog"; // Сработал task watchdog (наш ключевой симптом)
    case ESP_RST_WDT: return "Other Watchdog"; // Иной watchdog-ресет (не task/int)
    case ESP_RST_DEEPSLEEP: return "Deep Sleep"; // Пробуждение/рестарт после deep sleep
    case ESP_RST_BROWNOUT: return "Brownout"; // Просадка питания (brownout)
    case ESP_RST_SDIO: return "SDIO"; // Редкий reset по SDIO-подсистеме
    default: return "Unknown"; // Неизвестный код, чтобы не терять диагностику
  }
}

inline void logWebHeapStats(const char* stage){ // Унифицированный лог heap по этапам GET / для поиска OOM/фрагментации
  if(!kWebVerboseSerial) return; // В обычном режиме не печатаем heap, чтобы не замедлять отдачу сайта.
  const size_t free8 = heap_caps_get_free_size(MALLOC_CAP_8BIT); // Доступная 8-битная куча (основной индикатор свободной RAM)
  const size_t largest8 = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT); // Самый крупный непрерывный блок (критичен для reserve)
  const uint32_t fragPercent = free8 ? static_cast<uint32_t>(100U - ((largest8 * 100U) / free8)) : 100U; // Оценка фрагментации: чем выше, тем хуже крупные аллокации
  Serial.printf("[WEB][HEAP] %s | free8=%u largest8=%u frag~=%u%%\n", stage, // Печатаем метрики в привязке к этапу, чтобы видеть где деградирует память
                static_cast<unsigned>(free8), static_cast<unsigned>(largest8), static_cast<unsigned>(fragPercent)); // Явные uint для корректного форматирования printf
}




inline std::map<String, std::function<String()>> uiValueProviders; // Простая карта пользовательских генераторов значений UI

inline void registerUiValueProvider(const String &id, const std::function<String()> &provider){ // Регистрация генератора значения по id
  if(!id.length() || !provider) return; // Проверка на валидность
  uiValueProviders[id] = provider; // Сохраняем генератор значения для последующего переопределения
}

inline String resolveUiValueOverride(const String &id, const String &fallback){ // Получение значения через переопределение
  auto it = uiValueProviders.find(id); // Ищем пользовательский генератор по идентификатору
  if(it != uiValueProviders.end() && it->second){ // Если есть генератор
    return it->second(); // Возвращаем результат генератора
  }
  return fallback; // Иначе возвращаем исходное значение по умолчанию
}

// ---------- Создание веб-сервера ----------
inline AsyncWebServer server(80);  // Создаем веб-сервер на порту 80

// ---------- Глобальные переменные для UI ----------
inline String ThemeColor = "#1e1e1e"; // Цвет темы интерфейса
inline String ColorLED = "#00ff00"; // Выбор цвета для лентиы WS2815
inline String LEDColor = "#00ff00"; // Цвет LED-индикатора
inline int MotorSpeedSetting = 25; // Настройка скорости мотора (0-100)
inline int IntInput = 10; // Целочисленный ввод от пользователя
inline float FloatInput = 3.14f; // Ввод с плавающей точкой
inline float Speed;              // Скорость (например, датчика)
inline float Temperatura;        // Температура
inline String Timer1 = "12:00"; // Таймер 1
inline String Comment = "Hello!"; // Текстовый комментарий
inline String CommentClean; // Этап промывки и оставшееся время
inline String CurrentTime;       // Текущее время
inline int RandomVal;            // Случайное значение (например, для демонстрации)
inline String InfoString;        // Информационная строка
inline String InfoString1;       // Вспомогательная информационная строка
inline String InfoString2;
inline String InfoStringRS485Model; // Название модели платы RS485
inline String InfoStringDIN;     // Состояние входов DIN

inline String Ds18HelpText = "Шаг 1: нажмите поиск. Шаг 2: выберите индекс и назначьте датчики для бассейна/нагревателя. Для отвязки выберите \"❌ Отвязать датчик\"."; // Подсказка по правильной последовательности действий для DS18B20.
inline String Ds18ScanInfo = "Нажмите кнопку \"🔍 Поиск датчиков на шине\", чтобы получить список адресов DS18B20."; // Статус ручного поиска и назначения датчиков DS18B20 в popup.

inline int Ds18ScanButton = 0;   // Флаг кнопки ручного поиска датчиков DS18B20.
inline int Ds18Sensor0Index = -2; // Выбранный индекс датчика для температуры бассейна: -2 не применять, -1 отвязать.
inline int Ds18Sensor1Index = -2; // Выбранный индекс датчика для температуры после нагревателя: -2 не применять, -1 отвязать.
inline String Ds18Sensor0Address; // Текстовый адрес, назначенный на sensor0 (температура бассейна).
inline String Ds18Sensor1Address; // Текстовый адрес, назначенный на sensor1 (температура после нагревателя).


inline String OverlayPoolTemp;   // Температура воды в бассейне (оверлей)
inline String OverlayHeaterTemp; // Температура после нагревателя (оверлей)
inline String OverlayLevelUpper; // Верхний датчик уровня (оверлей)
inline String OverlayLevelLower; // Нижний датчик уровня (оверлей)
inline String OverlayPh;         // pH воды (оверлей)
inline String OverlayChlorine;   // Хлор (оверлей)
inline String OverlayFilterState; // Состояние фильтра (оверлей)
inline String OverlayDrainPit;   // Состояние сливной ямы (оверлей)
inline String OverlayLight;      // Освещенность на улице (оверлей)
inline String ModeSelect = "Normal"; // Режим работы (например, Auto/Manual)
inline String DaysSelect = "Sun"; // Выбор дней недели: по умолчанию промывка раз в неделю в воскресенье
inline String SetLamp = "timer"; // Режим работы лампы: по умолчанию включение по таймеру
inline String SetRGB = "timer"; // Режим управления RGB подсветкой: по умолчанию включение по таймеру
inline String StoredAPSSID;      // Сохраненный SSID точки доступа
inline String StoredAPPASS;      // Сохраненный пароль точки доступа
inline String authUsername;      // Логин для доступа к веб-интерфейсу
inline String authPassword;      // Пароль для доступа к веб-интерфейсу
inline String adminUsername;     // Логин администратора для всплывающих окон
inline String adminPassword;     // Пароль администратора для всплывающих окон
inline int button1 = 0;          // Состояние кнопки 1
inline int button2 = 0;          // Состояние кнопки 2
inline int RangeMin = 10;        // Минимальное значение диапазонного слайдера
inline int RangeMax = 40;        // Максимальное значение диапазонного слайдера
inline int jpg = 1;              // Флаг JPG отображения (например, переключение картинок)
inline String dashAppTitle = "MiniDash"; // Название приложения
inline bool dashInterfaceInitialized = false; // Флаг, что интерфейс уже инициализирован

extern float DS1;
extern float DS2;
extern bool DS1Available;
extern bool DS2Available;

extern bool DS1Assigned; // Признак, что датчик бассейна привязан.
extern bool DS2Assigned; // Признак, что датчик после нагревателя привязан.

extern bool ReadRelayArray[16]; // Readback состояний Modbus-реле (объявление, реализация в ModbusRTU_RS485.h)
extern bool ReadInputArray[16]; // Readback дискретных входов Modbus-реле (объявление, реализация в ModbusRTU_RS485.h)
extern bool AktualReadRelay;
extern bool AktualReadInput;
extern bool Pow_WS2815;
extern bool Pow_WS2815_autosvet;
extern bool WS2815_Time1;
extern bool Power_Filtr;
extern bool AirPump;
extern bool SolValveFilBack;
extern bool SolValveFiltration;
extern bool SolSandDump;
extern bool AirPumpAuto;
extern bool SolSandDumpAuto;
extern bool ValveBackwashAuto;
extern bool ValveFiltrationAuto;
extern bool CleanSequenceActive;

inline bool Rs485Enabled = true;
inline int Rs485BaudRate = 19200;
inline String Rs485UartMode = "8N1";
inline int Rs485SlaveId = 1;
inline int Rs485PollIntervalMs = 1000;
inline bool Rs485ForcePoll = false;
inline bool Rs485ManualRelayState[16] = {false};


String formatTemperatureString(float value, bool available);

void onDs18Sensor0Select(const int &value); // Callback назначения найденного индекса на sensor0.
void onDs18Sensor1Select(const int &value); // Callback назначения найденного индекса на sensor1.



void appendUiRegistryValues(JsonDocument &doc);



bool Slep = false; //Флаг режима сна
int Lumen_Ul, Saved_Lumen_Ul; //освещенность на улице
int Lumen_Ul_Raw; // Сырое значение АЦП GPIO3 для диагностики датчика освещенности.
//int Lumen_Ul_percent; //Освещенность в процентах

float PH, Saved_PH; //Кислотность воды
float PH1 = 4.1f, PH2 = 6.86f;          //Точки калибровки PH
float PH1_CAL = 3500.0f, PH2_CAL = 2900.0f; //Значения для данных точек калибровки PH от 0 до 4095 (12бит)
float Temper_PH; //Измеренная тепература для компенасации измерения PH
float Temper_Reference = 20.0f; // Температура при котором сенсор выдает свои параметры - 20.0 или 25.0 С для разных сенсоров PH;
bool Act_PH = false; //Активация калибровки
bool Act_Cl = false; //Активация калибровки
int analogValuePH_Comp; //корректированное значение АЦП после компенсации по температуре

int Saved_gmtOffset_correct, gmtOffset_correct; // Корректировка часового пояса призапросен времени из Интернета

//bool RestartESP32 = false; //флаг перезагрузки ESP32 по нажатию кнопки
bool CalCl = false; //флаг кнопки для запоминания значения калибровки по ОВП
unsigned long Timer_Callback; // Таймер опроса всех кнопок

/************************* Переменные для записи значений полученных по MQTT*******/

bool Lamp , Lamp1;		// Подсветка в бассейне -  Включение в ручную
bool Lamp_autosvet , Saved_Lamp_autosvet;
bool Power_Time1, Saved_Power_Time1;
uint16_t Saved_Lamp_timeON1, Saved_Lamp_timeOFF1;


bool Power_Heat, Power_Heat1;
bool Activation_Heat = false, Activation_Heat1; // Включение и Активация контроля включения нагрева воды
int Sider_heat,  Sider_heat1; 			// Sider_heat1; bool Sider_Heat; // Переменная для получения или передачи из в  Nextion c  сидера монитора уставки нагрева воды

bool RoomTemper = false;
float RoomTempOn = 3.0f;
float RoomTempOff = 4.0f;
bool Power_Warm_floor_heating = false;
	
bool Pow_Ul_light, Pow_Ul_light1; // Промывка фильтра
bool Ul_light_Time = false, Saved_Ul_light_Time; // Разрешения работы включения по времени
String Ul_light_timeON, Ul_light_timeOFF; // Утавки времени включения


bool Activation_Water_Level = true;
bool WaterLevelSensorUpper = false; // Сырой вход DI2: 1 = контакт верхнего уровнемера замкнут при падении уровня ниже датчика.
bool WaterLevelSensorLower = false; // Сырой вход DI1: 1 = контакт нижнего уровнемера замкнут при падении уровня ниже датчика.
bool WaterLevelSensorDrain = false; // Сырой вход DI3: 1 = контакт уровнемера ямы замкнут при уровне ниже датчика.
bool PoolLowerLevelLowConfirmed = false; // Подтверждено 10 замерами: уровень бассейна ниже нижнего датчика.
bool PoolUpperLevelReachedConfirmed = false; // Подтверждено 3 замерами: вода дошла до верхнего датчика бассейна.
bool DrainPitFullConfirmed = false; // Семантический статус: сливная яма заполнена до верхнего датчика.
uint8_t PoolLowerLevelLowSamples = 0; // Счетчик антидребезга нижнего датчика бассейна.
uint8_t PoolUpperLevelReachedSamples = 0; // Счетчик антидребезга верхнего датчика бассейна.
bool Power_Topping = false, Power_Topping1 = false; // Долив воды по уровню
bool Power_Topping_State = false;
bool Power_Drain = false;
bool Power_Drain_State = false;
bool DrainRestoreFiltrationState = false; // Состояние насоса до запуска режима слива
bool DrainModeLatched = false; // Признак активного режима слива для корректного восстановления насоса
bool DrainCycleRunning = false; // true только во время активной порции слива
bool DrainPauseRunning = false; // true во время паузы между порциями слива
unsigned long DrainCycleStartedAt = 0; // Время старта текущей порции слива
unsigned long DrainPauseStartedAt = 0; // Время старта паузы после порции слива
int DrainWorkMinutes = 15; // Длительность одной порции слива, минут
int DrainPauseMinutes = 15; // Пауза между порциями слива, минут
int DrainCyclesTarget = 10; // Сколько порций слива выполнить до автоматического отключения
int DrainCyclesDone = 0; // Сколько порций слива уже выполнено в текущем запуске
bool ToppingCycleRunning = false; // true только во время активного открытия клапана долива
bool ToppingPauseRunning = false; // true во время паузы после долива
bool ToppingFillToUpperActive = false; // Цикл долива до верхнего уровня после подтвержденного нижнего датчика.
unsigned long ToppingCycleStartedAt = 0; // Время старта текущего долива
unsigned long ToppingPauseStartedAt = 0; // Время старта паузы после долива
int ToppingWorkMinutes = 10; // Длительность одной порции долива, минут
int ToppingPauseMinutes = 60; // Пауза между порциями долива, минут

bool Saved_Power_H2O, Power_H2O2 = false; //Дозация перекеси водорода
bool Saved_Power_ACO, Power_ACO = false; 	//Дозация Активное Каталитическое Окисление «Active Catalytic Oxidation» ACO
bool ManualPulse_H2O2_Active = false;
bool ManualPulse_ACO_Active = false;
bool ManualPulse_H2O2_Request = false; // Разовый запрос ручной проверки насоса H2O2 из Web/Nextion.
bool ManualPulse_ACO_Request = false; // Разовый запрос ручной проверки насоса ACO из Web/Nextion.
unsigned long ManualPulse_H2O2_StartedAt = 0;
unsigned long ManualPulse_ACO_StartedAt = 0;
uint16_t PendingAcoDosingGraphEvents = 0; // Сколько включений ACO нужно привязать к ближайшей точке графика pH.
uint16_t PendingH2O2DosingGraphEvents = 0; // Сколько включений NaOCl нужно привязать к ближайшей точке графика CL.


bool PH_Control_ACO = false, Saved_PHControlACO; // Флаг для отслеживания предыдущего состояния PH_Control_ACO
int ACO_Work = 3, Saved_ACO_Work; // Период дозирования кислоты: 3 = 5 секунд работы раз в 5 минут


bool NaOCl_H2O2_Control = false, Saved_NaOCl_H2O2_Control;
int H2O2_Work = 3, Saved_H2O2_Work; // Период дозирования хлора: 3 = 5 секунд работы раз в 5 минут

int corr_ORP_temper_mV; 		// ОРП с учетом калибровки по температуре
int CalRastvor256mV	= 256;	//Калибровочный раствор
int Calibration_ORP_mV = 0; //Калибровочный кооффициент - разница между колибровочным раствором 256mV 25C	 и показаниями сенсора
int corrected_ORP_Eh_mV;		// ОРП с учетом калибровки по  калибровочному раствору 	
float Saved_ppmCl, ppmCl = 1.3; //Свободный Хлор CL2 -  млг/литр, норма: 1,3млг/литр

//Строки для хранения информации о таймерах
char Info_H2O2[50]; String Saved_Info_H2O2;
char Info_ACO[50];  String Saved_Info_ACO;

// ===== Настройки пределов pH =====
float PH_Lower = 7.2f; // Нижний предел pH: при дозировании кислоты насос останавливается после снижения до этого значения
float PH_setting = 7.6f; // Верхний предел pH для совместимости со старым ключом настроек
float PH_Upper = 7.6f; // Верхний предел pH: выше этого значения разрешается дозирование кислоты

// ===== Настройки пределов CL/ORP =====
float CL_Lower = 1.0f; // Нижний предел свободного хлора, ppm: ниже включается дозатор NaOCl
float CL_Upper = 3.0f; // Верхний предел свободного хлора, ppm: выше дозатор NaOCl выключается
int ORP_setting = 500; // Нижний предел ORP оставлен для совместимости и контроля по мВ

unsigned long NextionDispensersWriteHoldUntil = 0;
unsigned long NextionDispensersReadHoldUntil = 0;

inline void holdNextionDispensersWrites(unsigned long holdMs = 2500) {
  NextionDispensersWriteHoldUntil = millis() + holdMs;
}

inline void holdNextionDispensersReads(unsigned long holdMs = 1500) {
  NextionDispensersReadHoldUntil = millis() + holdMs;
}

inline bool nextionDispensersWriteHoldActive() {
  return NextionDispensersWriteHoldUntil != 0 &&
         static_cast<long>(millis() - NextionDispensersWriteHoldUntil) < 0;
}

inline bool nextionDispensersReadHoldActive() {
  return NextionDispensersReadHoldUntil != 0 &&
         static_cast<long>(millis() - NextionDispensersReadHoldUntil) < 0;
}

inline String PoolWaterLevelLogicInfo = "Логика контроля уровня в бассейне: IN1 нижний уровнемер и IN2 верхний уровнемер работают как замыкающие контакты при падении уровня. Старт долива разрешается только после 10 последовательных подтверждений нижнего уровня с интервалом 1 секунда по IN1. Долив открывает реле 14 на заданное время или до верхнего уровня IN2. Верхний уровень считается достигнутым после 3 последовательных подтверждений с интервалом 1 секунда, после этого реле 14 закрывается.";
inline String DrainPitLogicInfo = "Логика контроля слива бассейна в яму: слив в яму деактивирует контроль уровня в бассейне. В режиме слива насос фильтрации на реле 9 работает порциями: заданное время работы, затем пауза, затем следующая порция до заданного количества циклов. IN3 контролирует верхний уровень сливной ямы: При заполнении ямы по уровнемеру или по времени текущая порция слива останавливается и система ждет паузу до следующей порции слива по времени или по уровню. Количество порций слива устанавливается.";
inline String PoolWaterLevelStageInfo; // Текущий этап контроля уровня с таймерами для Web.
inline String DrainPitStageInfo; // Текущий этап слива в яму с таймерами для Web.
inline String CleanLogicInfo = "Промывка: реле 9 насос, реле 10 компрессор, реле 11 клапан FILTRATION, реле 12 клапан BACKWASH, реле 13 сброс песка. Последовательность: останов насоса, воздух, перевод клапанов, обратная промывка, возврат в фильтрацию, краткий сброс песка.";
inline String PhLogicInfo = "pH: насос кислоты ACO подключен к реле 7. При pH выше верхнего предела реле включается только импульсом 5 секунд; повтор задается списком. Остановка контроля при pH ниже нижнего предела или выключенной фильтрации.";
inline String ClLogicInfo = "CL: насос NaOCl подключен к реле 6. При хлоре ниже нижнего предела реле включается только импульсом 5 секунд; повтор задается списком. Остановка контроля при хлоре выше верхнего предела или выключенной фильтрации.";
inline String RelayShortInfo = "RS485: реле 9 насос фильтрации, 6 NaOCl, 7 ACO, 14 долив, входы 1/2/3 уровни воды.";
inline String Rs485UsageInfo = "Задействовано: R1 лампа бассейна; R2 питание WS2815; R6 насос NaOCl; R7 насос ACO; R9 насос фильтрации/слив; R10 компрессор клапанов; R11 клапан FILTRATION; R12 клапан BACKWASH; R13 сброс песка; R14 клапан долива; R15 теплый пол; R16 улица. Свободны/резерв: R3, R4, R5, R8. Входы: IN1 нижний уровень бассейна, IN2 верхний уровень бассейна, IN3 верхний уровень сливной ямы, IN4-IN16 свободны.";

inline bool poolLowerLevelLowRaw(){ // Сырой нижний датчик: 1 означает падение уровня ниже датчика.
  return WaterLevelSensorLower;
}

inline bool poolUpperLevelReachedRaw(){ // Верхний уровень достигнут, когда замыкающийся при падении контактор DI2 разомкнут.
  return !WaterLevelSensorUpper;
}

inline bool drainPitFullRaw(){ // Верхний уровень ямы достигнут, когда замыкающийся при падении контактор DI3 разомкнут.
  return !WaterLevelSensorDrain;
}

inline void updateWaterLevelDebounce(){ // Неблокирующий антидребезг уровнемеров: один контрольный замер в секунду.
  static unsigned long lastWaterLevelSampleAt = 0; // Время последнего контрольного замера.
  const unsigned long now = millis(); // Текущее время работы ESP32.

  DrainPitFullConfirmed = drainPitFullRaw(); // Для заполнения ямы используем мгновенное безопасное состояние: слив надо остановить быстро.
  if(lastWaterLevelSampleAt != 0 && now - lastWaterLevelSampleAt < 1000UL) return; // Ждем ровно секундный шаг антидребезга.
  lastWaterLevelSampleAt = now; // Фиксируем момент принятого замера.

  if(poolLowerLevelLowRaw()){ // Нижний датчик замкнулся: вода ниже нижнего уровня.
    if(PoolLowerLevelLowSamples < 10) PoolLowerLevelLowSamples++; // Набираем 10 последовательных подтверждений.
  } else { // Нижний датчик разомкнут: волна или уровень восстановился.
    PoolLowerLevelLowSamples = 0; // Для старта долива нужна новая полная серия из 10 замеров.
  }
  PoolLowerLevelLowConfirmed = PoolLowerLevelLowSamples >= 10; // Разрешение долива появляется только после 10 секунд стабильного нижнего уровня.

  if(poolUpperLevelReachedRaw()){ // Верхний датчик разомкнут: вода дошла до верхнего уровня.
    if(PoolUpperLevelReachedSamples < 3) PoolUpperLevelReachedSamples++; // Для остановки долива достаточно 3 стабильных замеров.
  } else { // Верхний датчик снова показывает уровень ниже верхнего датчика.
    PoolUpperLevelReachedSamples = 0; // Подтверждение верхнего уровня сбрасываем.
  }
  PoolUpperLevelReachedConfirmed = PoolUpperLevelReachedSamples >= 3; // Соленоид долива закрывается после 3 подтверждений.
}

inline int sanitizeDosingPeriodValue(int value) { // Приводим выбранный период дозирования к разрешенному значению.
  switch (value) { // Проверяем только те коды, которые реально есть в выпадающем списке.
    case 1: return 1; // 5 секунд работы раз в 15 секунд.
    case 2: return 2; // 5 секунд работы раз в 60 секунд.
    case 3: return 3; // 5 секунд работы раз в 5 минут.
    case 4: return 4; // 5 секунд работы раз в 15 минут.
    case 5: return 5; // 5 секунд работы раз в 30 минут.
    case 6: return 6; // 5 секунд работы раз в 1 час.
    case 7: return 7; // 5 секунд работы раз в 24 часа.
    case 8: return 8; // 5 секунд работы раз в 2 часа.
    case 9: return 9; // 5 секунд работы раз в 3 часа.
    case 10: return 10; // 5 секунд работы раз в 4 часа.
    case 11: return 11; // 5 секунд работы раз в 6 часов.
    case 12: return 12; // 5 секунд работы раз в 8 часов.
    case 13: return 13; // 5 секунд работы раз в 12 часов.
    default: return 3; // Любое поврежденное значение заменяем безопасным дефолтом 5 сек / 5 минут.
  }
}

inline int nextionDosingComboIndexFromMode(int mode) { // Nextion ComboBox хранит индекс строки, а ESP32 хранит код периода дозирования.
  switch (sanitizeDosingPeriodValue(mode)) { // Индексы строго соответствуют порядку строк в HMI-файле Nextion.
    case 1: return 0; // 5 сек / 15 сек.
    case 2: return 1; // 5 сек / 60 сек.
    case 3: return 2; // 5 сек / 5 мин.
    case 4: return 3; // 5 сек / 15 мин.
    case 5: return 4; // 5 сек / 30 мин.
    case 6: return 5; // 5 сек / 1 час.
    case 8: return 6; // 5 сек / 2 часа.
    case 9: return 7; // 5 сек / 3 часа.
    case 10: return 8; // 5 сек / 4 часа.
    case 11: return 9; // 5 сек / 6 часов.
    case 12: return 10; // 5 сек / 8 часов.
    case 13: return 11; // 5 сек / 12 часов.
    case 7: return 12; // 5 сек / 24 часа.
    default: return 2; // Безопасный индекс дефолта 5 сек / 5 минут.
  }
}

inline int dosingModeFromNextionComboIndex(int index) { // Преобразуем индекс ComboBox Nextion обратно в рабочий код ESP32.
  switch (index) { // Нельзя использовать index + 1, потому что 24 часа в HMI стоит последней строкой.
    case 0: return 1; // 5 сек / 15 сек.
    case 1: return 2; // 5 сек / 60 сек.
    case 2: return 3; // 5 сек / 5 мин.
    case 3: return 4; // 5 сек / 15 мин.
    case 4: return 5; // 5 сек / 30 мин.
    case 5: return 6; // 5 сек / 1 час.
    case 6: return 8; // 5 сек / 2 часа.
    case 7: return 9; // 5 сек / 3 часа.
    case 8: return 10; // 5 сек / 4 часа.
    case 9: return 11; // 5 сек / 6 часов.
    case 10: return 12; // 5 сек / 8 часов.
    case 11: return 13; // 5 сек / 12 часов.
    case 12: return 7; // 5 сек / 24 часа.
    default: return 3; // Любой битый индекс возвращаем к дефолту 5 сек / 5 минут.
  }
}

inline int nextionTenthsWhole(float value) { // Целая часть для пары Nextion Number-компонентов с шагом 0.1.
  const int scaled = static_cast<int>(round(value * 10.0f)); // Округляем один раз, чтобы целая и десятая часть не расходились.
  return scaled / 10; // Для 1.96 вернется 2, а не 1.
}

inline int nextionTenthsDigit(float value) { // Десятая часть для пары Nextion Number-компонентов с шагом 0.1.
  int scaled = static_cast<int>(round(value * 10.0f)); // Используем тот же принцип округления, что и для целой части.
  if (scaled < 0) scaled = -scaled; // Защита от случайного отрицательного ввода.
  return scaled % 10; // Возвращаем только одну цифру десятых.
}

inline unsigned long dosingPeriodMsFromMode(int mode) { // Переводим код периода дозирования в миллисекунды.
  switch (sanitizeDosingPeriodValue(mode)) { // Сначала нормализуем код, чтобы не получить нулевой период.
    case 1: return 15UL * 1000UL; // 15 секунд.
    case 2: return 60UL * 1000UL; // 60 секунд.
    case 3: return 5UL * 60UL * 1000UL; // 5 минут.
    case 4: return 15UL * 60UL * 1000UL; // 15 минут.
    case 5: return 30UL * 60UL * 1000UL; // 30 минут.
    case 6: return 60UL * 60UL * 1000UL; // 1 час.
    case 8: return 2UL * 60UL * 60UL * 1000UL; // 2 часа.
    case 9: return 3UL * 60UL * 60UL * 1000UL; // 3 часа.
    case 10: return 4UL * 60UL * 60UL * 1000UL; // 4 часа.
    case 11: return 6UL * 60UL * 60UL * 1000UL; // 6 часов.
    case 12: return 8UL * 60UL * 60UL * 1000UL; // 8 часов.
    case 13: return 12UL * 60UL * 60UL * 1000UL; // 12 часов.
    case 7: return 24UL * 60UL * 60UL * 1000UL; // 24 часа.
    default: return 5UL * 60UL * 1000UL; // Резервный возврат к дефолту 5 минут.
  }
}

inline int clampIntSetting(int value, int minValue, int maxValue, int fallback) { // Ограничиваем числовую настройку безопасным диапазоном.
  if (value < minValue || value > maxValue) return fallback; // Если значение из памяти повреждено, возвращаем понятный дефолт.
  return value; // Если значение корректное, оставляем его без изменений.
}

inline float clampFloatSetting(float value, float minValue, float maxValue, float fallback) { // Ограничиваем float-настройку безопасным диапазоном.
  if (isnan(value)) return fallback; // Если пришло не число, возвращаем дефолт.
  if (value < minValue) return minValue; // Если значение ниже допустимого, поднимаем до минимума.
  if (value > maxValue) return maxValue; // Если значение выше допустимого, опускаем до максимума.
  return value; // Если значение корректное, оставляем его без изменений.
}

inline void persistChemicalLimits() { // Сохраняем все пределы химии единым блоком.
  saveValue<float>("PH_Lower", PH_Lower); // Сохраняем нижний предел pH.
  saveValue<float>("PH_Upper", PH_Upper); // Сохраняем верхний предел pH.
  saveValue<float>("PH_setting", PH_Upper); // Сохраняем старый совместимый ключ верхней уставки pH.
  saveValue<float>("CL_Lower", CL_Lower); // Сохраняем нижний предел свободного хлора.
  saveValue<float>("CL_Upper", CL_Upper); // Сохраняем верхний предел свободного хлора.
}

inline bool chemicalLimitChanged(float oldValue, float newValue) { // Сравниваем float-настройки без реакции на микрошум.
  float delta = oldValue > newValue ? oldValue - newValue : newValue - oldValue;
  return delta >= 0.001f;
}

inline void persistChangedChemicalLimits(float oldPHLower, float oldPHUpper, float oldCLLower, float oldCLUpper) { // В NVS пишем только реально измененные пределы.
  if (chemicalLimitChanged(oldPHLower, PH_Lower)) saveValue<float>("PH_Lower", PH_Lower);
  if (chemicalLimitChanged(oldPHUpper, PH_Upper)) {
    saveValue<float>("PH_Upper", PH_Upper);
    saveValue<float>("PH_setting", PH_Upper);
  }
  if (chemicalLimitChanged(oldCLLower, CL_Lower)) saveValue<float>("CL_Lower", CL_Lower);
  if (chemicalLimitChanged(oldCLUpper, CL_Upper)) saveValue<float>("CL_Upper", CL_Upper);
}

inline void normalizeChemicalLimits() { // Исправляем поврежденные или перевернутые пределы pH/CL.
  PH_Lower = clampFloatSetting(PH_Lower, 6.0f, 9.0f, 7.2f); // Держим нижний pH в физически разумном диапазоне.
  PH_Upper = clampFloatSetting(PH_Upper, 6.0f, 9.0f, 7.6f); // Держим верхний pH в физически разумном диапазоне.
  CL_Lower = clampFloatSetting(CL_Lower, 0.0f, 10.0f, 1.0f); // Держим нижний CL в безопасном диапазоне ввода.
  CL_Upper = clampFloatSetting(CL_Upper, 0.0f, 10.0f, 3.0f); // Держим верхний CL в безопасном диапазоне ввода.
  if (PH_Lower >= PH_Upper) { // Верхний pH всегда должен быть выше нижнего, даже если нижний уперся в максимум.
    if (PH_Lower >= 8.9f) PH_Lower = 8.9f; // Оставляем место для минимального шага 0.1 pH.
    PH_Upper = clampFloatSetting(PH_Lower + 0.1f, 6.0f, 9.0f, 7.6f); // Восстанавливаем корректный гистерезис.
  }
  if (CL_Lower >= CL_Upper) { // Верхний CL всегда должен быть выше нижнего, иначе дозатор не имеет зоны остановки.
    if (CL_Lower >= 9.9f) CL_Lower = 9.9f; // Оставляем место для минимального шага 0.1 ppm.
    CL_Upper = clampFloatSetting(CL_Lower + 0.1f, 0.0f, 10.0f, 3.0f); // Восстанавливаем корректный гистерезис CL.
  }
  PH_setting = PH_Upper; // Синхронизируем старую переменную с новым верхним пределом pH.
}

inline bool applyChemistrySettingRequest(const String &id, const String &rawValue) { // Безопасно применяем пределы pH/CL из Web.
  float value = rawValue.toFloat(); // Преобразуем строку Web в число с плавающей точкой.
  const float oldPHLower = PH_Lower;
  const float oldPHUpper = PH_Upper;
  const float oldCLLower = CL_Lower;
  const float oldCLUpper = CL_Upper;
  if (id == "PH_Lower") PH_Lower = value; // Обновляем нижний предел pH.
  else if (id == "PH_Upper") PH_Upper = value; // Обновляем верхний предел pH.
  else if (id == "PH_setting") PH_Upper = value; // Старый ключ считаем верхним пределом pH.
  else if (id == "CL_Lower") CL_Lower = value; // Обновляем нижний предел свободного хлора.
  else if (id == "CL_Upper") CL_Upper = value; // Обновляем верхний предел свободного хлора.
  else return false; // Если ключ не относится к химии, не обрабатываем его здесь.
  normalizeChemicalLimits(); // После любого изменения исправляем диапазоны.
  persistChangedChemicalLimits(oldPHLower, oldPHUpper, oldCLLower, oldCLUpper); // Сохраняем только изменившиеся пределы.
  holdNextionDispensersReads();
  return true; // Сообщаем вызывающему коду, что ключ обработан.
}

inline void persistWaterControlModes() { // Сохраняем режимы воды сразу после автоматического или ручного изменения.
  saveValue<int>("Activation_Water_Level", Activation_Water_Level ? 1 : 0); // Сохраняем флаг автоматического контроля уровня.
  saveButtonState("Power_Drain", Power_Drain ? 1 : 0); // Сохраняем состояние кнопки слива.
  saveButtonState("Power_Topping", Power_Topping ? 1 : 0); // Сохраняем состояние ручного клапана долива.
}

inline bool applyWaterControlRequest(const String &id, bool requestedState) { // Централизованно применяем опасные команды уровня воды.
  if (id == "Power_Drain") { // Обрабатываем кнопку слива воды из бассейна.
    Power_Drain = requestedState; // Запоминаем требуемое состояние режима слива.
    if (Power_Drain) { // Если слив включили.
      Activation_Water_Level = false; // Автоматический контроль уровня при сливе запрещен.
      Power_Topping = false; // Долив при сливе запрещен.
      ToppingCycleRunning = false; // Текущий автоматический долив принудительно останавливаем.
      ToppingPauseRunning = false; // Пауза долива сбрасывается, потому что режим долива выключен.
    } // Конец защитных действий при включении слива.
    persistWaterControlModes(); // Сохраняем итоговое безопасное состояние в NVS.
    return true; // Сообщаем вызывающему коду, что команда обработана.
  } // Конец обработки Power_Drain.
  if (id == "Activation_Water_Level") { // Обрабатываем включение автоматического контроля уровня.
    Activation_Water_Level = requestedState && !Power_Drain; // Контроль уровня разрешен только при выключенном сливе.
    if (Activation_Water_Level) Power_Topping = false; // При старте автоматики ручной долив сбрасываем.
    persistWaterControlModes(); // Сохраняем итоговое безопасное состояние в NVS.
    return true; // Сообщаем вызывающему коду, что команда обработана.
  } // Конец обработки Activation_Water_Level.
  if (id == "Power_Topping") { // Обрабатываем ручной соленоидный клапан долива.
    Power_Topping = requestedState && !Power_Drain && !PoolUpperLevelReachedConfirmed; // Клапан нельзя открыть при сливе или подтвержденном верхнем уровне.
    persistWaterControlModes(); // Сохраняем итоговое безопасное состояние в NVS.
    return true; // Сообщаем вызывающему коду, что команда обработана.
  } // Конец обработки Power_Topping.
  return false; // Остальные id не относятся к опасным переключателям уровня.
}

inline String rs485ManualRelayKey(uint8_t relay) {
  return String("Rs485RelayManual") + String(relay + 1);
}

inline void persistRs485ManualRelay(uint8_t relay) {
  if (relay >= 16) return;
  const String key = rs485ManualRelayKey(relay);
  saveValue<int>(key.c_str(), Rs485ManualRelayState[relay] ? 1 : 0);
}

inline void loadRs485PanelSettings() {
  Rs485Enabled = loadValue<int>("Rs485Enabled", Rs485Enabled ? 1 : 0) != 0;
  Rs485BaudRate = loadValue<int>("Rs485BaudRate", Rs485BaudRate);
  if (Rs485BaudRate <= 0) Rs485BaudRate = 19200;
  Rs485UartMode = loadValue<String>("Rs485UartMode", Rs485UartMode);
  if (Rs485UartMode.length() == 0) Rs485UartMode = "8N1";
  Rs485SlaveId = loadValue<int>("Rs485SlaveId", Rs485SlaveId);
  if (Rs485SlaveId < 1) Rs485SlaveId = 1;
  if (Rs485SlaveId > 247) Rs485SlaveId = 247;
  Rs485PollIntervalMs = loadValue<int>("Rs485PollIntervalMs", Rs485PollIntervalMs);
  if (Rs485PollIntervalMs < 200) Rs485PollIntervalMs = 200;
  if (Rs485PollIntervalMs > 5000) Rs485PollIntervalMs = 5000;
  for (uint8_t relay = 0; relay < 16; relay++) {
    const String key = rs485ManualRelayKey(relay);
    Rs485ManualRelayState[relay] = loadValue<int>(key.c_str(), 0) != 0;
  }
}

inline bool applyRs485ConfigRequest(const String &key, const String &value) {
  if (key == "Rs485Enabled") {
    Rs485Enabled = value.toInt() != 0;
    saveValue<int>("Rs485Enabled", Rs485Enabled ? 1 : 0);
    return true;
  }
  if (key == "Rs485BaudRate") {
    Rs485BaudRate = value.toInt();
    if (Rs485BaudRate <= 0) Rs485BaudRate = 19200;
    saveValue<int>("Rs485BaudRate", Rs485BaudRate);
    return true;
  }
  if (key == "Rs485UartMode") {
    Rs485UartMode = value;
    if (Rs485UartMode.length() == 0) Rs485UartMode = "8N1";
    saveValue<String>("Rs485UartMode", Rs485UartMode);
    return true;
  }
  if (key == "Rs485SlaveId") {
    Rs485SlaveId = value.toInt();
    if (Rs485SlaveId < 1) Rs485SlaveId = 1;
    if (Rs485SlaveId > 247) Rs485SlaveId = 247;
    saveValue<int>("Rs485SlaveId", Rs485SlaveId);
    return true;
  }
  if (key == "Rs485PollIntervalMs") {
    Rs485PollIntervalMs = value.toInt();
    if (Rs485PollIntervalMs < 200) Rs485PollIntervalMs = 200;
    if (Rs485PollIntervalMs > 5000) Rs485PollIntervalMs = 5000;
    saveValue<int>("Rs485PollIntervalMs", Rs485PollIntervalMs);
    return true;
  }
  return false;
}

inline bool rs485RelayCommandState(uint8_t relay) {
  switch (relay) {
    case 0: return Lamp;
    case 1: return Pow_WS2815;
    case 2: return Rs485ManualRelayState[2];
    case 3: return Rs485ManualRelayState[3];
    case 4: return Power_Heat;
    case 5: return Power_H2O2 || ManualPulse_H2O2_Active;
    case 6: return Power_ACO || ManualPulse_ACO_Active;
    case 7: return Rs485ManualRelayState[7];
    case 8: return Power_Filtr;
    case 9: return AirPump || AirPumpAuto;
    case 10: return SolValveFiltration || ValveFiltrationAuto;
    case 11: return SolValveFilBack || ValveBackwashAuto;
    case 12: return SolSandDump || SolSandDumpAuto;
    case 13: return Power_Topping;
    case 14: return Power_Warm_floor_heating;
    case 15: return Pow_Ul_light;
    default: return false;
  }
}

inline bool applyRs485RelayRequest(uint8_t relay, bool requestedState) {
  if (relay >= 16) return false;
  switch (relay) {
    case 0:
      SetLamp = requestedState ? "on" : "off";
      Lamp = requestedState;
      Lamp_autosvet = false;
      Power_Time1 = false;
      saveValue<String>("SetLamp", SetLamp);
      saveButtonState("button_Lamp", Lamp ? 1 : 0);
      saveValue<int>("Lamp_autosvet", 0);
      saveValue<int>("Power_Time1", 0);
      return true;
    case 1:
      SetRGB = requestedState ? "on" : "off";
      Pow_WS2815 = requestedState;
      Pow_WS2815_autosvet = false;
      WS2815_Time1 = false;
      saveValue<String>("SetRGB", SetRGB);
      saveButtonState("button_WS2815", Pow_WS2815 ? 1 : 0);
      saveValue<int>("Pow_WS2815_autosvet", 0);
      saveValue<int>("WS2815_Time1", 0);
      return true;
    case 2:
    case 3:
    case 7:
      Rs485ManualRelayState[relay] = requestedState;
      persistRs485ManualRelay(relay);
      return true;
    case 4:
      Activation_Heat = requestedState;
      Power_Heat = requestedState;
      saveValue<int>("Activation_Heat", Activation_Heat ? 1 : 0);
      return true;
    case 5:
      Power_H2O2 = requestedState;
      if (!requestedState) ManualPulse_H2O2_Active = false;
      return true;
    case 6:
      Power_ACO = requestedState;
      if (!requestedState) ManualPulse_ACO_Active = false;
      return true;
    case 8:
      Power_Filtr = requestedState;
      saveButtonState("Power_Filtr", Power_Filtr ? 1 : 0);
      return true;
    case 9:
      AirPump = requestedState;
      saveButtonState("AirPump", AirPump ? 1 : 0);
      return true;
    case 10:
      SolValveFiltration = requestedState;
      saveButtonState("SolValveFiltration", SolValveFiltration ? 1 : 0);
      return true;
    case 11:
      SolValveFilBack = requestedState;
      saveButtonState("SolValveFilBack", SolValveFilBack ? 1 : 0);
      return true;
    case 12:
      SolSandDump = requestedState;
      saveButtonState("SolSandDump", SolSandDump ? 1 : 0);
      return true;
    case 13:
      applyWaterControlRequest("Power_Topping", requestedState);
      return true;
    case 14:
      RoomTemper = false;
      Power_Warm_floor_heating = requestedState;
      saveValue<int>("RoomTemper", 0);
      saveValue<int>("Power_Warm_floor_heating", Power_Warm_floor_heating ? 1 : 0);
      return true;
    case 15:
      Pow_Ul_light = requestedState;
      saveButtonState("Pow_Ul_light", Pow_Ul_light ? 1 : 0);
      return true;
    default:
      return false;
  }
}

inline void applyRs485AllOffRequest() {
  SetLamp = "off";
  Lamp = false;
  Lamp_autosvet = false;
  Power_Time1 = false;
  SetRGB = "off";
  Pow_WS2815 = false;
  Pow_WS2815_autosvet = false;
  WS2815_Time1 = false;
  Power_Heat = false;
  Activation_Heat = false;
  Power_H2O2 = false;
  ManualPulse_H2O2_Active = false;
  ManualPulse_H2O2_Request = false;
  Power_ACO = false;
  ManualPulse_ACO_Active = false;
  ManualPulse_ACO_Request = false;
  Power_Filtr = false;
  Power_Drain = false;
  AirPump = false;
  SolValveFiltration = false;
  SolValveFilBack = false;
  SolSandDump = false;
  AirPumpAuto = false;
  ValveFiltrationAuto = false;
  ValveBackwashAuto = false;
  SolSandDumpAuto = false;
  CleanSequenceActive = false;
  applyWaterControlRequest("Power_Topping", false);
  RoomTemper = false;
  Power_Warm_floor_heating = false;
  Pow_Ul_light = false;
  for (uint8_t relay = 0; relay < 16; relay++) {
    Rs485ManualRelayState[relay] = false;
    persistRs485ManualRelay(relay);
  }
  saveValue<String>("SetLamp", SetLamp);
  saveButtonState("button_Lamp", 0);
  saveValue<int>("Lamp_autosvet", 0);
  saveValue<int>("Power_Time1", 0);
  saveValue<String>("SetRGB", SetRGB);
  saveButtonState("button_WS2815", 0);
  saveValue<int>("Pow_WS2815_autosvet", 0);
  saveValue<int>("WS2815_Time1", 0);
  saveValue<int>("Activation_Heat", 0);
  saveButtonState("Power_Filtr", 0);
  saveButtonState("AirPump", 0);
  saveButtonState("SolValveFiltration", 0);
  saveButtonState("SolValveFilBack", 0);
  saveButtonState("SolSandDump", 0);
  saveValue<int>("RoomTemper", 0);
  saveValue<int>("Power_Warm_floor_heating", 0);
  saveButtonState("Pow_Ul_light", 0);
}


inline uint8_t unicodeCyrillicToKoi8R(uint32_t cp) {
  static const uint8_t lower[] = {
    0xC1,0xC2,0xD7,0xC7,0xC4,0xC5,0xD6,0xDA,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,
    0xD2,0xD3,0xD4,0xD5,0xC6,0xC8,0xC3,0xDE,0xDB,0xDD,0xDF,0xD9,0xD8,0xDC,0xC0,0xD1
  };
  static const uint8_t upper[] = {
    0xE1,0xE2,0xF7,0xE7,0xE4,0xE5,0xF6,0xFA,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,
    0xF2,0xF3,0xF4,0xF5,0xE6,0xE8,0xE3,0xFE,0xFB,0xFD,0xFF,0xF9,0xF8,0xFC,0xE0,0xF1
  };
  if (cp >= 0x0410 && cp <= 0x042F) return upper[cp - 0x0410];
  if (cp >= 0x0430 && cp <= 0x044F) return lower[cp - 0x0430];
  if (cp == 0x0401) return 0xB3;
  if (cp == 0x0451) return 0xA3;
  return 0;
}

inline String nextionKoi8R(const String &utf8Text) {
  String out;
  for (size_t i = 0; i < utf8Text.length();) {
    uint8_t c = static_cast<uint8_t>(utf8Text[i]);
    if (c < 0x80) { out += static_cast<char>(c); i++; continue; }
    uint32_t cp = 0;
    size_t advance = 1;
    if ((c & 0xE0) == 0xC0 && i + 1 < utf8Text.length()) {
      cp = ((c & 0x1F) << 6) | (static_cast<uint8_t>(utf8Text[i + 1]) & 0x3F);
      advance = 2;
    } else if ((c & 0xF0) == 0xE0 && i + 2 < utf8Text.length()) {
      cp = ((c & 0x0F) << 12) | ((static_cast<uint8_t>(utf8Text[i + 1]) & 0x3F) << 6) | (static_cast<uint8_t>(utf8Text[i + 2]) & 0x3F);
      advance = 3;
    } else if ((c & 0xF8) == 0xF0 && i + 3 < utf8Text.length()) {
      cp = ((c & 0x07) << 18) | ((static_cast<uint8_t>(utf8Text[i + 1]) & 0x3F) << 12) | ((static_cast<uint8_t>(utf8Text[i + 2]) & 0x3F) << 6) | (static_cast<uint8_t>(utf8Text[i + 3]) & 0x3F);
      advance = 4;
    }
    uint8_t koi = unicodeCyrillicToKoi8R(cp);
    if (koi) out += static_cast<char>(koi);
    else out += '?';
    i += advance;
  }
  return out;
}


inline String waterLevelNextionText() { // Формируем короткую русскую строку для поля set_topping.t0.
  if (Power_Drain && DrainCycleRunning) return "Слив: работа " + String(DrainCyclesDone + 1) + "/" + String(DrainCyclesTarget); // Показываем активную порцию слива.
  if (Power_Drain && DrainPauseRunning) return "Слив: пауза " + String(DrainCyclesDone) + "/" + String(DrainCyclesTarget); // Показываем паузу между порциями.
  if (Power_Drain) return "Слив: ожидание"; // Показываем включенный слив до старта следующей порции.
  if (Activation_Water_Level && ToppingCycleRunning) return "Долив: клапан открыт"; // Показываем фактическую работу клапана долива.
  if (Activation_Water_Level && ToppingPauseRunning) return "Долив: пауза"; // Показываем паузу после долива.
  if (Activation_Water_Level && ToppingFillToUpperActive) return "Долив: до верхнего"; // Показываем защелкнутый цикл долива до верхнего уровня.
  if (Activation_Water_Level) return "Контроль уровня включен"; // Показываем активный режим контроля уровня.
  return "Контроль уровня отключен"; // Показываем выключенный режим.
}

inline String formatStageDuration(unsigned long ms) { // Форматируем остаток времени для строк этапов без лишней длины.
  unsigned long seconds = (ms + 999UL) / 1000UL; // Округляем вверх, чтобы не показывать 0 сек до фактического конца.
  if (seconds >= 3600UL) { // Для длинных пауз удобнее часы и минуты.
    unsigned long hours = seconds / 3600UL;
    unsigned long minutes = (seconds % 3600UL) / 60UL;
    return String(hours) + " ч " + String(minutes) + " мин";
  }
  if (seconds >= 60UL) { // Для обычных рабочих циклов показываем минуты и секунды.
    unsigned long minutes = seconds / 60UL;
    unsigned long rest = seconds % 60UL;
    return String(minutes) + " мин " + String(rest) + " сек";
  }
  return String(seconds) + " сек"; // Короткий остаток показываем в секундах.
}

inline unsigned long remainingStageMs(unsigned long startedAt, unsigned long durationMs) { // Считает остаток этапа без блокировок.
  if (durationMs == 0UL) return 0UL; // Нулевая длительность означает, что ждать нечего.
  if (startedAt == 0UL) return durationMs; // Если старт еще не записан, показываем полную длительность.
  unsigned long elapsed = millis() - startedAt; // unsigned-разница корректно переживает переполнение millis().
  if (elapsed >= durationMs) return 0UL; // Этап уже должен завершиться.
  return durationMs - elapsed; // Возвращаем оставшееся время.
}

inline String buildPoolWaterLevelStageInfo() { // Текущий этап контроля уровня бассейна для Web-страницы.
  if (Power_Drain) return "Контроль уровня: временно остановлен, активен слив бассейна в яму."; // Во время слива долив воды запрещен.
  if (!Activation_Water_Level) return "Контроль уровня: выключен."; // Режим контроля уровня отключен пользователем.

  const unsigned long toppingWorkMs = static_cast<unsigned long>(ToppingWorkMinutes) * 60UL * 1000UL; // Настроенное время открытия клапана.
  const unsigned long toppingPauseMs = static_cast<unsigned long>(ToppingPauseMinutes) * 60UL * 1000UL; // Настроенная пауза после долива.

  if (ToppingCycleRunning) { // Сейчас открыт соленоид долива.
    String upper = poolUpperLevelReachedRaw()
      ? String("верхний датчик ") + String(PoolUpperLevelReachedSamples) + "/3"
      : String("верхний уровень еще не достигнут"); // Видно, набирается ли быстрый антидребезг остановки.
    return String("Долив: клапан открыт, осталось ") + formatStageDuration(remainingStageMs(ToppingCycleStartedAt, toppingWorkMs)) + "; " + upper + ".";
  }

  if (ToppingPauseRunning) { // После порции долива идет обязательная пауза.
    return String("Долив: пауза, осталось ") + formatStageDuration(remainingStageMs(ToppingPauseStartedAt, toppingPauseMs)) + ".";
  }

  if (PoolUpperLevelReachedConfirmed) return "Контроль уровня: верхний уровень достигнут, долив не требуется."; // Верхний датчик стабильно подтвердил нормальный уровень.
  if (ToppingFillToUpperActive) return "Контроль уровня: долив продолжается до верхнего датчика."; // Нижний уровень уже запускал цикл, работаем до верхнего уровня.
  if (PoolLowerLevelLowConfirmed) return "Контроль уровня: нижний уровень подтвержден, запуск долива до верхнего датчика."; // Нижний уровень подтвержден, автоматика запустит цикл долива.
  if (poolLowerLevelLowRaw()) { // Контакт нижнего датчика уже замкнут, но 10 замеров еще не набрано.
    return String("Контроль уровня: проверка нижнего датчика ") + String(PoolLowerLevelLowSamples) + "/10.";
  }
  return "Контроль уровня: уровень выше нижнего датчика, ожидание падения уровня."; // Нормальное ожидание без долива.
}

inline String buildDrainPitStageInfo() { // Текущий этап контроля слива в яму для Web-страницы.
  const String pit = DrainPitFullConfirmed ? String("яма заполнена") : String("яма не заполнена"); // Семантический статус DI3.
  if (!Power_Drain) return String("Слив: выключен; ") + pit + "."; // Режим слива отключен.

  const unsigned long drainWorkMs = static_cast<unsigned long>(DrainWorkMinutes) * 60UL * 1000UL; // Длительность порции слива.
  const unsigned long drainPauseMs = static_cast<unsigned long>(DrainPauseMinutes) * 60UL * 1000UL; // Пауза между порциями.

  if (DrainCycleRunning) { // Насос сейчас сливает воду.
    return String("Слив: порция ") + String(DrainCyclesDone + 1) + "/" + String(DrainCyclesTarget) +
           ", насос работает, осталось " + formatStageDuration(remainingStageMs(DrainCycleStartedAt, drainWorkMs)) +
           "; " + pit + ".";
  }
  if (DrainPauseRunning) { // Между порциями идет пауза.
    return String("Слив: пауза после ") + String(DrainCyclesDone) + "/" + String(DrainCyclesTarget) +
           ", осталось " + formatStageDuration(remainingStageMs(DrainPauseStartedAt, drainPauseMs)) +
           "; " + pit + ".";
  }
  if (DrainCyclesDone >= DrainCyclesTarget) return String("Слив: заданные порции выполнены ") + String(DrainCyclesDone) + "/" + String(DrainCyclesTarget) + ".";
  if (DrainPitFullConfirmed) return "Слив: ожидание, яма заполнена."; // До освобождения ямы новую порцию не запускаем.
  return String("Слив: ожидание старта порции ") + String(DrainCyclesDone + 1) + "/" + String(DrainCyclesTarget) + ".";
}

inline String serviceNextionText() { // Формируем понятную строку для Service.t0 без кракозябр.
  String ip = WiFi.localIP().toString(); // Берем текущий IP адрес ESP32.
  String mode = (WiFi.getMode() == WIFI_AP) ? "AP" : "STA"; // Показываем режим Wi-Fi.
  return "WiFi: " + mode + "\rIP: " + ip + "\rRSSI: " + String(WiFi.RSSI()) + " dBm\rHeap: " + String(ESP.getFreeHeap()); // Возвращаем компактный статус для Nextion.
}


bool Pow_WS2815, Pow_WS28151;		// Включение в ручную
bool Pow_WS2815_autosvet, Saved_Pow_WS2815_autosvet; 
bool WS2815_Time1, Saved_WS2815_Time1;

inline const char* currentBasinImagePath() { // Выбирает изображение бассейна по фактическому состоянию подсветки.
  if (Pow_WS2815 && Lamp) return "/Basin_RGB_LAMP.jpg";
  if (Pow_WS2815) return "/Basin_RGB.jpg";
  if (Lamp) return "/Basin_Lamp.jpg";
  return "/Basin.jpg";
}

uint16_t Saved_timeON_WS2815, Saved_timeOFF_WS2815;

inline int TimertestON = 0;        // Значение включения тестового таймера (минуты от начала суток)
inline int FiltrTimer1ON = 9 * 60;      // Время включения фильтрации №1: 09:00 по умолчанию
inline int FiltrTimer1OFF = 12 * 60;     // Время отключения фильтрации №1: 12:00 по умолчанию
inline int FiltrTimer2ON = 14 * 60;      // Время включения фильтрации №2: 14:00 по умолчанию
inline int FiltrTimer2OFF = 17 * 60;     // Время отключения фильтрации №2: 17:00 по умолчанию
inline int FiltrTimer3ON = 19 * 60;      // Время включения фильтрации №3: 19:00 по умолчанию
inline int FiltrTimer3OFF = 22 * 60;     // Время отключения фильтрации №3: 22:00 по умолчанию
inline int CleanTimer1ON = 12 * 60;      // Время включения промывки: 12:00 по умолчанию
inline int CleanTimer1OFF = 12 * 60;     // Время отключения промывки
inline int LampTimerON = 20 * 60;        // Таймер лампы: 20:00 по умолчанию
inline int LampTimerOFF = 60;            // Таймер лампы: 01:00 по умолчанию
inline int RgbTimerON = 20 * 60;         // Таймер RGB-подсветки: 20:00 по умолчанию
inline int RgbTimerOFF = 60;             // Таймер RGB-подсветки: 01:00 по умолчанию
inline int UlLightTimerON = 20 * 60;     // Таймер уличного освещения: 20:00 по умолчанию
inline int UlLightTimerOFF = 3 * 60;     // Таймер уличного освещения: 03:00 по умолчанию


bool ColorRGB = false;    //режим ручного задания цвета
int new_bright = 200; //переменная с яркостью установленной в интерфейсе вручную
bool LedAutoplay = true;
int LedAutoplayDuration = 30;
String LedPattern = "rainbow";
String LedColorMode = "auto";
String LedColorOrder = "GRB";
int LedBrightness = 200;
int currentPatternIndex = 0; //Номер цветовой программы в реальном времени

bool Index0,Index1,Index2,Index3,Index4,Index5,Index6,Index7,Index8,Index9,Index10,
Index11,Index12,Index13,Index14,Index15,Index16,Index17,Index18,Index19,Index20,
Index21,Index22,Index23,Index24;

	
bool Power_Filtr, Power_Filtr1;		// Фильтрация в бассейне -  Включение в ручную
bool Filtr_Time1 = false, Filtr_Time2 = false, Filtr_Time3 = false; // Разрешения работы включения по времени
bool Saved_Filtr_Time1, Saved_Filtr_Time2, Saved_Filtr_Time3;
uint16_t Saved_Filtr_timeON1, Saved_Filtr_timeOFF1, Saved_Filtr_timeON2, Saved_Filtr_timeOFF2, Saved_Filtr_timeON3, Saved_Filtr_timeOFF3; 

bool Power_Clean, Power_Clean1; // Промывка фильтра
bool Clean_Time1, Saved_Clean_Time1; // Разрешения работы включения по времени
uint16_t Saved_Clean_timeON1, Saved_Clean_timeOFF1;

bool AirPump, SolValveFilBack, SolValveFiltration, SolSandDump; // Тестовые реле компрессора воздуха, соленоидов клапанов и сброса песка
int TimerAirSetting = 60; // Время накачки воздуха компрессором (сек)
int TimerValveSetting = 30; // Время на переключение трехходовых клапанов (сек)
int TimerBackwashSetting = 120; // Время обратной промывки (сек)
int TimerSolSandDump = 30; // Время сброса песка (сек)

bool AirPumpAuto; // Автоматическое включение компрессора воздуха в режиме промывки
bool SolSandDumpAuto; // Автоматическое включение соленоида сброса песка после промывки
bool ValveBackwashAuto; // Автоматическое включение соленоида переключения клапанов в BACKWASH
bool ValveFiltrationAuto; // Автоматическое включение соленоида переключения клапанов в FILTRATION

bool CleanSequenceActive; // Флаг активной последовательности промывки
bool CleanResumeFiltration; // Нужно ли восстановить фильтрацию после промывки
bool CleanScheduleRequested; // Запрос промывки по расписанию
bool CleanManualRequested; // Запрос промывки вручную
bool FiltrationTimerActive; // Состояние фильтрации по таймерам в текущий момент

enum CleanSequenceStep : uint8_t { // Перечисление шагов промывки
  CleanStepIdle = 0, // Бездействие
  CleanStepStopPump, // Останов насоса
  CleanStepAirPump, // Накачка воздуха
  CleanStepValveToBackwash, // Перевод клапанов в BACKWASH
  CleanStepBackwash, // Обратная промывка
  CleanStepStopPumpAfter, // Остановка насоса после промывки
  CleanStepValveToFiltration, // Возврат клапанов в FILTRATION
  CleanStepStartPumpAfter, // Запуск насоса после возврата
  CleanStepSandDumpOn, // Включение сброса песка
  CleanStepSandDumpOff, // Отключение сброса песка
  CleanStepComplete // Завершение последовательности
};

CleanSequenceStep CleanStepState = CleanStepIdle; // Текущий шаг последовательности промывки
unsigned long CleanStepStartedAt = 0; // Метка времени начала текущего шага промывки

bool chk1, chk2, chk3, chk4, chk5, chk6, chk7; //Дни недели ПН, ВТ, СР, ЧТ, ПТ, СБ, ВС - для включения таймера в нужные дни
bool Saved_chk1, Saved_chk2, Saved_chk3, Saved_chk4, Saved_chk5, Saved_chk6, Saved_chk7;


inline void syncCleanDaysFromSelection(){ // Синхронизирует флаги дней недели по строке DaysSelect
  const char* tokens[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"}; // Краткие названия дней недели
  bool *flags[] = {&chk1, &chk2, &chk3, &chk4, &chk5, &chk6, &chk7}; // Указатели на флаги выбранных дней
  for(size_t i = 0; i < 7; i++){ // Проход по всем дням недели
    *flags[i] = DaysSelect.indexOf(tokens[i]) >= 0; // Флаг true, если день найден в строке DaysSelect
  }
}



inline void syncDaysSelectionFromClean(){ // Формирует строку DaysSelect на основе флагов chk1–chk7
  const char* tokens[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"}; // Краткие названия дней недели
  const bool flags[] = {chk1, chk2, chk3, chk4, chk5, chk6, chk7}; // Текущие значения флагов дней
  String next; // Строка для формирования результата
  for(size_t i = 0; i < 7; i++){ // Проход по всем дням недели
    if(flags[i]){ // Если день выбран
      if(next.length()) next += ","; // Добавляем запятую, если строка уже не пустая
      next += tokens[i]; // Добавляем название дня
    }
  }
  DaysSelect = next; // Сохраняем итоговую строку выбранных дней
}

inline uint16_t parseTimeToMinutes(const String &value){ // Преобразует строку "HH:MM" в минуты от начала суток
  int sep = value.indexOf(':'); // Ищем позицию разделителя часов и минут
  if(sep < 0) return 0; // Если формат неверный — возвращаем 0
  int hours = value.substring(0, sep).toInt(); // Извлекаем часы
  int minutes = value.substring(sep + 1).toInt(); // Извлекаем минуты
  hours = constrain(hours, 0, 23); // Ограничиваем часы допустимым диапазоном
  minutes = constrain(minutes, 0, 59); // Ограничиваем минуты допустимым диапазоном
  return static_cast<uint16_t>(hours * 60 + minutes); // Возвращаем общее количество минут
}


inline String formatMinutesToTime(uint16_t minutes){
  minutes = minutes % 1440; // Ограничение значением одних суток (1440 минут)
  uint16_t hours = minutes / 60; // Вычисление часов из общего количества минут
  uint16_t mins = minutes % 60; // Вычисление оставшихся минут
  char buffer[6]; // Буфер для строки времени формата HH:MM
  snprintf(buffer, sizeof(buffer), "%02u:%02u", hours, mins); // Формирование строки времени с ведущими нулями
  return String(buffer); // Возврат строки времени
}

struct UITimerEntry {
  String id; // Идентификатор таймера
  String label; // Отображаемое имя таймера
  uint16_t on = 0; // Время включения в минутах от начала суток
  uint16_t off = 0; // Время выключения в минутах от начала суток
  std::function<void(uint16_t, uint16_t)> callback = nullptr; // Callback при изменении времени таймера
};

class UIRegistry {
public:
  static String timerStorageKey(const String &id, const String &suffix){
    if(id == "UlLightTimer"){
      return String("UlLightT") + suffix; // Укороченный ключ хранения для UlLightTimer
    }
    return id + suffix; // Стандартный ключ хранения
  }

  static String legacyTimerStorageKey(const String &id, const String &suffix){
    if(id == "UlLightTimer"){
      return id + suffix; // Старый формат ключа для UlLightTimer
    }
    return String(); // Для остальных таймеров legacy-ключ отсутствует
  }

  UITimerEntry &registerTimer(const String &id, const String &label,
                              const std::function<void(uint16_t, uint16_t)> &cb){
    UITimerEntry *entry = findTimer(id); // Поиск таймера по идентификатору
    if(!entry){
      UITimerEntry created; // Создание нового таймера
      created.id = id; // Установка идентификатора
      created.label = label; // Установка отображаемого имени
      created.on = static_cast<uint16_t>(loadValue<int>((id + "_ON").c_str(), 0)); // Загрузка сохранённого времени включения
      created.off = static_cast<uint16_t>(loadValue<int>((id + "_OFF").c_str(), 0)); // Загрузка сохранённого времени выключения
      created.on = loadTimerValue(id, "_ON"); // Загрузка времени включения с учётом legacy-ключей
      created.off = loadTimerValue(id, "_OFF"); // Загрузка времени выключения с учётом legacy-ключей
      created.callback = cb; // Привязка callback-функции
      timers.push_back(created); // Добавление таймера в список
      entry = &timers.back(); // Получение указателя на добавленный таймер
    } else {
      entry->label = label; // Обновление отображаемого имени
      entry->callback = cb; // Обновление callback-функции
    }
    return *entry; // Возврат ссылки на таймер
  }

  UITimerEntry &timer(const String &id){
    UITimerEntry *entry = findTimer(id); // Поиск таймера по идентификатору
    if(!entry){
      return registerTimer(id, id, nullptr); // Создание таймера при отсутствии
    }
    return *entry; // Возврат найденного таймера
  }

  bool updateTimerField(const String &fieldId, const String &value){
    bool isOn = fieldId.endsWith("_ON"); // Проверка, относится ли поле ко времени включения
    bool isOff = fieldId.endsWith("_OFF"); // Проверка, относится ли поле ко времени выключения
    if(!isOn && !isOff) return false; // Поле не относится к таймеру
    String base = fieldId.substring(0, fieldId.length() - (isOn ? 3 : 4)); // Получение базового идентификатора таймера
    UITimerEntry *entry = findTimer(base); // Поиск таймера по базовому идентификатору
    if(!entry) return false; // Таймер не найден
    uint16_t minutes = parseTimeToMinutes(value); // Преобразование строки времени в минуты
    if(isOn) entry->on = minutes; // Обновление времени включения
    else entry->off = minutes; // Обновление времени выключения
    saveValue<int>(fieldId.c_str(), minutes); // Сохранение значения по исходному ключу
    String storageKey = timerStorageKey(base, isOn ? "_ON" : "_OFF"); // Формирование ключа хранения
    saveValue<int>(storageKey.c_str(), minutes); // Сохранение значения по ключу хранения
    if(entry->callback) entry->callback(entry->on, entry->off); // Вызов callback при изменении таймера
    return true; // Поле успешно обработано
  }

  void setTimerMinutes(const String &id, uint16_t onMinutes, uint16_t offMinutes, bool persist = true){
    UITimerEntry &entry = timer(id); // Получение таймера по идентификатору
    entry.on = onMinutes; // Установка времени включения
    entry.off = offMinutes; // Установка времени выключения
    if(persist){
      saveValue<int>((id + "_ON").c_str(), entry.on); // Сохранение времени включения
      saveValue<int>((id + "_OFF").c_str(), entry.off); // Сохранение времени выключения
      String onKey = timerStorageKey(id, "_ON"); // Формирование ключа времени включения
      String offKey = timerStorageKey(id, "_OFF"); // Формирование ключа времени выключения
      saveValue<int>(onKey.c_str(), entry.on); // Сохранение времени включения по ключу хранения
      saveValue<int>(offKey.c_str(), entry.off); // Сохранение времени выключения по ключу хранения
    }
    if(entry.callback) entry.callback(entry.on, entry.off); // Вызов callback после обновления таймера
  }

  const std::vector<UITimerEntry> &allTimers() const { return timers; }

private:
  std::vector<UITimerEntry> timers; // Контейнер всех зарегистрированных таймеров

  UITimerEntry *findTimer(const String &id){
    for(auto &entry : timers){
      if(entry.id == id) return &entry; // Возврат указателя на найденный таймер
    }
    return nullptr; // Таймер не найден
  }
  
    uint16_t defaultTimerValue(const String &id, const String &suffix){
    const bool isOn = suffix == "_ON";
    if(id == "FiltrTimer1") return isOn ? 9 * 60 : 12 * 60;
    if(id == "FiltrTimer2") return isOn ? 14 * 60 : 17 * 60;
    if(id == "FiltrTimer3") return isOn ? 19 * 60 : 22 * 60;
    if(id == "CleanTimer1") return 12 * 60;
    if(id == "LampTimer") return isOn ? 20 * 60 : 60;
    if(id == "RgbTimer") return isOn ? 20 * 60 : 60;
    if(id == "UlLightTimer") return isOn ? 20 * 60 : 3 * 60;
    return 0;
  }

  uint16_t loadTimerValue(const String &id, const String &suffix){
    String storageKey = timerStorageKey(id, suffix); // Формирование ключа хранения
    int value = loadValue<int>(storageKey.c_str(), -1); // Загрузка значения из хранилища
    if(value < 0){
      String legacyKey = legacyTimerStorageKey(id, suffix); // Формирование legacy-ключа
      if(legacyKey.length()){
        value = loadValue<int>(legacyKey.c_str(), 0); // Загрузка значения по legacy-ключу
      } else {
        value = defaultTimerValue(id, suffix); // Значение по умолчанию
      }
    }
    return static_cast<uint16_t>(value); // Возврат значения в минутах
  }
};

inline UIRegistry ui;

#include "settings_MQTT.h"       // Настройки и работа с MQTT (после UIRegistry)


inline void noopTimerCallback(uint16_t onMinutes, uint16_t offMinutes){
  (void)onMinutes; // Подавление предупреждения о неиспользуемом параметре
  (void)offMinutes; // Подавление предупреждения о неиспользуемом параметре
}

inline void onLampTimerChange(uint16_t onMinutes, uint16_t offMinutes){
  (void)onMinutes; // Подавление предупреждения о неиспользуемом параметре
  (void)offMinutes; // Подавление предупреждения о неиспользуемом параметре
}


extern void interface();
String uiValueForId(const String &id);
bool uiApplyValueForId(const String &id, const String &value);


// Безопасное экранирование строк для JSON ответов
String jsonEscape(const String &input){
  String output; // Выходная строка с экранированными символами
  output.reserve(input.length() + 8); // Резервируем память с небольшим запасом
  for(size_t i = 0; i < input.length(); i++){ // Посимвольный обход входной строки
    char c = input.charAt(i); // Текущий символ
    switch(c){
      case '\\': output += "\\\\"; break; // Экранирование обратного слэша
      case '\"': output += "\\\""; break; // Экранирование двойной кавычки
      case '\b': output += "\\b"; break; // Экранирование backspace
      case '\f': output += "\\f"; break; // Экранирование form feed
      case '\n': output += "\\n"; break; // Экранирование перевода строки
      case '\r': output += "\\r"; break; // Экранирование возврата каретки
      case '\t': output += "\\t"; break; // Экранирование табуляции
      default:
        if(static_cast<uint8_t>(c) < 0x20){ // Проверка на непечатаемые управляющие символы
          char buf[7]; // Буфер для unicode-последовательности
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<uint8_t>(c)); // Формирование escape-последовательности
          output += buf; // Добавление unicode-экранирования
        } else {
          output += c; // Добавление обычного символа без изменений
        }
    }
  }
  return output; // Возврат экранированной строки
}

String chipSeriesName(const esp_chip_info_t &info){
  switch(info.model){ // Определение серии чипа по модели
    case CHIP_ESP32: return "ESP32"; // Классический ESP32
    case CHIP_ESP32S2: return "ESP32-S2"; // Серия ESP32-S2
    case CHIP_ESP32S3: return "ESP32-S3"; // Серия ESP32-S3
    case CHIP_ESP32C3: return "ESP32-C3"; // Серия ESP32-C3
#ifdef CHIP_ESP32C2
    case CHIP_ESP32C2: return "ESP32-C2"; // Серия ESP32-C2
#endif
#ifdef CHIP_ESP32C6
    case CHIP_ESP32C6: return "ESP32-C6"; // Серия ESP32-C6
#endif
    case CHIP_ESP32H2: return "ESP32-H2"; // Серия ESP32-H2
    default: return "ESP32"; // Значение по умолчанию
  }
}

String buildChipIdentity(){
  esp_chip_info_t info; // Структура с информацией о чипе
  esp_chip_info(&info); // Заполнение структуры информацией о чипе

  const String series = chipSeriesName(info); // Получение имени серии чипа
  char buffer[96]; // Буфер для итоговой строки
  snprintf(buffer, sizeof(buffer), "%s rev %d", series.c_str(), info.revision); // Формирование строки с серией и ревизией
  return String(buffer); // Возврат строки идентификации чипа
}

inline bool readPsramStats(uint32_t &freePsram, uint32_t &totalPsram){
#if defined(ARDUINO_ARCH_ESP32)
  if(psramFound()){ // Проверка наличия PSRAM
    freePsram = ESP.getFreePsram(); // Получение объёма свободной PSRAM
    totalPsram = ESP.getPsramSize(); // Получение общего объёма PSRAM
    return true; // PSRAM доступна
  }
#endif
  freePsram = 0; // PSRAM отсутствует, свободная память = 0
  totalPsram = 0; // PSRAM отсутствует, общий объём = 0
  return false; // PSRAM недоступна
}

inline bool isAuthConfigured(){
  return authUsername.length() > 0 && authPassword.length() > 0; // Проверка, заданы ли логин и пароль
}

inline bool ensureAuthorized(AsyncWebServerRequest *request){
  if(!isAuthConfigured()) return true; // Если авторизация не настроена — доступ разрешён
  if(!request->authenticate(authUsername.c_str(), authPassword.c_str())){ // Проверка логина и пароля
    request->requestAuthentication(); // Запрос авторизации у клиента
    return false; // Доступ запрещён
  }
  return true; // Авторизация успешна
}

inline bool isAdminAuthConfigured(){
  return adminUsername.length() > 0 && adminPassword.length() > 0; // Проверка, заданы ли учётные данные администратора
}

inline bool ensureAdminAuthorized(AsyncWebServerRequest *request){
  if(!isAdminAuthConfigured()) return true; // Если админ-авторизация не настроена — доступ разрешён
  if(!request->authenticate(adminUsername.c_str(), adminPassword.c_str())){ // Проверка админских учётных данных
    request->requestAuthentication(); // Запрос авторизации у клиента
    return false; // Доступ запрещён
  }
  return true; // Администратор успешно авторизован
}


// ---------- Структуры UI ----------
struct Tab { String id; String title; }; // Описание вкладки UI: идентификатор и отображаемый заголовок
struct Element { String type; String id; String label; String value; String tab; }; // Описание элемента UI и вкладки, к которой он относится

struct Popup { String id; String title; String tabId; }; // Описание всплывающего окна и связанной вкладки

struct SidebarBlock { String type; String id; String label; String value; }; // Декларативный блок левой панели.
struct SystemPageDecl { String id; String title; String type; }; // Декларативная служебная страница Web UI.

inline std::vector<SidebarBlock> sidebarBlocks; // Блоки левой панели, объявленные в interface().
inline std::vector<SystemPageDecl> systemPageDecls; // Служебные страницы, объявленные в interface().

inline void resetSystemUiDeclarations() {
  sidebarBlocks.clear();
  systemPageDecls.clear();
}

inline void registerSidebarBlock(const String &type, const String &id, const String &label, const String &value = String()) {
  sidebarBlocks.push_back({type, id, label, value});
}

inline void registerSystemPage(const String &id, const String &title, const String &type) {
  for(auto &page : systemPageDecls){
    if(page.id == id || page.type == type){
      page.id = id;
      page.title = title;
      page.type = type;
      return;
    }
  }
  systemPageDecls.push_back({id, title, type});
}



// ---------- Класс MiniDash ----------
class MiniDash {
public:
    vector<Tab> tabs;         // Список зарегистрированных вкладок
    vector<Element> elements; // Список элементов пользовательского интерфейса
        vector<Popup> popups; // Список всплывающих окон
  void addTab(const String &id, const String &title){ tabs.push_back({id,title}); } // Добавляет новую вкладку
  void addElement(const String &tab, const Element &e){ Element x=e; x.tab=tab; elements.push_back(x); } // Добавляет элемент и привязывает его к вкладке
  
    void addPopup(const String &id, const String &title, const String &tabId){
    for(auto &popup : popups){
      if(popup.id == id) return; // Если popup с таким id уже существует — ничего не делаем
    }
    popups.push_back({id, title, tabId}); // Добавляем новое всплывающее окно
  }

  void begin(){
    if(!dashInterfaceInitialized){
      interface(); // Инициализация интерфейса
      dashInterfaceInitialized = true; // Флаг завершённой инициализации
    }
    setupServer(); // Запуск и настройка веб-сервера
  }

private:
// Настройка веб-сервера
  void setupServer(){
    MiniDash *self=this; // Указатель на текущий экземпляр для использования в колбэках

    // Подключаем кастомные генераторы значений UI, чтобы объединять состояния и форматировать строки
    registerUiValueProvider("DS1", [](){ return formatTemperatureString(DS1, DS1Available); }); // отображение температуры с учетом доступности датчика
    registerUiValueProvider("RoomTemp", [](){ return formatTemperatureString(DS1, DS1Available); }); // дублирует формат DS1 для помещения
    registerUiValueProvider("Power_ACO", [](){ // подставляем статус работы дозатора ACO
      const bool active = Power_ACO || ManualPulse_ACO_Active;
            return active ? String("Работа") : String("Откл.");
    });
    registerUiValueProvider("Power_ACO_Button", [](){ // кнопка также светится при ручном импульсе
        return (Power_ACO || ManualPulse_ACO_Active || ManualPulse_ACO_Request) ? String("1") : String("0");
    });
    registerUiValueProvider("Power_H2O2", [](){ // статус дозатора H2O2
      const bool active = Power_H2O2 || ManualPulse_H2O2_Active;
        return active ? String("Работа") : String("Откл.");
    });
    registerUiValueProvider("Power_H2O2_Button", [](){ // кнопка H2O2 реагирует на импульсы
      return (Power_H2O2 || ManualPulse_H2O2_Active || ManualPulse_H2O2_Request) ? String("1") : String("0");
    });
    registerUiValueProvider("Activation_Water_Level", [](){
      return Activation_Water_Level ? String("1") : String("0");
    });
    registerUiValueProvider("Power_Topping", [](){
      return Power_Topping ? String("1") : String("0");
    });
    registerUiValueProvider("Power_Drain", [](){
      return Power_Drain ? String("1") : String("0");
    });
    registerUiValueProvider("PoolWaterLevelStageInfo", [](){ // Строка этапа уровня всегда строится из текущих таймеров и датчиков.
      return buildPoolWaterLevelStageInfo();
    });
    registerUiValueProvider("DrainPitStageInfo", [](){ // Строка этапа слива всегда строится из текущих таймеров и датчика ямы.
      return buildDrainPitStageInfo();
    });
    registerUiValueProvider("Lumen_Ul", [](){ // Освещенность на Web должна брать живой процент, а не стартовое значение.
      return String(Lumen_Ul);
    });
    registerUiValueProvider("WaterLevelSensorUpper", [](){ // На Web показываем смысл, а не сырой замкнутый контакт DI2.
      return PoolUpperLevelReachedConfirmed ? String("верхний уровень достигнут") : String("уровень ниже верхнего датчика");
    });
    registerUiValueProvider("WaterLevelSensorLower", [](){ // Нижний датчик требует 10 последовательных подтверждений.
      if(PoolLowerLevelLowConfirmed) return String("нижний уровень подтвержден");
      if(poolLowerLevelLowRaw()){
        String msg = String("подтверждение нижнего уровня ") + String(PoolLowerLevelLowSamples) + String("/10"); // Собираем строку явно, чтобы lambda возвращала именно String.
        return msg;
      }
      return String("уровень выше нижнего датчика");
    });
    registerUiValueProvider("WaterLevelSensorDrain", [](){ // Для ямы DI3 инвертирован: разомкнутый контакт означает верхний уровень воды.
      return DrainPitFullConfirmed ? String("яма заполнена") : String("яма не заполнена");
    });

    // Кнопки промывки должны отражать и логику автоматики, и фактическое состояние реле.
    // Поэтому объединяем: ручной флаг UI + автофлаг шага + обратное чтение Modbus.
    registerUiValueProvider("AirPump", [](){
      const bool active = AirPump || AirPumpAuto || ReadRelayArray[9];
      return active ? String("1") : String("0");
    }); // компрессор воздуха
    registerUiValueProvider("SolValveFilBack", [](){
      const bool active = SolValveFilBack || ValveBackwashAuto || ReadRelayArray[11];
      return active ? String("1") : String("0");
    }); // соленоид трехходовых клапанов
      registerUiValueProvider("SolValveFiltration", [](){
      const bool active = SolValveFiltration || ValveFiltrationAuto || ReadRelayArray[10];
      return active ? String("1") : String("0");
    }); // соленоид трехходовых клапанов в FILTRATION
    registerUiValueProvider("SolSandDump", [](){
      const bool active = SolSandDump || SolSandDumpAuto || ReadRelayArray[12];
      return active ? String("1") : String("0");
    }); // соленоид сброса песка
    registerUiValueProvider("Power_Filtr", [](){
      const bool active = Power_Filtr || ReadRelayArray[8];
      return active ? String("1") : String("0");
    }); // насос фильтрации: команда + факт реле
    registerUiValueProvider("Power_Clean", [](){
      const bool active = Power_Clean || CleanSequenceActive || ReadRelayArray[3];
      return active ? String("1") : String("0");
    }); // промывка: логический цикл + факт реле


      server.on("/", HTTP_GET, [self](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;

      const uint32_t requestStartMs = millis(); // Общее время GET /: потом сравниваем с таймингами этапов
      uint32_t stageStartMs = requestStartMs; // Точка отсчёта длительности текущего этапа рендера
      uint32_t stageBuildHeaderMs = 0; // Время этапа сборки head/CSS для поиска узкого места
      uint32_t stageRenderTabsMs = 0; // Время рендера вкладок (контейнеры страниц)
      uint32_t stageRenderElementsMs = 0; // Суммарное время рендера элементов интерфейса
      uint32_t stageRenderPopupsMs = 0; // Время рендера popup-блоков
      uint32_t stageSendMs = 0; // Время постановки/старта отправки ответа клиенту
      if(kWebVerboseSerial) Serial.printf("[WEB:/] begin | resetReason=%s\n", webResetReasonToText(esp_reset_reason())); // Печатаем старт GET / только в debug-режиме.
      if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] start=%u ms\n", static_cast<unsigned>(0)); // Базовый маркер начала GET / только для диагностики.
      logWebHeapStats("before-html-build"); // Снимок кучи до генерации HTML для контроля фрагментации

       // Формируем HTML-страницу
      String html; // Единый буфер HTML, который далее отдаётся в chunked-режиме
      logWebHeapStats("before-reserve"); // Проверяем heap перед крупным reserve
      const size_t htmlReserveBytes = 260000; // Увеличенный reserve снижает realloc и риск обрыва при росте UI
      if(!html.reserve(htmlReserveBytes)){
        if(kWebVerboseSerial) Serial.printf("[WEB:/] html.reserve(%u) failed\n", static_cast<unsigned>(htmlReserveBytes)); // Явно фиксируем OOM/фрагментацию только в debug-режиме.
        logWebHeapStats("reserve-failed"); // Фиксируем состояние памяти в точке отказа reserve
        r->send(500, "text/plain", "Insufficient heap for HTML page"); // Явный ответ вместо краша при нехватке памяти
        return; // Прерываем обработку, чтобы не усугублять ситуацию с памятью
      }
      if(kWebVerboseSerial) Serial.printf("[WEB:/] html.reserve ok | reserved=%u\n", static_cast<unsigned>(htmlReserveBytes)); // Подтверждаем reserve только в debug-режиме.
      logWebHeapStats("after-reserve"); // Контроль, сколько heap осталось после резерва буфера

      html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'><title>";
      html += dashAppTitle;

      stageBuildHeaderMs = millis() - stageStartMs; // Фиксируем длительность buildHeader для диагностики WDT
      if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] buildHeader=%u ms\n", static_cast<unsigned>(stageBuildHeaderMs)); // Лог этапа buildHeader только для диагностики.
      stageStartMs = millis(); // Новый отсчёт для следующего крупного этапа сборки HTML

      html += "</title>"
      "<style>" // Начало встроенных CSS-стилей интерфейса
      "body{margin:0;background:"+ThemeColor+";color:#ddd;font-family:'Inter', 'Segoe UI', 'Roboto', Arial, sans-serif;} " // Базовые стили страницы и цвет темы
      "#sidebar{width:230px;position:fixed;left:0;top:0;background:#151515;height:100vh;padding:10px;box-sizing:border-box;transition:all 0.3s;overflow:auto;box-shadow:2px 0 12px rgba(0,0,0,0.45);} " // Боковая панель навигации
      "#sidebar.collapsed{width:0;padding:0;overflow:hidden;opacity:0;pointer-events:none;} " // Скрытое состояние боковой панели
      "#main{margin-left:230px;padding:20px;overflow:auto;height:100vh;box-sizing:border-box;transition:margin-left 0.3s;} " // Основная область контента
      "body.sidebar-hidden #main{margin-left:20px;} " // Отступ контента при скрытом сайдбаре
      "#toggleBtn{position:fixed;top:10px;left:230px;width:36px;height:48px;background:rgba(255,255,255,0.12);border:none;border-radius:0 6px 6px 0;cursor:pointer;z-index:1000;transition:left 0.3s, opacity 0.3s, background 0.3s;box-shadow:2px 2px 10px rgba(0,0,0,0.4);display:flex;align-items:center;justify-content:center;} " // Кнопка сворачивания/разворачивания сайдбара
      "#toggleBtn::before{content:'';width:22px;height:22px;display:block;opacity:0.7;background-repeat:no-repeat;background-position:center;background-size:contain;background-image:url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='white' stroke-width='1.6' stroke-linecap='round' stroke-linejoin='round'><path d='M3 5.5c0-1.1.9-2 2-2h6c1.7 0 3 1.3 3 3v13c0-1.7-1.3-3-3-3H5c-1.1 0-2-.9-2-2V5.5z'/><path d='M21 5.5c0-1.1-.9-2-2-2h-6c-1.7 0-3 1.3-3 3v13c0-1.7 1.3-3 3-3h6c1.1 0 2-.9 2-2V5.5z'/></svg>\");} " // SVG-иконка кнопки
      "body.sidebar-hidden #toggleBtn{left:16px;opacity:0.65;} " // Положение кнопки при скрытом сайдбаре
      "#sidebar button{width:100%;padding:10px;margin:6px 0;border:none;border-radius:8px;background:#2a2a2a;color:#ccc;text-align:left;cursor:pointer;font-weight:600;transition:0.2s;} " // Кнопки навигации в сайдбаре
      "#sidebar button.active{background:#4CAF50;color:#fff;} " // Активная кнопка вкладки
      ".card{background:#1d1d1f;padding:14px;border-radius:14px;margin-bottom:14px;box-shadow:0 4px 14px rgba(0,0,0,0.45);border:1px solid rgba(255,255,255,0.06);display:flex;flex-direction:column;gap:6px;} " // Универсальная карточка UI
      ".card.compact{padding:10px 12px;margin-bottom:10px;} " // Компактный вариант карточки
      ".card.compact label{font-size:0.78em;color:#b0b0b0;margin-bottom:2px;text-transform:uppercase;letter-spacing:0.3px;} " // Заголовки в компактных карточках
      ".card.compact input,.card.compact select,.card.compact .display-value{font-size:0.95em;padding:6px 8px;border-radius:8px;background:#111;border:1px solid #262626;color:#f6f6f6;} " // Поля ввода в компактных карточках
      ".card.compact .display-value{background:transparent;border:none;padding:2px 0;} " // Текстовое отображение значения
".select-days{--day-accent:#4cc3ff;display:flex;flex-wrap:nowrap;gap:8px;" // Контейнер выбора дней недели
      "justify-content:center;align-items:center;padding:8px;" // Выравнивание и отступы блока
      "background:linear-gradient(135deg,rgba(255,255,255,0.06),rgba(255,255,255,0.02));" // Фон панели выбора дней
      "border-radius:14px;border:1px solid rgba(255,255,255,0.12);" // Скругление и рамка
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 10px 24px rgba(0,0,0,0.45);" // Внутренняя и внешняя тень
      "overflow-x:auto;scrollbar-width:thin;scrollbar-color:rgba(255,255,255,0.2) transparent;} " // Горизонтальный скролл
      ".select-days::-webkit-scrollbar{height:6px;} " // Высота полосы прокрутки
 ".select-days::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.2);border-radius:999px;} " // Ползунок горизонтального скролла блока дней
      ".select-days .day-pill{position:relative;display:inline-flex;align-items:center;justify-content:center;" // Кнопка выбора одного дня
      "min-width:42px;height:32px;padding:0 12px;border-radius:999px;" // Размеры и форма кнопки дня
      "font-size:0.78em;font-weight:600;letter-spacing:0.08em;text-transform:uppercase;" // Типографика надписи дня
      "color:#b7c0ce;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.1);" // Цвета и рамка кнопки
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 6px 14px rgba(0,0,0,0.4);" // Объёмная тень кнопки
      "cursor:pointer;transition:transform 0.18s ease,box-shadow 0.18s ease,background 0.18s ease,color 0.18s ease;} " // Анимации и интерактивность
      ".select-days .day-pill input{position:absolute;opacity:0;pointer-events:none;} " // Скрытый checkbox внутри кнопки
      ".select-days .day-pill:hover{transform:translateY(-1px) scale(1.05);" // Эффект наведения на кнопку дня
      "box-shadow:0 10px 18px rgba(0,0,0,0.45);} " // Усиленная тень при наведении
      ".select-days .day-pill:active{transform:translateY(0) scale(0.98);} " // Эффект нажатия
      ".select-days .day-pill:has(input:checked){color:#fff;" // Стиль выбранного дня
      "background:linear-gradient(135deg,rgba(76,195,255,0.3),rgba(76,195,255,0.12));" // Фон выбранного дня
      "border-color:rgba(76,195,255,0.6);" // Рамка выбранного дня
      "box-shadow:0 0 0 1px rgba(76,195,255,0.35),0 12px 26px rgba(76,195,255,0.35);} " // Подсветка выбранного дня
      ".select-days .day-pill:has(input:checked)::after{content:'';position:absolute;inset:-6px;" // Светящийся ореол выбранного дня
      "border-radius:inherit;background:radial-gradient(circle,rgba(76,195,255,0.35),transparent 65%);" // Градиент свечения
      "opacity:0.75;filter:blur(6px);z-index:0;} " // Размытие и прозрачность ореола
      ".select-days .day-pill span{position:relative;z-index:1;} " // Текст поверх эффектов
      "@media (max-width:520px){" // Адаптация под маленькие экраны
      ".select-days{gap:6px;padding:6px;}" // Уменьшенные отступы блока дней
      ".select-days .day-pill{min-width:36px;height:30px;padding:0 10px;font-size:0.74em;}" // Компактные кнопки дней
      "} " // Конец media-запроса
      ".select-days.compact{gap:6px;padding:6px;background:#141416;border:1px solid #242424;} " // Компактный режим выбора дней
      "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:center;} " // Базовые стили таблиц
      "th{background:#333;} " // Заголовки таблицы
      ".dash-btn{display:inline-block;margin-top:6px;padding:8px 16px;border-radius:10px;border:1px solid rgba(255,255,255,0.18);background:#222;color:#ddd;font-weight:600;cursor:pointer;box-shadow:0 4px 10px rgba(0,0,0,0.35);transition:transform 0.15s ease, box-shadow 0.15s ease, background 0.15s ease, color 0.15s ease;letter-spacing:0.03em;text-transform:uppercase;font-size:0.78rem;} " // Универсальная кнопка дашборда
      ".dash-btn.on{background:linear-gradient(135deg,#3a7bd5,#00d2ff);color:#fff;} " // Активное состояние кнопки
      ".dash-btn.off{background:#222;color:#ddd;opacity:0.9;} " // Неактивное состояние кнопки
            "#AirPump.on,#SolValveFilBack.on,#SolSandDump.on{background:#2e7d32;border-color:#2e7d32;color:#fff;} " // Активные реле промывки
      "#AirPump.off,#SolValveFilBack.off,#SolSandDump.off{background:#3a3a3a;border-color:#3a3a3a;color:#fff;} " // Неактивные реле промывки
      "#AirPump.on,#SolValveFilBack.on,#SolValveFiltration.on,#SolSandDump.on{background:#2e7d32;border-color:#2e7d32;color:#fff;} " // Активные реле промывки
      "#AirPump.off,#SolValveFilBack.off,#SolValveFiltration.off,#SolSandDump.off{background:#3a3a3a;border-color:#3a3a3a;color:#fff;} " // Неактивные реле промывки
      ".dash-btn:hover{transform:translateY(-1px);box-shadow:0 6px 14px rgba(0,0,0,0.45);} " // Эффект наведения на кнопку
                ".page{display:none;position:relative;grid-template-columns: repeat(auto-fill, minmax(250px, 1fr)); gap:15px;} " // Страница интерфейса
      ".page.active{display:block;} " // Отображение активной страницы
      ".rs485-panel{display:flex;flex-direction:column;gap:12px;width:100%;} "
      ".rs485-card{background:#1d1d1f;border:1px solid rgba(255,255,255,0.08);border-radius:8px;box-shadow:0 10px 24px rgba(0,0,0,0.45);padding:14px;box-sizing:border-box;} "
      ".rs485-hero{display:flex;align-items:center;justify-content:space-between;gap:16px;} "
      ".rs485-title{font-size:1.25rem;font-weight:800;color:#fff;line-height:1.2;} "
      ".rs485-sub{color:#c7dcff;font-size:0.9rem;line-height:1.45;} "
      ".rs485-status{min-width:156px;padding:10px 14px;border-radius:8px;border:1px solid #1c5d9c;background:#10243f;color:#fff;font-weight:800;text-align:center;} "
      ".rs485-status.off{border-color:#7a2626;background:#3a1717;} "
      ".rs485-board{display:grid;grid-template-columns:minmax(220px,300px) 1fr;gap:12px;align-items:stretch;} "
      ".rs485-side{display:flex;flex-direction:column;gap:10px;} "
      ".rs485-enable{display:flex;align-items:center;gap:8px;font-weight:800;color:#fff;} "
      ".rs485-field{display:flex;flex-direction:column;gap:6px;} "
      ".rs485-field label{font-size:0.9rem;color:#fff;} "
      ".rs485-field select,.rs485-field input{width:100%;box-sizing:border-box;background:#111;border:1px solid #333;border-radius:6px;color:#fff;padding:8px 10px;font-size:0.95rem;} "
      ".rs485-note{font-size:0.88rem;line-height:1.45;color:#b9d1ff;} "
      ".rs485-image-card{display:grid;grid-template-columns:minmax(260px,0.95fr) 1.6fr;gap:10px;background:#121820;border:1px solid rgba(255,255,255,0.08);border-radius:8px;padding:10px;} "
      ".rs485-image{width:100%;min-height:210px;max-height:250px;object-fit:contain;background:#05090e;border:1px solid rgba(255,255,255,0.10);border-radius:6px;} "
      ".rs485-spec{background:#171b22;border:1px solid rgba(255,255,255,0.10);border-radius:6px;padding:12px;} "
      ".rs485-spec h4{margin:0 0 10px 0;color:#fff;font-size:1.05rem;} "
      ".rs485-spec-row{display:grid;grid-template-columns:110px 1fr;gap:10px;margin:7px 0;font-size:0.9rem;} "
      ".rs485-spec-row strong{color:#8fc3ff;} "
      ".rs485-controls{display:grid;grid-template-columns:repeat(5,minmax(120px,1fr));gap:10px;align-items:end;} "
      ".rs485-action{height:42px;border-radius:8px;border:1px solid rgba(255,255,255,0.16);background:#222a34;color:#fff;font-weight:800;cursor:pointer;} "
      ".rs485-action.danger{background:#641d1d;border-color:#983434;} "
      ".rs485-grid-title{font-size:1.1rem;font-weight:800;color:#fff;margin:2px 0 12px;} "
      ".rs485-relay-grid{display:grid;grid-template-columns:repeat(4,minmax(170px,1fr));gap:8px;} "
      ".rs485-relay-row{display:grid;grid-template-columns:48px 1fr;gap:10px;align-items:center;background:#111820;border:1px solid rgba(255,255,255,0.08);border-radius:7px;padding:8px;} "
      ".rs485-relay-row span{font-weight:800;color:#dfeeff;} "
      ".rs485-relay-btn{height:38px;border-radius:8px;border:1px solid #555;background:#242321;color:#fff;font-weight:900;letter-spacing:0.02em;cursor:pointer;} "
      ".rs485-relay-btn.on{background:#0e6b38;border-color:#22a357;color:#fff;} "
      ".rs485-input-grid{display:grid;grid-template-columns:repeat(4,minmax(150px,1fr));gap:8px;} "
      ".rs485-input-chip{height:34px;border-radius:7px;border:1px solid rgba(255,255,255,0.12);background:#232a32;color:#dceeff;font-weight:800;display:flex;align-items:center;justify-content:center;} "
      ".rs485-input-chip.on{background:#0d4269;border-color:#318bd0;color:#fff;} "
      ".rs485-assignments{display:grid;grid-template-columns:repeat(2,minmax(260px,1fr));gap:8px;color:#d9e7ff;font-size:0.92rem;line-height:1.35;} "
      ".rs485-assignment{background:#121820;border:1px solid rgba(255,255,255,0.08);border-radius:7px;padding:8px 10px;} "
      "@media (max-width:1050px){.rs485-board,.rs485-image-card{grid-template-columns:1fr;}.rs485-controls{grid-template-columns:repeat(2,minmax(140px,1fr));}.rs485-relay-grid,.rs485-input-grid,.rs485-assignments{grid-template-columns:repeat(2,minmax(140px,1fr));}} "
      "@media (max-width:640px){.rs485-hero{align-items:flex-start;flex-direction:column;}.rs485-controls,.rs485-relay-grid,.rs485-input-grid,.rs485-assignments{grid-template-columns:1fr;}.rs485-status{width:100%;box-sizing:border-box;}} "
            ".page-header{display:flex;flex-direction:row;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;margin-bottom:10px;} " // Заголовок страницы
      ".page-header h3{margin:0;} " // Заголовок без отступов
             ".page-datetime{font-size:clamp(0.95em, 1.6vw, 1.25em);letter-spacing:0.08em;text-align:right;font-weight:600;" // Блок даты и времени
      "margin-left:auto;display:inline-flex;align-items:center;justify-content:center;gap:8px;max-width:100%;white-space:nowrap;" // Выравнивание блока даты
      "padding:6px 12px;border-radius:12px;background:rgba(0,0,0,0.55);border:1px solid rgba(255,255,255,0.18);" // Фон и рамка даты
      "box-shadow:0 6px 14px rgba(0,0,0,0.4);" // Тень блока даты
      "color:#fff;text-shadow:0 0 10px rgba(0,0,0,0.55),0 1px 1px rgba(0,0,0,0.9);} " // Цвет и читаемость текста

                  ".timer-card{gap:12px;}" // Общие отступы внутри карточки таймера
      ".timer-card__header{font-size:0.85em;text-transform:uppercase;letter-spacing:0.08em;color:#9fb4c8;font-weight:600;}" // Заголовок карточки таймера
      ".timer-card__grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px;}" // Сетка полей таймера
      ".timer-card__column{display:flex;flex-direction:column;gap:6px;}" // Колонка таймера
      ".timer-card__column label{margin-bottom:0;font-size:0.78em;letter-spacing:0.08em;text-transform:uppercase;color:#c1d0e2;}" // Подписи полей таймера
      ".timer-card input[type=time]{width:100%;margin:0;}" // Поле времени на всю ширину
      "@media (max-width:640px){.timer-card__grid{grid-template-columns:1fr;}} " // Адаптация таймера под мобильные экраны
             
      "@keyframes rainbow-shift{0%{background-position:0% 50%;}100%{background-position:100% 50%;}} " // Анимация смещения градиента
      "label{display:block;margin-bottom:5px;font-size:0.9em;} " // Базовый стиль label
      "input,select{width:100%;padding:7px;margin-bottom:10px;background:#111;border:1px solid #333;color:#ddd;border-radius:6px;} " // Общие стили полей ввода
      "input[type=time]{width:30%;margin-right:auto;} " // Поле времени уменьшенной ширины
      "input[type=checkbox]{width:auto;margin-right:auto;} " // Чекбоксы без растягивания
      "label:has(input[type=checkbox]){display:flex;flex-direction:column;align-items:flex-start;gap:6px;width:fit-content;} " // Контейнер чекбокса с подписью
      "label:has(input[type=checkbox]) input[type=checkbox]{margin-bottom:0;transform:scale(1.4);transform-origin:left center;} " // Увеличенный чекбокс
      "input[type=range]{width:100%;} " // Ползунок на всю ширину
      "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:center;} " // Таблицы
      "th{background:#333;} " // Заголовки таблиц
      ".card.pro-card{background:linear-gradient(135deg,#0f0f12,#151a2d);border:1px solid rgba(129,193,255,0.4);} " // Pro-карточка
      ".card.pro-card label{color:#9ae7ff;text-transform:uppercase;letter-spacing:0.08em;font-size:0.75em;} " // Заголовки Pro-карточки
      ".card.pro-card input,.card.pro-card select{background:#090c10;border-color:#252f40;color:#eff6ff;} " // Поля ввода Pro-карточки
      ".graph-controls{display:flex;flex-direction:row;flex-wrap:nowrap;gap:18px;align-items:center;} " // Панель управления графиком
      ".graph-controls .control-group{flex:1;min-width:220px;display:flex;align-items:center;gap:10px;} " // Группа контролов графика
      ".graph-controls label{display:inline-flex;font-size:0.72em;color:#9fb4c8;letter-spacing:0.08em;text-transform:uppercase;margin-bottom:0;white-space:nowrap;} " // Подписи графика
      ".graph-controls select,.graph-controls input{margin-bottom:0;flex:1;} " // Поля управления графиком
      ".graph-card{position:relative;overflow:hidden;background:linear-gradient(180deg,#171924,#10131d);border-radius:8px;}"  // Карточка графика
      ".graph-heading{text-align:center;font-size:1.05em;font-weight:500;color:#dce5f2;margin-bottom:10px;}"  // Заголовок графика
      ".dash-graph{width:100%;background:#05070a;border-radius:0;}"  // Холст графика
      ".graph-axes{position:absolute;inset:0;pointer-events:none;font-size:0.85em;color:#cbd4df;}"  // Подписи осей
      ".graph-axes .axis-name{position:absolute;}"  // Названия осей
      ".graph-axes .axis-name:first-child{right:10px;bottom:8px;}"  // Подпись оси X
      ".graph-axes .axis-name:last-child{left:8px;top:8px;writing-mode:vertical-rl;transform:rotate(180deg);}"  // Подпись оси Y
      ".graph-table-wrap{max-height:220px;overflow-y:auto;overflow-x:auto;margin-top:0;padding:0;}"  // Прокрутка таблицы точек графика
      ".graph-table-wrap table{margin-top:0;border-collapse:separate;border-spacing:0;}"  // Таблица графика без сдвига заголовка при sticky.
      ".graph-table-wrap thead th{position:sticky;top:0;z-index:3;background:#303030;box-shadow:0 2px 0 #444;}"  // Фиксация заголовка таблицы графика
      ".level-sensor-card{display:grid;grid-template-columns:104px minmax(0,1fr);gap:14px;align-items:center;min-height:128px;}" // Карточка уровнемера с наглядной пиктограммой.
      ".level-sensor-visual{display:flex;align-items:center;justify-content:center;}" // Контейнер пиктограммы уровнемера.
      ".sensor-tank{position:relative;width:78px;height:112px;border:2px solid #5f7d95;border-radius:10px 10px 14px 14px;background:linear-gradient(180deg,#07111b,#0b1721);box-shadow:inset 0 0 14px rgba(0,0,0,0.65),0 10px 22px rgba(0,0,0,0.35);overflow:hidden;}" // Бак уровнемера.
      ".sensor-tank:before{content:'';position:absolute;left:8px;right:8px;top:8px;height:14px;border-radius:50%;background:rgba(255,255,255,0.08);z-index:2;}" // Блик на баке.
      ".sensor-water{position:absolute;left:7px;right:7px;bottom:7px;height:58%;border-radius:8px 8px 12px 12px;background:linear-gradient(180deg,#36ccff,#0878c7);box-shadow:0 -6px 16px rgba(63,207,255,0.3);transition:height .25s ease,background .25s ease;}" // Вода в пиктограмме уровнемера.
      ".sensor-water:before{content:'';position:absolute;left:0;right:0;top:-5px;height:10px;border-radius:50%;background:rgba(150,238,255,0.55);}" // Поверхность воды.
      ".sensor-mark{position:absolute;left:0;right:0;height:2px;background:rgba(255,255,255,0.55);z-index:4;}" // Контрольная отметка уровнемера.
      ".sensor-mark.high{top:24px}.sensor-mark.low{bottom:28px}" // Верхняя и нижняя отметки.
      ".sensor-probe{position:absolute;right:12px;top:10px;bottom:10px;width:4px;border-radius:4px;background:#b7c8d8;z-index:5;}" // Направляющая поплавкового датчика.
      ".sensor-float{position:absolute;right:5px;width:18px;height:18px;border-radius:50%;background:#ffd166;border:2px solid #ffe9a8;box-shadow:0 0 10px rgba(255,209,102,0.55);z-index:6;transition:bottom .25s ease,background .25s ease;}" // Поплавок датчика.
      ".level-sensor-info label{margin-bottom:8px;color:#dce6f2;font-weight:700;}" // Заголовок карточки уровнемера.
      ".level-sensor-status{font-size:1.12em;line-height:1.35;color:#fff;font-weight:800;}" // Текст статуса уровнемера.
      ".level-sensor-hint{margin-top:6px;color:#9fb4c8;font-size:0.82em;}" // Короткая подсказка к пиктограмме.
      ".sensor-upper.sensor-reached .sensor-water{height:82%;}.sensor-upper.sensor-reached .sensor-float{bottom:82px;background:#40e07b;border-color:#b7ffd0;}" // Верхний уровень достигнут.
      ".sensor-upper.sensor-below .sensor-water{height:58%;}.sensor-upper.sensor-below .sensor-float{bottom:58px;background:#ffd166;}" // Вода ниже верхнего датчика.
      ".sensor-lower.sensor-normal .sensor-water{height:62%;}.sensor-lower.sensor-normal .sensor-float{bottom:62px;background:#40e07b;border-color:#b7ffd0;}" // Нижний датчик в норме.
      ".sensor-lower.sensor-low .sensor-water{height:20%;background:linear-gradient(180deg,#ff8f70,#b92f2f);}.sensor-lower.sensor-low .sensor-float{bottom:22px;background:#ff5f5f;border-color:#ffc0c0;}" // Нижний уровень подтвержден.
      ".sensor-drain.sensor-empty .sensor-water{height:28%;}.sensor-drain.sensor-empty .sensor-float{bottom:30px;background:#40e07b;border-color:#b7ffd0;}" // Сливная яма свободна.
      ".sensor-drain.sensor-full .sensor-water{height:86%;background:linear-gradient(180deg,#ffb36b,#d64545);}.sensor-drain.sensor-full .sensor-float{bottom:86px;background:#ff5f5f;border-color:#ffc0c0;}" // Сливная яма заполнена.
      ".sensor-low .level-sensor-status,.sensor-full .level-sensor-status{color:#ffb4a8;}.sensor-reached .level-sensor-status,.sensor-normal .level-sensor-status,.sensor-empty .level-sensor-status{color:#baffce;}" // Цвет текста по состоянию.
      ".graph-tooltip{position:fixed;pointer-events:none;background:rgba(10,14,20,0.9);color:#eef4ff;padding:6px 10px;border:1px solid rgba(255,255,255,0.18);border-radius:6px;font-size:0.8em;z-index:1000;transform:translate(-50%,-120%);white-space:nowrap;box-shadow:0 4px 12px rgba(0,0,0,0.35);}"  // Всплывающая подсказка графика
      ".graph-tooltip.hidden{display:none;}"  // Скрытое состояние tooltip
      ".card:has(#ModeSelect),.card:has(#LEDColor),.card:has(#Timer1),"  // Специальные карточки UI
      ".card:has(#FloatInput),.card:has(#IntInput),"
      ".card:has(#RandomVal),.card:has(#DaysSelect),.card:has(#CommentClean){"
      "background:linear-gradient(145deg,#111,#080b13);border:1px solid rgba(159,180,255,0.25);" // Фон и рамка специальных карточек
      "box-shadow:0 18px 34px rgba(0,0,0,0.65);border-radius:18px;padding:14px 16px;" // Тени и скругления
      "transition:transform 0.18s ease,box-shadow 0.18s ease;" // Анимации карточек
      "position:relative;overflow:hidden;" // Контекст для эффектов
      "} " // Конец блока специальных карточек

      ".card:has(#ModeSelect):hover,.card:has(#LEDColor):hover,.card:has(#Timer1):hover," // Hover-эффект для ключевых карточек
        ".card:has(#FloatInput) label,.card:has(#IntInput) label," // Акцент на label в карточках ввода
      ".card:has(#RandomVal):hover,.card:has(#DaysSelect):hover,.card:has(#CommentClean):hover{" // Hover для случайного значения и дней
      "transform:translateY(-2px);box-shadow:0 22px 40px rgba(0,0,0,0.7);}" // Подъём карточки и усиленная тень
      ".card:has(#ModeSelect) label,.card:has(#LEDColor) label,.card:has(#Timer1) label," // Заголовки карточек
      ".card:has(#FloatInput) label,.card:has(#IntInput) label,.card:has(#CurrentTime) label," // Заголовки карточек ввода и времени
      ".card:has(#RandomVal) label,.card:has(#DaysSelect) label,.card:has(#CommentClean) label{" // Заголовки карточек значений и дней
      "font-size:0.72em;letter-spacing:0.08em;text-transform:uppercase;color:#94b4d6;" // Стиль заголовков карточек
      "} "
        ".time-settings-card{display:flex;flex-direction:column;gap:12px;align-items:stretch;} " // Карточка настройки времени
      ".time-settings-card .stat-group{gap:12px;} " // Отступы в группе времени
      ".time-settings-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px;width:100%;align-items:end;} " // Сетка полей времени
      ".time-settings-grid label{margin-bottom:6px;font-size:0.78em;letter-spacing:0.08em;text-transform:uppercase;color:#c1d0e2;} " // Подписи полей времени
      ".time-settings-field{display:flex;flex-direction:column;gap:6px;} " // Поле времени
      ".time-settings-field input,.time-settings-field select{width:100%;min-width:0;} " // Поля на всю ширину
      ".time-settings-actions{display:flex;flex-wrap:wrap;gap:10px;align-items:center;justify-content:flex-start;} " // Панель действий
      ".time-settings-status{font-size:0.85em;color:#9fb4c8;} " // Статус настройки времени
      "@media (max-width:680px){.time-settings-grid{grid-template-columns:1fr;}} " // Адаптация сетки времени
      ".card:has(#ModeSelect) select,.card:has(#LEDColor) input," // Поля выбора режима и цвета LED
      ".card:has(#Timer1) input,.card:has(#FloatInput) input,.card:has(#IntInput) input{" // Поля таймера и числового ввода
      "background:#06070c;border:1px solid rgba(255,255,255,0.12);border-radius:10px;" // Фон и рамка полей
      "color:#eef2ff;font-weight:600;padding:10px 12px;box-shadow:inset 0 0 0 1px rgba(255,255,255,0.03);" // Текст и внутренняя подсветка
      "width:100%;} " // Поля на всю ширину

      ".card:has(#LEDColor){" // Специальная карточка выбора цвета LED
      "--led-color:#33b8ff;" // CSS-переменная текущего цвета LED
      "background:linear-gradient(135deg,rgba(15,18,35,0.98),rgba(11,16,28,0.98));" // Фон карточки LED
      "border:1px solid rgba(110,173,255,0.35);" // Рамка карточки LED
      "box-shadow:0 16px 32px rgba(0,0,0,0.7),0 0 0 1px rgba(255,255,255,0.05);" // Глубокая тень карточки
      "padding:16px 18px;gap:10px;" // Отступы и расстояния внутри
      "}"
      ".card:has(#LEDColor)::before{" // Внутренний световой эффект
      "content:'';position:absolute;inset:8px;" // Позиционирование эффекта
      "background:radial-gradient(circle at 30% 25%,rgba(255,255,255,0.08),transparent 55%);" // Светлое пятно
      "pointer-events:none;z-index:0;" // Не влияет на взаимодействие
      "}"
      ".card:has(#LEDColor)::after{" // Внешнее цветовое свечение
      "content:'';position:absolute;inset:-50%;" // Выход за границы карточки
      "background:radial-gradient(circle at 50% 50%,var(--led-color),transparent 50%);" // Цветовое свечение от LED
      "opacity:0.25;filter:blur(32px);pointer-events:none;z-index:0;" // Размытие и прозрачность
      "}"
      ".card:has(#LEDColor) label{" // Заголовок карточки LED
      "display:flex;align-items:center;justify-content:space-between;gap:10px;" // Раскладка заголовка
      "letter-spacing:0.09em;color:#b4d6ff;position:relative;z-index:1;" // Цвет и приоритет слоя
      "}"
      ".card:has(#LEDColor) label::after{" // Отображение HEX-кода цвета
      "content:attr(data-color);font-family:'JetBrains Mono','SFMono-Regular',monospace;" // Моноширинный шрифт
      "font-size:0.82em;padding:4px 9px;border-radius:9px;" // Размеры бейджа
      "background:rgba(255,255,255,0.06);color:#eaf4ff;" // Фон и цвет текста
      "box-shadow:inset 0 0 0 1px rgba(255,255,255,0.08);" // Внутренняя рамка
      "}"
      "#sidebar .card:has(#ThemeColor){" // Карточка выбора темы в сайдбаре
      "--theme-color:#6dd5ed;" // CSS-переменная цвета темы
      "background:linear-gradient(145deg,rgba(16,20,32,0.96),rgba(10,14,24,0.96));" // Фон карточки темы
      "border:1px solid rgba(126,193,255,0.32);" // Рамка карточки темы
      "box-shadow:0 14px 26px rgba(0,0,0,0.65),0 0 0 1px rgba(255,255,255,0.05);" // Тень карточки
      "padding:14px 16px;position:relative;overflow:hidden;gap:8px;" // Отступы и контекст эффектов
      "}"
      "#sidebar .card:has(#ThemeColor)::before{" // Внутренний световой эффект темы
      "content:'';position:absolute;inset:10px;" // Позиционирование эффекта
      "background:radial-gradient(circle at 22% 30%,rgba(255,255,255,0.08),transparent 55%);" // Световой градиент
      "pointer-events:none;" // Без влияния на клики
      "}"
      "#sidebar .card:has(#ThemeColor)::after{" // Внешнее цветовое свечение темы
      "content:'';position:absolute;inset:-55%;" // Выход за границы
      "background:radial-gradient(circle at 50% 45%,var(--theme-color),transparent 52%);" // Цвет темы
      "opacity:0.2;filter:blur(28px);pointer-events:none;" // Размытие и прозрачность
      "}"
      "#sidebar .card:has(#ThemeColor) label{" // Заголовок карточки темы
      "display:flex;align-items:center;justify-content:space-between;gap:10px;" // Раскладка заголовка
      "letter-spacing:0.09em;color:#b7dbff;font-size:0.72em;text-transform:uppercase;" // Стиль текста
      "position:relative;z-index:1;" // Поверх эффектов
      "}"
      "#sidebar .card:has(#ThemeColor) label::after{" // Отображение HEX-кода темы
      "content:attr(data-color);font-family:'JetBrains Mono','SFMono-Regular',monospace;" // Моноширинный шрифт
      "font-size:0.78em;padding:4px 8px;border-radius:9px;" // Размер бейджа
      "background:rgba(255,255,255,0.06);color:#eaf4ff;" // Цвета бейджа
      "box-shadow:inset 0 0 0 1px rgba(255,255,255,0.08);" // Внутренняя рамка
      "}"
      "#ThemeColor{" // Сам input выбора цвета темы
      "height:50px;padding:0 10px;border-radius:12px;" // Размер и форма input
      "background:linear-gradient(145deg,#0c101b,#10182a);" // Фон input
      "border:1px solid rgba(255,255,255,0.16);" // Рамка input
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.1),0 12px 22px rgba(0,0,0,0.52);" // Тени input
      "cursor:pointer;position:relative;z-index:1;" // Поведение и слой
      "}"
      "#ThemeColor::-webkit-color-swatch-wrapper{padding:6px;border-radius:10px;}" // Обёртка color input (WebKit)
      "#ThemeColor::-webkit-color-swatch{border-radius:9px;border:1px solid rgba(255,255,255,0.16);" // Сам цветовой свотч (WebKit)
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.22);}" // Внутренняя рамка свотча
      "#ThemeColor::-moz-color-swatch{border-radius:9px;border:1px solid rgba(255,255,255,0.16);" // Цветовой свотч (Firefox)
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.22);}" // Внутренняя рамка свотча
      "#LEDColor{" // Input выбора цвета LED
      "height:58px;padding:0 10px;border-radius:14px;" // Размеры и форма input
      "background:linear-gradient(145deg,#0b101a,#101727);" // Фон input LED
      "border:1px solid rgba(255,255,255,0.18);" // Рамка input LED
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.12),0 14px 26px rgba(0,0,0,0.55);" // Внутренняя и внешняя тень
      "cursor:pointer;position:relative;z-index:1;" // Поведение курсора и слой
      "}"
      "#LEDColor::-webkit-color-swatch-wrapper{padding:6px;border-radius:12px;}" // Обёртка color input LED (WebKit)
      "#LEDColor::-webkit-color-swatch{border-radius:10px;border:1px solid rgba(255,255,255,0.18);" // Свотч LED (WebKit)
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.25);}" // Внутренняя рамка
      "#LEDColor::-moz-color-swatch{border-radius:10px;border:1px solid rgba(255,255,255,0.18);" // Свотч LED (Firefox)
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.25);}" // Внутренняя рамка

       ".card:has(#RandomVal) #RandomVal{" // Отображение случайного значения
      "font-size:1.5em;font-weight:700;color:#ffffff;text-shadow:0 4px 12px rgba(0,0,0,0.45);margin-top:6px;" // Акцентированное значение
      "} "
       ".card:has(#DaysSelect) .select-days{--day-accent:#4cc3ff;}" // Цвет акцента для выбора дней
      ".card:has(#CommentClean){--comment-accent:#4cc3ff;}" // Цвет акцента для этапа промывки
      ".card:has(#CommentClean) #CommentClean{" // Выделение этапа промывки
      "font-size:2em;font-weight:700;color:#e8f6ff;background:linear-gradient(145deg,rgba(12,30,52,0.9),rgba(8,14,24,0.9));"
      "border:1px solid rgba(76,195,255,0.45);box-shadow:0 0 0 1px rgba(255,255,255,0.04),0 10px 24px rgba(0,0,0,0.45),0 0 18px rgba(76,195,255,0.35);"
      "padding:10px 12px;border-radius:12px;}"
      ".stats-card{display:flex;flex-direction:column;gap:14px;} " // Карточка статистики
      ".stat-group{display:flex;flex-direction:column;gap:8px;} " // Группа статистики
      ".stat-heading{font-size:0.82em;color:#9fb4c8;letter-spacing:0.05em;text-transform:uppercase;padding-left:2px;} " // Заголовок группы статистики
      ".stat-list{list-style:none;padding:0;margin:0;display:flex;flex-direction:column;gap:8px;} " // Список статистики
      ".stat-list li{display:flex;justify-content:space-between;align-items:center;padding:8px 10px;border-radius:10px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.06);} " // Элемент статистики
      ".stat-list span{color:#9fb4c8;font-size:0.9em;} " // Название параметра
      ".stat-list strong{color:#fff;font-family:'JetBrains Mono','SFMono-Regular',monospace;font-size:0.95em;} " // Значение параметра
      ".btn-primary{display:inline-flex;align-items:center;gap:8px;padding:10px 18px;border-radius:12px;border:1px solid rgba(255,255,255,0.12);background:linear-gradient(135deg,#3a7bd5,#00d2ff);color:#fff;font-weight:700;letter-spacing:0.03em;text-transform:uppercase;box-shadow:0 12px 26px rgba(0,0,0,0.35),0 0 0 1px rgba(255,255,255,0.08);cursor:pointer;transition:transform 0.12s ease,box-shadow 0.12s ease;} " // Основная кнопка
      ".btn-primary:hover{transform:translateY(-1px);box-shadow:0 16px 30px rgba(0,0,0,0.45);} " // Hover основной кнопки
      ".btn-secondary{padding:8px 14px;border-radius:10px;border:1px solid rgba(255,255,255,0.12);background:rgba(255,255,255,0.05);color:#e2e6f0;font-weight:600;cursor:pointer;transition:background 0.15s ease,transform 0.12s ease;} " // Вторичная кнопка
      ".btn-secondary:hover{background:rgba(255,255,255,0.1);transform:translateY(-1px);} " // Hover вторичной кнопки
      ".btn-secondary:disabled{opacity:0.6;cursor:progress;} " // Заблокированная кнопка
      ".btn-danger{padding:8px 14px;border-radius:10px;border:1px solid rgba(255,87,87,0.25);background:linear-gradient(135deg,#ff5f6d,#ffc371);color:#0c0f16;font-weight:700;cursor:pointer;box-shadow:0 8px 18px rgba(255,95,109,0.35);transition:transform 0.12s ease,box-shadow 0.12s ease;} " // Кнопка опасного действия
      ".btn-danger:hover{transform:translateY(-1px);box-shadow:0 12px 26px rgba(255,95,109,0.45);} " // Hover опасной кнопки
      ".btn-danger:disabled{opacity:0.65;cursor:progress;} " // Заблокированная опасная кнопка
      ".stats-actions{display:flex;gap:10px;flex-wrap:wrap;margin-bottom:10px;} " // Панель действий статистики
      ".wifi-card{display:flex;flex-direction:column;gap:10px;} " // Карточка Wi-Fi
      ".wifi-field label{margin-bottom:6px;font-size:0.85em;color:#9fb4c8;text-transform:uppercase;letter-spacing:0.06em;} " // Подписи полей Wi-Fi
      ".input-with-action{display:flex;gap:8px;align-items:center;} " // Поле с кнопкой действия
      ".wifi-actions{display:flex;align-items:center;gap:14px;margin-top:10px;flex-wrap:wrap;} " // Кнопки Wi-Fi
      ".wifi-hint{color:#9fb4c8;font-size:0.9em;} " // Подсказка Wi-Fi
      ".wifi-status-card .stat-list li{background:rgba(0,0,0,0.3);} " // Статусная карточка Wi-Fi
      ".section-title{margin-top:10px;margin-bottom:6px;font-size:1em;color:#cfd7e0;} " // Заголовок секции
      ".wifi-modal{position:fixed;inset:0;background:rgba(0,0,0,0.55);display:flex;align-items:flex-start;justify-content:center;padding-top:60px;z-index:1200;} " // Модальное окно Wi-Fi
      ".wifi-modal.hidden{display:none;} " // Скрытое состояние модального окна
      ".wifi-modal-content{width:min(480px,90vw);background:#1a1c22;border:1px solid rgba(255,255,255,0.08);border-radius:14px;box-shadow:0 18px 40px rgba(0,0,0,0.65);overflow:hidden;} " // Контент Wi-Fi модалки
      ".wifi-modal-header{display:flex;align-items:center;justify-content:space-between;padding:12px 14px;border-bottom:1px solid rgba(255,255,255,0.06);} " // Заголовок Wi-Fi модалки
      ".wifi-scan-list{max-height:320px;overflow:auto;display:flex;flex-direction:column;} " // Список сетей Wi-Fi
       
            ".dash-modal{position:fixed;inset:0;background:rgba(0,0,0,0.6);display:flex;align-items:flex-start;justify-content:center;padding:60px 20px;z-index:1400;} " // Универсальная модалка дашборда
      ".dash-modal.hidden{display:none;} " // Скрытая модалка
      ".dash-modal-content{width:min(880px,95vw);background:#1a1c22;border:1px solid rgba(255,255,255,0.08);border-radius:16px;box-shadow:0 20px 50px rgba(0,0,0,0.7);overflow:hidden;} " // Контент модалки
      ".dash-modal-header{display:flex;align-items:center;justify-content:space-between;padding:14px 16px;border-bottom:1px solid rgba(255,255,255,0.08);} " // Заголовок модалки
      ".dash-modal-body{padding:16px;max-height:calc(100vh - 180px);overflow:auto;} " // Тело модалки
      ".popup-grid{display:flex;flex-direction:column;gap:15px;position:relative;width:100%;} " // Сетка popup-карточек
      ".popup-grid .card{width:100%;} " // Карточки внутри popup
      ".dash-modal[data-popup='Cal_PH'] .popup-grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:28px 14px;align-items:stretch;} "
      ".dash-modal[data-popup='Cal_PH'] .popup-grid .card{width:auto;min-width:0;} "
      ".dash-modal[data-popup='Cal_PH'] .popup-grid .card:nth-child(1),.dash-modal[data-popup='Cal_PH'] .popup-grid .card:nth-child(n+6){grid-column:1/-1;} "

            ".network-row{display:flex;flex-direction:column;align-items:flex-start;gap:4px;padding:12px 14px;background:transparent;border:none;border-bottom:1px solid rgba(255,255,255,0.05);color:#e9ecf4;text-align:left;cursor:pointer;transition:background 0.12s ease;} " // Строка сети Wi-Fi
      ".network-row:hover{background:rgba(255,255,255,0.04);} " // Hover подсветка строки сети
      ".network-ssid{font-weight:700;} " // Имя сети (SSID)
      ".network-meta{color:#9fb4c8;font-size:0.9em;} " // Дополнительная информация о сети
      ".empty-row{padding:16px;color:#9fb4c8;text-align:center;} " // Пустое состояние списка
      ".icon-btn{background:none;border:none;color:#fff;font-size:1.4em;cursor:pointer;line-height:1;} " // Иконка-кнопка без фона

      ".mqtt-grid{display:flex;flex-direction:column;gap:12px;} " // Сетка настроек MQTT
      ".mqtt-field{display:flex;flex-direction:column;gap:6px;} " // Поле настроек MQTT
      ".mqtt-page{flex-direction:column;gap:12px;} " // Страница MQTT карточками
      ".page.mqtt-page.active{display:flex;} " // MQTT показывается только как активная страница
      ".mqtt-page h3{margin:0;} " // Заголовок MQTT
      ".mqtt-page .card{margin:0;} " // Ровные отступы MQTT карточек
      ".mqtt-enable-line{display:flex;align-items:center;gap:6px;font-size:0.95em;} " // Чекбокс MQTT
      ".mqtt-enable-line input{width:16px;height:16px;margin:0;} " // Размер чекбокса MQTT
      ".mqtt-status-text{font-size:1.1em;font-weight:800;margin-top:6px;color:#eaf4ff;} " // Статус MQTT
      ".mqtt-status-text.off{color:#ffb3b3;} " // Отключенный MQTT
      ".mqtt-status-text.wait{color:#ffe4a6;} " // Ожидание MQTT
      ".mqtt-actions{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-top:8px;} " // Кнопки действий MQTT
      ".profile-hint{color:#9fb4c8;font-size:0.9em;} " // Подсказка профиля
      ".esp-link-card{background:#101722;border:1px solid rgba(91,141,196,0.65);border-radius:10px;padding:10px 8px;margin:10px 0 12px;box-shadow:0 10px 24px rgba(0,0,0,0.35);display:flex;flex-direction:column;gap:8px;} "
      ".esp-link-title{font-size:0.78rem;color:#bcd6f5;text-transform:uppercase;letter-spacing:0.05em;} "
      ".esp-link-state{border-radius:8px;border:1px solid #3ad76f;background:#0f5b2a;color:#fff;font-weight:900;text-align:center;padding:12px 8px;font-size:1rem;} "
      ".esp-link-card.off{border-color:#7a2626;background:#221114;} "
      ".esp-link-card.off .esp-link-state{border-color:#b93434;background:#6b1616;color:#fff;} "
      ".esp-link-last{font-size:0.78rem;color:#94a8bd;} "
      ".btn-toggle-on{background:linear-gradient(135deg,#4caf50,#81c784);color:#0b0f14;} " // Кнопка включённого состояния
      ".btn-mqtt{position:relative;overflow:hidden;background:linear-gradient(135deg,#1f2a44,#263555);border:1px solid rgba(111,168,255,0.35);color:#e6edff;box-shadow:0 12px 24px rgba(0,0,0,0.4);} " // Базовая кнопка MQTT
      ".btn-mqtt:before{content:'';position:absolute;inset:0;opacity:0;pointer-events:none;background:radial-gradient(circle at 20% 20%,rgba(255,255,255,0.28),transparent 45%);transition:opacity 0.18s ease;} " // Световой эффект кнопки MQTT
      ".btn-mqtt:hover:before{opacity:1;} " // Активация эффекта при наведении
      ".btn-mqtt.btn-warn{background:linear-gradient(135deg,#2d1e12,#3c2a18);border-color:rgba(255,193,7,0.45);color:#ffe9b3;} " // MQTT кнопка предупреждения
      ".btn-mqtt.btn-success{background:linear-gradient(135deg,#123420,#1d4b2a);border-color:rgba(76,175,80,0.55);color:#d5ffde;} " // MQTT кнопка успеха
      ".btn-mqtt.btn-activate-off{background:linear-gradient(135deg,#1b2b52,#23406f);border-color:rgba(64,139,255,0.55);color:#e5efff;} " // MQTT кнопка выключенного состояния
      ".btn-mqtt.btn-activate-on{background:linear-gradient(135deg,#0f3b1f,#13532a);border-color:rgba(76,175,80,0.6);color:#d8ffe4;} " // MQTT кнопка включённого состояния
   
      "@media (max-width:1024px){" // Адаптация под ноутбуки/планшеты
      "#sidebar{width:200px;} " // Узкая боковая панель
      "#main{margin-left:200px;padding:16px;} " // Уменьшенные отступы контента
      "#toggleBtn{left:200px;} " // Смещение кнопки меню
      "} "
      "@media (max-width:860px){" // Адаптация под мобильные/узкие экраны
      "#sidebar{width:240px;transform:translateX(0);z-index:1100;} " // Сайдбар поверх контента
      "body.sidebar-hidden #sidebar{transform:translateX(-100%);} " // Скрытие сайдбара сдвигом
      "body.sidebar-hidden #main{margin-left:0;} " // Контент без отступа
      "#main{margin-left:0;padding:14px;} " // Контент во всю ширину
      "#toggleBtn{left:14px;border-radius:8px;} " // Кнопка меню ближе к краю
      ".page{grid-template-columns:repeat(auto-fill,minmax(220px,1fr));} " // Более компактная сетка карточек
      "} "
      "@media (max-width:640px){" // Мобильные телефоны
      "#main{padding:12px;height:auto;min-height:100vh;} " // Меньше отступов и авто-высота
      ".page{grid-template-columns:1fr;gap:12px;} " // Карточки в один столбец
      ".page-header{flex-direction:column;align-items:flex-start;} " // Вертикальный заголовок
      ".page-datetime{margin-left:0;width:100%;justify-content:flex-start;} " // Дата/время на всю ширину
      ".graph-controls{flex-wrap:wrap;gap:12px;} " // Панель графика в несколько строк
      ".graph-controls .control-group{min-width:0;flex:1 1 180px;} " // Сжатие групп контролов
      "input[type=time]{width:100%;} " // Поля времени на всю ширину
      "label:has(input[type=checkbox]){width:100%;} " // Чекбокс на всю ширину
      ".time-settings-actions,.mqtt-actions{flex-direction:column;align-items:flex-start;} " // Кнопки действий столбиком
      ".wifi-modal{padding:20px 12px;} " // Меньше отступов модалки Wi-Fi
      ".dash-modal{padding:40px 12px;} " // Модалки ближе к краям
      ".dash-modal-body{max-height:calc(100vh - 160px);} " // Корректировка высоты модалки
      ".dash-modal[data-popup='Cal_PH'] .popup-grid{grid-template-columns:1fr;gap:15px;} "
      "} "
      "</style></head><body>"; // Завершение стилей и начало body

    // Sidebar
      html += "<div id='sidebar'>"; // Начало боковой панели
      bool first = true; // Флаг для первой вкладки (активной по умолчанию)
      for(auto &t : self->tabs){ // Перебор зарегистрированных вкладок
          html += "<button onclick=\"showPage('"+t.id+"',this)\""; // Кнопка вкладки с обработчиком
        if(first){ html += " class='active'"; first=false; } // Первая вкладка делается активной
        html += ">"+t.title+"</button>"; // Заголовок вкладки
      }
      html += "<hr>";
      bool themeRendered = false;
      for(auto &block : sidebarBlocks){
        if(block.type == "espStatus"){
          html += "<div class='esp-link-card off' id='"+block.id+"'>"
                  "<div class='esp-link-title'>"+block.label+"</div>"
                  "<div class='esp-link-state' id='"+block.id+"_state'>Нет связи с ESP32</div>"
                  "<div class='esp-link-last' id='"+block.id+"_last'>Последний отклик: --</div>"
                  "</div>";
        } else if(block.type == "themeColor"){
          html += "<div class='card'><label>"+block.label+"</label>"
                  "<input id='"+block.id+"' type='color' value='"+ThemeColor+"'></div>";
          themeRendered = true;
        }
      }
      if(!themeRendered){
        html += "<div class='card'><label>Theme color</label><input id='ThemeColor' type='color' value='"+ThemeColor+"'></div>";
      }
      if(systemPageDecls.empty()){
        html += "<button onclick=\"showPage('wifi',this)\">WiFi Settings</button>";
        html += "<button onclick=\"showPage('stats',this)\">Statistics</button>";
        html += "<button onclick=\"showPage('profile',this)\">Профиль</button>";
        html += "<button onclick=\"showPage('mqtt',this)\">Настройка MQTT</button>";
      } else {
        for(auto &page : systemPageDecls){
          html += "<button onclick=\"showPage('"+page.id+"',this)\">"+page.title+"</button>";
        }
      }
      html += "</div>"; // Конец боковой панели

      html += "<button id='toggleBtn' onclick='toggleSidebar()'></button>"; // Кнопка сворачивания сайдбара - кнопка боковой панели
      // Основной контент
      html += "<div id='main'>"; // Начало основной области

      auto renderTabElements = [&](const String &tabId){ // Лямбда для рендера элементов вкладки
        const uint32_t renderElementsStartMs = millis(); // Старт таймера рендера элементов конкретной вкладки
        uint16_t renderedCount = 0; // Счётчик отрисованных элементов для контроля полноты вывода
        for(auto &e : self->elements){ // Перебор всех UI-элементов
        if(e.tab != tabId || e.type != "image") continue; // Фильтр по вкладке и типу image

              String imgSrc = e.label; // Источник изображения
              if(!imgSrc.startsWith("http") && !imgSrc.startsWith("/")) imgSrc = "/" + imgSrc; // Приведение к относительному пути
              if(e.id == "Image1") imgSrc = currentBasinImagePath(); // Главная картинка бассейна зависит от RGB и лампы.
              if(imgSrc == "/getImage") imgSrc = "/getImage"; // Специальный endpoint изображения

              String widthRaw, heightRaw, leftRaw, topRaw, extraStyles; // Параметры размеров и позиционирования

              auto ensureUnit = [&](const String &raw)->String { // Приведение чисел к CSS-единицам
                  String trimmed = raw;
                  trimmed.trim(); // Удаление пробелов
                  if(trimmed.length() == 0) return trimmed; // Пустое значение
                  bool hasUnit = false;
                  for(int i=0;i<trimmed.length();i++){ // Проверка наличия единиц измерения
                      char c = trimmed[i];
                      if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                  }
                  return hasUnit ? trimmed : trimmed + "px"; // Добавление px при необходимости
              };

              String style = e.value; // Строка стилей элемента
              if(style.length() && style[style.length()-1] != ';') style += ';'; // Гарантия завершающего ;
              int tokenStart = 0;
              while(tokenStart < style.length()){ // Разбор CSS-токенов
                  int tokenEnd = style.indexOf(';', tokenStart);
                  if(tokenEnd < 0) tokenEnd = style.length();
                  String token = style.substring(tokenStart, tokenEnd);
                  token.trim();
                  if(token.length()){
                      int sep = token.indexOf(':');
                      if(sep > 0){
                          String key = token.substring(0, sep); key.trim(); // CSS-свойство
                          String val = token.substring(sep + 1); val.trim(); // Значение свойства
                          if(key.equalsIgnoreCase("width")) widthRaw = ensureUnit(val); // Ширина
                          else if(key.equalsIgnoreCase("height")) heightRaw = ensureUnit(val); // Высота
                          else if(key.equalsIgnoreCase("left")) leftRaw = ensureUnit(val); // Смещение слева
                          else if(key.equalsIgnoreCase("top")) topRaw = ensureUnit(val); // Смещение сверху
                          else extraStyles += key + ":" + val + ";"; // Прочие стили
                      } else {
                          extraStyles += token + ";"; // Свойство без значения
                      }
                  }
                  tokenStart = tokenEnd + 1; // Переход к следующему токену
              }

              bool hasCoords = leftRaw.length() || topRaw.length(); // Проверка абсолютных координат
              bool positionProvided = extraStyles.indexOf("position:") >= 0; // Явно задан position

              String imageStyle = "display:block; width:auto; height:auto; border-radius:12px; box-shadow:0 4px 12px rgba(0,0,0,0.5);"; // Базовый стиль изображения
              imageStyle += widthRaw.length() ? "width:"+widthRaw+";" : "max-width:100%;"; // Ширина изображения
              imageStyle += heightRaw.length() ? "height:"+heightRaw+";" : "height:auto;"; // Высота изображения
              imageStyle += extraStyles; // Пользовательские стили
              if(!positionProvided) imageStyle += hasCoords ? "position:absolute;" : "position:relative;"; // Позиционирование
              imageStyle += " z-index:1; object-fit:contain;"; // Слой и масштабирование

              String containerStyle = "position:relative; text-align:center; display:inline-flex; align-items:center; justify-content:center;" // Контейнер изображения
                                       " padding:0; width:fit-content; height:fit-content; background:transparent; border:none; box-shadow:none; margin:0 auto 14px auto;";
              if(widthRaw.length()) containerStyle += " width:"+widthRaw+";"; // Фиксация ширины контейнера
              if(heightRaw.length()) containerStyle += " height:"+heightRaw+";"; // Фиксация высоты контейнера
              if(hasCoords){
                  containerStyle += " position:absolute;"; // Абсолютное позиционирование контейнера
                  if(leftRaw.length()) containerStyle += " left:"+leftRaw+";"; // Смещение слева
                  if(topRaw.length()) containerStyle += " top:"+topRaw+";"; // Смещение сверху
                  containerStyle += " margin:0;"; // Убираем авто-отступы
              }

              html += "<div class=\"card\" style=\""+containerStyle+"\">"; // Карточка контейнера изображения
              html += "<img id='"+e.id+"' src='"+imgSrc+"' data-refresh='"+(imgSrc=="/getImage"?"getImage":"")+"' " // Тег изображения
                      "style='"+imageStyle+"'/>"; // Применение стилей изображения

       html += "</div>"; // Закрытие карточки изображения
          }

          for(auto &overlay : self->elements){ // Перебор элементов для наложений
              if(overlay.tab != tabId || overlay.type != "displayStringAbsolute") continue; // Фильтр по вкладке и типу
              String styleStr = overlay.value; // CSS-строка стилей
              auto readProp = [&](const String &prop)->String { // Чтение CSS-свойства из строки
                  String key = prop + ":";
                  int idx = styleStr.indexOf(key);
                  if(idx < 0) return "";
                  int start = idx + key.length();
                  int end = styleStr.indexOf(";", start);
                  if(end < 0) end = styleStr.length();
                  return styleStr.substring(start, end);
              };

              auto normalizeCoord = [&](const String &raw)->String { // Приведение координат к CSS-единицам
                  if(raw.length() == 0) return raw;
                  bool hasUnit = false;
                  for(int i=0;i<raw.length();i++){
                      char c = raw[i];
                      if((c>='0' && c<='9') || c=='-' || c=='.') continue;
                      hasUnit = true;
                      break;
                  }
                  return hasUnit ? raw : raw + "px";
              };

              String fontSizeStr = readProp("fontSize"); // Размер шрифта
              int fontSize = fontSizeStr.length() ? fontSizeStr.toInt() : 24; // Значение по умолчанию
              String color = readProp("color"); if(color.length() == 0) color = "#00ff00"; // Цвет текста
              String bgColor = readProp("bg"); if(bgColor.length() == 0) bgColor = "rgba(0,0,0,0.65)"; // Цвет фона
              String padding = readProp("padding"); if(padding.length() == 0) padding = "6px 12px"; // Внутренние отступы
              String borderRadius = readProp("borderRadius"); if(borderRadius.length() == 0) borderRadius = "14px"; // Скругление углов
              String leftRaw = readProp("x"); // Координата X
              String topRaw = readProp("y"); // Координата Y
                            String whiteSpace = readProp("white-space");
              if(whiteSpace.length() == 0) whiteSpace = readProp("whiteSpace");
              if(whiteSpace.length() == 0) whiteSpace = "nowrap";
              bool hasLeft = leftRaw.length(); // Есть X
              bool hasTop = topRaw.length(); // Есть Y
              String leftValue = hasLeft ? normalizeCoord(leftRaw) : "50%"; // Позиция по X
              String topValue = hasTop ? normalizeCoord(topRaw) : "50%"; // Позиция по Y
              String translateX = hasLeft ? "0%" : "-50%"; // Центрирование по X
              String translateY = hasTop ? "0%" : "-50%"; // Центрирование по Y
              String transform = "translate("+translateX+", "+translateY+")"; // CSS transform
              String panelStyle = "position:absolute; left:"+leftValue+"; top:"+topValue+"; transform:"+transform+"; " // Итоговый стиль панели
                                   "background:"+bgColor+"; color:"+color+"; font-size:"+String(fontSize)+"px; padding:"+padding+"; "
                                   "border-radius:"+borderRadius+"; display:inline-flex; align-items:center; justify-content:center; "
                                   "text-align:center; box-sizing:border-box; white-space:"+whiteSpace+"; max-width:90%; "
                                   "box-shadow:0 10px 20px rgba(0,0,0,0.45); z-index:2;";

              html += "<div id='"+overlay.id+"' style='"+panelStyle+"'></div>"; // Вывод абсолютного текстового блока
            }



          for(auto &e : self->elements){ // Перебор всех элементов вкладки
              if(e.tab != tabId) continue; // Фильтр по вкладке
              if(e.type=="displayStringAbsolute" || e.type=="image") continue; // Пропуск уже обработанных типов
              if(e.type=="displayGraph" || e.type=="displayGraphJS"){ // Графики
                  String config = e.value; // Конфигурация графика
                  auto readSetting = [&](const String &prop)->String { // Чтение параметра
                      String key = prop + ":";
                      int idx = config.indexOf(key);
                      if(idx < 0) return "";
                      int start = idx + key.length();
                      int end = config.indexOf(";", start);
                      if(end < 0) end = config.length();
                      return config.substring(start, end);
                  };
                  auto ensureUnit = [&](const String &raw)->String { // Добавление CSS-единиц
                      if(raw.length() == 0) return "";
                      bool hasUnit = false;
                      for(int i=0;i<raw.length();i++){
                          char c = raw[i];
                          if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                      }
                      return hasUnit ? raw : raw + "px";
                  };
                  String widthRaw = readSetting("width"); // Ширина
                  String heightRaw = readSetting("height"); // Высота
                  String valueName = readSetting("value"); // Источник данных
                  if(valueName.length()==0) valueName = readSetting("source"); // Альтернативное имя источника
                  if(valueName.length()==0) valueName = e.id; // По умолчанию id элемента
                  String seriesName = e.id; // Имя серии
                  int canvasWidth = widthRaw.length() ? widthRaw.toInt() : 320; // Ширина canvas
                  int canvasHeight = heightRaw.length() ? heightRaw.toInt() : 220; // Высота canvas
                  String widthStyle = widthRaw.length() ? ensureUnit(widthRaw) : "100%"; // CSS ширины
                  String heightStyle = heightRaw.length() ? ensureUnit(heightRaw) : "220px"; // CSS высоты
                  String left = ensureUnit(readSetting("left")); // Смещение слева
                  String top = ensureUnit(readSetting("top")); // Смещение сверху
                  String xLabel = readSetting("xLabel"); if(xLabel.length()==0) xLabel = "X Axis"; // Подпись оси X
                  String yLabel = readSetting("yLabel"); if(yLabel.length()==0) yLabel = "Y Axis"; // Подпись оси Y
                  String pointColor = readSetting("pointColor"); if(pointColor.length()==0) pointColor = "#ff8c42"; // Цвет точек
                  String lineColor = readSetting("lineColor"); if(lineColor.length()==0) lineColor = "#4CAF50"; // Цвет линии
                  String updateStepRaw = readSetting("updateStep"); // Шаг обновления
                  unsigned long updateStepMinutes = updateStepRaw.length() ? updateStepRaw.toInt() : 5;           // минуты
                  unsigned long maxUpdatePeriod = maxGraphUpdateInterval; // Для всех графиков верхний предел списка равен 24 часам.
                  unsigned long updateStep = updateStepMinutes * 60000UL; // В миллисекундах

                  if(maxUpdatePeriod < minGraphUpdateInterval) maxUpdatePeriod = minGraphUpdateInterval; // Минимальный предел
                  if(updateStep < minGraphUpdateInterval) updateStep = minGraphUpdateInterval; // Минимальный шаг
                  if(updateStep > maxUpdatePeriod) updateStep = maxUpdatePeriod; // Ограничение шага
                  if(updateStep < 60000UL) updateStep = 5UL * 60000UL; // После часа список строим минутным шагом, а не секундами.
                  String maxPointsRaw = readSetting("maxPoints"); // Максимум точек
                  int defaultGraphMax = maxPointsRaw.length() ? maxPointsRaw.toInt() : maxGraphPoints; // Значение по умолчанию: 100 точек.
                  if(defaultGraphMax < minGraphPoints) defaultGraphMax = minGraphPoints; // Минимум
                  if(defaultGraphMax > maxGraphPoints) defaultGraphMax = maxGraphPoints; // Максимум для значения по умолчанию
                  int maxSelectablePoints = maxGraphPoints; // Ограничение выбора
                  if(maxSelectablePoints < minGraphPoints) maxSelectablePoints = minGraphPoints; // Минимум выбора
                  unsigned long defaultUpdate = defaultGraphUpdateInterval; // Новые графики по умолчанию обновляются раз в 60 минут.
                  GraphSettings seriesSettings{defaultUpdate, defaultGraphMax}; // Настройки серии

                  bool graphSettingsLoadedFromFlash = false; // Нужен для миграции старых конфигов, где maxPoints случайно стал 30.
                  auto runtimeSeriesSettings = seriesConfig.find(seriesName); // Ищем уже примененные настройки серии в RAM.
                  if(runtimeSeriesSettings != seriesConfig.end()){ // Для служебных графиков берем актуальный интервал без лишней записи во flash.
                    seriesSettings = runtimeSeriesSettings->second; // Например, графики насосов следуют периоду дозирования.
                  }
                  else if(!loadGraphSettings(seriesName, seriesSettings)){ // Загрузка сохранённых настроек
                    graphSettingsLoadedFromFlash = loadGraphSettings(valueName, seriesSettings);
                  } else {
                    graphSettingsLoadedFromFlash = true; // Настройки серии прочитаны из SPIFFS.
                  }
                  if(graphSettingsLoadedFromFlash && maxPointsRaw.length() && defaultGraphMax == maxGraphPoints && seriesSettings.maxPoints == 30){ // Исправляем старый баг: при сохранении интервала мог записаться дефолт 30 вместо 100.
                    seriesSettings.maxPoints = maxGraphPoints;
                    saveGraphSettings(seriesName, seriesSettings);
                  }
                  int graphUpdateInterval = seriesSettings.updateInterval; // Интервал обновления графика
                  if(graphUpdateInterval < (int)minGraphUpdateInterval) graphUpdateInterval = minGraphUpdateInterval;
                  if(graphUpdateInterval > (int)maxGraphUpdateInterval) graphUpdateInterval = maxGraphUpdateInterval;
                  if(graphUpdateInterval > (int)maxUpdatePeriod) graphUpdateInterval = maxUpdatePeriod;
                  int graphMaxPoints = seriesSettings.maxPoints; // Максимум точек
                  if(graphMaxPoints < minGraphPoints) graphMaxPoints = minGraphPoints;
                  if(graphMaxPoints > maxSelectablePoints) graphMaxPoints = maxSelectablePoints;
                  String graphUpdateStr = String(graphUpdateInterval); // Строка интервала
                  String graphMaxStr = String(graphMaxPoints); // Строка количества точек
                  String tableId = "graphTable_"+e.id; // ID таблицы
                  String containerStyle = "width:"+widthStyle+";max-width:100%;height:auto;padding:10px 12px;"; // Стиль контейнера
                  containerStyle += "display:flex;flex-direction:column;";
                  if(left.length() || top.length()){
                      containerStyle += "position:absolute;"; // Абсолютное позиционирование
                      if(left.length()) containerStyle += "left:"+left+";";
                      if(top.length()) containerStyle += "top:"+top+";";
                  }
                  html += "<div class='card graph-card' style='"+containerStyle+"'>"; // Карточка графика
                  html += "<div class='graph-heading'>"+e.label+"</div>"; // Заголовок графика
                  html += "<canvas id='graph_"+e.id+"' class='dash-graph' data-graph-id='"+e.id+"' width='"+String(canvasWidth)+"' height='"+String(canvasHeight)+"' data-series='"+seriesName+"' data-value-name='"+valueName+"' data-table-id='"+tableId+"' data-update-interval='"+String(graphUpdateInterval)+"' data-max-points='"+String(graphMaxPoints)+"' data-line-color='"+lineColor+"' data-point-color='"+pointColor+"' data-x-label='"+xLabel+"' data-y-label='"+yLabel+"' style='border:1px solid rgba(255,255,255,0.08);height:"+heightStyle+";'></canvas>"; // Canvas графика
                  html += "<div class='graph-axes'><span class='axis-name'>"+xLabel+"</span><span class='axis-name'>"+yLabel+"</span></div>"; // Подписи осей
              html += "</div>"; // Закрытие карточки графика
                  html += "<div class='card graph-controls' style='margin-bottom:0;'>"; // Панель управления графиком
                  html += "<div class='control-group'><label>Интервал обновления</label>"; // Выбор интервала
                  html += "<select class='graph-update-interval' data-graph='"+e.id+"'>"; // Select интервала
                  auto formatIntervalLabel = [](unsigned long valMs){ // Форматирование интервала: секунды, минуты, затем часы+минуты.
                      if(valMs < 60000UL){
                          unsigned long seconds = valMs / 1000UL;
                          return String(seconds) + " сек";
                      }
                      unsigned long totalMinutes = valMs / 60000UL;
                      if(valMs < 60UL * 60000UL){
                          return String(totalMinutes) + " мин";
                      }
                      unsigned long hours = totalMinutes / 60UL;
                      unsigned long minutes = totalMinutes % 60UL;
                      if(minutes == 0UL) return String(hours) + "ч";
                      return String(hours) + "ч " + String(minutes) + " мин";
                  };
                  String graphUpdateOptionsAdded = ";"; // Храним добавленные значения, чтобы не дублировать 5 мин/60 мин.
                  bool graphUpdateOptionAdded = false; // Следим, попал ли текущий runtime-интервал в список.
                  auto addGraphIntervalOption = [&](unsigned long opt){ // Добавляет вариант интервала без дублей и без старых 100 мс/1 сек.
                      if(opt < minGraphUpdateInterval || opt > maxUpdatePeriod) return; // Интервалы вне разрешенного диапазона не показываем.
                      String val = String(opt);
                      String marker = String(";") + val + ";";
                      if(graphUpdateOptionsAdded.indexOf(marker) >= 0) return; // Не повторяем значения, которые попали в соседнюю группу.
                      graphUpdateOptionsAdded += val + ";"; // Запоминаем добавленный интервал.
                      bool selected = graphUpdateStr==val; // Проверяем выбранный пункт.
                      if(selected) graphUpdateOptionAdded = true; // Запоминаем, что текущий интервал уже есть в списке.
                      html += String("<option value='") + val + "'" + (selected ? " selected" : "") + ">" + formatIntervalLabel(opt) + "</option>"; // Опция интервала
                  };

                  static const unsigned long graphIntervalOptions[] = { // Короткий практичный список интервалов от 5 секунд до 24 часов.
                    5UL * 1000UL, 10UL * 1000UL, 15UL * 1000UL, 30UL * 1000UL, 45UL * 1000UL,
                    1UL * 60000UL, 2UL * 60000UL, 5UL * 60000UL, 10UL * 60000UL, 15UL * 60000UL,
                    20UL * 60000UL, 30UL * 60000UL, 45UL * 60000UL,
                    60UL * 60000UL, 90UL * 60000UL, 2UL * 60UL * 60000UL, 3UL * 60UL * 60000UL,
                    4UL * 60UL * 60000UL, 6UL * 60UL * 60000UL, 8UL * 60UL * 60000UL,
                    12UL * 60UL * 60000UL, 24UL * 60UL * 60000UL
                  };
                  for(unsigned long opt : graphIntervalOptions){
                      addGraphIntervalOption(opt); // Добавляем только оптимальные интервалы без длинного списка по секундам.
                  }
                  if(!graphUpdateOptionAdded){ // Если сохраненный период нестандартный для списка, добавляем его явно.
                      html += String("<option value='") + graphUpdateStr + "' selected>" + formatIntervalLabel(graphUpdateInterval) + "</option>"; // Текущий интервал серии.
                  }
                  html += "</select></div>"; // Закрытие select
                  html += "<div class='control-group'><label>Точек на графике</label>"; // Выбор количества точек
                  html += "<input class='graph-max-points' data-graph='"+e.id+"' type='number' min='1' max='"+String(maxSelectablePoints)+"' value='"+graphMaxStr+"'>"; // Input max точек
                  html += "</div>";
                  html += "</div>";
                  html += "<div class='card graph-table-wrap'>"; // Карточка таблицы
                  html += "<table id='"+tableId+"' style='min-width:400px;'>"; // Таблица данных
                  html += "<thead><tr><th>№</th><th>Дата и время</th><th>Значение</th><th>События</th></tr></thead><tbody></tbody>"; // Заголовок таблицы
                  html += "</table></div>";
                  continue; // Переход к следующему элементу
              }
              if(e.type=="rs485Panel"){
                  String imageSrc = e.value;
                  if(imageSrc.length() && !imageSrc.startsWith("http") && !imageSrc.startsWith("/")) imageSrc = "/" + imageSrc;
                  if(!imageSrc.length()) imageSrc = "/huaqingjun.jpg";

                  html += "<div class='rs485-panel' id='"+e.id+"'>";
                  html += "<div class='rs485-card rs485-hero'>"
                          "<div><div class='rs485-title'>"+e.label+"</div>"
                          "<div class='rs485-sub'>Modbus RTU: Huaqingjun FC05/FC03 0/4, N4D3E16 FC06/FC03 0x0070/0x00C0<br>"
                          "Huaqingjun 16CH+DI16, Serial2 19200 8N1, RX GPIO16, TX GPIO17, DE/RE auto</div></div>"
                          "<div id='rs485-status' class='rs485-status'>RS485 disabled</div>"
                          "</div>";
                  html += "<div class='rs485-card rs485-board'>"
                          "<div class='rs485-side'>"
                          "<label class='rs485-enable'><input id='Rs485Enabled' type='checkbox'> RS485 включено</label>"
                          "<div class='rs485-field'><label>Тип платы</label><select id='Rs485BoardType'><option>Huaqingjun 16CH relay + DI16</option></select></div>"
                          "<div class='rs485-note'>Профиль Huaqingjun: отдельное реле переключается FC05 по coil 0..15, реле читаются из регистра 0, входы из регистра 4.</div>"
                          "</div>"
                          "<div class='rs485-image-card'>"
                          "<img class='rs485-image' src='"+imageSrc+"' alt='Huaqingjun 16-channel RS485 relay module'>"
                          "<div class='rs485-spec'><h4>Huaqingjun 16CH relay + DI16</h4>"
                          "<div class='rs485-spec-row'><strong>Питание</strong><span>DC 12/24 В на клеммы питания платы</span></div>"
                          "<div class='rs485-spec-row'><strong>Интерфейс</strong><span>RS485 A/B, общий GND с ESP32 обязателен</span></div>"
                          "<div class='rs485-spec-row'><strong>ESP32 UART</strong><span>RX GPIO16, TX GPIO17, DE/RE auto</span></div>"
                          "<div class='rs485-spec-row'><strong>Выходы</strong><span>16 механических реле, контакты COM/NO/NC для внешней нагрузки</span></div>"
                          "<div class='rs485-spec-row'><strong>Входы</strong><span>DI1..DI16, читаются отдельной 16-битной маской</span></div>"
                          "<div class='rs485-spec-row'><strong>Slave ID</strong><span>1..247, адрес платы можно изменить с этой страницы</span></div>"
                          "<div class='rs485-spec-row'><strong>Команда реле</strong><span>FC05, coil 0..15, ON 0xFF00, OFF 0x0000</span></div>"
                          "<div class='rs485-spec-row'><strong>Состояния</strong><span>FC03: реле register 0, входы register 4</span></div>"
                          "<div class='rs485-spec-row'><strong>UART режимы</strong><span>1200..115200 бод, 8N1/8N2/8O1/8E1</span></div>"
                          "</div></div></div>";
                  html += "<div class='rs485-card rs485-controls'>"
                          "<div class='rs485-field'><label>Скорость</label><select id='Rs485BaudRate'><option>1200</option><option>2400</option><option>4800</option><option>9600</option><option selected>19200</option><option>38400</option><option>57600</option><option>115200</option></select></div>"
                          "<div class='rs485-field'><label>UART</label><select id='Rs485UartMode'><option selected>8N1</option><option>8N2</option><option>8O1</option><option>8E1</option></select></div>"
                          "<div class='rs485-field'><label>Modbus Slave ID</label><input id='Rs485SlaveId' type='number' min='1' max='247' value='1'></div>"
                          "<div class='rs485-field'><label>Опрос, мс</label><input id='Rs485PollIntervalMs' type='number' min='200' max='5000' step='100' value='1000'></div>"
                          "<button class='rs485-action' id='rs485-poll-now' type='button'>Опросить сейчас</button>"
                          "<button class='rs485-action danger' id='rs485-all-off' type='button'>Выключить все</button>"
                          "</div>";
                  html += "<div class='rs485-card'><div class='rs485-grid-title'>Реле</div><div class='rs485-relay-grid'>";
                  for(uint8_t relay = 0; relay < 16; relay++){
                      html += "<div class='rs485-relay-row'><span>R"+String(relay + 1)+"</span>"
                              "<button class='rs485-relay-btn off' data-relay='"+String(relay)+"' type='button'>OFF</button></div>";
                  }
                  html += "</div></div>";
                  html += "<div class='rs485-card'><div class='rs485-grid-title'>Дискретные входы</div><div class='rs485-input-grid'>";
                  for(uint8_t input = 0; input < 16; input++){
                      html += "<div class='rs485-input-chip' data-input='"+String(input)+"'>DI"+String(input + 1)+": OFF</div>";
                  }
                  html += "</div></div>";
                  static const char* assignments[] = {
                    "R1: лампа бассейна",
                    "R2: питание RGB-ленты WS2815",
                    "R3: резерв / ручное реле",
                    "R4: резерв / ручное реле",
                    "R5: нагреватель воды",
                    "R6: насос NaOCl",
                    "R7: насос ACO / кислота pH",
                    "R8: резерв / ручное реле",
                    "R9: насос фильтрации и слив воды",
                    "R10: компрессор воздуха клапанов",
                    "R11: клапан FILTRATION",
                    "R12: клапан BACKWASH",
                    "R13: сброс песка после промывки",
                    "R14: соленоид долива воды",
                    "R15: теплый пол помещения",
                    "R16: уличное освещение",
                    "DI1: нижний уровень бассейна",
                    "DI2: верхний уровень бассейна",
                    "DI3: верхний уровень сливной ямы",
                    "DI4-DI16: свободные входы"
                  };
                  html += "<div class='rs485-card'><div class='rs485-grid-title'>ℹ️ Назначение реле и входов RS485</div><div class='rs485-assignments'>";
                  for(const char* assignment : assignments){
                      html += "<div class='rs485-assignment'>";
                      html += assignment;
                      html += "</div>";
                  }
                  html += "</div></div></div>";
                  renderedCount++;
                  continue;
              }
              String val = uiValueForId(e.id); // Текущее значение элемента UI
              auto isLevelSensorDisplay = [&](const String &id)->bool { // Три уровнемера рисуем отдельной пиктограммой с водой и поплавком.
                  return id == "WaterLevelSensorUpper" || id == "WaterLevelSensorLower" || id == "WaterLevelSensorDrain";
              };
              auto levelSensorStateClass = [&](const String &id)->String { // Начальный CSS-класс состояния уровнемера при загрузке страницы.
                  if(id == "WaterLevelSensorUpper") return PoolUpperLevelReachedConfirmed ? String("sensor-upper sensor-reached") : String("sensor-upper sensor-below");
                  if(id == "WaterLevelSensorLower") return PoolLowerLevelLowConfirmed ? String("sensor-lower sensor-low") : String("sensor-lower sensor-normal");
                  if(id == "WaterLevelSensorDrain") return DrainPitFullConfirmed ? String("sensor-drain sensor-full") : String("sensor-drain sensor-empty");
                  return String();
              };
              auto levelSensorHint = [&](const String &id)->String { // Подпись к пиктограмме, чтобы было понятно какая отметка контролируется.
                  if(id == "WaterLevelSensorUpper") return String("верхняя отметка бассейна");
                  if(id == "WaterLevelSensorLower") return String("нижняя отметка бассейна");
                  if(id == "WaterLevelSensorDrain") return String("верхняя отметка сливной ямы");
                  return String();
              };
              if((e.type=="display" || e.type=="displayString") && isLevelSensorDisplay(e.id)){ // Наглядные карточки уровнемеров вместо обычного текста.
                  html += "<div class='card level-sensor-shell'>";
                  html += "<div class='level-sensor-card "+levelSensorStateClass(e.id)+"' data-level-sensor='"+e.id+"'>";
                  html += "<div class='level-sensor-visual' aria-hidden='true'><div class='sensor-tank'>"
                          "<div class='sensor-mark high'></div><div class='sensor-mark low'></div>"
                          "<div class='sensor-water'></div><div class='sensor-probe'></div><div class='sensor-float'></div>"
                          "</div></div>";
                  html += "<div class='level-sensor-info'><label>"+e.label+"</label>"
                          "<div id='"+e.id+"' class='level-sensor-status'>"+val+"</div>"
                          "<div class='level-sensor-hint'>"+levelSensorHint(e.id)+"</div></div>";
                  html += "</div></div>";
                  renderedCount++; // Подсчитываем реально добавленные элементы в HTML
                  continue;
              }
              if(e.type=="timer"){ // Элемент таймера
                  UITimerEntry &timer = ui.timer(e.id); // Получение таймера из реестра
                  html += "<div class='card timer-card'>"; // Карточка таймера
                  html += "<div class='timer-card__header'>" + e.label + "</div>"; // Заголовок таймера
                  html += "<div class='timer-card__grid'>"; // Сетка полей времени
                  html += "<div class='timer-card__column'><label for='" + e.id + "_ON'>Включение</label>" // Поле времени включения
                          "<input id='" + e.id + "_ON' type='time' value='" + formatMinutesToTime(timer.on) + "'></div>";
                  html += "<div class='timer-card__column'><label for='" + e.id + "_OFF'>Отключение</label>" // Поле времени отключения
                          "<input id='" + e.id + "_OFF' type='time' value='" + formatMinutesToTime(timer.off) + "'></div>";
                  html += "</div></div>"; // Закрытие сетки и карточки
                  continue; // Переход к следующему элементу
              }


              html += "<div class='card'>"; // Обычная карточка элемента

              if(e.type=="text") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='text' value='"+val+"'>"; // Текстовое поле
              else if(e.type=="int") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='number' value='"+val+"'>"; // Целочисленный ввод
              else if(e.type=="float") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='number' step='0.1' value='"+val+"'>"; // Вещественное число
              else if(e.type=="time") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='time' value='"+val+"'>"; // Ввод времени
              else if(e.type=="clockselect"){ // Панель настройки времени
                html += "<div class='stat-group'><div class='stat-heading'>"+e.label+"</div>"
                          "<div class='time-settings-grid'>"
                          "<div class='time-settings-field'><label for='gmtOffset'>Часовой пояс</label><select id='gmtOffset'></select></div>"
                          "<div class='time-settings-field'><label for='manual-date'>Дата</label><input id='manual-date' type='date'></div>"
                          "<div class='time-settings-field'><label for='manual-time'>Время</label><input id='manual-time' type='time' step='1' data-skip-save='1'></div>"
                          "</div>"
                          "<div class='time-settings-actions'>"
                          "<button class='btn-primary' id='manual-time-btn' onclick='applyManualTime()'>Установить время</button>"
                          "<div class='time-settings-status' id='manual-time-status'></div>"
                          "</div></div>";
              }
              else if(e.type=="color") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='color' value='"+val+"'>"; // Выбор цвета
              else if(e.type=="checkbox"){ // Чекбокс
                String checked = (val == "1" || val.equalsIgnoreCase("true")) ? " checked" : ""; // Определение состояния
                html += "<label><input id='"+e.id+"' type='checkbox' value='1'"+checked+">"+e.label+"</label>"; // HTML чекбокса
              }
              
              else if(e.type=="slider"){ // Слайдер
                  String cfg = e.value; // Конфигурация слайдера
                  auto readSliderCfg = [&](const String &prop)->String { // Чтение параметра конфигурации
                      String token = prop + "=";
                      int start = cfg.indexOf(token);
                      if(start < 0) return "";
                      start += token.length();
                      int end = cfg.indexOf(';', start);
                      if(end < 0) end = cfg.length();
                      return cfg.substring(start, end);
                  };
                  float minCfg = 0; // Минимум
                  float maxCfg = 100; // Максимум
                  float stepCfg = 1; // Шаг
                  String minStr = readSliderCfg("min");
                  if(minStr.length()) minCfg = minStr.toFloat(); // Парсинг минимума
                  String maxStr = readSliderCfg("max");
                  if(maxStr.length()) maxCfg = maxStr.toFloat(); // Парсинг максимума
                  String stepStr = readSliderCfg("step");
                  if(stepStr.length()) stepCfg = stepStr.toFloat(); // Парсинг шага
                  if(!val.length()) val = readSliderCfg("value"); // Значение по умолчанию
                  if(!val.length()){
                      int stepInt = static_cast<int>(stepCfg);
                      bool integerStep = (static_cast<float>(stepInt) == stepCfg); // Проверка целого шага
                      val = String(minCfg, integerStep ? 0 : 2); // Начальное значение
                  }
                  html += "<label>"+e.label+"</label><input id='"+e.id+"' type='range' min='"+String(minCfg)+"' max='"+String(maxCfg)+"' step='"+String(stepCfg)+"' value='"+val+"' oninput='updateSlider(this)'><span id='"+e.id+"Val'> "+val+"</span>"; // HTML слайдера
              }
              else if(e.type=="button"){ // Кнопка ON/OFF
                  String state = val.length() ? val : "0"; // Состояние кнопки
                  String text = (state == "1") ? "ON" : "OFF"; // Текст кнопки
                  String cssState = (state == "1") ? " on" : " off"; // CSS-класс состояния

                
                  String cfg = e.value; // Конфигурация кнопки
                  auto readProp = [&](const String &prop)->String { // Чтение CSS-параметра
                      String key = prop + ":";
                      int idx = cfg.indexOf(key);
                      if(idx < 0) return "";
                      int start = idx + key.length();
                      int end = cfg.indexOf(";", start);
                      if(end < 0) end = cfg.length();
                      return cfg.substring(start, end);
                  };
                  auto ensureUnit = [&](const String &raw)->String { // Приведение к CSS-единицам
                      if(raw.length() == 0) return "";
                      bool hasUnit = false;
                      for(int i=0;i<raw.length();i++){
                          char c = raw[i];
                          if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                      }
                      return hasUnit ? raw : raw + "px";
                  };
                  String x = readProp("x"); // Смещение по X
                  String y = readProp("y"); // Смещение по Y
                  String w = readProp("width"); // Ширина
                  String h = readProp("height"); // Высота
                  String btnColor = readProp("color"); // Цвет кнопки
                  String styleExtra;
                  if(x.length()) styleExtra += "margin-left:"+ensureUnit(x)+";"; // Отступ слева
                  if(y.length()) styleExtra += "margin-top:"+ensureUnit(y)+";"; // Отступ сверху
                  if(w.length()) styleExtra += "width:"+ensureUnit(w)+";"; // Ширина кнопки
                  if(h.length()) styleExtra += "height:"+ensureUnit(h)+";"; // Высота кнопки
                  if(btnColor.length()){
                      String normalized = btnColor;
                      normalized.trim();
                      if(!normalized.startsWith("#") && (normalized.length()==3 || normalized.length()==6)){
                          normalized = "#" + normalized;
                      }
                      styleExtra += "background:"+normalized+";border-color:"+normalized+";"; // Цвет фона и рамки
                  }
                  String styleAttr = styleExtra.length() ? " style='"+styleExtra+"'" : ""; // Атрибут style

                  html += "<label>"+e.label+"</label><button id='"+e.id+"' class='dash-btn"+cssState+"' data-type='dashButton' data-state='"+state+"'"+styleAttr+">"+text+"</button>"; // HTML кнопки
              }
              else if(e.type=="popupButton"){ // Кнопка открытия popup
                  String btnText = e.label.length() ? e.label : "Open"; // Текст кнопки
                  html += "<button class='btn-primary' data-popup-open='"+e.value+"'>"+btnText+"</button>"; // HTML popup-кнопки
              }
              else if(e.type=="display" || e.type=="displayString")  // Отображение строки
                html += "<label>"+e.label+"</label><div id='"+e.id+"' style='font-size:1.2em; min-height:1.5em; color:#fff;'>"+val+"</div>"; // Блок отображения
              else if(e.type=="displayStringAbsolute") { // Абсолютное позиционирование строки
                  int x=0, y=0, fontSize=16;
                  String color="#ffffff";
                  String style = e.value;
                  int idx;

                  if((idx = style.indexOf("x:"))!=-1) x = style.substring(idx+2, style.indexOf(";", idx)).toInt(); // X координата
                  if((idx = style.indexOf("y:"))!=-1) y = style.substring(idx+2, style.indexOf(";", idx)).toInt(); // Y координата
                  if((idx = style.indexOf("fontSize:"))!=-1) fontSize = style.substring(idx+9, style.indexOf(";", idx)).toInt(); // Размер шрифта
                  if((idx = style.indexOf("color:"))!=-1) color = style.substring(idx+6); // Цвет текста

                  html += "<div id='"+e.id+"' style='position:absolute; left:"+String(x)+"px; top:"+String(y)+"px; font-size:"+String(fontSize)+"px; color:"+color+";'>"+e.label+"</div>"; // HTML абсолютного текста
              }

              else if(e.type=="dropdown"){ // Выпадающий список
                  html += "<label>"+e.label+"</label><select id='"+e.id+"'>"; // Начало select
                  if(e.value.length()){
                      int start = 0;
                      while(start < e.value.length()){
                          int end = e.value.indexOf('\n', start);
                          if(end < 0) end = e.value.length();
                          String line = e.value.substring(start, end);
                          line.trim();
                          if(line.length()){
                              int sep = line.indexOf('=');
                              String optValue = sep >= 0 ? line.substring(0, sep) : line; // Значение option
                              String optLabel = sep >= 0 ? line.substring(sep + 1) : line; // Текст option
                              html += String("<option value='") + optValue + "'" + (val==optValue?" selected":"") + ">" + optLabel + "</option>"; // Option
                          }
                          start = end + 1;
                      }
                  } else {
                      html += String("<option value='Normal'") + (val=="Normal"?" selected":"") + ">Normal</option>"; // Значение по умолчанию
                      html += String("<option value='Eco'") + (val=="Eco"?" selected":"") + ">Eco</option>";
                      html += String("<option value='Turbo'") + (val=="Turbo"?" selected":"") + ">Turbo</option>";
                  }
                  html += "</select>"; // Закрытие select
              }
              else if(e.type=="selectdays"){ // Выбор дней недели
                  html += "<label>"+e.label+"</label>"; // Заголовок
                  html += "<div id='"+e.id+"' class='select-days'>"; // Контейнер дней
                 const char* dayValues[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"}; // Значения дней
                  const char* dayLabels[] = {"ПН","ВТ","СР","ЧТ","ПТ","СБ","ВС"}; // Подписи дней
                  for(int i=0;i<7;i++){
                      bool checked = val.indexOf(dayValues[i])>=0; // Проверка выбранного дня
                      html += "<label class='day-pill'><input type='checkbox' value='"+String(dayValues[i])+"' "
                          +(checked?"checked":"")+"><span>"+String(dayLabels[i])+"</span></label>"; // Чекбокс дня
                  }
                  html += "</div>"; // Закрытие контейнера дней
              }

          else if(e.type=="range"){ // Элемент типа диапазон (двухползунковый range)
                  String cfg = e.value; // Строка конфигурации диапазона (min/max/step)
                  auto readRangeCfg = [&](const String &prop)->String { // Лямбда для чтения параметров из cfg
                      String token = prop + "="; // Формируем ключ вида "min=", "max=", "step="
                      int start = cfg.indexOf(token); // Ищем начало параметра
                      if(start < 0) return ""; // Если параметр не найден — возвращаем пустую строку
                      start += token.length(); // Смещаемся к значению параметра
                      int end = cfg.indexOf(';', start); // Ищем конец параметра
                      if(end < 0) end = cfg.length(); // Если ';' нет — берём до конца строки
                      return cfg.substring(start, end); // Возвращаем значение параметра
                  };
                  float minCfg = 0; // Минимальное значение диапазона по умолчанию
                  float maxCfg = 100; // Максимальное значение диапазона по умолчанию
                  float stepCfg = 1; // Шаг изменения по умолчанию
                  String minStr = readRangeCfg("min"); // Читаем min из конфигурации
                  if(minStr.length()) minCfg = minStr.toFloat(); // Применяем min, если задан
                  String maxStr = readRangeCfg("max"); // Читаем max из конфигурации
                  if(maxStr.length()) maxCfg = maxStr.toFloat(); // Применяем max, если задан
                  String stepStr = readRangeCfg("step"); // Читаем step из конфигурации
                  if(stepStr.length()) stepCfg = stepStr.toFloat(); // Применяем step, если задан

                  float minVal = minCfg; // Текущее минимальное значение диапазона
                  float maxVal = maxCfg; // Текущее максимальное значение диапазона
                  if(val.length()){ // Если сохранённое значение существует
                      int sep = val.indexOf('-'); // Ищем разделитель значений "min-max"
                      if(sep >= 0){
                          minVal = val.substring(0, sep).toFloat(); // Извлекаем минимальное значение
                          maxVal = val.substring(sep + 1).toFloat(); // Извлекаем максимальное значение
                      }
                  }

              int precision = stepCfg < 1.0f ? 2 : 0; // Количество знаков после запятой (для дробного шага)


               String rangeLabel = e.label.length() ? e.label : "Range Slider";
                  String rangeId = e.id;
                  html += "<label>"+rangeLabel+"</label>"
                          "<div class='range-slider' id='"+rangeId+"'>"
                          "<input type='range' id='"+rangeId+"Min' min='"+String(minCfg)+"' max='"+String(maxCfg)+"' step='"+String(stepCfg)+"' value='" + String(minVal, precision) + "'>"
                          "<input type='range' id='"+rangeId+"Max' min='"+String(minCfg)+"' max='"+String(maxCfg)+"' step='"+String(stepCfg)+"' value='" + String(maxVal, precision) + "'>"
                          "<div class='slider-track'></div>"
                          "</div>"
                          "<div class='range-display'>Range: <span id='"+rangeId+"Val'>" + String(minVal, precision) + " - " + String(maxVal, precision) + "</span></div>"
                          "<style>"
                          ".range-slider { position: relative; width: 100%; height: 22px; --thumb-size: 20px; }"
                          ".range-slider input[type=range] { position: absolute; width: 100%; pointer-events: none; -webkit-appearance: none; background: none; }"
                          ".range-slider input[type=range]::-webkit-slider-thumb { pointer-events: all; width: var(--thumb-size); height: var(--thumb-size); border-radius: 50%; background: #1e88e5; -webkit-appearance: none; cursor: pointer; position: relative; z-index: 3; }"
                          ".slider-track { position: absolute; height: 6px; top: 50%; transform: translateY(-50%); background: #444; border-radius: 3px; left: calc(var(--thumb-size) / 2); right: calc(var(--thumb-size) / 2); }"
                                                    ".range-display { margin-top: 12px; line-height: 1.4; }"
                          "</style>"
                          "<script>"
                          "(() => {"
                          "  const rangeId = '"+rangeId+"';"
                          "  const minEl = document.getElementById(rangeId + 'Min');"
                          "  const maxEl = document.getElementById(rangeId + 'Max');"
                          "  const track = document.querySelector('#' + rangeId + ' .slider-track');"
                          "  const display = document.getElementById(rangeId + 'Val');"
                          "  if(!minEl || !maxEl) return;"
                          "  const minLimit = "+String(minCfg)+";"
                          "  const maxLimit = "+String(maxCfg)+";"
                          "  let saveTimer = null;"
                          "  const updateRangeSlider = (shouldSave = true) => {"
                          "    let min = parseFloat(minEl.value);"
                          "    let max = parseFloat(maxEl.value);"
                          "    if(min > max){ [min, max] = [max, min]; minEl.value=min; maxEl.value=max; }"
                          "    const fixed = " + String(precision) + ";"
                          "    if(display) display.innerText = min.toFixed(fixed) + ' - ' + max.toFixed(fixed);"
                          "    const span = (maxLimit - minLimit) || 1;"
                          "    const minPct = ((min - minLimit) / span) * 100;"
                          "    const maxPct = ((max - minLimit) / span) * 100;"
                          "    if(track) track.style.background = `linear-gradient(to right, #444 ${minPct}%, #1e88e5 ${minPct}%, #1e88e5 ${maxPct}%, #444 ${maxPct}%)`;"
                          "    if(shouldSave){"
                          "      const minSaved = min.toFixed(fixed);"
                          "      const maxSaved = max.toFixed(fixed);"
                          "      if(saveTimer) clearTimeout(saveTimer);"
                          "      saveTimer = setTimeout(() => {"
                          "        fetch('/save?key=' + encodeURIComponent(rangeId) + '&val=' + encodeURIComponent(minSaved + '-' + maxSaved));"
                          "      }, 300);"
                          "    }"
                          "  };"
                          "  minEl.addEventListener('input', () => updateRangeSlider(true));"
                          "  maxEl.addEventListener('input', () => updateRangeSlider(true));"
                          "  updateRangeSlider(false);"
                          "})();"
                          "</script>";
              }

                            html += "</div>";
            renderedCount++; // Подсчитываем реально добавленные элементы в HTML
          }
        if(renderedCount == 0){
          String currentTabTitle;
          for(auto &tab : self->tabs){
            if(tab.id == tabId){
              currentTabTitle = tab.title;
              break;
            }
          }
          if(currentTabTitle.indexOf("RS485") >= 0){
            html += "<div class='rs485-panel' id='Rs485Panel'>";
            html += "<div class='rs485-card rs485-hero'><div><div class='rs485-title'>RS485 16CH + DI16</div>"
                    "<div class='rs485-sub'>Modbus RTU: Huaqingjun FC05/FC03 0/4, N4D3E16 FC06/FC03 0x0070/0x00C0<br>"
                    "Huaqingjun 16CH+DI16, Serial2 19200 8N1, RX GPIO16, TX GPIO17, DE/RE auto</div></div>"
                    "<div id='rs485-status' class='rs485-status off'>RS485 disabled</div></div>";
            html += "<div class='rs485-card rs485-board'><div class='rs485-side'>"
                    "<label class='rs485-enable'><input id='Rs485Enabled' type='checkbox'> RS485 включено</label>"
                    "<div class='rs485-field'><label>Тип платы</label><select id='Rs485BoardType'><option>Huaqingjun 16CH relay + DI16</option></select></div>"
                    "<div class='rs485-note'>Профиль Huaqingjun: отдельное реле переключается FC05 по coil 0..15, реле читаются из регистра 0, входы из регистра 4.</div>"
                    "</div><div class='rs485-image-card'>"
                    "<img class='rs485-image' src='/huaqingjun.jpg' alt='Huaqingjun 16-channel RS485 relay module'>"
                    "<div class='rs485-spec'><h4>Huaqingjun 16CH relay + DI16</h4>"
                    "<div class='rs485-spec-row'><strong>Питание</strong><span>DC 12/24 В</span></div>"
                    "<div class='rs485-spec-row'><strong>Интерфейс</strong><span>RS485 A/B, общий GND с ESP32 обязателен</span></div>"
                    "<div class='rs485-spec-row'><strong>ESP32 UART</strong><span>RX GPIO16, TX GPIO17, DE/RE auto</span></div>"
                    "<div class='rs485-spec-row'><strong>Выходы</strong><span>16 реле COM/NO/NC</span></div>"
                    "<div class='rs485-spec-row'><strong>Входы</strong><span>DI1..DI16</span></div>"
                    "<div class='rs485-spec-row'><strong>Команда</strong><span>FC05 coil 0..15</span></div>"
                    "<div class='rs485-spec-row'><strong>Состояния</strong><span>FC03: реле register 0, входы register 4</span></div>"
                    "</div></div></div>";
            html += "<div class='rs485-card rs485-controls'>"
                    "<div class='rs485-field'><label>Скорость</label><select id='Rs485BaudRate'><option>1200</option><option>2400</option><option>4800</option><option>9600</option><option selected>19200</option><option>38400</option><option>57600</option><option>115200</option></select></div>"
                    "<div class='rs485-field'><label>UART</label><select id='Rs485UartMode'><option selected>8N1</option><option>8N2</option><option>8O1</option><option>8E1</option></select></div>"
                    "<div class='rs485-field'><label>Modbus Slave ID</label><input id='Rs485SlaveId' type='number' min='1' max='247' value='1'></div>"
                    "<div class='rs485-field'><label>Опрос, мс</label><input id='Rs485PollIntervalMs' type='number' min='200' max='5000' step='100' value='1000'></div>"
                    "<button class='rs485-action' id='rs485-poll-now' type='button'>Опросить сейчас</button>"
                    "<button class='rs485-action danger' id='rs485-all-off' type='button'>Выключить все</button></div>";
            html += "<div class='rs485-card'><div class='rs485-grid-title'>Реле</div><div class='rs485-relay-grid'>";
            for(uint8_t relay = 0; relay < 16; relay++){
              html += "<div class='rs485-relay-row'><span>R"+String(relay + 1)+"</span><button class='rs485-relay-btn off' data-relay='"+String(relay)+"' type='button'>OFF</button></div>";
            }
            html += "</div></div><div class='rs485-card'><div class='rs485-grid-title'>Дискретные входы</div><div class='rs485-input-grid'>";
            for(uint8_t input = 0; input < 16; input++){
              html += "<div class='rs485-input-chip' data-input='"+String(input)+"'>DI"+String(input + 1)+": OFF</div>";
            }
            html += "</div></div><div class='rs485-card'><div class='rs485-grid-title'>ℹ️ Назначение реле и входов RS485</div>"
                    "<div class='rs485-assignments'>"
                    "<div class='rs485-assignment'>R1: лампа бассейна</div>"
                    "<div class='rs485-assignment'>R2: питание RGB-ленты WS2815</div>"
                    "<div class='rs485-assignment'>R3/R4/R8: резерв / ручное реле</div>"
                    "<div class='rs485-assignment'>R5: нагреватель воды</div>"
                    "<div class='rs485-assignment'>R6: насос NaOCl</div>"
                    "<div class='rs485-assignment'>R7: насос ACO / кислота pH</div>"
                    "<div class='rs485-assignment'>R9: насос фильтрации и слив воды</div>"
                    "<div class='rs485-assignment'>R10: компрессор воздуха клапанов</div>"
                    "<div class='rs485-assignment'>R11: клапан FILTRATION</div>"
                    "<div class='rs485-assignment'>R12: клапан BACKWASH</div>"
                    "<div class='rs485-assignment'>R13: сброс песка после промывки</div>"
                    "<div class='rs485-assignment'>R14: соленоид долива воды</div>"
                    "<div class='rs485-assignment'>R15: теплый пол помещения</div>"
                    "<div class='rs485-assignment'>R16: уличное освещение</div>"
                    "<div class='rs485-assignment'>DI1: нижний уровень бассейна</div>"
                    "<div class='rs485-assignment'>DI2: верхний уровень бассейна</div>"
                    "<div class='rs485-assignment'>DI3: верхний уровень сливной ямы</div>"
                    "<div class='rs485-assignment'>DI4-DI16: свободные входы</div>"
                    "</div></div></div>";
            renderedCount++;
          }
        }
        if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] renderElements tab=%s ms=%u count=%u\n", // Помогает увидеть вкладку, где начинается торможение
                      tabId.c_str(),
                      static_cast<unsigned>(millis() - renderElementsStartMs),
                      static_cast<unsigned>(renderedCount));
                };

      bool firstPage = true;
      for(auto &t : self->tabs){
           html += "<div id='"+t.id+"' class='page"+String(firstPage?" active":"")+"'>"
                  "<div class='page-header'><h3>"+t.title+"</h3>"
                  "<div class='page-datetime' id='page-datetime-"+t.id+"'>--</div></div>";
          const uint32_t renderElementsTabStartMs = millis(); // Таймер рендера элементов внутри текущей tab-страницы
          renderTabElements(t.id); // Рендерим элементы текущей вкладки в общий HTML
          stageRenderElementsMs += (millis() - renderElementsTabStartMs); // Накапливаем время renderElements по всем вкладкам
          html += "</div>";
          firstPage = false;
      }

      stageRenderTabsMs = millis() - stageStartMs; // Общая длительность этапа renderTabs
      if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] renderTabs=%u ms\n", static_cast<unsigned>(stageRenderTabsMs)); // Лог renderTabs только в debug-режиме.
      if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] renderElements=%u ms\n", static_cast<unsigned>(stageRenderElementsMs)); // Лог renderElements только в debug-режиме.
      stageStartMs = millis(); // Переключаем таймер на измерение этапа renderPopups

      for(auto &popup : self->popups){
          html += "<div id='popup-"+popup.id+"' class='dash-modal hidden' data-popup='"+popup.id+"'>"
                  "<div class='dash-modal-content'>"
                  "<div class='dash-modal-header'><h4>"+popup.title+"</h4>"
                  "<button class='icon-btn' onclick=\"closePopup('"+popup.id+"')\">&times;</button></div>"
                  "<div class='dash-modal-body'><div class='popup-grid'>";
          const uint32_t renderPopupElementsStartMs = millis(); // Таймер элементов popup, чтобы отделить вкладки от модалок
          renderTabElements(popup.tabId); // Используем тот же рендер для содержимого модального окна
          stageRenderElementsMs += (millis() - renderPopupElementsStartMs); // Добавляем время popup-элементов в общий счётчик
          html += "</div></div></div></div>";
      }

      stageRenderPopupsMs = millis() - stageStartMs; // Длительность этапа renderPopups
      if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] renderPopups=%u ms\n", static_cast<unsigned>(stageRenderPopupsMs)); // Лог renderPopups только в debug-режиме.

      auto systemPageEnabled = [&](const String &type)->bool{
        if(systemPageDecls.empty()) return true;
        for(auto &page : systemPageDecls){
          if(page.type == type) return true;
        }
        return false;
      };
      auto resolveSystemPage = [&](const String &type, const String &fallbackId, const String &fallbackTitle)->SystemPageDecl{
        for(auto &page : systemPageDecls){
          if(page.type == type) return page;
        }
        return SystemPageDecl{fallbackId, fallbackTitle, type};
      };
      const SystemPageDecl wifiPage = resolveSystemPage("wifi", "wifi", "WiFi Settings");
      const SystemPageDecl statsPage = resolveSystemPage("stats", "stats", "Statistics");
      const SystemPageDecl mqttPage = resolveSystemPage("mqtt", "mqtt", "Настройка MQTT");
      const SystemPageDecl profilePage = resolveSystemPage("profile", "profile", "Профиль");

      
      // ====== WiFi страница ======
      if(systemPageEnabled("wifi")){
      html += "<div id='"+wifiPage.id+"' class='page'>"
              "<div class='page-header'><h3>"+wifiPage.title+"</h3>"
              "<div class='page-datetime' id='page-datetime-"+wifiPage.id+"'>--</div></div>"
              "<div class='card compact wifi-card'>"
              "<div class='wifi-field'><label>SSID</label><div class='input-with-action'>"
              "<input id='ssid' value='"+loadValue<String>("ssid",defaultSSID)+"'>"
              "<button class='btn-secondary' id='scan-btn' onclick='scanWiFi()'>Scan WiFi</button>"
              "</div></div>"
              
              "<div class='wifi-field'><label>Password</label><input id='pass' type='password' value='"+loadValue<String>("pass",defaultPASS)+"'></div>"
              "</div>"
              "<h4 class='section-title'>Backup STA Settings</h4>"
              "<div class='card compact wifi-card'>"
              "<div class='wifi-field'><label>Backup SSID</label><input id='ssid2' value='"+loadValue<String>("ssid2", String(""))+"'></div>"
              "<div class='wifi-field'><label>Backup Password</label><input id='pass2' type='password' value='"+loadValue<String>("pass2", String(""))+"'></div>"
              "</div>"
              "<h4 class='section-title'>AP Settings</h4>"
              "<div class='card compact wifi-card'>"
              "<div class='wifi-field'><label>AP SSID</label><input id='ap_ssid' value='"+loadValue<String>("apSSID", String(apSSID))+"'></div>"
              "<div class='wifi-field'><label>AP Password</label><input id='ap_pass' type='password' value='"+loadValue<String>("apPASS", String(apPASS))+"'></div>"
              "</div>"
              
              "<div class='wifi-field'><label>Hostname</label><input id='hostname' value='"+loadValue<String>("hostname", String(defaultHostname))+"'></div>"

              "<div class='wifi-actions'><button class='btn-primary' onclick='saveWiFi()'>Save WiFi</button>"
              "<div class='wifi-hint'>Текущее состояние сети обновляется автоматически</div></div>"
              "<div class='card compact stats-card wifi-status-card'>"
              "<div class='stat-group'><div class='stat-heading'>Сеть</div><ul class='stat-list'>"
              "<li><span>Wi-Fi Status (текущее состояние)</span><strong id='wifi-status'>--</strong></li>"
              "<li><span>Wi-Fi Mode (текущий режим STA/AP)</span><strong id='wifi-mode'>--</strong></li>"
              "<li><span>SSID (имя Wi-Fi сети)</span><strong id='wifi-ssid'>--</strong></li>"
              "<li><span>Local IP (текущий IP-адрес)</span><strong id='wifi-ip'>--</strong></li>"
              "<li><span>Signal Strength (RSSI) (уровень сигнала Wi-Fi)</span><strong id='wifi-rssi'>--</strong></li>"
              "</ul></div></div>"
              "<div id='wifi-scan-modal' class='wifi-modal hidden'>"
              "<div class='wifi-modal-content'>"
              "<div class='wifi-modal-header'><h4>Доступные сети</h4><button class='icon-btn' onclick='closeWifiModal()'>&times;</button></div>"
              "<div id='wifi-scan-list' class='wifi-scan-list'></div>"
              "</div></div>"
              "</div>";
      }

      // ====== Statistics страница ======
      if(systemPageEnabled("stats")){
      html += "<div id='"+statsPage.id+"' class='page'>"
              "<div class='page-header'><h3>"+statsPage.title+"</h3>"
              "<div class='page-datetime' id='page-datetime-"+statsPage.id+"'>--</div></div>"
              "<div class='stats-actions'>"
              "<button class='btn-secondary' id='refresh-stats-btn' onclick='fetchStats(true)'>Обновить информацию</button>"
              "<button class='btn-danger' id='reboot-btn' onclick='rebootEsp()'>Перезагрузить ESP</button>"
              "</div>"
              "<div class='card compact stats-card'>"
              "<div class='stat-group'><div class='stat-heading'>Система</div><ul class='stat-list'>"
              "<li><span>Chip Model / Revision (модель и ревизия чипа)</span><strong id='stat-chip'>--</strong></li>"
              "<li><span>CPU Frequency (MHz) (текущая частота процессора)</span><strong id='stat-cpu'>--</strong></li>"
              "<li><span>Temperature (температура чипа)</span><strong id='stat-temp'>--</strong></li>"
              "<li><span>Uptime (время непрерывной работы устройства)</span><strong id='stat-uptime'>--</strong></li>"
              "<li><span>MAC Address (уникальный адрес)</span><strong id='stat-mac'>--</strong></li>"
              "</ul></div>"
              "<div class='stat-group'><div class='stat-heading'>Память и хранилище</div><ul class='stat-list'>"
              "<li><span>Free Heap (свободная оперативная память) — оперативная память для работы приложения</span><strong id='stat-heap'>--</strong></li>"
               "<li><span>PSRAM (использовано / свободно) — внешняя оперативная память</span><strong id='stat-psram'>--</strong></li>"
              "<li><span>Flash (использовано / свободно) — загружается основная прошивка и OTA-обновления</span><strong id='stat-flash'>--</strong></li>"
              "<li><span>OTA (использовано / свободно) — служебные разделы для обновления прошивки по воздуху</span><strong id='stat-ota'>--</strong></li>"
              "<li><span>NVS (раздел настроек) — постоянные настройки и конфигурации устройства</span><strong id='stat-nvs'>--</strong></li>"
              "<li><span>SPIFFS Used / Free (использовано / свободно в SPIFFS) — файлы интерфейса, логи или другие пользовательские данные</span><strong id='stat-spiffs'>--</strong></li>"
              "</ul></div>"
              "</div></div>";
      }

      // ====== MQTT страница ======
      if(systemPageEnabled("mqtt")){
        String mqttStatusClass = mqttEnabled ? (mqttClient.connected() ? "" : " wait") : " off";
        String mqttStatusText = mqttEnabled
          ? (mqttClient.connected() ? "Подключено: " + mqttHost + ":" + String(mqttPort) : "Ожидание подключения: " + mqttHost + ":" + String(mqttPort))
          : "MQTT отключен";
        if(!mqttHost.length()) mqttStatusText = "MQTT сервер не задан";
      html += "<div id='"+mqttPage.id+"' class='page mqtt-page'><h3>"+mqttPage.title+"</h3>"
              "<div class='card compact'><label class='mqtt-enable-line'><input id='mqtt-enabled' type='checkbox'"+String(mqttEnabled ? " checked" : "")+">MQTT включен</label></div>"
              "<div class='card compact'><div class='mqtt-field'><label>MQTT сервер</label><input id='mqtt-host' value='"+mqttHost+"'></div></div>"
              "<div class='card compact'><div class='mqtt-field'><label>MQTT порт</label><input id='mqtt-port' type='number' min='1' max='65535' value='"+String(mqttPort)+"'></div></div>"
              "<div class='card compact'><div class='mqtt-field'><label>MQTT пользователь</label><input id='mqtt-user' value='"+mqttUsername+"'></div></div>"
              "<div class='card compact'><div class='mqtt-field'><label>MQTT пароль</label><input id='mqtt-pass' type='password' value='"+mqttPassword+"'></div></div>"
              "<div class='card compact'><div class='mqtt-field'><label>Базовый MQTT топик</label><input id='mqtt-base-topic' value='"+mqttBaseTopic+"'></div></div>"
              "<div class='card compact'><label>Состояние</label><div id='mqtt-status-text' class='mqtt-status-text"+mqttStatusClass+"'>"+mqttStatusText+"</div></div>"
              "<div class='mqtt-actions'>"
              "<button class='btn-primary btn-mqtt btn-success' id='mqtt-save-btn' onclick='saveMqttSettings()'>Сохранить настройки</button>"
              "</div></div>";
      }

 // ====== Профиль ======
      if(systemPageEnabled("profile")){
      html += "<div id='"+profilePage.id+"' class='page'><h3>"+profilePage.title+"</h3>"
              "<div class='card compact'>"
              "<h4>Доступ к веб-интерфейсу</h4>"
              "<div class='mqtt-grid'>"
              "<div class='mqtt-field'><label>Логин</label><input id='profile-user' value='"+authUsername+"'></div>"
              "<div class='mqtt-field'><label>Пароль</label><input id='profile-pass' type='password' value='"+authPassword+"'></div>"
              "</div>"
              "<span class='profile-hint'>Пустые поля отключают аутентификацию.</span>"
              "</div>"
              "<div class='card compact'>"
              "<h4>Администратор (для всплывающих окон)</h4>"
              "<div class='mqtt-grid'>"
              "<div class='mqtt-field'><label>Логин администратора</label><input id='profile-admin-user' value='"+adminUsername+"'></div>"
              "<div class='mqtt-field'><label>Пароль администратора</label><input id='profile-admin-pass' type='password' value='"+adminPassword+"'></div>"
              "</div>"
              "<span class='profile-hint'>Пустые поля отключают аутентификацию.</span>"
              "</div>"
              "<div class='mqtt-actions'>"
              "<button class='btn-primary btn-mqtt btn-success' onclick='saveProfileSettings()'>Сохранить</button>"
              "</div>"
              "<div id='profile-status' class='profile-hint'></div>"
              "</div></div>";
      }

      // ====== Основной скрипт страницы ======
            String timerIdsScript = "<script>const timerIds = [";
      bool firstTimerId = true;
      for(const auto &element : self->elements){
        if(element.type != "timer") continue;
        if(!firstTimerId) timerIdsScript += ",";
        timerIdsScript += "\"" + element.id + "\"";
        firstTimerId = false;
      }
      timerIdsScript += "];</script>";
      html += timerIdsScript;

      html += String("<script>const popupAuthRequired=") + (isAdminAuthConfigured() ? "true" : "false") + ";</script>";

      html += R"rawliteral(
  <script>
    let wifiStatusInterval = null;
    let mqttStatusInterval = null;
    let mqttEnabledState = false;
    let mqttConnectedState = false;
    let mqttEnabledTouched = false;
    let mqttSaveTimer = null;
    let espLiveLastOkAt = 0;

  function formatConnectionClock(dateObj){
    const pad = (num)=>String(num).padStart(2, '0');
    return `${pad(dateObj.getHours())}:${pad(dateObj.getMinutes())}:${pad(dateObj.getSeconds())}`;
  }

  function setEspConnectionState(connected){
    const card = document.getElementById('EspLinkStatus');
    const state = document.getElementById('EspLinkStatus_state');
    const last = document.getElementById('EspLinkStatus_last');
    if(!card || !state) return;
    card.classList.toggle('off', !connected);
    state.innerText = connected ? 'ESP32 на связи' : 'Нет связи с ESP32';
    if(last && connected) last.innerText = 'Последний отклик: ' + formatConnectionClock(new Date());
  }

  function markEspLiveOk(){
    espLiveLastOkAt = Date.now();
    setEspConnectionState(true);
  }

  function checkEspLiveTimeout(){
    if(!espLiveLastOkAt || Date.now() - espLiveLastOkAt > 3000){
      setEspConnectionState(false);
    }
  }

  // Показ выбранной страницы и скрытие остальных
  function showPage(id,btn){
    document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
    const targetPage = document.getElementById(id);
    if(!targetPage) return;
    targetPage.classList.add('active'); // Отображаем выбранную страницу
    document.querySelectorAll('#sidebar button').forEach(b=>b.classList.remove('active'));
    if(btn) btn.classList.add('active'); // Активируем кнопку меню

      if(id.startsWith('tab')){
      resizeCustomGraphs(); // Подгоняем размеры графиков под контейнер
      customGraphCanvases.forEach(canvas=>{
        const page = canvas.closest('.page');
        if(page && page.id===id){
          fetchCustomGraph(canvas); // Загружаем данные для графика
          restartCustomGraphInterval(canvas); // Запускаем интервал обновления
        }
      });
    }
    if(id === 'stats') startStatsUpdates();
    else stopStatsUpdates();
    if(id === 'wifi') startWifiStatusUpdates();
    else stopWifiStatusUpdates();
    if(id === 'mqtt') startMqttStatusUpdates();
    else stopMqttStatusUpdates();
  }


  function openPopup(id){
    const modal = document.getElementById('popup-' + id);
    if(!modal) return;
    modal.classList.remove('hidden');
    modal.onclick = (e)=>{ if(e.target === modal) closePopup(id); };
  }

  async function ensurePopupAuthorized(){
    if(!popupAuthRequired) return true;

    const user = prompt('Логин администратора');
    if(user === null) return false;
    const pass = prompt('Пароль администратора');
    if(pass === null) return false;
    const token = btoa(`${user}:${pass}`);
    try {
      const res = await fetch('/popup/auth', {
        headers: { Authorization: `Basic ${token}` },
      });
      if(res.ok){
        return true;
      }
    } catch (err) {
      console.warn('Popup auth verification failed', err);
    }
    alert('Неверный логин или пароль.');
            return false;
    }


  function closePopup(id){
    const modal = document.getElementById('popup-' + id);
    if(modal) modal.classList.add('hidden');
  }

  document.addEventListener('click', async (event)=>{
    const trigger = event.target.closest('[data-popup-open]');
    if(!trigger) return;
    const allowed = await ensurePopupAuthorized();
    if(allowed) openPopup(trigger.dataset.popupOpen);
  });


 // Скрыть/показать боковую панель
function toggleSidebar(){
 let sb=document.getElementById('sidebar');
 sb.classList.toggle('collapsed');
 document.body.classList.toggle('sidebar-hidden', sb.classList.contains('collapsed'));
}

// Отметить, что пользователь изменил элемент вручную
  const markManualChange = el=>{
    if(!el) return;
    el.dataset.manual = '1';
  };

  const wifiInputEdited = {ssid: false, pass: false, ssid2: false, pass2: false};
    const mqttInputEdited = {host: false, port: false, user: false, pass: false, baseTopic: false};
  function scheduleMqttSave(delay = 800){
    if(mqttSaveTimer) clearTimeout(mqttSaveTimer);
    mqttSaveTimer = setTimeout(()=>saveMqttSettings(false), delay);
  }
  function maybeAutoEnableMqttFromHost(hostValue){
    const checkbox = document.getElementById('mqtt-enabled');
    if(!checkbox || mqttEnabledTouched) return;
    if(String(hostValue || '').trim().length > 0) checkbox.checked = true;
  }
  const watchWifiInput = (id)=>{
    const el = document.getElementById(id);
    if(!el) return;
    el.addEventListener('input', ()=>{ wifiInputEdited[id] = true; });
    el.addEventListener('focus', ()=>{
      wifiInputEdited[id] = true;
      markManualChange(el);
    });
  };
  const initializeWifiInputs = ()=>{
    watchWifiInput('ssid');
    watchWifiInput('pass');
    watchWifiInput('ssid2');
    watchWifiInput('pass2');
    [['mqtt-host','host'],['mqtt-port','port'],['mqtt-user','user'],['mqtt-pass','pass'],['mqtt-base-topic','baseTopic']].forEach(([id,key])=>{
      const el = document.getElementById(id);
      if(!el) return;
      el.addEventListener('input', ()=>{ mqttInputEdited[key] = true; markManualChange(el); if(key === 'host') maybeAutoEnableMqttFromHost(el.value); scheduleMqttSave(); });
      el.addEventListener('change', ()=>{ mqttInputEdited[key] = true; markManualChange(el); if(key === 'host') maybeAutoEnableMqttFromHost(el.value); scheduleMqttSave(100); });
      el.addEventListener('focus', ()=>{ mqttInputEdited[key] = true; markManualChange(el); });
    });
    const mqttEnabledInput = document.getElementById('mqtt-enabled');
    if(mqttEnabledInput){
      mqttEnabledInput.addEventListener('change', ()=>{
        mqttEnabledTouched = true;
        markManualChange(mqttEnabledInput);
        scheduleMqttSave(50);
      });
    }
  };
  if(document.readyState === 'loading'){
    document.addEventListener('DOMContentLoaded', initializeWifiInputs);
  } else {
    initializeWifiInputs();
  }

  const formatBytes = (value)=>{
    const num = Number(value);
    if(isNaN(num)) return String(value || 'N/A');
    const units = ['B','KB','MB','GB'];
    let idx = 0; let val = num;
    while(val >= 1024 && idx < units.length-1){ val /= 1024; idx++; }
    const decimals = val >= 10 ? 1 : 2;
    return `${val.toFixed(decimals)} ${units[idx]}`;
  };

  const formatMegaBytes = (value)=>{
    const num = Number(value);
    if(isNaN(num)) return String(value || 'N/A');
    return `${(num / (1024 * 1024)).toFixed(2)} MB`;
  };


  const renderSpiffs = (used, free, total)=>{
    if(typeof used === 'undefined' || typeof free === 'undefined') return '--';
    const percent = total ? ((Number(used)/Number(total))*100).toFixed(1) : '0.0';
    return `${formatBytes(used)} used / ${formatBytes(free)} free (${percent}%)`;
  };

  function updateLevelSensorVisual(id, value){
    const card = document.querySelector(`.level-sensor-card[data-level-sensor="${id}"]`);
    if(!card) return;
    card.classList.remove('sensor-upper','sensor-lower','sensor-drain','sensor-reached','sensor-below','sensor-normal','sensor-low','sensor-empty','sensor-full');
    const text = String(value || '').toLowerCase();
    if(id === 'WaterLevelSensorUpper'){
      card.classList.add('sensor-upper', text.includes('достиг') ? 'sensor-reached' : 'sensor-below');
    } else if(id === 'WaterLevelSensorLower'){
      const low = text.includes('подтвержд') || text.includes('/10') || text.includes('низ');
      card.classList.add('sensor-lower', low ? 'sensor-low' : 'sensor-normal');
    } else if(id === 'WaterLevelSensorDrain'){
      card.classList.add('sensor-drain', text.includes('заполн') ? 'sensor-full' : 'sensor-empty');
    }
  }

  const updateStat = (id, value)=>{
    const el = document.getElementById(id);
    if(el) el.innerText = value;
    updateLevelSensorVisual(id, value);
  };

  function setRs485RelayButton(btn, isOn){
    if(!btn) return;
    btn.dataset.state = isOn ? '1' : '0';
    btn.classList.toggle('on', isOn);
    btn.classList.toggle('off', !isOn);
    btn.innerText = isOn ? 'ON' : 'OFF';
  }

  function syncRs485Panel(j){
    const status = document.getElementById('rs485-status');
    const enabled = j.Rs485Enabled == 1 || j.Rs485Enabled === true || j.Rs485Enabled === '1';
    if(status){
      status.innerText = enabled ? 'RS485 enabled' : 'RS485 disabled';
      status.classList.toggle('off', !enabled);
    }
    const enabledInput = document.getElementById('Rs485Enabled');
    if(enabledInput && !isUserEditing(enabledInput)) enabledInput.checked = enabled;
    const baud = document.getElementById('Rs485BaudRate');
    if(baud && !isUserEditing(baud) && typeof j.Rs485BaudRate !== 'undefined') baud.value = String(j.Rs485BaudRate);
    const uart = document.getElementById('Rs485UartMode');
    if(uart && !isUserEditing(uart) && typeof j.Rs485UartMode !== 'undefined') uart.value = String(j.Rs485UartMode);
    const slave = document.getElementById('Rs485SlaveId');
    if(slave && !isUserEditing(slave) && typeof j.Rs485SlaveId !== 'undefined') slave.value = String(j.Rs485SlaveId);
    const poll = document.getElementById('Rs485PollIntervalMs');
    if(poll && !isUserEditing(poll) && typeof j.Rs485PollIntervalMs !== 'undefined') poll.value = String(j.Rs485PollIntervalMs);

    const relayStates = Array.isArray(j.Rs485RelayActual) ? j.Rs485RelayActual : j.Rs485Relays;
    if(Array.isArray(relayStates)){
      relayStates.forEach((state, idx)=>{
        setRs485RelayButton(document.querySelector(`.rs485-relay-btn[data-relay="${idx}"]`), state == 1 || state === true || state === '1');
      });
    }
    if(Array.isArray(j.Rs485Inputs)){
      j.Rs485Inputs.forEach((state, idx)=>{
        const chip = document.querySelector(`.rs485-input-chip[data-input="${idx}"]`);
        if(!chip) return;
        const isOn = state == 1 || state === true || state === '1';
        chip.classList.toggle('on', isOn);
        chip.innerText = `DI${idx + 1}: ${isOn ? 'ON' : 'OFF'}`;
      });
    }
  }

  function saveRs485Config(key, value){
    if(!key) return;
    fetch('/save?key='+encodeURIComponent(key)+'&val='+encodeURIComponent(value));
  }


    const updateMqttActivationButton = (enabled, connected)=>{
    const checkbox = document.getElementById('mqtt-enabled');
    if(checkbox && !isUserEditing(checkbox)) checkbox.checked = Boolean(enabled);

    const host = (document.getElementById('mqtt-host') || {}).value || '';
    const port = (document.getElementById('mqtt-port') || {}).value || '';
    const status = document.getElementById('mqtt-status-text');
    if(status){
      status.classList.remove('off', 'wait');
      if(!host.trim()){
        status.innerText = 'MQTT сервер не задан';
        status.classList.add('off');
      } else if(!enabled){
        status.innerText = 'MQTT отключен';
        status.classList.add('off');
      } else if(connected){
        status.innerText = `Подключено: ${host}:${port || '1883'}`;
      } else {
        status.innerText = `Ожидание подключения: ${host}:${port || '1883'}`;
        status.classList.add('wait');
      }
    }

    const btn = document.getElementById('mqtt-activate-btn');
    if(btn){
      const active = Boolean(enabled && connected);
      btn.dataset.enabled = enabled ? '1' : '0';
      btn.classList.toggle('btn-activate-on', active);
      btn.classList.toggle('btn-activate-off', !active);
      btn.innerText = 'Реконект MQTT';
    }
  };

  function fetchMqttConfig(){
    fetch('/mqtt/config').then(r=>r.json()).then(data=>{
      const host = document.getElementById('mqtt-host');
      const port = document.getElementById('mqtt-port');
      const user = document.getElementById('mqtt-user');
      const pass = document.getElementById('mqtt-pass');
      const baseTopic = document.getElementById('mqtt-base-topic');
      if(host && !mqttInputEdited.host) host.value = data.host || '';
      if(port && !mqttInputEdited.port) port.value = data.port || '';
      if(user && !mqttInputEdited.user) user.value = data.user || '';
      if(pass && !mqttInputEdited.pass) pass.value = data.pass || '';
      if(baseTopic && !mqttInputEdited.baseTopic) baseTopic.value = data.baseTopic || '';
      mqttEnabledState = Boolean(data.enabled);
      mqttConnectedState = Boolean(data.enabled && data.connected);
      mqttEnabledTouched = Boolean((data.host || '').trim() && !data.enabled);
      updateMqttActivationButton(mqttEnabledState, mqttConnectedState);
    }).catch(()=>{
      mqttConnectedState = false;
      updateMqttActivationButton(false, false);
      
    });
  }

  function saveMqttSettings(manual = true){
    if(mqttSaveTimer){
      clearTimeout(mqttSaveTimer);
      mqttSaveTimer = null;
    }
    const saveBtn = document.getElementById('mqtt-save-btn');
    const originalText = saveBtn ? saveBtn.innerText : '';
    if(saveBtn && manual){ saveBtn.disabled = true; saveBtn.innerText = 'Сохранение...'; }
    const enabledInput = document.getElementById('mqtt-enabled');
    const hostValue = (document.getElementById('mqtt-host') || {}).value || '';
    const enabledValue = enabledInput ? (enabledInput.checked ? '1' : '0') : (mqttEnabledState ? '1' : '0');

    const payload = new URLSearchParams({
      host: hostValue,
      port: (document.getElementById('mqtt-port') || {}).value || '',
      user: (document.getElementById('mqtt-user') || {}).value || '',
      pass: (document.getElementById('mqtt-pass') || {}).value || '',
      baseTopic: (document.getElementById('mqtt-base-topic') || {}).value || '',
      enabled: enabledValue
    });
    mqttEnabledState = enabledValue === '1' && hostValue.trim().length > 0;
    updateMqttActivationButton(mqttEnabledState, mqttConnectedState && mqttEnabledState);
    fetch('/mqtt/save',{method:'POST', body:payload})
      .then(()=>{
        if(manual){
          mqttInputEdited.host = mqttInputEdited.port = mqttInputEdited.user = mqttInputEdited.pass = mqttInputEdited.baseTopic = false;
          ['mqtt-host','mqtt-port','mqtt-user','mqtt-pass','mqtt-base-topic','mqtt-enabled'].forEach(id=>{
            const el = document.getElementById(id);
            if(el && document.activeElement !== el) clearManualFlag(el, 150);
          });
        }
        setTimeout(fetchMqttConfig, manual ? 300 : 1000);
      })

      .finally(()=>{ if(saveBtn && manual){ saveBtn.disabled = false; saveBtn.innerText = originalText || 'Сохранить настройки'; } });
  }

function saveProfileSettings(){
 const statusEl = document.getElementById('profile-status');
    const user = (document.getElementById('profile-user') || {}).value || '';
    const pass = (document.getElementById('profile-pass') || {}).value || '';
    const adminUser = (document.getElementById('profile-admin-user') || {}).value || '';
    const adminPass = (document.getElementById('profile-admin-pass') || {}).value || '';
    if(statusEl) statusEl.innerText = 'Сохранение...';

    const payload = new URLSearchParams({user, pass, adminUser, adminPass});
    fetch('/profile/save', {method:'POST', body: payload})
      .then(()=>{ if(statusEl) statusEl.innerText = 'Сохранено.'; })
      .catch(()=>{ if(statusEl) statusEl.innerText = 'Ошибка сохранения.'; })
      .finally(()=>{ setTimeout(()=>{ if(statusEl) statusEl.innerText = ''; }, 2500); });
  }

  function toggleMqttActivation(){
    const checkbox = document.getElementById('mqtt-enabled');
    if(checkbox){
      checkbox.checked = !checkbox.checked;
      markManualChange(checkbox);
    } else {
      mqttEnabledState = !mqttEnabledState;
    }
    saveMqttSettings(true);
  }

  function startMqttStatusUpdates(){
    if(mqttStatusInterval) return;
    fetchMqttConfig();
    mqttStatusInterval = setInterval(fetchMqttConfig, 4000);
  }

  function stopMqttStatusUpdates(){
    if(!mqttStatusInterval) return;
    clearInterval(mqttStatusInterval);
    mqttStatusInterval = null;
  }


  function fetchStats(manual=false){
    const refreshBtn = document.getElementById('refresh-stats-btn');
    if(manual && refreshBtn){
      refreshBtn.disabled = true;
      refreshBtn.innerText = 'Обновление...';
    }
    fetch('/stats').then(r=>r.json()).then(data=>{
      updateStat('stat-chip', data.chipModelRevision || '--');
      const cpuFreq = typeof data.cpuFreqMHz !== 'undefined' ? data.cpuFreqMHz : null;
      updateStat('stat-cpu', cpuFreq !== null ? `${cpuFreq} MHz` : 'N/A');
      const tempVal = (typeof data.temperature !== 'undefined' && data.temperature !== null) ? data.temperature : 'N/A';
      const tempText = isNaN(Number(tempVal)) ? String(tempVal) : `${Number(tempVal).toFixed(1)} °C`;
      updateStat('stat-temp', tempText);
      updateStat('stat-uptime', data.uptime || '--');
      updateStat('stat-mac', data.mac || 'N/A');
      updateStat('stat-heap', typeof data.freeHeap !== 'undefined' ? formatBytes(data.freeHeap) : '--');
       const freePsram = (typeof data.freePsram === 'undefined' || data.freePsram === null) ? null : data.freePsram;
      const totalPsram = (typeof data.totalPsram === 'undefined' || data.totalPsram === null) ? null : data.totalPsram;
      let psramText = 'N/A';
      if(!isNaN(Number(freePsram)) && !isNaN(Number(totalPsram))){
        const usedPsram = Number(totalPsram) - Number(freePsram);
        psramText = `${formatMegaBytes(usedPsram)} / ${formatMegaBytes(freePsram)}`;
      } else if(!isNaN(Number(freePsram))){
        psramText = formatMegaBytes(freePsram);
      }
      updateStat('stat-psram', psramText);
            const flashUsed = typeof data.flashUsed !== 'undefined' && data.flashUsed !== null ? Number(data.flashUsed) : null;
      const flashFree = typeof data.flashFree !== 'undefined' && data.flashFree !== null ? Number(data.flashFree) : null;
      const flashText = (!isNaN(flashUsed) && !isNaN(flashFree))
        ? `${formatBytes(flashUsed)} / ${formatBytes(flashFree)}`
        : (typeof data.flashTotal !== 'undefined' && data.flashTotal !== null ? formatBytes(data.flashTotal) : '--');
      updateStat('stat-flash', flashText);
      updateStat('stat-nvs', typeof data.nvsTotal !== 'undefined' && data.nvsTotal !== null ? formatBytes(data.nvsTotal) : 'N/A');
      const otaUsed = typeof data.otaUsed !== 'undefined' && data.otaUsed !== null ? Number(data.otaUsed) : null;
      const otaFree = typeof data.otaFree !== 'undefined' && data.otaFree !== null ? Number(data.otaFree) : null;
      const otaParts = [];
      if(!isNaN(otaUsed) && !isNaN(otaFree)){
        otaParts.push(`${formatBytes(otaUsed)} / ${formatBytes(otaFree)}`);
      }
      if(data.otaRunningLabel){
        otaParts.push(`Активный: ${data.otaRunningLabel}`);
      }
      if(typeof data.otaSlot0Size !== 'undefined' && data.otaSlot0Size !== null){
        otaParts.push(`ota_0: ${formatBytes(data.otaSlot0Size)}`);
      }
      if(typeof data.otaSlot1Size !== 'undefined' && data.otaSlot1Size !== null){
        otaParts.push(`ota_1: ${formatBytes(data.otaSlot1Size)}`);
      }
      updateStat('stat-ota', otaParts.length ? otaParts.join(' · ') : 'N/A');
      updateStat('stat-spiffs', renderSpiffs(data.spiffsUsed, data.spiffsFree, data.spiffsTotal));
    }).catch(()=>{}).finally(()=>{
      if(manual && refreshBtn){
        refreshBtn.disabled = false;
        refreshBtn.innerText = 'Обновить информацию';
      }
    });
  }

  function startStatsUpdates(){
    fetchStats();
  }

  function stopStatsUpdates(){
  }

  function rebootEsp(){
    const rebootBtn = document.getElementById('reboot-btn');
    const original = rebootBtn ? rebootBtn.innerText : '';
    if(rebootBtn){
      rebootBtn.disabled = true;
      rebootBtn.innerText = 'Перезагрузка...';
    }
    fetch('/restart',{method:'POST'})
      .catch(()=>{})
      .finally(()=>{
        setTimeout(()=>location.reload(), 4000);
      });
  }

    function ensureTimeZoneOptions(){
    const select = document.getElementById('gmtOffset');
    if(!select || select.options.length) return;
    for(let offset = -12; offset <= 14; offset++){
      const opt = document.createElement('option');
      const sign = offset >= 0 ? '+' : '';
      opt.value = String(offset);
      opt.textContent = `GMT${sign}${offset}`;
      select.appendChild(opt);
    }
  }

  function syncManualTimeInputs(value){
    const dateInput = document.getElementById('manual-date');
    const timeInput = document.getElementById('manual-time');
    if(!dateInput || !timeInput) return;
    if(dateInput.dataset.manual === '1' || timeInput.dataset.manual === '1') return;
    const parsed = parseDeviceDateTime(value || '');
    if(!parsed) return;
    const pad = (num)=>String(num).padStart(2, '0');
    const dateStr = `${parsed.getFullYear()}-${pad(parsed.getMonth()+1)}-${pad(parsed.getDate())}`;
    const timeStr = `${pad(parsed.getHours())}:${pad(parsed.getMinutes())}:${pad(parsed.getSeconds())}`;
    if(dateInput.value !== dateStr) dateInput.value = dateStr;
    if(timeInput.value !== timeStr) timeInput.value = timeStr;
  }

  function applyManualTime(){
    const statusEl = document.getElementById('manual-time-status');
    const dateInput = document.getElementById('manual-date');
    const timeInput = document.getElementById('manual-time');
    const tzSelect = document.getElementById('gmtOffset');
    if(!dateInput || !timeInput) return;
    const dateVal = dateInput.value || '';
    const timeVal = timeInput.value || '';
    if(!dateVal || !timeVal){
      if(statusEl) statusEl.innerText = 'Укажите дату и время.';
      return;
    }
    if(statusEl) statusEl.innerText = 'Установка...';
    const payload = new URLSearchParams({date: dateVal, time: timeVal});
    if(tzSelect && tzSelect.value !== ''){
      payload.append('gmtOffset', tzSelect.value);
    }
    fetch('/time/set', {method:'POST', body: payload})
      .then(async r=>{
        let data = null;
        try{
          data = await r.json();
        } catch(err){
          data = null;
        }
        if(!r.ok){
          const message = data && data.error ? data.error : 'Ошибка установки.';
          throw new Error(message);
        }
        return data;
      })
      .then(data=>{
        if(data && data.status === 'ok'){
          if(statusEl) statusEl.innerText = 'Время установлено.';
          if(data.current){
            updatePageDateTime(data.current);
            syncManualTimeInputs(data.current);
          } else {
            const parsed = parseManualDateTime(dateVal, timeVal);
            if(parsed){
              updatePageDateTime(formatDateTime(parsed));
              syncManualTimeInputs(formatDateTime(parsed));
            }
          }
        } else if(statusEl) {
          statusEl.innerText = data && data.error ? data.error : 'Ошибка установки.';
        }
      })
      .catch(err=>{ if(statusEl) statusEl.innerText = err.message || 'Ошибка установки.'; })
      .finally(()=>{ setTimeout(()=>{ if(statusEl) statusEl.innerText = ''; }, 2500); });
  }

  function parseManualDateTime(dateVal, timeVal){
    if(!dateVal || !timeVal) return null;
    const dateParts = dateVal.includes('.') ? dateVal.split('.') : dateVal.split('-');
    if(dateParts.length !== 3) return null;
    let year = 0;
    let month = 0;
    let day = 0;
    if(dateVal.includes('.')){
      day = Number(dateParts[0]);
      month = Number(dateParts[1]);
      year = Number(dateParts[2]);
    } else {
      year = Number(dateParts[0]);
      month = Number(dateParts[1]);
      day = Number(dateParts[2]);
    }
    const timeParts = timeVal.split(':').map(Number);
    const hour = timeParts[0] || 0;
    const minute = timeParts[1] || 0;
    const second = timeParts[2] || 0;
    if(!year || !month || !day) return null;
    return new Date(year, month - 1, day, hour, minute, second);
  }

  function initTimeSettings(){
    ensureTimeZoneOptions();
    const dateInput = document.getElementById('manual-date');
    const timeInput = document.getElementById('manual-time');
    const tzSelect = document.getElementById('gmtOffset');
    [dateInput, timeInput, tzSelect].forEach(el=>{
      if(!el) return;
      listenToManual(el);
    });
  }


  const updateWifiStatus = (data)=>{
    updateStat('wifi-status', data.wifiStatus || 'N/A');
    updateStat('wifi-mode', data.wifiMode || 'N/A');
    updateStat('wifi-ssid', data.ssid && data.ssid.length ? data.ssid : 'N/A');
    updateStat('wifi-ip', data.localIp && data.localIp.length ? data.localIp : 'N/A');
    const rssiVal = (typeof data.rssi !== 'undefined' && data.rssi !== null) ? data.rssi : 'N/A';
    const rssiText = isNaN(Number(rssiVal)) ? String(rssiVal) : `${rssiVal} dBm`;
    updateStat('wifi-rssi', rssiText);
    // Не перезаписываем поля WiFi статусом соединения: форма должна показывать сохраненные настройки, а не случайно стирать ввод пользователя.

  };

  function fetchWifiStatus(){
    fetch('/stats').then(r=>r.json()).then(updateWifiStatus).catch(()=>{});
  }

  function startWifiStatusUpdates(){
    if(wifiStatusInterval) return;
    fetchWifiStatus();
    wifiStatusInterval = setInterval(fetchWifiStatus, 3000);
  }

  function stopWifiStatusUpdates(){
    if(!wifiStatusInterval) return;
    clearInterval(wifiStatusInterval);
    wifiStatusInterval = null;
  }

// Подсветка и отображение текущего значения для панели LED Color
const refreshLedColorUI = (val)=>{
  const ledInput = document.getElementById('LEDColor');
  if(!ledInput) return;
  let colorValue = String(val || ledInput.value || '').toUpperCase();
  if(!colorValue.startsWith('#')) colorValue = '#' + colorValue.replace(/^#?/, '');
  if(colorValue === '#') colorValue = '#33B8FF';
  const card = ledInput.closest('.card');
  if(card) card.style.setProperty('--led-color', colorValue);
  const label = card ? card.querySelector('label') : null;
  if(label) label.setAttribute('data-color', colorValue);
};
// Подсветка и отображение текущего значения для панели Theme Color
const refreshThemeColorUI = (val)=>{
  const themeInput = document.getElementById('ThemeColor');
  if(!themeInput) return;
  let colorValue = String(val || themeInput.value || '').toUpperCase();
  if(!colorValue.startsWith('#')) colorValue = '#' + colorValue.replace(/^#?/, '');
  if(colorValue === '#') colorValue = '#6DD5ED';
  const card = themeInput.closest('.card');
  if(card) card.style.setProperty('--theme-color', colorValue);
  const label = card ? card.querySelector('label') : null;
  if(label) label.setAttribute('data-color', colorValue);
  document.body.style.background = colorValue;
};

const chemicalInputIds = new Set(['PH_Lower','PH_Upper','CL_Lower','CL_Upper','PH_setting','PH1','PH2','PH1_CAL','PH2_CAL','Temper_Reference','Temper_PH']);
const rs485InputIds = new Set(['Rs485Enabled','Rs485BoardType','Rs485BaudRate','Rs485UartMode','Rs485SlaveId','Rs485PollIntervalMs']);

// Очистка флага ручного изменения через delay (по умолчанию 600 мс)
const clearManualFlag = (el, delay=600)=>{
  if(!el) return;
  setTimeout(()=>{
    if(el.dataset.manual === '1') el.dataset.manual = '';
  }, delay);
};

// Debounce сохранения для часто меняющихся контролов
const saveDebounceTimers = {};
const scheduleSave = (key, value, delay=300)=>{
  if(!key) return;
  if(saveDebounceTimers[key]) clearTimeout(saveDebounceTimers[key]);
  saveDebounceTimers[key] = setTimeout(()=>{
    fetch('/save?key='+encodeURIComponent(key)+'&val='+encodeURIComponent(value))
      .finally(()=>{
        const el = document.getElementById(key);
        const holdMs = chemicalInputIds.has(key) ? 4000 : 1500;
        if(el && document.activeElement !== el) clearManualFlag(el, holdMs);
      });
    delete saveDebounceTimers[key];
  }, delay);
};



// Обновление слайдера и отправка значения на сервер
function updateSlider(el){
 document.getElementById(el.id+'Val').innerText=' '+el.value;
 scheduleSave(el.id, el.value);
}

function isUserEditing(el){
  return !!el && (el.dataset.manual === '1' || document.activeElement === el);
}

function isRangeSliderEditing(id){
  const minEl = document.getElementById(id+"Min");
  const maxEl = document.getElementById(id+"Max");
  return isUserEditing(minEl) || isUserEditing(maxEl);
}

// Обновление значения input (текст, число и т.д.) с проверкой ручного ввода
function updateInputValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || isUserEditing(el)) return;
  const text = String(value);
  if(el.value !== text) el.value = text; //Динамическое обновление "Comment", "Start Time", "Enter Integer", "Enter Float"
  if(id==='LEDColor') refreshLedColorUI(text);
  else if(id==='ThemeColor') refreshThemeColorUI(text);
}

function updateCheckboxValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || isUserEditing(el)) return;
  const isOn = value == 1 || value === true || value === "1" || String(value).toLowerCase() === "true";
  el.checked = isOn;
}


function updateSelectValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || isUserEditing(el)) return;
  const text = String(value);
  if(el.value != text) el.value = text;
}

function applyLiveValue(id, value){
  const el = document.getElementById(id);
  if(!el || isUserEditing(el)) return;
  if(el.classList.contains('rs485-panel')) return;
  if(el.classList.contains('range-slider')){
    if(isRangeSliderEditing(id)) return;
    const parts = String(value||'').split('-').map(v=>v.trim()).filter(Boolean);
    if(parts.length >= 2){
      setRangeSliderUI(id, parts[0], parts[1]);
    }
    return;
  }
  if(el.matches('button[data-type="dashButton"]')){
    syncDashButton(id, value);
    return;
  }
  if(el.matches('input[type=checkbox]')){
    updateCheckboxValue(id, value);
    return;
  }
  if(el.matches('input[type=range]')){
    updateSliderDisplay(id, value);
    return;
  }
  if(el.matches('input[type=text],input[type=number],input[type=time],input[type=color]')){
    updateInputValue(id, value);
    return;
  }
  if(el.tagName === 'SELECT'){
    updateSelectValue(id, value);
    return;
  }
  if(el.matches('div.select-days')){
    // Обновляем любой элемент выбора дней, независимо от суффикса id.
    updateDaysSelection(id, value);
    return;
  }
  updateStat(id, value);
}


function updateDaysSelection(id, value){
  const container = document.getElementById(id);
  if(!container || container.dataset.manual === '1') return;
  const tokens = (value||"").split(',').map(s=>s.trim()).filter(Boolean);
  const selected = new Set(tokens);
  container.querySelectorAll('input[type=checkbox]').forEach(chk=>{
    chk.checked = selected.has(chk.value);
  });
}

function updateSliderDisplay(id, value){
  const sl = document.getElementById(id);
  if(isUserEditing(sl)) return;
  if(sl && sl.value != value) sl.value = value;
  const label = document.getElementById(id+'Val');
  if(label) label.innerText = ' ' + value;
}
function setRangeSliderUI(id, minVal, maxVal){
  const minEl = document.getElementById(id+"Min");
  const maxEl = document.getElementById(id+"Max");
  if(!minEl || !maxEl) return;
  let min = parseFloat(minVal);
  let max = parseFloat(maxVal);
  if(min > max) [min, max] = [max, min];
  minEl.value = min;
  maxEl.value = max;
  const display = document.getElementById(id+"Val");
  if(display) display.innerText = min + ' - ' + max;
  const track = document.querySelector('#'+id+' .slider-track');
  if(track){
    const minLimit = parseFloat(minEl.min || 0);
    const maxLimit = parseFloat(minEl.max || 100);
    const span = (maxLimit - minLimit) || 1;
    const minPct = ((min - minLimit) / span) * 100;
    const maxPct = ((max - minLimit) / span) * 100;
    track.style.background = `linear-gradient(to right, #444 ${minPct}%, #1e88e5 ${minPct}%, #1e88e5 ${maxPct}%, #444 ${maxPct}%)`;
  }
}

function syncDashButton(id, state){
  const btn = document.getElementById(id);
  if(!btn || typeof state === 'undefined') return;
  const isOn = state == 1 || state === true || state === "1";
  btn.dataset.state = isOn ? "1" : "0";
  btn.classList.toggle('on', isOn);
  btn.classList.toggle('off', !isOn);
  btn.innerText = isOn ? "ON" : "OFF";
}

function toggleButton(id){
  const btn = document.getElementById(id);
  if(!btn) return;
  let newState = btn.dataset.state == "1" ? 0 : 1;
  btn.dataset.state = newState;
  btn.classList.toggle('on', newState == 1);
  btn.classList.toggle('off', newState == 0);
  fetch('/button?id=' + encodeURIComponent(id) + '&state=' + newState)
    .then(resp => {
      btn.innerText = newState == 1 ? "ON" : "OFF";
    });
}

document.querySelectorAll('button[data-type="dashButton"]').forEach(b=>{
  b.addEventListener('click', ()=>toggleButton(b.id));
});

// document.getElementById('ThemeColor').addEventListener('change', (e)=>{
//   const c = e.target.value;
//   document.body.style.background = c;
//   fetch('/save?key=ThemeColor&val='+encodeURIComponent(c));
// });
const listenToManual = el=>{
  if(!el) return;
  el.addEventListener('focus', ()=> markManualChange(el));
  el.addEventListener('blur', ()=> clearManualFlag(el, chemicalInputIds.has(el.id) ? 4000 : 600));
};

function initRs485PanelControls(){
  document.querySelectorAll('.rs485-relay-btn').forEach(btn=>{
    btn.addEventListener('click', ()=>{
      const relay = Number(btn.dataset.relay);
      const next = btn.dataset.state === '1' ? 0 : 1;
      setRs485RelayButton(btn, next === 1);
      fetch('/rs485/relay?idx='+encodeURIComponent(relay)+'&state='+next)
        .then(r=>r.json())
        .then(data=>{
          if(data && typeof data.state !== 'undefined') setRs485RelayButton(btn, data.state == 1);
        })
        .catch(()=>{});
    });
  });

  ['Rs485BaudRate','Rs485UartMode','Rs485SlaveId','Rs485PollIntervalMs'].forEach(id=>{
    const el = document.getElementById(id);
    if(!el) return;
    listenToManual(el);
    el.addEventListener('change', ()=>{
      markManualChange(el);
      saveRs485Config(id, el.value);
      clearManualFlag(el, 1200);
    });
  });

  const enabledInput = document.getElementById('Rs485Enabled');
  if(enabledInput){
    listenToManual(enabledInput);
    enabledInput.addEventListener('change', ()=>{
      markManualChange(enabledInput);
      saveRs485Config('Rs485Enabled', enabledInput.checked ? 1 : 0);
      clearManualFlag(enabledInput, 1200);
    });
  }

  const pollNow = document.getElementById('rs485-poll-now');
  if(pollNow){
    pollNow.addEventListener('click', ()=>fetch('/rs485/poll', {method:'POST'}).finally(()=>setTimeout(fetchLive, 250)));
  }

  const allOff = document.getElementById('rs485-all-off');
  if(allOff){
    allOff.addEventListener('click', ()=>{
      fetch('/rs485/all-off', {method:'POST'}).finally(()=>setTimeout(fetchLive, 250));
    });
  }
}

if(document.readyState === 'loading'){
  document.addEventListener('DOMContentLoaded', initTimeSettings);
} else {
  initTimeSettings();
}
initRs485PanelControls();

const themeInput = document.getElementById('ThemeColor');
if(themeInput){
  listenToManual(themeInput);
  refreshThemeColorUI(themeInput.value);
  themeInput.addEventListener('change', (e)=>{
    markManualChange(themeInput);
    const c = e.target.value;
    refreshThemeColorUI(c);
    fetch('/save?key=ThemeColor&val='+encodeURIComponent(c));
  });
}


document.querySelectorAll('input[type=text],input[type=number],input[type=time],input[type=color],select').forEach(el=>{
  if(el.id=='ThemeColor') return;
  if(rs485InputIds.has(el.id)) return;
    if(el.dataset.skipSave === '1'){
    listenToManual(el);
    return;
  }
  if(chemicalInputIds.has(el.id)){
    el.addEventListener('input', ()=>{
      markManualChange(el);
      scheduleSave(el.id, el.value, 500);
    });
  }
  el.addEventListener('change', ()=>{
    markManualChange(el);
    if(el.id==='LEDColor') refreshLedColorUI(el.value);
    fetch('/save?key='+el.id+'&val='+encodeURIComponent(el.value));
  });
  listenToManual(el);
  if(el.id==='LEDColor') refreshLedColorUI(el.value);
});

document.querySelectorAll('input[type=checkbox][id]').forEach(el=>{
  el.addEventListener('change', ()=>{
    markManualChange(el);
    const value = el.checked ? 1 : 0;
    fetch('/save?key='+el.id+'&val='+encodeURIComponent(value));
    clearManualFlag(el);
  });
  listenToManual(el);
});

document.querySelectorAll('div.select-days').forEach(el=>{
  // Сохраняем выбор по каждому .select-days, даже если id отличается.
  el.querySelectorAll('input[type=checkbox]').forEach(chk=>{
    chk.addEventListener('change', ()=>{
      let selected = Array.from(el.querySelectorAll('input[type=checkbox]:checked')).map(c=>c.value).join(',');
      el.dataset.manual = '1';
      fetch('/save?key='+el.id+'&val='+encodeURIComponent(selected));
      clearManualFlag(el);
    });
  });
});


const escapeHtml = (text='')=>String(text)
  .replace(/&/g,'&amp;')
  .replace(/</g,'&lt;')
  .replace(/>/g,'&gt;')
  .replace(/"/g,'&quot;')
  .replace(/'/g,'&#039;');

const authLabel = (auth)=>{
  const val = (auth || '').toString().toUpperCase();
  if(val.includes('WPA3')) return 'WPA3';
  if(val.includes('WPA2')) return 'WPA2';
  if(val.includes('WPA')) return 'WPA';
  if(val.includes('WEP')) return 'WEP';
  return 'open';
};

function closeWifiModal(){
  const modal = document.getElementById('wifi-scan-modal');
  if(modal) modal.classList.add('hidden');
}

function renderWifiScanList(networks){
  const list = document.getElementById('wifi-scan-list');
  const modal = document.getElementById('wifi-scan-modal');
  if(!list || !modal) return;
  list.innerHTML = '';
  if(!networks || !networks.length){
    list.innerHTML = '<div class="empty-row">Сети не найдены</div>';
  } else {
    networks.forEach(net=>{
      const btn = document.createElement('button');
      btn.className = 'network-row';
      const ssid = escapeHtml(net.ssid || '(hidden)');
      const rssi = typeof net.rssi !== 'undefined' ? `${net.rssi} dBm` : 'n/a';
      const auth = authLabel(net.auth);
      btn.innerHTML = `<div class='network-ssid'>${ssid}</div><div class='network-meta'>${rssi} · ${auth}</div>`;
      btn.addEventListener('click', ()=>{
        const ssidInput = document.getElementById('ssid');
        if(ssidInput){
          ssidInput.value = net.ssid || '';
          wifiInputEdited.ssid = true;
        }
        closeWifiModal();
      });
      list.appendChild(btn);
    });
  }
  modal.classList.remove('hidden');
  modal.onclick = (e)=>{ if(e.target === modal) closeWifiModal(); };
}

function showWifiScanPlaceholder(text){
  const list = document.getElementById('wifi-scan-list');
  const modal = document.getElementById('wifi-scan-modal');
  if(!list || !modal) return;
  list.innerHTML = `<div class="empty-row">${escapeHtml(text)}</div>`;
  modal.classList.remove('hidden');
}

function scanWiFi(){
  const btn = document.getElementById('scan-btn');
  if(btn){ btn.disabled = true; btn.innerText = 'Scanning...'; }
  showWifiScanPlaceholder('Идет сканирование...');
  fetch('/wifi/scan')
    .then(r=>r.json())
    .then(data=>{ renderWifiScanList(Array.isArray(data) ? data : []); })
    .catch(()=>{ renderWifiScanList([]); })
    .finally(()=>{
      if(btn){ btn.disabled = false; btn.innerText = 'Scan WiFi'; }
    });
}

function saveWiFi(){
 let s=document.getElementById('ssid').value;
 let p=document.getElementById('pass').value;
 let s2=document.getElementById('ssid2').value;
 let p2=document.getElementById('pass2').value;
 let aps=document.getElementById('ap_ssid').value;
 let app=document.getElementById('ap_pass').value;
 let h=document.getElementById('hostname').value;
 const body = new URLSearchParams({ssid:s, pass:p, ssid2:s2, pass2:p2, ap_ssid:aps, ap_pass:app, hostname:h});
 fetch('/wifi/save', {method:'POST', body})
   .then(r=>r.json())
   .then(data=>{
     const connected = data && data.connected ? ' (подключено)' : ' (AP mode)';
     wifiInputEdited.ssid = false;
     wifiInputEdited.pass = false;
     wifiInputEdited.ssid2 = false;
     wifiInputEdited.pass2 = false;
     alert('WiFi сохранен' + connected);
     fetchStats(true);
   })
   .catch(()=>alert('Не удалось сохранить WiFi'));
}

let customGraphCanvases = Array.from(document.querySelectorAll("canvas[id^='graph_']"));
const customGraphTimers = new Map();
const graphDataCache = new Map();
const graphTooltip = document.createElement('div');
graphTooltip.className = 'graph-tooltip hidden';
document.body.appendChild(graphTooltip);

customGraphCanvases.forEach(canvas=>{
  canvas.addEventListener('mousemove', evt=>showGraphTooltip(canvas, evt));
  canvas.addEventListener('mouseleave', hideGraphTooltip);
});

function resizeCustomGraphs(){
  customGraphCanvases.forEach(canvas=>{
    if(!canvas.parentElement) return;
    const w = canvas.parentElement.clientWidth;
    if(w && w > 0) canvas.width = w;
    const h = parseFloat(getComputedStyle(canvas).height);
    if(!isNaN(h) && h > 0) canvas.height = h;
  });
}

function populateGraphTable(tableId, data){
  if(!tableId || !data) return;
  const table = document.getElementById(tableId);
  if(!table) return;
  const tbody = table.querySelector('tbody');
  if(!tbody) return;
  tbody.innerHTML = '';
  for(let i = data.length - 1, row = 1; i >= 0; i--, row++){
    const point = data[i];
    const tr = document.createElement('tr');
    const events = Number(point.events || 0);
    const eventText = events > 0 ? ('x' + events) : '';
     tr.innerHTML = '<td>'+row+'</td><td>'+point.time+'</td><td>'+point.value+'</td><td>'+eventText+'</td>';
    tbody.appendChild(tr);
  }
}

function formatGraphNumber(value){
  const num = Number(value);
  if(!Number.isFinite(num)) return '--';
  const abs = Math.abs(num);
  const digits = abs >= 100 ? 0 : (abs >= 10 ? 1 : 2);
  return num.toFixed(digits).replace(/\.?0+$/, '');
}

function formatGraphHeadline(canvas, value){
  const yLabel = (canvas.dataset.yLabel || '').toLowerCase();
  const series = (canvas.dataset.series || '').toLowerCase();
  let unit = '';
  if(yLabel.includes('temperature') || yLabel.includes('темпер')) unit = '°C';
  else if(yLabel.includes('ph') || series.includes('ph')) unit = ' pH';
  else if(yLabel.includes('хлор') || yLabel.includes('cl') || yLabel.includes('ppm')) unit = ' ppm';
  return formatGraphNumber(value) + unit;
}

function graphEventName(canvas){
  const yLabel = (canvas.dataset.yLabel || '').toLowerCase();
  const series = (canvas.dataset.series || '').toLowerCase();
  if(yLabel.includes('хлор') || yLabel.includes('cl') || yLabel.includes('ppm')) return 'NaOCl';
  if(yLabel.includes('ph') || series.includes('ph')) return 'ACO';
  return 'Dose';
}

function graphEventShortName(canvas){
  const fullName = graphEventName(canvas);
  if(fullName === 'NaOCl') return 'Cl';
  if(fullName === 'ACO') return 'ACO';
  return 'D';
}

function graphPointPosition(metrics, index){
  const count = metrics.points.length;
  const x = count <= 1
    ? metrics.plotLeft + metrics.plotWidth / 2
    : metrics.plotLeft + index * (metrics.plotWidth / (count - 1));
  const value = Number(metrics.points[index].value);
  const y = metrics.plotBottom - ((value - metrics.minValue) / (metrics.maxValue - metrics.minValue || 1)) * metrics.plotHeight;
  return {x, y};
}

function drawCustomGraph(canvas,data){
  if(!canvas || !canvas.getContext) return;
  const ctx = canvas.getContext('2d');
  if(!ctx) return;
  const width = canvas.width;
  const height = canvas.height;
  ctx.clearRect(0,0,width,height);
  const bg = ctx.createLinearGradient(0, 0, 0, height);
  bg.addColorStop(0, '#05070a');
  bg.addColorStop(0.48, '#07131c');
  bg.addColorStop(1, '#04070c');
  ctx.fillStyle = bg;
  ctx.fillRect(0,0,width,height);
  if(!data.length){
    graphDataCache.set(canvas, {points: [], plotLeft: 0, plotWidth: width, plotBottom: height, minValue: 0, maxValue: 1});
    populateGraphTable(canvas.dataset.tableId, []);
    return;
  }

  const maxPointsAttr = parseInt(canvas.dataset.maxPoints);
  const maxPoints = !isNaN(maxPointsAttr) && maxPointsAttr > 0
    ? maxPointsAttr
    : 10;
  const pointsToDraw = data.slice(-maxPoints);
  const values = pointsToDraw.map(point=>Number(point.value)).filter(Number.isFinite);
  let minValue = values.length ? Math.min(...values) : 0;
  let maxValue = values.length ? Math.max(...values) : 1;
  if(minValue === maxValue){
    minValue -= 1;
    maxValue += 1;
  } else {
    const padding = (maxValue - minValue) * 0.18;
    minValue -= padding;
    maxValue += padding;
  }

  const plotTop = Math.max(84, Math.min(height - 70, Math.round(height * 0.48)));
  const plotBottom = Math.max(plotTop + 32, height - 38);
  const plotLeft = 14;
  const plotRight = Math.max(plotLeft + 10, width - 16);
  const plotWidth = plotRight - plotLeft;
  const plotHeight = plotBottom - plotTop;
  const metrics = {points: pointsToDraw, plotLeft, plotWidth, plotBottom, plotHeight, minValue, maxValue};

  ctx.strokeStyle = 'rgba(255,255,255,0.07)';
  ctx.lineWidth = 1;
  for(let i=0;i<=4;i++){
    let y = plotTop + i*(plotHeight/4);
    ctx.beginPath();
    ctx.moveTo(plotLeft,y);
    ctx.lineTo(plotRight,y);
    ctx.stroke();
  }
  for(let i=0;i<=10;i++){
    let x = plotLeft + i*(plotWidth/10);
    ctx.beginPath();
    ctx.moveTo(x,plotTop);
    ctx.lineTo(x,plotBottom);
    ctx.stroke();
  }

  const latest = pointsToDraw[pointsToDraw.length - 1];
  const headlineSize = Math.max(34, Math.min(86, Math.round(height * 0.22)));
  ctx.save();
  ctx.font = '700 ' + headlineSize + 'px "Inter", "Segoe UI", system-ui, sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  ctx.shadowColor = 'rgba(0, 255, 255, 0.95)';
  ctx.shadowBlur = 22;
  ctx.fillStyle = 'rgba(26, 245, 255, 0.96)';
  const headlineValue = typeof canvas.dataset.liveValue !== 'undefined' ? canvas.dataset.liveValue : latest.value;
  ctx.fillText(formatGraphHeadline(canvas, headlineValue), width / 2, Math.max(42, plotTop * 0.45));
  ctx.restore();

  const lineColor = canvas.dataset.lineColor || '#4CAF50';
  const pointColor = canvas.dataset.pointColor || '#ff0000';
  ctx.strokeStyle = lineColor;
  ctx.lineWidth = 2;
  ctx.beginPath();
  for(let i=0;i<pointsToDraw.length;i++){
    const {x, y} = graphPointPosition(metrics, i);
    if(i==0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();

  ctx.fillStyle = pointColor;
  for(let i=0;i<pointsToDraw.length;i++){
    const {x, y} = graphPointPosition(metrics, i);
    ctx.beginPath();
    ctx.arc(x,y,3,0,2*Math.PI);
    ctx.fill();
  }

  const eventName = graphEventShortName(canvas);
  ctx.save();
  ctx.font = '700 12px "Inter", "Segoe UI", system-ui, sans-serif';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  for(let i=0;i<pointsToDraw.length;i++){
    const events = Number(pointsToDraw[i].events || 0);
    if(!events) continue;
    const {x, y} = graphPointPosition(metrics, i);
    const label = eventName + ' x' + events;
    const labelWidth = ctx.measureText(label).width + 12;
    const labelY = Math.max(plotTop + 14, Math.min(plotBottom - 12, y - 22));
    ctx.strokeStyle = 'rgba(255, 209, 102, 0.72)';
    ctx.lineWidth = 1.5;
    ctx.setLineDash([5, 5]);
    ctx.beginPath();
    ctx.moveTo(x, plotTop);
    ctx.lineTo(x, plotBottom);
    ctx.stroke();
    ctx.setLineDash([]);
    ctx.fillStyle = 'rgba(255, 209, 102, 0.16)';
    ctx.fillRect(x - labelWidth / 2, labelY - 10, labelWidth, 20);
    ctx.strokeStyle = 'rgba(255, 209, 102, 0.85)';
    ctx.strokeRect(x - labelWidth / 2, labelY - 10, labelWidth, 20);
    ctx.fillStyle = 'rgba(255, 240, 190, 0.96)';
    ctx.fillText(label, x, labelY);
    ctx.fillStyle = 'rgba(255, 209, 102, 0.95)';
    ctx.beginPath();
    ctx.arc(x, y, 5, 0, 2*Math.PI);
    ctx.fill();
  }
  ctx.restore();

  const labelOffsets = [-14, 10, -24, 6];
  const labelStride = Math.max(1, Math.ceil(pointsToDraw.length * 54 / Math.max(plotWidth, 1)));
  ctx.font = '12px "Inter", "Segoe UI", system-ui, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillStyle = 'rgba(235, 242, 255, 0.86)';
  for(let i=0;i<pointsToDraw.length;i++){
    if(i % labelStride !== 0 && i !== pointsToDraw.length - 1) continue;
    const {x, y} = graphPointPosition(metrics, i);
    const offset = labelOffsets[i % labelOffsets.length];
    const labelY = Math.min(height - 6, Math.max(12, y + offset));
    const label = formatGraphNumber(pointsToDraw[i].value);
    ctx.fillText(label, x, labelY);
  }
  graphDataCache.set(canvas, metrics);
  populateGraphTable(canvas.dataset.tableId, pointsToDraw);
}

function hideGraphTooltip(){
  graphTooltip.classList.add('hidden');
}

function showGraphTooltip(canvas, evt){
  if(!canvas || !graphDataCache.has(canvas)) return hideGraphTooltip();
  const metrics = graphDataCache.get(canvas);
  const points = metrics.points;
  if(!points || !points.length) return hideGraphTooltip();

  const rect = canvas.getBoundingClientRect();
  const relX = evt.clientX - rect.left;
  const scaleX = (canvas.width || rect.width) / rect.width;
  const scaleY = (canvas.height || rect.height) / rect.height;
  const canvasX = relX * scaleX;
  const step = points.length > 1 ? (metrics.plotWidth / (points.length - 1)) : metrics.plotWidth;
  let index = points.length > 1 ? Math.round((canvasX - metrics.plotLeft) / (step || 1)) : 0;
  if(index < 0) index = 0;
  if(index >= points.length) index = points.length - 1;
  const point = points[index];
  const {x, y} = graphPointPosition(metrics, index);

  const eventCount = Number(point.events || 0);
  const eventText = eventCount > 0 ? ` | ${graphEventName(canvas)} x${eventCount}` : '';
  graphTooltip.textContent = `${point.time}: ${formatGraphHeadline(canvas, point.value)}${eventText}`;
  graphTooltip.style.left = `${rect.left + (x / scaleX)}px`;
  graphTooltip.style.top = `${rect.top + (y / scaleY)}px`;
  graphTooltip.classList.remove('hidden');
}


function fetchCustomGraph(canvas){
  if(!canvas) return;
  const series = canvas.dataset.series || canvas.id;
  fetch('/graphData?series='+encodeURIComponent(series))
    .then(r=>r.json())
    .then(j=>drawCustomGraph(canvas,j));
}

function restartCustomGraphInterval(canvas){
  if(!canvas) return;
  if(customGraphTimers.has(canvas)) clearInterval(customGraphTimers.get(canvas));
  const interval = parseInt(canvas.dataset.updateInterval);
  const delay = (!isNaN(interval) && interval >= 5000) ? interval : 5000;
  const timer = setInterval(()=>fetchCustomGraph(canvas), delay);
  customGraphTimers.set(canvas, timer);
}

resizeCustomGraphs();
customGraphCanvases.forEach(canvas=>{
  fetchCustomGraph(canvas);
  restartCustomGraphInterval(canvas);
});


document.querySelectorAll('.graph-update-interval').forEach(select=>{
  const graphId = select.dataset.graph;
  const canvas = document.getElementById('graph_'+graphId);
  const series = canvas ? (canvas.dataset.series || graphId) : graphId;
  if(canvas && canvas.dataset.updateInterval) select.value = canvas.dataset.updateInterval;
  select.addEventListener('change', ()=>{
    let value = parseInt(select.value);
    if(isNaN(value) || value < 5000) value = 5000;
    select.value = value;
    const target = document.getElementById('graph_'+graphId);
    if(!target) return;
    target.dataset.updateInterval = value;
    // fetch('/save?key=graphUpdateInterval_'+graphId+'&val='+encodeURIComponent(value));
    fetch('/save?key=graphUpdateInterval_'+graphId+'&series='+encodeURIComponent(series)+'&val='+encodeURIComponent(value));
    fetchCustomGraph(target);
    restartCustomGraphInterval(target);
  });
});

document.querySelectorAll('.graph-max-points').forEach(input=>{
  const graphId = input.dataset.graph;
  const canvas = document.getElementById('graph_'+graphId);
  const series = canvas ? (canvas.dataset.series || graphId) : graphId;
  if(canvas && canvas.dataset.maxPoints) input.value = canvas.dataset.maxPoints;
  input.addEventListener('change', ()=>{
    let value = parseInt(input.value);
    if(isNaN(value) || value < 1) value = 1;
    const max = parseInt(input.max) || 100;
    if(value > max) value = max;
    input.value = value;
    const target = document.getElementById('graph_'+graphId);
    if(!target) return;
    target.dataset.maxPoints = value;
    // fetch('/save?key=graphMaxPoints_'+graphId+'&val='+encodeURIComponent(value));
    fetch('/save?key=graphMaxPoints_'+graphId+'&series='+encodeURIComponent(series)+'&val='+encodeURIComponent(value));
    fetchCustomGraph(target);
  });
});

window.addEventListener('resize', ()=>{
  resizeCustomGraphs();
  customGraphCanvases.forEach(canvas=>{
    fetchCustomGraph(canvas);
  });
});

    let pageDateTimeBase = null;
    let lastFilterImageState = null;
    let lastBasinImage = null;
    let pageDateTimeLastSync = 0;
    let liveInFlight = false;

    const russianWeekdays = ['Воскресенье','Понедельник','Вторник','Среда','Четверг','Пятница','Суббота']; // Русские названия дней недели для шапки страниц.

    function formatDateTime(dateObj){
      const pad = (num)=>String(num).padStart(2, '0');
      return `${pad(dateObj.getDate())}.${pad(dateObj.getMonth()+1)}.${dateObj.getFullYear()} ` +
             `${pad(dateObj.getHours())}:${pad(dateObj.getMinutes())}:${pad(dateObj.getSeconds())} ${russianWeekdays[dateObj.getDay()]}`;
    }

    function parseDeviceDateTime(value){
      const match = /^(\d{2})\.(\d{2})\.(\d{4})\s+(\d{2}):(\d{2}):(\d{2})(?:\s+\S+)?$/.exec(String(value || '').trim()); // Принимаем время как с днем недели, так и без него.
      if(!match) return null;
      const [, dd, mm, yyyy, hh, min, ss] = match;
      return new Date(Number(yyyy), Number(mm) - 1, Number(dd), Number(hh), Number(min), Number(ss));
    }

    function renderPageDateTime(dateObj){
      const text = formatDateTime(dateObj);
      document.querySelectorAll('.page-datetime').forEach(el=>{
        el.innerText = text;
      });
    }


    function updatePageDateTime(value){
          const parsed = parseDeviceDateTime(value);
      if(parsed){
        const nowMs = Date.now();
        if(pageDateTimeBase){
          const expectedMs = pageDateTimeBase.getTime() + Math.floor((nowMs - pageDateTimeLastSync) / 1000) * 1000;
          if(Math.abs(parsed.getTime() - expectedMs) <= 1100){
            return;
          }
        }
        pageDateTimeBase = parsed;
        pageDateTimeLastSync = nowMs;
        renderPageDateTime(parsed);
        return;
      }
      document.querySelectorAll('.page-datetime').forEach(el=>{
        el.innerText = value;
      });
    }

    function tickPageDateTime(){
      if(!pageDateTimeBase) return;
      const elapsedSeconds = Math.floor((Date.now() - pageDateTimeLastSync) / 1000);
      const current = new Date(pageDateTimeBase.getTime() + elapsedSeconds * 1000);
      renderPageDateTime(current);
    }


    function fetchLive(){
    if(liveInFlight) return;
    liveInFlight = true;
    fetch('/live').then(r=>r.json()).then(j=>{
        markEspLiveOk();
        if(typeof j.CurrentTime !== 'undefined') updatePageDateTime(j.CurrentTime);
        if(typeof j.CurrentTime !== 'undefined') syncManualTimeInputs(j.CurrentTime);
        if(typeof j.gmtOffset !== 'undefined') updateSelectValue('gmtOffset', j.gmtOffset);
    if(document.getElementById('RandomVal')) document.getElementById('RandomVal').innerText=j.RandomVal;
    if(document.getElementById('InfoString')) document.getElementById('InfoString').innerText=j.InfoString;
    if(document.getElementById('InfoString1')) document.getElementById('InfoString1').innerText=j.InfoString1;
    if(document.getElementById('InfoString2')) document.getElementById('InfoString2').innerText=j.InfoString2;
        if(document.getElementById('InfoStringDIN')) document.getElementById('InfoStringDIN').innerText=j.InfoStringDIN;
       
    if(document.getElementById('OverlayPoolTemp')) document.getElementById('OverlayPoolTemp').innerText=j.OverlayPoolTemp;
    if(document.getElementById('OverlayHeaterTemp')) document.getElementById('OverlayHeaterTemp').innerText=j.OverlayHeaterTemp;
    if(document.getElementById('OverlayLevelUpper')) document.getElementById('OverlayLevelUpper').innerText=j.OverlayLevelUpper;
    if(document.getElementById('OverlayLevelLower')) document.getElementById('OverlayLevelLower').innerText=j.OverlayLevelLower;
    if(document.getElementById('OverlayPh')) document.getElementById('OverlayPh').innerText=j.OverlayPh;
    if(document.getElementById('OverlayChlorine')) document.getElementById('OverlayChlorine').innerText=j.OverlayChlorine;
    if(document.getElementById('OverlayFilterState')) document.getElementById('OverlayFilterState').innerText=j.OverlayFilterState;
    if(document.getElementById('OverlayDrainPit')) document.getElementById('OverlayDrainPit').innerText=j.OverlayDrainPit;
    if(document.getElementById('OverlayLight')) document.getElementById('OverlayLight').innerText=j.OverlayLight;
    if(typeof j.BasinImage !== 'undefined' && j.BasinImage !== lastBasinImage){
      lastBasinImage = j.BasinImage;
      const basinImage = document.getElementById('Image1');
      if(basinImage) basinImage.src = j.BasinImage;
    }
    syncDashButton('button1', j.button1);
    syncDashButton('button2', j.button2);
    syncDashButton('button_Lamp', j.button_Lamp);
    syncDashButton('button_WS2815', j.button_WS2815);
    syncDashButton('Pow_Ul_light', j.Pow_Ul_light);
    syncDashButton('Power_H2O2_Button', j.Power_H2O2_Button);
    syncDashButton('Power_ACO_Button', j.Power_ACO_Button);
    syncDashButton('Power_Topping', j.Power_Topping);
        syncDashButton('Power_Drain', j.Power_Drain);
    syncDashButton('AirPump', j.AirPump);
    syncDashButton('SolValveFilBack', j.SolValveFilBack);
    syncDashButton('SolValveFiltration', j.SolValveFiltration);
    syncDashButton('SolSandDump', j.SolSandDump);
    if(typeof j.MotorSpeed !== 'undefined') updateSliderDisplay('MotorSpeed', j.MotorSpeed);
    if(typeof j.RangeMin !== 'undefined' && typeof j.RangeMax !== 'undefined') setRangeSliderUI('RangeSlider', j.RangeMin, j.RangeMax);
    if(typeof j.LEDColor !== 'undefined') updateInputValue('LEDColor', j.LEDColor);
    if(typeof j.LedPattern !== 'undefined') updateSelectValue('LedPattern', j.LedPattern);
    if(typeof j.LedColorMode !== 'undefined') updateSelectValue('LedColorMode', j.LedColorMode);
    if(typeof j.LedBrightness !== 'undefined') updateSliderDisplay('LedBrightness', j.LedBrightness);
    if(typeof j.LedColorOrder !== 'undefined') updateSelectValue('LedColorOrder', j.LedColorOrder);
    if(typeof j.LedAutoplay !== 'undefined') updateSelectValue('LedAutoplay', j.LedAutoplay);
    if(typeof j.LedAutoplayDuration !== 'undefined') updateSliderDisplay('LedAutoplayDuration', j.LedAutoplayDuration);
    if(typeof j.ModeSelect !== 'undefined') updateSelectValue('ModeSelect', j.ModeSelect);
    if(typeof j.ThemeColor !== 'undefined') updateInputValue('ThemeColor', j.ThemeColor); 
    if(typeof j.SetLamp !== 'undefined') updateSelectValue('SetLamp', j.SetLamp);
       if(typeof j.SetRGB !== 'undefined') updateSelectValue('SetRGB', j.SetRGB);
        if(typeof j.DaysSelect !== 'undefined') updateDaysSelection('DaysSelect', j.DaysSelect);
    if(typeof j.IntInput !== 'undefined') updateInputValue('IntInput', j.IntInput);
    if(typeof j.FloatInput !== 'undefined') updateInputValue('FloatInput', j.FloatInput);

    if(typeof j.Timer1 !== 'undefined') updateInputValue('Timer1', j.Timer1);
    if(typeof j.Power_Time1 !== 'undefined') updateCheckboxValue('Power_Time1', j.Power_Time1);
    if(typeof j.Ul_light_Time !== 'undefined') updateCheckboxValue('Ul_light_Time', j.Ul_light_Time);
    if(typeof j.WS2815_Time1 !== 'undefined') updateCheckboxValue('WS2815_Time1', j.WS2815_Time1);
 if(typeof j.Activation_Water_Level !== 'undefined') updateCheckboxValue('Activation_Water_Level', j.Activation_Water_Level);
        if(typeof j.Power_Filtr !== 'undefined') syncDashButton('Power_Filtr', j.Power_Filtr);
    if(typeof j.Filtr_Time1 !== 'undefined') updateCheckboxValue('Filtr_Time1', j.Filtr_Time1);
    if(typeof j.Filtr_Time2 !== 'undefined') updateCheckboxValue('Filtr_Time2', j.Filtr_Time2);
    if(typeof j.Filtr_Time3 !== 'undefined') updateCheckboxValue('Filtr_Time3', j.Filtr_Time3);

    if(typeof j.Power_Clean !== 'undefined') syncDashButton('Power_Clean', j.Power_Clean);
    if(typeof j.Clean_Time1 !== 'undefined') updateCheckboxValue('Clean_Time1', j.Clean_Time1);
    if(Array.isArray(timerIds)){
      timerIds.forEach(id=>{
        const onKey = id + "_ON";
        const offKey = id + "_OFF";
        if(typeof j[onKey] !== 'undefined') updateInputValue(onKey, j[onKey]);
        if(typeof j[offKey] !== 'undefined') updateInputValue(offKey, j[offKey]);
      });
    }

    if(typeof j.Comment !== 'undefined') updateInputValue('Comment', j.Comment);
    if(typeof j.Lumen_Ul !== 'undefined') updateStat('Lumen_Ul', j.Lumen_Ul);
      if(typeof j.DS1 !== 'undefined') updateStat('DS1', j.DS1);
          if(typeof j.RoomTemp !== 'undefined') updateStat('RoomTemp', j.RoomTemp);
    // if(typeof j.Sider_heat !== 'undefined') updateInputValue('Sider_heat', j.Sider_heat);
        if(typeof j.Sider_heat !== 'undefined') updateSliderDisplay('Sider_heat', j.Sider_heat);
    if(typeof j.Activation_Heat !== 'undefined') updateCheckboxValue('Activation_Heat', j.Activation_Heat);
    if(typeof j.Power_Heat !== 'undefined') updateStat('Power_Heat', j.Power_Heat);
        if(typeof j.RoomTempOn !== 'undefined' && typeof j.RoomTempOff !== 'undefined') setRangeSliderUI('RoomTempRange', j.RoomTempOn, j.RoomTempOff);
    if(typeof j.RoomTemper !== 'undefined') updateCheckboxValue('RoomTemper', j.RoomTemper);
    if(typeof j.Power_Warm_floor_heating !== 'undefined') updateStat('Power_Warm_floor_heating', j.Power_Warm_floor_heating);
    if(typeof j.WaterLevelSensorUpper !== 'undefined') updateStat('WaterLevelSensorUpper', j.WaterLevelSensorUpper);
    if(typeof j.WaterLevelSensorLower !== 'undefined') updateStat('WaterLevelSensorLower', j.WaterLevelSensorLower);
        if(typeof j.WaterLevelSensorDrain !== 'undefined') updateStat('WaterLevelSensorDrain', j.WaterLevelSensorDrain);
    if(typeof j.Power_Topping_State !== 'undefined') updateStat('Power_Topping_State', j.Power_Topping_State);
        if(typeof j.Power_Drain_State !== 'undefined') updateStat('Power_Drain_State', j.Power_Drain_State);
    if(typeof j.PoolWaterLevelStageInfo !== 'undefined') updateStat('PoolWaterLevelStageInfo', j.PoolWaterLevelStageInfo);
    if(typeof j.DrainPitStageInfo !== 'undefined') updateStat('DrainPitStageInfo', j.DrainPitStageInfo);
    if(typeof j.PH !== 'undefined') updateStat('PH', j.PH);
    if(typeof j.analogValuePH !== 'undefined') updateStat('analogValuePH', j.analogValuePH);
    if(typeof j.PH_Control_ACO !== 'undefined') updateCheckboxValue('PH_Control_ACO', j.PH_Control_ACO);
        if(typeof j.PH_Lower !== 'undefined') updateInputValue('PH_Lower', j.PH_Lower);
    if(typeof j.PH_setting !== 'undefined') updateInputValue('PH_setting', j.PH_setting);
        if(typeof j.PH_Upper !== 'undefined') updateInputValue('PH_Upper', j.PH_Upper);
    if(typeof j.ACO_Work !== 'undefined') updateSelectValue('ACO_Work', j.ACO_Work);
    if(typeof j.Power_ACO !== 'undefined') updateStat('Power_ACO', j.Power_ACO);
    if(typeof j.ppmCl !== 'undefined') updateStat('ppmCl', j.ppmCl);
    if(typeof j.corrected_ORP_Eh_mV !== 'undefined') updateStat('corrected_ORP_Eh_mV', j.corrected_ORP_Eh_mV);
    if(typeof j.NaOCl_H2O2_Control !== 'undefined') updateCheckboxValue('NaOCl_H2O2_Control', j.NaOCl_H2O2_Control);
        if(typeof j.CL_Lower !== 'undefined') updateInputValue('CL_Lower', j.CL_Lower);
    if(typeof j.CL_Upper !== 'undefined') updateInputValue('CL_Upper', j.CL_Upper);
    if(typeof j.ORP_setting !== 'undefined') updateInputValue('ORP_setting', j.ORP_setting);
    if(typeof j.H2O2_Work !== 'undefined') updateSelectValue('H2O2_Work', j.H2O2_Work);
    if(typeof j.Power_H2O2 !== 'undefined') updateStat('Power_H2O2', j.Power_H2O2);
    if(typeof j.Temper_Reference !== 'undefined') updateInputValue('Temper_Reference', j.Temper_Reference);
    customGraphCanvases.forEach(canvas=>{
      const key = canvas.dataset.valueName;
      if(!key || typeof j[key] === 'undefined') return;
      canvas.dataset.liveValue = j[key];
      const cached = graphDataCache.get(canvas);
      if(cached && cached.points) drawCustomGraph(canvas, cached.points);
    });
    syncRs485Panel(j);
    
        if(typeof j.FilterImageState !== 'undefined' && j.FilterImageState !== lastFilterImageState){
      lastFilterImageState = j.FilterImageState;
      const filterImage = document.getElementById('FilterImage');
      if(filterImage){
        filterImage.src = (j.FilterImageState === 1) ? '/anim1.gif' : '/img2.jpg';
      }
    }
    
    Object.keys(j).forEach(key=>applyLiveValue(key, j[key]));

    }).catch(()=>{}).finally(()=>{ liveInFlight = false; });
  }
setInterval(fetchLive, 2000); // Фоновый live-опрос раз в 2 секунды снижает нагрузку на ESP32 и ускоряет Web UI.
setInterval(tickPageDateTime, 1000);
setInterval(checkEspLiveTimeout, 500);
checkEspLiveTimeout();
fetchMqttConfig();

function setImg(x){
  fetch('/setjpg?val='+x).then(resp=>{
    document.querySelectorAll('img[data-refresh="getImage"]').forEach(img=>{
      img.src = '/getImage?ts=' + Date.now();
    });
  });
}
</script></body></html>
)rawliteral";

 const uint32_t htmlBuildMs = millis() - requestStartMs; // Полное время сборки HTML до начала отправки
      if(kWebVerboseSerial) Serial.printf("[WEB:/] html-built | bytes=%u reserve=%u buildMs=%u\n", static_cast<unsigned>(html.length()), static_cast<unsigned>(htmlReserveBytes), static_cast<unsigned>(htmlBuildMs)); // Контроль итогового размера только в debug-режиме.
      logWebHeapStats("after-html-build-before-send"); // Проверяем heap после сборки и до сетевой отправки

      if(kWebVerboseSerial) Serial.println("[WEB:/] chunked-send begin"); // Лог chunked-send выключен в обычной работе.
      const uint32_t sendStartMs = millis(); // Отсчёт длительности этапа постановки chunked-отправки
      auto htmlPtr = std::make_shared<String>(std::move(html)); // Буфер живёт до конца chunk-отправки и не освобождается преждевременно
      const size_t htmlTotalLen = htmlPtr->length(); // Полная длина ответа для корректной нарезки чанков
      AsyncWebServerResponse *response = r->beginChunkedResponse("text/html; charset=UTF-8", // Chunked режим устраняет единичную тяжёлую отправку 120KB+
        [htmlPtr, htmlTotalLen, sendStartMs](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
          if(index == 0){ // Первый вызов: логируем параметры чанков, чтобы видеть реальный maxLen
            if(kWebVerboseSerial) Serial.printf("[WEB:/][chunk] start total=%u maxChunk=%u\n", // Диагностика реального размера порции только в debug-режиме.
                          static_cast<unsigned>(htmlTotalLen),
                          static_cast<unsigned>(maxLen));
          }
          if(index >= htmlTotalLen){ // Достигнут конец буфера: штатно завершаем передачу
            if(kWebVerboseSerial) Serial.printf("[WEB:/][chunk] done total=%u sendMs=%u\n", // Финальный маркер только в debug-режиме.
                          static_cast<unsigned>(htmlTotalLen),
                          static_cast<unsigned>(millis() - sendStartMs));
            return 0; // Нормальное завершение chunked-ответа после передачи всего буфера
          }
          if(maxLen == 0){ // Важный фикс: 0 здесь не конец, просим стек повторить вызов позже
            return RESPONSE_TRY_AGAIN; // Предотвращает обрыв страницы, который был виден на скриншоте
          }
          const size_t remaining = htmlTotalLen - index; // Сколько байт ещё осталось отправить клиенту
          const size_t toCopy = remaining < maxLen ? remaining : maxLen; // Отдаём только доступный кусок без выхода за границы буфера
          memcpy(buffer, htmlPtr->c_str() + index, toCopy); // Копируем текущий кусок в буфер сети без доп.алокаций
          return toCopy; // Сообщаем стеку размер готового чанка
        }
      );
      r->send(response); // Запускаем асинхронную отправку chunked-ответа клиенту
      stageSendMs = millis() - sendStartMs; // Время очереди/старта отправки chunked-ответа
      if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] sendQueued=%u ms\n", static_cast<unsigned>(stageSendMs)); // Лог сетевого этапа только в debug-режиме.
      const uint32_t slowestMs = max(max(stageBuildHeaderMs, stageRenderTabsMs), max(stageRenderPopupsMs, stageSendMs)); // Вычисляем узкое место текущего запроса
      const char *slowestStage = (slowestMs == stageBuildHeaderMs) ? "buildHeader" : // Имя самого долгого этапа для быстрого анализа
                                 (slowestMs == stageRenderTabsMs) ? "renderTabs" :
                                 (slowestMs == stageRenderPopupsMs) ? "renderPopups" : "send";
      if(kWebVerboseSerial) Serial.printf("[WEB:/][timing] slowestStage=%s %u ms\n", slowestStage, static_cast<unsigned>(slowestMs)); // Итоговый лог только в debug-режиме.
      if(kWebVerboseSerial) Serial.printf("[WEB:/] send queued | totalMs=%u\n", static_cast<unsigned>(millis() - requestStartMs)); // Полная длительность только в debug-режиме.
      logWebHeapStats("after-send-queued"); // Контроль heap после постановки ответа в отправку
    });

        server.on("/popup/auth", HTTP_GET, [](AsyncWebServerRequest *r){
      if(!ensureAdminAuthorized(r)) return;
      r->send(200, "text/plain", "OK");
    });


    // ---------------- SAVE ----------------
       server.on("/save", HTTP_GET, [self](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      if(r->hasParam("key") && r->hasParam("val")){
        String key = r->getParam("key")->value();
        String valStr = r->getParam("val")->value();
                if(ui.updateTimerField(key, valStr)){
          r->send(200,"text/plain","OK");
          return;
        }
        if(applyWaterControlRequest(key, valStr.toInt() != 0)){ // Для уровня воды сначала применяем взаимные блокировки и сохраняем итог.
          r->send(200,"text/plain","OK"); // Возвращаем успешный ответ после безопасного применения.
          return; // Не отдаем эти опасные поля в общий обработчик без блокировок.
        }
        if(applyChemistrySettingRequest(key, valStr)){ // Пределы pH/CL нормализуем и сохраняем сразу.
          r->send(200,"text/plain","OK"); // Возвращаем успешный ответ после безопасного применения.
          return; // Не отдаем эти пределы в общий обработчик без проверки диапазонов.
        }
        if(applyRs485ConfigRequest(key, valStr)){
          r->send(200,"text/plain","OK");
          return;
        }
        if(uiApplyValueForId(key, valStr)){
          r->send(200,"text/plain","OK");
          return;
        }

      //Вывод информации из EEPROM  на WEB страницу
        if(key=="ACO_Work"){
          ACO_Work = sanitizeDosingPeriodValue(valStr.toInt()); // Web тоже сохраняет только разрешенный код периода.
          holdNextionDispensersReads(); // После Web-изменения не даем старому значению Nextion сразу перетереть ESP32.
          saveValue<int>("ACO_Work", ACO_Work);
          r->send(200,"text/plain","OK");
          return;
        }
        if(key=="H2O2_Work"){
          H2O2_Work = sanitizeDosingPeriodValue(valStr.toInt()); // Web тоже сохраняет только разрешенный код периода.
          holdNextionDispensersReads(); // После Web-изменения не даем старому значению Nextion сразу перетереть ESP32.
          saveValue<int>("H2O2_Work", H2O2_Work);
          r->send(200,"text/plain","OK");
          return;
        }

           if(key=="ThemeColor") { ThemeColor = valStr; saveValue<String>(key.c_str(), valStr); }
          else if(key=="gmtOffset") {
          applyGmtOffsetFromEsp(valStr.toInt(), true); // Веб-изменение GMT сразу сохраняем в ESP, сдвигаем локальное время и отправляем в Nextion.
          syncNextionRtcFromEpoch(getCurrentEpoch()); // После сдвига GMT сразу обновляем RTC Nextion новым локальным временем.
        }
                else if(key=="graphMainMaxPoints") {
          int valInt = valStr.toInt();
          if(valInt < minGraphPoints) valInt = minGraphPoints;
          if(valInt > maxGraphPoints) valInt = maxGraphPoints;
          maxPoints = valInt;
          trimGraphPoints(graphPoints, maxPoints);
          saveGraphSeries("main", graphPoints);
          saveGraphSettings("main", GraphSettings{updateInterval, maxPoints});
          seriesConfig["main"].maxPoints = maxPoints;
        }
        else if(key=="graphMainUpdateInterval") {
          int valInt = valStr.toInt();
          if(valInt < minGraphUpdateInterval) valInt = minGraphUpdateInterval;
          if(valInt > maxGraphUpdateInterval) valInt = maxGraphUpdateInterval;
          updateInterval = valInt;
          saveGraphSettings("main", GraphSettings{updateInterval, maxPoints});
          seriesConfig["main"].updateInterval = updateInterval;
          seriesLastUpdate["main"] = 0;
        }
        else if(key.startsWith("graphUpdateInterval_")) {
          String series = key.substring(String("graphUpdateInterval_").length());
          if(r->hasParam("series")) series = r->getParam("series")->value();
          int valInt = valStr.toInt();
          if(valInt < minGraphUpdateInterval) valInt = minGraphUpdateInterval;
          if(valInt > maxGraphUpdateInterval) valInt = maxGraphUpdateInterval;
          int preservedMaxPoints = maxGraphPoints; // При сохранении только интервала не сбрасываем Max Points до глобальных 30.
          auto runtimeCfg = seriesConfig.find(series);
          if(runtimeCfg != seriesConfig.end() && runtimeCfg->second.maxPoints > 0) preservedMaxPoints = runtimeCfg->second.maxPoints;
          GraphSettings cfg{static_cast<unsigned long>(valInt), preservedMaxPoints};
          loadGraphSettings(series, cfg);
          cfg.updateInterval = valInt;
          if(cfg.maxPoints < minGraphPoints) cfg.maxPoints = preservedMaxPoints;
          if(cfg.maxPoints > maxGraphPoints) cfg.maxPoints = maxGraphPoints;
          saveGraphSettings(series, cfg);
          seriesConfig[series].updateInterval = cfg.updateInterval;
          seriesConfig[series].maxPoints = cfg.maxPoints;
          seriesLastUpdate[series] = 0;
        }
        else if(key.startsWith("graphMaxPoints_")) {
          String series = key.substring(String("graphMaxPoints_").length());
          if(r->hasParam("series")) series = r->getParam("series")->value();
          int valInt = valStr.toInt();
          if(valInt < minGraphPoints) valInt = minGraphPoints;
          if(valInt > maxGraphPoints) valInt = maxGraphPoints;
          GraphSettings cfg{updateInterval, valInt};
          loadGraphSettings(series, cfg);
          cfg.maxPoints = valInt;
          saveGraphSettings(series, cfg);
          seriesConfig[series].maxPoints = cfg.maxPoints;
          if(seriesConfig[series].updateInterval == 0) seriesConfig[series].updateInterval = cfg.updateInterval;
          auto it = customGraphSeries.find(series);
          if(it != customGraphSeries.end()){
            trimGraphPoints(it->second, valInt);
            saveGraphSeries(it->first, it->second);
          }
        }
        else if(key=="ssid") saveValue<String>(key.c_str(), valStr);
        else if(key=="pass") saveValue<String>(key.c_str(), valStr);
        else if(key=="ssid2") saveValue<String>(key.c_str(), valStr);
        else if(key=="pass2") saveValue<String>(key.c_str(), valStr);
        
        else if(key=="apSSID") { StoredAPSSID = valStr; saveValue<String>(key.c_str(), valStr); }
        else if(key=="apPASS") { StoredAPPASS = valStr; saveValue<String>(key.c_str(), valStr); }
      }
      r->send(200,"text/plain","OK");
    });

    server.on("/time/set", HTTP_POST, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      auto paramOr = [&](const char* name)->String{
        if(r->hasParam(name, true)) return r->getParam(name, true)->value();
        if(r->hasParam(name)) return r->getParam(name)->value();
        return String();
      };
      String dateStr = paramOr("date");
      String timeStr = paramOr("time");
      String offsetStr = paramOr("gmtOffset");
      if(offsetStr.length()){
        applyGmtOffsetFromEsp(offsetStr.toInt(), false); // При ручной установке даты/времени GMT пишем в Nextion без отдельного сдвига времени.
      }
      if(dateStr.length() < 8 || timeStr.length() < 4){
        r->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Некорректная дата/время\"}");
        return;
      }
      int year = 0;
      int month = 0;
      int day = 0;
      auto parseNumberParts = [&](const String &value, int *parts, int maxParts){
        int count = 0;
        String current;
        for(size_t i = 0; i < value.length(); ++i){
          char c = value.charAt(i);
          if(c >= '0' && c <= '9'){
            current += c;
          } else if(current.length()){
            parts[count++] = current.toInt();
            current = "";
            if(count >= maxParts) break;
          }
        }
        if(current.length() && count < maxParts){
          parts[count++] = current.toInt();
        }
        return count;
      };
      auto parseDate = [&](const String &value){
        int parts[3] = {0, 0, 0};
        int count = parseNumberParts(value, parts, 3);
        if(count != 3) return;
        if(parts[0] >= 1000){
          year = parts[0];
          month = parts[1];
          day = parts[2];
        } else if(parts[2] >= 1000){
          day = parts[0];
          month = parts[1];
          year = parts[2];
        }
      };
      parseDate(dateStr);

      int hour = 0;
      int minute = 0;
      int second = 0;

      auto parseTime = [&](const String &value){
        int parts[3] = {0, 0, 0};
        int count = parseNumberParts(value, parts, 3);
        if(count < 2) return;
        hour = parts[0];
        minute = parts[1];
        if(count >= 3) second = parts[2];
      };
      parseTime(timeStr);

      if(!isValidDateTime(year, month, day, hour, minute, second)){
        r->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Некорректная дата/время\"}");
        return;
      }
      time_t epoch = buildEpoch(year, month, day, hour, minute, second);
      if(epoch <= 0){
        r->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Не удалось вычислить время\"}");
        return;
      }
      setBaseEpoch(epoch);
      syncNextionRtcFromEpoch(epoch);
      String payloadJson = "{\"status\":\"ok\",\"current\":\"" + jsonEscape(getCurrentDateTime()) + "\"}";
      r->send(200, "application/json", payloadJson);
    });


    server.on("/mqtt/config", HTTP_GET, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      String json = "{\"host\":\""+jsonEscape(mqttHost)+"\",";
      json += "\"port\":" + String(mqttPort) + ",";
      json += "\"user\":\""+jsonEscape(mqttUsername)+"\",";
      json += "\"pass\":\""+jsonEscape(mqttPassword)+"\",";
      json += "\"baseTopic\":\""+jsonEscape(mqttBaseTopic)+"\",";
      json += "\"enabled\":" + String(mqttEnabled ? 1 : 0) + ",";
      json += "\"connected\":" + String((mqttEnabled && mqttClient.connected()) ? 1 : 0) + "}";
      r->send(200, "application/json", json);
    });

    server.on("/mqtt/save", HTTP_POST, [](AsyncWebServerRequest *r){
       if(!ensureAuthorized(r)) return;
      auto paramOr = [&](const char* name, const String &fallback)->String{
        if(r->hasParam(name, true)) return r->getParam(name, true)->value();
        if(r->hasParam(name)) return r->getParam(name)->value();
        return fallback;
      };

      mqttHost = paramOr("host", mqttHost);
      int requestedMqttPort = paramOr("port", String(mqttPort)).toInt();
      if(requestedMqttPort <= 0) requestedMqttPort = 1883;
      if(requestedMqttPort > 65535) requestedMqttPort = 65535;
      mqttPort = static_cast<uint16_t>(requestedMqttPort);
      mqttUsername = paramOr("user", mqttUsername);
      mqttPassword = paramOr("pass", mqttPassword);
      mqttApplyBaseTopic(paramOr("baseTopic", mqttBaseTopic));
      mqttEnabled = mqttHost.length() > 0 && (paramOr("enabled", mqttHost.length() ? "1" : "0").toInt() == 1);

     
      saveMqttSettings();
      applyMqttState();
      r->send(200, "application/json", "{\"status\":\"saved\"}");
    });


    server.on("/mqtt/activate", HTTP_POST, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      bool enabled = mqttEnabled;
      if(r->hasParam("enabled", true)) enabled = r->getParam("enabled", true)->value().toInt() == 1;
      else if(r->hasParam("enabled")) enabled = r->getParam("enabled")->value().toInt() == 1;
      mqttEnabled = mqttHost.length() > 0 && enabled;
      saveMqttSettings();
      applyMqttState();
      r->send(200, "application/json", "{\"status\":\"updated\"}");
    });

    server.on("/profile/save", HTTP_POST, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      auto paramOr = [&](const char* name, const String &fallback)->String{
        if(r->hasParam(name, true)) return r->getParam(name, true)->value();
        if(r->hasParam(name)) return r->getParam(name)->value();
        return fallback;
      };
      authUsername = paramOr("user", authUsername);
      authPassword = paramOr("pass", authPassword);
      adminUsername = paramOr("adminUser", adminUsername);
      adminPassword = paramOr("adminPass", adminPassword);
      saveValue<String>("authUser", authUsername);
      saveValue<String>("authPass", authPassword);
      saveValue<String>("adminUser", adminUsername);
      saveValue<String>("adminPass", adminPassword);
      r->send(200, "application/json", "{\\\"status\\\":\\\"saved\\\"}");
    });

    server.on("/button", HTTP_GET, [](AsyncWebServerRequest *r){
            if(!ensureAuthorized(r)) return;
      if(r->hasParam("id") && r->hasParam("state")){
        String id = r->getParam("id")->value();
        int state = r->getParam("state")->value().toInt();
       if(applyWaterControlRequest(id, state != 0)){ // Кнопки слива/долива применяем через защитную логику.
          r->send(200, "text/plain", "OK");
          return;
       }
       if(uiApplyValueForId(id, String(state))){
          r->send(200, "text/plain", "OK");
          return;
        }
        r->send(200, "text/plain", "OK");
      } else {
        r->send(400, "text/plain", "Missing params");
      }
    });

    server.on("/rs485/relay", HTTP_GET, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      if(!r->hasParam("idx") || !r->hasParam("state")){
        r->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Missing params\"}");
        return;
      }
      int relay = r->getParam("idx")->value().toInt();
      bool state = r->getParam("state")->value().toInt() != 0;
      if(relay < 0 || relay > 15 || !applyRs485RelayRequest(static_cast<uint8_t>(relay), state)){
        r->send(400, "application/json", "{\"status\":\"error\",\"error\":\"Bad relay\"}");
        return;
      }
      AktualReadRelay = false;
      String json = "{\"status\":\"ok\",\"relay\":" + String(relay) + ",\"state\":" + String(rs485RelayCommandState(static_cast<uint8_t>(relay)) ? 1 : 0) + "}";
      r->send(200, "application/json", json);
    });

    server.on("/rs485/all-off", HTTP_POST, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      applyRs485AllOffRequest();
      AktualReadRelay = false;
      r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/rs485/poll", HTTP_POST, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      Rs485ForcePoll = true;
      r->send(200, "application/json", "{\"status\":\"ok\"}");
    });

    server.on("/live", HTTP_GET, [](AsyncWebServerRequest *r){
          if(!ensureAuthorized(r)) return;
      JsonDocument doc; // ArduinoJson v7 сам расширяет документ: не держим 64 КБ heap на каждый быстрый /live.

      doc["CurrentTime"] = CurrentTime; // Временная метка для синхронизации времени страницы
      doc["gmtOffset"] = gmtOffset_correct; // Часовой пояс (GMT offset)
      doc["FilterImageState"] = jpg; // Выбор картинки бассейна (анимация/статик)
      doc["BasinImage"] = currentBasinImagePath(); // Картинка бассейна по состоянию RGB и лампы
      doc["button_Lamp"] = Lamp ? 1 : 0; // Отображение состояния кнопки лампы, которая не объявлена через UI
      for (const auto &timer : ui.allTimers()) {
        doc[String(timer.id + "_ON")] = formatMinutesToTime(timer.on);
        doc[String(timer.id + "_OFF")] = formatMinutesToTime(timer.off);
      }

      doc["ACO_Work"] = ACO_Work;
      doc["H2O2_Work"] = H2O2_Work;
      doc["Rs485Enabled"] = Rs485Enabled ? 1 : 0;
      doc["Rs485BaudRate"] = Rs485BaudRate;
      doc["Rs485UartMode"] = Rs485UartMode;
      doc["Rs485SlaveId"] = Rs485SlaveId;
      doc["Rs485PollIntervalMs"] = Rs485PollIntervalMs;
      JsonArray rs485Relays = doc["Rs485Relays"].to<JsonArray>();
      JsonArray rs485RelayActual = doc["Rs485RelayActual"].to<JsonArray>();
      JsonArray rs485Inputs = doc["Rs485Inputs"].to<JsonArray>();
      for(uint8_t relay = 0; relay < 16; relay++){
        rs485Relays.add(rs485RelayCommandState(relay) ? 1 : 0);
        rs485RelayActual.add(ReadRelayArray[relay] ? 1 : 0);
      }
      for(uint8_t input = 0; input < 16; input++){
        rs485Inputs.add(ReadInputArray[input] ? 1 : 0);
      }

      doc["Temperatura"] = Temperatura; // Онлайн-значение для верхней подписи графика температуры
      doc["PH"] = PH; // Онлайн-значение для верхней подписи графика pH
      doc["ppmCl"] = ppmCl; // Онлайн-значение для верхней подписи графика хлора
            appendUiRegistryValues(doc);
      doc["PH_Lower"] = PH_Lower; // В live-ответе всегда отдаем актуальный нижний предел pH из рабочей переменной.
      doc["PH_Upper"] = PH_Upper; // В live-ответе всегда отдаем актуальный верхний предел pH из рабочей переменной.
      doc["PH_setting"] = PH_Upper; // Старый ключ Web синхронизируем с верхним пределом pH.
      doc["CL_Lower"] = CL_Lower; // В live-ответе всегда отдаем актуальный нижний предел CL из рабочей переменной.
      doc["CL_Upper"] = CL_Upper; // В live-ответе всегда отдаем актуальный верхний предел CL из рабочей переменной.
      // String s;
      // serializeJson(doc, s);
      // r->send(200, "application/json", s);
      AsyncResponseStream *response = r->beginResponseStream("application/json");
      serializeJson(doc, *response);
      r->send(response);
    });

    server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *r){
            if(!ensureAuthorized(r)) return;
      auto formatUptime = [](){
        unsigned long seconds = millis() / 1000;
        unsigned long hours = seconds / 3600;
        unsigned long minutes = (seconds % 3600) / 60;
        unsigned long secs = seconds % 60;
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", hours, minutes, secs);
        return String(buffer);
      };

      size_t total = SPIFFS.totalBytes();
      size_t used = SPIFFS.usedBytes();
      size_t freeSpace = total > used ? (total - used) : 0;

      String chipModelRevision = buildChipIdentity();
      uint32_t cpuFreq = getCpuFrequencyMhz();
#ifdef ARDUINO_ARCH_ESP32
      float temperatureVal = temperatureRead();
#else
      float temperatureVal = NAN;
#endif
      String temperature = isnan(temperatureVal) ? String("N/A") : String(temperatureVal, 1);
      String mac = WiFi.macAddress();

      WifiStatusInfo wifiInfo = getWifiStatus();
      String wifiMode = wifiInfo.modeText;
      String ssid = (wifiInfo.ssid.length()) ? wifiInfo.ssid : String("N/A");
      String localIp = (wifiInfo.ip.length()) ? wifiInfo.ip : String("N/A");
      String rssi = wifiInfo.rssi != 0 ? String(wifiInfo.rssi) : String("N/A");
      uint32_t freePsramVal = 0;
      uint32_t totalPsramVal = 0;
      bool hasPsram = readPsramStats(freePsramVal, totalPsramVal);
            size_t flashTotal = 0;
      size_t nvsTotal = 0;
      String otaRunningLabel = "";
      size_t otaRunningSize = 0;
      size_t otaSlot0Size = 0;
      size_t otaSlot1Size = 0;
      size_t flashUsed = 0;
      size_t flashFree = 0;
      size_t otaUsed = 0;
      size_t otaFree = 0;
#ifdef ARDUINO_ARCH_ESP32
      flashTotal = ESP.getFlashChipSize();
      flashFree = ESP.getFreeSketchSpace();
      if(flashTotal >= flashFree){
        flashUsed = flashTotal - flashFree;
      }
      if(const esp_partition_t *nvsPartition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, nullptr)){
        nvsTotal = nvsPartition->size;
      }
      if(const esp_partition_t *running = esp_ota_get_running_partition()){
        if(running->label){
          otaRunningLabel = running->label;
        }
        otaRunningSize = running->size;
        otaUsed = ESP.getSketchSize();
        if(otaRunningSize >= otaUsed){
          otaFree = otaRunningSize - otaUsed;
        }
      }
      if(const esp_partition_t *slot0 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr)){
        otaSlot0Size = slot0->size;
      }
      if(const esp_partition_t *slot1 = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, nullptr)){
        otaSlot1Size = slot1->size;
      }
#endif

      String json = "{";
      json += "\"chipModelRevision\":\""+jsonEscape(chipModelRevision)+"\",";
      json += "\"cpuFreqMHz\":"+String(cpuFreq)+",";
      json += "\"temperature\":\""+jsonEscape(temperature)+"\",";
      json += "\"uptime\":\""+jsonEscape(formatUptime())+"\",";
      json += "\"mac\":\""+jsonEscape(mac)+"\",";
      json += "\"freeHeap\":"+String(ESP.getFreeHeap())+",";
      json += "\"freePsram\":";
      json += hasPsram ? String(freePsramVal) : String("null");
      json += ",";
      json += "\"totalPsram\":";
      json += hasPsram ? String(totalPsramVal) : String("null");
      json += ",";
            json += "\"flashTotal\":";
      json += flashTotal ? String(flashTotal) : String("null");
      json += ",";
      json += "\"flashUsed\":";
      json += flashTotal ? String(flashUsed) : String("null");
      json += ",";
      json += "\"flashFree\":";
      json += flashTotal ? String(flashFree) : String("null");
      json += ",";
      json += "\"nvsTotal\":";
      json += nvsTotal ? String(nvsTotal) : String("null");
      json += ",";
      json += "\"otaRunningLabel\":\""+jsonEscape(otaRunningLabel)+"\",";
      json += "\"otaRunningSize\":";
      json += otaRunningSize ? String(otaRunningSize) : String("null");
      json += ",";
      json += "\"otaUsed\":";
      json += otaRunningSize ? String(otaUsed) : String("null");
      json += ",";
      json += "\"otaFree\":";
      json += otaRunningSize ? String(otaFree) : String("null");
      json += ",";
      json += "\"otaSlot0Size\":";
      json += otaSlot0Size ? String(otaSlot0Size) : String("null");
      json += ",";
      json += "\"otaSlot1Size\":";
      json += otaSlot1Size ? String(otaSlot1Size) : String("null");
      json += ",";
      json += "\"spiffsUsed\":"+String(used)+",";
      json += "\"spiffsFree\":"+String(freeSpace)+",";
      json += "\"spiffsTotal\":"+String(total)+",";
      json += "\"wifiMode\":\""+jsonEscape(wifiMode)+"\",";
      json += "\"wifiStatus\":\""+jsonEscape(wifiInfo.statusText)+"\",";
      json += "\"ssid\":\""+jsonEscape(ssid)+"\",";
      json += "\"localIp\":\""+jsonEscape(localIp)+"\",";
      json += "\"rssi\":\""+jsonEscape(rssi)+"\"";
      json += "}";

      r->send(200, "application/json", json);
    });

    server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *r){
            if(!ensureAuthorized(r)) return;
      r->send(200, "application/json", scanWifiNetworksJson());
    });

    server.on("/wifi/save", HTTP_POST, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      auto paramOr = [&](const char* name, const String &fallback)->String{
        if(r->hasParam(name, true)) return r->getParam(name, true)->value();
        if(r->hasParam(name)) return r->getParam(name)->value();
        return fallback;
      };

      String ssid = paramOr("ssid", loadValue<String>("ssid", String(defaultSSID)));
      String pass = paramOr("pass", loadValue<String>("pass", String(defaultPASS)));
      String backupSsid = paramOr("ssid2", loadValue<String>("ssid2", String("")));
      String backupPass = paramOr("pass2", loadValue<String>("pass2", String("")));
      String apSsid = paramOr("ap_ssid", loadValue<String>("apSSID", String(::apSSID)));
      String apPass = paramOr("ap_pass", loadValue<String>("apPASS", String(::apPASS)));
      String host = paramOr("hostname", loadValue<String>("hostname", String(defaultHostname)));

      StoredAPSSID = apSsid;
      StoredAPPASS = apPass;
      saveWifiConfig(ssid, pass, backupSsid, backupPass, apSsid, apPass, host);
      WifiStatusInfo wifiInfo = getWifiStatus();

      String response = "{\\\"saved\\\":1,\\\"connected\\\":" + String(wifiIsConnected() ? 1 : 0) + ",";
      response += "\\\"status\\\":\\\"" + wifiInfo.statusText + "\\\",";
      response += "\\\"ip\\\":\\\"" + (wifiIsConnected() ? WiFi.localIP().toString() : String("")) + "\\\"}";

      r->send(200, "application/json", response);


    });

    server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *r){
            if(!ensureAuthorized(r)) return;
      r->send(200, "text/plain", "Restarting");
      r->client()->stop();
      delay(100);
      ESP.restart();
    });


    server.on("/graphData", HTTP_GET, [](AsyncWebServerRequest *r){
            if(!ensureAuthorized(r)) return;
      String series = "main";
      if(r->hasParam("series")) series = r->getParam("series")->value();
      const vector<GraphPoint> *points = &graphPoints;
      if(series != "main"){
        static const vector<GraphPoint> emptySeries;
        auto it = customGraphSeries.find(series);
        if(it != customGraphSeries.end() && !it->second.empty()) points = &it->second;
        else points = &emptySeries;
      }
      String s="[";
      for(int i=0;i<points->size();i++){
        s+="{\"time\":\""+(*points)[i].time+"\",\"value\":"+String((*points)[i].value);
        if((*points)[i].events > 0) s+=",\"events\":"+String((*points)[i].events); // Передаем счетчик дозирований только там, где он есть.
        s+="}";
        if(i<points->size()-1) s+=",";
      }
      s+="]";
      r->send(200,"application/json",s);
    });

server.on("/getImage", HTTP_GET, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
    String path = (jpg == 1) ? "/anim1.gif" : "/anim2.gif";
    Serial.println("GET IMAGE -> " + path);

    if(!SPIFFS.exists(path)){
        Serial.println("Image not found: " + path);
        r->send(404, "text/plain", "Image not found");
        return;
    }

    File f = SPIFFS.open(path, "r");
    if(!f){
        Serial.println("Failed to open file: " + path);
        r->send(500, "text/plain", "Failed to open file");
        return;
    }

    Serial.printf("Serving file %s, size %u bytes\n", path.c_str(), f.size());
    r->send(SPIFFS, path, "image/gif");
    f.close();
});


    server.on("/setjpg", HTTP_GET, [](AsyncWebServerRequest *r){
            if(!ensureAuthorized(r)) return;
      if(r->hasParam("val")){
        String v = r->getParam("val")->value();
        int newv = v.toInt();
        if(newv != 1 && newv != 2) newv = 1;
        jpg = newv;
        saveValue<int>("jpg", jpg);
        Serial.printf("jpg set to %d\n", jpg);
        r->send(200, "text/plain", "OK");
      } else {
        r->send(400, "text/plain", "Missing val");
      }
    });


    server.serveStatic("/anim1.gif", SPIFFS, "/anim1.gif"); // Отдаёт GIF-анимацию anim1.gif из SPIFFS по HTTP пути /anim1.gif
    server.serveStatic("/Basin.jpg", SPIFFS, "/Basin.jpg"); // Отдаёт изображение Basin.jpg из SPIFFS по HTTP пути /Basin.jpg
    server.serveStatic("/Basin_RGB.jpg", SPIFFS, "/Basin_RGB.jpg"); // Отдаёт изображение Basin_RGB.jpg из SPIFFS.
    server.serveStatic("/Basin_Lamp.jpg", SPIFFS, "/Basin_Lamp.jpg"); // Отдаёт изображение Basin_Lamp.jpg из SPIFFS.
    server.serveStatic("/Basin_RGB_LAMP.jpg", SPIFFS, "/Basin_RGB_LAMP.jpg"); // Отдаёт изображение Basin_RGB_LAMP.jpg из SPIFFS.
    server.serveStatic("/huaqingjun.jpg", SPIFFS, "/huaqingjun.jpg"); // Отдаёт изображение RS485-реле Huaqingjun из SPIFFS.
    server.serveStatic("/img1.jpg", SPIFFS, "/img1.jpg"); // Отдаёт изображение img1.jpg из SPIFFS по HTTP пути /img1.jpg
    server.serveStatic("/img2.jpg", SPIFFS, "/img2.jpg"); // Отдаёт изображение img2.jpg из SPIFFS по HTTP пути /img2.jpg

        server.onNotFound([](AsyncWebServerRequest *r){
      WifiStatusInfo wifiInfo = getWifiStatus();
      bool apMode = wifiInfo.apActive && !wifiIsConnected(); // Captive portal нужен только в автономном AP.
      if(apMode){
        String host = wifiHostName();
        if(!host.length()) host = String(defaultHostname);
        String url = String("http://") + host + String(".local/");
        AsyncWebServerResponse *response = r->beginResponse(302, "text/plain", "Redirecting to ESP32 setup portal");
        response->addHeader("Location", url);
        response->addHeader("Cache-Control", "no-store");
        r->send(response);
        return;
      }
      r->send(404, "text/plain", "Not found");
    });


    server.begin();
  }
} dash;
