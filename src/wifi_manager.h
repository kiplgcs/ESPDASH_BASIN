#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <DNSServer.h>
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
inline SemaphoreHandle_t prefsMutex = nullptr;
inline DNSServer captiveDnsServer;
inline bool captiveDnsActive = false;
inline const byte captiveDnsPort = 53;

inline void ensurePrefsMutex() { if (prefsMutex == nullptr) prefsMutex = xSemaphoreCreateMutex(); }
inline void lockPrefs() { ensurePrefsMutex(); if (prefsMutex) xSemaphoreTake(prefsMutex, portMAX_DELAY); }
inline void unlockPrefs() { if (prefsMutex) xSemaphoreGive(prefsMutex); }
inline const char *defaultSSID = "OAB_2.4G_RPT";
inline const char *defaultPASS = "OAB-245901:oab--245901";
inline const char *defaultSSIDr = "OAB_2.4G";
inline const char *defaultPASSr = "OAB-245901:oab--245901";
inline const char *apSSID = "ESP32";
inline const char *apPASS = "12345678";
inline const char *defaultHostname = "ESP32";

inline void saveButtonState(const char *key, int val) {
  lockPrefs();
  prefs.begin("buttons", false);
  prefs.putInt(key, val);
  prefs.end();
    unlockPrefs();
}

inline int loadButtonState(const char *key, int def = 0) {
    lockPrefs();
  prefs.begin("buttons", false);
  int val = prefs.getInt(key, def);
  prefs.end();
    unlockPrefs();
  return val;
}

template <typename T> void saveValue(const char *key, T val);
// Универсальная функция чтения значения из NVS с автоматической инициализацией
template <typename T> T loadValue(const char *key, T def) {
    lockPrefs();
  prefs.begin("MINIDASH", true);
  T val;
  if (prefs.isKey(key)) {
    if constexpr (std::is_same<T, float>::value)
    val = prefs.getFloat(key, def);   // Сохраняем тип float
    else if constexpr (std::is_same<T, int>::value)
      val = prefs.getInt(key, def);     // Сохраняем тип int
    else if constexpr (std::is_same<T, String>::value)
      val = prefs.getString(key, def);
    else
      val = def;                        // Любой другой тип — возвращаем дефолт
    prefs.end();
        unlockPrefs();
    return val;
  }
  prefs.end();
    unlockPrefs();
  val = def;
  saveValue<T>(key, val);
  return val;
}
// Универсальная функция сохранения значения в NVS
template <typename T> void saveValue(const char *key, T val) {
    lockPrefs();
  prefs.begin("MINIDASH", false);
  if constexpr (std::is_same<T, float>::value)
    prefs.putFloat(key, val);
  else if constexpr (std::is_same<T, int>::value)
    prefs.putInt(key, val);
  else if constexpr (std::is_same<T, String>::value)
    prefs.putString(key, val.c_str());
  prefs.end();
    unlockPrefs();
}

