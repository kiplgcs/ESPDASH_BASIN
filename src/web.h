// ------------------- web.h - веб-интерфейс и HTTP -------------------
#pragma once                    // Защищает от повторного включения этого заголовочного файла

#include <Arduino.h>             // Основная библиотека Arduino для ESP32
#include <AsyncTCP.h>            // Асинхронный TCP для ESP32 (не блокирующий)
#include <ESPAsyncWebServer.h>   // Асинхронный веб-сервер для ESP32
#include <esp_system.h>
#include <vector>                // STL вектор для хранения элементов UI
#include <functional>
#include <map>                   // контейнер для провайдеров значений UI
#include <esp_chip_info.h>
#include <esp_efuse.h>
#include "graph.h"               // Пользовательские графики (кастомные)
#include "fs_utils.h"            // Вспомогательные функции для работы с файловой системой
#include "wifi_manager.h"                // Логика Wi-Fi и хранение параметров
#include "settings_MQTT.h"       // Настройки и работа с MQTT
#include <ArduinoJson.h>

using std::vector;              // Используем vector без указания std:: каждый раз

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
// Эти переменные управляют значениями элементов интерфейса
inline String ThemeColor;        // Цвет темы интерфейса
inline String ColorLED; //Выбор цвета для лентиы WS2815
inline String LEDColor;          // Цвет LED-индикатора
inline int MotorSpeedSetting;    // Настройка скорости мотора (0-100)
inline int IntInput;             // Целочисленный ввод от пользователя
inline float FloatInput;         // Ввод с плавающей точкой
inline float Speed;              // Скорость (например, датчика)
inline float Temperatura;        // Температура
inline String Timer1;            // Таймер 1
inline String Comment;           // Текстовый комментарий
inline String CurrentTime;       // Текущее время
inline int RandomVal;            // Случайное значение (например, для демонстрации)
inline String InfoString;        // Информационная строка
inline String InfoString1;       // Вспомогательная информационная строка
inline String InfoString2;
inline String InfoStringDIN;     // Состояние входов DIN

inline String OverlayPoolTemp;   // Температура воды в бассейне (оверлей)
inline String OverlayHeaterTemp; // Температура после нагревателя (оверлей)
inline String OverlayLevelUpper; // Верхний датчик уровня (оверлей)
inline String OverlayLevelLower; // Нижний датчик уровня (оверлей)
inline String OverlayPh;         // pH воды (оверлей)
inline String OverlayChlorine;   // Хлор (оверлей)
inline String OverlayFilterState; // Состояние фильтра (оверлей)
inline String ModeSelect;        // Режим работы (например, Auto/Manual)
inline String DaysSelect;        // Выбор дней недели
inline String SetLamp;           // Режим работы лампы
inline String SetRGB;            // Режим управления RGB подсветкой
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
String formatTemperatureString(float value, bool available);

void appendUiRegistryValues(JsonDocument &doc);



bool Slep = false; //Флаг режима сна
int Lumen_Ul, Saved_Lumen_Ul; //освещенность на улице
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
bool Activation_Heat, Activation_Heat1; // Включение и Активация контроля включения нагрева воды
int Sider_heat,  Sider_heat1; 			// Sider_heat1; bool Sider_Heat; // Переменная для получения или передачи из в  Nextion c  сидера монитора уставки нагрева воды

bool RoomTemper = false;
float RoomTempOn = 3.0f;
float RoomTempOff = 4.0f;
bool Power_Warm_floor_heating = false;
	
bool Pow_Ul_light, Pow_Ul_light1; // Промывка фильтра
bool Ul_light_Time, Saved_Ul_light_Time; // Разрешения работы включения по времени
String Ul_light_timeON, Ul_light_timeOFF; // Утавки времени включения


bool Activation_Water_Level = false;
bool WaterLevelSensorUpper = false;
bool WaterLevelSensorLower = false;
bool Power_Topping, Power_Topping1; // Долив воды по уровню
bool Power_Topping_State = false;

bool Saved_Power_H2O, Power_H2O2 = false; //Дозация перекеси водорода
bool Saved_Power_ACO, Power_ACO = false; 	//Дозация Активное Каталитическое Окисление «Active Catalytic Oxidation» ACO
bool ManualPulse_H2O2_Active = false;
bool ManualPulse_ACO_Active = false;
unsigned long ManualPulse_H2O2_StartedAt = 0;
unsigned long ManualPulse_ACO_StartedAt = 0;


bool PH_Control_ACO, Saved_PHControlACO; // Флаг для отслеживания предыдущего состояния PH_Control_ACO
int ACO_Work = 11, Saved_ACO_Work;


bool NaOCl_H2O2_Control, Saved_NaOCl_H2O2_Control;
int H2O2_Work = 11, Saved_H2O2_Work;

int corr_ORP_temper_mV; 		// ОРП с учетом калибровки по температуре
int CalRastvor256mV	= 256;	//Калибровочный раствор
int Calibration_ORP_mV = 0; //Калибровочный кооффициент - разница между колибровочным раствором 256mV 25C	 и показаниями сенсора
int corrected_ORP_Eh_mV;		// ОРП с учетом калибровки по  калибровочному раствору 	
float Saved_ppmCl, ppmCl = 1.3; //Свободный Хлор CL2 -  млг/литр, норма: 1,3млг/литр

//Строки для хранения информации о таймерах
char Info_H2O2[50]; String Saved_Info_H2O2;
char Info_ACO[50];  String Saved_Info_ACO;

// ===== Настройки пределов pH =====
float PH_setting = 7.2; // Верхний предел для включения дозирования

// ===== Настройки пределов ORP =====
int ORP_setting = 500; // Нижний предел для включения дозирования


bool Pow_WS2815, Pow_WS28151;		// Включение в ручную
bool Pow_WS2815_autosvet, Saved_Pow_WS2815_autosvet; 
bool WS2815_Time1, Saved_WS2815_Time1;

uint16_t Saved_timeON_WS2815, Saved_timeOFF_WS2815;

