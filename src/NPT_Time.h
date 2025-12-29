// Подключаем необходимые библиотеки
#pragma once

#include <WiFiUdp.h> // Библиотека для работы с UDP-протоколом
#include <NTPClient.h> // Библиотека для работы с NTP-клиентом
#include "EasyNextionLibrary.h"
#include <Wire.h>
#include <time.h>
#include "wifi_manager.h"
//#include <RTClib.h>
//#include <TimeLib.h>
//#include <Adafruit_I2CDevice.h

// Параметры NTP-сервера
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.google.com";
//int gmtOffset_correct = 3; //GMT+3:00, Москва/Краснодар
const int daylightOffset = 0; // Смещение летнего времени (0 - отключено)

inline int period_get_NPT_Time = 5000; // 10сек./3мин - время через которое будет обновлятся время из интерента

// Внешние зависимости из других модулей
extern WiFiUDP ntpUDP;
extern EasyNex myNex;
extern int Saved_gmtOffset_correct;
extern int gmtOffset_correct;

// Переменные времени по умолчанию
inline int npt_seconds, seconds;
inline int npt_minutes, minutes;
inline int npt_hours, hours;
inline int npt_Day, Day;        //День месяца
inline int npt_Month, Month;      //Месяц
inline int npt_Year, Year;       //Год

inline int npt_DayOfWeek=1, DayOfWeek=1;  //День недели - Значение от 1 (Понедельник) до 7 (воскресенье)
const char* daysOfWeek[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};


const int kNextionInvalidValue = 777777;
inline const char *kLastEpochKey = "lastEpoch"; // Ключ в NVS для хранения последнего известного времени (epoch)
inline time_t baseEpoch = 0;                    // Базовый epoch, от которого ведем отсчет
inline unsigned long baseEpochMillis = 0;       // millis() в момент установки baseEpoch
inline bool baseEpochReady = false;             // Флаг, что базовое время валидно
inline time_t lastSavedEpoch = 0;               // Последний epoch, сохраненный в NVS

bool isValidDateTime(int year, int month, int day, int hour, int minute, int second) {
  if (year < 2024 || year > 2040) return false;
  if (month < 1 || month > 12) return false;
  if (day < 1 || day > 31) return false;
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;
  return true;
}

bool isValidDayOfWeek(int dayOfWeek) {
  return dayOfWeek >= 1 && dayOfWeek <= 7;
}
// Собираем epoch из компонентов даты/времени, чтобы работать единым форматом времени
time_t buildEpoch(int year, int month, int day, int hour, int minute, int second) {
  struct tm timeInfo = {};
  timeInfo.tm_year = year - 1900;
  timeInfo.tm_mon = month - 1;
  timeInfo.tm_mday = day;
  timeInfo.tm_hour = hour;
  timeInfo.tm_min = minute;
  timeInfo.tm_sec = second;
  return mktime(&timeInfo);
}
// Раскладываем epoch обратно в компоненты даты/времени для UI/логики
void syncComponentsFromEpoch(time_t epoch) {
  struct tm *tmInfo = localtime(&epoch);
  if (!tmInfo) return;
  seconds = tmInfo->tm_sec;
  minutes = tmInfo->tm_min;
  hours = tmInfo->tm_hour;
  Day = tmInfo->tm_mday;
  Month = tmInfo->tm_mon + 1;
  Year = tmInfo->tm_year + 1900;
  int normalizedDayOfWeek = (tmInfo->tm_wday + 6) % 7 + 1;
  DayOfWeek = normalizedDayOfWeek;
}
// Устанавливаем базовое время: фиксируем epoch, стартовую метку millis и сохраняем в NVS
// Сохраняем не чаще раза в минуту, чтобы не изнашивать память
void setBaseEpoch(time_t epoch) {
  if (epoch <= 0) return;
  baseEpoch = epoch;
  baseEpochMillis = millis();
  baseEpochReady = true;
  syncComponentsFromEpoch(epoch);
  if (lastSavedEpoch == 0 || epoch > lastSavedEpoch + 60 || epoch + 60 < lastSavedEpoch) {
    saveValue<int>(kLastEpochKey, static_cast<int>(epoch));
    lastSavedEpoch = epoch;
  }
}
// Загружаем последнее известное время из NVS (если нет Nextion/Интернета)
bool loadBaseEpochFromStorage() {
  int storedEpoch = loadValue<int>(kLastEpochKey, 0);
  if (storedEpoch > 0) {
    setBaseEpoch(static_cast<time_t>(storedEpoch));
    return true;
  }
  return false;
}
// Получаем текущее время: baseEpoch + прошедшие секунды от millis
time_t getCurrentEpoch() {
  if (!baseEpochReady) return 0;
  unsigned long elapsedSeconds = (millis() - baseEpochMillis) / 1000;
  return baseEpoch + elapsedSeconds;
}
// Вспомогательная функция для расписаний (часы/минуты из единого источника времени)
bool getCurrentHourMinute(int &currentHour, int &currentMinute) {
  time_t epoch = getCurrentEpoch();
  if (epoch <= 0) return false;
  struct tm *tmInfo = localtime(&epoch);
  if (!tmInfo) return false;
  currentHour = tmInfo->tm_hour;
  currentMinute = tmInfo->tm_min;
  return true;
}
// Форматирование epoch в строку для веб-интерфейса
String formatEpoch(time_t epoch) {
  if (epoch <= 0) return String("--");
  struct tm *tmInfo = localtime(&epoch);
  if (!tmInfo) return String("--");
  char buf[20];
  strftime(buf, sizeof(buf), "%d.%m.%Y %H:%M:%S", tmInfo);
  return String(buf);
}
// Текущее время в строковом виде для UI
String getCurrentDateTime() {
  return formatEpoch(getCurrentEpoch());
}

