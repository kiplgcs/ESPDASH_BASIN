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



void onDs18Sensor0Select(const int &value) { // Обрабатываем назначение датчика на температуру бассейна.
  if (assignDs18SensorFromIndex(sensor0, value, Ds18Sensor0Address, kDs18Sensor0StorageKey, Ds18ScanInfo)) { // Пишем выбранный адрес в sensor0 и обновляем статус.
    saveValue<int>(kDs18Sensor0IndexStorageKey, value); // Сохраняем выбранный индекс в NVS.
    if (DS1Assigned) { // Применяем настройки только если адрес действительно назначен.
      sensors.setResolution(sensor0, 12); // Фиксируем точность измерения 12 бит.
      sensors.requestTemperaturesByAddress(sensor0); // Сразу запрашиваем первое измерение для нового адреса.
    } // Завершаем блок применения настроек для sensor0.
  } // Завершаем блок успешного назначения sensor0.
}

void onDs18Sensor1Select(const int &value) { // Обрабатываем назначение датчика на температуру после нагревателя.
  if (assignDs18SensorFromIndex(sensor1, value, Ds18Sensor1Address, kDs18Sensor1StorageKey, Ds18ScanInfo)) { // Пишем выбранный адрес в sensor1 и обновляем статус.
    saveValue<int>(kDs18Sensor1IndexStorageKey, value); // Сохраняем выбранный индекс в NVS.
    if (DS2Assigned) { // Применяем настройки только если адрес действительно назначен.
      sensors.setResolution(sensor1, 12); // Фиксируем точность измерения 12 бит.
      sensors.requestTemperaturesByAddress(sensor1); // Сразу запрашиваем первое измерение для нового адреса.
    } // Завершаем блок применения настроек для sensor1.
  } // Завершаем блок успешного назначения sensor1.
}




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


constexpr BaseType_t kWebMqttCore = 0; // Назначаем ядро 0 для общего сетевого контура WiFi+WEB+MQTT.
constexpr BaseType_t kMainLogicCore = 1; // Назначаем ядро 1 для всей основной прикладной логики.
TaskHandle_t webMqttTaskHandle = nullptr; // Сохраняем дескриптор задачи Web+MQTT для диагностики и контроля.

void webMqttTask(void * /*parameter*/) { // Определяем задачу FreeRTOS, которая будет крутить только WiFi+WEB+MQTT.
  for (;;) { // Запускаем бесконечный цикл, потому что задача должна работать весь аптайм контроллера.
    wifiModuleLoop(); // Выполняем обслуживание WiFi и WEB-интерфейса в сетевом контуре ядра 0.
    handleMqttLoop(); // Выполняем обслуживание MQTT в том же сетевом контуре ядра 0.
    vTaskDelay(pdMS_TO_TICKS(2)); // Делаем короткую паузу, чтобы отдать CPU другим задачам FreeRTOS.
  } // Закрываем цикл непрерывного обслуживания сетевого контура.
} // Закрываем функцию задачи, закрепляемой за ядром 0.


// ---------- NTP (синхронизация времени) ----------
WiFiUDP ntpUDP;


