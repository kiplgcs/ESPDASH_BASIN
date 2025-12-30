// main.cpp — UI ESPDASH-PRO проект для ESP32 в Visual Studio Code
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
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
#include <Adafruit_ADS1X15.h> // Библиотека для работы с модулями ADS1115 и ADS1015
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

  // Подключение к Wi-Fi с использованием сохранённых данных и кнопок
  StoredAPSSID = loadValue<String>("apSSID", String(apSSID));
  StoredAPPASS = loadValue<String>("apPASS", String(apPASS));
  button1 = loadButtonState("button1", 0);
  button2 = loadButtonState("button2", 0);
  // Sider_heat = loadValue<int>("Sider_heat", 28);
  // Activation_Heat = loadValue<int>("Activation_Heat", 0) != 0;
  // Sider_heat1 = Sider_heat;
  // Activation_Heat1 = Activation_Heat;
  initWiFiModule();

  // Инициализация файловой системы SPIFFS
  initFileSystem();

  // Загрузка параметра jpg из файловой системы (по умолчанию 1)
  jpg = loadValue<int>("jpg", 1);

  // Инициализация времени из сохраненного значения (если есть)
  loadBaseEpochFromStorage();

  // Загрузка и применение MQTT параметров
  loadMqttSettings();
  applyMqttState();

  // Запуск OTA-обновлений на порту 8080
  beginWebUpdate();


/************************* Настраиваем  АЦП модуль ADS1115 16-бит *********************************/
  //Для подключения 16-битного АЦП ADS1115:
  // в базовом варианте платы NodeMCU-32S I2C интерфейс завязан на пины 21 и 22
  Wire.begin(8, 9); //Wire.begin(23, 22);  // Инициализируем I2C с SDA  и SCL 

  //Serial.println("Getting single-ended readings from AIN0..3");
  //Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
  // Входной диапазон (или усиление) АЦП можно изменить с помощью следующих
  // функции, но будьте осторожны, чтобы не превышать VDD + 0,3 В макс или
  // превышать верхний и нижний пределы, если вы отрегулируете диапазон ввода! 
  //Неправильное изменение этих значений может привести к повреждению вашего АЦП!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default) //тут настраиваем опорное напряжение
  ads1.setGain(GAIN_TWOTHIRDS);
  ads2.setGain(GAIN_TWOTHIRDS);


  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
  // ads.begin(); //инициализируем модуль с ранее настроенными параметрами

  ads1.begin(0x48);
  ads2.begin(0x49);
 
  /************************* инициализируем монитор Nextion*********************************/
 
  //myNex.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, RXD1, TXD1); // Инициализируем порт со своими пинами


  myNex.lastCurrentPageId = 1;  // При первом запуске цикла currentPageId и lastCurrentPageId
                                // должны иметь разные значения из-за запуска функции firstRefresh()
  myNex.writeStr("page 0");     // Для синхронизации страницы Nextion в случае сброса на Arduino
  //triggerRestartNextion = true; //Флаг чтения всех необходимых переменных из Nextion, после перезагрузки контроллера.

  //Прерываем по пину  RX порта для выполнения функции получения данных для монитора Nextion
  //attachInterrupt(digitalPinToInterrupt(RXD1), ActivUARTInterrupt, RISING); //CHANGE //FALLING //RISING); 
  /************************* инициализируем и получаем время*********************************/
  //setup_rtc(); - отключена, т.к. настройка серверов идет в основной функции запроса
  
  setup_ds18();


















  // ---------- Загрузка сохранённых значений ----------
  ThemeColor = loadValue<String>("ThemeColor","#1e1e1e");  // Цвет темы
  LEDColor = loadValue<String>("LEDColor","#00ff00");      // Цвет LED
  MotorSpeedSetting = loadValue<int>("MotorSpeed",25);     // Скорость мотора
  IntInput = loadValue<int>("IntInput",10);               // Целое число
  FloatInput = loadValue<float>("FloatInput",3.14);       // Число с плавающей точкой
  Timer1 = loadValue<String>("Timer1","12:00");           // Таймер
  Comment = loadValue<String>("Comment","Hello!");        // Комментарий
  // ModeSelect = loadValue<String>("ModeSelect","Normal");  // Режим работы
  ModeSelect = loadValue<String>("ModeSelect","Normal");  // Режим работы
  DaysSelect = loadValue<String>("DaysSelect","Mon,Wed,Fri"); // Дни недели
  RangeMin = loadValue<int>("RangeMin", RangeMin);        // Минимум диапазона
  RangeMax = loadValue<int>("RangeMax", RangeMax);        // Максимум диапазона

  Power_Filtr = loadValue<int>("Power_Filtr", 0) != 0;
  Filtr_Time1 = loadValue<int>("Filtr_Time1", 0) != 0;
  Filtr_Time2 = loadValue<int>("Filtr_Time2", 0) != 0;
  Filtr_Time3 = loadValue<int>("Filtr_Time3", 0) != 0;
  // Filtr_timeON1 = loadValue<String>("Filtr_timeON1", "00:00");
  // Filtr_timeOFF1 = loadValue<String>("Filtr_timeOFF1", "00:00");
  // Filtr_timeON2 = loadValue<String>("Filtr_timeON2", "00:00");
  // Filtr_timeOFF2 = loadValue<String>("Filtr_timeOFF2", "00:00");
  // Filtr_timeON3 = loadValue<String>("Filtr_timeON3", "00:00");
  // Filtr_timeOFF3 = loadValue<String>("Filtr_timeOFF3", "00:00");
  Power_Clean = loadValue<int>("Power_Clean", 0) != 0;
  Clean_Time1 = loadValue<int>("Clean_Time1", 0) != 0;
  // Clean_timeON1 = loadValue<String>("Clean_timeON1", "00:00");
  // Clean_timeOFF1 = loadValue<String>("Clean_timeOFF1", "00:00");
  syncCleanDaysFromSelection();




  Pow_WS2815 = loadButtonState("button_WS2815", 1) != 0;
    Pow_WS2815_autosvet = loadValue<int>("Pow_WS2815_autosvet", 0) != 0;
  WS2815_Time1 = loadValue<int>("WS2815_Time1", 0) != 0;
  // timeON_WS2815 = loadValue<String>("timeON_WS2815", "00:00");
  // timeOFF_WS2815 = loadValue<String>("timeOFF_WS2815", "00:00");
    SetRGB = loadValue<String>("SetRGB", "off");
  ColorLED = loadValue<String>("LEDColor","#00ff00");      // WS2815
  LedPattern = loadValue<String>("LedPattern", LedPattern);
  LedColorMode = loadValue<String>("LedColorMode", LedColorMode);
  LedColorOrder = loadValue<String>("LedColorOrder", LedColorOrder);
  ColorRGB = LedColorMode.equalsIgnoreCase("manual");
  LedAutoplay = loadValue<int>("LedAutoplay", LedAutoplay ? 1 : 0) != 0;
  LedAutoplayDuration = loadValue<int>("LedAutoplayDuration", LedAutoplayDuration);
  if(LedAutoplayDuration < 1) LedAutoplayDuration = 1;
  LedBrightness = loadValue<int>("LedBrightness", LedBrightness);
  new_bright = LedBrightness;


  // SetLamp = loadValue<String>("SetLamp", "");
  Lamp = loadButtonState("button_Lamp", 0) != 0;
  // Lamp_autosvet = false;
  Lamp_autosvet = loadValue<int>("Lamp_autosvet", 0) != 0;
  Power_Time1 = loadValue<int>("Power_Time1", 0) != 0;
  // Lamp_timeON1 = loadValue<String>("Lamp_timeON1", "00:00");
  // Lamp_timeOFF1 = loadValue<String>("Lamp_timeOFF1", "00:00");
  // if (SetLamp.length()) {
  //   applyLampModeFromSetLamp();
  // } else {
  //   syncSetLampFromFlags();
  //   saveValue<String>("SetLamp", SetLamp);
  // }
   SetLamp = loadValue<String>("SetLamp", "off");

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