namespace wifi_internal {
inline String staSsid;    // SSID внешней сети
inline String staPass;    // Пароль внешней сети
inline String staBackupSsid; // Резервный SSID внешней сети
inline String staBackupPass; // Пароль резервной внешней сети
inline String apSsid;     // Имя точки доступа устройства
inline String apPass;     // Пароль точки доступа
inline String hostName;   // Имя хоста для mDNS и DHCP
inline bool staAttemptInProgress = false; // Флаг текущей попытки подключения к STA
inline bool fallbackApActive = false;     // Флаг активного аварийного AP
inline bool mdnsStarted = false;          // Указывает, что mDNS уже запущен
inline bool usingBackupSta = false;       // Сейчас подключаемся к резервной STA-сети
inline int attemptCount = 0;              // Счетчик попыток подключения
inline const int maxAttempts = 5;         // Максимум попыток перед переходом в AP
inline unsigned long lastAttemptStarted = 0; // Когда стартовала текущая попытка
inline unsigned long lastStatusCheck = 0;    // Когда последний раз проверяли статус
inline unsigned long lastSignalScan = 0;     // Когда последний раз сравнивали RSSI основной и резервной сети
inline unsigned long checkInterval = 1000;      // 1s until stable connection
inline const unsigned long attemptInterval = 5000; // retry window while connecting
inline const unsigned long signalScanInterval = 120000; // Не сканируем WiFi чаще 1 раза в 2 минуты
inline const int signalSwitchHysteresisDb = 8; // Переключаемся только при заметно лучшем сигнале, чтобы не прыгать между AP

inline void beginMdns() {
  if (mdnsStarted)
    return;
  if (MDNS.begin(hostName.c_str())) {
    MDNS.addService("http", "tcp", 80); // Регистрируем веб-сервис
    mdnsStarted = true;                  // Больше не пытаемся запускать повторно
  }
}

inline void stopMdns() {
  if (!mdnsStarted)
    return;
  MDNS.end();
  mdnsStarted = false;
}

inline void startCaptiveDns() {
  captiveDnsServer.stop();
  captiveDnsServer.start(captiveDnsPort, "*", WiFi.softAPIP()); // Wildcard DNS для captive portal в AP.
  captiveDnsActive = true;
}

inline void stopCaptiveDns() {
  if (captiveDnsActive) captiveDnsServer.stop();
  captiveDnsActive = false;
}

inline void startHiddenApForSta() {
  WiFi.softAP(apSsid.c_str(), apPass.c_str(), 1, true); // Поднимаем скрытый AP, чтобы устройство оставалось доступным
}

inline void activateFallbackAp();

inline bool chooseBestStaBySignal(bool connectedMode) {
  if (!staBackupSsid.length()) {
    usingBackupSta = false;
    return false;
  }
  if (!staSsid.length()) {
    const bool changed = !usingBackupSta;
    usingBackupSta = true;
    return changed;
  }

  wifi_mode_t prevMode = WiFi.getMode();
  if (prevMode == WIFI_MODE_NULL) WiFi.mode(WIFI_MODE_STA);
  else if (prevMode == WIFI_MODE_AP) WiFi.mode(WIFI_MODE_APSTA);

  int primaryRssi = -1000;
  int backupRssi = -1000;
  int16_t count = WiFi.scanNetworks(false, true);
  if (count < 0) count = 0;
  for (int i = 0; i < count; i++) {
    String found = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    if (found == staSsid && rssi > primaryRssi) primaryRssi = rssi;
    if (found == staBackupSsid && rssi > backupRssi) backupRssi = rssi;
  }
  WiFi.scanDelete();

  if (prevMode == WIFI_MODE_AP) WiFi.mode(prevMode);

  bool desiredBackup = usingBackupSta;
  const int hysteresis = connectedMode ? signalSwitchHysteresisDb : 0;
  if (backupRssi > -1000 && (primaryRssi <= -1000 || backupRssi >= primaryRssi + hysteresis)) {
    desiredBackup = true;
  } else if (primaryRssi > -1000 && (backupRssi <= -1000 || primaryRssi >= backupRssi + hysteresis)) {
    desiredBackup = false;
  }

  if (desiredBackup != usingBackupSta) {
    Serial.printf("[WiFi] Auto selected %s SSID by RSSI: primary=%d dBm, backup=%d dBm\n",
                  desiredBackup ? "backup" : "primary", primaryRssi, backupRssi);
    usingBackupSta = desiredBackup;
    attemptCount = 0;
    return true;
  }
  return false;
}

inline void startStaAttempt() {
  if (!staSsid.length() && staBackupSsid.length()) usingBackupSta = true;
  const String &connectSsid = usingBackupSta ? staBackupSsid : staSsid;
  const String &connectPass = usingBackupSta ? staBackupPass : staPass;
  if (!connectSsid.length()) {
    activateFallbackAp();
    return;
  }
  WiFi.mode(WIFI_MODE_APSTA);                 // Разрешаем одновременный STA и AP
  WiFi.setHostname(hostName.c_str());         // Применяем имя хоста перед подключением
  startHiddenApForSta();                      // Оставляем доступ через скрытый AP на время попытки
  WiFi.begin(connectSsid.c_str(), connectPass.c_str());
  staAttemptInProgress = true;                // Отмечаем, что попытка идет
  lastAttemptStarted = millis();              // Запоминаем время старта
  attemptCount++;                             // Увеличиваем счетчик попыток
  Serial.printf("[WiFi] Connecting to %s SSID \"%s\" (attempt %d/%d), hostname: %s\n",
                usingBackupSta ? "backup" : "primary",
                connectSsid.c_str(), attemptCount, maxAttempts, hostName.c_str());
}

inline void activateFallbackAp() {
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid.c_str(), apPass.c_str()); // Запускаем только AP, чтобы пользователь смог подключиться
    startCaptiveDns(); // В AP режиме все DNS-запросы ведем на ESP32 для автоперехода браузеров.
  fallbackApActive = true;                     // Запоминаем, что мы в аварийном режиме
  staAttemptInProgress = false;
  attemptCount = 0;
  checkInterval = 1000;
  Serial.printf("[WiFi] Fallback AP active: SSID \"%s\" | IP: %s\n",
                apSsid.c_str(), WiFi.softAPIP().toString().c_str());
}

