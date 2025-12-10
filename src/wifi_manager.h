#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <type_traits>

struct WifiStatusInfo {
  String statusText;
  String modeText;
  String ssid;
  String ip;
  int rssi;
  bool apActive;
};

// Persistent storage helpers and Wi-Fi defaults
inline Preferences prefs;
inline const char *defaultSSID = "OAB-GeelyM";
inline const char *defaultPASS = "83913381";
inline const char *apSSID = "ESP32";
inline const char *apPASS = "12345678";
inline const char *defaultHostname = "ESP32";

inline void saveButtonState(const char *key, int val) {
  prefs.begin("buttons", false);
  prefs.putInt(key, val);
  prefs.end();
}

inline int loadButtonState(const char *key, int def = 0) {
  prefs.begin("buttons", false);
  int val = prefs.getInt(key, def);
  prefs.end();
  return val;
}

template <typename T> void saveValue(const char *key, T val);

template <typename T> T loadValue(const char *key, T def) {
  prefs.begin("MINIDASH", true);
  T val;
  if (prefs.isKey(key)) {
    if constexpr (std::is_same<T, float>::value)
      val = prefs.getFloat(key, def);
    else if constexpr (std::is_same<T, int>::value)
      val = prefs.getInt(key, def);
    else if constexpr (std::is_same<T, String>::value)
      val = prefs.getString(key, def);
    else
      val = def;
    prefs.end();
    return val;
  }
  prefs.end();
  val = def;
  saveValue<T>(key, val);
  return val;
}

template <typename T> void saveValue(const char *key, T val) {
  prefs.begin("MINIDASH", false);
  if constexpr (std::is_same<T, float>::value)
    prefs.putFloat(key, val);
  else if constexpr (std::is_same<T, int>::value)
    prefs.putInt(key, val);
  else if constexpr (std::is_same<T, String>::value)
    prefs.putString(key, val.c_str());
  prefs.end();
}

namespace wifi_internal {
inline String staSsid;
inline String staPass;
inline String apSsid;
inline String apPass;
inline String hostName;
inline bool staAttemptInProgress = false;
inline bool fallbackApActive = false;
inline bool mdnsStarted = false;
inline int attemptCount = 0;
inline const int maxAttempts = 5;
inline unsigned long lastAttemptStarted = 0;
inline unsigned long lastStatusCheck = 0;
inline unsigned long checkInterval = 1000;      // 1s until stable connection
inline const unsigned long attemptInterval = 5000; // retry window while connecting

inline void beginMdns() {
  if (mdnsStarted)
    return;
  if (MDNS.begin(hostName.c_str())) {
    MDNS.addService("http", "tcp", 80);
    mdnsStarted = true;
  }
}

inline void startHiddenApForSta() {
  WiFi.softAP(apSsid.c_str(), apPass.c_str(), 1, true);
}

inline void startStaAttempt() {
  WiFi.mode(WIFI_MODE_APSTA);
  WiFi.setHostname(hostName.c_str());
  startHiddenApForSta();
  WiFi.begin(staSsid.c_str(), staPass.c_str());
  staAttemptInProgress = true;
  lastAttemptStarted = millis();
  attemptCount++;
}

inline void activateFallbackAp() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid.c_str(), apPass.c_str());
  fallbackApActive = true;
  staAttemptInProgress = false;
  attemptCount = 0;
  checkInterval = 1000;
}

inline void onConnected() {
  fallbackApActive = false;
  staAttemptInProgress = false;
  checkInterval = 120000; // 120 seconds
  attemptCount = 0;
  WiFi.mode(WIFI_STA);
  beginMdns();
}

inline String buildModeText() {
  wifi_mode_t mode = WiFi.getMode();
  switch (mode) {
  case WIFI_MODE_STA:
    return "STA";
  case WIFI_MODE_AP:
    return "AP";
  case WIFI_MODE_APSTA:
    return "STA+AP";
  default:
    return "Unknown";
  }
}

inline String buildStatusText() {
  if (WiFi.status() == WL_CONNECTED)
    return "Connected";
  if (staAttemptInProgress && attemptCount <= maxAttempts)
    return "Connecting";
  if (fallbackApActive)
    return "AP Only";
  return "Disconnected";
}