/* ---------- Loop ---------- */
void loop() {
  wifiModuleLoop();
  // Обновление времени через NTP/Nextion/память
  NPT_Time(period_get_NPT_Time);
  CurrentTime = getCurrentDateTime();   // Получение текущего времени

TimerControlRelay(10000);  // TimerControlRelay(600); //Контроль включения реле по таймерам

ControlModbusRelay(1000);
loop_PH(2000);
loop_CL2(2000);

 /**************************** *********************************************************************/
   Nextion_Transmit(500); // Отправка в Nextion по очереди
  //if(Power_Clean){Power_Filtr = false;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра
  // Проверяем, активирован ли триггер
   if (triggerActivated_Nextion) {/*NextionDelay (3000);*/ delay(2000); NextionDelay ();} //Отложенное чтение по флагу - переменных из Nextion, связанное с поздним обновление в Nextion информации.
   if (triggerRestartNextion) {/*RestartNextionDelay(5000); */ delay(3000); RestartNextionDelay();} //Для отложенного чтения всех необходимых переменных после перезагрузки ESP
  ///////////////////////////////////////////////////////////////////////////////////////
  Temp_DS18B20(5000); //Измеряем температуру
 ///****************************  Nextion - проверка прихода данных Tx/Rx ****************************************/
  while (MySerial.available()){myNex.NextionListen();} //для непрерывного чтения данных из порта, пока они доступны
 /**************************** *********************************************************************/
  // Nextion_Transmit(500); // Отправка в Nextion по очереди
  // if(Power_Clean){Power_Filtr = false;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра












  // Генерация случайных значений для демонстрации
  RandomVal = random(0,50);                     // Случайное число
  Speed = random(150, 350) / 10.0f;            // Случайная скорость
  // Temperatura = random(220, 320) / 10.0f;      // Случайная температура
  Temperatura = DS1;      // Температура в бассейне
  
  // Формирование информационных строк
  InfoString = "Random value is " + String(RandomVal) + " at " + CurrentTime + "Pow_WS2815 = " + String(Pow_WS2815);
  InfoString1 = /*"Speed " + String(Speed, 1) + " / Temp " + String(Temperatura, 1)*/ + " button1 = " + String(button1)
              + " RangeSlider = " + String(RangeMin) + " / " + String(RangeMax);
  
           

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

  // Обработка MQTT-клиента
  handleMqttLoop();




  loop_WS2815();  




  slow(period_slow_Time); //Периодически отправляем данные для обратной связи - "period_slow_Time" Период обновления данных - зависит от "Nx_dim_id" nекущеuj - считанного значения яркости Nextion

}
