// main.cpp — UI ESPDASH-PRO проект для ESP32 в Visual Studio Code
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <Wire.h> // I2C и ADS1115 подключаем здесь, чтобы не дублировать в PH_CL2.h
#include <esp_system.h>
#include "NPT_Time.h"

#include "wifi_manager.h"        // Логика Wi-Fi и сохранение параметров
#include "fs_utils.h"    // Функции для работы с файловой системой SPIFFS
#include "graph.h"       // Функции для графиков и визуализации
#include "web.h"         // Функции работы Web-панели (ESP-DASH)
#include "ui - JeeUI2.h"         // Построитель UI в стиле JeeUI2
#include "interface - JeeUI2.h"  // Описание веб-интерфейса
#include "settings_MQTT.h"       // Настройки и работа с MQTT
#include "WebUpdate.h"    // OTA-обновление через AsyncOTA

#include "Timer_Relay.h"
#include "ds18.h"

/************************* Подключаем библиотеку  АЦП модуль ADS1115 16-бит *********************************/
#include <Adafruit_ADS1X15.h> // Библиотека для работы с модулями ADS1115 и ADS1015 (используется в PH_CL2.h)
//Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1115 ads1; // Первый ADS1115 - PH
Adafruit_ADS1115 ads2; // Второй ADS1115 - Хлор 
//Adafruit_ADS10

#include "ModbusRTU_RS485.h"

#include "Nextion_Rx.h"
#include "Nextion_Tx.h"

#include "LED_WS2815.h"

#include "PH_CL2.h"

#include <Slow.h> //Периодически выполняем  - для обратной связи с устройствами


// ---------- NTP (синхронизация времени) ----------
WiFiUDP ntpUDP;