inline int TimertestON = 0;        // Значение включения тестового таймера (минуты от начала суток)
inline int TimertestOFF = 0;       // Значение отключения тестового таймера (минуты от начала суток)
inline int FiltrTimer1ON = 0;      // Время включения фильтрации №1 в минутах
inline int FiltrTimer1OFF = 0;     // Время отключения фильтрации №1 в минутах
inline int FiltrTimer2ON = 0;      // Время включения фильтрации №2
inline int FiltrTimer2OFF = 0;     // Время отключения фильтрации №2
inline int FiltrTimer3ON = 0;      // Время включения фильтрации №3
inline int FiltrTimer3OFF = 0;     // Время отключения фильтрации №3
inline int CleanTimer1ON = 0;      // Время включения промывки в минутах
inline int CleanTimer1OFF = 0;     // Время отключения промывки в минутах
inline int LampTimerON = 0;        // Таймер лампы: начало
inline int LampTimerOFF = 0;       // Таймер лампы: конец
inline int RgbTimerON = 0;         // Таймер RGB-подсветки: начало
inline int RgbTimerOFF = 0;        // Таймер RGB-подсветки: конец
inline int UlLightTimerON = 0;     // Таймер уличного освещения: начало
inline int UlLightTimerOFF = 0;    // Таймер уличного освещения: конец


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
bool Filtr_Time1, Filtr_Time2, Filtr_Time3; // Разрешения работы включения по времени
bool Saved_Filtr_Time1, Saved_Filtr_Time2, Saved_Filtr_Time3;
uint16_t Saved_Filtr_timeON1, Saved_Filtr_timeOFF1, Saved_Filtr_timeON2, Saved_Filtr_timeOFF2, Saved_Filtr_timeON3, Saved_Filtr_timeOFF3; 

bool Power_Clean, Power_Clean1; // Промывка фильтра
bool Clean_Time1, Saved_Clean_Time1; // Разрешения работы включения по времени
uint16_t Saved_Clean_timeON1, Saved_Clean_timeOFF1;

bool chk1, chk2, chk3, chk4, chk5, chk6, chk7; //Дни недели ПН, ВТ, СР, ЧТ, ПТ, СБ, ВС - для включения таймера в нужные дни
bool Saved_chk1, Saved_chk2, Saved_chk3, Saved_chk4, Saved_chk5, Saved_chk6, Saved_chk7;

// inline void syncCleanDaysFromSelection(){
//   const char* tokens[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
//   bool *flags[] = {&chk1, &chk2, &chk3, &chk4, &chk5, &chk6, &chk7};
//   for(size_t i = 0; i < 7; i++){
//     *flags[i] = DaysSelect.indexOf(tokens[i]) >= 0;
//   }
// }

// inline void syncDaysSelectionFromClean(){
//   const char* tokens[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
//   const bool flags[] = {chk1, chk2, chk3, chk4, chk5, chk6, chk7};
//   String next;
//   for(size_t i = 0; i < 7; i++){
//     if(flags[i]){
//       if(next.length()) next += ",";
//       next += tokens[i];
//     }
//   }
//   DaysSelect = next;
// }

// inline uint16_t parseTimeToMinutes(const String &value){
//   int sep = value.indexOf(':');
//   if(sep < 0) return 0;
//   int hours = value.substring(0, sep).toInt();
//   int minutes = value.substring(sep + 1).toInt();
//   hours = constrain(hours, 0, 23);
//   minutes = constrain(minutes, 0, 59);
//   return static_cast<uint16_t>(hours * 60 + minutes);
// }

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
// inline String formatMinutesToTime(uint16_t minutes){
//   minutes = minutes % 1440;
//   uint16_t hours = minutes / 60;
//   uint16_t mins = minutes % 60;
//   char buffer[6];
//   snprintf(buffer, sizeof(buffer), "%02u:%02u", hours, mins);
//   return String(buffer);
// }

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
  
  uint16_t loadTimerValue(const String &id, const String &suffix){
    String storageKey = timerStorageKey(id, suffix); // Формирование ключа хранения
    int value = loadValue<int>(storageKey.c_str(), -1); // Загрузка значения из хранилища
    if(value < 0){
      String legacyKey = legacyTimerStorageKey(id, suffix); // Формирование legacy-ключа
      if(legacyKey.length()){
        value = loadValue<int>(legacyKey.c_str(), 0); // Загрузка значения по legacy-ключу
      } else {
        value = 0; // Значение по умолчанию
      }
    }
    return static_cast<uint16_t>(value); // Возврат значения в минутах
  }
};

