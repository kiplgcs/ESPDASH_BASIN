//wifi_utils.h — подключение
#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <type_traits>

inline Preferences prefs;
inline const char* defaultSSID = "OAB-GeelyM";
inline const char* defaultPASS = "83913381";
inline const char* apSSID = "ESP32_AP";
inline const char* apPASS = "12345678";

template<typename T>
void saveValue(const char* key, T val);

template<typename T>
T loadValue(const char* key, T def){
  prefs.begin("MINIDASH", true);
  T val;
  if(prefs.isKey(key)){
    if constexpr (std::is_same<T, float>::value) val = prefs.getFloat(key, def);
    else if constexpr (std::is_same<T, int>::value) val = prefs.getInt(key, def);
    else if constexpr (std::is_same<T, String>::value) val = prefs.getString(key, def);
    else val = def;
    prefs.end();
    return val;
  }
  prefs.end();
  val = def;
  saveValue<T>(key, val);
  return val;
}

template<typename T>
void saveValue(const char* key, T val){
  prefs.begin("MINIDASH", false);
  if constexpr (std::is_same<T, float>::value) prefs.putFloat(key, val);
  else if constexpr (std::is_same<T, int>::value) prefs.putInt(key, val);
  else if constexpr (std::is_same<T, String>::value) prefs.putString(key, val.c_str());
  prefs.end();
}

inline void saveButtonState(const char* key, int val){
  prefs.begin("buttons", false);
  prefs.putInt(key, val);
  prefs.end();
}

inline int loadButtonState(const char* key, int def = 0){
  prefs.begin("buttons", false);
  int val = prefs.getInt(key, def);
  prefs.end();
  return val;
}

inline void setupWiFi(String &StoredAPSSID, String &StoredAPPASS, int &button1, int &button2){
 // Приоритетно работаем в STA, точку доступа включаем только если подключение не удалось.
  WiFi.mode(WIFI_STA);
  prefs.begin("wifi", false);
  auto readStringWithDefault = [&](const char* key, const char* def)->String {
    if(prefs.isKey(key)){
      return prefs.getString(key);
    }
    String val = def;
    prefs.putString(key, val);
    return val;
  };

  String ssid = readStringWithDefault("ssid", defaultSSID);
  String pass = readStringWithDefault("pass", defaultPASS);
  StoredAPSSID = readStringWithDefault("apSSID", apSSID);
  StoredAPPASS = readStringWithDefault("apPASS", apPASS);
  prefs.end();

  button1 = loadButtonState("button1", 0);
  button2 = loadButtonState("button2", 0);

  WiFi.begin(ssid.c_str(), pass.c_str());

  int attempts = 0;
  while(WiFi.status() != WL_CONNECTED && attempts < 20){
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if(WiFi.status() == WL_CONNECTED){
    // Убедимся, что режим AP выключен после успешного подключения к роутеру.
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    // Не удалось подключиться в STA — поднимем точку доступа для конфигурации.
    WiFi.mode(WIFI_AP);
    WiFi.softAP(StoredAPSSID.c_str(), StoredAPPASS.c_str());
    Serial.println("AP mode started");
  }
}
