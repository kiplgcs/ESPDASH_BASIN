#pragma once

#include <Arduino.h>
#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "wifi_manager.h" // Хранилище настроек и кнопок для сохранения адресов DS18B20.

DeviceAddress sensor0 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Адрес датчика помещения (по умолчанию не назначен).
DeviceAddress sensor1= {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Адрес датчика улицы (по умолчанию не назначен).
constexpr size_t kMaxDs18Sensors = 16; // Максимум адресов для хранения (для выбора в UI).
inline DeviceAddress ds18Found[kMaxDs18Sensors]; // Буфер найденных адресов DS18B20.
inline int ds18FoundCount = 0; // Количество найденных датчиков на шине.

#define ONE_WIRE_BUS 2  // Пин для подключения DS18B20
// #define LOW_ALARM 30               // Задаем нижний порог температурной тревоги
// #define HIGH_ALARM 40              // Задаем верхний порог температурной тревоги

OneWire oneWire(ONE_WIRE_BUS);      // Инициализируем объект OneWire
DallasTemperature sensors(&oneWire); // Инициализируем объект DallasTemperature

// Ключи NVS для привязки датчиков должны быть не длиннее 15 символов (ограничение ESP32 Preferences).
constexpr const char *kDs18Sensor0StorageKey = "ds18_s0_addr";
constexpr const char *kDs18Sensor1StorageKey = "ds18_s1_addr";
constexpr const char *kDs18Sensor0IndexStorageKey = "ds18_s0_idx";
constexpr const char *kDs18Sensor1IndexStorageKey = "ds18_s1_idx";

// float DS1, Saved_DS1, DS2, Saved_DS2; 
//String DS11, DS22;   // Строковая переменная для хранения температуры

float DS1 = 0, Saved_DS1;
float DS2 = 0, Saved_DS2;
inline bool DS1Available = false;
inline bool DS2Available = false;
inline bool DS1Assigned = false;
inline bool DS2Assigned = false;

inline String formatTemperatureString(float value, bool available) {
  return available ? (String(value, 1) + " \u00B0C") : "n/a";
}

inline String formatDeviceAddress(const DeviceAddress &addr) { // Форматируем адрес DS18B20 в строку.
  String result; // Строка для накопления результата.
  for (int i = 0; i < 8; i++) { // Проходим 8 байт адреса.
    if (addr[i] < 16) result += "0"; // Добавляем ведущий ноль для однобайтных значений.
    result += String(addr[i], HEX); // Записываем байт адреса в HEX.
    if (i < 7) result += "-"; // Разделяем байты дефисами.
  }
  result.toUpperCase(); // Приводим к верхнему регистру для читаемости.
  return result; // Возвращаем строковое представление адреса.
}

inline bool deviceAddressEquals(const DeviceAddress &left, const DeviceAddress &right) { // Сравниваем два адреса.
  return memcmp(left, right, sizeof(DeviceAddress)) == 0; // Возвращаем результат сравнения.
}

inline bool deviceAddressIsZero(const DeviceAddress &addr) { // Проверяем, назначен ли адрес.
  for (uint8_t i = 0; i < sizeof(DeviceAddress); i++) {
    if (addr[i] != 0x00) return false;
  }
  return true;
}

inline bool parseDeviceAddressString(const String &input, DeviceAddress &out) { // Разбор строки адреса DS18B20.
  int byteIndex = 0; // Текущий индекс байта адреса.
  int nibbleCount = 0; // Счетчик полубайт (HEX символов).
  uint8_t current = 0; // Текущий собираемый байт.
  for (size_t i = 0; i < input.length() && byteIndex < 8; i++) { // Идем по строке до заполнения адреса.
    char c = input[i]; // Берем очередной символ.
    int value = -1; // Значение HEX символа по умолчанию.
    if (c >= '0' && c <= '9') value = c - '0'; // Преобразуем цифру.
    else if (c >= 'a' && c <= 'f') value = 10 + (c - 'a'); // Преобразуем букву в нижнем регистре.
    else if (c >= 'A' && c <= 'F') value = 10 + (c - 'A'); // Преобразуем букву в верхнем регистре.
    if (value < 0) continue; // Пропускаем не HEX символы (например, дефисы).
    current = static_cast<uint8_t>((current << 4) | value); // Наращиваем байт адреса.
    nibbleCount++; // Увеличиваем счетчик полубайт.
    if (nibbleCount == 2) { // Если собрали байт полностью.
      out[byteIndex++] = current; // Сохраняем байт адреса.
      current = 0; // Сбрасываем текущий байт.
      nibbleCount = 0; // Сбрасываем счетчик полубайт.
    }
  }
  return byteIndex == 8; // Валидный адрес только если заполнены 8 байт.
}

inline void loadSavedDs18Address(const char *key, DeviceAddress &target, String &label) { // Загружаем адрес из NVS.
  String defaultAddress = formatDeviceAddress(target); // Формируем адрес по умолчанию.
  String stored = loadValue<String>(key, defaultAddress); // Получаем сохраненный адрес из памяти.
  DeviceAddress parsed; // Буфер для разобранного адреса.
  if (parseDeviceAddressString(stored, parsed)) { // Проверяем корректность сохраненной строки.
    memcpy(target, parsed, sizeof(DeviceAddress)); // Записываем адрес в переменную датчика.
    label = deviceAddressIsZero(parsed) ? String("не назначен") : formatDeviceAddress(parsed); // Обновляем строку для UI.
  } else {
    label = defaultAddress; // Если строка некорректна, показываем адрес по умолчанию.
  }
}

inline String scanDs18Sensors() { // Сканируем шину 1-Wire и собираем адреса DS18B20.
  String result = ""; // Строка результата для UI.
  sensors.requestTemperatures(); // Запрашиваем обновление данных для корректного ответа шины.
  int qty = sensors.getDeviceCount(); // Считываем количество устройств на шине.
  ds18FoundCount = 0; // Сбрасываем счетчик найденных адресов.

  result += "Найдено устройств: " + String(qty) + "\n"; // Пишем итоговое количество на шине.
   for (int i = 0; i < qty && ds18FoundCount < static_cast<int>(kMaxDs18Sensors); i++) { // Обрабатываем не более kMaxDs18Sensors датчиков.
    DeviceAddress addr; // Буфер адреса устройства.
    if (sensors.getAddress(addr, i)) { // Пытаемся получить адрес устройства.
      memcpy(ds18Found[ds18FoundCount], addr, sizeof(DeviceAddress)); // Сохраняем адрес в локальный кэш.
      result += "Адрес " + String(i) + ": " + formatDeviceAddress(addr) + "\n"; // Добавляем адрес в результат.
      ds18FoundCount++; // Увеличиваем число найденных датчиков.
    } else {
      result += "Ошибка получения адреса устройства " + String(i) + "\n"; // Сообщаем об ошибке чтения адреса.
    }
  }

if (qty > static_cast<int>(kMaxDs18Sensors)) { // Если датчиков больше лимита, предупреждаем.
    result += "Показаны только первые " + String(kMaxDs18Sensors) + " датчиков.\n"; // Сообщаем о лимите.
  }
  if (ds18FoundCount == 0) { // Если адреса не найдены.
    result += "Датчики на шине не обнаружены.\n"; // Пишем, что датчиков нет.
  }

  result += "Можно назначить одинаковые или разные адреса для помещения и улицы.\n"; // Подсказка по назначению.

  return result; // Возвращаем итоговую строку.
}

inline bool assignDs18SensorFromIndex(DeviceAddress &target, int index, String &label, const char *storageKey, String &info, DeviceAddress *other, String *otherLabel, const char *otherStorageKey, const char *targetName, const char *otherName) { // Назначаем датчик по индексу.
  if (ds18FoundCount == 0) { // Если нет кэша адресов, делаем скан.
    info = scanDs18Sensors(); // Обновляем информацию сканирования.
  }
  (void)other;
  (void)otherLabel;
  (void)otherStorageKey;
  (void)targetName;
  (void)otherName;
if (index == -1) { // Значение -1 означает снятие назначения адреса.
    DeviceAddress emptyAddress = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    memcpy(target, emptyAddress, sizeof(DeviceAddress));
    label = "не назначен";
    saveValue<String>(storageKey, formatDeviceAddress(emptyAddress));
    if (&target == &sensor0) {
      DS1Assigned = false;
      DS1Available = false;
    } else if (&target == &sensor1) {
      DS2Assigned = false;
      DS2Available = false;
    }
    info = "Привязка датчика снята.";
    return true;
  }
  if (index < -1) { // Служебные отрицательные значения игнорируем (без изменения привязки).
    info = "Ожидание выбора датчика.";
    return false;
  }

    if (index >= ds18FoundCount || index >= static_cast<int>(kMaxDs18Sensors)) { // Индекс вне диапазона найденных датчиков.
    info = "Индекс " + String(index) + " недоступен. Сначала выполните поиск датчиков.";
    return false;
  }

  memcpy(target, ds18Found[index], sizeof(DeviceAddress)); // Копируем выбранный адрес в целевую переменную.
    if (deviceAddressIsZero(target)) { // Защита от пустого адреса в кэше.
    info = "Выбран пустой адрес. Повторите поиск датчиков.";
    return false;
  }
  label = formatDeviceAddress(target); // Обновляем строковый адрес для UI.
  saveValue<String>(storageKey, label); // Сохраняем адрес в память.
  if (&target == &sensor0) {
    DS1Assigned = true;
  } else if (&target == &sensor1) {
    DS2Assigned = true;
  }
  info = "Выбран датчик " + String(index) + ": " + label; // Сообщаем об успешном выборе.
  return true; // Возвращаем успех.
}

// bool ds_setup = false; // Флаг настройки датчиков
// unsigned long ds_timer = 0; // Переменная для отслеживания времени
// float t = 0; // Переменная для хранения температуры

// float ds(int interval) {
//   if (!ds_setup || (millis() - ds_timer >= interval)) {
//     ds_timer = millis();
//     sensors.begin(); // Инициализация библиотеки для работы с датчиками
//     int qty = sensors.getDeviceCount(); // Получаем количество датчиков на шине
//     Serial.print("Поиск датчиков DALLAS: ");
//     Serial.print(qty);
//     Serial.println(" устройств");

//     for (int i = 0; i < qty; i++) {
//       DeviceAddress addr;
//       if (sensors.getAddress(addr, i)) {
//         Serial.print("Адрес устройства ");
//         for (int j = 0; j < 8; j++) {
//           if (addr[j] < 16) Serial.print("0");
//           Serial.print(addr[j], HEX); // Вывод адреса датчика в шестнадцатеричном формате
//           if (j < 7) Serial.print("-");
//         }
//         Serial.println();
//       }
//     }
//     ds_setup = true;
//   }

//   sensors.requestTemperatures(); // Запрос температур с датчиков
//   t = sensors.getTempCByIndex(0); // Получаем температуру с первого датчика (при необходимости измените индекс)

//   // Ваш дальнейший код для обработки температуры

//   return t; // Возвращаем значение температуры
// }


// // Функция формирования строки с количеством и адресами всех датчиков
// String getDallasSensorsInfo() {
//   String result = "";
//   int qty = sensors.getDeviceCount();

//   result += "Найдено устройств: " + String(qty) + "\n";

//   for (int i = 0; i < qty; i++) {
//     DeviceAddress addr;
//     if (sensors.getAddress(addr, i)) {
//       result += "Адрес " + String(i) + ": ";
//       for (int j = 0; j < 8; j++) {
//         if (addr[j] < 16) result += "0";
//         result += String(addr[j], HEX);
//         if (j < 7) result += "-";
//       }
//       result += "\n";
//     } else {
//       result += "Ошибка получения адреса устройства " + String(i) + "\n";
//     }
//   }

//   return result;
// }





/************************* Инициализируем работу с датчиков температуры DS18B20 *********************************/
void setup_ds18 (String &sensor0Label, String &sensor1Label){ // Инициализация DS18B20 и загрузка адресов.
  sensors.begin(); // Запускаем библиотеку DallasTemperature.

  loadSavedDs18Address(kDs18Sensor0StorageKey, sensor0, sensor0Label); // Загружаем адрес датчика sensor0.
  loadSavedDs18Address(kDs18Sensor1StorageKey, sensor1, sensor1Label); // Загружаем адрес датчика sensor1.

  DS1Assigned = !deviceAddressIsZero(sensor0);
  DS2Assigned = !deviceAddressIsZero(sensor1);
  if (DS1Assigned) {
    sensors.setResolution(sensor0, 12);// устанавливаем разрешение датчика от 9 до 12 бит
  }
  if (DS2Assigned) {
    sensors.setResolution(sensor1, 12);// устанавливаем разрешение датчика от 9 до 12 бит
  }

  sensors.setWaitForConversion(true); // Блокирующий режим: всегда читаем готовое измерение.


}






// void Temp_DS18B20(int interval_Temp_DS18B20){ //Считываем датчики по заданному таймеру
//   static unsigned long timer;  
//   if (interval_Temp_DS18B20 + timer > millis()) return;
//   timer = millis();


  

// if (sensors.isConversionComplete()) { // Эта строка кода проверяет, завершено ли преобразование данных в датчике. 
//  float DS_1 = sensors.getTempC(sensor1);
// if (DS_1 != -127) {DS1=DS_1;
// } //не работает только округляет... - DS1 = static_cast<int>(DS_1 * 10.0) / 10.0;  // Удаляем символ сотой части
   
   

// sensors.requestTemperaturesByAddress(sensor1);  // Запрашиваем следующее измерение температуры для указанного датчика



//   }



void Temp_DS18B20(int interval_Temp_DS18B20) {
  static unsigned long timer;  
  if (millis() - timer < interval_Temp_DS18B20) return;
  timer = millis();
 sensors.requestTemperatures(); // В блокирующем режиме дожидаемся готового измерения.

  // Датчик помещения (sensor0)
  DS1Assigned = !deviceAddressIsZero(sensor0);
  DS1Available = false;
  if (DS1Assigned) {
    float temp0 = sensors.getTempC(sensor0);
    if (temp0 != DEVICE_DISCONNECTED_C && temp0 > -100 && temp0 < 150) {
      DS1 = roundf(temp0 * 10) / 10.0;  // округление до десятых
      DS1Available = true;
    } else {
      DS1 = 0.0f;
    }
  } else {
    DS1 = 0.0f;
  }

  // Датчик улицы (sensor1)
  DS2Assigned = !deviceAddressIsZero(sensor1);
  DS2Available = false;
  if (DS2Assigned) {
    float temp1 = sensors.getTempC(sensor1);
    if (temp1 != DEVICE_DISCONNECTED_C && temp1 > -100 && temp1 < 150) {
      DS2 = roundf(temp1 * 10) / 10.0;  // округление до десятых
      DS2Available = true;
    } else {
      DS2 = 0.0f;
    }
  } else {
    DS2 = 0.0f;
  }
  
}







//   //interval_DS18B20 = (jee.param("interval_DS18B20").toInt())*1000;//Записываем полученныей интервал в переменную для возможности задавать из интерфейса

//  // отправляем запрос на измерение температуры - если датчик один на шине
//   ds_sensor.requestTemperatures();
//   // считываем данные из регистра датчика
//   float temperature = ds_sensor.getTempCByIndex(0);
  
//   ds_sensor.requestTemperatures();


//   if(temperature != -127 && temperature != 85)  Temp_DS_18B20 = String(temperature); //jee.var("Temp_DS18B20", String(temperature));
//   //if(temperature != -127) {jee.var("temperature", String(temperature));}
//   //Включаем циркуляцию воды чтоб не замерзла вода 
//   //if(temperature < 0.35 && temperature > -5) {Power_Nasos=true;} else if (temperature > 0.7 && temperature < 1 || temperature < -5){Power_Nasos=false;}



//   // switch(Step_addr){ //измеряем по очереди температуру на разных датчиках

//   // case 1: {
//   //   ds_sensor.requestTemperatures(); 
//   //   DS11=ds_sensor.getTempC(sensor1);
//   //   if(DS11 != -127) {jee.var("DS1", String(DS11));}  //jee.var("DS3", String(random(1000)));
    
//   //   Step_addr=2; break;} 


//   // case 2: {
//   //   ds_sensor.requestTemperatures(); 
//   //   DS21=ds_sensor.getTempC(sensor2);
//   //   if(DS21 != -127) {jee.var("DS2", String(DS21));} //jee.var("DS4", String(random(1000))); 
//   //   //Serial.print("D18B20 - 4  =  "); Serial.print(DS4); Serial.println("  C");
  
//   //   Step_addr=1; break;} 
    
          
//   //   }//END switch

// }






























// //#include <OneWire.h>
// // #include <DallasTemperature.h>

// // #define DS_PIN 4

// // OneWire oneWire_ds(DS_PIN);
// // DallasTemperature ds_sensor(&oneWire_ds);

// // bool ds_setup;

// // float ds(int interval)
// // {
    

// //     static bool ds_setup;
// //     static unsigned long ds_timer;
// //     static unsigned int ds_interval = interval;
// //     static float t;

// //     if(!ds_setup)
// //     {
// //         ds_sensor.begin();
// //         ds_setup = true;
// //     }

// //     if (ds_timer + ds_interval > millis())
// //         return t;
// //     ds_timer = millis();
// //     ds_sensor.requestTemperatures();
// //     t = ds_sensor.getTempCByIndex(0);
// // }

// /************************* Настраиваем работу с датчиком температуры DS18B20 *********************************/

// #include <OneWire.h>
// #include <DallasTemperature.h>

// #define DS_PIN 26 //DS_PIN 27//пин подключения датчиков температуры DS18B20

// OneWire  oneWire_ds(DS_PIN);
// DallasTemperature ds_sensor(&oneWire_ds);

// byte qty; //количество градусников на шине

// DeviceAddress sensor0 = {0x28, 0xFF, 0x64, 0x1E, 0x83, 0x7A, 0x5, 0x83};
// DeviceAddress sensor1 = {0x28, 0xFF, 0x64, 0x1E, 0x83, 0x61, 0xBE, 0x5E};



// float /*DS1= 20,DS2= 20,DS3= 20,DS4= 20,*/DS11,DS21;

// String Temp_DS_18B20; // Для вывода на Web страницу

// /*OneWire  ds(pin_DS18B20);        // Линия 1-Wire для опроса датчика температуры DS18B20

// float celsius;

// float term_addr1;       //Содержит последнее измеренное значение температуры в X
// float term_addr2;       //Содержит последнее измеренное значение температуры в XX
// float term_addr3;       //Содержит последнее измеренное значение температуры в XXX
// float term_addr4;       //Содержит последнее измеренное значение температуры в XXXX
// float term_addr5;       //Содержит последнее измеренное значение температуры в XXXXX
// float term_addr6;       //Содержит последнее измеренное значение температуры в XXXXXX
// float term_addr7;       //Содержит последнее измеренное значение температуры в XXXXX
// float term_addr8;       //Содержит последнее измеренное значение температуры в XXXXXX




// unsigned long Startup_time_s;     //Время с момента включения контроллера в секундах
// unsigned char StepTEMP=1;         //Номер шага в функции измерения температуры
// */

// unsigned char Step_addr=1;        //измеряем по очереди температуру на разных датчиках



// /************************* Инициализируем работу с датчиков температуры DS18B20 *********************************/
// void setup_ds18 (void){
// sensors.begin();
// // ds_sensor.setResolution(12);  // устанавливаем разрешение датчика от 9 до 12 бит
// // qty = ds_sensor.getDeviceCount(); //Записываем колличество градусников на шине
// // Serial.print("Search DALLAS: ");
// // Serial.print(qty); Serial.println("  devices");
// // Serial.print(ds_sensor.getTempCByIndex(1));  Serial.println("index");

  
//     // показываем сколько датчиков нашли на шине
//   Serial.print("Found ");
//   Serial.print(sensors.getDeviceCount(), DEC);
//   Serial.println(" devices.");

//     // достаем адрес датчика с индесом 0 
//   if (!sensors.getAddress(sensor0, 0)){ 
//     Serial.println("Unable to find address for Device 0"); 
//   } 

//     // отпаравляем адрес из массива в монитор порта
//   Serial.print("address sensor 0: ");   
//   for (uint8_t i = 0; i < 8; i++)  {  
//     Serial.print("0x");   
//     Serial.print(sensor0[i], HEX);
//     Serial.print(", ");
//   }
//   Serial.println();

//       // достаем адрес датчика с индесом 1 
//   if (!sensors.getAddress(sensor1, 1)){ 
//     Serial.println("Unable to find address for Device 0"); 
//   } 

//     // отпаравляем адрес из массива в монитор порта
//   Serial.print("address sensor 1: ");   
//   for (uint8_t i = 0; i < 8; i++)  {  
//     Serial.print("0x");   
//     Serial.print(sensor1[i], HEX);
//     Serial.print(", ");
//   }
//   Serial.println();
 
//     // устанавливаем разрешение датчика 11 бит (может быть 9, 10, 11, 12) 
//     // на точность измерения температуры показатель не влияет.
//   sensors.setResolution(sensor0, 11); 
//   sensors.setResolution(sensor1, 11); 
 

// }






// void Temp_DS18B20(int interval_Temp_DS18B20){ //Считываем датчики по заданному таймеру
//   static unsigned long timer;  
//   if (interval_Temp_DS18B20 + timer > millis()) return;
//   timer = millis();
//   //interval_DS18B20 = (jee.param("interval_DS18B20").toInt())*1000;//Записываем полученныей интервал в переменную для возможности задавать из интерфейса


// //  // отправляем запрос на измерение температуры - если датчик один на шине
// //   ds_sensor.requestTemperatures();
// //   // считываем данные из регистра датчика
// //   float temperature = ds_sensor.getTempCByIndex(0);
// //   ds_sensor.requestTemperatures();

// //char myStr8[10]; // Для преобразования Float в String
// //memset(myStr8, '\0', 10); // 10 байт не в HEX представлении, очистить массив перед записью данных 


//   switch(Step_addr){ //измеряем по очереди температуру на разных датчиках

//   case 1: {
//     ds_sensor.requestTemperatures(); 
//     DS11=ds_sensor.getTempC(sensor0);
//     if(DS11 != -127) {jee.var("DS1", String(DS11));}  //jee.var("DS3", String(random(1000)));
    
   
//     //dtostrf(DS11, 2, 2, myStr8);  // выводим в строку myStr8 2 разряда до, 2 разряда после запятой
//     String MyStr = String(DS11*100/100, 2);

//     myNex.writeStr("page0.t0.txt", MyStr + " C"); //выводим на Nextion
//     myNex.writeStr("heat.t1.txt", MyStr + " C"); //выводим на Nextion
//     Temp_DS_18B20 = MyStr; // Для вывода на Web страницу
    
//     Step_addr=2; break;} 


//   case 2: {
//     ds_sensor.requestTemperatures(); 
//     DS21=ds_sensor.getTempC(sensor1);
//     if(DS21 != -127) {jee.var("DS2", String(DS21));} //jee.var("DS4", String(random(1000))); 

//     String MyStr1 = String(DS21*100/100, 2);
//     myNex.writeStr("heat.t2.txt", MyStr1 + " C"); //выводим на Nextion

//     Step_addr=1; break;} 
    

    

    
          
//     }//END switch

    

// }