// Читаем время из Nextion RTC и валидируем значения
bool fetchNextionTime(int &readSeconds, int &readMinutes, int &readHours, int &readDay,
                      int &readMonth, int &readYear, int &readDayOfWeek) {
  int hoursValue = myNex.readNumber("rtc3");
  if (hoursValue == kNextionInvalidValue) {
    return false;
  }

  int secondsValue = myNex.readNumber("rtc5"); delay(50);
  int minutesValue = myNex.readNumber("rtc4"); delay(50);
  int dayValue = myNex.readNumber("rtc2"); delay(50);
  int monthValue = myNex.readNumber("rtc1"); delay(50);
  int yearValue = myNex.readNumber("rtc0");
  int day_OfWeek = myNex.readNumber("rtc6");

  if (secondsValue == kNextionInvalidValue || minutesValue == kNextionInvalidValue ||
      dayValue == kNextionInvalidValue || monthValue == kNextionInvalidValue ||
      yearValue == kNextionInvalidValue || day_OfWeek == kNextionInvalidValue) {
    return false;
  }

  int normalizedDayOfWeek = day_OfWeek == 0 ? 7 : day_OfWeek;
  if (!isValidDateTime(yearValue, monthValue, dayValue, hoursValue, minutesValue, secondsValue)) {
    return false;
  }
  if (!isValidDayOfWeek(normalizedDayOfWeek)) {
    return false;
  }

  readSeconds = secondsValue;
  readMinutes = minutesValue;
  readHours = hoursValue;
  readDay = dayValue;
  readMonth = monthValue;
  readYear = yearValue;
  readDayOfWeek = normalizedDayOfWeek;
  return true;
}
// Пробуем взять время из Nextion и сразу установить baseEpoch
bool readNextionTime() {
  int readSeconds = 0;
  int readMinutes = 0;
  int readHours = 0;
  int readDay = 0;
  int readMonth = 0;
  int readYear = 0;
  int readDayOfWeek = 0;
  if (!fetchNextionTime(readSeconds, readMinutes, readHours, readDay, readMonth, readYear, readDayOfWeek)) {
    return false;
  }

  time_t epoch = buildEpoch(readYear, readMonth, readDay, readHours, readMinutes, readSeconds);
  if (epoch <= 0) {
    return false;
  }
  setBaseEpoch(epoch);
  return true;
}

bool isSameTimeDate(int hourA, int minuteA, int secondA, int dayA, int monthA, int yearA,
                    int hourB, int minuteB, int secondB, int dayB, int monthB, int yearB) {
  return hourA == hourB && minuteA == minuteB && secondA == secondB &&
         dayA == dayB && monthA == monthB && yearA == yearB;
}

int iii=1; //переменная для счета до отправки на Web -Обновляенм время на первой странице
void getTimeFromRTC(int interval_TimeFromRTC) {
  static unsigned long timer;
if (interval_TimeFromRTC + timer > millis()) return; 
timer = millis();
//---------------------------------------------------------------------------------

// Увеличиваем время на одну секунду
  seconds++;
  
  // Проверяем, достигли ли 60 секунд
  if (seconds >= 60) {
    seconds = 0;
    minutes++;
    
    // Проверяем, достигли ли 60 минут
    if (minutes >= 60) {
      minutes = 0;
      hours++;  

      // Проверяем, достигли ли 24 часов
      if (hours >= 24) {
        hours = 0;
      }
    }
  }


  iii++;
  if(iii>=3) {iii=1; delay(5);}
}


void TimeRTC(int interval_RTC) {
  static unsigned long timer;
if (interval_RTC + timer > millis()) return; 
timer = millis();
//---------------------------------------------------------------------------------



  

}



// Функция для провирки наличия Интернета
bool checkInternetAvailability() {

  if (WiFi.status() != WL_CONNECTED) {return false;}  // WiFi недоступен -  Интернет недоступен

  const char* host = "www.google.com";  // Имя хоста для проверки подключения

  WiFiClient client;
  if (client.connect(host, 80)) {  // Попытка подключения к хосту на порт 80
    client.stop();  // Закрываем соединение
    return true;  // Интернет доступен
  } else {
    return false;  // Интернет недоступен
  }
}