inline String buildModeText() {
  wifi_mode_t mode = WiFi.getMode();
  switch (mode) {
  case WIFI_MODE_STA:
    return "STA";    // Работаем только как клиент
  case WIFI_MODE_AP:
    return "AP";     // Только точка доступа
  case WIFI_MODE_APSTA:
    return "STA+AP"; // Гибридный режим
  default:
    return "Unknown"; // Непредвиденное состояние
  }
}

inline void onConnected() {
  fallbackApActive = false;    // AP больше не нужен
  staAttemptInProgress = false;
  checkInterval = 120000;      // 120 seconds — замедляем проверки статуса при стабильной связи
  attemptCount = 0;            // Сбрасываем счетчик
    stopCaptiveDns();            // В STA captive DNS не нужен
  WiFi.mode(WIFI_STA);         // Оставляем только STA
  beginMdns();                 // Стартуем mDNS для удобного доступа
  Serial.printf("[WiFi] Connected: SSID \"%s\" | IP: %s | RSSI: %d dBm | Mode: %s\n",
                WiFi.SSID().c_str(),
                WiFi.localIP().toString().c_str(),
                WiFi.RSSI(),
                buildModeText().c_str());
}

inline String buildStatusText() {
  if (WiFi.status() == WL_CONNECTED)
    return "Connected";          // Соединение установлено
  if (staAttemptInProgress && attemptCount <= maxAttempts)
    return "Connecting";         // Идет попытка подключения
  if (fallbackApActive)
    return "AP Only";            // Мы в автономном AP режиме
  return "Disconnected";         // Подключение отсутствует
}

inline void ensureConnection() {
  const unsigned long now = millis();
  if (now - lastStatusCheck < checkInterval)
    return; // Еще не время следующей проверки
  lastStatusCheck = now;

  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    onConnected(); // Сбрасываем счетчики и отключаем AP
    if (staBackupSsid.length() && now - lastSignalScan >= signalScanInterval) {
      lastSignalScan = now;
      if (chooseBestStaBySignal(true)) {
        WiFi.disconnect(true);
        staAttemptInProgress = false;
        attemptCount = 0;
        checkInterval = 1000;
        startStaAttempt();
      }
    }
    return;
  }

  if (checkInterval != 1000)
    checkInterval = 1000; // Возвращаем быстрые проверки, если потеряли сеть

  if (fallbackApActive) {
    return; // Уже в режиме AP, дополнительных действий не требуется
  }

  if (staAttemptInProgress && (now - lastAttemptStarted) < attemptInterval) {
    return; // still waiting for current attempt
  }

  if (attemptCount < maxAttempts) {
    startStaAttempt();
  } else if (!usingBackupSta && staBackupSsid.length()) {
    Serial.printf("[WiFi] Primary SSID \"%s\" unavailable, switching to backup SSID \"%s\"\n",
                  staSsid.c_str(), staBackupSsid.c_str());
    WiFi.disconnect(true);
    staAttemptInProgress = false;
    usingBackupSta = true;
    attemptCount = 0;
    startStaAttempt();
  } else {
    activateFallbackAp();
  }
}
} // namespace wifi_internal

inline void initWiFiModule() {
  using namespace wifi_internal;
  staSsid = loadValue<String>("ssid", String(defaultSSID));      // Читаем сохраненный SSID
  staPass = loadValue<String>("pass", String(defaultPASS));      // Читаем пароль
  staBackupSsid = loadValue<String>("ssid2", defaultSSIDr);        // Читаем резервный SSID
  staBackupPass = loadValue<String>("pass2", defaultPASSr);        // Читаем пароль резервной сети
  apSsid = loadValue<String>("apSSID", String(::apSSID));        // Имя AP из NVS
  apPass = loadValue<String>("apPASS", String(::apPASS));        // Пароль AP из NVS
  hostName = loadValue<String>("hostname", String(defaultHostname)); // Имя хоста из NVS

    Serial.printf("[WiFi] Config: STA SSID \"%s\" | Backup SSID \"%s\" | AP SSID \"%s\" | Hostname \"%s\"\n",
                staSsid.c_str(), staBackupSsid.c_str(), apSsid.c_str(), hostName.c_str());
                
  WiFi.persistent(false);        // Не сохраняем настройки Wi-Fi во флэш автоматически
  WiFi.mode(WIFI_STA);           // Первичный режим — STA
  stopMdns();                    // Гарантируем чистый запуск mDNS
  WiFi.setHostname(hostName.c_str());
  lastSignalScan = millis();
  chooseBestStaBySignal(false);  // При старте сразу выбираем основную/резервную сеть с лучшим RSSI
  startStaAttempt();             // Запускаем первую попытку подключения
}