/* ---------- Setup ---------- */
void setup() {
  Serial.begin(115200);

  // if (!SPIFFS.begin(true)) {   // true = форматировать, если пусто/битое
  //   Serial.println("SPIFFS mount failed");
  // } else {
  //   Serial.println("SPIFFS mounted");
  // }

  delay(100);
  Serial.println("\n[BOOT] ESPDASH starting...");
  const char *resetReasonText = []() -> const char * {
    switch (esp_reset_reason()) {
      case ESP_RST_POWERON:
        return "Power On";
      case ESP_RST_EXT:
        return "External Reset";
      case ESP_RST_SW:
        return "Software Reset";
      case ESP_RST_PANIC:
        return "Panic";
      case ESP_RST_INT_WDT:
        return "Interrupt Watchdog";
      case ESP_RST_TASK_WDT:
        return "Task Watchdog";
      case ESP_RST_WDT:
        return "Other Watchdog";
      case ESP_RST_DEEPSLEEP:
        return "Deep Sleep";
      case ESP_RST_BROWNOUT:
        return "Brownout";
      case ESP_RST_SDIO:
        return "SDIO";
      default:
        return "Unknown";
    }
  }();
  Serial.printf("[BOOT] Reset reason: %s\n", resetReasonText);
  Serial.printf("[BOOT] Chip model: %s | Cores: %u | Revision: %u\n",
                ESP.getChipModel(), ESP.getChipCores(), ESP.getChipRevision());

  // Подключение к Wi-Fi с использованием сохранённых данных и кнопок
  StoredAPSSID = loadValue<String>("apSSID", String(apSSID));
  StoredAPPASS = loadValue<String>("apPASS", String(apPASS));
  button1 = loadButtonState("button1", 0);
  button2 = loadButtonState("button2", 0);
  // Sider_heat = loadValue<int>("Sider_heat", 28);
  // Activation_Heat = loadValue<int>("Activation_Heat", 0) != 0;
  // Sider_heat1 = Sider_heat;
  // Activation_Heat1 = Activation_Heat;
    Serial.println("[BOOT] Initializing Wi-Fi...");
  initWiFiModule();

  // Инициализация файловой системы SPIFFS
    Serial.println("[BOOT] Initializing filesystem...");
  initFileSystem();

  // Загрузка параметра jpg из файловой системы (по умолчанию 1)
  jpg = loadValue<int>("jpg", 1);

  // Инициализация времени из сохраненного значения (если есть)
    Serial.println("[BOOT] Loading persisted time...");
  loadBaseEpochFromStorage();

  // Загрузка и применение MQTT параметров
  Serial.println("[BOOT] Loading MQTT settings...");
  loadMqttSettings();
  Serial.println("[BOOT] Applying MQTT state...");
  applyMqttState();

  // Загрузка настроек доступа к веб-интерфейсу
  authUsername = loadValue<String>("authUser", "");
  authPassword = loadValue<String>("authPass", "");
  adminUsername = loadValue<String>("adminUser", "");
  adminPassword = loadValue<String>("adminPass", "");

  beginWebUpdate();  // Запуск OTA-обновлений на порту 8080

  setup_ADS1115_PH_CL2(); //Настраиваем  АЦП модуль ADS1115 16-бит

  setup_Nextion(); //Настраиваем Nextion монитор

  setup_ds18();


  // // ---------- Загрузка сохранённых значений ----------
  // ThemeColor = loadValue<String>("ThemeColor","#1e1e1e");  // Цвет темы
  // LEDColor = loadValue<String>("LEDColor","#00ff00");      // Цвет LED
  // MotorSpeedSetting = loadValue<int>("MotorSpeed",25);     // Скорость мотора
  // IntInput = loadValue<int>("IntInput",10);               // Целое число
  // FloatInput = loadValue<float>("FloatInput",3.14);       // Число с плавающей точкой
  // Timer1 = loadValue<String>("Timer1","12:00");           // Таймер
  // Comment = loadValue<String>("Comment","Hello!");        // Комментарий
  // // ModeSelect = loadValue<String>("ModeSelect","Normal");  // Режим работы
  // ModeSelect = loadValue<String>("ModeSelect","Normal");  // Режим работы
  // DaysSelect = loadValue<String>("DaysSelect","Mon,Wed,Fri"); // Дни недели
  // RangeMin = loadValue<float>("RangeMin", RangeMin);        // Минимум диапазона
  // RangeMax = loadValue<float>("RangeMax", RangeMax);        // Максимум диапазона
  // // PH1_CAL = loadValue<float>("PH1_CAL", PH1_CAL);         // АЦП_mV для PH1
  // // PH2_CAL = loadValue<float>("PH2_CAL", PH2_CAL);         // АЦП_mV для PH2
  // PH1 = loadValue<float>("PH1_MIN", PH1);                 // Точки калибровки PH (минимум)
  // PH2 = loadValue<float>("PH2_MAX", PH2);                 // Точки калибровки PH (максимум)
  // Temper_Reference = loadValue<float>("Temper_Reference", Temper_Reference);
  // Power_Filtr = loadValue<int>("Power_Filtr", 0) != 0;
  // Filtr_Time1 = loadValue<int>("Filtr_Time1", 0) != 0;
  // Filtr_Time2 = loadValue<int>("Filtr_Time2", 0) != 0;
  // Filtr_Time3 = loadValue<int>("Filtr_Time3", 0) != 0;
  // // Filtr_timeON1 = loadValue<String>("Filtr_timeON1", "00:00");
  // // Filtr_timeOFF1 = loadValue<String>("Filtr_timeOFF1", "00:00");
  // // Filtr_timeON2 = loadValue<String>("Filtr_timeON2", "00:00");
  // // Filtr_timeOFF2 = loadValue<String>("Filtr_timeOFF2", "00:00");
  // // Filtr_timeON3 = loadValue<String>("Filtr_timeON3", "00:00");
  // // Filtr_timeOFF3 = loadValue<String>("Filtr_timeOFF3", "00:00");
  // Power_Clean = loadValue<int>("Power_Clean", 0) != 0;
  // Clean_Time1 = loadValue<int>("Clean_Time1", 0) != 0;
  // // Clean_timeON1 = loadValue<String>("Clean_timeON1", "00:00");
  // // Clean_timeOFF1 = loadValue<String>("Clean_timeOFF1", "00:00");
    // ---------- Загрузка значений интерфейса через декларативные элементы ----------
  interface();
  syncCleanDaysFromSelection();




  // Pow_WS2815 = loadButtonState("button_WS2815", 1) != 0;
  //   Pow_WS2815_autosvet = loadValue<int>("Pow_WS2815_autosvet", 0) != 0;
  // WS2815_Time1 = loadValue<int>("WS2815_Time1", 0) != 0;
  // // timeON_WS2815 = loadValue<String>("timeON_WS2815", "00:00");
  // // timeOFF_WS2815 = loadValue<String>("timeOFF_WS2815", "00:00");
  //   SetRGB = loadValue<String>("SetRGB", "off");
  // ColorLED = loadValue<String>("LEDColor","#00ff00");      // WS2815
  // LedPattern = loadValue<String>("LedPattern", LedPattern);
  // LedColorMode = loadValue<String>("LedColorMode", LedColorMode);
  // LedColorOrder = loadValue<String>("LedColorOrder", LedColorOrder);
  ColorRGB = LedColorMode.equalsIgnoreCase("manual");
  // LedAutoplay = loadValue<int>("LedAutoplay", LedAutoplay ? 1 : 0) != 0;
  // LedAutoplayDuration = loadValue<int>("LedAutoplayDuration", LedAutoplayDuration);
  if(LedAutoplayDuration < 1) LedAutoplayDuration = 1;
  // LedBrightness = loadValue<int>("LedBrightness", LedBrightness);
  new_bright = LedBrightness;


  // // SetLamp = loadValue<String>("SetLamp", "");
  // Lamp = loadButtonState("button_Lamp", 0) != 0;
  // // Lamp_autosvet = false;
  // Lamp_autosvet = loadValue<int>("Lamp_autosvet", 0) != 0;
  // Power_Time1 = loadValue<int>("Power_Time1", 0) != 0;
  // // Lamp_timeON1 = loadValue<String>("Lamp_timeON1", "00:00");
  // // Lamp_timeOFF1 = loadValue<String>("Lamp_timeOFF1", "00:00");
  // // if (SetLamp.length()) {
  // //   applyLampModeFromSetLamp();
  // // } else {
  // //   syncSetLampFromFlags();
  // //   saveValue<String>("SetLamp", SetLamp);
  // // }
  //  SetLamp = loadValue<String>("SetLamp", "off");

  // Pow_Ul_light = loadButtonState("Pow_Ul_light", 0) != 0;
  // Ul_light_Time = loadValue<int>("Ul_light_Time", 0) != 0;
  setup_WS2815();



  // ---------- Настройка графиков ----------
  loadGraph();

  dash.begin(); // Запуск дашборда



  setup_Modbus();


Serial.printf(
  "Heap Free: %u | Heap Min: %u | Max Block: %u | PSRAM Free: %u\n",
  ESP.getFreeHeap(),
  ESP.getMinFreeHeap(),
  ESP.getMaxAllocHeap(),
  ESP.getFreePsram()
);

}


