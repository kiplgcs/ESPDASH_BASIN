// ------------------- web.h - веб-интерфейс и HTTP -------------------
#pragma once                    // Защищает от повторного включения этого заголовочного файла

#include <Arduino.h>             // Основная библиотека Arduino для ESP32
#include <AsyncTCP.h>            // Асинхронный TCP для ESP32 (не блокирующий)
#include <ESPAsyncWebServer.h>   // Асинхронный веб-сервер для ESP32
#include <esp_system.h>
#include <vector>                // STL вектор для хранения элементов UI
#include <functional>
#include <esp_chip_info.h>
#include <esp_efuse.h>
#include "graph.h"               // Пользовательские графики (кастомные)
#include "fs_utils.h"            // Вспомогательные функции для работы с файловой системой
#include "wifi_manager.h"                // Логика Wi-Fi и хранение параметров
#include "settings_MQTT.h"       // Настройки и работа с MQTT
#include <ArduinoJson.h>

using std::vector;              // Используем vector без указания std:: каждый раз

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


// inline void applyLampModeFromSetLamp() {
//   if (SetLamp == "off") {
//     Lamp = false;
//     Lamp_autosvet = false;
//     Power_Time1 = false;
//   } else if (SetLamp == "on") {
//     Lamp = true;
//     Lamp_autosvet = false;
//     Power_Time1 = false;
//   } else if (SetLamp == "auto") {
//     Lamp_autosvet = true;
//     Power_Time1 = false;
//   } else if (SetLamp == "timer") {
//     Power_Time1 = true;
//     Lamp_autosvet = false;
//   }
// }

// inline void syncSetLampFromFlags() {
//   if (Power_Time1) {
//     SetLamp = "timer";
//   } else if (Lamp_autosvet) {
//     SetLamp = "auto";
//   } else {
//     SetLamp = Lamp ? "on" : "off";
//   }
// }


bool Pow_WS2815, Pow_WS28151;		// Включение в ручную
bool Pow_WS2815_autosvet, Saved_Pow_WS2815_autosvet; 
bool WS2815_Time1, Saved_WS2815_Time1;
// String timeON_WS2815, timeOFF_WS2815; // Утавки времени включения-отключения LED ленты
// String Saved_timeON_WS2815, Saved_timeOFF_WS2815;
uint16_t Saved_timeON_WS2815, Saved_timeOFF_WS2815;


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
// String Filtr_timeON1, Filtr_timeOFF1, Filtr_timeON2, Filtr_timeOFF2, Filtr_timeON3, Filtr_timeOFF3; // Утавки времени включения
// String Saved_Filtr_timeON1, Saved_Filtr_timeOFF1, Saved_Filtr_timeON2, Saved_Filtr_timeOFF2, Saved_Filtr_timeON3, Saved_Filtr_timeOFF3; 
uint16_t Saved_Filtr_timeON1, Saved_Filtr_timeOFF1, Saved_Filtr_timeON2, Saved_Filtr_timeOFF2, Saved_Filtr_timeON3, Saved_Filtr_timeOFF3; 


bool Power_Clean, Power_Clean1; // Промывка фильтра
bool Clean_Time1, Saved_Clean_Time1; // Разрешения работы включения по времени
// String Clean_timeON1, Clean_timeOFF1; // Утавки времени включения
// String Saved_Clean_timeON1, Saved_Clean_timeOFF1;
uint16_t Saved_Clean_timeON1, Saved_Clean_timeOFF1;

bool chk1, chk2, chk3, chk4, chk5, chk6, chk7; //Дни недели ПН, ВТ, СР, ЧТ, ПТ, СБ, ВС - для включения таймера в нужные дни
bool Saved_chk1, Saved_chk2, Saved_chk3, Saved_chk4, Saved_chk5, Saved_chk6, Saved_chk7;

inline void syncCleanDaysFromSelection(){
  const char* tokens[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  bool *flags[] = {&chk1, &chk2, &chk3, &chk4, &chk5, &chk6, &chk7};
  for(size_t i = 0; i < 7; i++){
    *flags[i] = DaysSelect.indexOf(tokens[i]) >= 0;
  }
}

inline void syncDaysSelectionFromClean(){
  const char* tokens[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  const bool flags[] = {chk1, chk2, chk3, chk4, chk5, chk6, chk7};
  String next;
  for(size_t i = 0; i < 7; i++){
    if(flags[i]){
      if(next.length()) next += ",";
      next += tokens[i];
    }
  }
  DaysSelect = next;
}

inline uint16_t parseTimeToMinutes(const String &value){
  int sep = value.indexOf(':');
  if(sep < 0) return 0;
  int hours = value.substring(0, sep).toInt();
  int minutes = value.substring(sep + 1).toInt();
  hours = constrain(hours, 0, 23);
  minutes = constrain(minutes, 0, 59);
  return static_cast<uint16_t>(hours * 60 + minutes);
}

inline String formatMinutesToTime(uint16_t minutes){
  minutes = minutes % 1440;
  uint16_t hours = minutes / 60;
  uint16_t mins = minutes % 60;
  char buffer[6];
  snprintf(buffer, sizeof(buffer), "%02u:%02u", hours, mins);
  return String(buffer);
}

struct UITimerEntry {
  String id;
  String label;
  uint16_t on = 0;
  uint16_t off = 0;
  std::function<void(uint16_t, uint16_t)> callback = nullptr;
};

class UIRegistry {
public:
  static String timerStorageKey(const String &id, const String &suffix){
    if(id == "UlLightTimer"){
      return String("UlLightT") + suffix;
    }
    return id + suffix;
  }

  static String legacyTimerStorageKey(const String &id, const String &suffix){
    if(id == "UlLightTimer"){
      return id + suffix;
    }
    return String();
  }

  UITimerEntry &registerTimer(const String &id, const String &label,
                              const std::function<void(uint16_t, uint16_t)> &cb){
    UITimerEntry *entry = findTimer(id);
    if(!entry){
      UITimerEntry created;
      created.id = id;
      created.label = label;
      created.on = static_cast<uint16_t>(loadValue<int>((id + "_ON").c_str(), 0));
      created.off = static_cast<uint16_t>(loadValue<int>((id + "_OFF").c_str(), 0));
      created.on = loadTimerValue(id, "_ON");
      created.off = loadTimerValue(id, "_OFF");
      created.callback = cb;
      timers.push_back(created);
      entry = &timers.back();
    } else {
      entry->label = label;
      entry->callback = cb;
    }
    return *entry;
  }

  UITimerEntry &timer(const String &id){
    UITimerEntry *entry = findTimer(id);
    if(!entry){
      return registerTimer(id, id, nullptr);
    }
    return *entry;
  }

  bool updateTimerField(const String &fieldId, const String &value){
    bool isOn = fieldId.endsWith("_ON");
    bool isOff = fieldId.endsWith("_OFF");
    if(!isOn && !isOff) return false;
    String base = fieldId.substring(0, fieldId.length() - (isOn ? 3 : 4));
    UITimerEntry *entry = findTimer(base);
    if(!entry) return false;
    uint16_t minutes = parseTimeToMinutes(value);
    if(isOn) entry->on = minutes;
    else entry->off = minutes;
    saveValue<int>(fieldId.c_str(), minutes);
    String storageKey = timerStorageKey(base, isOn ? "_ON" : "_OFF");
    saveValue<int>(storageKey.c_str(), minutes);
    if(entry->callback) entry->callback(entry->on, entry->off);
    return true;
  }

  void setTimerMinutes(const String &id, uint16_t onMinutes, uint16_t offMinutes, bool persist = true){
    UITimerEntry &entry = timer(id);
    entry.on = onMinutes;
    entry.off = offMinutes;
    if(persist){
      saveValue<int>((id + "_ON").c_str(), entry.on);
      saveValue<int>((id + "_OFF").c_str(), entry.off);
      String onKey = timerStorageKey(id, "_ON");
      String offKey = timerStorageKey(id, "_OFF");
      saveValue<int>(onKey.c_str(), entry.on);
      saveValue<int>(offKey.c_str(), entry.off);
    }
    if(entry.callback) entry.callback(entry.on, entry.off);
  }

  const std::vector<UITimerEntry> &allTimers() const { return timers; }

private:
  std::vector<UITimerEntry> timers;

  UITimerEntry *findTimer(const String &id){
    for(auto &entry : timers){
      if(entry.id == id) return &entry;
    }
    return nullptr;
  }
  
  uint16_t loadTimerValue(const String &id, const String &suffix){
    String storageKey = timerStorageKey(id, suffix);
    int value = loadValue<int>(storageKey.c_str(), -1);
    if(value < 0){
      String legacyKey = legacyTimerStorageKey(id, suffix);
      if(legacyKey.length()){
        value = loadValue<int>(legacyKey.c_str(), 0);
      } else {
        value = 0;
      }
    }
    return static_cast<uint16_t>(value);
  }
};

inline UIRegistry ui;

inline void noopTimerCallback(uint16_t onMinutes, uint16_t offMinutes){
  (void)onMinutes;
  (void)offMinutes;
}

inline void onLampTimerChange(uint16_t onMinutes, uint16_t offMinutes){
  (void)onMinutes;
  (void)offMinutes;
}




bool Power_Heat, Power_Heat1;
bool Activation_Heat, Activation_Heat1; // Включение и Активация контроля включения нагрева воды
int Sider_heat,  Sider_heat1; 			// Sider_heat1; bool Sider_Heat; // Переменная для получения или передачи из в  Nextion c  сидера монитора уставки нагрева воды

bool RoomTemper = false;
float RoomTempOn = 3.0f;
float RoomTempOff = 4.0f;
bool Power_Warm_floor_heating = false;
//float temper1; int set_temper1;  //Для котроля измениения
//bool Relay4_Time1, Relay4_Time2; // Разрешения работы включения по времени
//String Relay4_timeON1, Relay4_timeOFF1, Relay4_timeON2, Relay4_timeOFF2; // Утавки времени включения
	
bool Pow_Ul_light, Pow_Ul_light1; // Промывка фильтра
bool Ul_light_Time, Saved_Ul_light_Time; // Разрешения работы включения по времени
String Ul_light_timeON, Ul_light_timeOFF; // Утавки времени включения
// String Saved_Ul_light_timeON, Saved_Ul_light_timeOFF;

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
//bool Saved_Power_APF, Power_APF = false;		//Высокоэффективный коагулянт и флокулянт «All Poly Floc» APF

// bool Saved_Test_Pump, Test_Pump; //Разрешение на работу тамера дозации ACO
// bool Saved_Activation_Timer_H2O2, Activation_Timer_H2O2;//Разрешение на работу тамера дозации H2O2
//bool Saved_Activation_Timer_APF, Activation_Timer_APF; //Разрешение на работу тамера дозации APF

//String Info_H2O2_H2O2_APF;  //Таймера включения дозаторов  H2O2, H2O2, APF

// String Saved_Timer_H2O2_Start, Timer_H2O2_Start;
// String Saved_Timer_H2O2_Work, Timer_H2O2_Work;

// String Saved_Timer_ACO_Start, Timer_ACO_Start;
// String Saved_Timer_ACO_Work, Timer_ACO_Work;

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

//Получение по ModbusRTU Объявляем переменные Input1, ..., Input16
//bool Input1, Input2, Input3, Input4, Input5, Input6, Input7, Input8, Input9, Input10, Input11, Input12, Input13, Input14, Input15, Input16;

















extern void interface();
String uiValueForId(const String &id);
bool uiApplyValueForId(const String &id, const String &value);

// Безопасное экранирование строк для JSON ответов
String jsonEscape(const String &input){
  String output;
  output.reserve(input.length() + 8);
  for(size_t i = 0; i < input.length(); i++){
    char c = input.charAt(i);
    switch(c){
      case '\\': output += "\\\\"; break;
      case '\"': output += "\\\""; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      default:
        if(static_cast<uint8_t>(c) < 0x20){
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<uint8_t>(c));
          output += buf;
        } else {
          output += c;
        }
    }
  }
  return output;
}

String chipSeriesName(const esp_chip_info_t &info){
  switch(info.model){
    case CHIP_ESP32: return "ESP32";
    case CHIP_ESP32S2: return "ESP32-S2";
    case CHIP_ESP32S3: return "ESP32-S3";
    case CHIP_ESP32C3: return "ESP32-C3";
#ifdef CHIP_ESP32C2
    case CHIP_ESP32C2: return "ESP32-C2";
#endif
#ifdef CHIP_ESP32C6
    case CHIP_ESP32C6: return "ESP32-C6";
#endif
    case CHIP_ESP32H2: return "ESP32-H2";
    default: return "ESP32";
  }
}

String buildChipIdentity(){
  esp_chip_info_t info;
  esp_chip_info(&info);

  const String series = chipSeriesName(info);
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "%s rev %d", series.c_str(), info.revision);
  return String(buffer);
}

