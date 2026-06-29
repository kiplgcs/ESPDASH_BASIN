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

#include "ds18.h"
#include "Timer_Relay.h"

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

inline void captureDosingPumpGraphEvents(){ // Считаем фронты включения насосов для меток на графиках PH/CL.
  static bool acoWasActive = false; // Помним прошлое состояние ACO, чтобы один импульс считался один раз.
  static bool h2o2WasActive = false; // Помним прошлое состояние NaOCl, чтобы один импульс считался один раз.
  const bool acoActive = Power_ACO || ManualPulse_ACO_Active; // ACO активен от автоматики или ручной проверки.
  const bool h2o2Active = Power_H2O2 || ManualPulse_H2O2_Active; // NaOCl активен от автоматики или ручной проверки.

  if(acoActive && !acoWasActive && PendingAcoDosingGraphEvents < 65535) PendingAcoDosingGraphEvents++; // Метка ACO попадет в ближайшую точку графика pH.
  if(h2o2Active && !h2o2WasActive && PendingH2O2DosingGraphEvents < 65535) PendingH2O2DosingGraphEvents++; // Метка NaOCl попадет в ближайшую точку графика CL.

  acoWasActive = acoActive; // Обновляем память фронта ACO.
  h2o2WasActive = h2o2Active; // Обновляем память фронта NaOCl.
}

inline uint16_t dosingEventsForGraphSeries(const String &series){ // Возвращаем накопленные события для конкретного графика.
  if(series == "FloatPH") return PendingAcoDosingGraphEvents; // Насос кислоты ACO относится к графику pH.
  if(series == "FloatСl") return PendingH2O2DosingGraphEvents; // Насос NaOCl относится к графику хлора; в id используется кириллическая С.
  return 0; // Для остальных графиков служебных событий нет.
}

inline void clearDosingEventsAfterGraphSample(const String &series, bool pointAdded){ // Сбрасываем счетчик только после записи точки.
  if(!pointAdded) return; // Если интервал графика еще не прошел, события должны дождаться следующей точки.
  if(series == "FloatPH") PendingAcoDosingGraphEvents = 0; // События ACO уже записаны в точку pH.
  if(series == "FloatСl") PendingH2O2DosingGraphEvents = 0; // События NaOCl уже записаны в точку хлора.
}

inline void syncChlorineOrpGraphRuntimeSettings(){ // Синхронизирует скрытую ORP-серию с видимым графиком ppmCl без расширения формата точки.
  auto chlorineSettings = seriesConfig.find(chlorinePpmGraphSeriesId()); // Берем текущие настройки видимого хлорного графика.
  if(chlorineSettings == seriesConfig.end()) return; // Если UI еще не зарегистрировал график, синхронизацию пропускаем.
  seriesConfig[chlorineOrpGraphSeriesId()] = chlorineSettings->second; // Скрытая серия ORP пишет точки с тем же интервалом и лимитом.
}


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

  pinMode(3, INPUT_PULLDOWN); // GPIO3 используется как аналоговый вход датчика освещенности; внутренняя подтяжка вниз страхует обрыв внешнего GND.
  analogReadResolution(12); // Для датчика освещенности используем шкалу ADC 0..4095.
#ifdef ARDUINO_ARCH_ESP32
  analogSetPinAttenuation(3, ADC_11db); // Диапазон ADC до 3.3 В, чтобы датчик не упирался в 100% слишком рано.
