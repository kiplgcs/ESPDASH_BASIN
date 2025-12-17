// main.cpp — UI ESPDASH-PRO проект для ESP32 в Visual Studio Code
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

#include "wifi_manager.h"        // Логика Wi-Fi и сохранение параметров
#include "fs_utils.h"    // Функции для работы с файловой системой SPIFFS
#include "graph.h"       // Функции для графиков и визуализации
#include "web.h"         // Функции работы Web-панели (ESP-DASH)
#include "ui - JeeUI2.h"         // Построитель UI в стиле JeeUI2
#include "interface - JeeUI2.h"  // Описание веб-интерфейса
#include "settings_MQTT.h"       // Настройки и работа с MQTT
#include "WebUpdate.h"    // OTA-обновление через AsyncOTA


// ---------- NTP (синхронизация времени) ----------
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 10800, 1000); 
// NTP сервер, смещение +3 часа (МСК), обновление каждые 1 секунду

/* ---------- Setup ---------- */
void setup() {
  Serial.begin(115200);

  // Подключение к Wi-Fi с использованием сохранённых данных и кнопок
  StoredAPSSID = loadValue<String>("apSSID", String(apSSID));
  StoredAPPASS = loadValue<String>("apPASS", String(apPASS));
  button1 = loadButtonState("button1", 0);
  button2 = loadButtonState("button2", 0);
  initWiFiModule();

  // Инициализация файловой системы SPIFFS
  initFileSystem();

  // Загрузка параметра jpg из файловой системы (по умолчанию 1)
  jpg = loadValue<int>("jpg", 1);

  // Запуск NTP-клиента
  timeClient.begin();

  // Загрузка и применение MQTT параметров
  applyMqttState();

  // Запуск OTA-обновлений на порту 8080
  beginWebUpdate();

  // ---------- Загрузка сохранённых значений ----------
  ThemeColor = loadValue<String>("ThemeColor","#1e1e1e");  // Цвет темы
  LEDColor = loadValue<String>("LEDColor","#00ff00");      // Цвет LED
  MotorSpeedSetting = loadValue<int>("MotorSpeed",25);     // Скорость мотора
  IntInput = loadValue<int>("IntInput",10);               // Целое число
  FloatInput = loadValue<float>("FloatInput",3.14);       // Число с плавающей точкой
  Timer1 = loadValue<String>("Timer1","12:00");           // Таймер
  Comment = loadValue<String>("Comment","Hello!");        // Комментарий
  ModeSelect = loadValue<String>("ModeSelect","Normal");  // Режим работы
  DaysSelect = loadValue<String>("DaysSelect","Mon,Wed,Fri"); // Дни недели
  RangeMin = loadValue<int>("RangeMin", RangeMin);        // Минимум диапазона
  RangeMax = loadValue<int>("RangeMax", RangeMax);        // Максимум диапазона


  // ---------- Настройка графиков ----------
  loadGraph();

  dash.begin(); // Запуск дашборда

}

/* ---------- Loop ---------- */
void loop() {
  wifiModuleLoop();
  // Обновление времени через NTP
  if(wifiIsConnected()){
    timeClient.update();
  }
  CurrentTime = timeClient.getFormattedTime();  // Получение текущего времени

  // Генерация случайных значений для демонстрации
  RandomVal = random(0,50);                     // Случайное число
  Speed = random(150, 350) / 10.0f;            // Случайная скорость
  Temperatura = random(220, 320) / 10.0f;      // Случайная температура

  // Формирование информационных строк
  InfoString = "Random value is " + String(RandomVal) + " at " + CurrentTime;
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
  DaysSelect = ({ String out=""; String d[7]={"Mon","Tue","Wed","Thu","Fri","Sat","Sun"}; 
    for(int i=0;i<7;i++) if(random(0,2)) out += (out==""?"":",") + d[i]; out == "" ? "Mon" : out; });
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
  Comment = (String[]){"Все ок 👍","Работает как зверь 🦾","Сегодня повезёт! ✨","Турбо-режим активирован 🚀","Отличный выбор 😉","Готово! 🔧","Запускаю магию 🪄","Миссия выполнена ✅"}[random(0,8)];
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


}
  