// Основная синхронизация времени.
// Порядок действий:
// 1) Пытаемся прочитать время из Nextion (если доступен) и использовать как базу.
// 2) Если есть Интернет — запрашиваем NTP и обновляем базу (и при необходимости Nextion).
// 3) Если Интернета нет и Nextion недоступен — поднимаем время из NVS.
void NPT_Time(int interval_NPT_Time){ // переодически синхронизируем время из Интеренета
static unsigned long timer;
if (interval_NPT_Time + timer > millis()) return; 
timer = millis();
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
  int nextionSeconds = 0;
  int nextionMinutes = 0;
  int nextionHours = 0;
  int nextionDay = 0;
  int nextionMonth = 0;
  int nextionYear = 0;
  int nextionDayOfWeek = 0;
    // 1) Пытаемся читать время из Nextion (RTC) — это приоритетный локальный источник
  bool nextionAvailable = fetchNextionTime(nextionSeconds, nextionMinutes, nextionHours, nextionDay, nextionMonth, nextionYear, nextionDayOfWeek);
  if (nextionAvailable) {
    time_t nextionEpoch = buildEpoch(nextionYear, nextionMonth, nextionDay, nextionHours, nextionMinutes, nextionSeconds);
    if (nextionEpoch > 0) {
      setBaseEpoch(nextionEpoch);
    }
  }

// 2) Если Интернет доступен — берём NTP и синхронизируем всё
if (checkInternetAvailability()) { //Если Интерент доступен

    int gmtOffsetNextion = myNex.readNumber("pageRTC.n5.val"); delay(50);
    if (gmtOffsetNextion != kNextionInvalidValue) {
      Saved_gmtOffset_correct = gmtOffset_correct = gmtOffsetNextion;
    }

    if(gmtOffset_correct <= 1 || gmtOffset_correct >= 10){gmtOffset_correct=3;}

    // NTPClient timeClient(ntpUDP, ntpServer, 3600*gmtOffset_correct, daylightOffset); //Для корректировки часового пояса, если вдруг другой часовой установлен
    NTPClient timeClient1(ntpUDP, ntpServer1, 3600 * gmtOffset_correct, daylightOffset);
    NTPClient timeClient2(ntpUDP, ntpServer2, 3600 * gmtOffset_correct, daylightOffset);
    // timeClient.update(); //функция для получения текущей даты и времени с NTP-сервера.
      

    // Запрос времени с первого сервера
    timeClient1.begin();
    timeClient1.update();

    time_t epochTime = timeClient1.getEpochTime();
    if (epochTime == 0) {
      timeClient2.begin();
      timeClient2.update();
      epochTime = timeClient2.getEpochTime();
    }

      timeClient2.begin();
      timeClient2.update();

    if (epochTime != 0) {
      tm *timeInfo = localtime(&epochTime);
      if (timeInfo != nullptr) {
        npt_hours = timeInfo->tm_hour;
        npt_minutes = timeInfo->tm_min;
        npt_seconds = timeInfo->tm_sec;
        npt_Day = timeInfo->tm_mday;
        npt_Month = timeInfo->tm_mon + 1;
        npt_Year = timeInfo->tm_year + 1900;
        npt_DayOfWeek = (timeInfo->tm_wday + 6) % 7 + 1;
      }
    }

if(isValidDateTime(npt_Year, npt_Month, npt_Day, npt_hours, npt_minutes, npt_seconds)) { period_get_NPT_Time = 60000; //Большой таймер повторных запросов - если время считали правильно
      setBaseEpoch(epochTime);
     // Если Nextion доступен и его время отличается — обновляем RTC на дисплее
      bool shouldUpdateNextion = nextionAvailable && !isSameTimeDate(
        npt_hours, npt_minutes, npt_seconds, npt_Day, npt_Month, npt_Year,
        nextionHours, nextionMinutes, nextionSeconds, nextionDay, nextionMonth, nextionYear
      );

      DayOfWeek = npt_DayOfWeek; //Записываем корректный день недели
      
      if (shouldUpdateNextion) {
        myNex.writeStr("rtc5=" + String(npt_seconds));
        myNex.writeStr("rtc4=" + String(npt_minutes));
        myNex.writeStr("rtc3=" + String(npt_hours));
        myNex.writeStr("rtc2=" + String(npt_Day));
        myNex.writeStr("rtc1=" + String(npt_Month));
        myNex.writeStr("rtc0=" + String(npt_Year));
        myNex.writeStr("rtc6=" + String(npt_DayOfWeek == 7 ? 0 : npt_DayOfWeek));
      }
    } else {
      period_get_NPT_Time = 5000; //Короткий таймер повторных запросов - если время считали не правильно или нет интернета
    }
  } else {
        // 3) Интернета нет: если Nextion не ответил и базового времени еще нет — берём из NVS
    if (!nextionAvailable && !baseEpochReady) {
      loadBaseEpochFromStorage();
    }
  }
  
  
  
} //Закрываем общую функцию таймера синхронизации времени