inline UIRegistry ui;

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
      return active ? String("? Работа") : String("?? Откл.");
    });
    registerUiValueProvider("Power_ACO_Button", [](){ // кнопка также светится при ручном импульсе
      return (Power_ACO || ManualPulse_ACO_Active) ? String("1") : String("0");
    });
    registerUiValueProvider("Power_H2O2", [](){ // статус дозатора H2O2
      const bool active = Power_H2O2 || ManualPulse_H2O2_Active;
      return active ? String("? Работа") : String("?? Откл.");
    });
    registerUiValueProvider("Power_H2O2_Button", [](){ // кнопка H2O2 реагирует на импульсы
      return (Power_H2O2 || ManualPulse_H2O2_Active) ? String("1") : String("0");
    });


    server.on("/", HTTP_GET, [self](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
       // Формируем HTML-страницу
      String html;
      html.reserve(120000);
      html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'><title>";
      html += dashAppTitle;
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
      ".dash-btn:hover{transform:translateY(-1px);box-shadow:0 6px 14px rgba(0,0,0,0.45);} " // Эффект наведения на кнопку
                ".page{display:none;position:relative;grid-template-columns: repeat(auto-fill, minmax(250px, 1fr)); gap:15px;} " // Страница интерфейса
      ".page.active{display:block;} " // Отображение активной страницы
            ".page-header{display:flex;flex-direction:row;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;margin-bottom:10px;} " // Заголовок страницы
      ".page-header h3{margin:0;} " // Заголовок без отступов
             ".page-datetime{font-size:clamp(0.95em, 1.6vw, 1.25em);letter-spacing:0.08em;text-align:right;font-weight:600;" // Блок даты и времени
      "margin-left:auto;display:inline-flex;align-items:center;justify-content:center;max-width:100%;" // Выравнивание блока даты
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
   ".select-days{--day-accent:#4cc3ff;display:flex;flex-wrap:nowrap;gap:8px;" // Контейнер выбора дней
      "justify-content:center;align-items:center;padding:8px;" // Выравнивание элементов дней
      "background:linear-gradient(135deg,rgba(255,255,255,0.06),rgba(255,255,255,0.02));" // Фон панели дней
      "border-radius:14px;border:1px solid rgba(255,255,255,0.12);" // Скругление и рамка
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 10px 24px rgba(0,0,0,0.45);" // Тени панели
      "overflow-x:auto;scrollbar-width:thin;scrollbar-color:rgba(255,255,255,0.2) transparent;} " // Горизонтальный скролл
      ".select-days::-webkit-scrollbar{height:6px;} " // Высота скроллбара
      ".select-days::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.2);border-radius:999px;} " // Ползунок скроллбара
      ".select-days .day-pill{position:relative;display:inline-flex;align-items:center;justify-content:center;" // Кнопка дня
      "min-width:42px;height:32px;padding:0 12px;border-radius:999px;" // Размеры и форма кнопки
      "font-size:0.78em;font-weight:600;letter-spacing:0.08em;text-transform:uppercase;" // Шрифт кнопки дня
      "color:#b7c0ce;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.1);" // Цвет и рамка
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 6px 14px rgba(0,0,0,0.4);" // Тени кнопки
      "cursor:pointer;transition:transform 0.18s ease,box-shadow 0.18s ease,background 0.18s ease,color 0.18s ease;} " // Анимации кнопки
      ".select-days .day-pill input{position:absolute;opacity:0;pointer-events:none;} " // Скрытый input
      ".select-days .day-pill:hover{transform:translateY(-1px) scale(1.05);" // Hover-эффект кнопки
      "box-shadow:0 10px 18px rgba(0,0,0,0.45);} " // Усиленная тень при наведении
      ".select-days .day-pill:active{transform:translateY(0) scale(0.98);} " // Эффект нажатия
      ".select-days .day-pill:has(input:checked){color:#fff;" // Стиль выбранного дня
      "background:linear-gradient(135deg,rgba(76,195,255,0.3),rgba(76,195,255,0.12));" // Фон выбранного дня
      "border-color:rgba(76,195,255,0.6);" // Рамка выбранного дня
      "box-shadow:0 0 0 1px rgba(76,195,255,0.35),0 12px 26px rgba(76,195,255,0.35);} " // Подсветка выбранного дня
      ".select-days .day-pill:has(input:checked)::after{content:'';position:absolute;inset:-6px;" // Свечение выбранного дня
      "border-radius:inherit;background:radial-gradient(circle,rgba(76,195,255,0.35),transparent 65%);" // Градиент свечения
      "opacity:0.75;filter:blur(6px);z-index:0;} " // Размытие свечения
      ".select-days .day-pill span{position:relative;z-index:1;} " // Текст поверх эффектов
      "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:center;} " // Таблицы
      "th{background:#333;} " // Заголовки таблиц
      ".card.pro-card{background:linear-gradient(135deg,#0f0f12,#151a2d);border:1px solid rgba(129,193,255,0.4);} " // Pro-карточка
      ".card.pro-card label{color:#9ae7ff;text-transform:uppercase;letter-spacing:0.08em;font-size:0.75em;} " // Заголовки Pro-карточки
      ".card.pro-card input,.card.pro-card select{background:#090c10;border-color:#252f40;color:#eff6ff;} " // Поля ввода Pro-карточки
      ".graph-controls{display:flex;flex-direction:row;flex-wrap:nowrap;gap:18px;align-items:center;} " // Панель управления графиком
      ".graph-controls .control-group{flex:1;min-width:220px;display:flex;align-items:center;gap:10px;} " // Группа контролов графика
      ".graph-controls label{display:inline-flex;font-size:0.72em;color:#9fb4c8;letter-spacing:0.08em;text-transform:uppercase;margin-bottom:0;white-space:nowrap;} " // Подписи графика
      ".graph-controls select,.graph-controls input{margin-bottom:0;flex:1;} " // Поля управления графиком
 ".graph-card{position:relative;}"  // Карточка графика
      ".graph-heading{text-align:center;}"  // Заголовок графика
      ".dash-graph{width:100%;background:#05070a;}"  // Холст графика
      ".graph-axes{position:absolute;inset:0;pointer-events:none;font-size:0.85em;color:#cbd4df;}"  // Подписи осей
      ".graph-axes .axis-name{position:absolute;}"  // Названия осей
      ".graph-axes .axis-name:first-child{right:10px;bottom:8px;}"  // Подпись оси X
      ".graph-axes .axis-name:last-child{left:8px;top:8px;writing-mode:vertical-rl;transform:rotate(180deg);}"  // Подпись оси Y
      ".graph-tooltip{position:fixed;pointer-events:none;background:rgba(10,14,20,0.9);color:#eef4ff;padding:6px 10px;border:1px solid rgba(255,255,255,0.18);border-radius:6px;font-size:0.8em;z-index:1000;transform:translate(-50%,-120%);white-space:nowrap;box-shadow:0 4px 12px rgba(0,0,0,0.35);}"  // Всплывающая подсказка графика
      ".graph-tooltip.hidden{display:none;}"  // Скрытое состояние tooltip
      ".card:has(#ModeSelect),.card:has(#LEDColor),.card:has(#Timer1),"  // Специальные карточки UI
      ".card:has(#FloatInput),.card:has(#IntInput),"
      ".card:has(#RandomVal),.card:has(#DaysSelect){"
      "background:linear-gradient(145deg,#111,#080b13);border:1px solid rgba(159,180,255,0.25);" // Фон и рамка специальных карточек
      "box-shadow:0 18px 34px rgba(0,0,0,0.65);border-radius:18px;padding:14px 16px;" // Тени и скругления
      "transition:transform 0.18s ease,box-shadow 0.18s ease;" // Анимации карточек
      "position:relative;overflow:hidden;" // Контекст для эффектов
      "} " // Конец блока специальных карточек

      ".card:has(#ModeSelect):hover,.card:has(#LEDColor):hover,.card:has(#Timer1):hover," // Hover-эффект для ключевых карточек
        ".card:has(#FloatInput) label,.card:has(#IntInput) label," // Акцент на label в карточках ввода
      ".card:has(#RandomVal):hover,.card:has(#DaysSelect):hover{" // Hover для случайного значения и дней
      "transform:translateY(-2px);box-shadow:0 22px 40px rgba(0,0,0,0.7);}" // Подъём карточки и усиленная тень
      ".card:has(#ModeSelect) label,.card:has(#LEDColor) label,.card:has(#Timer1) label," // Заголовки карточек
      ".card:has(#FloatInput) label,.card:has(#IntInput) label,.card:has(#CurrentTime) label," // Заголовки карточек ввода и времени
      ".card:has(#RandomVal) label,.card:has(#DaysSelect) label{" // Заголовки карточек значений и дней
      "font-size:0.72em;letter-spacing:0.08em;text-transform:uppercase;color:#94b4d6;" // Стиль заголовков карточек
      "} "
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

            ".network-row{display:flex;flex-direction:column;align-items:flex-start;gap:4px;padding:12px 14px;background:transparent;border:none;border-bottom:1px solid rgba(255,255,255,0.05);color:#e9ecf4;text-align:left;cursor:pointer;transition:background 0.12s ease;} " // Строка сети Wi-Fi
      ".network-row:hover{background:rgba(255,255,255,0.04);} " // Hover подсветка строки сети
      ".network-ssid{font-weight:700;} " // Имя сети (SSID)
      ".network-meta{color:#9fb4c8;font-size:0.9em;} " // Дополнительная информация о сети
      ".empty-row{padding:16px;color:#9fb4c8;text-align:center;} " // Пустое состояние списка
      ".icon-btn{background:none;border:none;color:#fff;font-size:1.4em;cursor:pointer;line-height:1;} " // Иконка-кнопка без фона

      ".mqtt-grid{display:flex;flex-direction:column;gap:12px;} " // Сетка настроек MQTT
      ".mqtt-field{display:flex;flex-direction:column;gap:6px;} " // Поле настроек MQTT
      ".mqtt-actions{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-top:8px;} " // Кнопки действий MQTT
      ".profile-hint{color:#9fb4c8;font-size:0.9em;} " // Подсказка профиля
      ".btn-toggle-on{background:linear-gradient(135deg,#4caf50,#81c784);color:#0b0f14;} " // Кнопка включённого состояния
      ".btn-mqtt{position:relative;overflow:hidden;background:linear-gradient(135deg,#1f2a44,#263555);border:1px solid rgba(111,168,255,0.35);color:#e6edff;box-shadow:0 12px 24px rgba(0,0,0,0.4);} " // Базовая кнопка MQTT
      ".btn-mqtt:before{content:'';position:absolute;inset:0;opacity:0;pointer-events:none;background:radial-gradient(circle at 20% 20%,rgba(255,255,255,0.28),transparent 45%);transition:opacity 0.18s ease;} " // Световой эффект кнопки MQTT
      ".btn-mqtt:hover:before{opacity:1;} " // Активация эффекта при наведении
      ".btn-mqtt.btn-warn{background:linear-gradient(135deg,#2d1e12,#3c2a18);border-color:rgba(255,193,7,0.45);color:#ffe9b3;} " // MQTT кнопка предупреждения
      ".btn-mqtt.btn-success{background:linear-gradient(135deg,#123420,#1d4b2a);border-color:rgba(76,175,80,0.55);color:#d5ffde;} " // MQTT кнопка успеха
      ".btn-mqtt.btn-activate-off{background:linear-gradient(135deg,#1b2b52,#23406f);border-color:rgba(64,139,255,0.55);color:#e5efff;} " // MQTT кнопка выключенного состояния
      ".btn-mqtt.btn-activate-on{background:linear-gradient(135deg,#0f3b1f,#13532a);border-color:rgba(76,175,80,0.6);color:#d8ffe4;} " // MQTT кнопка включённого состояния
   
      "</style></head><body>"; // Завершение стилей и начало body

    // Sidebar
      html += "<div id='sidebar'>"; // Начало боковой панели
      bool first = true; // Флаг для первой вкладки (активной по умолчанию)
      for(auto &t : self->tabs){ // Перебор зарегистрированных вкладок
        html += "<button onclick=\"showPage('"+t.id+"',this)\""; // Кнопка вкладки с обработчиком
        if(first){ html += " class='active'"; first=false; } // Первая вкладка делается активной
        html += ">"+t.title+"</button>"; // Заголовок вкладки
      }
      html += "<hr><div class='card'><label>Theme color</label>" // Разделитель и карточка цвета темы
              "<input id='ThemeColor' type='color' value='"+ThemeColor+"'></div>"; // Input выбора цвета темы
      html += "<button onclick=\"showPage('wifi',this)\">WiFi Settings</button>"; // Кнопка настроек Wi-Fi
      html += "<button onclick=\"showPage('stats',this)\">Statistics</button>"; // Кнопка статистики
            html += "<button onclick=\"showPage('profile',this)\">Профиль</button>"; // Кнопка профиля
      html += "<button onclick=\"showPage('mqtt',this)\">Настройка MQTT</button>"; // Кнопка настроек MQTT
      html += "</div>"; // Конец боковой панели

      html += "<button id='toggleBtn' onclick='toggleSidebar()'>?</button>"; // Кнопка сворачивания сайдбара

      // Основной контент
      html += "<div id='main'>"; // Начало основной области

      auto renderTabElements = [&](const String &tabId){ // Лямбда для рендера элементов вкладки
        for(auto &e : self->elements){ // Перебор всех UI-элементов
              if(e.tab != tabId || e.type != "image") continue; // Фильтр по вкладке и типу image

              String imgSrc = e.label; // Источник изображения
              if(!imgSrc.startsWith("http") && !imgSrc.startsWith("/")) imgSrc = "/" + imgSrc; // Приведение к относительному пути
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
                  String updatePeriodRaw = readSetting("updatePeriod_of_Time"); // Период обновления
                  String updateStepRaw = readSetting("updateStep"); // Шаг обновления
                  unsigned long maxUpdatePeriodMinutes = updatePeriodRaw.length() ? updatePeriodRaw.toInt() : 10; // минуты
                  unsigned long updateStepMinutes = updateStepRaw.length() ? updateStepRaw.toInt() : 1;           // минуты
                  unsigned long maxUpdatePeriod = maxUpdatePeriodMinutes * 60000UL; // В миллисекундах
                  unsigned long updateStep = updateStepMinutes * 60000UL; // В миллисекундах

                  if(maxUpdatePeriod < minGraphUpdateInterval) maxUpdatePeriod = minGraphUpdateInterval; // Минимальный предел
                  if(updateStep < minGraphUpdateInterval) updateStep = minGraphUpdateInterval; // Минимальный шаг
                  if(updateStep > maxUpdatePeriod) updateStep = maxUpdatePeriod; // Ограничение шага
                  const size_t maxUpdateOptions = 120; // Максимум вариантов
                  unsigned long optionCount = maxUpdatePeriod / updateStep; // Количество опций
                  if(optionCount > maxUpdateOptions){
                    updateStep = maxUpdatePeriod / maxUpdateOptions; // Автокоррекция шага
                    if(updateStep < minGraphUpdateInterval) updateStep = minGraphUpdateInterval;
                  }
                  String maxPointsRaw = readSetting("maxPoints"); // Максимум точек
                  int defaultGraphMax = maxPointsRaw.length() ? maxPointsRaw.toInt() : (maxPoints > 0 ? maxPoints : 30); // Значение по умолчанию
                  if(defaultGraphMax < minGraphPoints) defaultGraphMax = minGraphPoints; // Минимум
                  int maxSelectablePoints = defaultGraphMax; // Ограничение выбора
                  if(maxSelectablePoints > maxGraphPoints) maxSelectablePoints = maxGraphPoints; // Максимум
                  unsigned long defaultUpdate = updateInterval > 0 ? updateInterval : updateStep; // Интервал обновления
                  GraphSettings seriesSettings{defaultUpdate, maxSelectablePoints}; // Настройки серии

                  if(!loadGraphSettings(seriesName, seriesSettings)){ // Загрузка сохранённых настроек
                    loadGraphSettings(valueName, seriesSettings);
                  }
                  int graphUpdateInterval = seriesSettings.updateInterval; // Интервал обновления графика
                  if(graphUpdateInterval < (int)minGraphUpdateInterval) graphUpdateInterval = minGraphUpdateInterval;
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
                  html += "<canvas id='graph_"+e.id+"' class='dash-graph' data-graph-id='"+e.id+"' width='"+String(canvasWidth)+"' height='"+String(canvasHeight)+"' data-series='"+seriesName+"' data-table-id='"+tableId+"' data-update-interval='"+String(graphUpdateInterval)+"' data-max-points='"+String(graphMaxPoints)+"' data-line-color='"+lineColor+"' data-point-color='"+pointColor+"' data-x-label='"+xLabel+"' data-y-label='"+yLabel+"' style='border:1px solid rgba(255,255,255,0.08);height:"+heightStyle+";'></canvas>"; // Canvas графика
                  html += "<div class='graph-axes'><span class='axis-name'>"+xLabel+"</span><span class='axis-name'>"+yLabel+"</span></div>"; // Подписи осей
              html += "</div>"; // Закрытие карточки графика
                  html += "<div class='card graph-controls' style='margin-bottom:0;'>"; // Панель управления графиком
                  html += "<div class='control-group'><label>Update Interval</label>"; // Выбор интервала
                  html += "<select class='graph-update-interval' data-graph='"+e.id+"'>"; // Select интервала
                  auto formatIntervalLabel = [](unsigned long valMs){ // Форматирование интервала
                      if(valMs % 60000 == 0){
                          unsigned long minutes = valMs / 60000;
                          return String(minutes) + " min";
                      }
                      if(valMs % 1000 == 0){
                          unsigned long seconds = valMs / 1000;
                          return String(seconds) + " sec";
                      }
                      return String(valMs) + " ms";
                  };
                  if(maxUpdatePeriod >= 1000){
                      const unsigned long oneSecond = 1000;
                      String val = String(oneSecond);
                      html += String("<option value='") + val + "'" + (graphUpdateStr==val ? " selected" : "") + ">" + formatIntervalLabel(oneSecond) + "</option>"; // Опция 1 секунда
                  }

                  for(unsigned long opt = updateStep; opt <= maxUpdatePeriod; opt += updateStep){
                      String val = String(opt);
                      html += String("<option value='") + val + "'" + (graphUpdateStr==val ? " selected" : "") + ">" + formatIntervalLabel(opt) + "</option>"; // Опции интервалов
                                          if(maxUpdatePeriod - opt < updateStep) break;
                  }
                  html += "</select></div>"; // Закрытие select
                  html += "<div class='control-group'><label>Max Points</label>"; // Выбор количества точек
                  html += "<input class='graph-max-points' data-graph='"+e.id+"' type='number' min='1' max='"+String(maxSelectablePoints)+"' value='"+graphMaxStr+"'>"; // Input max точек
                  html += "</div>";
                  html += "</div>";
                  html += "<div class='card' style='overflow-x:auto;'>"; // Карточка таблицы
                  html += "<table id='"+tableId+"' style='min-width:400px;'>"; // Таблица данных
                  html += "<thead><tr><th>#</th><th>Date &amp; Time</th><th>Value</th></tr></thead><tbody></tbody>"; // Заголовок таблицы
                  html += "</table></div>";
                  continue; // Переход к следующему элементу
              }
              String val = uiValueForId(e.id); // Текущее значение элемента UI
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
                          "  const updateRangeSlider = () => {"
                          "    let min = parseFloat(minEl.value);"
                          "    let max = parseFloat(maxEl.value);"
                          "    if(min > max){ [min, max] = [max, min]; minEl.value=min; maxEl.value=max; }"
                          "    const fixed = " + String(precision) + ";"
                          "    if(display) display.innerText = min.toFixed(fixed) + ' - ' + max.toFixed(fixed);"
                          "    const span = (maxLimit - minLimit) || 1;"
                          "    const minPct = ((min - minLimit) / span) * 100;"
                          "    const maxPct = ((max - minLimit) / span) * 100;"
                          "    if(track) track.style.background = `linear-gradient(to right, #444 ${minPct}%, #1e88e5 ${minPct}%, #1e88e5 ${maxPct}%, #444 ${maxPct}%)`;"
                          "    const minSaved = min.toFixed(fixed);"
                          "    const maxSaved = max.toFixed(fixed);"
                          "    fetch('/save?key=' + encodeURIComponent(rangeId) + '&val=' + encodeURIComponent(minSaved + '-' + maxSaved));"
                          "  };"
                          "  minEl.addEventListener('input', updateRangeSlider);"
                          "  maxEl.addEventListener('input', updateRangeSlider);"
                          "  updateRangeSlider();"
                          "})();"
                          "</script>";
              }

                            html += "</div>";
          }
                };

      bool firstPage = true;
      for(auto &t : self->tabs){
           html += "<div id='"+t.id+"' class='page"+String(firstPage?" active":"")+"'>"
                  "<div class='page-header'><h3>"+t.title+"</h3>"
                  "<div class='page-datetime' id='page-datetime-"+t.id+"'>--</div></div>";
          renderTabElements(t.id);
          html += "</div>";
          firstPage = false;
      }

      for(auto &popup : self->popups){
          html += "<div id='popup-"+popup.id+"' class='dash-modal hidden' data-popup='"+popup.id+"'>"
                  "<div class='dash-modal-content'>"
                  "<div class='dash-modal-header'><h4>"+popup.title+"</h4>"
                  "<button class='icon-btn' onclick=\"closePopup('"+popup.id+"')\">&times;</button></div>"
                  "<div class='dash-modal-body'><div class='popup-grid'>";
          renderTabElements(popup.tabId);
          html += "</div></div></div></div>";
      }

      
      // ====== WiFi страница ======
      html += "<div id='wifi' class='page'>"
              "<div class='page-header'><h3>WiFi Settings</h3>"
              "<div class='page-datetime' id='page-datetime-wifi'>--</div></div>"
              "<div class='card compact wifi-card'>"
              "<div class='wifi-field'><label>SSID</label><div class='input-with-action'>"
              "<input id='ssid' value='"+loadValue<String>("ssid",defaultSSID)+"'>"
              "<button class='btn-secondary' id='scan-btn' onclick='scanWiFi()'>Scan WiFi</button>"
              "</div></div>"
              
              "<div class='wifi-field'><label>Password</label><input id='pass' type='password' value='"+loadValue<String>("pass",defaultPASS)+"'></div>"
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

      // ====== Statistics страница ======
      html += "<div id='stats' class='page'>"
              "<div class='page-header'><h3>Statistics</h3>"
              "<div class='page-datetime' id='page-datetime-stats'>--</div></div>"
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
              "<li><span>Free Heap (свободная оперативная память)</span><strong id='stat-heap'>--</strong></li>"
               "<li><span>PSRAM (использовано / свободно в PSRAM)</span><strong id='stat-psram'>--</strong></li>"
              "<li><span>SPIFFS Used / Free (использовано / свободно в SPIFFS)</span><strong id='stat-spiffs'>--</strong></li>"
              "</ul></div>"
              "</div></div>";

      // ====== MQTT страница ======
      html += "<div id='mqtt' class='page'><h3>Настройка MQTT</h3>"
              "<div class='card compact'>"
              "<div class='mqtt-grid'>"
              "<div class='mqtt-field'><label>MQTT Host</label><input id='mqtt-host' value='"+mqttHost+"'></div>"
              "<div class='mqtt-field'><label>MQTT Port</label><input id='mqtt-port' type='number' value='"+String(mqttPort)+"'></div>"
              "<div class='mqtt-field'><label>MQTT Username</label><input id='mqtt-user' value='"+mqttUsername+"'></div>"
              "<div class='mqtt-field'><label>MQTT Password</label><input id='mqtt-pass' type='password' value='"+mqttPassword+"'></div>"
              "</div>"
              "<div class='mqtt-actions'>"
              "<button class='btn-primary btn-mqtt btn-success' id='mqtt-save-btn' onclick='saveMqttSettings()'>Сохранить настройки</button>"
                          "<button class='btn-secondary btn-mqtt btn-activate-off' id='mqtt-activate-btn' data-enabled='0' onclick='toggleMqttActivation()'>Реконект MQTT</button>"
              "</div>"
              "</div></div>";

 // ====== Профиль ======
      html += "<div id='profile' class='page'><h3>Профиль</h3>"
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

  // Показ выбранной страницы и скрытие остальных
  function showPage(id,btn){
    document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
    document.getElementById(id).classList.add('active'); // Отображаем выбранную страницу
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

  const wifiInputEdited = {ssid: false, pass: false};
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

  const updateStat = (id, value)=>{
    const el = document.getElementById(id);
    if(el) el.innerText = value;
  };


    const updateMqttActivationButton = (enabled, connected)=>{

    const btn = document.getElementById('mqtt-activate-btn');
    if(!btn) return;
    const active = Boolean(enabled && connected);
    btn.dataset.enabled = active ? '1' : '0';
    btn.classList.toggle('btn-activate-on', active);
    btn.classList.toggle('btn-activate-off', !active);
    btn.innerText = 'Реконект MQTT';
  };

  function fetchMqttConfig(){
    fetch('/mqtt/config').then(r=>r.json()).then(data=>{
      const host = document.getElementById('mqtt-host');
      const port = document.getElementById('mqtt-port');
      const user = document.getElementById('mqtt-user');
      const pass = document.getElementById('mqtt-pass');
      if(host) host.value = data.host || '';
      if(port) port.value = data.port || '';
      if(user) user.value = data.user || '';
      if(pass) pass.value = data.pass || '';
      mqttEnabledState = Boolean(data.enabled);
      mqttConnectedState = Boolean(data.enabled && data.connected);
      updateMqttActivationButton(mqttEnabledState, mqttConnectedState);
    }).catch(()=>{
      mqttConnectedState = false;
      updateMqttActivationButton(false, false);
      
    });
  }

  function saveMqttSettings(){
    const saveBtn = document.getElementById('mqtt-save-btn');
    const originalText = saveBtn ? saveBtn.innerText : '';
    if(saveBtn){ saveBtn.disabled = true; saveBtn.innerText = 'Сохранение...'; }

    const payload = new URLSearchParams({
      host: (document.getElementById('mqtt-host') || {}).value || '',
      port: (document.getElementById('mqtt-port') || {}).value || '',
      user: (document.getElementById('mqtt-user') || {}).value || '',
      pass: (document.getElementById('mqtt-pass') || {}).value || '',
      enabled: (document.getElementById('mqtt-activate-btn') || {dataset:{enabled:'0'}}).dataset.enabled === '1' ? '1' : '0'
    });
    fetch('/mqtt/save',{method:'POST', body:payload})
      .then(()=>setTimeout(fetchMqttConfig, 300))

      .finally(()=>{ if(saveBtn){ saveBtn.disabled = false; saveBtn.innerText = originalText || 'Сохранить настройки'; } });
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
    const btn = document.getElementById('mqtt-activate-btn');
    const enable = !(btn && btn.dataset.enabled === '1');
    mqttEnabledState = enable;
    mqttConnectedState = enable ? mqttConnectedState : false;
    const original = btn ? btn.innerText : '';
    if(btn){
      btn.disabled = true;
           btn.innerText = 'Реконектируем MQTT...';
      btn.classList.toggle('btn-activate-on', enable);
      btn.classList.toggle('btn-activate-off', !enable);
    }
    const payload = new URLSearchParams({enabled: enable ? '1' : '0'});
    fetch('/mqtt/activate',{method:'POST', body:payload})
      .then(()=>setTimeout(fetchMqttConfig, 400))
            .finally(()=>{ if(btn){ btn.disabled = false; btn.innerText = original || 'Реконект MQTT'; } });
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

  const updateWifiStatus = (data)=>{
    updateStat('wifi-status', data.wifiStatus || 'N/A');
    updateStat('wifi-mode', data.wifiMode || 'N/A');
    updateStat('wifi-ssid', data.ssid && data.ssid.length ? data.ssid : 'N/A');
    updateStat('wifi-ip', data.localIp && data.localIp.length ? data.localIp : 'N/A');
    const rssiVal = (typeof data.rssi !== 'undefined' && data.rssi !== null) ? data.rssi : 'N/A';
    const rssiText = isNaN(Number(rssiVal)) ? String(rssiVal) : `${rssiVal} dBm`;
    updateStat('wifi-rssi', rssiText);
    if(!wifiInputEdited.ssid && data.ssid && data.ssid.length){
      const ssidInput = document.getElementById('ssid');
      if(ssidInput) ssidInput.value = data.ssid;
    }
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

// Очистка флага ручного изменения через delay (по умолчанию 600 мс)
const clearManualFlag = (el, delay=600)=>{
  if(!el) return;
  setTimeout(()=>{
    if(el.dataset.manual === '1') el.dataset.manual = '';
  }, delay);
};
// Обновление слайдера и отправка значения на сервер
function updateSlider(el){
 document.getElementById(el.id+'Val').innerText=' '+el.value;
 fetch('/save?key='+el.id+'&val='+encodeURIComponent(el.value));
}
// Обновление значения input (текст, число и т.д.) с проверкой ручного ввода
function updateInputValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || el.dataset.manual === '1') return;
  const text = String(value);
  if(el.value !== text) el.value = text; //Динамическое обновление "Comment", "Start Time", "Enter Integer", "Enter Float"
  if(id==='LEDColor') refreshLedColorUI(text);
  else if(id==='ThemeColor') refreshThemeColorUI(text);
}

function updateCheckboxValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || el.dataset.manual === '1') return;
  const isOn = value == 1 || value === true || value === "1" || String(value).toLowerCase() === "true";
  el.checked = isOn;
}


function updateSelectValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || el.dataset.manual === '1') return;
  const text = String(value);
  if(el.value != text) el.value = text;
}

function applyLiveValue(id, value){
  const el = document.getElementById(id);
  if(!el || el.dataset.manual === '1') return;
  if(el.classList.contains('range-slider')){
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
  el.addEventListener('blur', ()=> clearManualFlag(el));
};


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
 let aps=document.getElementById('ap_ssid').value;
 let app=document.getElementById('ap_pass').value;
 let h=document.getElementById('hostname').value;
 const body = new URLSearchParams({ssid:s, pass:p, ap_ssid:aps, ap_pass:app, hostname:h});
 fetch('/wifi/save', {method:'POST', body})
   .then(r=>r.json())
   .then(data=>{
     const connected = data && data.connected ? ' (подключено)' : ' (AP mode)';
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
     tr.innerHTML = '<td>'+row+'</td><td>'+point.time+'</td><td>'+point.value+'</td>';
    tbody.appendChild(tr);
  }
}

function drawCustomGraph(canvas,data){
  if(!canvas || !canvas.getContext || !data.length) return;
  const ctx = canvas.getContext('2d');
  if(!ctx) return;
  const width = canvas.width;
  const height = canvas.height;
  ctx.clearRect(0,0,width,height);
  ctx.fillStyle = '#05070a';
  ctx.fillRect(0,0,width,height);

  ctx.strokeStyle = 'rgba(255,255,255,0.07)';
  ctx.lineWidth = 1;
  for(let i=0;i<=4;i++){
    let y = i*(height/4);
    ctx.beginPath();
    ctx.moveTo(0,y);
    ctx.lineTo(width,y);
    ctx.stroke();
  }

  const maxPointsAttr = parseInt(canvas.dataset.maxPoints);
  const maxPoints = !isNaN(maxPointsAttr) && maxPointsAttr > 0
    ? maxPointsAttr
    : 10;
  const pointsToDraw = data.slice(-maxPoints);
  const lineColor = canvas.dataset.lineColor || '#4CAF50';
  const pointColor = canvas.dataset.pointColor || '#ff0000';
  ctx.strokeStyle = lineColor;
  ctx.lineWidth = 2;
  ctx.beginPath();
  for(let i=0;i<pointsToDraw.length;i++){
    const x = i*(width/(pointsToDraw.length || 1));
    const y = height - (pointsToDraw[i].value/50.0)*height;
    if(i==0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();

  ctx.fillStyle = pointColor;
  for(let i=0;i<pointsToDraw.length;i++){
    const x = i*(width/(pointsToDraw.length || 1));
    const y = height - (pointsToDraw[i].value/50.0)*height;
    ctx.beginPath();
    ctx.arc(x,y,3,0,2*Math.PI);
    ctx.fill();
  }

  const labelOffsets = [-14, 10, -24, 6];
  ctx.font = '12px "Inter", "Segoe UI", system-ui, sans-serif';
  ctx.textAlign = 'center';
  ctx.fillStyle = 'rgba(235, 242, 255, 0.86)';
  for(let i=0;i<pointsToDraw.length;i++){
    const x = i*(width/(pointsToDraw.length || 1));
    const y = height - (pointsToDraw[i].value/50.0)*height;
    const offset = labelOffsets[i % labelOffsets.length];
    const labelY = Math.min(height - 6, Math.max(12, y + offset));
    const label = `${pointsToDraw[i].value}`;
    ctx.fillText(label, x, labelY);
  }
  graphDataCache.set(canvas, pointsToDraw);
  populateGraphTable(canvas.dataset.tableId, pointsToDraw);
}

function hideGraphTooltip(){
  graphTooltip.classList.add('hidden');
}

function showGraphTooltip(canvas, evt){
  if(!canvas || !graphDataCache.has(canvas)) return hideGraphTooltip();
  const points = graphDataCache.get(canvas);
  if(!points || !points.length) return hideGraphTooltip();

  const rect = canvas.getBoundingClientRect();
  const relX = evt.clientX - rect.left;
  const width = canvas.width || rect.width;
  const height = canvas.height || rect.height;
  const step = points.length ? (width / (points.length || 1)) : width;
  let index = Math.round(relX / (step || 1));
  if(index < 0) index = 0;
  if(index >= points.length) index = points.length - 1;
  const point = points[index];
  const x = index * (width / (points.length || 1));
  let y = height - (point.value / 50.0) * height;
  if(!isFinite(y)) y = height / 2;

  graphTooltip.textContent = `${point.time}: ${point.value}`;
  graphTooltip.style.left = `${rect.left + x}px`;
  graphTooltip.style.top = `${rect.top + y}px`;
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
  const delay = (!isNaN(interval) && interval > 0) ? interval : 1000;
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
    if(isNaN(value) || value < 100) value = 100;
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
    if(value > 50) value = 50;
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
    let pageDateTimeLastSync = 0;

    function formatDateTime(dateObj){
      const pad = (num)=>String(num).padStart(2, '0');
      return `${pad(dateObj.getDate())}.${pad(dateObj.getMonth()+1)}.${dateObj.getFullYear()} ` +
             `${pad(dateObj.getHours())}:${pad(dateObj.getMinutes())}:${pad(dateObj.getSeconds())}`;
    }

    function parseDeviceDateTime(value){
      const match = /^(\d{2})\.(\d{2})\.(\d{4}) (\d{2}):(\d{2}):(\d{2})$/.exec(value);
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
        pageDateTimeBase = parsed;
        pageDateTimeLastSync = Date.now();
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
    fetch('/live').then(r=>r.json()).then(j=>{
        if(typeof j.CurrentTime !== 'undefined') updatePageDateTime(j.CurrentTime);
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
    syncDashButton('button1', j.button1);
    syncDashButton('button2', j.button2);
    syncDashButton('button_Lamp', j.button_Lamp);
    syncDashButton('button_WS2815', j.button_WS2815);
    syncDashButton('Pow_Ul_light', j.Pow_Ul_light);
    syncDashButton('Power_H2O2_Button', j.Power_H2O2_Button);
    syncDashButton('Power_ACO_Button', j.Power_ACO_Button);
    syncDashButton('Power_Topping', j.Power_Topping);
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
    if(typeof j.Lumen_Ul !== 'undefined') updateInputValue('Lumen_Ul', j.Lumen_Ul);
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
    if(typeof j.Power_Topping_State !== 'undefined') updateStat('Power_Topping_State', j.Power_Topping_State);
    if(typeof j.PH !== 'undefined') updateStat('PH', j.PH);
    if(typeof j.analogValuePH !== 'undefined') updateStat('analogValuePH', j.analogValuePH);
    if(typeof j.PH_Control_ACO !== 'undefined') updateCheckboxValue('PH_Control_ACO', j.PH_Control_ACO);
    if(typeof j.PH_setting !== 'undefined') updateInputValue('PH_setting', j.PH_setting);
    if(typeof j.ACO_Work !== 'undefined') updateSelectValue('ACO_Work', j.ACO_Work);
    if(typeof j.Power_ACO !== 'undefined') updateStat('Power_ACO', j.Power_ACO);
    if(typeof j.ppmCl !== 'undefined') updateStat('ppmCl', j.ppmCl);
    if(typeof j.corrected_ORP_Eh_mV !== 'undefined') updateStat('corrected_ORP_Eh_mV', j.corrected_ORP_Eh_mV);
    if(typeof j.NaOCl_H2O2_Control !== 'undefined') updateCheckboxValue('NaOCl_H2O2_Control', j.NaOCl_H2O2_Control);
    if(typeof j.ORP_setting !== 'undefined') updateInputValue('ORP_setting', j.ORP_setting);
    if(typeof j.H2O2_Work !== 'undefined') updateSelectValue('H2O2_Work', j.H2O2_Work);
    if(typeof j.Power_H2O2 !== 'undefined') updateStat('Power_H2O2', j.Power_H2O2);
    if(typeof j.Temper_Reference !== 'undefined') updateInputValue('Temper_Reference', j.Temper_Reference);
    
        if(typeof j.FilterImageState !== 'undefined' && j.FilterImageState !== lastFilterImageState){
      lastFilterImageState = j.FilterImageState;
      const filterImage = document.getElementById('FilterImage');
      if(filterImage){
        filterImage.src = (j.FilterImageState === 1) ? '/anim1.gif' : '/img2.jpg';
      }
    }
    
    Object.keys(j).forEach(key=>applyLiveValue(key, j[key]));

    });
  }
setInterval(fetchLive, 1000);
setInterval(tickPageDateTime, 1000);
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

      r->send(200,"text/html",html);
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
        if(uiApplyValueForId(key, valStr)){
          r->send(200,"text/plain","OK");
          return;
        }

           if(key=="ThemeColor") { ThemeColor = valStr; saveValue<String>(key.c_str(), valStr); }

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
          GraphSettings cfg{static_cast<unsigned long>(valInt), maxPoints};
          loadGraphSettings(series, cfg);
          cfg.updateInterval = valInt;
          saveGraphSettings(series, cfg);
          seriesConfig[series].updateInterval = cfg.updateInterval;
          if(seriesConfig[series].maxPoints == 0) seriesConfig[series].maxPoints = cfg.maxPoints;
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
        
        else if(key=="apSSID") { StoredAPSSID = valStr; saveValue<String>(key.c_str(), valStr); }
        else if(key=="apPASS") { StoredAPPASS = valStr; saveValue<String>(key.c_str(), valStr); }
      }
      r->send(200,"text/plain","OK");
    });

    server.on("/mqtt/config", HTTP_GET, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      String json = "{\\\"host\\\":\\\""+jsonEscape(mqttHost)+"\\\",";
      json += "\\\"port\\\":" + String(mqttPort) + ",";
      json += "\\\"user\\\":\\\""+jsonEscape(mqttUsername)+"\\\",";
      json += "\\\"pass\\\":\\\""+jsonEscape(mqttPassword)+"\\\",";
      json += "\\\"enabled\\\":" + String(mqttEnabled ? 1 : 0) + ",";
      json += "\\\"connected\\\":" + String((mqttEnabled && mqttClient.connected()) ? 1 : 0) + "}";
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
      mqttPort = static_cast<uint16_t>(paramOr("port", String(mqttPort)).toInt());
      mqttUsername = paramOr("user", mqttUsername);
      mqttPassword = paramOr("pass", mqttPassword);
      mqttEnabled = paramOr("enabled", mqttEnabled ? "1" : "0").toInt() == 1;

     
      saveMqttSettings();
      applyMqttState();
      r->send(200, "application/json", "{\\\"status\\\":\\\"saved\\\"}");
    });


    server.on("/mqtt/activate", HTTP_POST, [](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
      bool enabled = mqttEnabled;
      if(r->hasParam("enabled", true)) enabled = r->getParam("enabled", true)->value().toInt() == 1;
      else if(r->hasParam("enabled")) enabled = r->getParam("enabled")->value().toInt() == 1;
      mqttEnabled = enabled;
      saveMqttSettings();
      applyMqttState();
      r->send(200, "application/json", "{\\\"status\\\":\\\"updated\\\"}");
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
       if(uiApplyValueForId(id, String(state))){
          r->send(200, "text/plain", "OK");
          return;
        }
        r->send(200, "text/plain", "OK");
      } else {
        r->send(400, "text/plain", "Missing params");
      }
    });

    server.on("/live", HTTP_GET, [](AsyncWebServerRequest *r){
          if(!ensureAuthorized(r)) return;
      // Увеличенный буфер, чтобы сериализация не обрезалась на длинных строках
      // StaticJsonDocument<4096> doc;
      // StaticJsonDocument<6144> doc;
            StaticJsonDocument<12288> doc;
      doc["CurrentTime"] = CurrentTime; // Временная метка для синхронизации времени страницы
      doc["FilterImageState"] = jpg; // Выбор картинки бассейна (анимация/статик)
      doc["button_Lamp"] = Lamp ? 1 : 0; // Отображение состояния кнопки лампы, которая не объявлена через UI
      for (const auto &timer : ui.allTimers()) {
        doc[String(timer.id + "_ON")] = formatMinutesToTime(timer.on);
        doc[String(timer.id + "_OFF")] = formatMinutesToTime(timer.off);
      }
            appendUiRegistryValues(doc);
      String s;
      serializeJson(doc, s);
      r->send(200, "application/json", s);
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
      String apSsid = paramOr("ap_ssid", loadValue<String>("apSSID", String(::apSSID)));
      String apPass = paramOr("ap_pass", loadValue<String>("apPASS", String(::apPASS)));
      String host = paramOr("hostname", loadValue<String>("hostname", String(defaultHostname)));

      StoredAPSSID = apSsid;
      StoredAPPASS = apPass;
      saveWifiConfig(ssid, pass, apSsid, apPass, host);
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
        s+="{\"time\":\""+(*points)[i].time+"\",\"value\":"+String((*points)[i].value)+"}";
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
    server.serveStatic("/img1.jpg", SPIFFS, "/img1.jpg"); // Отдаёт изображение img1.jpg из SPIFFS по HTTP пути /img1.jpg
    server.serveStatic("/img2.jpg", SPIFFS, "/img2.jpg"); // Отдаёт изображение img2.jpg из SPIFFS по HTTP пути /img2.jpg

    server.begin();
  }
} dash;
