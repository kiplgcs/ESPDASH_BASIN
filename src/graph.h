//graph.cpp — работа графика
#pragma once // защита от повторного включения заголовочного файла

#include <Arduino.h> // базовые типы Arduino, millis(), String и т.п.
#include <ArduinoJson.h> // работа с JSON для сохранения/загрузки настроек
#include <algorithm> // lower_bound для сохранения хронологического порядка точек
#include <functional> // std::function для callback'ов источников данных
#include <map> // ассоциативные контейнеры для хранения серий и настроек
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
inline const unsigned long graphFlushIntervalMs = 30000;
inline const size_t graphFlushPendingLimit = 32;
inline std::map<String, size_t> seriesUnsavedTail;
inline std::map<String, unsigned long> seriesLastFlush;
inline std::map<String, bool> seriesRewriteRequired; // Повтор атомарной rewrite только после реальной ошибки файловой системы.

inline size_t graphTrimSlackFor(size_t limit){
  const size_t half = limit / 2;
  return half > graphTrimSlackMin ? half : graphTrimSlackMin;
}

// Web и основная логика работают на разных ядрах, поэтому снимок истории и
// добавление точки не должны одновременно менять один и тот же vector/map.
inline SemaphoreHandle_t graphDataMutex = nullptr;

inline void ensureGraphDataMutex(){
  if(graphDataMutex == nullptr) graphDataMutex = xSemaphoreCreateMutex();
}

class GraphDataGuard {
 public:
  GraphDataGuard(){
    ensureGraphDataMutex();
    locked = graphDataMutex != nullptr && xSemaphoreTake(graphDataMutex, portMAX_DELAY) == pdTRUE;
  }
  ~GraphDataGuard(){ if(locked) xSemaphoreGive(graphDataMutex); }
  GraphDataGuard(const GraphDataGuard&) = delete;
  GraphDataGuard& operator=(const GraphDataGuard&) = delete;

 private:
  bool locked = false;
};

inline String graphDataPath(const String &series){ // формирует путь к файлу данных графика
  return "/graph_" + sanitizeSeriesId(series) + ".dat"; // имя файла на основе серии
}

struct GraphSettings { unsigned long updateInterval; int maxPoints; }; // настройки графика: интервал и размер

inline String graphConfigPath(const String &series){ // путь к JSON-файлу настроек серии
  return "/graph_config_" + sanitizeSeriesId(series) + ".json"; // имя файла конфигурации
}

