//graph.cpp — работа графика
#pragma once // защита от повторного включения заголовочного файла

#include <Arduino.h> // базовые типы Arduino, millis(), String и т.п.
#include <ArduinoJson.h> // работа с JSON для сохранения/загрузки настроек
#include <functional> // std::function для callback'ов источников данных
#include <map> // ассоциативные контейнеры для хранения серий и настроек
#include <vector> // динамические массивы точек графика

#include "fs_utils.h" // утилиты работы с файловой системой (SPIFFS)

using std::vector; // упрощение записи vector<>

struct GraphPoint {
  String time; // Время точки графика.
  float value; // Значение измеряемого параметра.
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
  while(points.size() > limit) points.erase(points.begin()); // удаляем самые старые точки
}

inline const unsigned long minGraphUpdateInterval = 5000; // Минимальный интервал обновления графика: 5 секунд.
inline const unsigned long maxGraphUpdateInterval = 24UL * 60UL * 60UL * 1000UL; // Максимальный интервал обновления графика: 24 часа.
inline const unsigned long defaultGraphUpdateInterval = 60UL * 60UL * 1000UL; // Интервал графиков по умолчанию: 60 минут.
inline const unsigned long graphSaveIntervalMs = 15000; // SPIFFS сохраняем реже, чтобы flash-записи не рвали плавность RGB.
inline const int minGraphPoints = 1; // минимальное допустимое количество точек
inline const int maxGraphPoints = 100; // максимальное допустимое количество точек
inline const char* chlorinePpmGraphSeriesId(){ return "FloatСl"; } // ID видимого графика хлора ppmCl; буква С здесь кириллическая, как в UI.
inline const char* chlorineOrpGraphSeriesId(){ return "FloatClOrp"; } // ID скрытой серии ORP, которая не меняет формат точки графика.
inline bool isChlorinePpmGraphSeries(const String &series){ return series == chlorinePpmGraphSeriesId(); } // Проверяем, что запрошена видимая серия хлора.

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

inline void saveGraphSeries(const String &series, const vector<GraphPoint> &points){ // сохранение данных графика
  if(!spiffsMounted) return; // если SPIFFS недоступен — выходим
  String path = graphDataPath(series); // путь к файлу данных
  File f = SPIFFS.open(path, FILE_WRITE); // открываем файл на запись
  if(!f){ // если файл не открылся
    Serial.println("Failed to open graph file for writing: " + path); // вывод ошибки
    return; // выходим
  }
  for(const auto &p : points){ // перебираем все точки
    f.printf("%s,%.3f,%u\n", p.time.c_str(), p.value, static_cast<unsigned>(p.events)); // сохраняем время, значение и события в CSV-формате
  }
  f.close(); // закрываем файл
}

inline std::map<String, unsigned long> graphLastSave; // Последняя запись серии в SPIFFS.

inline void saveGraphSeriesThrottled(const String &series, const vector<GraphPoint> &points){ // Сохраняем серию без частых flash-записей.
  unsigned long now = millis(); // Текущее время для ограничения частоты записи.
  unsigned long last = graphLastSave[series]; // Последняя запись конкретной серии.
  if(last != 0 && now - last < graphSaveIntervalMs) return; // В RAM данные уже обновлены, SPIFFS подождет.
  graphLastSave[series] = now; // Фиксируем время до записи, чтобы не повторять при долгой операции.
  saveGraphSeries(series, points); // Фактическая запись на SPIFFS.
}

