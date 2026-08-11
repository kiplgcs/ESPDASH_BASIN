#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <esp_wifi.h>
#include <type_traits>

struct WifiStatusInfo {
  String statusText;
  String modeText;
  String ssid;
  String ip;
  String networkRole;
  String disconnectReasonText;
  int rssi;
  int primaryRssi;
  int backupRssi;
  bool apActive;
  bool usingBackup;
  int statusCode;
  int disconnectReasonCode;
  uint32_t connectCount;
  unsigned long connectedForSec;
  unsigned long lastConnectedAgoSec;
  unsigned long lastDisconnectedAgoSec;
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
inline const int maxAttempts = 2;         // Две полноценные попытки на сеть дают быстрый, но не дерганый failover.
inline unsigned long lastAttemptStarted = 0; // Когда стартовала текущая попытка
inline unsigned long lastStatusCheck = 0;    // Когда последний раз проверяли статус
inline unsigned long checkInterval = 1000;      // 1s until stable connection
inline const unsigned long attemptInterval = 12000; // Полная попытка подключения, включая DHCP.
inline const unsigned long connectedCheckInterval = 1000; // Быстро обнаруживаем реальный разрыв.
inline const unsigned long primaryRecoveryScanInterval = 30000; // На резервной сети регулярно ищем восстановившуюся основную.
inline const unsigned long degradedPrimaryScanInterval = 30000; // При слабой основной сравниваем ее с резервной.
inline const unsigned long normalPrimaryScanInterval = 120000; // При хорошем сигнале сравнение редкое и не нагружает радио.
inline const unsigned long fallbackRecoveryScanInterval = 30000; // В свободной AP периодически ищем уже сохраненные STA-сети.
inline const unsigned long fallbackClientQuietInterval = 10000; // После ухода клиента даем Web-сессии завершиться перед сканированием.
inline const unsigned long primaryRetryCooldown = 60000; // После неудачи не пытаемся вернуться на основную каждую секунду.
inline const int primaryDegradedRssiDbm = -75; // Ниже этого уровня основная сеть считается деградировавшей.
inline const int usablePrimaryRssiDbm = -78; // Минимальный пригодный сигнал при близком уровне резервной сети.
inline const int signalSwitchHysteresisDb = 12; // Резервная должна быть заметно лучше, а не случайно выиграть пару dB.
inline const int primaryReturnMarginDb = 6; // Для возврата основная должна приблизиться к уровню резервной сети.
inline const int requiredHealthyScans = 2; // Сеть должна подтвердиться двумя последовательными сканированиями.
inline bool wasStaConnected = false; // Помнит, что STA уже была подключена, чтобы отличать обрыв от штатного старта.
inline unsigned long lastStaConnectedAt = 0; // Последний момент успешного подключения.
inline unsigned long lastStaDisconnectedAt = 0; // Последний момент обнаруженной потери Wi-Fi.
inline uint32_t staConnectCount = 0; // Счетчик успешных STA-подключений с момента загрузки.
inline wl_status_t lastWifiStatus = WL_IDLE_STATUS; // Последний статус Wi-Fi для Web-диагностики.
inline String connectedStaSsid; // SSID текущего непрерывного подключения для корректного счетчика времени.
inline uint8_t lastDisconnectReason = 0; // Код причины от драйвера Wi-Fi.
inline unsigned long lastDisconnectEventAt = 0; // Время последнего события разрыва от драйвера.
inline unsigned long primaryRetryNotBefore = 0; // Пауза перед проверочным возвратом с резервной сети.
inline unsigned long lastAutoScanAt = 0; // Последнее автоматическое сканирование.
inline int lastPrimaryRssi = -1000; // Последний найденный RSSI основной сети.
inline int lastBackupRssi = -1000; // Последний найденный RSSI резервной сети.
inline int primaryHealthyScanCount = 0; // Подтверждения восстановления основной сети.
inline int degradedPrimaryScanCount = 0; // Подтверждения деградации основной сети.
inline bool autoSwitchRequested = false; // Результат асинхронного сканирования требует сменить STA.
inline bool autoSwitchToBackup = false; // Целевая сеть для отложенного переключения.
inline bool wifiScanActive = false; // Асинхронное сканирование не блокирует прикладной loop.
inline bool wifiScanForFallbackRecovery = false; // Отличаем фоновое восстановление STA от ручного сканирования Web.
inline bool wifiScanForManualRequest = false; // Ручной Scan WiFi не должен завершаться результатом отмененного фонового поиска.
inline uint8_t manualScanRetryCount = 0; // При пустом первом проходе выполняем один повтор с нормальным временем на канал.
inline unsigned long wifiScanStartedAt = 0;
inline unsigned long lastScanPollAt = 0;
inline bool manualScanQueued = false;
inline bool manualScanReady = false;
inline String manualScanNetworksJson = "[]";
inline SemaphoreHandle_t wifiScanMutex = nullptr;
inline bool savedStaApplyPending = false; // Новые STA-настройки применяем после отправки HTTP-ответа браузеру.
inline unsigned long savedStaApplyAt = 0;
inline const unsigned long savedStaApplyDelay = 2000;
inline unsigned long lastApClientSeenAt = 0;

inline String normalizedMdnsHostName() { // ESPmDNS 2.0.14 хранит lowercase-копию, но передает в IDF исходную строку.
  String normalized = hostName;
  normalized.trim();
  normalized.toLowerCase();
  String safe;
  safe.reserve(normalized.length());
  for (size_t i = 0; i < normalized.length() && safe.length() < 63; i++) {
    const char c = normalized[i];
    const bool valid = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-';
    safe += valid ? c : '-';
  }
  while (safe.startsWith("-")) safe.remove(0, 1);
  while (safe.endsWith("-")) safe.remove(safe.length() - 1);
  return safe.length() ? safe : String("esp32");
}

inline void beginMdns() {
  if (mdnsStarted)
    return;
  const String mdnsHost = normalizedMdnsHostName();
  if (MDNS.begin(mdnsHost.c_str())) {
    MDNS.addService("http", "tcp", 80); // Регистрируем веб-сервис
    mdnsStarted = true;                  // Больше не пытаемся запускать повторно
    Serial.printf("[WiFi] mDNS ready: http://%s.local/\n", mdnsHost.c_str());
  } else {
    Serial.printf("[WiFi] mDNS start failed for hostname \"%s\"\n", mdnsHost.c_str());
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

inline void activateFallbackAp();
inline void startStaAttempt();

inline void ensureWifiScanMutex() {
  if (wifiScanMutex == nullptr) wifiScanMutex = xSemaphoreCreateMutex();
}

inline String wifiScanEscape(String value) {
  value.replace("\\", "\\\\");
  value.replace("\"", "\\\"");
  return value;
}

inline String wifiAuthText(wifi_auth_mode_t authMode) {
  switch (authMode) {
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA";
    case WIFI_AUTH_WPA2_PSK: return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK: return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
    default: return "open";
  }
}

inline String wifiScanNetworkJsonEntry(const String &ssid, int rssi, const String &authText) {
  return "{\"ssid\":\"" + wifiScanEscape(ssid) + "\"," +
         "\"rssi\":" + String(rssi) + ',' +
         "\"auth\":\"" + wifiScanEscape(authText) + "\"}";
}

inline String connectedNetworkFallbackJson() {
  if (WiFi.status() != WL_CONNECTED) return "[]";
  const String currentSsid = WiFi.SSID();
  if (!currentSsid.length()) return "[]";
  return "[" + wifiScanNetworkJsonEntry(currentSsid, WiFi.RSSI(), "подключена") + "]";
}

inline void evaluateScanForAutomaticSwitch(int primaryRssi, int backupRssi) {
  const unsigned long now = millis();
  if (WiFi.status() == WL_CONNECTED) {
    const String actualSsid = WiFi.SSID();
    const bool connectedToBackup = staBackupSsid.length() && actualSsid == staBackupSsid && actualSsid != staSsid;
    if (connectedToBackup) {
      if (now < primaryRetryNotBefore || primaryRssi <= -1000) {
        primaryHealthyScanCount = 0;
        return;
      }
      const bool primaryHealthy = primaryRssi >= usablePrimaryRssiDbm &&
                                  (backupRssi <= -1000 || primaryRssi >= backupRssi - primaryReturnMarginDb);
      primaryHealthyScanCount = primaryHealthy ? primaryHealthyScanCount + 1 : 0;
      if (primaryHealthyScanCount >= requiredHealthyScans) {
        autoSwitchRequested = true;
        autoSwitchToBackup = false;
        primaryHealthyScanCount = 0;
        Serial.printf("[WiFi] Primary network recovered: primary=%d dBm, backup=%d dBm\n", primaryRssi, backupRssi);
      }
      return;
    }

    const int currentPrimaryRssi = WiFi.RSSI();
    const bool primaryDegraded = backupRssi > -1000 &&
                                 backupRssi >= currentPrimaryRssi + signalSwitchHysteresisDb;
    degradedPrimaryScanCount = primaryDegraded ? degradedPrimaryScanCount + 1 : 0;
    if (degradedPrimaryScanCount >= requiredHealthyScans) {
      autoSwitchRequested = true;
      autoSwitchToBackup = true;
      degradedPrimaryScanCount = 0;
      primaryRetryNotBefore = now + primaryRetryCooldown;
      Serial.printf("[WiFi] Primary network degraded: primary=%d dBm, backup=%d dBm\n", currentPrimaryRssi, backupRssi);
    }
    return;
  }

}

inline bool startAsyncWifiScan(bool fallbackRecovery = false, bool manualRequest = false) {
  if (wifiScanActive) return true;
  wifi_mode_t mode = WiFi.getMode();
  if (mode == WIFI_MODE_NULL) WiFi.mode(WIFI_MODE_STA);
  else if (mode == WIFI_MODE_AP) WiFi.mode(WIFI_MODE_APSTA); // Видимая аварийная AP при сканировании не останавливается.
  WiFi.setSleep(false); // Off-channel scan и Web AP устойчивее без энергосбережения радиомодуля.
  WiFi.scanDelete();
  const uint32_t channelTimeMs = manualRequest ? 220U : 160U; // Полный ручной поиск видит сильные сети, но не уводит AP с ее канала надолго.
  const int16_t state = WiFi.scanNetworks(true, true, false, channelTimeMs); // Асинхронный активный поиск не блокирует управление бассейном.
  if (state == WIFI_SCAN_RUNNING) {
    wifiScanActive = true;
    wifiScanForFallbackRecovery = fallbackRecovery;
    wifiScanForManualRequest = manualRequest;
    wifiScanStartedAt = millis();
    lastAutoScanAt = wifiScanStartedAt;
    Serial.printf("[WiFi] %s scan started (%lu ms/channel)\n",
                  manualRequest ? "Manual" : (fallbackRecovery ? "Fallback recovery" : "Background"),
                  static_cast<unsigned long>(channelTimeMs));
    return true;
  }
  wifiScanForFallbackRecovery = false;
  wifiScanForManualRequest = false;
  return false;
}

inline void restoreStableFallbackApMode() {
  if (!fallbackApActive) return;
  if (WiFi.softAPgetStationNum() > 0) { // Не меняем режим радио под подключенным пользователем: AP остается доступной.
    if (!captiveDnsActive) startCaptiveDns();
    beginMdns();
    return;
  }
  if (WiFi.getMode() != WIFI_MODE_AP) {
    WiFi.setAutoReconnect(false); // В сервисном AP драйвер STA не должен запускать фоновые переподключения.
    WiFi.disconnect(false, false);
    WiFi.mode(WIFI_MODE_AP);
    WiFi.softAPsetHostname(hostName.c_str());
    WiFi.softAP(apSsid.c_str(), apPass.c_str()); // После сканирования возвращаем постоянную AP с теми же параметрами.
  }
  if (!captiveDnsActive) startCaptiveDns();
  beginMdns(); // esp32.local должен работать и на интерфейсе точки доступа.
}

inline void finishManualScan(const String &networksJson) {
  ensureWifiScanMutex();
  if (wifiScanMutex) xSemaphoreTake(wifiScanMutex, portMAX_DELAY);
  if (manualScanQueued) {
    manualScanNetworksJson = networksJson;
    manualScanReady = true;
    manualScanQueued = false;
  }
  if (wifiScanMutex) xSemaphoreGive(wifiScanMutex);
}

inline void pollAsyncWifiScan() {
  if (!wifiScanActive) return;
  const unsigned long now = millis();
  if (now - lastScanPollAt < 100) return;
  lastScanPollAt = now;

  int16_t count = WiFi.scanComplete();
  if (count == WIFI_SCAN_RUNNING) {
    if (now - wifiScanStartedAt < 10000) return;
    count = WIFI_SCAN_FAILED;
  }

  const bool fallbackRecovery = wifiScanForFallbackRecovery;
  const bool manualRequest = wifiScanForManualRequest;
  wifiScanActive = false;
  wifiScanForFallbackRecovery = false;
  wifiScanForManualRequest = false;
  if (count < 0) {
    WiFi.scanDelete();
    if (manualRequest && manualScanRetryCount == 0) {
      manualScanRetryCount++;
      if (startAsyncWifiScan(false, true)) return; // Разовый повтор вместо ложного "сети не найдены".
    }
    if (manualRequest) finishManualScan(connectedNetworkFallbackJson());
    restoreStableFallbackApMode();
    return;
  }

  if (count == 0 && manualRequest && manualScanRetryCount == 0) {
    WiFi.scanDelete();
    manualScanRetryCount++;
    if (startAsyncWifiScan(false, true)) return; // На занятом AP-канале первый проход иногда возвращает пустой список.
  }

  int primaryRssi = -1000;
  int backupRssi = -1000;
  String networksJson = "[";
  bool currentConnectedSsidFound = false;
  const String currentConnectedSsid = WiFi.status() == WL_CONNECTED ? WiFi.SSID() : String();
  for (int i = 0; i < count; i++) {
    const String foundSsid = WiFi.SSID(i);
    const int foundRssi = WiFi.RSSI(i);
    if (foundSsid == staSsid && foundRssi > primaryRssi) primaryRssi = foundRssi;
    if (foundSsid == staBackupSsid && foundRssi > backupRssi) backupRssi = foundRssi;
    if (currentConnectedSsid.length() && foundSsid == currentConnectedSsid) currentConnectedSsidFound = true;
    if (i) networksJson += ',';
    networksJson += wifiScanNetworkJsonEntry(foundSsid, foundRssi, wifiAuthText(WiFi.encryptionType(i)));
  }
  if (currentConnectedSsid.length() && !currentConnectedSsidFound) {
    if (count > 0) networksJson += ',';
    networksJson += wifiScanNetworkJsonEntry(currentConnectedSsid, WiFi.RSSI(), "подключена");
  }
  networksJson += ']';
  lastPrimaryRssi = primaryRssi;
  lastBackupRssi = backupRssi;
  if (fallbackApActive && fallbackRecovery && WiFi.softAPgetStationNum() == 0) {
    if (primaryRssi > -1000 && now >= primaryRetryNotBefore) {
      autoSwitchRequested = true;
      autoSwitchToBackup = false;
    } else if (backupRssi > -1000 && staBackupSsid.length()) {
      autoSwitchRequested = true;
      autoSwitchToBackup = true;
    }
  } else if (!fallbackApActive) {
    evaluateScanForAutomaticSwitch(primaryRssi, backupRssi);
  }
  finishManualScan(networksJson);
  if (manualRequest) Serial.printf("[WiFi] Manual scan completed: %d network(s)\n", count);
  WiFi.scanDelete();
  restoreStableFallbackApMode();
}

inline void startStaAttempt() {
  if (!staSsid.length() && staBackupSsid.length()) usingBackupSta = true;
  const String &connectSsid = usingBackupSta ? staBackupSsid : staSsid;
  const String &connectPass = usingBackupSta ? staBackupPass : staPass;
  if (!connectSsid.length()) {
    activateFallbackAp();
    return;
  }
  WiFi.setAutoReconnect(true);
  if (WiFi.getMode() != WIFI_MODE_STA) WiFi.mode(WIFI_MODE_STA);
  WiFi.setHostname(hostName.c_str());         // Применяем имя хоста перед подключением
  WiFi.setSleep(false);                       // Убираем power-save задержки и потери быстрых Web-запросов.
  WiFi.begin(connectSsid.c_str(), connectPass.c_str());
  staAttemptInProgress = true;                // Отмечаем, что попытка идет
  lastAttemptStarted = millis();              // Запоминаем время старта
  attemptCount++;                             // Увеличиваем счетчик попыток
  Serial.printf("[WiFi] Connecting to %s SSID \"%s\" (attempt %d/%d), hostname: %s\n",
                usingBackupSta ? "backup" : "primary",
                connectSsid.c_str(), attemptCount, maxAttempts, hostName.c_str());
}

inline void activateFallbackAp() {
  const wifi_mode_t previousMode = WiFi.getMode();
  const bool alreadyActive = fallbackApActive && (previousMode == WIFI_MODE_AP || previousMode == WIFI_MODE_APSTA);
  WiFi.setAutoReconnect(false); // Неверные старые STA-реквизиты не должны мешать работе страницы настройки.
  WiFi.disconnect(false, false); // Останавливаем только STA; сохраненную конфигурацию и радио не стираем.
  if (WiFi.getMode() != WIFI_MODE_AP) WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPsetHostname(hostName.c_str());
  if (!alreadyActive) WiFi.softAP(apSsid.c_str(), apPass.c_str()); // Чистый AP не прыгает по каналам из-за фонового STA-сканирования.
  if (!captiveDnsActive) startCaptiveDns(); // В AP режиме все DNS-запросы ведем на ESP32 для автоперехода браузеров.
  fallbackApActive = true;                     // Запоминаем, что мы в аварийном режиме
  staAttemptInProgress = false;
  attemptCount = 0;
  checkInterval = 1000;
  lastAutoScanAt = millis();
  lastApClientSeenAt = lastAutoScanAt;
  stopMdns();
  beginMdns(); // Публикуем http://esp32.local и в автономной AP.
  if (!alreadyActive) {
    Serial.printf("[WiFi] Fallback AP active: SSID \"%s\" | IP: %s\n",
                  apSsid.c_str(), WiFi.softAPIP().toString().c_str());
  }
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
  const String actualSsid = WiFi.SSID();
  const bool firstConnectionAfterLoss = !wasStaConnected || actualSsid != connectedStaSsid;
  wasStaConnected = true;
  usingBackupSta = staBackupSsid.length() && actualSsid == staBackupSsid && actualSsid != staSsid;
  if (firstConnectionAfterLoss) {
    lastStaConnectedAt = millis();
    connectedStaSsid = actualSsid;
    staConnectCount++;
  }
  fallbackApActive = false;    // AP больше не нужен
  staAttemptInProgress = false;
  checkInterval = connectedCheckInterval; // Проверяем чаще, чтобы Web быстро видел реальный обрыв.
  attemptCount = 0;            // Сбрасываем счетчик
    stopCaptiveDns();            // В STA captive DNS не нужен
  if (WiFi.getMode() != WIFI_STA) WiFi.mode(WIFI_STA); // Оставляем только STA без лишней перенастройки радио.
  beginMdns();                 // Стартуем mDNS для удобного доступа
  if (firstConnectionAfterLoss) {
    Serial.printf("[WiFi] Connected to %s network: SSID \"%s\" | IP: %s | RSSI: %d dBm | Mode: %s\n",
                  usingBackupSta ? "backup" : "primary",
                  WiFi.SSID().c_str(),
                  WiFi.localIP().toString().c_str(),
                  WiFi.RSSI(),
                  buildModeText().c_str());
  }
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

inline String disconnectReasonText(uint8_t reason) {
  switch (reason) {
    case 0: return "После запуска отключений не зафиксировано";
    case 1: return "Соединение прервано без уточненной причины";
    case 2: return "Роутер завершил сеанс авторизации; возможна его перезагрузка";
    case 3: return "Роутер завершил подключение ESP32";
    case 4: return "Роутер слишком долго не подтверждал подключение";
    case 5: return "Роутер не принимает новые устройства: достигнут лимит клиентов";
    case 6:
    case 7:
    case 9: return "ESP32 потеряла авторизацию или регистрацию на роутере";
    case 8:
    case 36: return "Штатное отключение ESP32 при переподключении или смене сети";
    case 10:
    case 11: return "Параметры WiFi роутера несовместимы с ESP32";
    case 12:
    case 46:
    case 47: return "Роутер инициировал переход или отключение ESP32";
    case 13:
    case 14:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 29: return "Не совпали параметры защиты WiFi; проверьте пароль и режим WPA2/WPA3";
    case 15:
    case 16:
    case 204: return "Не удалось вовремя завершить защищенное WPA-подключение";
    case 33: return "Роутеру не хватает пропускной способности для соединения";
    case 34: return "Потеряно слишком много пакетов подтверждения; радиоканал нестабилен";
    case 39: return "Роутер не ответил за отведенное время";
    case 67: return "Не удалось установить устойчивый радиоканал";
    case 68: return "Рабочий WiFi-канал занят или сильно перегружен";
    case 200: return "Пропал радиосигнал роутера или роутер был перезагружен";
    case 201: return "Сеть с указанным именем не найдена";
    case 202: return "Роутер отклонил авторизацию; проверьте пароль WiFi";
    case 203: return "Роутер не принял подключение ESP32";
    case 205: return "Общая ошибка подключения к роутеру";
    case 206: return "Роутер перезапустил внутренний счетчик времени соединения";
    case 207: return "ESP32 выполняла переход между точками доступа";
    case 208:
    case 209: return "Роутер слишком долго откладывал повторное подключение";
    default: return "Редкая ошибка WiFi; диагностический код " + String(reason);
  }
}

inline void requestStaSwitch(bool toBackup, const char *reason) {
  if (toBackup && !staBackupSsid.length()) return;
  if (!toBackup && !staSsid.length()) toBackup = true;
  Serial.printf("[WiFi] Switching to %s network: %s\n", toBackup ? "backup" : "primary", reason);
  if (fallbackApActive) {
    fallbackApActive = false;
    stopCaptiveDns();
  }
  WiFi.disconnect(false, false);
  stopMdns();
  wasStaConnected = false;
  staAttemptInProgress = false;
  usingBackupSta = toBackup;
  attemptCount = 0;
  checkInterval = 1000;
  startStaAttempt();
}

inline void ensureConnection() {
  const unsigned long now = millis();

  bool manualScanRequested = false;
  ensureWifiScanMutex();
  if (wifiScanMutex) xSemaphoreTake(wifiScanMutex, portMAX_DELAY);
  manualScanRequested = manualScanQueued;
  if (wifiScanMutex) xSemaphoreGive(wifiScanMutex);

  if (manualScanRequested && wifiScanActive && !wifiScanForManualRequest) {
    // Кнопка Scan WiFi имеет собственный полный проход. Отмененный фоновый поиск не публикуем как пустой результат.
    esp_wifi_scan_stop();
    WiFi.scanDelete();
    wifiScanActive = false;
    wifiScanForFallbackRecovery = false;
    wifiScanForManualRequest = false;
    autoSwitchRequested = false;
  }
  if (manualScanRequested && !wifiScanActive) {
    if (!startAsyncWifiScan(false, true)) {
      finishManualScan(connectedNetworkFallbackJson());
      restoreStableFallbackApMode();
    }
  }

  if (fallbackApActive && wifiScanActive && wifiScanForFallbackRecovery && WiFi.softAPgetStationNum() > 0) {
    esp_wifi_scan_stop(); // Клиент AP важнее фонового поиска: немедленно возвращаем радиоканал Web-интерфейсу.
    WiFi.scanDelete();
    wifiScanActive = false;
    wifiScanForFallbackRecovery = false;
    wifiScanForManualRequest = false;
    autoSwitchRequested = false;
    lastApClientSeenAt = now;
    restoreStableFallbackApMode();
  }
  pollAsyncWifiScan();

  if (savedStaApplyPending && static_cast<long>(now - savedStaApplyAt) >= 0) {
    savedStaApplyPending = false;
    WiFi.scanDelete();
    wifiScanActive = false;
    WiFi.disconnect(false, false);
    stopCaptiveDns();
    stopMdns();
    fallbackApActive = false;
    wasStaConnected = false;
    staAttemptInProgress = false;
    usingBackupSta = !staSsid.length() && staBackupSsid.length();
    attemptCount = 0;
    checkInterval = 1000;
    primaryHealthyScanCount = 0;
    degradedPrimaryScanCount = 0;
    primaryRetryNotBefore = 0;
    lastAutoScanAt = now;
    startStaAttempt(); // HTTP-ответ уже успел уйти; теперь безопасно выключить AP и проверить новые реквизиты.
    return;
  }

  if (autoSwitchRequested) {
    if (fallbackApActive && WiFi.softAPgetStationNum() > 0) {
      autoSwitchRequested = false; // Пользователь успел вернуться в AP — не обрываем его ради автоматического STA.
      lastApClientSeenAt = now;
      restoreStableFallbackApMode();
      return;
    }
    const bool targetBackup = autoSwitchToBackup;
    autoSwitchRequested = false;
    requestStaSwitch(targetBackup, fallbackApActive ? "saved SSID is available" : "confirmed signal policy");
    return;
  }

  if (now - lastStatusCheck < checkInterval)
    return; // Еще не время следующей проверки
  lastStatusCheck = now;

  wl_status_t status = WiFi.status();
  lastWifiStatus = status;
  if (status == WL_CONNECTED) {
    if (!wasStaConnected || fallbackApActive || staAttemptInProgress) onConnected(); // Переход в устойчивое состояние.
    else checkInterval = connectedCheckInterval; // Не трогаем Wi-Fi-режим и не пишем лог при обычной проверке.
    const int currentRssi = WiFi.RSSI();
    const bool connectedToBackup = staBackupSsid.length() && WiFi.SSID() == staBackupSsid && WiFi.SSID() != staSsid;
    const unsigned long scanInterval = connectedToBackup
                                         ? primaryRecoveryScanInterval
                                         : (currentRssi <= primaryDegradedRssiDbm ? degradedPrimaryScanInterval : normalPrimaryScanInterval);
    const bool shouldScan = connectedToBackup
                              ? (staSsid.length() && now >= primaryRetryNotBefore)
                              : staBackupSsid.length();
    if (shouldScan && !wifiScanActive && now - lastAutoScanAt >= scanInterval) {
      startAsyncWifiScan();
    }
    return;
  }

  if (checkInterval != 1000)
    checkInterval = 1000; // Возвращаем быстрые проверки, если потеряли сеть
  if (wasStaConnected) {
    wasStaConnected = false;
    lastStaDisconnectedAt = now;
    stopMdns();
    staAttemptInProgress = false;
    attemptCount = 0;
    usingBackupSta = !staSsid.length() && staBackupSsid.length(); // После любого обрыва сначала проверяем приоритетную сеть.
    Serial.printf("[WiFi] Disconnected: status=%d, reason=%u (%s), reconnecting...\n",
                  static_cast<int>(status), static_cast<unsigned>(lastDisconnectReason),
                  disconnectReasonText(lastDisconnectReason).c_str());
  }

  if (fallbackApActive && !staAttemptInProgress) {
    const uint8_t apClients = WiFi.softAPgetStationNum();
    if (apClients > 0) {
      lastApClientSeenAt = now;
      return; // Пока пользователь на сайте, никакого фонового сканирования и переключения радиоканала.
    }
    if (now - lastApClientSeenAt < fallbackClientQuietInterval) return;
    if (!wifiScanActive && now - lastAutoScanAt >= fallbackRecoveryScanInterval) startAsyncWifiScan(true);
    return;
  }

  if (staAttemptInProgress && (now - lastAttemptStarted) < attemptInterval) {
    return; // still waiting for current attempt
  }

  if (attemptCount < maxAttempts) {
    startStaAttempt();
  } else if (!usingBackupSta && staBackupSsid.length()) {
    Serial.printf("[WiFi] Primary SSID \"%s\" unavailable, switching to backup SSID \"%s\"\n",
                  staSsid.c_str(), staBackupSsid.c_str());
    WiFi.disconnect(false, false);
    staAttemptInProgress = false;
    usingBackupSta = true;
    attemptCount = 0;
    primaryRetryNotBefore = now + primaryRetryCooldown;
    startStaAttempt();
  } else if (fallbackApActive) {
    activateFallbackAp(); // Возвращаемся к неблокирующему поиску, не перезапуская видимую AP.
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
  WiFi.setAutoReconnect(true);   // Драйвер помогает быстро восстановить краткий радиосбой между проверками менеджера.
  WiFi.setSleep(false);          // Стабильная задержка важнее экономии энергии для стационарного контроллера.
  WiFi.onEvent([](WiFiEvent_t, WiFiEventInfo_t info) {
    lastDisconnectReason = info.wifi_sta_disconnected.reason;
    lastDisconnectEventAt = millis();
    lastStaDisconnectedAt = lastDisconnectEventAt;
  }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  stopMdns();                    // Гарантируем чистый запуск mDNS
  WiFi.setHostname(hostName.c_str());
  usingBackupSta = !staSsid.length() && staBackupSsid.length(); // При старте основная сеть всегда имеет приоритет.
  lastAutoScanAt = millis();
  startStaAttempt();             // Запускаем первую попытку подключения
}

inline void wifiModuleLoop() { if (captiveDnsActive) captiveDnsServer.processNextRequest(); wifi_internal::ensureConnection(); }

inline WifiStatusInfo getWifiStatus() {
  WifiStatusInfo info{};
  const unsigned long now = millis();
  wifi_mode_t mode = WiFi.getMode();
  info.modeText = wifi_internal::buildModeText(); // STA / AP / STA+AP
  info.statusText = wifi_internal::buildStatusText(); // Connected / Connecting / AP Only / Disconnected
  info.ssid = WiFi.SSID();
  const bool connected = WiFi.status() == WL_CONNECTED;
  info.ip = (!connected && (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA)) ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  info.rssi = WiFi.isConnected() ? WiFi.RSSI() : 0;
  info.apActive = mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA;
  info.usingBackup = connected && wifi_internal::staBackupSsid.length() &&
                     info.ssid == wifi_internal::staBackupSsid && info.ssid != wifi_internal::staSsid;
  if (connected) info.networkRole = info.usingBackup ? "Резервная сеть WiFi" : "Основная сеть WiFi";
  else if (wifi_internal::fallbackApActive) info.networkRole = "Аварийная точка доступа ESP32";
  else info.networkRole = wifi_internal::usingBackupSta ? "Подключение к резервной сети WiFi" : "Подключение к основной сети WiFi";
  info.disconnectReasonCode = wifi_internal::lastDisconnectReason;
  info.disconnectReasonText = wifi_internal::disconnectReasonText(wifi_internal::lastDisconnectReason);
  info.primaryRssi = wifi_internal::lastPrimaryRssi;
  info.backupRssi = wifi_internal::lastBackupRssi;
  info.statusCode = static_cast<int>(WiFi.status());
  info.connectCount = wifi_internal::staConnectCount;
  info.connectedForSec = connected && wifi_internal::lastStaConnectedAt ? (now - wifi_internal::lastStaConnectedAt) / 1000UL : 0;
  info.lastConnectedAgoSec = wifi_internal::lastStaConnectedAt ? (now - wifi_internal::lastStaConnectedAt) / 1000UL : 0;
  info.lastDisconnectedAgoSec = wifi_internal::lastStaDisconnectedAt ? (now - wifi_internal::lastStaDisconnectedAt) / 1000UL : 0;
  return info;
}

inline String wifiHostName() { return wifi_internal::hostName; } // Текущее имя хоста

inline bool requestWifiNetworkScan() {
  using namespace wifi_internal;
  ensureWifiScanMutex();
  if (wifiScanMutex) xSemaphoreTake(wifiScanMutex, portMAX_DELAY);
  manualScanQueued = true;
  manualScanReady = false;
  manualScanRetryCount = 0;
  if (wifiScanMutex) xSemaphoreGive(wifiScanMutex);
  return true; // Радио запускается из основного loop, а не из AsyncWebServer task.
}

inline String wifiNetworkScanStateJson() {
  using namespace wifi_internal;
  ensureWifiScanMutex();
  if (wifiScanMutex) xSemaphoreTake(wifiScanMutex, portMAX_DELAY);
  const bool ready = manualScanReady;
  const bool scanning = wifiScanActive || manualScanQueued;
  const String networks = manualScanNetworksJson;
  if (wifiScanMutex) xSemaphoreGive(wifiScanMutex);
  return String("{\"scanning\":") + (scanning ? "true" : "false") +
         ",\"ready\":" + (ready ? "true" : "false") +
         ",\"networks\":" + networks + "}";
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

  savedStaApplyPending = true;
  savedStaApplyAt = millis() + savedStaApplyDelay; // Не обрываем AP раньше, чем браузер получит подтверждение сохранения.
  return true;
}

inline bool wifiIsConnected() { return WiFi.status() == WL_CONNECTED; }
inline String currentApSsid() { return wifi_internal::apSsid; }
inline String currentApPass() { return wifi_internal::apPass; }