inline void ensureConnection() {
  const unsigned long now = millis();
  if (now - lastStatusCheck < checkInterval)
    return;
  lastStatusCheck = now;

  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    onConnected();
    return;
  }

  if (checkInterval != 1000)
    checkInterval = 1000;

  if (fallbackApActive) {
    return;
  }

  if (staAttemptInProgress && (now - lastAttemptStarted) < attemptInterval) {
    return; // still waiting for current attempt
  }

  if (attemptCount < maxAttempts) {
    startStaAttempt();
  } else {
    activateFallbackAp();
  }
}
} // namespace wifi_internal

inline void initWiFiModule() {
  using namespace wifi_internal;
  staSsid = loadValue<String>("ssid", String(defaultSSID));
  staPass = loadValue<String>("pass", String(defaultPASS));
  apSsid = loadValue<String>("apSSID", String(::apSSID));
  apPass = loadValue<String>("apPASS", String(::apPASS));
  hostName = loadValue<String>("hostname", String(defaultHostname));

  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostName.c_str());
  startStaAttempt();
}

inline void wifiModuleLoop() { wifi_internal::ensureConnection(); }

inline WifiStatusInfo getWifiStatus() {
  WifiStatusInfo info{};
  info.modeText = wifi_internal::buildModeText();
  info.statusText = wifi_internal::buildStatusText();
  info.ssid = WiFi.SSID();
  info.ip = WiFi.localIP().toString();
  info.rssi = WiFi.isConnected() ? WiFi.RSSI() : 0;
  info.apActive = WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA;
  return info;
}

inline String wifiHostName() { return wifi_internal::hostName; }

inline String scanWifiNetworksJson() {
  wifi_mode_t prevMode = WiFi.getMode();
  bool apOnly = (prevMode == WIFI_MODE_AP);
  wifi_mode_t targetMode = prevMode;
  if (apOnly)
    targetMode = WIFI_MODE_APSTA;
  else if (prevMode == WIFI_MODE_NULL)
    targetMode = WIFI_MODE_STA;

  if (targetMode != prevMode) {
    WiFi.mode(targetMode);
    delay(50);
  }

  WiFi.scanDelete();
  int16_t n = WiFi.scanNetworks(false, true);
  if (n < 0)
    n = 0;

  auto escape = [](String v) {
    v.replace("\\", "\\\\");
    v.replace("\"", "\\\"");
    return v;
  };

  String json = "[";
  for (int i = 0; i < n; i++) {
    String auth = "open";
    wifi_auth_mode_t enc = WiFi.encryptionType(i);
    switch (enc) {
    case WIFI_AUTH_WEP:
      auth = "WEP";
      break;
    case WIFI_AUTH_WPA_PSK:
      auth = "WPA";
      break;
    case WIFI_AUTH_WPA2_PSK:
      auth = "WPA2";
      break;
    case WIFI_AUTH_WPA_WPA2_PSK:
      auth = "WPA/WPA2";
      break;
    case WIFI_AUTH_WPA3_PSK:
      auth = "WPA3";
      break;
    case WIFI_AUTH_WPA2_WPA3_PSK:
      auth = "WPA2/WPA3";
      break;
    default:
      auth = "open";
      break;
    }
    json += "{\"ssid\":\"" + escape(WiFi.SSID(i)) + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
    json += "\"auth\":\"" + auth + "\"}";
    if (i < n - 1)
      json += ",";
  }
  json += "]";
  WiFi.scanDelete();

  if (targetMode != prevMode) {
    WiFi.mode(prevMode);
  }
  return json;
}

inline bool saveWifiConfig(const String &ssid, const String &pass, const String &apSsidIn, const String &apPassIn) {
  using namespace wifi_internal;
  staSsid = ssid.length() ? ssid : String(defaultSSID);
  staPass = pass;
  apSsid = apSsidIn.length() ? apSsidIn : String(::apSSID);
  apPass = apPassIn;

  saveValue<String>("ssid", staSsid);
  saveValue<String>("pass", staPass);
  saveValue<String>("apSSID", apSsid);
  saveValue<String>("apPASS", apPass);

  WiFi.disconnect(true);
  fallbackApActive = false;
  staAttemptInProgress = false;
  attemptCount = 0;
  checkInterval = 1000;
  startStaAttempt();
  return true;
}

inline bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }
inline String currentApSsid() { return wifi_internal::apSsid; }
inline String currentApPass() { return wifi_internal::apPass; }
