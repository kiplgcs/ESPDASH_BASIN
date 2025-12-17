#include <Arduino.h>
#include <JeeUI2.h>
#include "LittleFS.h"
jeeui2 jee;
#include "interface.h"

#define MAN "MAN"
//#define TEM "TEM"
//#define HUM "HUM"

#include "SPI.h"

#include <time.h>

#include "ds18.h" //Измеряем температуру с помощью DS18B20

/************************* Подключаем библиотеку  АЦП модуль ADS1115 16-бит *********************************/
#include <Adafruit_ADS1X15.h> // Библиотека для работы с модулями ADS1115 и ADS1015
//Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
Adafruit_ADS1115 ads1; // Первый ADS1115 - PH
Adafruit_ADS1115 ads2; // Второй ADS1115 - Хлор 
//Adafruit_ADS1015 ads;     /* Use this for the 12-bit version */

#include "ModbusRTU_RS485.h"

#include "WiFi-MQTT.h"   //Мультиподключение к WiFi и проверка работы MQTT
#include "Nextion_Rx.h"
#include "Nextion_Tx.h"
#include "NPT_Time.h"     //NTP (время из интернета)

#include <ArduinoJson.h>
#include <Timer_Relay.h> // Контроль таймеров времени и включения по таймерам

#include "LED_WS2815.h"

#include "PH_CL2.h"

#include "WebUpdate.h" //Загрузка скетча по WiFi

#include <Slow.h> //Периодически выполняем  - для обратной связи с устройствами

#include <esp_wifi.h> // Библиотека для реализации режима сна модема
//#include <esp_sleep.h> // Библиотека для реализации режима сна ESP32


void slow(int interval); //Периодически отправляем данные по MQTT
void mqttCallback(String topic, String payload);
void onConnect();




// #define FORMAT_SPIFFS_IF_FAILED true


void setup() {

  //  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
  //         Serial.println("SPIFFS Mount Failed");
  //         return;
  //     }

  // Попытка монтировать LittleFS, автоформат при ошибке
  if (!LittleFS.begin(true)) {   // true = форматировать, если FS битый
      Serial.println("LittleFS mount failed");
      // сюда можно сделать fallback или остановку
  } else {
      Serial.println("LittleFS mounted OK");
  }

  // esp_task_wdt_init(10, true); // Настройка сторожевого таймера - Watchdog Timer на 10 секунд - если ESP32 зависнет


	Serial.begin(115200);
	//jee.mqtt("192.168.0.100", 1883, "", "", mqttCallback, onConnect,  true); // суперфункция, обеспечит всю работу с mqtt, последний аргумент - разрешение удаленного управления
  //jee.udp(); // Ответ на UDP запрс. port 4243
	//jee.ap(5000); // Таймаут - в течении которого, если мы не сможем подключиться к сети STA - запускаем точку доступа AP (если закоментировать - то будем постоянно пытаться подключиться к точке STA)
  

	jee.ui(interface); //Создаем функцию с обратным вызовом с именем "interface" в которой будет весь Web интерфейс вместо отдельной вкладки файла html
  jee.update(update);
	jee.begin(true); //применяем и инициализируем Web сервер
	update();
  parameters(); // Запись переменных из EEPROM после перезапуска

  jee.mqtt("", 1883, "", "", mqttCallback, onConnect,  true); // суперфункция, обеспечит всю работу с mqtt, последний аргумент - разрешение удаленного управления


 /************************* Подключение к существующей Wi-Fi сети*********************************/
   
  MDNS.begin(jee.param("ap_ssid1").c_str());//Web имя при запросе: http://HostName
  WiFi.setHostname(jee.param("ap_ssid1").c_str()); //Имя в роутере
  WiFi.begin(jee.param("ssid1").c_str(), jee.param("pass1").c_str()); //   ESP.restart();
  /************************* Инициализация портов *********************************/

  /************************* Настраиваем  АЦП модуль ADS1115 16-бит *********************************/
  //Для подключения 16-битного АЦП ADS1115:
  // в базовом варианте платы NodeMCU-32S I2C интерфейс завязан на пины 21 и 22
  Wire.begin(23, 22);  // Инициализируем I2C с SDA  и SCL 

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
  /************************* DS18B20*********************************/
   setup_ds18(); //ищем датчики DS18B20 и выводим информацию о них

  //********************************** Modbus RTU ******************************************//

  //  Serial2.begin(57600, SERIAL_8N1, GPIO_NUM_17, GPIO_NUM_16); 
  // //Serial2.begin(19200);
  // pinMode(REDEPIN, OUTPUT);              // Задаем DE_RE как выход 
  // digitalWrite(REDEPIN, LOW);  
  // attachInterrupt(digitalPinToInterrupt(GPIO_NUM_17), ActivUARTInterrupt, RISING ); //CHANGE //FALLING //RISING); //Прерываем по пину 16 (RX2) порта №2 для выполнения функции.
  //**************************************************************************************//
  setup_Modbus();
//********************************** Modbus RTU ******************************************//


  /************************* Прошивка по WiFi*********************************/
  WebUpdate_setup(); //Загрузка скетча в ESP
  /**************************END setup********************************/
  
  

  

  setup_WS2815();


  // // Переход в режим сна модема, если потух экран Nextion
  // //https://voltiq.ru/esp32-sleep-modes-power-consumption/
  // if (Nx_dim_id < 20) {
  //   //esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

  //   // // Выключение WiFi перед входом в глубокий сон (опционально)
  //   // WiFi.disconnect(true);
  //   // WiFi.mode(WIFI_OFF);

  //   // Включение таймера для пробуждения через 5 секунд
  //   esp_sleep_enable_timer_wakeup(10 * 1000000); // 5 секунд в микросекундах
  //   //esp_light_sleep_start();// Вход в режим легкого сна
  //   esp_deep_sleep_start();// Вход в режим глубокого сна

  // } else{
  //   //esp_wifi_set_ps(WIFI_PS_NONE);
  // } 

}




