//graph.cpp — работа графика
#pragma once // защита от повторного включения заголовочного файла

#include <Arduino.h> // базовые типы Arduino, millis(), String и т.п.
#include <ArduinoJson.h> // работа с JSON для сохранения/загрузки настроек
#include <algorithm> // lower_bound для сохранения хронологического порядка точек
#include <functional> // std::function для callback'ов источников данных
#include <map> // ассоциативные контейнеры для хранения серий и настроек
#include <memory> // immutable shared_ptr-снимки history для Web
#include <utility> // std::move для передачи большого буфера истории без копии
#include <vector> // динамические массивы точек графика

#include "fs_utils.h" // утилиты работы с файловой системой (SPIFFS)

using std::vector; // упрощение записи vector<>

// Числовая метка времени заметно компактнее строки "08.08.2026 13:15:09".
// Это важно для истории до 5000 точек и одновременно позволяет браузеру
// корректно разбивать график на сегодня, вчера и предыдущие дни.
struct GraphPoint {
  uint32_t epoch = 0; // Локальное время устройства в формате Unix epoch.
  float value = 0; // Значение измеряемого параметра.
  uint16_t events = 0; // Сколько служебных событий попало в интервал этой точки.
}; // одна точка графика: время + значение + необязательные события

inline String sanitizeSeriesId(const String &series){ // приводит имя серии к безопасному имени файла
  String safe = series; // копируем исходное имя серии
  safe.replace("/", "_"); // запрещаем символы путей
  safe.replace("\\", "_"); // запрещаем escape-пути
  safe.replace("..", "_"); // защита от выхода за каталог
  if(!safe.length()) safe = "main"; // если имя пустое — используем main
  if(safe.length() > 16) safe = safe.substring(0, 16); // ограничиваем длину имени файла
  return safe; // возвращаем безопасное имя серии
}

inline void trimGraphPoints(vector<GraphPoint> &points, size_t limit){ // ограничивает количество точек в серии
  if(limit == 0) return; // если лимит нулевой — ничего не делаем
  if(points.size() <= limit) return; // серия уже укладывается в лимит
  points.erase(points.begin(), points.begin() + (points.size() - limit)); // удаляем старый блок одной операцией
}

inline const unsigned long minGraphUpdateInterval = 5000; // Минимальный интервал обновления графика: 5 секунд.
inline const unsigned long maxGraphUpdateInterval = 24UL * 60UL * 60UL * 1000UL; // Максимальный интервал обновления графика: 24 часа.
inline const unsigned long defaultGraphUpdateInterval = 60UL * 60UL * 1000UL; // Интервал графиков по умолчанию: 60 минут.
inline const int minGraphPoints = 1; // минимальное допустимое количество точек
inline const int defaultGraphPoints = 100; // безопасный стартовый размер; пользователь может увеличить его до максимума
inline const int maxGraphPoints = 5000; // верхняя граница выбора Max Points для каждой серии
inline const char* chlorinePpmGraphSeriesId(){ return "FloatСl"; } // ID видимого графика хлора ppmCl; буква С здесь кириллическая, как в UI.
inline const char* chlorineOrpGraphSeriesId(){ return "FloatClOrp"; } // ID отдельного графика ORP, который также дает ORP-колонку таблице ppmCl.
inline bool isChlorinePpmGraphSeries(const String &series){ return series == chlorinePpmGraphSeriesId(); } // Проверяем, что запрошена видимая серия хлора.

// При длинной истории нельзя полностью переписывать файл после каждой точки.
// Новые записи сбрасываются хвостом, а полная перезапись выполняется только
// после накопления запаса сверх выбранного лимита.
inline const size_t graphTrimSlackMin = 64;
inline const size_t graphTrimSlackMax = 512; // Не держим по 2500 лишних точек при лимите 5000.
inline const unsigned long graphFlushIntervalMs = 30000;
inline const size_t graphFlushPendingLimit = 32;
inline std::map<String, size_t> seriesUnsavedTail;
inline std::map<String, unsigned long> seriesLastFlush;
inline std::map<String, bool> seriesRewriteRequired; // Повтор атомарной rewrite только после реальной ошибки файловой системы.

inline size_t graphTrimSlackFor(size_t limit){
  const size_t half = limit / 2;
  const size_t slack = half > graphTrimSlackMin ? half : graphTrimSlackMin;
  return slack < graphTrimSlackMax ? slack : graphTrimSlackMax;
}

// Web и основная логика работают на разных ядрах, поэтому снимок истории и
// добавление точки не должны одновременно менять один и тот же vector/map.
inline SemaphoreHandle_t graphDataMutex = nullptr;

inline void ensureGraphDataMutex(){
  if(graphDataMutex == nullptr) graphDataMutex = xSemaphoreCreateMutex();
}

class GraphDataGuard {
 public:
  explicit GraphDataGuard(TickType_t waitTicks = portMAX_DELAY){
    ensureGraphDataMutex();
    locked_ = graphDataMutex != nullptr && xSemaphoreTake(graphDataMutex, waitTicks) == pdTRUE;
  }
  ~GraphDataGuard(){ if(locked_) xSemaphoreGive(graphDataMutex); }
  GraphDataGuard(const GraphDataGuard&) = delete;
  GraphDataGuard& operator=(const GraphDataGuard&) = delete;
  bool locked() const { return locked_; }

 private:
  bool locked_ = false;
};

// Настройки могут одновременно сохраняться из нескольких HTTP-запросов. Их
// временные файлы общие для серии, поэтому отдельный mutex защищает весь цикл
// write -> flush -> atomic rename. В отличие от graphDataMutex ожидание здесь
// ограничено: сетевой callback сможет вернуть 503, а не зависнуть до WDT.
inline SemaphoreHandle_t graphSettingsIoMutex = nullptr;

inline void ensureGraphSettingsIoMutex(){
  if(graphSettingsIoMutex == nullptr) graphSettingsIoMutex = xSemaphoreCreateMutex();
}

