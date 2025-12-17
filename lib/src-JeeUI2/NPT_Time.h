// Подключаем необходимые библиотеки
//#include <WiFiUdp.h> // Библиотека для работы с UDP-протоколом
#include <NTPClient.h> // Библиотека для работы с NTP-клиентом
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


// Объект NTP-клиента для обновления времени из Интернета
WiFiUDP ntpUDP;
// NTPClient timeClient1(ntpUDP, ntpServer1, 3600 * gmtOffset_correct, daylightOffset);
// NTPClient timeClient2(ntpUDP, ntpServer2, 3600 * gmtOffset_correct, daylightOffset);


// Переменные времени по умолчанию
int npt_seconds, seconds;
int npt_minutes, minutes;
int npt_hours, hours;
int npt_Day, Day;        //День месяца
int npt_Month, Month;      //Месяц
int npt_Year, Year;       //Год

int npt_DayOfWeek=1, DayOfWeek=1;  //День недели - Значение от 1 (Понедельник) до 7 (воскресенье)
const char* daysOfWeek[] = {"ПН", "ВТ", "СР", "ЧТ", "ПТ", "СБ", "ВС"};



// void setup_rtc() {
// //setTime(0, 0, 0, 1, 1, 2023); // Установка времени 00:00:00 1 января 2023 года
// // Настройка и запуск NTP-клиента
// //timeClient.begin();
// //configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
// }

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
  if(iii>=3) {iii=1; jee.var("Time", ""); delay(5); if(!Act_PH && !Act_Cl) jee.var("Time", String(hours) + ":" + String(minutes) + ":" + String(seconds) + ", " + String(Day) + ":" + String(Month) + ":" + String(Year) + "г, " + String(daysOfWeek[DayOfWeek - 1]));}
  
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


   
    Saved_gmtOffset_correct = gmtOffset_correct = myNex.readNumber("pageRTC.n5.val"); delay(50);

    if(gmtOffset_correct > 1 && gmtOffset_correct < 10){jee.var("gmtOffset_correct", String(gmtOffset_correct)); } else {gmtOffset_correct=3;}

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
      if(npt_Year >= 2024 && npt_Year < 2040) { period_get_NPT_Time = 60000; //Большой таймер повторных запросов - если время считали правильно
        
        seconds = npt_seconds; myNex.writeStr("rtc5=" + String(seconds));
        minutes = npt_minutes; myNex.writeStr("rtc4=" + String(minutes));
        hours = npt_hours; myNex.writeStr("rtc3=" + String(hours));
        Day = npt_Day; myNex.writeStr("rtc2=" + String(Day));
        Month = npt_Month; myNex.writeStr("rtc1=" + String(Month));
        Year = npt_Year; myNex.writeStr("rtc0=" + String(Year));
        DayOfWeek = npt_DayOfWeek; //Записываем корректный день недели
           
      } else {
          period_get_NPT_Time = 5000; //Короткий таймер повторных запросов - если время считали не правильно или нет интернета
        }

  } else {//Если нет Интернета, то время запрашиваем из монитора Nextion:


    if (myNex.readNumber("rtc3") != 777777) {
        

    seconds = myNex.readNumber("rtc5"); delay(50);
    minutes = myNex.readNumber("rtc4"); delay(50);
    hours = myNex.readNumber("rtc3"); delay(50);
    Day = myNex.readNumber("rtc2"); delay(50);
    Month = myNex.readNumber("rtc1"); delay(50);
    Year = myNex.readNumber("rtc0"); 

     int day_OfWeek = myNex.readNumber("rtc6"); // Читаем день недели - Значение от 0 (воскресенье) до 6 (суббота)
    //DayOfWeek = (day_OfWeek == 0) ? 7 : day_OfWeek; // Преобразуем значения от 0-6 в 1-7 (понедельник-воскресенье) - вызывает перезагрузку ЦП в вледствии несовпадения данные если Nextion отключен
    
    if (day_OfWeek == 0) {DayOfWeek = 7;} else if (day_OfWeek > 1 && day_OfWeek < 7) {DayOfWeek = day_OfWeek;}// Преобразуем значения от 0-6 в 1-7 (понедельник-воскресенье)

    }
  }
  
  
  
} //Закрываем общую функцию таймера синхронизации времени