// int Step = 1;  //Шаг SWITCH

// int Step1 = 0;


// // Глобальные переменные для динамических данных
// float Temp_before_heater = 25.0;
// float Temp_after_heater = 26.5;
// float CurrentPH = 7.2;
// float CurrentCl_mV = 500;
// int Lumen_Ul_value = 50;
// // bool Act_PH = true;
// // bool Act_Cl = true;
// // === Функция обновления динамических значений ===
// void updateInterface() {
//     // Генерация случайных значений
//     Temp_before_heater = random(200, 300) / 10.0; // 20.0 — 30.0
//     Temp_after_heater  = random(250, 350) / 10.0; // 25.0 — 35.0
//     CurrentPH          = random(65, 80) / 10.0;   // 6.5 — 8.0
//     CurrentCl_mV       = random(450, 650);        // 450 — 650 mV
//     Lumen_Ul_value     = random(0, 101);          // 0 — 100 %

//     // Обновление интерфейса
//     jee.text("DS1", String(Temp_before_heater, 1));
//     jee.text("DS2", String(Temp_after_heater, 1));

//     if (Act_PH) {
//         jee.text("PH1", String(CurrentPH, 2));
//     }

//     if (Act_Cl) {
//         jee.text("Cl", String(CurrentCl_mV, 0));
//     }

//     jee.text("Lumen_Ul", String(Lumen_Ul_value));
// }