class GraphSettingsIoGuard {
 public:
  explicit GraphSettingsIoGuard(TickType_t waitTicks){
    ensureGraphSettingsIoMutex();
    locked_ = graphSettingsIoMutex != nullptr && xSemaphoreTake(graphSettingsIoMutex, waitTicks) == pdTRUE;
  }
  ~GraphSettingsIoGuard(){ if(locked_) xSemaphoreGive(graphSettingsIoMutex); }
  GraphSettingsIoGuard(const GraphSettingsIoGuard&) = delete;
  GraphSettingsIoGuard& operator=(const GraphSettingsIoGuard&) = delete;
  bool locked() const { return locked_; }

 private:
  bool locked_ = false;
};

inline String graphDataPath(const String &series){ // формирует путь к файлу данных графика
  return "/graph_" + sanitizeSeriesId(series) + ".dat"; // имя файла на основе серии
}

struct GraphSettings { unsigned long updateInterval; int maxPoints; }; // настройки графика: интервал и размер

// HTTP callbacks AsyncWebServer выполняются в задаче async_tcp. Любая запись
// SPIFFS из этой задачи может остановить её на время erase/program flash и
// привести к Task WDT. Поэтому Web только кладёт маленький запрос в RAM, а
// фактическую запись выполняет отдельная служебная задача.
enum class GraphSettingsSaveState : uint8_t { Unknown, Pending, Saving, Saved, Failed };

struct GraphSettingsSaveJob {
  uint32_t id = 0;
  String series;
  String requestToken;
  uint32_t clientRevision = 0;
  GraphSettings settings{defaultGraphUpdateInterval, defaultGraphPoints};
  GraphSettingsSaveState state = GraphSettingsSaveState::Unknown;
  bool uiSnapshotNotified = false;
  String error;
  unsigned long completedAt = 0;
};

inline bool publishGraphSeriesSettings(const String &series, GraphSettings settings,
                                       GraphSettings *previous, bool requireExisting);

inline SemaphoreHandle_t graphSettingsQueueMutex = nullptr;
inline TaskHandle_t graphSettingsWorkerHandle = nullptr;
inline std::vector<GraphSettingsSaveJob> graphSettingsSaveJobs;
inline uint32_t graphSettingsNextJobId = 1;
inline constexpr size_t graphSettingsMaxActiveJobs = 8; // Только Pending/Saving ограничивают прием новых сохранений.
inline constexpr size_t graphSettingsMaxHistoryJobs = 64; // Завершенные job остаются доступны status polling отдельно.
inline constexpr unsigned long graphSettingsJobRetentionMs = 120000;

inline void ensureGraphSettingsQueueMutex(){
  if(graphSettingsQueueMutex == nullptr) graphSettingsQueueMutex = xSemaphoreCreateMutex();
}

inline const char *graphSettingsSaveStateText(GraphSettingsSaveState state){
  switch(state){
    case GraphSettingsSaveState::Pending: return "pending";
    case GraphSettingsSaveState::Saving: return "saving";
    case GraphSettingsSaveState::Saved: return "saved";
    case GraphSettingsSaveState::Failed: return "failed";
    default: return "unknown";
  }
}

inline String graphConfigPath(const String &series){ // путь к JSON-файлу настроек серии
  String safe = sanitizeSeriesId(series);
  if(safe.length() > 12) safe = safe.substring(0, 12); // 14 + 12 + 5 = 31 байт: предел имени SPIFFS.
  return "/graph_config_" + safe + ".json"; // имя файла конфигурации
}

inline bool loadGraphSettings(const String &series, GraphSettings &settings,
                              TickType_t waitTicks = pdMS_TO_TICKS(250)){ // загрузка настроек серии из SPIFFS
  if(!spiffsMounted) return false; // если файловая система не смонтирована — выходим
  GraphSettingsIoGuard ioGuard(waitTicks); // Не читаем путь между rename старого и нового файла.
  if(!ioGuard.locked()) return false; // Сетевой запрос не должен ждать Flash без ограничения.
  String path = graphConfigPath(series); // получаем путь к конфигурации
  const String configStem = path.substring(0, path.length() - 5); // Убираем .json, чтобы не превысить SPIFFS name limit 31.
  const String temporaryPath = configStem + ".tmp";
  const String backupPath = configStem + ".bak";
  if(!SPIFFS.exists(path) && SPIFFS.exists(backupPath)) SPIFFS.rename(backupPath, path); // Восстановление после reset между двумя rename.
  if(SPIFFS.exists(path)){
    if(SPIFFS.exists(backupPath)) SPIFFS.remove(backupPath);
    if(SPIFFS.exists(temporaryPath)) SPIFFS.remove(temporaryPath);
  }
  if(!SPIFFS.exists(path)) return false; // если файла нет — используем значения по умолчанию
  File f = SPIFFS.open(path, FILE_READ); // открываем файл на чтение
  if(!f) return false; // если файл не открылся — ошибка
  String payload = f.readString(); // читаем весь файл в строку
  f.close(); // закрываем файл
  if(payload.length() == 0) return false; // пустой файл — игнорируем
  StaticJsonDocument<256> doc; // JSON-документ фиксированного размера
  if(deserializeJson(doc, payload) != DeserializationError::Ok) return false; // парсинг JSON
  settings.updateInterval = doc["updateInterval"] | settings.updateInterval; // загружаем интервал обновления
  settings.maxPoints = doc["maxPoints"] | settings.maxPoints; // загружаем лимит точек
  return true; // успешная загрузка
}