#endif

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
  ACO_Work = sanitizeDosingPeriodValue(loadValue<int>("ACO_Work", ACO_Work)); // Загружаем и сразу нормализуем период ACO.
  H2O2_Work = sanitizeDosingPeriodValue(loadValue<int>("H2O2_Work", H2O2_Work)); // Загружаем и сразу нормализуем период NaOCl.
  DrainWorkMinutes = clampIntSetting(loadValue<int>("DrainWorkMinutes", DrainWorkMinutes), 1, 30, 15); // Загружаем длительность порции слива.
  DrainPauseMinutes = clampIntSetting(loadValue<int>("DrainPauseMinutes", DrainPauseMinutes), 1, 30, 15); // Загружаем паузу между порциями слива.
  DrainCyclesTarget = clampIntSetting(loadValue<int>("DrainCyclesTarget", DrainCyclesTarget), 1, 30, 10); // Загружаем количество порций слива.
  ToppingWorkMinutes = clampIntSetting(loadValue<int>("ToppingWorkMinutes", ToppingWorkMinutes), 5, 60, 10); // Загружаем длительность долива.
  ToppingPauseMinutes = clampIntSetting(loadValue<int>("ToppingPauseMinutes", ToppingPauseMinutes), 5, 60, 60); // Загружаем паузу между доливами.

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

  applyGmtOffsetFromNextion(false); // При старте подхватываем часовой пояс из Nextion без сдвига еще не считанного RTC.
  readNextionTime(); // При старте сразу пробуем взять время с RTC Nextion

  setupDs18Bindings(); // Загружаем и применяем связанные настройки DS18B20.

  interface(); // Первичная сборка UI нужна для загрузки/сохранения связанных значений из EEPROM
  dashInterfaceInitialized = true; // Критический фикс: запрещаем повторный вызов interface() в dash.begin(), иначе вкладки/элементы дублируются
  normalizeChemicalLimits(); // Исправляем сохраненные пределы pH/CL после загрузки из NVS.
  persistChemicalLimits(); // Сохраняем исправленные пределы, чтобы после следующей перезагрузки не вернулся мусор.
  if (Power_Drain) Activation_Water_Level = false; // Если из памяти поднялся слив, автоматический контроль уровня запрещаем.
  if (Power_Drain) Power_Topping = false; // Если из памяти поднялся слив, ручной долив запрещаем.
  persistWaterControlModes(); // Сохраняем итоговые безопасные режимы уровня воды после загрузки.
  
  syncCleanDaysFromSelection();

  ColorRGB = LedColorMode.equalsIgnoreCase("manual");

  if(LedAutoplayDuration < 1) LedAutoplayDuration = 1;

  new_bright = LedBrightness;

  setup_WS2815();


  // ---------- Настройка графиков ----------
  loadGraph();

  dash.begin(); // Запуск дашборда
  registerGraphSource(chlorineOrpGraphSeriesId(), [](){ return static_cast<float>(corrected_ORP_Eh_mV); }, "", defaultGraphUpdateInterval, maxGraphPoints); // Скрыто пишем ORP для таблицы хлорного графика стандартным форматом time,value,events.


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


inline void acoServiceLoop(){ // Сервисная обработка кнопки ручного импульса ACO без вмешательства в автоматику
  static bool initialized = false; // Флаг первого прохода, чтобы очистить сохраненное состояние кнопки.
  if(!initialized){
    ManualPulse_ACO_Request = false; // Не запускаем насос из-за старого сохраненного состояния после перезагрузки.
    saveButtonState("Power_ACO_Button", 0); // В памяти кнопка ручной проверки всегда хранится отпущенной.
    initialized = true; // Дальше первый проход больше не повторяется.
  }
  if(ManualPulse_ACO_Request){ // Web/Nextion запросил ручную проверку насоса.
    ManualPulse_ACO_Active = true; // Запускаем отдельный ручной импульс, не трогая автоматический Power_ACO.
    ManualPulse_ACO_StartedAt = millis(); // Запоминаем старт для ограничения импульса пятью секундами.
    ManualPulse_ACO_Request = false; // Запрос одноразовый, после приема сразу сбрасываем.
    saveButtonState("Power_ACO_Button", 0); // Не сохраняем кнопку включенной между циклами и перезагрузками.
  }
} // Конец acoServiceLoop

inline void h2o2ServiceLoop(){ // Сервисная обработка кнопки ручного импульса H2O2 без вмешательства в автоматику
  static bool initialized = false; // Флаг первого прохода, чтобы очистить сохраненное состояние кнопки.
  if(!initialized){
    ManualPulse_H2O2_Request = false; // Не запускаем насос из-за старого сохраненного состояния после перезагрузки.
    saveButtonState("Power_H2O2_Button", 0); // В памяти кнопка ручной проверки всегда хранится отпущенной.
    initialized = true; // Дальше первый проход больше не повторяется.
  }
  if(ManualPulse_H2O2_Request){ // Web/Nextion запросил ручную проверку насоса.
    ManualPulse_H2O2_Active = true; // Запускаем отдельный ручной импульс, не трогая автоматический Power_H2O2.
    ManualPulse_H2O2_StartedAt = millis(); // Запоминаем старт для ограничения импульса пятью секундами.
    ManualPulse_H2O2_Request = false; // Запрос одноразовый, после приема сразу сбрасываем.
    saveButtonState("Power_H2O2_Button", 0); // Не сохраняем кнопку включенной между циклами и перезагрузками.
  }
} // Конец h2o2ServiceLoop