inline void acoServiceLoop(){ // Сервисная обработка кнопки ручного импульса ACO без id-логики
  static bool lastUiState = false; // Предыдущее состояние UI-кнопки ACO
  const bool uiState = Power_ACO; // Текущее состояние кнопки из UI
  const bool actualState = ReadRelayArray[6]; // Фактическое состояние реле ACO из Modbus
  if(uiState != actualState){ // Если UI изменил состояние относительно фактического
    if(uiState && !lastUiState){ // Отслеживаем фронт нажатия кнопки
      ManualPulse_ACO_Active = true; // Активируем ручной импульс ACO
      ManualPulse_ACO_StartedAt = millis(); // Фиксируем время запуска импульса
    } // Конец обработки фронта
    Power_ACO = actualState; // Возвращаем значение к фактическому состоянию
    saveButtonState("Power_ACO_Button", actualState ? 1 : 0); // Сохраняем корректное состояние кнопки
  } // Конец обработки расхождения UI и фактического состояния
  lastUiState = uiState; // Запоминаем состояние кнопки для детекции фронта
} // Конец acoServiceLoop

inline void h2o2ServiceLoop(){ // Сервисная обработка кнопки ручного импульса H2O2 без id-логики
  static bool lastUiState = false; // Предыдущее состояние UI-кнопки H2O2
  const bool uiState = Power_H2O2; // Текущее состояние кнопки из UI
  const bool actualState = ReadRelayArray[5]; // Фактическое состояние реле H2O2 из Modbus
  if(uiState != actualState){ // Если UI изменил состояние относительно фактического
    if(uiState && !lastUiState){ // Отслеживаем фронт нажатия кнопки
      ManualPulse_H2O2_Active = true; // Активируем ручной импульс H2O2
      ManualPulse_H2O2_StartedAt = millis(); // Фиксируем время запуска импульса
    } // Конец обработки фронта
    Power_H2O2 = actualState; // Возвращаем значение к фактическому состоянию
    saveButtonState("Power_H2O2_Button", actualState ? 1 : 0); // Сохраняем корректное состояние кнопки
  } // Конец обработки расхождения UI и фактического состояния
  lastUiState = uiState; // Запоминаем состояние кнопки для детекции фронта
} // Конец h2o2ServiceLoop