inline bool saveGraphSettings(const String &series, const GraphSettings &settings,
                              TickType_t waitTicks = pdMS_TO_TICKS(250)){ // атомарное сохранение настроек графика
  if(!spiffsMounted) return false; // если SPIFFS недоступен — выходим
  GraphSettingsIoGuard ioGuard(waitTicks); // Два HTTP save одной серии не используют один .tmp одновременно.
  if(!ioGuard.locked()) return false; // Вместо неограниченного ожидания вызывающий код сможет вернуть ошибку.
  String path = graphConfigPath(series); // путь к файлу настроек
  const String configStem = path.substring(0, path.length() - 5); // .tmp/.bak заменяют .json и остаются в лимите SPIFFS.
  const String temporaryPath = configStem + ".tmp";
  const String backupPath = configStem + ".bak";
  if(!SPIFFS.exists(path) && SPIFFS.exists(backupPath)) SPIFFS.rename(backupPath, path); // Сначала возвращаем рабочую копию после прошлого reset.
  if(SPIFFS.exists(path) && SPIFFS.exists(backupPath)) SPIFFS.remove(backupPath); // Старый backup больше не нужен.
  if(SPIFFS.exists(temporaryPath)) SPIFFS.remove(temporaryPath); // Остаток только от ранее прерванной попытки.
  File f = SPIFFS.open(temporaryPath, FILE_WRITE); // Рабочий config остается доступен до полной записи нового.
  if(!f){ // если файл не открылся
    Serial.println("Failed to open graph config for writing: " + path); // сообщение об ошибке
    return false; // выходим
  }
  StaticJsonDocument<256> doc; // создаём JSON-документ
  doc["updateInterval"] = settings.updateInterval; // сохраняем интервал обновления
  doc["maxPoints"] = settings.maxPoints; // сохраняем максимальное количество точек
  const size_t written = serializeJson(doc, f); // записываем JSON во временный файл
  f.flush();
  const bool writeOk = written > 0 && f.getWriteError() == 0;
  f.close(); // закрываем файл
  if(!writeOk){
    SPIFFS.remove(temporaryPath);
    Serial.println("Failed to write graph config: " + path);
    return false;
  }

  // Сначала полностью подготовлен новый файл. Старый лишь переименовывается в
  // backup и может быть восстановлен при любой ошибке следующего rename.
  if(SPIFFS.exists(backupPath)) SPIFFS.remove(backupPath);
  const bool hadOriginal = SPIFFS.exists(path);
  if(hadOriginal && !SPIFFS.rename(path, backupPath)){
    SPIFFS.remove(temporaryPath);
    return false;
  }
  if(!SPIFFS.rename(temporaryPath, path)){
    if(hadOriginal && SPIFFS.exists(backupPath)) SPIFFS.rename(backupPath, path);
    SPIFFS.remove(temporaryPath);
    return false;
  }
  if(SPIFFS.exists(backupPath)) SPIFFS.remove(backupPath);
  return true;
}

inline void pruneGraphSettingsJobsLocked(unsigned long now){
  graphSettingsSaveJobs.erase(
    std::remove_if(graphSettingsSaveJobs.begin(), graphSettingsSaveJobs.end(),
      [now](const GraphSettingsSaveJob &job){
        const bool completed = job.state == GraphSettingsSaveState::Saved || job.state == GraphSettingsSaveState::Failed;
        return completed && job.completedAt != 0 && now - job.completedAt >= graphSettingsJobRetentionMs;
      }),
    graphSettingsSaveJobs.end());
}

inline bool graphSettingsJobIsActive(const GraphSettingsSaveJob &job){
  return job.state == GraphSettingsSaveState::Pending || job.state == GraphSettingsSaveState::Saving;
}

inline bool graphSettingsJobIsCompleted(const GraphSettingsSaveJob &job){
  return job.state == GraphSettingsSaveState::Saved || job.state == GraphSettingsSaveState::Failed;
}

inline bool makeRoomInGraphSettingsHistoryLocked(){
  // vector сохраняет FIFO-порядок, поэтому первый completed — самый старый.
  while(graphSettingsSaveJobs.size() >= graphSettingsMaxHistoryJobs){
    const auto completed = std::find_if(graphSettingsSaveJobs.begin(), graphSettingsSaveJobs.end(),
      [](const GraphSettingsSaveJob &job){ return graphSettingsJobIsCompleted(job); });
    if(completed == graphSettingsSaveJobs.end()) return false;
    graphSettingsSaveJobs.erase(completed);
  }
  return true;
}

inline uint32_t queueGraphSettingsSave(const String &series, const GraphSettings &settings,
                                       const String &requestToken, uint32_t clientRevision){
  // Без живого worker Pending-job никто не обработает, а Web будет опрашивать
  // его бесконечно. Нулевой id позволяет HTTP handler сразу вернуть 503.
  if(graphSettingsWorkerHandle == nullptr){
    Serial.println("[GRAPH] Settings save rejected: worker is not running");
    return 0;
  }
  ensureGraphSettingsQueueMutex();
  if(graphSettingsQueueMutex == nullptr || xSemaphoreTake(graphSettingsQueueMutex, pdMS_TO_TICKS(20)) != pdTRUE) return 0;
  pruneGraphSettingsJobsLocked(millis());
  if(requestToken.length()){
    for(const auto &existing : graphSettingsSaveJobs){
      if(existing.series != series || existing.requestToken != requestToken ||
         existing.clientRevision != clientRevision) continue;
      const bool sameSettings = existing.settings.updateInterval == settings.updateInterval &&
                                existing.settings.maxPoints == settings.maxPoints;
      const uint32_t existingId = sameSettings ? existing.id : 0;
      xSemaphoreGive(graphSettingsQueueMutex);
      return existingId; // Идемпотентный повтор потерянного HTTP ACK не создаёт новую Flash-job.
    }
  }
  const size_t activeJobs = static_cast<size_t>(std::count_if(
    graphSettingsSaveJobs.begin(), graphSettingsSaveJobs.end(),
    [](const GraphSettingsSaveJob &job){ return graphSettingsJobIsActive(job); }));
  if(activeJobs >= graphSettingsMaxActiveJobs || !makeRoomInGraphSettingsHistoryLocked()){
    xSemaphoreGive(graphSettingsQueueMutex);
    return 0;
  }
  uint32_t id = graphSettingsNextJobId++;
  if(id == 0) id = graphSettingsNextJobId++;
  GraphSettingsSaveJob job;
  job.id = id;
  job.series = series;
  job.requestToken = requestToken;
  job.clientRevision = clientRevision;
  job.settings = settings;
  job.state = GraphSettingsSaveState::Pending;
  graphSettingsSaveJobs.push_back(std::move(job));
  xSemaphoreGive(graphSettingsQueueMutex);
  return id;
}

