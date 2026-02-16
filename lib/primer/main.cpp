// main.cpp — UI ESPDASH-PRO проект для ESP32 в Visual Studio Code
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

#include "WebUpdate.h"    // OTA-обновление через AsyncOTA


#include "ds18.h"
#include "P4_ESP32.h"


void onDs18Sensor0Select(const int &value) { // Обработчик выбора индекса для sensor0.
    if (assignDs18SensorFromIndex(sensor0, value, Ds18Sensor0Address, kDs18Sensor0StorageKey, InfoString, &sensor1, &Ds18Sensor1Address, kDs18Sensor1StorageKey, "помещения", "улицы")) { // Назначаем адрес датчика.
    saveValue<int>(kDs18Sensor0IndexStorageKey, value); // Сохраняем выбранный индекс в короткий ключ NVS.
    if (DS1Assigned) {
      sensors.setResolution(sensor0, 12); // Выставляем разрешение sensor0.
      sensors.requestTemperaturesByAddress(sensor0); // Запрашиваем измерение для sensor0.
    }
    if (DS2Assigned) {
      sensors.setResolution(sensor1, 12); // Выставляем разрешение sensor1.
      sensors.requestTemperaturesByAddress(sensor1); // Запрашиваем измерение для sensor1.
    }
  }
}

void onDs18Sensor1Select(const int &value) { // Обработчик выбора индекса для sensor1.
 if (assignDs18SensorFromIndex(sensor1, value, Ds18Sensor1Address, kDs18Sensor1StorageKey, InfoString, &sensor0, &Ds18Sensor0Address, kDs18Sensor0StorageKey, "улицы", "помещения")) { // Назначаем адрес датчика.
    saveValue<int>(kDs18Sensor1IndexStorageKey, value); // Сохраняем выбранный индекс в короткий ключ NVS.
    if (DS1Assigned) {
      sensors.setResolution(sensor0, 12); // Выставляем разрешение sensor0.
      sensors.requestTemperaturesByAddress(sensor0); // Запрашиваем измерение для sensor0.
    }
    if (DS2Assigned) {
      sensors.setResolution(sensor1, 12); // Выставляем разрешение sensor1.
      sensors.requestTemperaturesByAddress(sensor1); // Запрашиваем измерение для sensor1.
    }
  }
}


// ---------- NTP (синхронизация времени) ----------
 WiFiUDP ntpUDP;


