//graph.cpp — работа графика
#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <functional>
#include <map>
#include <vector>

#include "fs_utils.h"

using std::vector;

struct GraphPoint { String time; float value; };

inline String sanitizeSeriesId(const String &series){
  String safe = series;
  safe.replace("/", "_");
  safe.replace("\\", "_");
  safe.replace("..", "_");
  if(!safe.length()) safe = "main";
  if(safe.length() > 16) safe = safe.substring(0, 16);
  return safe;
}

inline void trimGraphPoints(vector<GraphPoint> &points, size_t limit){
  if(limit == 0) return;
  while(points.size() > limit) points.erase(points.begin());
}

inline const unsigned long minGraphUpdateInterval = 100;
inline const int minGraphPoints = 1;
inline const int maxGraphPoints = 50;

inline String graphDataPath(const String &series){
  return "/graph_" + sanitizeSeriesId(series) + ".dat";
}

struct GraphSettings { unsigned long updateInterval; int maxPoints; };

inline String graphConfigPath(const String &series){
  return "/graph_config_" + sanitizeSeriesId(series) + ".json";
}

inline bool loadGraphSettings(const String &series, GraphSettings &settings){
  if(!spiffsMounted) return false;
  String path = graphConfigPath(series);
  if(!SPIFFS.exists(path)) return false;
  File f = SPIFFS.open(path, FILE_READ);
  if(!f) return false;
  String payload = f.readString();
  f.close();
  if(payload.length() == 0) return false;
  StaticJsonDocument<256> doc;
  if(deserializeJson(doc, payload) != DeserializationError::Ok) return false;
  settings.updateInterval = doc["updateInterval"] | settings.updateInterval;
  settings.maxPoints = doc["maxPoints"] | settings.maxPoints;
  return true;
}

inline void saveGraphSettings(const String &series, const GraphSettings &settings){
  if(!spiffsMounted) return;
  String path = graphConfigPath(series);
          if(SPIFFS.exists(path)){
          SPIFFS.remove(path);
        }
  File f = SPIFFS.open(path, FILE_WRITE);
  if(!f){
    Serial.println("Failed to open graph config for writing: " + path);
    return;
  }
  StaticJsonDocument<256> doc;
  doc["updateInterval"] = settings.updateInterval;
  doc["maxPoints"] = settings.maxPoints;
  serializeJson(doc, f);
  f.close();
}

inline void saveGraphSeries(const String &series, const vector<GraphPoint> &points){
  if(!spiffsMounted) return;
  String path = graphDataPath(series);
  File f = SPIFFS.open(path, FILE_WRITE);
  if(!f){
    Serial.println("Failed to open graph file for writing: " + path);
    return;
  }
  for(const auto &p : points){
    f.printf("%s,%.3f\n", p.time.c_str(), p.value);
  }
  f.close();
}

inline void loadGraphSeries(const String &series, vector<GraphPoint> &points){
  points.clear();
  if(!spiffsMounted) return;
  String path = graphDataPath(series);
  if(!SPIFFS.exists(path)) return;
  File f = SPIFFS.open(path, FILE_READ);
  if(!f){
    Serial.println("Failed to open graph file for reading: " + path);
    return;
  }
  while(f.available()){
    String line = f.readStringUntil('\n');
    line.trim();
    if(line.length() == 0) continue;
    int sep = line.indexOf(',');
    if(sep < 0) continue;
    GraphPoint gp;
    gp.time = line.substring(0, sep);
    gp.value = line.substring(sep+1).toFloat();
    points.push_back(gp);
  }
  f.close();
}

inline vector<GraphPoint> graphPoints;
inline int maxPoints;
inline unsigned long updateInterval;
// inline unsigned long lastUpdate = 0;
inline std::map<String, vector<GraphPoint>> customGraphSeries;
inline std::map<String, std::function<float()>> graphValueProviders;
inline std::map<String, GraphSettings> seriesConfig;
inline std::map<String, unsigned long> seriesLastUpdate;