inline bool getGraphSettingsSaveJob(uint32_t id, GraphSettingsSaveJob &snapshot){
  ensureGraphSettingsQueueMutex();
  if(id == 0 || graphSettingsQueueMutex == nullptr || xSemaphoreTake(graphSettingsQueueMutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;
  bool found = false;
  for(const auto &job : graphSettingsSaveJobs){
    if(job.id != id) continue;
    snapshot = job;
    found = true;
    break;
  }
  xSemaphoreGive(graphSettingsQueueMutex);
  return found;
}

inline bool consumeGraphSettingsUiSnapshotNotification(uint32_t id){
  ensureGraphSettingsQueueMutex();
  if(id == 0 || graphSettingsQueueMutex == nullptr ||
     xSemaphoreTake(graphSettingsQueueMutex, pdMS_TO_TICKS(20)) != pdTRUE) return false;
  bool shouldInvalidate = false;
  for(auto &job : graphSettingsSaveJobs){
    if(job.id != id || job.state != GraphSettingsSaveState::Saved || job.uiSnapshotNotified) continue;
    job.uiSnapshotNotified = true;
    shouldInvalidate = true;
    break;
  }
  xSemaphoreGive(graphSettingsQueueMutex);
  return shouldInvalidate;
}

inline void graphSettingsWorkerTask(void *){
  for(;;){
    uint32_t jobId = 0;
    String series;
    GraphSettings settings{defaultGraphUpdateInterval, defaultGraphPoints};

    if(graphSettingsQueueMutex != nullptr && xSemaphoreTake(graphSettingsQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE){
      for(auto &job : graphSettingsSaveJobs){
        if(job.state != GraphSettingsSaveState::Pending) continue;
        job.state = GraphSettingsSaveState::Saving;
        jobId = job.id;
        series = job.series;
        settings = job.settings;
        break;
      }
      xSemaphoreGive(graphSettingsQueueMutex);
    }

    if(jobId == 0){
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    const bool fileSaved = saveGraphSettings(series, settings, pdMS_TO_TICKS(1000));
    const bool saved = fileSaved && publishGraphSeriesSettings(series, settings, nullptr, false);
    // Выбранная job обязана перейти из Saving в конечное состояние. Worker —
    // единственный consumer, а producers держат queue-lock микросекунды.
    if(graphSettingsQueueMutex != nullptr && xSemaphoreTake(graphSettingsQueueMutex, portMAX_DELAY) == pdTRUE){
      for(auto &job : graphSettingsSaveJobs){
        if(job.id != jobId) continue;
        job.state = saved ? GraphSettingsSaveState::Saved : GraphSettingsSaveState::Failed;
        job.error = saved ? String() : String("Не удалось сохранить настройки во Flash");
        job.completedAt = millis();
        break;
      }
      xSemaphoreGive(graphSettingsQueueMutex);
    }
    if(!saved) Serial.printf("[GRAPH] Deferred settings write failed: %s\n", series.c_str());
    vTaskDelay(1);
  }
}

inline bool startGraphSettingsWorker(){
  ensureGraphSettingsQueueMutex();
  if(graphSettingsQueueMutex == nullptr){
    Serial.println("[GRAPH] Failed to create settings queue mutex");
    return false;
  }
  if(graphSettingsWorkerHandle != nullptr) return true;
  graphSettingsSaveJobs.reserve(graphSettingsMaxHistoryJobs); // Ни одного realloc из async_tcp при enqueue.
  TaskHandle_t createdWorker = nullptr;
  const BaseType_t result = xTaskCreatePinnedToCore(graphSettingsWorkerTask, "GraphSettingsIO", 4096, nullptr, 1,
                                                    &createdWorker, 0);
  if(result != pdPASS || createdWorker == nullptr){
    graphSettingsWorkerHandle = nullptr;
    Serial.println("[GRAPH] Failed to create settings worker task");
    return false;
  }
  graphSettingsWorkerHandle = createdWorker;
  return true;
}


inline bool saveGraphSeries(const String &series, const vector<GraphPoint> &points){ // полная перезапись данных графика
  if(!spiffsMounted) return false; // если SPIFFS недоступен — выходим
  String path = graphDataPath(series); // путь к файлу данных
  const String temporaryPath = path + ".tmp";
  const String backupPath = path + ".bak";
  if(SPIFFS.exists(temporaryPath)) SPIFFS.remove(temporaryPath);
  File f = SPIFFS.open(temporaryPath, FILE_WRITE); // сначала пишем отдельный файл, не повреждая рабочую историю
  if(!f){ // если файл не открылся
    Serial.println("Failed to open graph file for writing: " + path); // вывод ошибки
    return false; // выходим
  }
  bool writeOk = true;
  // File.printf на каждую из 5000 точек создаёт тысячи отдельных обращений к
  // SPIFFS. Небольшой stack-буфер находится во внутренней RAM и записывается
  // крупными блоками; это одновременно быстрее и безопасно при Flash cache off.
  char writeBuffer[1024];
  size_t bufferedBytes = 0;
  auto flushWriteBuffer = [&]() -> bool {
    if(bufferedBytes == 0) return true;
    const size_t written = f.write(reinterpret_cast<const uint8_t *>(writeBuffer), bufferedBytes);
    if(written != bufferedBytes) return false;
    bufferedBytes = 0;
    vTaskDelay(1); // Между Flash-блоками обслуживаются idle/network задачи.
    return true;
  };
  for(const auto &p : points){ // перебираем все точки
    char line[80];
    const int lineLength = snprintf(line, sizeof(line), "%lu,%.3f,%u\n",
                                    static_cast<unsigned long>(p.epoch), p.value,
                                    static_cast<unsigned>(p.events));
    if(lineLength <= 0 || static_cast<size_t>(lineLength) >= sizeof(line)){
      writeOk = false;
      break;
    }
    if(bufferedBytes + static_cast<size_t>(lineLength) > sizeof(writeBuffer) && !flushWriteBuffer()){
      writeOk = false;
      break;
    }
    memcpy(writeBuffer + bufferedBytes, line, static_cast<size_t>(lineLength));
    bufferedBytes += static_cast<size_t>(lineLength);
  }
  if(writeOk && !flushWriteBuffer()) writeOk = false;
  f.flush();
  if(f.getWriteError() != 0) writeOk = false;
  f.close(); // закрываем файл
  if(!writeOk){
    SPIFFS.remove(temporaryPath);
    Serial.println("Failed to write complete graph file: " + path);
    return false;
  }

  if(SPIFFS.exists(backupPath)) SPIFFS.remove(backupPath);
  const bool hadOriginal = SPIFFS.exists(path);
  if(hadOriginal && !SPIFFS.rename(path, backupPath)){
    SPIFFS.remove(temporaryPath);
    return false;
  }
  if(!SPIFFS.rename(temporaryPath, path)){
    if(hadOriginal && SPIFFS.exists(backupPath)) SPIFFS.rename(backupPath, path);
    SPIFFS.remove(temporaryPath);
    return false;
  }
  if(SPIFFS.exists(backupPath)) SPIFFS.remove(backupPath);
  seriesUnsavedTail[series] = 0; // файл полностью соответствует массиву
  seriesRewriteRequired[series] = false;
  seriesLastFlush[series] = millis(); // запоминаем момент полной записи
  return true;
}

inline bool appendGraphTailToFile(const String &series, const vector<GraphPoint> &points, size_t count){ // дозапись накопленного хвоста
  if(!spiffsMounted || count == 0 || points.empty()) return false; // нечего или некуда писать
  if(count > points.size()) count = points.size(); // защита от рассинхронизации счётчика
  File f = SPIFFS.open(graphDataPath(series), FILE_APPEND); // открываем файл один раз на всю пачку
  if(!f) return false;
  for(size_t i = points.size() - count; i < points.size(); i++){
    const auto &p = points[i];
    if(f.printf("%lu,%.3f,%u\n", static_cast<unsigned long>(p.epoch), p.value, static_cast<unsigned>(p.events)) == 0){
      f.close();
      return false;
    }
  }
  f.flush();
  const bool writeOk = f.getWriteError() == 0;
  f.close();
  return writeOk;
}

// Разбор старой строки "08.08.2026 13:15:09" нужен для сохранения уже
// накопленной пользователем истории при переходе на компактный формат.
inline uint32_t parseLegacyGraphTime(const String &text){
  if(text.length() < 19) return 0;
  struct tm timeInfo = {};
  timeInfo.tm_mday = text.substring(0, 2).toInt();
  timeInfo.tm_mon = text.substring(3, 5).toInt() - 1;
  timeInfo.tm_year = text.substring(6, 10).toInt() - 1900;
  timeInfo.tm_hour = text.substring(11, 13).toInt();
  timeInfo.tm_min = text.substring(14, 16).toInt();
  timeInfo.tm_sec = text.substring(17, 19).toInt();
  if(timeInfo.tm_year < 100 || timeInfo.tm_mon < 0 || timeInfo.tm_mon > 11) return 0;
  const time_t parsed = mktime(&timeInfo);
  return parsed > 0 ? static_cast<uint32_t>(parsed) : 0;
}

inline void loadGraphSeries(const String &series, vector<GraphPoint> &points,
                            size_t pointLimit = static_cast<size_t>(maxGraphPoints)){ // загрузка последних точек графика из файла
  points.clear(); // очищаем текущие данные
  if(!spiffsMounted) return; // если SPIFFS недоступен — выходим
  if(pointLimit == 0) pointLimit = static_cast<size_t>(defaultGraphPoints); // Нулевой лимит не должен загрузить неограниченный файл.
  if(pointLimit > static_cast<size_t>(maxGraphPoints)) pointLimit = static_cast<size_t>(maxGraphPoints);
  points.reserve(pointLimit); // Один PSRAM-блок вместо каскада realloc при загрузке длинной истории.
  const size_t loadSlack = graphTrimSlackFor(pointLimit); // Размер ограниченного временного хвоста при последовательном чтении.
  String path = graphDataPath(series); // путь к файлу данных
  const String temporaryPath = path + ".tmp";
  const String backupPath = path + ".bak";
  if(!SPIFFS.exists(path) && SPIFFS.exists(backupPath)) SPIFFS.rename(backupPath, path); // восстановление после сбоя между двумя rename
  if(SPIFFS.exists(path)){
    if(SPIFFS.exists(backupPath)) SPIFFS.remove(backupPath);
    if(SPIFFS.exists(temporaryPath)) SPIFFS.remove(temporaryPath);
  }
  if(!SPIFFS.exists(path)) return; // если файл отсутствует — выходим
  File f = SPIFFS.open(path, FILE_READ); // открываем файл на чтение
  if(!f){ // если файл не открылся
    Serial.println("Failed to open graph file for reading: " + path); // вывод ошибки
    return; // выходим
  }
  bool convertedLegacy = false; // старый строковый файл после чтения будет переписан компактно
  while(f.available()){ // читаем файл построчно
    String line = f.readStringUntil('\n'); // читаем одну строку
    line.trim(); // удаляем пробелы и переводы строк
    if(line.length() == 0) continue; // пропускаем пустые строки
    const int sep = line.indexOf(','); // первый разделитель отделяет время
    if(sep < 0) continue; // если формат неверный — пропускаем
    const int sep2 = line.indexOf(',', sep + 1); // необязательный счётчик событий
    const String stamp = line.substring(0, sep);
    GraphPoint gp; // создаём точку графика
    if(stamp.indexOf('.') >= 0 || stamp.indexOf(':') >= 0){
      gp.epoch = parseLegacyGraphTime(stamp);
      convertedLegacy = true;
    } else {
      gp.epoch = static_cast<uint32_t>(strtoul(stamp.c_str(), nullptr, 10));
    }
    if(gp.epoch == 0) continue; // точка без достоверного времени не участвует в графике
    gp.value = line.substring(sep + 1, sep2 < 0 ? line.length() : sep2).toFloat();
    gp.events = sep2 < 0 ? 0 : static_cast<uint16_t>(line.substring(sep2 + 1).toInt());
    points.push_back(gp); // добавляем точку в массив
    if(points.size() > pointLimit + loadSlack){
      // Для старого/поврежденного большого файла держим только хвост. Так при
      // maxPoints=100 временный vector не разрастается до тысяч записей.
      trimGraphPoints(points, pointLimit);
    }
  }
  f.close(); // закрываем файл
  trimGraphPoints(points, pointLimit); // В RAM всегда остается не больше выбранного Max Points.
  seriesUnsavedTail[series] = 0;
  seriesRewriteRequired[series] = false;
  seriesLastFlush[series] = millis();
  if(convertedLegacy && !points.empty()) saveGraphSeries(series, points); // история не теряется, а только меняет формат
}

// Основная серия графика "main"
inline vector<GraphPoint> graphPoints; // хранит точки основной серии графика с именем "main"

// Глобальные параметры графиков
inline int maxPoints; // максимальное количество точек, отображаемых на графике
inline unsigned long updateInterval; // интервал обновления графиков в миллисекундах

// Пользовательские серии графиков
inline std::map<String, vector<GraphPoint>> customGraphSeries; // набор графиков по имени серии и их точек

// Web читает immutable history без graphDataMutex. Writer под graphDataMutex
// создает новую ограниченную tail-копию, затем за короткую критсекцию заменяет
// shared_ptr. Уже начатый HTTP-ответ продолжает владеть старым vector, поэтому
// ни realloc, ни trim основной истории не инвалидируют его память.
struct GraphHistorySnapshot {
  std::shared_ptr<const vector<GraphPoint>> points;
  uint32_t revision = 0;
};

inline SemaphoreHandle_t graphHistorySnapshotMutex = nullptr;
inline std::map<String, GraphHistorySnapshot> graphHistorySnapshots;
inline uint32_t graphHistoryNextRevision = 1;

inline void ensureGraphHistorySnapshotMutex(){
  if(graphHistorySnapshotMutex == nullptr) graphHistorySnapshotMutex = xSemaphoreCreateMutex();
}

inline void publishGraphHistorySnapshotLocked(const String &series,
                                              const vector<GraphPoint> &source,
                                              size_t limit){
  // Вызывать только при уже удерживаемом graphDataMutex: source меняет writer.
  if(limit == 0 || limit > source.size()) limit = source.size();
  const size_t first = source.size() - limit;
  auto mutableSnapshot = std::make_shared<vector<GraphPoint>>();
  mutableSnapshot->reserve(limit);
  mutableSnapshot->insert(mutableSnapshot->end(), source.begin() + first, source.end());

  ensureGraphHistorySnapshotMutex();
  if(graphHistorySnapshotMutex == nullptr ||
     xSemaphoreTake(graphHistorySnapshotMutex, portMAX_DELAY) != pdTRUE) return;
  uint32_t revision = graphHistoryNextRevision++;
  if(revision == 0) revision = graphHistoryNextRevision++;
  graphHistorySnapshots[series] = {
    std::shared_ptr<const vector<GraphPoint>>(std::move(mutableSnapshot)), revision
  };
  xSemaphoreGive(graphHistorySnapshotMutex);
}

inline GraphHistorySnapshot getGraphHistorySnapshot(const String &series){
  ensureGraphHistorySnapshotMutex();
  if(graphHistorySnapshotMutex == nullptr ||
     xSemaphoreTake(graphHistorySnapshotMutex, portMAX_DELAY) != pdTRUE) return {};
  const auto it = graphHistorySnapshots.find(series);
  GraphHistorySnapshot snapshot = it != graphHistorySnapshots.end() ? it->second : GraphHistorySnapshot{};
  xSemaphoreGive(graphHistorySnapshotMutex);
  return snapshot;
}

// Источники данных (функции получения значений)
inline std::map<String, std::function<float()>> graphValueProviders; // функции, возвращающие текущее значение для каждой серии

// Конфигурации серий
inline std::map<String, GraphSettings> seriesConfig; // настройки (интервал, точки) для каждой серии графика

// Конфигурация графиков — это маленький RAM-объект, который нужен HTTP при
// построении страницы и сохранении настроек. Он намеренно НЕ защищается
// graphDataMutex: тот mutex может удерживаться во время записи большой history
// во SPIFFS. Под graphConfigMutex запрещены файловые операции и работа с
// массивами точек, поэтому снимок настроек всегда получается быстро и без 503.
inline SemaphoreHandle_t graphConfigMutex = nullptr;

inline void ensureGraphConfigMutex(){
  if(graphConfigMutex == nullptr) graphConfigMutex = xSemaphoreCreateMutex();
}

class GraphConfigGuard {
 public:
  GraphConfigGuard(){
    ensureGraphConfigMutex();
    locked_ = graphConfigMutex != nullptr && xSemaphoreTake(graphConfigMutex, portMAX_DELAY) == pdTRUE;
  }
  ~GraphConfigGuard(){ if(locked_) xSemaphoreGive(graphConfigMutex); }
  GraphConfigGuard(const GraphConfigGuard&) = delete;
  GraphConfigGuard& operator=(const GraphConfigGuard&) = delete;
  bool locked() const { return locked_; }

 private:
  bool locked_ = false;
};

inline GraphSettings normalizeGraphSettings(GraphSettings settings){
  if(settings.updateInterval < minGraphUpdateInterval) settings.updateInterval = minGraphUpdateInterval;
  if(settings.updateInterval > maxGraphUpdateInterval) settings.updateInterval = maxGraphUpdateInterval;
  if(settings.maxPoints < minGraphPoints) settings.maxPoints = minGraphPoints;
  if(settings.maxPoints > maxGraphPoints) settings.maxPoints = maxGraphPoints;
  return settings;
}

// Полный неизменяемый снимок для построения GET /. Копирование выполняется под
// отдельным коротким lock; после возврата HTTP больше не обращается к общей map.
inline std::map<String, GraphSettings> snapshotGraphSeriesConfig(){
  GraphConfigGuard guard;
  if(!guard.locked()) return {};
  return seriesConfig;
}

inline bool getGraphSeriesSettings(const String &series, GraphSettings &settings){
  GraphConfigGuard guard;
  if(!guard.locked()) return false;
  const auto it = seriesConfig.find(series);
  if(it == seriesConfig.end()) return false;
  settings = it->second;
  return true;
}

// Публикует только RAM-конфигурацию. Файл и history здесь принципиально не
// трогаются. requireExisting=true удобен HTTP: неизвестная серия не создаётся.
inline bool publishGraphSeriesSettings(const String &series, GraphSettings settings,
                                       GraphSettings *previous = nullptr,
                                       bool requireExisting = false){
  settings = normalizeGraphSettings(settings);
  GraphConfigGuard guard;
  if(!guard.locked()) return false;
  auto it = seriesConfig.find(series);
  if(requireExisting && it == seriesConfig.end()) return false;
  if(previous) *previous = it != seriesConfig.end()
                               ? it->second
                               : GraphSettings{defaultGraphUpdateInterval, defaultGraphPoints};
  seriesConfig[series] = settings;
  if(series == "main"){
    updateInterval = settings.updateInterval;
    maxPoints = settings.maxPoints;
  }
  return true;
}

enum class GraphSettingsUpdateResult : uint8_t { Accepted, UnknownSeries, QueueBusy, InternalError };

// Для HTTP порядок RAM-публикации обязан совпадать с FIFO-порядком Flash-job.
// Один короткий config-lock сериализует эти две операции, иначе два почти
// одновременных POST могли оставить в RAM более старое значение, а во Flash —
// более новое. Worker не берет graphConfigMutex, поэтому Flash здесь не ждём.
inline GraphSettingsUpdateResult queueGraphSeriesSettingsUpdate(
  const String &series, GraphSettings settings, uint32_t &jobId,
  const String &requestToken, uint32_t clientRevision
){
  jobId = 0;
  settings = normalizeGraphSettings(settings);
  GraphConfigGuard guard;
  if(!guard.locked()) return GraphSettingsUpdateResult::InternalError;
  auto it = seriesConfig.find(series);
  if(it == seriesConfig.end()) return GraphSettingsUpdateResult::UnknownSeries;
  jobId = queueGraphSettingsSave(series, settings, requestToken, clientRevision);
  if(jobId == 0) return GraphSettingsUpdateResult::QueueBusy;
  return GraphSettingsUpdateResult::Accepted;
}

// Время последнего обновления каждой серии
inline std::map<String, unsigned long> seriesLastUpdate; // таймстемп последнего добавления точки в серию

inline bool storeGraphPoint(const String &series, vector<GraphPoint> &points,
                            const GraphPoint &point, size_t limit); // реализация расположена после регистрации источников

// Загружает основной график и его настройки
inline void loadGraph(){ // инициализация основной серии графика
  GraphSettings mainCfg{defaultGraphUpdateInterval, defaultGraphPoints}; // Служебная серия main по умолчанию не занимает память под все 5000 точек.
  loadGraphSettings("main", mainCfg); // попытка загрузить сохранённые настройки из SPIFFS
  mainCfg = normalizeGraphSettings(mainCfg);

  {
    GraphDataGuard guard; // история защищена отдельно от маленькой конфигурации
    loadGraphSeries("main", graphPoints, static_cast<size_t>(mainCfg.maxPoints)); // загружаем только выбранный хвост основной серии
    if(graphPoints.size() > static_cast<size_t>(mainCfg.maxPoints)){
      trimGraphPoints(graphPoints, static_cast<size_t>(mainCfg.maxPoints));
      saveGraphSeries("main", graphPoints);
    }
    seriesLastUpdate["main"] = 0; // сбрасываем время последнего обновления
    publishGraphHistorySnapshotLocked("main", graphPoints, static_cast<size_t>(mainCfg.maxPoints));
  }
  publishGraphSeriesSettings("main", mainCfg); // отдельный быстрый RAM-снимок для Web
}

// Добавляет точку в основной график с учетом интервала
inline void addGraphPoint(uint32_t epoch, float v){ // добавление новой точки в основную серию
  if(epoch == 0 || !isfinite(v)) return; // без достоверного времени/значения точка бессмысленна
  GraphSettings cfg{defaultGraphUpdateInterval, defaultGraphPoints};
  getGraphSeriesSettings("main", cfg); // config-lock никогда не ожидает SPIFFS/history
  cfg = normalizeGraphSettings(cfg);

  GraphDataGuard guard; // web может одновременно запрашивать снимок истории

  unsigned long now = millis(); // текущее время работы устройства
  unsigned long last = seriesLastUpdate["main"]; // время последнего добавления точки
  if(now - last < cfg.updateInterval) return; // выходим, если интервал ещё не прошёл

  seriesLastUpdate["main"] = now; // обновляем время последнего добавления

  if(storeGraphPoint("main", graphPoints, {epoch, v, 0}, static_cast<size_t>(cfg.maxPoints))){
    publishGraphHistorySnapshotLocked("main", graphPoints, static_cast<size_t>(cfg.maxPoints));
  }
}

// Регистрирует источник данных для графика
inline void registerGraphSource( // регистрация новой серии графика
  const String &name, // имя серии
  const std::function<float()> &getter, // функция получения текущего значения
  const String &fallback="", // резервная серия для загрузки данных
  unsigned long defaultInterval=updateInterval, // интервал обновления по умолчанию
  int defaultMaxPoints=maxPoints // количество точек по умолчанию
){
  unsigned long safeDefaultInterval = defaultInterval < minGraphUpdateInterval
                                     ? minGraphUpdateInterval
                                     : defaultInterval; // защита от слишком малого интервала
  if(safeDefaultInterval > maxGraphUpdateInterval) safeDefaultInterval = maxGraphUpdateInterval; // Защита от интервалов больше 24 часов.

  GraphSettings cfg{safeDefaultInterval, defaultMaxPoints}; // создаём конфигурацию серии
  bool loaded = loadGraphSettings(name, cfg); // пробуем загрузить сохранённые настройки
  if(!loaded && fallback.length()) loadGraphSettings(fallback, cfg); // fallback-настройки при необходимости

  cfg = normalizeGraphSettings(cfg);

  // Сначала известен сохраненный лимит, и только потом читается history. Это
  // исключает временную загрузку всего старого файла при maxPoints=100.
  vector<GraphPoint> stored; // временное хранилище загруженных точек
  stored.reserve(static_cast<size_t>(cfg.maxPoints));
  loadGraphSeries(name, stored, static_cast<size_t>(cfg.maxPoints)); // загружаем выбранный хвост серии
  if(stored.empty() && fallback.length()){
    loadGraphSeries(fallback, stored, static_cast<size_t>(cfg.maxPoints)); // если пусто — пробуем fallback
  }

  {
    GraphDataGuard guard; // публикуем history/provider отдельно от config-lock
    graphValueProviders[name] = getter; // сохраняем функцию-источник значений
    seriesLastUpdate[name] = 0; // инициализируем таймер обновления
    if(!stored.empty()){ // если данные серии были загружены
      customGraphSeries[name] = std::move(stored); // Передаем PSRAM-буфер без второй большой копии.
    }
    const auto historyIt = customGraphSeries.find(name);
    if(historyIt != customGraphSeries.end()){
      publishGraphHistorySnapshotLocked(name, historyIt->second, static_cast<size_t>(cfg.maxPoints));
    } else {
      static const vector<GraphPoint> emptyHistory;
      publishGraphHistorySnapshotLocked(name, emptyHistory, 0);
    }
  }
  publishGraphSeriesSettings(name, cfg); // после регистрации серия видна Web в готовом состоянии
}

// Добавляет точку в серию, сохраняя порядок даже после ручной коррекции часов.
inline bool storeGraphPoint(const String &series, vector<GraphPoint> &points,
                            const GraphPoint &point, size_t limit){
  if(point.epoch == 0 || !isfinite(point.value)) return false;

  if(!points.empty() && point.epoch <= points.back().epoch){
    auto it = std::lower_bound(points.begin(), points.end(), point.epoch,
      [](const GraphPoint &existing, uint32_t epoch){ return existing.epoch < epoch; });
    if(it != points.end() && it->epoch == point.epoch){
      *it = point; // одинаковую секунду обновляем без дубля
      points.erase(it + 1, points.end()); // старый future-tail после перевода часов больше не должен возвращаться клиенту
    } else {
      points.erase(it, points.end()); // новая шкала времени начинается здесь; будущие метки от старых часов недостоверны
      points.push_back(point);
    }
    trimGraphPoints(points, limit);
    if(!saveGraphSeries(series, points)) seriesRewriteRequired[series] = true;
    return true; // точка уже принята в RAM; ошибку SPIFFS повторим без дублирования события
  }

  if(seriesRewriteRequired[series]){
    points.push_back(point); // после ошибки нельзя append к старому файлу с future-tail
    trimGraphPoints(points, limit);
    saveGraphSeries(series, points); // флаг снимется только после успешной атомарной замены
    return true;
  }

  points.push_back(point);
  size_t &pending = seriesUnsavedTail[series];
  pending++;
  unsigned long &lastFlush = seriesLastFlush[series];
  const unsigned long now = millis();

  if(limit > 0 && points.size() > limit + graphTrimSlackFor(limit)){
    trimGraphPoints(points, limit);
    if(!saveGraphSeries(series, points)) seriesRewriteRequired[series] = true;
    return true;
  }

  if(pending >= graphFlushPendingLimit || lastFlush == 0 || now - lastFlush >= graphFlushIntervalMs){
    if(appendGraphTailToFile(series, points, pending)){
      pending = 0;
      lastFlush = now;
    } else if(saveGraphSeries(series, points)){
      pending = 0; // Полная атомарная rewrite исправляет возможный частичный append.
    } else {
      seriesRewriteRequired[series] = true;
    }
  }
  return true;
}

// Добавляет точку в пользовательскую серию
inline bool addSeriesPoint(const String &series, uint32_t epoch, float value, uint16_t events = 0){ // добавление точки в указанную серию
  if(epoch == 0 || !isfinite(value)) return false; // недостоверные показания не портят масштаб графика
  GraphSettings cfg{defaultGraphUpdateInterval, defaultGraphPoints};
  getGraphSeriesSettings(series, cfg); // короткий config snapshot до history-lock
  cfg = normalizeGraphSettings(cfg);

  GraphDataGuard guard; // web может одновременно копировать эту серию в ответ

  unsigned long now = millis(); // текущее время
  unsigned long last = seriesLastUpdate[series]; // последнее обновление серии
  if(now - last < cfg.updateInterval) return false; // пропускаем обновление, если рано

  seriesLastUpdate[series] = now; // фиксируем время обновления

  auto &points = customGraphSeries[series]; // получаем массив точек серии
  const bool stored = storeGraphPoint(series, points, {epoch, value, events}, static_cast<size_t>(cfg.maxPoints)); // пакетное сохранение с лимитом
  if(stored) publishGraphHistorySnapshotLocked(series, points, static_cast<size_t>(cfg.maxPoints));
  return stored;
}