void loop() {
	jee.handle();
  // esp_task_wdt_reset(); // Сброс Watchdog Timer - проверка не зависла ли микросхема и сброс таймера.

  slow(period_slow_Time); //Периодически отправляем данные для обратной связи - "period_slow_Time" Период обновления данных - зависит от "Nx_dim_id" nекущеuj - считанного значения яркости Nextion
  
  ControlWiFi(interval_Wifi_Connect); // Контроль подкдлючения к WiFi



  // updateInterface();




  //Без этого таймера опроса кнопок перегружался цикл
  if( (millis()- Timer_Callback) >1500) { Timer_Callback=millis(); 
  jee.btnCallback("ConnectWiFi", booootn); // Постоянно проверяем нажата ли кнопка
  jee.btnCallback("ConnectAP", btnAP);
  jee.btnCallback("RestartESP32", RestESP);
  if(Act_Cl) {jee.btnCallback("CalCl", btnCalCl);}
  }
 /**************************** *********************************************************************/
  Nextion_Transmit(500); // Отправка в Nextion по очереди
  //if(Power_Clean){Power_Filtr = false;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра
  // Проверяем, активирован ли триггер
  if (triggerActivated_Nextion) {/*NextionDelay (3000);*/ delay(2000); NextionDelay ();} //Отложенное чтение по флагу - переменных из Nextion, связанное с поздним обновление в Nextion информации.
  if (triggerRestartNextion) {/*RestartNextionDelay(5000); */ delay(3000); RestartNextionDelay();} //Для отложенного чтения всех необходимых переменных после перезагрузки ESP
  ///////////////////////////////////////////////////////////////////////////////////////
   Temp_DS18B20(5000); //Измеряем температуру
 /**************************** *********************************************************************/
  Nextion_Transmit(500); // Отправка в Nextion по очереди
  if(Power_Clean){Power_Filtr = false;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра
    /****************************  Счет и синхронизация времени между интернетом - монитором - ESP32 *****************************************/
  NPT_Time(period_get_NPT_Time); // раз в 5сек/3 мин. - Обновление времени из Интерента
  getTimeFromRTC(1000); // Считаем время (1 раз в секунду) в цикле для работы программы.
  TimeRTC(2000); // Обновление данных о времени на Web странице
  /**************************** ********************************************************************************************************************/
  TimerControlRelay(10000);  // TimerControlRelay(600); //Контроль включения реле по таймерам
  ControlModbusRelay(5000); // Контролируем актуальное состояние реле - отправлем данные по ModBuss На плату


  loop_WS2815();

  if(Act_PH) {loop_PH(1500); Act_Cl=false;} else {loop_PH(20000);} //Если калибруем, то вызываем чаще
  if(Act_Cl) {loop_CL2(1600); Act_PH=false;} else {loop_CL2(20100);}  //Если калибруем, то вызываем чаще


  //loop_Modbus(5000);


  ///////////////////////////////////////////////////////////////////////


 ///****************************  Nextion - проверка прихода данных Tx/Rx ****************************************/
  while (MySerial.available()){myNex.NextionListen();} //для непрерывного чтения данных из порта, пока они доступны

 /**************************** Загрузка скетчи по WiFi ******************************/
  AsyncElegantOTA.loop(); //Открываем WEB  - проверяем обращение к серверу



  // // Переход в режим сна модема, если потух экран Nextion
  // //https://voltiq.ru/esp32-sleep-modes-power-consumption/
  // if (Nx_dim_id < 20) {
  //   //esp_wifi_set_ps(WIFI_PS_MIN_MODEM);

  //   // // Выключение WiFi перед входом в глубокий сон (опционально)
  //   // WiFi.disconnect(true);
  //   // WiFi.mode(WIFI_OFF);

  //   // Включение таймера для пробуждения через 5 секунд
  //   esp_sleep_enable_timer_wakeup(10 * 1000000); // 5 секунд в микросекундах
  //   esp_light_sleep_start();// Вход в режим легкого сна
  //   //esp_deep_sleep_start();// Вход в режим глубокого сна

  // } else if(Nx_dim_id > 20 && Slep){ Slep = false
  //   ControlWiFi(0);
  // } 
  
} /************************END - loop**********************************/









  //////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////////



// void slow(int interval){ // Обратная связь - раз в XXX сек 
// static unsigned long timer;
// if (interval + timer > millis()) return;
// timer = millis();


// if(Lamp==true){ // периодически проверяем  - обновляем если была перезагрузка
// Error err =  RS485.addRequest(40001,       1,       0x05,       0,     0xFF00); //R1_ON  // Включить реле №1
// myNex.writeNum("page0.b0.pic", 2); 
// } else if(Lamp==false) {
// Error err =  RS485.addRequest(40001,       1,       0x05,       0,     0x0000); //R1_ON  // Отключить реле №1
// myNex.writeNum("page0.b0.pic", 1); 
// }



// // if(Sider_heat>DS11+0.1){ //Передаем на Nextion -пераметр для включения или отключения нагрев воды в бассейне по температуре воды 
// //   Error err =  RS485.addRequest(40001,       1,       0x05,       1,     0xFF00); //R2_ON  // Включить реле №2
// //   myNex.writeNum("page0.va0.val", 1);
// //   Sider_heat1=false;
// //   } else if (Sider_heat<DS11-0.1) {
// //   Error err =  RS485.addRequest(40001,       1,       0x05,       1,     0x0000); //R2_ON  // Отключить реле №2
// //   myNex.writeNum("page0.va0.val", 0);
// //   Sider_heat1=true;
// //   }


// if(Filtr==true && Filtr1==true){
// Error err =  RS485.addRequest(40001,       1,       0x05,       2,     0xFF00); //R1_ON  // Включить реле №3
// myNex.writeNum("page0.b1.pic", 4); 
// Filtr1=false;
// } else if (Filtr == false && Filtr1==false){
// Error err =  RS485.addRequest(40001,       1,       0x05,       2,     0x0000); //R1_ON  // Отключить реле №3
// myNex.writeNum("page0.b1.pic", 3); 
// Filtr1=true;
// }


// // if(Power_LED== true){jee.var("Power_LED", "true");} else if (Power_LED == false){jee.var("Power_LED", "false");}
// // if(Power_Nasos == true){jee.var("Power_Nasos", "true");} else if (Power_Nasos == false){jee.var("Power_Nasos", "false");}
// // if(Power_Hlor == true){jee.var("Power_Hlor", "true");} else if (Power_Hlor == false){jee.var("Power_Hlor", "false");}
// // if(Power_Waterfall == true){jee.var("Power_Waterfall", "true");} else if (Power_Waterfall == false){jee.var("Power_Waterfall", "false");}

// jee.var("RSSI_WiFi", String(WiFi.RSSI())); // выводим мощность принимаемого сигнала


//   // jee.var("new_bright", String(new_bright));
//   // jee.var("autoplayDuration", String(autoplayDuration)); 

  
//   //rtc.get(&Sec, &min1, &hour, &Day0, &Month0, &Year0); // Чтение времени из часов DS1307
//   //jee.var("Time", "Время : "+String(hour)+":"+String(min1)+", Дата : " +String(Day0)+":"+String(Month0)+":"+String(Year0)+"г."); //Выводим в Интерфейс и отправляем по MQTT
//   //if(jee.param("Time") == "null") {ESP.restart();}

// }









//////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////





void onConnect(){ 

}

void mqttCallback(String topic, String payload){ 
  	Serial.println("Message [" + topic + " - " + payload + "] ");

}