inline void loadGraph(){
  GraphSettings mainCfg{static_cast<unsigned long>(1000), 30};
  loadGraphSettings("main", mainCfg);
  if(mainCfg.updateInterval < minGraphUpdateInterval) mainCfg.updateInterval = minGraphUpdateInterval;
  if(mainCfg.maxPoints < minGraphPoints) mainCfg.maxPoints = minGraphPoints;
  if(mainCfg.maxPoints > maxGraphPoints) mainCfg.maxPoints = maxGraphPoints;
  updateInterval = mainCfg.updateInterval;
  maxPoints = mainCfg.maxPoints;
  loadGraphSeries("main", graphPoints);
  // trimGraphPoints(graphPoints, maxPoints);
  seriesConfig["main"] = mainCfg;
  seriesLastUpdate["main"] = 0;
}

inline void addGraphPoint(String t, float v){
  GraphSettings cfg = seriesConfig.count("main") ? seriesConfig["main"] : GraphSettings{updateInterval, maxPoints};
  unsigned long now = millis();
  unsigned long last = seriesLastUpdate["main"];
  if(now - last < cfg.updateInterval) return;
  seriesLastUpdate["main"] = now;
  int seriesMax = cfg.maxPoints;
  if(seriesMax < minGraphPoints) seriesMax = minGraphPoints;
  if(seriesMax > maxGraphPoints) seriesMax = maxGraphPoints;
  graphPoints.push_back({t,v});
  trimGraphPoints(graphPoints, maxPoints);
  trimGraphPoints(graphPoints, seriesMax);
  saveGraphSeries("main", graphPoints);
}

// inline void registerGraphSource(const String &name, const std::function<float()> &getter){
inline void registerGraphSource(const String &name, const std::function<float()> &getter, const String &fallback="", unsigned long defaultInterval=updateInterval, int defaultMaxPoints=maxPoints){
  graphValueProviders[name] = getter;
  vector<GraphPoint> stored;
  loadGraphSeries(name, stored);
  if(stored.empty() && fallback.length()) loadGraphSeries(fallback, stored);
  unsigned long safeDefaultInterval = defaultInterval < minGraphUpdateInterval ? minGraphUpdateInterval : defaultInterval;
  GraphSettings cfg{safeDefaultInterval, defaultMaxPoints};
  bool loaded = loadGraphSettings(name, cfg);
  if(!loaded && fallback.length()) loadGraphSettings(fallback, cfg);
  if(cfg.updateInterval < minGraphUpdateInterval) cfg.updateInterval = minGraphUpdateInterval;
  if(cfg.maxPoints < minGraphPoints) cfg.maxPoints = minGraphPoints;
  if(cfg.maxPoints > maxGraphPoints) cfg.maxPoints = maxGraphPoints;
  seriesConfig[name] = cfg;
  seriesLastUpdate[name] = 0;
  if(!stored.empty()){
    // GraphSettings cfg{updateInterval, maxPoints};
    // loadGraphSettings(name, cfg);
    // int seriesMax = cfg.maxPoints;
    // if(seriesMax < minGraphPoints) seriesMax = minGraphPoints;
    // if(seriesMax > maxGraphPoints) seriesMax = maxGraphPoints;
    // trimGraphPoints(stored, seriesMax);
    trimGraphPoints(stored, cfg.maxPoints);
    customGraphSeries[name] = stored;
  }
}

inline void addSeriesPoint(const String &series, const String &t, float value){
  GraphSettings cfg = seriesConfig.count(series) ? seriesConfig[series] : GraphSettings{updateInterval, maxPoints};
  if(cfg.updateInterval < minGraphUpdateInterval) cfg.updateInterval = minGraphUpdateInterval;
  if(cfg.maxPoints < minGraphPoints) cfg.maxPoints = minGraphPoints;
  if(cfg.maxPoints > maxGraphPoints) cfg.maxPoints = maxGraphPoints;
  unsigned long now = millis();
  unsigned long last = seriesLastUpdate[series];
  if(now - last < cfg.updateInterval) return;
  seriesLastUpdate[series] = now;

  auto &points = customGraphSeries[series];
  points.push_back({t, value});
  // trimGraphPoints(points, maxGraphPoints);
  trimGraphPoints(points, cfg.maxPoints);
  saveGraphSeries(series, points);
}