inline bool readPsramStats(uint32_t &freePsram, uint32_t &totalPsram){
#if defined(ARDUINO_ARCH_ESP32)
  if(psramFound()){
    freePsram = ESP.getFreePsram();
    totalPsram = ESP.getPsramSize();
    return true;
  }
#endif
  freePsram = 0;
  totalPsram = 0;
  return false;
}

inline bool isAuthConfigured(){
  return authUsername.length() > 0 && authPassword.length() > 0;
}

inline bool ensureAuthorized(AsyncWebServerRequest *request){
  if(!isAuthConfigured()) return true;
  if(!request->authenticate(authUsername.c_str(), authPassword.c_str())){
    request->requestAuthentication();
    return false;
  }
  return true;
}

inline bool isAdminAuthConfigured(){
  return adminUsername.length() > 0 && adminPassword.length() > 0;
}

inline bool ensureAdminAuthorized(AsyncWebServerRequest *request){
  if(!isAdminAuthConfigured()) return true;
  if(!request->authenticate(adminUsername.c_str(), adminPassword.c_str())){
    request->requestAuthentication();
    return false;
  }
  return true;
}


// ---------- Структуры UI ----------
struct Tab { String id; String title; };
struct Element { String type; String id; String label; String value; String tab; };

struct Popup { String id; String title; String tabId; };



// ---------- Класс MiniDash ----------
class MiniDash {
public:
    vector<Tab> tabs;         // Список вкладок
    vector<Element> elements; // Список элементов UI
        vector<Popup> popups;
  void addTab(const String &id, const String &title){ tabs.push_back({id,title}); }
  void addElement(const String &tab, const Element &e){ Element x=e; x.tab=tab; elements.push_back(x); }
  
    void addPopup(const String &id, const String &title, const String &tabId){
    for(auto &popup : popups){
      if(popup.id == id) return;
    }
    popups.push_back({id, title, tabId});
  }