namespace {
constexpr int kP4Core = 0;
constexpr int kAppCore = 1;
constexpr uint32_t kTaskStackSize = 8192;

void appSetup() {
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

  const int rtcLegacyMicrosPerHour = loadValue<int>("rtcCorrUsHr", loadValue<int>("rtcHourlyCorrection", 0) * 1000000);
  const int rtcLegacyMicrosPerMinute = loadValue<int>("rtcCorrUsMin", rtcLegacyMicrosPerHour / 60);
  const int rtcLegacyMillisPerMinute = rtcLegacyMicrosPerMinute / 1000;
  const int rtcStoredMillisPerMinute = loadValue<int>(kRtcCorrectionStorageKey, rtcLegacyMillisPerMinute);
  rtcCorrectionMillisPerMinute = normalizeRtcCorrectionMsPerMinute(rtcStoredMillisPerMinute);
  saveValue<int>(kRtcCorrectionStorageKey, rtcCorrectionMillisPerMinute);
  
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




  setup_ds18(Ds18Sensor0Address, Ds18Sensor1Address); // Инициализируем датчики и загружаем адреса.

    // Загружаем последние выбранные индексы DS18B20 из коротких ключей NVS (<=15 символов).
  Ds18Sensor0Index = loadValue<int>(kDs18Sensor0IndexStorageKey, Ds18Sensor0Index);
  Ds18Sensor1Index = loadValue<int>(kDs18Sensor1IndexStorageKey, Ds18Sensor1Index);

  interface(); // Для загрузки и выгрузки из памяти EEPOM сохраненных данных
  syncCleanDaysFromSelection();

  ColorRGB = LedColorMode.equalsIgnoreCase("manual");

  if(LedAutoplayDuration < 1) LedAutoplayDuration = 1;

  new_bright = LedBrightness;




  // ---------- Настройка графиков ----------
  loadGraph();

  dash.begin(); // Запуск дашборда

  Serial.printf(
    "Heap Free: %u | Heap Min: %u | Max Block: %u | PSRAM Free: %u\n",
    ESP.getFreeHeap(),
    ESP.getMinFreeHeap(),
    ESP.getMaxAllocHeap(),
    ESP.getFreePsram()
  );

}


void appLoop() {
  
 static int lastDs18ScanButton = 0; // Предыдущее состояние кнопки поиска датчиков.
  static uint32_t lastCommentUpdate = 0; // Таймер для обновления комментариев.
  wifiModuleLoop(); // Обновляем состояние Wi-Fi модуля.

  if (Ds18ScanButton != lastDs18ScanButton) { // Проверяем, изменилось ли состояние кнопки.
    lastDs18ScanButton = Ds18ScanButton; // Запоминаем текущее состояние кнопки.
    if (Ds18ScanButton == 1) { // Если нажали кнопку поиска.
      InfoString = scanDs18Sensors(); // Выполняем поиск и выводим информацию.
      Ds18Sensor0Index = -1; // Сбрасываем выбор, чтобы можно было переназначить.
      Ds18Sensor1Index = -1; // Сбрасываем выбор, чтобы можно было переназначить.
      Ds18ScanButton = 0; // Сбрасываем кнопку в интерфейсе.
      saveButtonState("ds18ScanButton", 0); // Сохраняем сброс кнопки в память.
    }
  }

  // Обновление времени через NTP/Nextion/память
  NPT_Time(period_get_NPT_Time);
  CurrentTime = getCurrentDateTime();   // Получение текущего времени




 ///////////////////////////////////////////////////////////////////////////////////////
  Temp_DS18B20(5000); //Измеряем температуру
 ///****************************  Nextion - проверка прихода данных Tx/Rx ****************************************/




  // Генерация случайных значений для демонстрации
  RandomVal = random(0,50);                     // Случайное число
  Speed = random(150, 350) / 10.0f;            // Случайная скорость
  // Temperatura = random(220, 320) / 10.0f;      // Случайная температура
  Temperatura = DS1;      // Температура в бассейне
  



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
//  "💧 Вода чистая",
//     "🧪 pH в норме",
//     "🌡 Температура стабильна",
//     "🧹 Фильтр промыт",
//     "✅ Все системы в порядке",
//     "⚠️ Низкий уровень воды",
//     "🔆 Подсветка включена",
//     "🕒 Работа по таймеру",
//     "Все ок 👍",
//     "Работает как зверь 🦾",
//     "Сегодня повезёт! ✨",
//     "Турбо-режим активирован 🚀",
//     "Отличный выбор 😉",
//     "Готово! 🔧",
//     "Запускаю магию 🪄",
//     "Миссия выполнена ✅",
//     "Стабильный поток 🌊",
//     "Идеальный баланс ⚖️",
//     "Система бодра 🧠",
//     "Проверка пройдена 🧾",
//     "Свет сияет 💡",
//     "Насос в тонусе 🏋️",
//     "Фильтр крутится 🔄",
//     "Датчики на посту 📡",
//     "Режим комфорта 😌",
//     "Температура в цель 🎯",
//     "Плавный ход 🛶",
//     "Лёгкий бриз 🌬️",
//     "Чистота на уровне 🧽",
//     "Скорость стабильна 🧭",
//     "Пузырьки счастья 🫧",
//     "Охлаждение ровное ❄️",
//     "Прогрев идёт 🔥",
//     "Свежесть гарантирована 🍃",
//     "Система на чеку 🛡️",
//     "Таймеры синхронизированы ⏱️",
//     "Запас мощности есть ⚡",
//     "Сетевое соединение крепкое 📶",
//     "Путь открыт 🛤️",
//     "Отзыв отличный ⭐",
//     "Ровные показатели 📈",
//     "Мягкий режим 🧸",
//     "Пики сглажены 🪂",
//     "Зелёный свет 🟢",
//     "Сигнал принят ✅",
//     "Отличная циркуляция 🔁",
//     "Все параметры в норме 🧩",
//     "Безопасность активна 🧯",
//     "Экономичный режим 💸",
//     "Мощность оптимальна 🧰",
//     "Стабильная работа 🧊",
//     "Без перебоев 🧯",
//     "Уровень точный 🎛️",
//     "График красивый 📊",
//     "Всё идёт по плану 🗺️",
//     "Сервисный режим ✅",
//     "Уверенный старт 🏁",
//     "Режим тишины 🤫",
//     "Комфортно и тихо 🪶",
//     "Модуляция плавная 🎚️",
//     "Капля к капле 💦",
//     "Бережный режим 🤍",
//     "Держим курс 🧭",
//     "Система улыбается 😄",
//     "Дышим ровно 🫁",
//     "Всё под контролем 🎮"
"Соблюдай регламент безопасности на опасном производственном объекте и подтверждай готовность перед пуском",
    "Проверь исправность блокировок и только затем запускай оборудование, это обязательное требование",
    "Работай в СИЗ по нормам объекта и подавай личный пример ответственного поведения",
    "Перед включением убедись, что зона ограждена и посторонних нет, так мы бережем людей",
    "Заполняй журнал осмотров без пропусков и поддерживай культуру дисциплины",
    "При отклонениях немедленно останови процесс и доложи руководителю смены",
    "Контролируй давление и температуру по регламенту, соблюдение сохраняет безопасность",
    "Выполняй блокировку энергии перед обслуживанием и не допускай самовольного пуска",
    "Следи за исправностью датчиков и сигнализации, это основа надежной защиты",
    "Соблюдай порядок на рабочем месте и помогай коллегам поддерживать чистоту",
    "Проверяй заземление и целостность кабелей до начала смены",
    "Носи каску, очки и перчатки всегда, даже при кратких операциях",
    "Прерывай работу при утечке и действуй по плану ликвидации аварий",
    "Держи пути эвакуации свободными и напоминай об этом товарищам",
    "Согласуй любые изменения режима с ответственным лицом и фиксируй решения",
    "Соблюдай дистанцию до движущихся частей и используй ограждения",
    "Проверяй исправность клапанов и пломб перед подачей среды",
    "Не допускай перегрузок оборудования и корректируй режим по инструкции",
    "Своевременно проходи инструктажи и поддерживай обучение команды",
    "Убедись в наличии разрешения на работы повышенной опасности",
    "Используй только исправный инструмент и сообщай о дефектах",
    "Планируй операции так, чтобы минимизировать риски для людей и среды",
    "Поддерживай устойчивую связь с диспетчером и подтверждай команды",
    "Следи за уровнем жидкости в пределах нормы и не допускай сухого хода",
    "Проверяй состояние фильтров и не игнорируй плановое обслуживание",
    "При шуме или вибрации выше нормы остановись и проведи диагностику",
    "Действуй спокойно при тревоге и выполняй предписанные шаги без суеты",
    "Соблюдай запрет на курение и открытый огонь в опасных зонах",
    "Не работай в одиночку там, где регламент требует напарника",
    "Фиксируй все замечания и предлагай улучшения безопасности",
    "Соблюдай порядок переключений и подтверждай каждое действие",
    "Проверяй герметичность соединений и устраняй течи по инструкции",
    "Сохраняй концентрацию и исключай отвлечения во время операций",
    "Разрешай доступ на площадку только уполномоченным сотрудникам",
    "Используй ограждения и бирки блокировки, чтобы никто не пострадал",
    "Соблюдай температурные пределы и не допускай перегрева",
    "Поддерживай исправность огнетушителей и контролируй сроки",
    "Следи за уровнем газоанализаторов и вовремя калибруй их",
    "Не обходи защиту и не отключай сигнализацию без разрешения",
    "Уважай знаки безопасности и напоминай о них коллегам",
    "Делай осмотр перед запуском и после остановки по чек-листу",
    "Сообщай о неисправностях сразу, чтобы предотвратить инциден",
    "Соблюдай режимы вентиляции и контролируй свежий воздух",
    "Держи инструкции под рукой и действуй строго по процедурам",
    "Проверяй резервное питание и готовность аварийного останова",
    "Выполняй обходы вовремя и подтверждай результаты в журнале",
    "Не превышай допустимую скорость и контролируй плавность работы",
    "Поддерживай безопасное расстояние до горячих поверхностей",
    "Убедись, что средства связи работают, и регулярно проверяй канал",
    "Используй разрешенные материалы и исключай самовольные замены",
    "Работай без спешки, безопасность важнее минутной экономии",
    "Соблюдай график обслуживания и помогай коллегам выполнять план",
    "Действуй по инструкции при отключении питания и не рискуй",
    "Проверяй освещенность рабочих зон и обеспечивай видимость",
    "Следи за запиранием шкафов и доступом к опасным узлам",
    "Не допускай скопления отходов и держи площадку чистой",
    "Проверяй корректность показаний приборов и реагируй на отклонения",
    "Уважай процедуру допуска подрядчиков и контролируй их работу",
    "Поддерживай порядок документов и своевременно обновляй записи",
    "Соблюдай нормы шума и используй защиту слуха при необходимости",
    "Планируй остановы заранее и координируй их с ответственными",
    "Помни о личной ответственности и поощряй безопасные привычки",
    "Следи за исправностью аварийных кнопок и не закрывай доступ",
    "Проверяй состояние лестниц и площадок перед подъемом",
    "Действуй честно и сообщай о рисках, это помогает всем",
    "Подавай пример культуры безопасности и поддерживай коллег"
  };
    int commentIntervalSeconds = MotorSpeedCommentSetting;
  if (commentIntervalSeconds < 1) {
    commentIntervalSeconds = 1;
  }
  if ((uint32_t)(millis() - lastCommentUpdate) >= static_cast<uint32_t>(commentIntervalSeconds) * 1000U) {
    Comment = comments[random(0, 66)];
        lastCommentUpdate = millis();
  }
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
}

void p4Task(void *parameter) {
  setupP4();
  for (;;) {
    loopP4();
    vTaskDelay(1);
  }
}

void appTask(void *parameter) {
  appSetup();
  for (;;) {
    appLoop();
    vTaskDelay(1);
  }
}
}  // namespace

/* ---------- Setup ---------- */
void setup() {
  Serial.begin(115200);
  xTaskCreatePinnedToCore(p4Task, "P4Task", kTaskStackSize, nullptr, 2, nullptr, kP4Core);
  xTaskCreatePinnedToCore(appTask, "AppTask", kTaskStackSize, nullptr, 1, nullptr, kAppCore);
}

/* ---------- Loop ---------- */
void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