inline bool loadGraphSettings(const String &series, GraphSettings &settings){ // загрузка настроек серии из SPIFFS
  if(!spiffsMounted) return false; // если файловая система не смонтирована — выходим
  String path = graphConfigPath(series); // получаем путь к конфигурации
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

inline void saveGraphSettings(const String &series, const GraphSettings &settings){ // сохранение настроек графика
  if(!spiffsMounted) return; // если SPIFFS недоступен — выходим
  String path = graphConfigPath(series); // путь к файлу настроек
  if(SPIFFS.exists(path)){ // если файл уже существует
    SPIFFS.remove(path); // удаляем старую версию
  }
  File f = SPIFFS.open(path, FILE_WRITE); // открываем файл на запись
  if(!f){ // если файл не открылся
    Serial.println("Failed to open graph config for writing: " + path); // сообщение об ошибке
    return; // выходим
  }
  StaticJsonDocument<256> doc; // создаём JSON-документ
  doc["updateInterval"] = settings.updateInterval; // сохраняем интервал обновления
  doc["maxPoints"] = settings.maxPoints; // сохраняем максимальное количество точек
  serializeJson(doc, f); // записываем JSON в файл
  f.close(); // закрываем файл
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
  for(const auto &p : points){ // перебираем все точки
    if(f.printf("%lu,%.3f,%u\n", static_cast<unsigned long>(p.epoch), p.value, static_cast<unsigned>(p.events)) == 0){
      writeOk = false;
      break;
    }
  }
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

inline void loadGraphSeries(const String &series, vector<GraphPoint> &points){ // загрузка точек графика из файла
  points.clear(); // очищаем текущие данные
  if(!spiffsMounted) return; // если SPIFFS недоступен — выходим
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
  }
  f.close(); // закрываем файл
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

// Источники данных (функции получения значений)
inline std::map<String, std::function<float()>> graphValueProviders; // функции, возвращающие текущее значение для каждой серии

// Конфигурации серий
inline std::map<String, GraphSettings> seriesConfig; // настройки (интервал, точки) для каждой серии графика

// Время последнего обновления каждой серии
inline std::map<String, unsigned long> seriesLastUpdate; // таймстемп последнего добавления точки в серию

inline bool storeGraphPoint(const String &series, vector<GraphPoint> &points,
                            const GraphPoint &point, size_t limit); // реализация расположена после регистрации источников

// Загружает основной график и его настройки
inline void loadGraph(){ // инициализация основной серии графика
  GraphDataGuard guard; // защищаем общие контейнеры от сетевой задачи
  GraphSettings mainCfg{defaultGraphUpdateInterval, defaultGraphPoints}; // Служебная серия main по умолчанию не занимает память под все 5000 точек.
  loadGraphSettings("main", mainCfg); // попытка загрузить сохранённые настройки из SPIFFS

  if(mainCfg.updateInterval < minGraphUpdateInterval) mainCfg.updateInterval = minGraphUpdateInterval; // защита от слишком частых обновлений
  if(mainCfg.updateInterval > maxGraphUpdateInterval) mainCfg.updateInterval = maxGraphUpdateInterval; // защита от слишком редких обновлений
  if(mainCfg.maxPoints < minGraphPoints) mainCfg.maxPoints = minGraphPoints; // защита от нулевого/отрицательного размера
  if(mainCfg.maxPoints > maxGraphPoints) mainCfg.maxPoints = maxGraphPoints; // ограничение максимального размера графика

  updateInterval = mainCfg.updateInterval; // сохраняем интервал обновления как глобальный
  maxPoints = mainCfg.maxPoints; // сохраняем лимит точек как глобальный

  loadGraphSeries("main", graphPoints); // загружаем сохранённые точки основной серии
  if(graphPoints.size() > static_cast<size_t>(mainCfg.maxPoints)){
    trimGraphPoints(graphPoints, static_cast<size_t>(mainCfg.maxPoints));
    saveGraphSeries("main", graphPoints);
  }

  seriesConfig["main"] = mainCfg; // сохраняем конфигурацию основной серии
  seriesLastUpdate["main"] = 0; // сбрасываем время последнего обновления
}

// Добавляет точку в основной график с учетом интервала
inline void addGraphPoint(uint32_t epoch, float v){ // добавление новой точки в основную серию
  if(epoch == 0 || !isfinite(v)) return; // без достоверного времени/значения точка бессмысленна
  GraphDataGuard guard; // web может одновременно запрашивать снимок истории
  GraphSettings cfg = seriesConfig.count("main") ? seriesConfig["main"] : GraphSettings{updateInterval, maxPoints}; // берём настройки серии или используем глобальные
  if(cfg.updateInterval < minGraphUpdateInterval) cfg.updateInterval = minGraphUpdateInterval; // Подстраховка от старых настроек 100 мс/1 сек.
  if(cfg.updateInterval > maxGraphUpdateInterval) cfg.updateInterval = maxGraphUpdateInterval; // Ограничиваем редкие интервалы 24 часами.

  unsigned long now = millis(); // текущее время работы устройства
  unsigned long last = seriesLastUpdate["main"]; // время последнего добавления точки
  if(now - last < cfg.updateInterval) return; // выходим, если интервал ещё не прошёл

  seriesLastUpdate["main"] = now; // обновляем время последнего добавления

  const size_t limit = static_cast<size_t>(cfg.maxPoints < maxPoints ? cfg.maxPoints : maxPoints); // строгий из двух лимитов
  storeGraphPoint("main", graphPoints, {epoch, v, 0}, limit); // сохраняем точку пакетно
}

// Регистрирует источник данных для графика
inline void registerGraphSource( // регистрация новой серии графика
  const String &name, // имя серии
  const std::function<float()> &getter, // функция получения текущего значения
  const String &fallback="", // резервная серия для загрузки данных
  unsigned long defaultInterval=updateInterval, // интервал обновления по умолчанию
  int defaultMaxPoints=maxPoints // количество точек по умолчанию
){
  GraphDataGuard guard; // регистрация затрагивает общие карты и массивы
  graphValueProviders[name] = getter; // сохраняем функцию-источник значений

  vector<GraphPoint> stored; // временное хранилище загруженных точек
  loadGraphSeries(name, stored); // загружаем сохранённые точки серии
  if(stored.empty() && fallback.length()) loadGraphSeries(fallback, stored); // если пусто — пробуем fallback

  unsigned long safeDefaultInterval = defaultInterval < minGraphUpdateInterval
                                     ? minGraphUpdateInterval
                                     : defaultInterval; // защита от слишком малого интервала
  if(safeDefaultInterval > maxGraphUpdateInterval) safeDefaultInterval = maxGraphUpdateInterval; // Защита от интервалов больше 24 часов.

  GraphSettings cfg{safeDefaultInterval, defaultMaxPoints}; // создаём конфигурацию серии
  bool loaded = loadGraphSettings(name, cfg); // пробуем загрузить сохранённые настройки
  if(!loaded && fallback.length()) loadGraphSettings(fallback, cfg); // fallback-настройки при необходимости

  if(cfg.updateInterval < minGraphUpdateInterval) cfg.updateInterval = minGraphUpdateInterval; // нормализация интервала
  if(cfg.updateInterval > maxGraphUpdateInterval) cfg.updateInterval = maxGraphUpdateInterval; // нормализация верхнего предела
  if(cfg.maxPoints < minGraphPoints) cfg.maxPoints = minGraphPoints; // минимальное количество точек
  if(cfg.maxPoints > maxGraphPoints) cfg.maxPoints = maxGraphPoints; // максимальное количество точек

  seriesConfig[name] = cfg; // сохраняем настройки серии
  seriesLastUpdate[name] = 0; // инициализируем таймер обновления

  if(!stored.empty()){ // если данные серии были загружены
    const bool oversized = stored.size() > static_cast<size_t>(cfg.maxPoints);
    trimGraphPoints(stored, cfg.maxPoints); // приводим количество точек к допустимому
    customGraphSeries[name] = stored; // сохраняем точки пользовательской серии
    if(oversized) saveGraphSeries(name, customGraphSeries[name]); // файл также приводим к выбранному лимиту
  }
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
  GraphDataGuard guard; // web может одновременно копировать эту серию в ответ
  GraphSettings cfg = seriesConfig.count(series)
                        ? seriesConfig[series]
                        : GraphSettings{updateInterval, maxPoints}; // получаем настройки серии или используем дефолт

  if(cfg.updateInterval < minGraphUpdateInterval) cfg.updateInterval = minGraphUpdateInterval; // защита интервала
  if(cfg.updateInterval > maxGraphUpdateInterval) cfg.updateInterval = maxGraphUpdateInterval; // защита верхнего предела интервала
  if(cfg.maxPoints < minGraphPoints) cfg.maxPoints = minGraphPoints; // защита размера
  if(cfg.maxPoints > maxGraphPoints) cfg.maxPoints = maxGraphPoints; // ограничение размера

  unsigned long now = millis(); // текущее время
  unsigned long last = seriesLastUpdate[series]; // последнее обновление серии
  if(now - last < cfg.updateInterval) return false; // пропускаем обновление, если рано

  seriesLastUpdate[series] = now; // фиксируем время обновления

  auto &points = customGraphSeries[series]; // получаем массив точек серии
  return storeGraphPoint(series, points, {epoch, value, events}, static_cast<size_t>(cfg.maxPoints)); // пакетное сохранение с лимитом
}