/* ---------- Loop ---------- */
void loop() {
  wifiModuleLoop();
  acoServiceLoop(); // Обработка ручного импульса ACO по состоянию
  h2o2ServiceLoop(); // Обработка ручного импульса H2O2 по состоянию
  // Обновление времени через NTP/Nextion/память
  NPT_Time(period_get_NPT_Time);
  CurrentTime = getCurrentDateTime();   // Получение текущего времени

TimerControlRelay(10000);  // TimerControlRelay(600); //Контроль включения реле по таймерам
updateManualPumpPulses(); // Для прверки перельстатических насосов - счет таймера - 1 сек
ControlModbusRelay(1000);
loop_PH(2000);
loop_CL2(2100);

/**************************** *********************************************************************/
  Nextion_Transmit(500); // Отправка в Nextion по очереди
  //if(Power_Clean){Power_Filtr = false;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра
  // Проверяем, активирован ли триггер (без блокирующих delay)
  static unsigned long nextionDelayAt = 0;
  static unsigned long nextionRestartAt = 0;

  if (triggerActivated_Nextion) {
    if (nextionDelayAt == 0) {
      nextionDelayAt = millis() + 2000; // отложить на 2 секунды
    }
    if (nextionDelayAt != 0 && static_cast<long>(millis() - nextionDelayAt) >= 0) {
      NextionDelay();
      nextionDelayAt = 0;
    }
  } else {
    nextionDelayAt = 0;
  }

  if (triggerRestartNextion) {
    if (nextionRestartAt == 0) {
      nextionRestartAt = millis() + 3000; // отложить на 3 секунды
    }
    if (nextionRestartAt != 0 && static_cast<long>(millis() - nextionRestartAt) >= 0) {
      RestartNextionDelay();
      nextionRestartAt = 0;
    }
  } else {
    nextionRestartAt = 0;
  }


 ///////////////////////////////////////////////////////////////////////////////////////
  Temp_DS18B20(5000); //Измеряем температуру
 ///****************************  Nextion - проверка прихода данных Tx/Rx ****************************************/
  // Ограничиваем количество чтений за итерацию, чтобы не блокировать основной цикл при шуме на линии
  for (int i = 0; i < 20 && MySerial.available(); ++i) {
    myNex.NextionListen();
  }
 /**************************** *********************************************************************/
  // Nextion_Transmit(500); // Отправка в Nextion по очереди
  // if(Power_Clean){Power_Filtr = false;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра






  // Генерация случайных значений для демонстрации
  RandomVal = random(0,50);                     // Случайное число
  Speed = random(150, 350) / 10.0f;            // Случайная скорость
  // Temperatura = random(220, 320) / 10.0f;      // Случайная температура
  Temperatura = DS1;      // Температура в бассейне
  
  // Формирование информационных строк
  String dinStatus = "DIN входы 1-8: ";
  for (int i = 0; i < 8; ++i) {
    dinStatus += String(i + 1) + (ReadInputArray[i] ? "=1" : "=0");
    if (i < 7) {
      dinStatus += " ";
    }
  }
  dinStatus += "\nDIN входы 9-16: ";
  for (int i = 8; i < 16; ++i) {
    dinStatus += String(i + 1) + (ReadInputArray[i] ? "=1" : "=0");
    if (i < 15) {
      dinStatus += " ";
    }
  }

  InfoString = "Random value is " + String(RandomVal) + " at " + CurrentTime + "Pow_WS2815 = " + String(Pow_WS2815);
  InfoStringDIN = dinStatus;
  InfoString1 = /*"Speed " + String(Speed, 1) + " / Temp " + String(Temperatura, 1)*/ + " button1 = " + String(button1)
              + " RangeSlider = " + String(RangeMin) + " / " + String(RangeMax);
  
   
  OverlayPoolTemp = "🌡 Бассейн: " + formatTemperatureString(DS1, DS1Available);
  OverlayHeaterTemp = "♨️ После нагревателя: " + formatTemperatureString(DS2, DS2Available);
  OverlayLevelUpper = String("🛟 Верхний уровень: ") + (WaterLevelSensorUpper ? "Активен" : "Нет уровня");
  OverlayLevelLower = String("🛟 Нижний уровень: ") + (WaterLevelSensorLower ? "Активен" : "Нет уровня");
  OverlayPh = "🧪 pH: " + String(PH, 2);
  OverlayChlorine = "🧴 Cl: " + String(ppmCl, 3) + " ppm";
  OverlayFilterState = String("🧽 Фильтр: ") + (Power_Clean ? "Промывка" : (Power_Filtr ? "Фильтрация" : "Остановлен"));
  
  if (Power_Clean) {
    jpg = 2;
  } else if (Power_Filtr) {
    jpg = 1;
  } else {
    jpg = 1;
  }

//   // ---------- Рандомный цвет LED ----------
//   // LEDColor = "#" + String((random(0x1000000) | 0x1000000), HEX).substring(1);
//   const char hexDigits[] = "0123456789ABCDEF";
//   String color = "#";
//   int colorValues[] = { random(0,256), random(0,256), random(0,256) };
//   for(int i=0; i<3; i++){
//     color += hexDigits[(colorValues[i] >> 4) & 0xF];
//     color += hexDigits[colorValues[i] & 0xF];
//   }
//   LEDColor = color;

//   // ---------- Рандомный выбор режима ----------
//   // ModeSelect = (String[]){"Normal", "Eco", "Turbo"}[random(0, 3)];
//   const char* modes[] = {"Normal","Eco","Turbo"};
//   ModeSelect = String(modes[random(0,3)]);

  // ---------- Рандомный выбор дней недели ----------
  // DaysSelect = ({ String out=""; String d[7]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"}; 
  //   for(int i=0;i<7;i++) if(random(0,2)) out += (out==""?"":",") + d[i]; out == "" ? "Mon" : out; });
  // const char* weekDays[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  // String selectedDays;
  // for(int i=0; i<7; i++){
  //   if(random(0,2)){
  //     if(selectedDays.length()) selectedDays += ",";
  //     selectedDays += weekDays[i];
  //   }
  // }
  // if(selectedDays.length() == 0){
  //   selectedDays = weekDays[random(0,7)]; // хотя бы один день
  // }
  // DaysSelect = selectedDays;

//   // ---------- Рандомные значения для элементов ----------
//   IntInput = random(0,100);
//   FloatInput = random(0,100) / 10.0f;

// // ---------- Рандомные значения времени ----------
//   // Timer1 = String((random(0,24) < 10 ? "0" : "")) + String(random(0,24)) + ":" + String((random(0,60) < 10 ? "0" : "")) + String(random(0,60));
//   int hour = random(0,24);
//   int minute = random(0,60);
//   Timer1 = (hour < 10 ? "0" : "") + String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute);

  //  ---------- Рандомный текст ----------
  static const char* const comments[] = {
 "💧 Вода чистая",
    "🧪 pH в норме",
    "🌡 Температура стабильна",
    "🧹 Фильтр промыт",
    "✅ Все системы в порядке",
    "⚠️ Низкий уровень воды",
    "🔆 Подсветка включена",
    "🕒 Работа по таймеру",
    "Все ок 👍",
    "Работает как зверь 🦾",
    "Сегодня повезёт! ✨",
    "Турбо-режим активирован 🚀",
    "Отличный выбор 😉",
    "Готово! 🔧",
    "Запускаю магию 🪄",
    "Миссия выполнена ✅",
    "Стабильный поток 🌊",
    "Идеальный баланс ⚖️",
    "Система бодра 🧠",
    "Проверка пройдена 🧾",
    "Свет сияет 💡",
    "Насос в тонусе 🏋️",
    "Фильтр крутится 🔄",
    "Датчики на посту 📡",
    "Режим комфорта 😌",
    "Температура в цель 🎯",
    "Плавный ход 🛶",
    "Лёгкий бриз 🌬️",
    "Чистота на уровне 🧽",
    "Скорость стабильна 🧭",
    "Пузырьки счастья 🫧",
    "Охлаждение ровное ❄️",
    "Прогрев идёт 🔥",
    "Свежесть гарантирована 🍃",
    "Система на чеку 🛡️",
    "Таймеры синхронизированы ⏱️",
    "Запас мощности есть ⚡",
    "Сетевое соединение крепкое 📶",
    "Путь открыт 🛤️",
    "Отзыв отличный ⭐",
    "Ровные показатели 📈",
    "Мягкий режим 🧸",
    "Пики сглажены 🪂",
    "Зелёный свет 🟢",
    "Сигнал принят ✅",
    "Отличная циркуляция 🔁",
    "Все параметры в норме 🧩",
    "Безопасность активна 🧯",
    "Экономичный режим 💸",
    "Мощность оптимальна 🧰",
    "Стабильная работа 🧊",
    "Без перебоев 🧯",
    "Уровень точный 🎛️",
    "График красивый 📊",
    "Всё идёт по плану 🗺️",
    "Сервисный режим ✅",
    "Уверенный старт 🏁",
    "Режим тишины 🤫",
    "Комфортно и тихо 🪶",
    "Модуляция плавная 🎚️",
    "Капля к капле 💦",
    "Бережный режим 🤍",
    "Держим курс 🧭",
    "Система улыбается 😄",
    "Дышим ровно 🫁",
    "Всё под контролем 🎮"
  };
    Comment = comments[random(0, 66)];
  // Comment = "Random note " + String(random(1000,9999));

//   MotorSpeedSetting = random(1,50);
//   RangeMin = random(0,49);
//   RangeMax = random(49,100);
//   button1 = random(0,2);
//   button2 = random(0,2);




  // ---------- Добавление точек в графики с интервалом ----------
  addGraphPoint(CurrentTime, RandomVal); // Обновление графика RandomVal
  for(auto &entry : graphValueProviders){
    addSeriesPoint(entry.first, CurrentTime, entry.second()); // Обновление всех графиков
  }

  
  handleMqttLoop();// Обработка MQTT-клиента

  loop_WS2815();  




  slow(period_slow_Time); //Периодически отправляем данные для обратной связи - "period_slow_Time" Период обновления данных - зависит от "Nx_dim_id" nекущеuj - считанного значения яркости Nextion

}