/* ---------- Loop ---------- */
void loop() {

  static bool mainCoreLogged = false; // Запоминаем, что диагностический лог по ядру loop уже был напечатан.
  if (!mainCoreLogged) { // Проверяем, что стартовый лог по ядру основной логики еще не выводился.
    Serial.printf("[BOOT] loop() main logic confirmed on core %d\n", xPortGetCoreID()); // Печатаем фактическое ядро, где реально выполняется основной loop.
    mainCoreLogged = true; // Фиксируем флаг, чтобы не засорять UART повторяющимися логами.
  } // Закрываем одноразовый блок диагностики ядра основной логики.

  loop_WS2815(); // Обслуживаем RGB в начале цикла, чтобы лента не ждала тяжелые блоки UI/Nextion/SPIFFS.

  handleDs18ScanButton(); // Обрабатываем кнопку поиска датчиков DS18B20.

    acoServiceLoop(); // Обработка ручного импульса ACO по состоянию
  h2o2ServiceLoop(); // Обработка ручного импульса H2O2 по состоянию
  serviceNextionSerial(32); // Сначала разгружаем UART Nextion, чтобы пользовательские события не ждали NTP/датчики.
  pollNextionDispensersControlsAsync(); // Приоритетно принимаем изменения с экрана дозаторов.
  pollNextionGmtOffsetAsync(); // Приоритетно принимаем подтвержденный GMT с Nextion.
  // Обновление времени через NTP/Nextion/память
  if (!nextionAsyncReadActive() && MySerial.available() == 0) { // RTC/NTP не должны вклиниваться в активный обмен Nextion.
    NPT_Time(period_get_NPT_Time);
  }
  CurrentTime = getCurrentDateTime();   // Получение текущего времени

TimerControlRelay(1000);  // Контроль дозаторов и таймеров раз в 1 секунду, чтобы импульс насоса не растягивался.
updateCleanSequence(); // Обновление последовательности промывки
updateManualPumpPulses(); // Для прверки перельстатических насосов - счет таймера - 1 сек
enforceDosingHardLimit(); // Дополнительный предохранитель: насосы PH/CL не могут держаться включенными дольше 5 секунд.
ControlModbusRelay(40); // RS485 проверяем в 2 раза реже, чтобы не забивать цикл и UART Nextion.
loop_PH(2000); // Обработка логики PH
loop_CL2(2100); // Обработка логики хлора

/**************************** *********************************************************************/
  serviceNextionSerial(32);
  if (!nextionAsyncReadActive()) {
    Nextion_Transmit(500); // Отправка в Nextion по очереди
  }
  pollNextionDispensersControlsAsync();
  pollNextionDispensersSettingsAsync();
  pollNextionFiltrSwitchesAsync();
  pollNextionGmtOffsetAsync();
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
  serviceNextionSerial(32);
  pollNextionDispensersControlsAsync();
  pollNextionDispensersSettingsAsync();
  pollNextionFiltrSwitchesAsync();
  pollNextionGmtOffsetAsync();
 /**************************** *********************************************************************/
  // Nextion_Transmit(500); // Отправка в Nextion по очереди
  // if(Power_Clean){Power_Filtr = false;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра






  static unsigned long lastUiModelUpdate = 0; // Тяжелые строки для web/UI обновляем порциями, а не на каждом обороте loop.
  if (millis() - lastUiModelUpdate >= 250) {
    lastUiModelUpdate = millis();

  // Генерация случайных значений для демонстрации
  RandomVal = random(0,50);                     // Случайное число
  Speed = random(150, 350) / 10.0f;            // Случайная скорость
  // Temperatura = random(220, 320) / 10.0f;      // Случайная температура
  Temperatura = DS1;      // Температура в бассейне
  
  // Формирование информационных строк
  String dinStatus = "🧩 Плата Modbus RTU RS485 Relay 16CH + DI16\n";

  dinStatus += "\n🔌 РЕЛЕ 1-8  : ";
  for (int i = 0; i < 8; ++i) {
    dinStatus += String(i + 1) + (ReadRelayArray[i] ? "🟢" : "⚫");
    if (i < 7) {
      dinStatus += " ";
    }
  }
  dinStatus += "\n🔌 РЕЛЕ 9-16 : ";
  for (int i = 8; i < 16; ++i) {
    dinStatus += String(i + 1) + (ReadRelayArray[i] ? "🟢" : "⚫");
    if (i < 15) {
      dinStatus += " ";
    }
  }

  dinStatus += "\n\n📥 ВХОДЫ 1-8 : ";
  for (int i = 0; i < 8; ++i) {
    dinStatus += String(i + 1) + (ReadInputArray[i] ? "🔵" : "⚪");
    if (i < 7) {
      dinStatus += " ";
    }
  }

  dinStatus += "\n📥 ВХОДЫ 9-16: ";
  for (int i = 8; i < 16; ++i) {
    dinStatus += String(i + 1) + (ReadInputArray[i] ? "🔵" : "⚪");
    if (i < 15) {
      dinStatus += " ";
    }
  }

  dinStatus += "\n\n🟢/🔵 = активно   ⚫/⚪ = неактивно";


  InfoString = "Random value is " + String(RandomVal) + " at " + CurrentTime + "Pow_WS2815 = " + String(Pow_WS2815);
  InfoStringRS485Model = "Waveshare Modbus RTU RS485 Relay 16CH + DI16";
  InfoStringDIN = dinStatus;
  InfoString1 = /*"Speed " + String(Speed, 1) + " / Temp " + String(Temperatura, 1)*/ + " button1 = " + String(button1)
              + " RangeSlider = " + String(RangeMin) + " / " + String(RangeMax);
  
   
  OverlayPoolTemp = "🌡 Бассейн: " + formatTemperatureString(DS1, DS1Available);
  OverlayHeaterTemp = "♨️ После нагревателя: " + formatTemperatureString(DS2, DS2Available);
  OverlayLevelUpper = String("🛟 Верхний: ") + (PoolUpperLevelReachedConfirmed ? "достиг датчика" : "ниже датчика");
  OverlayLevelLower = String("🛟 Нижний: ") + (PoolLowerLevelLowConfirmed ? "ниже датчика" : "выше датчика");
  OverlayPh = "🧪 pH: " + String(PH, 2);
  OverlayChlorine = "🧴 Cl: " + String(ppmCl, 3) + " ppm";
  OverlayDrainPit = String("🧯 Яма: ") + (DrainPitFullConfirmed ? "заполнена" : "свободна");
  OverlayLight = "🔆 Свет: " + String(Lumen_Ul) + "%";
  PoolWaterLevelStageInfo = buildPoolWaterLevelStageInfo(); // Обновляем строку этапа уровня для Web и MQTT-совместимых UI-значений.
  DrainPitStageInfo = buildDrainPitStageInfo(); // Обновляем строку этапа слива для Web и MQTT-совместимых UI-значений.
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
  loop_WS2815(); // Перед возможными SPIFFS-записями даем ленте шанс выдать очередной кадр.
  captureDosingPumpGraphEvents(); // Перед записью точки фиксируем включения дозаторов для меток на PH/CL.
  addGraphPoint(CurrentTime, RandomVal); // Обновление графика RandomVal
  syncChlorineOrpGraphRuntimeSettings(); // Перед записью точек держим ORP-историю синхронной с графиком ppmCl.
  for(auto &entry : graphValueProviders){
    float graphValue = entry.second(); // Читаем значение один раз, чтобы событие и точка относились к одному измерению.
    uint16_t graphEvents = dosingEventsForGraphSeries(entry.first); // Для PH/CL добавляем число включений дозатора за интервал.
    bool pointAdded = addSeriesPoint(entry.first, CurrentTime, graphValue, graphEvents); // Обновление всех графиков.
    clearDosingEventsAfterGraphSample(entry.first, pointAdded); // После записи точки сбрасываем только записанные события дозирования.
  }

  }


  loop_WS2815();  




  slow(period_slow_Time); //Периодически отправляем данные для обратной связи - "period_slow_Time" Период обновления данных - зависит от "Nx_dim_id" nекущеuj - считанного значения яркости Nextion

}