  void begin(){
    if(!dashInterfaceInitialized){
      interface();
      dashInterfaceInitialized = true;
    }
    setupServer();
  }

private:
// Настройка веб-сервера
  void setupServer(){
    MiniDash *self=this;

    server.on("/", HTTP_GET, [self](AsyncWebServerRequest *r){
      if(!ensureAuthorized(r)) return;
       // Формируем HTML-страницу
      String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'><title>";
      html += dashAppTitle;
      html += "</title>"
      "<style>"
      "body{margin:0;background:"+ThemeColor+";color:#ddd;font-family:'Inter', 'Segoe UI', 'Roboto', Arial, sans-serif;} "
      "#sidebar{width:230px;position:fixed;left:0;top:0;background:#151515;height:100vh;padding:10px;box-sizing:border-box;transition:all 0.3s;overflow:auto;box-shadow:2px 0 12px rgba(0,0,0,0.45);} "
      "#sidebar.collapsed{width:0;padding:0;overflow:hidden;opacity:0;pointer-events:none;} "
      "#main{margin-left:230px;padding:20px;overflow:auto;height:100vh;box-sizing:border-box;transition:margin-left 0.3s;} "
      "body.sidebar-hidden #main{margin-left:20px;} "
      "#toggleBtn{position:fixed;top:10px;left:230px;width:36px;height:48px;background:rgba(255,255,255,0.12);border:none;border-radius:0 6px 6px 0;cursor:pointer;z-index:1000;transition:left 0.3s, opacity 0.3s, background 0.3s;box-shadow:2px 2px 10px rgba(0,0,0,0.4);display:flex;align-items:center;justify-content:center;} "
      "#toggleBtn::before{content:'';width:22px;height:22px;display:block;opacity:0.7;background-repeat:no-repeat;background-position:center;background-size:contain;background-image:url(\"data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' stroke='white' stroke-width='1.6' stroke-linecap='round' stroke-linejoin='round'><path d='M3 5.5c0-1.1.9-2 2-2h6c1.7 0 3 1.3 3 3v13c0-1.7-1.3-3-3-3H5c-1.1 0-2-.9-2-2V5.5z'/><path d='M21 5.5c0-1.1-.9-2-2-2h-6c-1.7 0-3 1.3-3 3v13c0-1.7 1.3-3 3-3h6c1.1 0 2-.9 2-2V5.5z'/></svg>\");} "
      "body.sidebar-hidden #toggleBtn{left:16px;opacity:0.65;} "
      "#sidebar button{width:100%;padding:10px;margin:6px 0;border:none;border-radius:8px;background:#2a2a2a;color:#ccc;text-align:left;cursor:pointer;font-weight:600;transition:0.2s;} "
      "#sidebar button.active{background:#4CAF50;color:#fff;} "
      ".card{background:#1d1d1f;padding:14px;border-radius:14px;margin-bottom:14px;box-shadow:0 4px 14px rgba(0,0,0,0.45);border:1px solid rgba(255,255,255,0.06);display:flex;flex-direction:column;gap:6px;} "
      ".card.compact{padding:10px 12px;margin-bottom:10px;} "
      ".card.compact label{font-size:0.78em;color:#b0b0b0;margin-bottom:2px;text-transform:uppercase;letter-spacing:0.3px;} "
      ".card.compact input,.card.compact select,.card.compact .display-value{font-size:0.95em;padding:6px 8px;border-radius:8px;background:#111;border:1px solid #262626;color:#f6f6f6;} "
      ".card.compact .display-value{background:transparent;border:none;padding:2px 0;} "
".select-days{--day-accent:#4cc3ff;display:flex;flex-wrap:nowrap;gap:8px;"
      "justify-content:center;align-items:center;padding:8px;"
      "background:linear-gradient(135deg,rgba(255,255,255,0.06),rgba(255,255,255,0.02));"
      "border-radius:14px;border:1px solid rgba(255,255,255,0.12);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 10px 24px rgba(0,0,0,0.45);"
      "overflow-x:auto;scrollbar-width:thin;scrollbar-color:rgba(255,255,255,0.2) transparent;} "
      ".select-days::-webkit-scrollbar{height:6px;} "
      ".select-days::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.2);border-radius:999px;} "
      ".select-days .day-pill{position:relative;display:inline-flex;align-items:center;justify-content:center;"
      "min-width:42px;height:32px;padding:0 12px;border-radius:999px;"
      "font-size:0.78em;font-weight:600;letter-spacing:0.08em;text-transform:uppercase;"
      "color:#b7c0ce;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.1);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 6px 14px rgba(0,0,0,0.4);"
      "cursor:pointer;transition:transform 0.18s ease,box-shadow 0.18s ease,background 0.18s ease,color 0.18s ease;} "
      ".select-days .day-pill input{position:absolute;opacity:0;pointer-events:none;} "
      ".select-days .day-pill:hover{transform:translateY(-1px) scale(1.05);"
      "box-shadow:0 10px 18px rgba(0,0,0,0.45);} "
      ".select-days .day-pill:active{transform:translateY(0) scale(0.98);} "
      ".select-days .day-pill:has(input:checked){color:#fff;"
      "background:linear-gradient(135deg,rgba(76,195,255,0.3),rgba(76,195,255,0.12));"
      "border-color:rgba(76,195,255,0.6);"
      "box-shadow:0 0 0 1px rgba(76,195,255,0.35),0 12px 26px rgba(76,195,255,0.35);} "
      ".select-days .day-pill:has(input:checked)::after{content:'';position:absolute;inset:-6px;"
      "border-radius:inherit;background:radial-gradient(circle,rgba(76,195,255,0.35),transparent 65%);"
      "opacity:0.75;filter:blur(6px);z-index:0;} "
      ".select-days .day-pill span{position:relative;z-index:1;} "
      "@media (max-width:520px){"
      ".select-days{gap:6px;padding:6px;}"
      ".select-days .day-pill{min-width:36px;height:30px;padding:0 10px;font-size:0.74em;}"
      "} "
      ".select-days.compact{gap:6px;padding:6px;background:#141416;border:1px solid #242424;} "
      "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:center;} "
      "th{background:#333;} "
      ".dash-btn{display:inline-block;margin-top:6px;padding:8px 16px;border-radius:10px;border:1px solid rgba(255,255,255,0.18);background:#222;color:#ddd;font-weight:600;cursor:pointer;box-shadow:0 4px 10px rgba(0,0,0,0.35);transition:transform 0.15s ease, box-shadow 0.15s ease, background 0.15s ease, color 0.15s ease;letter-spacing:0.03em;text-transform:uppercase;font-size:0.78rem;} "
      ".dash-btn.on{background:linear-gradient(135deg,#3a7bd5,#00d2ff);color:#fff;} "
      ".dash-btn.off{background:#222;color:#ddd;opacity:0.9;} "
      ".dash-btn:hover{transform:translateY(-1px);box-shadow:0 6px 14px rgba(0,0,0,0.45);} "
                ".page{display:none;position:relative;grid-template-columns: repeat(auto-fill, minmax(250px, 1fr)); gap:15px;} "
      ".page.active{display:block;} "
            ".page-header{display:flex;flex-direction:row;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:10px;margin-bottom:10px;} "
      ".page-header h3{margin:0;} "
             ".page-datetime{font-size:clamp(0.95em, 1.6vw, 1.25em);letter-spacing:0.08em;text-align:right;font-weight:600;"
      "margin-left:auto;display:inline-flex;align-items:center;justify-content:center;max-width:100%;"
      "padding:6px 12px;border-radius:12px;background:rgba(0,0,0,0.55);border:1px solid rgba(255,255,255,0.18);"
      "box-shadow:0 6px 14px rgba(0,0,0,0.4);"
      "color:#fff;text-shadow:0 0 10px rgba(0,0,0,0.55),0 1px 1px rgba(0,0,0,0.9);} "
      
            ".timer-card{gap:12px;}"
      ".timer-card__header{font-size:0.85em;text-transform:uppercase;letter-spacing:0.08em;color:#9fb4c8;font-weight:600;}"
      ".timer-card__grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:14px;}"
      ".timer-card__column{display:flex;flex-direction:column;gap:6px;}"
      ".timer-card__column label{margin-bottom:0;font-size:0.78em;letter-spacing:0.08em;text-transform:uppercase;color:#c1d0e2;}"
      ".timer-card input[type=time]{width:100%;margin:0;}"
      "@media (max-width:640px){.timer-card__grid{grid-template-columns:1fr;}} "
            
      "@keyframes rainbow-shift{0%{background-position:0% 50%;}100%{background-position:100% 50%;}} "
      "label{display:block;margin-bottom:5px;font-size:0.9em;} "
      "input,select{width:100%;padding:7px;margin-bottom:10px;background:#111;border:1px solid #333;color:#ddd;border-radius:6px;} "
      "input[type=time]{width:30%;margin-right:auto;} "
      "input[type=checkbox]{width:auto;margin-right:auto;} "
      "label:has(input[type=checkbox]){display:flex;flex-direction:column;align-items:flex-start;gap:6px;width:fit-content;} "
      "label:has(input[type=checkbox]) input[type=checkbox]{margin-bottom:0;transform:scale(1.4);transform-origin:left center;} "
      "input[type=range]{width:100%;} "
   ".select-days{--day-accent:#4cc3ff;display:flex;flex-wrap:nowrap;gap:8px;"
      "justify-content:center;align-items:center;padding:8px;"
      "background:linear-gradient(135deg,rgba(255,255,255,0.06),rgba(255,255,255,0.02));"
      "border-radius:14px;border:1px solid rgba(255,255,255,0.12);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 10px 24px rgba(0,0,0,0.45);"
      "overflow-x:auto;scrollbar-width:thin;scrollbar-color:rgba(255,255,255,0.2) transparent;} "
      ".select-days::-webkit-scrollbar{height:6px;} "
      ".select-days::-webkit-scrollbar-thumb{background:rgba(255,255,255,0.2);border-radius:999px;} "
      ".select-days .day-pill{position:relative;display:inline-flex;align-items:center;justify-content:center;"
      "min-width:42px;height:32px;padding:0 12px;border-radius:999px;"
      "font-size:0.78em;font-weight:600;letter-spacing:0.08em;text-transform:uppercase;"
      "color:#b7c0ce;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.1);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.08),0 6px 14px rgba(0,0,0,0.4);"
      "cursor:pointer;transition:transform 0.18s ease,box-shadow 0.18s ease,background 0.18s ease,color 0.18s ease;} "
      ".select-days .day-pill input{position:absolute;opacity:0;pointer-events:none;} "
      ".select-days .day-pill:hover{transform:translateY(-1px) scale(1.05);"
      "box-shadow:0 10px 18px rgba(0,0,0,0.45);} "
      ".select-days .day-pill:active{transform:translateY(0) scale(0.98);} "
      ".select-days .day-pill:has(input:checked){color:#fff;"
      "background:linear-gradient(135deg,rgba(76,195,255,0.3),rgba(76,195,255,0.12));"
      "border-color:rgba(76,195,255,0.6);"
      "box-shadow:0 0 0 1px rgba(76,195,255,0.35),0 12px 26px rgba(76,195,255,0.35);} "
      ".select-days .day-pill:has(input:checked)::after{content:'';position:absolute;inset:-6px;"
      "border-radius:inherit;background:radial-gradient(circle,rgba(76,195,255,0.35),transparent 65%);"
      "opacity:0.75;filter:blur(6px);z-index:0;} "
      ".select-days .day-pill span{position:relative;z-index:1;} "
      "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:center;} "
      "th{background:#333;} "
      ".card.pro-card{background:linear-gradient(135deg,#0f0f12,#151a2d);border:1px solid rgba(129,193,255,0.4);} "
      ".card.pro-card label{color:#9ae7ff;text-transform:uppercase;letter-spacing:0.08em;font-size:0.75em;} "
      ".card.pro-card input,.card.pro-card select{background:#090c10;border-color:#252f40;color:#eff6ff;} "
      ".graph-controls{display:flex;flex-direction:row;flex-wrap:nowrap;gap:18px;align-items:center;} "
      ".graph-controls .control-group{flex:1;min-width:220px;display:flex;align-items:center;gap:10px;} "
      ".graph-controls label{display:inline-flex;font-size:0.72em;color:#9fb4c8;letter-spacing:0.08em;text-transform:uppercase;margin-bottom:0;white-space:nowrap;} "
      ".graph-controls select,.graph-controls input{margin-bottom:0;flex:1;} "
 ".graph-card{position:relative;}" 
      ".graph-heading{text-align:center;}" 
      ".dash-graph{width:100%;background:#05070a;}" 
      ".graph-axes{position:absolute;inset:0;pointer-events:none;font-size:0.85em;color:#cbd4df;}" 
      ".graph-axes .axis-name{position:absolute;}" 
      ".graph-axes .axis-name:first-child{right:10px;bottom:8px;}" 
      ".graph-axes .axis-name:last-child{left:8px;top:8px;writing-mode:vertical-rl;transform:rotate(180deg);}" 
      ".graph-tooltip{position:fixed;pointer-events:none;background:rgba(10,14,20,0.9);color:#eef4ff;padding:6px 10px;border:1px solid rgba(255,255,255,0.18);border-radius:6px;font-size:0.8em;z-index:1000;transform:translate(-50%,-120%);white-space:nowrap;box-shadow:0 4px 12px rgba(0,0,0,0.35);}" 
      ".graph-tooltip.hidden{display:none;}" 
      ".card:has(#ModeSelect),.card:has(#LEDColor),.card:has(#Timer1)," 
      ".card:has(#FloatInput),.card:has(#IntInput),"
      ".card:has(#RandomVal),.card:has(#DaysSelect){"
      "background:linear-gradient(145deg,#111,#080b13);border:1px solid rgba(159,180,255,0.25);"
      "box-shadow:0 18px 34px rgba(0,0,0,0.65);border-radius:18px;padding:14px 16px;"
      "transition:transform 0.18s ease,box-shadow 0.18s ease;"
      "position:relative;overflow:hidden;"
      "} "
      ".card:has(#ModeSelect):hover,.card:has(#LEDColor):hover,.card:has(#Timer1):hover,"
        ".card:has(#FloatInput) label,.card:has(#IntInput) label,"
      ".card:has(#RandomVal):hover,.card:has(#DaysSelect):hover{"
      "transform:translateY(-2px);box-shadow:0 22px 40px rgba(0,0,0,0.7);}"
      ".card:has(#ModeSelect) label,.card:has(#LEDColor) label,.card:has(#Timer1) label,"
      ".card:has(#FloatInput) label,.card:has(#IntInput) label,.card:has(#CurrentTime) label,"
      ".card:has(#RandomVal) label,.card:has(#DaysSelect) label{"
      "font-size:0.72em;letter-spacing:0.08em;text-transform:uppercase;color:#94b4d6;"
      "} "
      ".card:has(#ModeSelect) select,.card:has(#LEDColor) input,"
      ".card:has(#Timer1) input,.card:has(#FloatInput) input,.card:has(#IntInput) input{"
      "background:#06070c;border:1px solid rgba(255,255,255,0.12);border-radius:10px;"
      "color:#eef2ff;font-weight:600;padding:10px 12px;box-shadow:inset 0 0 0 1px rgba(255,255,255,0.03);"
      "width:100%;} "

      ".card:has(#LEDColor){"
      "--led-color:#33b8ff;"
      "background:linear-gradient(135deg,rgba(15,18,35,0.98),rgba(11,16,28,0.98));"
      "border:1px solid rgba(110,173,255,0.35);"
      "box-shadow:0 16px 32px rgba(0,0,0,0.7),0 0 0 1px rgba(255,255,255,0.05);"
      "padding:16px 18px;gap:10px;"
      "}"
      ".card:has(#LEDColor)::before{"
      "content:'';position:absolute;inset:8px;"
      "background:radial-gradient(circle at 30% 25%,rgba(255,255,255,0.08),transparent 55%);"
      "pointer-events:none;z-index:0;"
      "}"
      ".card:has(#LEDColor)::after{"
      "content:'';position:absolute;inset:-50%;"
      "background:radial-gradient(circle at 50% 50%,var(--led-color),transparent 50%);"
      "opacity:0.25;filter:blur(32px);pointer-events:none;z-index:0;"
      "}"
      ".card:has(#LEDColor) label{"
      "display:flex;align-items:center;justify-content:space-between;gap:10px;"
      "letter-spacing:0.09em;color:#b4d6ff;position:relative;z-index:1;"
      "}"
      ".card:has(#LEDColor) label::after{"
      "content:attr(data-color);font-family:'JetBrains Mono','SFMono-Regular',monospace;"
      "font-size:0.82em;padding:4px 9px;border-radius:9px;"
      "background:rgba(255,255,255,0.06);color:#eaf4ff;"
      "box-shadow:inset 0 0 0 1px rgba(255,255,255,0.08);"
      "}"
      "#sidebar .card:has(#ThemeColor){"
      "--theme-color:#6dd5ed;"
      "background:linear-gradient(145deg,rgba(16,20,32,0.96),rgba(10,14,24,0.96));"
      "border:1px solid rgba(126,193,255,0.32);"
      "box-shadow:0 14px 26px rgba(0,0,0,0.65),0 0 0 1px rgba(255,255,255,0.05);"
      "padding:14px 16px;position:relative;overflow:hidden;gap:8px;"
      "}"
      "#sidebar .card:has(#ThemeColor)::before{"
      "content:'';position:absolute;inset:10px;"
      "background:radial-gradient(circle at 22% 30%,rgba(255,255,255,0.08),transparent 55%);"
      "pointer-events:none;"
      "}"
      "#sidebar .card:has(#ThemeColor)::after{"
      "content:'';position:absolute;inset:-55%;"
      "background:radial-gradient(circle at 50% 45%,var(--theme-color),transparent 52%);"
      "opacity:0.2;filter:blur(28px);pointer-events:none;"
      "}"
      "#sidebar .card:has(#ThemeColor) label{"
      "display:flex;align-items:center;justify-content:space-between;gap:10px;"
      "letter-spacing:0.09em;color:#b7dbff;font-size:0.72em;text-transform:uppercase;"
      "position:relative;z-index:1;"
      "}"
      "#sidebar .card:has(#ThemeColor) label::after{"
      "content:attr(data-color);font-family:'JetBrains Mono','SFMono-Regular',monospace;"
      "font-size:0.78em;padding:4px 8px;border-radius:9px;"
      "background:rgba(255,255,255,0.06);color:#eaf4ff;"
      "box-shadow:inset 0 0 0 1px rgba(255,255,255,0.08);"
      "}"
      "#ThemeColor{"
      "height:50px;padding:0 10px;border-radius:12px;"
      "background:linear-gradient(145deg,#0c101b,#10182a);"
      "border:1px solid rgba(255,255,255,0.16);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.1),0 12px 22px rgba(0,0,0,0.52);"
      "cursor:pointer;position:relative;z-index:1;"
      "}"
      "#ThemeColor::-webkit-color-swatch-wrapper{padding:6px;border-radius:10px;}"
      "#ThemeColor::-webkit-color-swatch{border-radius:9px;border:1px solid rgba(255,255,255,0.16);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.22);}"
      "#ThemeColor::-moz-color-swatch{border-radius:9px;border:1px solid rgba(255,255,255,0.16);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.22);}"
      "#LEDColor{"
      "height:58px;padding:0 10px;border-radius:14px;"
      "background:linear-gradient(145deg,#0b101a,#101727);"
      "border:1px solid rgba(255,255,255,0.18);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.12),0 14px 26px rgba(0,0,0,0.55);"
      "cursor:pointer;position:relative;z-index:1;"
      "}"
      "#LEDColor::-webkit-color-swatch-wrapper{padding:6px;border-radius:12px;}"
      "#LEDColor::-webkit-color-swatch{border-radius:10px;border:1px solid rgba(255,255,255,0.18);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.25);}"
      "#LEDColor::-moz-color-swatch{border-radius:10px;border:1px solid rgba(255,255,255,0.18);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.25);}"

       ".card:has(#RandomVal) #RandomVal{"
      "font-size:1.5em;font-weight:700;color:#ffffff;text-shadow:0 4px 12px rgba(0,0,0,0.45);margin-top:6px;"
      "} "
      // ".card:has(#DaysSelect) .select-days{gap:8px;padding:8px 6px;background:rgba(255,255,255,0.02);"
      // "border-radius:12px;border:1px solid rgba(255,255,255,0.12);box-shadow: inset 0 0 0 1px rgba(255,255,255,0.02);}"
      // ".card:has(#DaysSelect) .select-days label{border:1px solid rgba(255,255,255,0.08);"
      // "padding:5px 10px;border-radius:9px;background:rgba(255,255,255,0.02);transition:background 0.2s ease,color 0.2s ease;}"
      // ".card:has(#DaysSelect) .select-days input[type=checkbox]{accent-color:#4CAF50;}"
            ".card:has(#DaysSelect) .select-days{--day-accent:#4cc3ff;}"
      ".stats-card{display:flex;flex-direction:column;gap:14px;} "
      ".stat-group{display:flex;flex-direction:column;gap:8px;} "
      ".stat-heading{font-size:0.82em;color:#9fb4c8;letter-spacing:0.05em;text-transform:uppercase;padding-left:2px;} "
      ".stat-list{list-style:none;padding:0;margin:0;display:flex;flex-direction:column;gap:8px;} "
      ".stat-list li{display:flex;justify-content:space-between;align-items:center;padding:8px 10px;border-radius:10px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.06);} "
      ".stat-list span{color:#9fb4c8;font-size:0.9em;} "
      ".stat-list strong{color:#fff;font-family:'JetBrains Mono','SFMono-Regular',monospace;font-size:0.95em;} "
      ".btn-primary{display:inline-flex;align-items:center;gap:8px;padding:10px 18px;border-radius:12px;border:1px solid rgba(255,255,255,0.12);background:linear-gradient(135deg,#3a7bd5,#00d2ff);color:#fff;font-weight:700;letter-spacing:0.03em;text-transform:uppercase;box-shadow:0 12px 26px rgba(0,0,0,0.35),0 0 0 1px rgba(255,255,255,0.08);cursor:pointer;transition:transform 0.12s ease,box-shadow 0.12s ease;} "
      ".btn-primary:hover{transform:translateY(-1px);box-shadow:0 16px 30px rgba(0,0,0,0.45);} "
      ".btn-secondary{padding:8px 14px;border-radius:10px;border:1px solid rgba(255,255,255,0.12);background:rgba(255,255,255,0.05);color:#e2e6f0;font-weight:600;cursor:pointer;transition:background 0.15s ease,transform 0.12s ease;} "
      ".btn-secondary:hover{background:rgba(255,255,255,0.1);transform:translateY(-1px);} "
      ".btn-secondary:disabled{opacity:0.6;cursor:progress;} "
      ".btn-danger{padding:8px 14px;border-radius:10px;border:1px solid rgba(255,87,87,0.25);background:linear-gradient(135deg,#ff5f6d,#ffc371);color:#0c0f16;font-weight:700;cursor:pointer;box-shadow:0 8px 18px rgba(255,95,109,0.35);transition:transform 0.12s ease,box-shadow 0.12s ease;} "
      ".btn-danger:hover{transform:translateY(-1px);box-shadow:0 12px 26px rgba(255,95,109,0.45);} "
      ".btn-danger:disabled{opacity:0.65;cursor:progress;} "
      ".stats-actions{display:flex;gap:10px;flex-wrap:wrap;margin-bottom:10px;} "
      ".wifi-card{display:flex;flex-direction:column;gap:10px;} "
      ".wifi-field label{margin-bottom:6px;font-size:0.85em;color:#9fb4c8;text-transform:uppercase;letter-spacing:0.06em;} "
      ".input-with-action{display:flex;gap:8px;align-items:center;} "
      ".wifi-actions{display:flex;align-items:center;gap:14px;margin-top:10px;flex-wrap:wrap;} "
      ".wifi-hint{color:#9fb4c8;font-size:0.9em;} "
      ".wifi-status-card .stat-list li{background:rgba(0,0,0,0.3);} "
      ".section-title{margin-top:10px;margin-bottom:6px;font-size:1em;color:#cfd7e0;} "
      ".wifi-modal{position:fixed;inset:0;background:rgba(0,0,0,0.55);display:flex;align-items:flex-start;justify-content:center;padding-top:60px;z-index:1200;} "
      ".wifi-modal.hidden{display:none;} "
      ".wifi-modal-content{width:min(480px,90vw);background:#1a1c22;border:1px solid rgba(255,255,255,0.08);border-radius:14px;box-shadow:0 18px 40px rgba(0,0,0,0.65);overflow:hidden;} "
      ".wifi-modal-header{display:flex;align-items:center;justify-content:space-between;padding:12px 14px;border-bottom:1px solid rgba(255,255,255,0.06);} "
      ".wifi-scan-list{max-height:320px;overflow:auto;display:flex;flex-direction:column;} "
      
            ".dash-modal{position:fixed;inset:0;background:rgba(0,0,0,0.6);display:flex;align-items:flex-start;justify-content:center;padding:60px 20px;z-index:1400;} "
      ".dash-modal.hidden{display:none;} "
      ".dash-modal-content{width:min(880px,95vw);background:#1a1c22;border:1px solid rgba(255,255,255,0.08);border-radius:16px;box-shadow:0 20px 50px rgba(0,0,0,0.7);overflow:hidden;} "
      ".dash-modal-header{display:flex;align-items:center;justify-content:space-between;padding:14px 16px;border-bottom:1px solid rgba(255,255,255,0.08);} "
      ".dash-modal-body{padding:16px;max-height:calc(100vh - 180px);overflow:auto;} "
      ".popup-grid{display:flex;flex-direction:column;gap:15px;position:relative;width:100%;} "
      ".popup-grid .card{width:100%;} "
      
      ".network-row{display:flex;flex-direction:column;align-items:flex-start;gap:4px;padding:12px 14px;background:transparent;border:none;border-bottom:1px solid rgba(255,255,255,0.05);color:#e9ecf4;text-align:left;cursor:pointer;transition:background 0.12s ease;} "
      ".network-row:hover{background:rgba(255,255,255,0.04);} "
      ".network-ssid{font-weight:700;} "
      ".network-meta{color:#9fb4c8;font-size:0.9em;} "
      ".empty-row{padding:16px;color:#9fb4c8;text-align:center;} "
      ".icon-btn{background:none;border:none;color:#fff;font-size:1.4em;cursor:pointer;line-height:1;} "

      ".mqtt-grid{display:flex;flex-direction:column;gap:12px;} "
      ".mqtt-field{display:flex;flex-direction:column;gap:6px;} "
      ".mqtt-actions{display:flex;flex-wrap:wrap;gap:10px;align-items:center;margin-top:8px;} "
      ".profile-hint{color:#9fb4c8;font-size:0.9em;} "
      ".btn-toggle-on{background:linear-gradient(135deg,#4caf50,#81c784);color:#0b0f14;} "
      ".btn-mqtt{position:relative;overflow:hidden;background:linear-gradient(135deg,#1f2a44,#263555);border:1px solid rgba(111,168,255,0.35);color:#e6edff;box-shadow:0 12px 24px rgba(0,0,0,0.4);} "
      ".btn-mqtt:before{content:'';position:absolute;inset:0;opacity:0;pointer-events:none;background:radial-gradient(circle at 20% 20%,rgba(255,255,255,0.28),transparent 45%);transition:opacity 0.18s ease;} "
      ".btn-mqtt:hover:before{opacity:1;} "
      ".btn-mqtt.btn-warn{background:linear-gradient(135deg,#2d1e12,#3c2a18);border-color:rgba(255,193,7,0.45);color:#ffe9b3;} "
      ".btn-mqtt.btn-success{background:linear-gradient(135deg,#123420,#1d4b2a);border-color:rgba(76,175,80,0.55);color:#d5ffde;} "
      ".btn-mqtt.btn-activate-off{background:linear-gradient(135deg,#1b2b52,#23406f);border-color:rgba(64,139,255,0.55);color:#e5efff;} "
      ".btn-mqtt.btn-activate-on{background:linear-gradient(135deg,#0f3b1f,#13532a);border-color:rgba(76,175,80,0.6);color:#d8ffe4;} "
   
      "</style></head><body>";

      // Sidebar
      html += "<div id='sidebar'>";
      bool first = true;
      for(auto &t : self->tabs){
        html += "<button onclick=\"showPage('"+t.id+"',this)\"";
        if(first){ html += " class='active'"; first=false; }
        html += ">"+t.title+"</button>";
      }
      html += "<hr><div class='card'><label>Theme color</label>"
              "<input id='ThemeColor' type='color' value='"+ThemeColor+"'></div>";
      html += "<button onclick=\"showPage('wifi',this)\">WiFi Settings</button>";
      html += "<button onclick=\"showPage('stats',this)\">Statistics</button>";
            html += "<button onclick=\"showPage('profile',this)\">Профиль</button>";
      html += "<button onclick=\"showPage('mqtt',this)\">Настройка MQTT</button>";
      html += "</div>";

      html += "<button id='toggleBtn' onclick='toggleSidebar()'>?</button>";

      // Основной контент
      html += "<div id='main'>";

      auto renderTabElements = [&](const String &tabId){
        for(auto &e : self->elements){
              if(e.tab != tabId || e.type != "image") continue;

              String imgSrc = e.label;
              if(!imgSrc.startsWith("http") && !imgSrc.startsWith("/")) imgSrc = "/" + imgSrc;
              if(imgSrc == "/getImage") imgSrc = "/getImage";

              String widthRaw, heightRaw, leftRaw, topRaw, extraStyles;

              auto ensureUnit = [&](const String &raw)->String {
                  String trimmed = raw;
                  trimmed.trim();
                  if(trimmed.length() == 0) return trimmed;
                  bool hasUnit = false;
                  for(int i=0;i<trimmed.length();i++){
                      char c = trimmed[i];
                      if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                  }
                  return hasUnit ? trimmed : trimmed + "px";
              };

              String style = e.value;
              if(style.length() && style[style.length()-1] != ';') style += ';';
              int tokenStart = 0;
              while(tokenStart < style.length()){
                  int tokenEnd = style.indexOf(';', tokenStart);
                  if(tokenEnd < 0) tokenEnd = style.length();
                  String token = style.substring(tokenStart, tokenEnd);
                  token.trim();
                  if(token.length()){
                      int sep = token.indexOf(':');
                      if(sep > 0){
                          String key = token.substring(0, sep); key.trim();
                          String val = token.substring(sep + 1); val.trim();
                          if(key.equalsIgnoreCase("width")) widthRaw = ensureUnit(val);
                          else if(key.equalsIgnoreCase("height")) heightRaw = ensureUnit(val);
                          else if(key.equalsIgnoreCase("left")) leftRaw = ensureUnit(val);
                          else if(key.equalsIgnoreCase("top")) topRaw = ensureUnit(val);
                          else extraStyles += key + ":" + val + ";";
                      } else {
                          extraStyles += token + ";";
                      }
                  }
                  tokenStart = tokenEnd + 1;
              }

              bool hasCoords = leftRaw.length() || topRaw.length();
              bool positionProvided = extraStyles.indexOf("position:") >= 0;

              String imageStyle = "display:block; width:auto; height:auto; border-radius:12px; box-shadow:0 4px 12px rgba(0,0,0,0.5);";
              imageStyle += widthRaw.length() ? "width:"+widthRaw+";" : "max-width:100%;";
              imageStyle += heightRaw.length() ? "height:"+heightRaw+";" : "height:auto;";
              imageStyle += extraStyles;
              if(!positionProvided) imageStyle += hasCoords ? "position:absolute;" : "position:relative;";
              imageStyle += " z-index:1; object-fit:contain;";

              String containerStyle = "position:relative; text-align:center; display:inline-flex; align-items:center; justify-content:center;"
                                       " padding:0; width:fit-content; height:fit-content; background:transparent; border:none; box-shadow:none; margin:0 auto 14px auto;";
              if(widthRaw.length()) containerStyle += " width:"+widthRaw+";";
              if(heightRaw.length()) containerStyle += " height:"+heightRaw+";";
              if(hasCoords){
                  containerStyle += " position:absolute;";
                  if(leftRaw.length()) containerStyle += " left:"+leftRaw+";";
                  if(topRaw.length()) containerStyle += " top:"+topRaw+";";
                  containerStyle += " margin:0;";
              }

              html += "<div class=\"card\" style=\""+containerStyle+"\">";
              html += "<img id='"+e.id+"' src='"+imgSrc+"' data-refresh='"+(imgSrc=="/getImage"?"getImage":"")+"' "
                      "style='"+imageStyle+"'/>";

       html += "</div>";
          }

          for(auto &overlay : self->elements){
              if(overlay.tab != tabId || overlay.type != "displayStringAbsolute") continue;
              String styleStr = overlay.value;
              auto readProp = [&](const String &prop)->String {
                  String key = prop + ":";
                  int idx = styleStr.indexOf(key);
                  if(idx < 0) return "";
                  int start = idx + key.length();
                  int end = styleStr.indexOf(";", start);
                  if(end < 0) end = styleStr.length();
                  return styleStr.substring(start, end);
              };

              auto normalizeCoord = [&](const String &raw)->String {
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

              String fontSizeStr = readProp("fontSize");
              int fontSize = fontSizeStr.length() ? fontSizeStr.toInt() : 24;
              String color = readProp("color"); if(color.length() == 0) color = "#00ff00";
              String bgColor = readProp("bg"); if(bgColor.length() == 0) bgColor = "rgba(0,0,0,0.65)";
              String padding = readProp("padding"); if(padding.length() == 0) padding = "6px 12px";
              String borderRadius = readProp("borderRadius"); if(borderRadius.length() == 0) borderRadius = "14px";
              String leftRaw = readProp("x");
              String topRaw = readProp("y");
              bool hasLeft = leftRaw.length();
              bool hasTop = topRaw.length();
              String leftValue = hasLeft ? normalizeCoord(leftRaw) : "50%";
              String topValue = hasTop ? normalizeCoord(topRaw) : "50%";
              String translateX = hasLeft ? "0%" : "-50%";
              String translateY = hasTop ? "0%" : "-50%";
              String transform = "translate("+translateX+", "+translateY+")";
              String panelStyle = "position:absolute; left:"+leftValue+"; top:"+topValue+"; transform:"+transform+"; "
                                   "background:"+bgColor+"; color:"+color+"; font-size:"+String(fontSize)+"px; padding:"+padding+"; "
                                   "border-radius:"+borderRadius+"; display:inline-flex; align-items:center; justify-content:center; "
                                   "text-align:center; box-sizing:border-box; white-space:nowrap; max-width:90%; "
                                   "box-shadow:0 10px 20px rgba(0,0,0,0.45); z-index:2;";

              html += "<div id='"+overlay.id+"' style='"+panelStyle+"'>"+overlay.label+"</div>";
            }



          for(auto &e : self->elements){
              if(e.tab != tabId) continue;
              if(e.type=="displayStringAbsolute" || e.type=="image") continue;
              if(e.type=="displayGraph" || e.type=="displayGraphJS"){
                  String config = e.value;
                  auto readSetting = [&](const String &prop)->String {
                      String key = prop + ":";
                      int idx = config.indexOf(key);
                      if(idx < 0) return "";
                      int start = idx + key.length();
                      int end = config.indexOf(";", start);
                      if(end < 0) end = config.length();
                      return config.substring(start, end);
                  };
                  auto ensureUnit = [&](const String &raw)->String {
                      if(raw.length() == 0) return "";
                      bool hasUnit = false;
                      for(int i=0;i<raw.length();i++){
                          char c = raw[i];
                          if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                      }
                      return hasUnit ? raw : raw + "px";
                  };
                  String widthRaw = readSetting("width");
                  String heightRaw = readSetting("height");
                  String valueName = readSetting("value");
                  if(valueName.length()==0) valueName = readSetting("source");
                  if(valueName.length()==0) valueName = e.id;
                  String seriesName = e.id;
                  int canvasWidth = widthRaw.length() ? widthRaw.toInt() : 320;
                  int canvasHeight = heightRaw.length() ? heightRaw.toInt() : 220;
                  String widthStyle = widthRaw.length() ? ensureUnit(widthRaw) : "100%";
                  String heightStyle = heightRaw.length() ? ensureUnit(heightRaw) : "220px";
                  String left = ensureUnit(readSetting("left"));
                  String top = ensureUnit(readSetting("top"));
                  String xLabel = readSetting("xLabel"); if(xLabel.length()==0) xLabel = "X Axis";
                  String yLabel = readSetting("yLabel"); if(yLabel.length()==0) yLabel = "Y Axis";
                  String pointColor = readSetting("pointColor"); if(pointColor.length()==0) pointColor = "#ff8c42";
                  String lineColor = readSetting("lineColor"); if(lineColor.length()==0) lineColor = "#4CAF50";
                  String updatePeriodRaw = readSetting("updatePeriod_of_Time");
                  String updateStepRaw = readSetting("updateStep");
                  unsigned long maxUpdatePeriodMinutes = updatePeriodRaw.length() ? updatePeriodRaw.toInt() : 10; // минуты
                  unsigned long updateStepMinutes = updateStepRaw.length() ? updateStepRaw.toInt() : 1;           // минуты
                  unsigned long maxUpdatePeriod = maxUpdatePeriodMinutes * 60000UL;
                  unsigned long updateStep = updateStepMinutes * 60000UL;

                  if(maxUpdatePeriod < minGraphUpdateInterval) maxUpdatePeriod = minGraphUpdateInterval;
                  if(updateStep < minGraphUpdateInterval) updateStep = minGraphUpdateInterval;
                  if(updateStep > maxUpdatePeriod) updateStep = maxUpdatePeriod;
                  const size_t maxUpdateOptions = 120;
                  unsigned long optionCount = maxUpdatePeriod / updateStep;
                  if(optionCount > maxUpdateOptions){
                    updateStep = maxUpdatePeriod / maxUpdateOptions;
                    if(updateStep < minGraphUpdateInterval) updateStep = minGraphUpdateInterval;
                  }
                  String maxPointsRaw = readSetting("maxPoints");
                  int defaultGraphMax = maxPointsRaw.length() ? maxPointsRaw.toInt() : (maxPoints > 0 ? maxPoints : 30);
                  if(defaultGraphMax < minGraphPoints) defaultGraphMax = minGraphPoints;
                  int maxSelectablePoints = defaultGraphMax;
                  if(maxSelectablePoints > maxGraphPoints) maxSelectablePoints = maxGraphPoints;
                  unsigned long defaultUpdate = updateInterval > 0 ? updateInterval : updateStep;
                  GraphSettings seriesSettings{defaultUpdate, maxSelectablePoints};

                  if(!loadGraphSettings(seriesName, seriesSettings)){
                    loadGraphSettings(valueName, seriesSettings);
                  }
                  int graphUpdateInterval = seriesSettings.updateInterval;
                  if(graphUpdateInterval < (int)minGraphUpdateInterval) graphUpdateInterval = minGraphUpdateInterval;
                  if(graphUpdateInterval > (int)maxUpdatePeriod) graphUpdateInterval = maxUpdatePeriod;
                  int graphMaxPoints = seriesSettings.maxPoints;
                  if(graphMaxPoints < minGraphPoints) graphMaxPoints = minGraphPoints;
                  if(graphMaxPoints > maxSelectablePoints) graphMaxPoints = maxSelectablePoints;
                  String graphUpdateStr = String(graphUpdateInterval);
                  String graphMaxStr = String(graphMaxPoints);
                  String tableId = "graphTable_"+e.id;
                  String containerStyle = "width:"+widthStyle+";max-width:100%;height:auto;padding:10px 12px;";
                  containerStyle += "display:flex;flex-direction:column;";
                  if(left.length() || top.length()){
                      containerStyle += "position:absolute;";
                      if(left.length()) containerStyle += "left:"+left+";";
                      if(top.length()) containerStyle += "top:"+top+";";
                  }
                  html += "<div class='card graph-card' style='"+containerStyle+"'>";
                  html += "<div class='graph-heading'>"+e.label+"</div>";
                  html += "<canvas id='graph_"+e.id+"' class='dash-graph' data-graph-id='"+e.id+"' width='"+String(canvasWidth)+"' height='"+String(canvasHeight)+"' data-series='"+seriesName+"' data-table-id='"+tableId+"' data-update-interval='"+String(graphUpdateInterval)+"' data-max-points='"+String(graphMaxPoints)+"' data-line-color='"+lineColor+"' data-point-color='"+pointColor+"' data-x-label='"+xLabel+"' data-y-label='"+yLabel+"' style='border:1px solid rgba(255,255,255,0.08);height:"+heightStyle+";'></canvas>";
                  html += "<div class='graph-axes'><span class='axis-name'>"+xLabel+"</span><span class='axis-name'>"+yLabel+"</span></div>";
              html += "</div>";
                  html += "<div class='card graph-controls' style='margin-bottom:0;'>";
                  html += "<div class='control-group'><label>Update Interval</label>";
                  html += "<select class='graph-update-interval' data-graph='"+e.id+"'>";
                  auto formatIntervalLabel = [](unsigned long valMs){
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
                      html += String("<option value='") + val + "'" + (graphUpdateStr==val ? " selected" : "") + ">" + formatIntervalLabel(oneSecond) + "</option>";
                  }

                  for(unsigned long opt = updateStep; opt <= maxUpdatePeriod; opt += updateStep){
                      String val = String(opt);
                      html += String("<option value='") + val + "'" + (graphUpdateStr==val ? " selected" : "") + ">" + formatIntervalLabel(opt) + "</option>";
                                          if(maxUpdatePeriod - opt < updateStep) break;
                  }
                  html += "</select></div>";
                  html += "<div class='control-group'><label>Max Points</label>";
                  html += "<input class='graph-max-points' data-graph='"+e.id+"' type='number' min='1' max='"+String(maxSelectablePoints)+"' value='"+graphMaxStr+"'>";
                  html += "</div>";
                  html += "</div>";
                  html += "<div class='card' style='overflow-x:auto;'>";
                  html += "<table id='"+tableId+"' style='min-width:400px;'>";
                  html += "<thead><tr><th>#</th><th>Date &amp; Time</th><th>Value</th></tr></thead><tbody></tbody>";
                  html += "</table></div>";
                  continue;
              }
              String val = uiValueForId(e.id);
              if(e.type=="timer"){
                  UITimerEntry &timer = ui.timer(e.id);
                  html += "<div class='card timer-card'>";
                  html += "<div class='timer-card__header'>" + e.label + "</div>";
                  html += "<div class='timer-card__grid'>";
                  html += "<div class='timer-card__column'><label for='" + e.id + "_ON'>Включение</label>"
                          "<input id='" + e.id + "_ON' type='time' value='" + formatMinutesToTime(timer.on) + "'></div>";
                  html += "<div class='timer-card__column'><label for='" + e.id + "_OFF'>Отключение</label>"
                          "<input id='" + e.id + "_OFF' type='time' value='" + formatMinutesToTime(timer.off) + "'></div>";
                  html += "</div></div>";
                  continue;
              }


              html += "<div class='card'>";

              if(e.type=="text") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='text' value='"+val+"'>";
              else if(e.type=="int") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='number' value='"+val+"'>";
              else if(e.type=="float") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='number' step='0.1' value='"+val+"'>";
              else if(e.type=="time") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='time' value='"+val+"'>";
              else if(e.type=="color") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='color' value='"+val+"'>";
              else if(e.type=="checkbox"){
                String checked = (val == "1" || val.equalsIgnoreCase("true")) ? " checked" : "";
                html += "<label><input id='"+e.id+"' type='checkbox' value='1'"+checked+">"+e.label+"</label>";
              }
              
              else if(e.type=="slider"){
                  String cfg = e.value;
                  auto readSliderCfg = [&](const String &prop)->String {
                      String token = prop + "=";
                      int start = cfg.indexOf(token);
                      if(start < 0) return "";
                      start += token.length();
                      int end = cfg.indexOf(';', start);
                      if(end < 0) end = cfg.length();
                      return cfg.substring(start, end);
                  };
                  float minCfg = 0;
                  float maxCfg = 100;
                  float stepCfg = 1;
                  String minStr = readSliderCfg("min");
                  if(minStr.length()) minCfg = minStr.toFloat();
                  String maxStr = readSliderCfg("max");
                  if(maxStr.length()) maxCfg = maxStr.toFloat();
                  String stepStr = readSliderCfg("step");
                  if(stepStr.length()) stepCfg = stepStr.toFloat();
                  if(!val.length()) val = readSliderCfg("value");
                  if(!val.length()){
                      int stepInt = static_cast<int>(stepCfg);
                      bool integerStep = (static_cast<float>(stepInt) == stepCfg);
                      val = String(minCfg, integerStep ? 0 : 2);
                  }
                  html += "<label>"+e.label+"</label><input id='"+e.id+"' type='range' min='"+String(minCfg)+"' max='"+String(maxCfg)+"' step='"+String(stepCfg)+"' value='"+val+"' oninput='updateSlider(this)'><span id='"+e.id+"Val'> "+val+"</span>";
              }
              else if(e.type=="button"){
                  String state = val.length() ? val : "0";
                  String text = (state == "1") ? "ON" : "OFF";
                  String cssState = (state == "1") ? " on" : " off";

                
                  String cfg = e.value;
                  auto readProp = [&](const String &prop)->String {
                      String key = prop + ":";
                      int idx = cfg.indexOf(key);
                      if(idx < 0) return "";
                      int start = idx + key.length();
                      int end = cfg.indexOf(";", start);
                      if(end < 0) end = cfg.length();
                      return cfg.substring(start, end);
                  };
                  auto ensureUnit = [&](const String &raw)->String {
                      if(raw.length() == 0) return "";
                      bool hasUnit = false;
                      for(int i=0;i<raw.length();i++){
                          char c = raw[i];
                          if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                      }
                      return hasUnit ? raw : raw + "px";
                  };
                  String x = readProp("x");
                  String y = readProp("y");
                  String w = readProp("width");
                  String h = readProp("height");
                  String btnColor = readProp("color");
                  String styleExtra;
                  if(x.length()) styleExtra += "margin-left:"+ensureUnit(x)+";";
                  if(y.length()) styleExtra += "margin-top:"+ensureUnit(y)+";";
                  if(w.length()) styleExtra += "width:"+ensureUnit(w)+";";
                  if(h.length()) styleExtra += "height:"+ensureUnit(h)+";";
                  if(btnColor.length()){
                      String normalized = btnColor;
                      normalized.trim();
                      if(!normalized.startsWith("#") && (normalized.length()==3 || normalized.length()==6)){
                          normalized = "#" + normalized;
                      }
                      styleExtra += "background:"+normalized+";border-color:"+normalized+";";
                  }
                  String styleAttr = styleExtra.length() ? " style='"+styleExtra+"'" : "";

                  html += "<label>"+e.label+"</label><button id='"+e.id+"' class='dash-btn"+cssState+"' data-type='dashButton' data-state='"+state+"'"+styleAttr+">"+text+"</button>";
              }
              else if(e.type=="popupButton"){
                  String btnText = e.label.length() ? e.label : "Open";
                  html += "<button class='btn-primary' data-popup-open='"+e.value+"'>"+btnText+"</button>";
              }
              else if(e.type=="display" || e.type=="displayString") 
                html += "<label>"+e.label+"</label><div id='"+e.id+"' style='font-size:1.2em; min-height:1.5em; color:#fff;'>"+val+"</div>";
              else if(e.type=="displayStringAbsolute") {
                  int x=0, y=0, fontSize=16;
                  String color="#ffffff";
                  String style = e.value;
                  int idx;

                  if((idx = style.indexOf("x:"))!=-1) x = style.substring(idx+2, style.indexOf(";", idx)).toInt();
                  if((idx = style.indexOf("y:"))!=-1) y = style.substring(idx+2, style.indexOf(";", idx)).toInt();
                  if((idx = style.indexOf("fontSize:"))!=-1) fontSize = style.substring(idx+9, style.indexOf(";", idx)).toInt();
                  if((idx = style.indexOf("color:"))!=-1) color = style.substring(idx+6);

                  html += "<div id='"+e.id+"' style='position:absolute; left:"+String(x)+"px; top:"+String(y)+"px; font-size:"+String(fontSize)+"px; color:"+color+";'>"+e.label+"</div>";
              }

              else if(e.type=="dropdown"){
                  html += "<label>"+e.label+"</label><select id='"+e.id+"'>";
                  if(e.value.length()){
                      int start = 0;
                      while(start < e.value.length()){
                          int end = e.value.indexOf('\n', start);
                          if(end < 0) end = e.value.length();
                          String line = e.value.substring(start, end);
                          line.trim();
                          if(line.length()){
                              int sep = line.indexOf('=');
                              String optValue = sep >= 0 ? line.substring(0, sep) : line;
                              String optLabel = sep >= 0 ? line.substring(sep + 1) : line;
                              html += String("<option value='") + optValue + "'" + (val==optValue?" selected":"") + ">" + optLabel + "</option>";
                          }
                          start = end + 1;
                      }
                  } else {
                      html += String("<option value='Normal'") + (val=="Normal"?" selected":"") + ">Normal</option>";
                      html += String("<option value='Eco'") + (val=="Eco"?" selected":"") + ">Eco</option>";
                      html += String("<option value='Turbo'") + (val=="Turbo"?" selected":"") + ">Turbo</option>";
                  }
                  html += "</select>";
              }
              else if(e.type=="selectdays"){
                  html += "<label>"+e.label+"</label>";
                  html += "<div id='"+e.id+"' class='select-days'>";
                 const char* dayValues[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
                  const char* dayLabels[] = {"ПН","ВТ","СР","ЧТ","ПТ","СБ","ВС"};
                  for(int i=0;i<7;i++){
                      bool checked = val.indexOf(dayValues[i])>=0;
                      html += "<label class='day-pill'><input type='checkbox' value='"+String(dayValues[i])+"' "
                          +(checked?"checked":"")+"><span>"+String(dayLabels[i])+"</span></label>";
                  }
                  html += "</div>";
              }
          else if(e.type=="range"){
                  String cfg = e.value;
                  auto readRangeCfg = [&](const String &prop)->String {
                      String token = prop + "=";
                      int start = cfg.indexOf(token);
                      if(start < 0) return "";
                      start += token.length();
                      int end = cfg.indexOf(';', start);
                      if(end < 0) end = cfg.length();
                      return cfg.substring(start, end);
                  };
                  float minCfg = 0;
                  float maxCfg = 100;
                  float stepCfg = 1;
                  String minStr = readRangeCfg("min");
                  if(minStr.length()) minCfg = minStr.toFloat();
                  String maxStr = readRangeCfg("max");
                  if(maxStr.length()) maxCfg = maxStr.toFloat();
                  String stepStr = readRangeCfg("step");
                  if(stepStr.length()) stepCfg = stepStr.toFloat();

                  float minVal = minCfg;
                  float maxVal = maxCfg;
                  if(val.length()){
                      int sep = val.indexOf('-');
                      if(sep >= 0){
                          minVal = val.substring(0, sep).toFloat();
                          maxVal = val.substring(sep + 1).toFloat();
                      }
                  }

              int precision = stepCfg < 1.0f ? 2 : 0;

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

document.querySelectorAll('div[id$="DaysSelect"]').forEach(el=>{
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

// ====== ������� ��� ����� � live ======
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
        if(typeof j.Power_Filtr !== 'undefined') updateCheckboxValue('Power_Filtr', j.Power_Filtr);
    if(typeof j.Filtr_Time1 !== 'undefined') updateCheckboxValue('Filtr_Time1', j.Filtr_Time1);
    if(typeof j.Filtr_Time2 !== 'undefined') updateCheckboxValue('Filtr_Time2', j.Filtr_Time2);
    if(typeof j.Filtr_Time3 !== 'undefined') updateCheckboxValue('Filtr_Time3', j.Filtr_Time3);

    if(typeof j.Power_Clean !== 'undefined') updateCheckboxValue('Power_Clean', j.Power_Clean);
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
    
    });
  }
setInterval(fetchLive, 1000);
setInterval(tickPageDateTime, 1000);
fetchMqttConfig();

// ====== ??????? ???????????? ??????????? (???????? /setjpg ? ????????????? ????????) ======
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
      StaticJsonDocument<6144> doc;
      doc["CurrentTime"] = CurrentTime;
      doc["RandomVal"] = RandomVal;
      doc["InfoString"] = InfoString;
      doc["InfoString1"] = InfoString1;
      doc["InfoString2"] = InfoString2;
      doc["InfoStringDIN"] = InfoStringDIN;
            doc["OverlayPoolTemp"] = OverlayPoolTemp;
      doc["OverlayHeaterTemp"] = OverlayHeaterTemp;
      doc["OverlayLevelUpper"] = OverlayLevelUpper;
      doc["OverlayLevelLower"] = OverlayLevelLower;
      doc["FilterImageState"] = jpg;
      doc["button1"] = button1;
      doc["button2"] = button2;
      doc["button_Lamp"] = Lamp ? 1 : 0;
      doc["Pow_Ul_light"] = Pow_Ul_light ? 1 : 0;
      doc["MotorSpeed"] = MotorSpeedSetting;
      doc["RangeMin"] = RangeMin;
      doc["RangeMax"] = RangeMax;
      doc["LEDColor"] = LEDColor;
      doc["LedPattern"] = LedPattern;
      doc["LedColorMode"] = LedColorMode;
      doc["LedColorOrder"] = LedColorOrder;
      doc["LedBrightness"] = LedBrightness;
      doc["LedAutoplay"] = LedAutoplay ? 1 : 0;
      doc["LedAutoplayDuration"] = LedAutoplayDuration;
      doc["ModeSelect"] = ModeSelect;
      doc["SetLamp"] = SetLamp;
      doc["SetRGB"] = SetRGB;
      doc["DaysSelect"] = DaysSelect;
      doc["IntInput"] = IntInput;
      doc["FloatInput"] = FloatInput;
      doc["Timer1"] = Timer1;
      doc["Power_Time1"] = Power_Time1 ? 1 : 0;
      doc["Ul_light_Time"] = Ul_light_Time ? 1 : 0;
      for (const auto &timer : ui.allTimers()) {
        doc[String(timer.id + "_ON")] = formatMinutesToTime(timer.on);
        doc[String(timer.id + "_OFF")] = formatMinutesToTime(timer.off);
      }
      doc["WS2815_Time1"] = WS2815_Time1 ? 1 : 0;
      doc["Activation_Water_Level"] = Activation_Water_Level ? 1 : 0;
      doc["Power_Topping"] = Power_Topping ? 1 : 0;
      doc["Power_Filtr"] = Power_Filtr ? 1 : 0;
      doc["Filtr_Time1"] = Filtr_Time1 ? 1 : 0;
      doc["Filtr_Time2"] = Filtr_Time2 ? 1 : 0;
      doc["Filtr_Time3"] = Filtr_Time3 ? 1 : 0;
      doc["Power_Clean"] = Power_Clean ? 1 : 0;
      doc["Clean_Time1"] = Clean_Time1 ? 1 : 0;
      doc["DS1"] = String(DS1, 1) + " °C";
      doc["RoomTemp"] = String(DS1, 1) + " °C";
      doc["Sider_heat"] = Sider_heat;
      doc["Activation_Heat"] = Activation_Heat ? 1 : 0;
      doc["Power_Heat"] = Power_Heat ? "Нагрев" : "Откл.";
      doc["RoomTempOn"] = RoomTempOn;
      doc["RoomTempOff"] = RoomTempOff;
      doc["RoomTemper"] = RoomTemper ? 1 : 0;
      doc["Power_Warm_floor_heating"] = Power_Warm_floor_heating ? "Включен" : "Откл.";
      doc["WaterLevelSensorUpper"] = WaterLevelSensorUpper ? "Активен" : "Откл.";
      doc["WaterLevelSensorLower"] = WaterLevelSensorLower ? "Активен" : "Откл.";
      doc["Power_Topping_State"] = Power_Topping_State ? "Включен" : "Откл.";
      doc["PH"] = String(PH, 2);
      doc["analogValuePH"] = String(analogValuePH_Comp) + " mV";
      doc["Temper_Reference"] = Temper_Reference;
      doc["PH_Control_ACO"] = PH_Control_ACO ? 1 : 0;
      doc["PH_setting"] = PH_setting;
      doc["ACO_Work"] = ACO_Work;
      const bool powerAcoActive = Power_ACO || ManualPulse_ACO_Active;
      const bool powerH2O2Active = Power_H2O2 || ManualPulse_H2O2_Active;
      doc["Power_ACO"] = powerAcoActive ? "Работа" : "Откл.";
      doc["Power_ACO_Button"] = powerAcoActive ? 1 : 0;
      doc["ppmCl"] = String(ppmCl, 3);
      doc["corrected_ORP_Eh_mV"] = String(corrected_ORP_Eh_mV);
      doc["NaOCl_H2O2_Control"] = NaOCl_H2O2_Control ? 1 : 0;
      doc["ORP_setting"] = ORP_setting;
      doc["H2O2_Work"] = H2O2_Work;
      doc["Power_H2O2"] = powerH2O2Active ? "Работа" : "Откл.";
      doc["Power_H2O2_Button"] = powerH2O2Active ? 1 : 0;
      doc["Lumen_Ul"] = Lumen_Ul;
      doc["Comment"] = Comment;
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


    // ---------------- SET JPG (??????????? jpg ??????????) ----------------
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


    server.serveStatic("/anim1.gif", SPIFFS, "/anim1.gif");
    server.serveStatic("/Basin.jpg", SPIFFS, "/Basin.jpg");
    server.serveStatic("/img1.jpg", SPIFFS, "/img1.jpg");
    server.serveStatic("/img2.jpg", SPIFFS, "/img2.jpg");
    server.begin();
  }
} dash;