/* ---------- Setup ---------- */
void setup() {
  Serial.begin(115200);

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

  gmtOffset_correct = loadValue<int>("gmtOffset", 3);
  gmtOffset_correct = normalizeGmtOffset(gmtOffset_correct);
  Saved_gmtOffset_correct = gmtOffset_correct;

 // Загрузка параметров из EEPROM после перезагрузки
  auto sanitizeDosingPeriod = [](int value) -> int {
    if(value < 1) return 1;
    if(value > 13) return 13;
    return value;
  };
  ACO_Work = sanitizeDosingPeriod(loadValue<int>("ACO_Work", ACO_Work));
  H2O2_Work = sanitizeDosingPeriod(loadValue<int>("H2O2_Work", H2O2_Work));

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

  readNextionTime(); // При старте сразу пробуем взять время с RTC Nextion

  setup_ds18(Ds18Sensor0Address, Ds18Sensor1Address); // Загружаем привязанные адреса DS18B20 из NVS при старте.

  Ds18Sensor0Index = loadValue<int>(kDs18Sensor0IndexStorageKey, Ds18Sensor0Index); // Поднимаем из NVS последний индекс для sensor0.
  Ds18Sensor1Index = loadValue<int>(kDs18Sensor1IndexStorageKey, Ds18Sensor1Index); // Поднимаем из NVS последний индекс для sensor1.

  interface(); // Первичная сборка UI нужна для загрузки/сохранения связанных значений из EEPROM
  dashInterfaceInitialized = true; // Критический фикс: запрещаем повторный вызов interface() в dash.begin(), иначе вкладки/элементы дублируются
  
  syncCleanDaysFromSelection();

  ColorRGB = LedColorMode.equalsIgnoreCase("manual");

  if(LedAutoplayDuration < 1) LedAutoplayDuration = 1;

  new_bright = LedBrightness;

  setup_WS2815();


  // ---------- Настройка графиков ----------
  loadGraph();

  dash.begin(); // Запуск дашборда


  setup_Modbus();

  xTaskCreatePinnedToCore(webMqttTask, "WebMqttCoreTask", 8192, nullptr, 2, &webMqttTaskHandle, kWebMqttCore); // Создаем и прикрепляем задачу WiFi+WEB+MQTT именно к ядру 0.
  Serial.printf("[BOOT] CORE MAP -> WiFi+WEB+MQTT: %d | Main logic(loop): %d\n", kWebMqttCore, kMainLogicCore); // Печатаем явную карту ядер, чтобы одним взглядом видеть распределение.



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

  static bool mainCoreLogged = false; // Запоминаем, что диагностический лог по ядру loop уже был напечатан.
  if (!mainCoreLogged) { // Проверяем, что стартовый лог по ядру основной логики еще не выводился.
    Serial.printf("[BOOT] loop() main logic confirmed on core %d\n", xPortGetCoreID()); // Печатаем фактическое ядро, где реально выполняется основной loop.
    mainCoreLogged = true; // Фиксируем флаг, чтобы не засорять UART повторяющимися логами.
  } // Закрываем одноразовый блок диагностики ядра основной логики.


  static int lastDs18ScanButton = 0; // Храним прошлое состояние кнопки, чтобы ловить только фронт нажатия.

  
  if (Ds18ScanButton != lastDs18ScanButton) { // Проверяем изменение состояния кнопки поиска.
    lastDs18ScanButton = Ds18ScanButton; // Запоминаем новое состояние кнопки.
    if (Ds18ScanButton == 1) { // Выполняем поиск строго по нажатию.
      Ds18ScanInfo = scanDs18Sensors(); // Сканируем шину и выводим найденные адреса.
      Ds18ScanButton = 0; // Сбрасываем кнопку после выполнения поиска.
      saveButtonState("ds18ScanButton", 0); // Сохраняем сброшенное состояние, чтобы UI не залипал.
    } // Завершаем обработку фронта нажатия кнопки поиска.
  } // Завершаем проверку события по кнопке поиска.
  

  
  acoServiceLoop(); // Обработка ручного импульса ACO по состоянию
  h2o2ServiceLoop(); // Обработка ручного импульса H2O2 по состоянию
  // Обновление времени через NTP/Nextion/память
  NPT_Time(period_get_NPT_Time);
  CurrentTime = getCurrentDateTime();   // Получение текущего времени

TimerControlRelay(10000);  // TimerControlRelay(600); //Контроль включения реле по таймерам
updateCleanSequence(); // Обновление последовательности промывки
updateManualPumpPulses(); // Для прверки перельстатических насосов - счет таймера - 1 сек
ControlModbusRelay(1000); // Отправка команд на Modbus-реле
loop_PH(2000); // Обработка логики PH
loop_CL2(2100); // Обработка логики хлора

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
  // OverlayFilterState = String("🧽 Фильтр: ") + (Power_Clean ? "Промывка" : (Power_Filtr ? "Фильтрация" : "Остановлен"));
  String filterStateDetails;
  if (Power_Clean || CleanSequenceActive) {
    String cleanStage = CommentClean;
    if (cleanStage.length() == 0) {
      cleanStage = "Промывка активна";
    }
    filterStateDetails = "Промывка / " + cleanStage;
  } else if (Power_Filtr) {
    filterStateDetails = FiltrationTimerActive ? "Фильтрация (по таймеру)" : "Фильтрация (ручной режим)";
  } else {
    filterStateDetails = "Остановлен";
  }
  OverlayFilterState = String("🧽 Фильтр: ") + filterStateDetails;




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


  loop_WS2815();  




  slow(period_slow_Time); //Периодически отправляем данные для обратной связи - "period_slow_Time" Период обновления данных - зависит от "Nx_dim_id" nекущеuj - считанного значения яркости Nextion

}