inline void loadGraphSeries(const String &series, vector<GraphPoint> &points){ // загрузка точек графика из файла
  points.clear(); // очищаем текущие данные
  if(!spiffsMounted) return; // если SPIFFS недоступен — выходим
  String path = graphDataPath(series); // путь к файлу данных
  if(!SPIFFS.exists(path)) return; // если файл отсутствует — выходим
  File f = SPIFFS.open(path, FILE_READ); // открываем файл на чтение
  if(!f){ // если файл не открылся
    Serial.println("Failed to open graph file for reading: " + path); // вывод ошибки
    return; // выходим
  }
  while(f.available()){ // читаем файл построчно
    String line = f.readStringUntil('\n'); // читаем одну строку
    line.trim(); // удаляем пробелы и переводы строк
    if(line.length() == 0) continue; // пропускаем пустые строки
    int sep = line.indexOf(','); // ищем разделитель CSV
    if(sep < 0) continue; // если формат неверный — пропускаем
    GraphPoint gp; // создаём точку графика
    gp.time = line.substring(0, sep); // извлекаем временную метку
    int sep2 = line.indexOf(',', sep + 1); // второй разделитель появился в новых файлах с событиями.
    if(sep2 < 0){ // Старый формат: время,значение.
      gp.value = line.substring(sep+1).toFloat(); // извлекаем значение
      gp.events = 0; // В старых точках событий не было.
    } else { // Новый формат: время,значение,события.
      gp.value = line.substring(sep+1, sep2).toFloat(); // извлекаем значение
      gp.events = static_cast<uint16_t>(line.substring(sep2+1).toInt()); // извлекаем количество событий
    }
    points.push_back(gp); // добавляем точку в массив
  }
  f.close(); // закрываем файл
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

// Загружает основной график и его настройки
inline void loadGraph(){ // инициализация основной серии графика
  GraphSettings mainCfg{defaultGraphUpdateInterval, maxGraphPoints}; // По умолчанию график обновляется раз в 60 минут и хранит 100 точек.
  loadGraphSettings("main", mainCfg); // попытка загрузить сохранённые настройки из SPIFFS

  if(mainCfg.updateInterval < minGraphUpdateInterval) mainCfg.updateInterval = minGraphUpdateInterval; // защита от слишком частых обновлений
  if(mainCfg.updateInterval > maxGraphUpdateInterval) mainCfg.updateInterval = maxGraphUpdateInterval; // защита от слишком редких обновлений
  if(mainCfg.maxPoints < minGraphPoints) mainCfg.maxPoints = minGraphPoints; // защита от нулевого/отрицательного размера
  if(mainCfg.maxPoints > maxGraphPoints) mainCfg.maxPoints = maxGraphPoints; // ограничение максимального размера графика

  updateInterval = mainCfg.updateInterval; // сохраняем интервал обновления как глобальный
  maxPoints = mainCfg.maxPoints; // сохраняем лимит точек как глобальный

  loadGraphSeries("main", graphPoints); // загружаем сохранённые точки основной серии

  seriesConfig["main"] = mainCfg; // сохраняем конфигурацию основной серии
  seriesLastUpdate["main"] = 0; // сбрасываем время последнего обновления
}

// Добавляет точку в основной график с учетом интервала
inline void addGraphPoint(String t, float v){ // добавление новой точки в основную серию
  GraphSettings cfg = seriesConfig.count("main") ? seriesConfig["main"] : GraphSettings{updateInterval, maxPoints}; // берём настройки серии или используем глобальные
  if(cfg.updateInterval < minGraphUpdateInterval) cfg.updateInterval = minGraphUpdateInterval; // Подстраховка от старых настроек 100 мс/1 сек.
  if(cfg.updateInterval > maxGraphUpdateInterval) cfg.updateInterval = maxGraphUpdateInterval; // Ограничиваем редкие интервалы 24 часами.

  unsigned long now = millis(); // текущее время работы устройства
  unsigned long last = seriesLastUpdate["main"]; // время последнего добавления точки
  if(now - last < cfg.updateInterval) return; // выходим, если интервал ещё не прошёл

  seriesLastUpdate["main"] = now; // обновляем время последнего добавления

  graphPoints.push_back({t, v, 0}); // добавляем новую точку (время + значение без событий)
  trimGraphPoints(graphPoints, maxPoints); // обрезаем массив по глобальному лимиту
  trimGraphPoints(graphPoints, cfg.maxPoints); // дополнительно учитываем лимит серии
  saveGraphSeriesThrottled("main", graphPoints); // сохраняем обновлённую серию в SPIFFS без частых блокировок
}

// Регистрирует источник данных для графика
inline void registerGraphSource( // регистрация новой серии графика
  const String &name, // имя серии
  const std::function<float()> &getter, // функция получения текущего значения
  const String &fallback="", // резервная серия для загрузки данных
  unsigned long defaultInterval=updateInterval, // интервал обновления по умолчанию
  int defaultMaxPoints=maxPoints // количество точек по умолчанию
){
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
    trimGraphPoints(stored, cfg.maxPoints); // приводим количество точек к допустимому
    customGraphSeries[name] = stored; // сохраняем точки пользовательской серии
  }
}

// Добавляет точку в пользовательскую серию
inline bool addSeriesPoint(const String &series, const String &t, float value, uint16_t events = 0){ // добавление точки в указанную серию
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
  points.push_back({t, value, events}); // добавляем новую точку вместе со счетчиком событий
  trimGraphPoints(points, cfg.maxPoints); // обрезаем массив по лимиту
  saveGraphSeriesThrottled(series, points); // сохраняем серию в файл без частых блокировок
  return true; // сообщаем вызывающему коду, что точка реально добавлена
}
