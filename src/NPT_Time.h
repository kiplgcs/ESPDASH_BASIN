// Подключаем необходимые библиотеки
#include <WiFiUdp.h> // Библиотека для работы с UDP-протоколом
#include <NTPClient.h> // Библиотека для работы с NTP-клиентом
#include "EasyNextionLibrary.h"
#include <Wire.h>
//#include <RTClib.h>
//#include <TimeLib.h>
//#include <Adafruit_I2CDevice.h

// Параметры NTP-сервера
const char *ntpServer1 = "pool.ntp.org";
const char *ntpServer2 = "time.google.com";
//int gmtOffset_correct = 3; //GMT+3:00, Москва/Краснодар
const int daylightOffset = 0; // Смещение летнего времени (0 - отключено)

int period_get_NPT_Time = 5000; // 10сек./3мин - время через которое будет обновлятся время из интерента

// Внешние зависимости из других модулей
extern WiFiUDP ntpUDP;
extern EasyNex myNex;
extern int Saved_gmtOffset_correct;
extern int gmtOffset_correct;

// Переменные времени по умолчанию
int npt_seconds, seconds;
int npt_minutes, minutes;
int npt_hours, hours;
int npt_Day, Day;        //День месяца
int npt_Month, Month;      //Месяц
int npt_Year, Year;       //Год

int npt_DayOfWeek=1, DayOfWeek=1;  //День недели - Значение от 1 (Понедельник) до 7 (воскресенье)
const char* daysOfWeek[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};


const int kNextionInvalidValue = 777777;

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

  seconds = readSeconds;
  minutes = readMinutes;
  hours = readHours;
  Day = readDay;
  Month = readMonth;
  Year = readYear;
  DayOfWeek = readDayOfWeek;
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


void NPT_Time(int interval_NPT_Time){ // переодически синхронизируем время из Интеренета
static unsigned long timer;
if (interval_NPT_Time + timer > millis()) return; 
timer = millis();
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
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

    // Проверка на корректность времени с первого сервера
    if (timeClient1.getEpochTime() != 0) {
    
      npt_hours = timeClient1.getHours();
      npt_minutes = timeClient1.getMinutes();
      npt_seconds = timeClient1.getSeconds();
      
      // Извлекаем месяц и год из эпохального времени
      time_t epochTime = timeClient1.getEpochTime();
      tm *timeInfo = localtime(&epochTime);

      npt_DayOfWeek = (timeInfo->tm_wday + 6) % 7 + 1; // Преобразуем значения от 0-6 в 1-7 (понедельник-воскресенье)
      npt_Day = timeInfo->tm_mday;
      npt_Month = timeInfo->tm_mon + 1;  // Месяцы в C начинаются с 0, поэтому добавляем 1
      npt_Year = timeInfo->tm_year + 1900;  // Добавляем 1900, так как struct tm хранит год с 1900

    } else {
      // Если время с первого сервера некорректно, запрашиваем с второго сервера
      timeClient2.begin();
      timeClient2.update();

      // Проверка на корректность времени с второго сервера
      if (timeClient2.getEpochTime() != 0) {
      
        npt_hours = timeClient2.getHours();
        npt_minutes = timeClient2.getMinutes();
        npt_seconds = timeClient2.getSeconds();

        // Извлекаем месяц и год из эпохального времени
        time_t epochTime = timeClient2.getEpochTime();
        tm *timeInfo = localtime(&epochTime);
        
     
        npt_DayOfWeek = (timeInfo->tm_wday + 1) % 7 + 1; // Преобразуем значения от 0-6 в 1-7 (понедельник-воскресенье)
        npt_Day = timeInfo->tm_mday;
        npt_Month = timeInfo->tm_mon + 1;  // Месяцы в C начинаются с 0, поэтому добавляем 1
        npt_Year = timeInfo->tm_year + 1900;  // Добавляем 1900, так как struct tm хранит год с 1900
        
      }  
    }//Закрываем функцию проверки успешности получения эпохального времени
    

        //проверяем правильно ли получено время (по году) из интернета. 
      // if(npt_Year >= 2024 && npt_Year < 2040) { period_get_NPT_Time = 60000; //Большой таймер повторных запросов - если время считали правильно
        
      //   seconds = npt_seconds; myNex.writeStr("rtc5=" + String(seconds));
      //   minutes = npt_minutes; myNex.writeStr("rtc4=" + String(minutes));
      //   hours = npt_hours; myNex.writeStr("rtc3=" + String(hours));
      //   Day = npt_Day; myNex.writeStr("rtc2=" + String(Day));
      //   Month = npt_Month; myNex.writeStr("rtc1=" + String(Month));
      //   Year = npt_Year; myNex.writeStr("rtc0=" + String(Year));
      if(isValidDateTime(npt_Year, npt_Month, npt_Day, npt_hours, npt_minutes, npt_seconds)) { period_get_NPT_Time = 60000; //Большой таймер повторных запросов - если время считали правильно
        int nextionSeconds = 0;
        int nextionMinutes = 0;
        int nextionHours = 0;
        int nextionDay = 0;
        int nextionMonth = 0;
        int nextionYear = 0;
        int nextionDayOfWeek = 0;
        bool nextionAvailable = fetchNextionTime(nextionSeconds, nextionMinutes, nextionHours, nextionDay, nextionMonth, nextionYear, nextionDayOfWeek);

        bool shouldUpdateNextion = nextionAvailable && !isSameTimeDate(
          npt_hours, npt_minutes, npt_seconds, npt_Day, npt_Month, npt_Year,
          nextionHours, nextionMinutes, nextionSeconds, nextionDay, nextionMonth, nextionYear
        );

        seconds = npt_seconds;
        minutes = npt_minutes;
        hours = npt_hours;
        Day = npt_Day;
        Month = npt_Month;
        Year = npt_Year;

        DayOfWeek = npt_DayOfWeek; //Записываем корректный день недели
          
        
        if (shouldUpdateNextion) {
          myNex.writeStr("rtc5=" + String(seconds));
          myNex.writeStr("rtc4=" + String(minutes));
          myNex.writeStr("rtc3=" + String(hours));
          myNex.writeStr("rtc2=" + String(Day));
          myNex.writeStr("rtc1=" + String(Month));
          myNex.writeStr("rtc0=" + String(Year));
          myNex.writeStr("rtc6=" + String(DayOfWeek == 7 ? 0 : DayOfWeek));
        }

      } else {
          period_get_NPT_Time = 5000; //Короткий таймер повторных запросов - если время считали не правильно или нет интернета
        }

  } else {//Если нет Интернета, то время запрашиваем из монитора Nextion:


        readNextionTime();
  }
  
  
  
} //Закрываем общую функцию таймера синхронизации времени