inline void wifiModuleLoop() { if (captiveDnsActive) captiveDnsServer.processNextRequest(); wifi_internal::ensureConnection(); }

inline WifiStatusInfo getWifiStatus() {
  WifiStatusInfo info{};
  wifi_mode_t mode = WiFi.getMode();
  info.modeText = wifi_internal::buildModeText(); // STA / AP / STA+AP
  info.statusText = wifi_internal::buildStatusText(); // Connected / Connecting / AP Only / Disconnected
  info.ssid = WiFi.SSID();
  info.ip = (mode == WIFI_MODE_AP) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  info.rssi = WiFi.isConnected() ? WiFi.RSSI() : 0;
  info.apActive = mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA;
  return info;
}

inline String wifiHostName() { return wifi_internal::hostName; } // Текущее имя хоста

inline String scanWifiNetworksJson() {
  wifi_mode_t prevMode = WiFi.getMode();
  bool apOnly = (prevMode == WIFI_MODE_AP);
  wifi_mode_t targetMode = prevMode;

    bool staWasEnabled = WiFi.getMode() == WIFI_MODE_STA || WiFi.getMode() == WIFI_MODE_APSTA;
  
    if (apOnly)
    targetMode = WIFI_MODE_APSTA; // Для сканирования добавляем STA
  else if (prevMode == WIFI_MODE_NULL)
    targetMode = WIFI_MODE_STA;   // Если Wi-Fi выключен — включаем STA

  if (targetMode != prevMode) {
    WiFi.mode(targetMode);

    if (apOnly) WiFi.enableSTA(true); // В режиме AP модуль STA был выключен, включаем для сканирования


    delay(50);
  }

  WiFi.scanDelete();
  int16_t n = WiFi.scanNetworks(false, true); // Быстрое сканирование в текущем канале
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
  WiFi.scanDelete(); // Очищаем результаты сканирования

  if (targetMode != prevMode) {

    if (apOnly && !staWasEnabled)
      WiFi.enableSTA(false); // Возвращаем исходное состояние STA

    WiFi.mode(prevMode);
  }
  return json; // Готовый список сетей в JSON
}

inline bool saveWifiConfig(const String &ssid, const String &pass, const String &backupSsid, const String &backupPass, const String &apSsidIn, const String &apPassIn, const String &hostNameIn) {
  using namespace wifi_internal;
  String storedSsid = loadValue<String>("ssid", String(defaultSSID));
  String storedPass = loadValue<String>("pass", String(defaultPASS));
  staSsid = ssid.length() ? ssid : (storedSsid.length() ? storedSsid : String(defaultSSID)); // Не затираем SSID пустой строкой.
  staPass = pass.length() ? pass : storedPass;                  // Пустой пароль из формы не стирает сохраненный пароль.
  staBackupSsid = backupSsid;                                    // Пустой резервный SSID отключает резервную STA-сеть.
  staBackupPass = backupPass;                                    // Пароль резервной сети хранится как введён.
  apSsid = apSsidIn.length() ? apSsidIn : String(::apSSID);     // Новое имя AP
  apPass = apPassIn;                                            // Новый пароль AP
  hostName = hostNameIn.length() ? hostNameIn : String(defaultHostname); // Новое имя хоста

  saveValue<String>("ssid", staSsid);
  saveValue<String>("pass", staPass);
  saveValue<String>("ssid2", staBackupSsid);
  saveValue<String>("pass2", staBackupPass);
  saveValue<String>("apSSID", apSsid);
  saveValue<String>("apPASS", apPass);
  saveValue<String>("hostname", hostName);

  WiFi.disconnect(true);
  stopMdns();
  fallbackApActive = false;
  staAttemptInProgress = false;
  usingBackupSta = false;
  attemptCount = 0;                            // Сбрасываем счетчик — новые попытки возможны после изменения настроек
  checkInterval = 1000;                        // Проверяем статус чаще, пока нет подключения
  lastSignalScan = millis();
  chooseBestStaBySignal(false);
  startStaAttempt();
  return true;
}

inline bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }
inline String currentApSsid() { return wifi_internal::apSsid; }
inline String currentApPass() { return wifi_internal::apPass; }
