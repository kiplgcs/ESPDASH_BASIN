#pragma once
#include <Arduino.h>
#include <math.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "wifi_manager.h"

// DeviceAddress sensor0 = {0x28, 0xff, 0x64, 0x1e, 0x83, 0x7a, 0x05, 0x83}; // Указываем адрес датчика 28-ff-64-1e-83-7a-05-83
// DeviceAddress sensor1= {0x28, 0xff, 0x64, 0x1e, 0x83, 0x61, 0xbe, 0x5e}; // Указываем адрес датчика 28-ff-64-1e-83-61-be-5e

DeviceAddress sensor0 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Температура бассейна.
DeviceAddress sensor1 = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Температура после нагревателя.

constexpr size_t kMaxDs18Sensors = 16; // Служебная строка логики DS18B20.
inline DeviceAddress ds18Found[kMaxDs18Sensors]; // Служебная строка логики DS18B20.
inline int ds18FoundCount = 0; // Служебная строка логики DS18B20.

#define ONE_WIRE_BUS 14

OneWire oneWire(ONE_WIRE_BUS); // Служебная строка логики DS18B20.
DallasTemperature sensors(&oneWire); // Служебная строка логики DS18B20.

constexpr const char *kDs18Sensor0StorageKey = "ds18_s0_addr"; // Служебная строка логики DS18B20.
constexpr const char *kDs18Sensor1StorageKey = "ds18_s1_addr"; // Служебная строка логики DS18B20.
constexpr const char *kDs18Sensor0IndexStorageKey = "ds18_s0_idx"; // Служебная строка логики DS18B20.
constexpr const char *kDs18Sensor1IndexStorageKey = "ds18_s1_idx"; // Служебная строка логики DS18B20.


float DS1 = 0, Saved_DS1; // Служебная строка логики DS18B20.
float DS2 = 0, Saved_DS2; // Служебная строка логики DS18B20.
inline bool DS1Available = false; // Служебная строка логики DS18B20.
inline bool DS2Available = false; // Служебная строка логики DS18B20.
inline bool DS1Assigned = false; // Служебная строка логики DS18B20.
inline bool DS2Assigned = false; // Служебная строка логики DS18B20.

inline String formatTemperatureString(float value, bool available) { // Начало inline-функции для работы с адресами/состоянием DS18B20.
  return available ? (String(value, 1) + " \u00B0C") : "n/a"; // Возвращаем рассчитанный результат вызывающему коду.
}

inline String formatDeviceAddress(const DeviceAddress &addr) { // Начало inline-функции для работы с адресами/состоянием DS18B20.
  String result; // Служебная строка логики DS18B20.
  for (int i = 0; i < 8; i++) { // Последовательно обрабатываем элементы адреса или список датчиков.
    if (addr[i] < 16) result += "0"; // Проверяем условие корректности данных/состояния.
    result += String(addr[i], HEX); // Служебная строка логики DS18B20.
    if (i < 7) result += "-"; // Проверяем условие корректности данных/состояния.
  }
  result.toUpperCase(); // Служебная строка логики DS18B20.
  return result; // Возвращаем рассчитанный результат вызывающему коду.
}

inline bool deviceAddressIsZero(const DeviceAddress &addr) { // Начало inline-функции для работы с адресами/состоянием DS18B20.
  for (uint8_t i = 0; i < sizeof(DeviceAddress); i++) { // Последовательно обрабатываем элементы адреса или список датчиков.
    if (addr[i] != 0x00) return false; // Проверяем условие корректности данных/состояния.
  }
  return true; // Возвращаем рассчитанный результат вызывающему коду.
}

inline bool parseDeviceAddressString(const String &input, DeviceAddress &out) { // Начало inline-функции для работы с адресами/состоянием DS18B20.
  int byteIndex = 0; // Служебная строка логики DS18B20.
  int nibbleCount = 0; // Служебная строка логики DS18B20.
  uint8_t current = 0; // Служебная строка логики DS18B20.
  for (size_t i = 0; i < input.length() && byteIndex < 8; i++) { // Последовательно обрабатываем элементы адреса или список датчиков.
    char c = input[i]; // Служебная строка логики DS18B20.
    int value = -1; // Служебная строка логики DS18B20.
    if (c >= '0' && c <= '9') value = c - '0'; // Проверяем условие корректности данных/состояния.
    else if (c >= 'a' && c <= 'f') value = 10 + (c - 'a'); // Служебная строка логики DS18B20.
    else if (c >= 'A' && c <= 'F') value = 10 + (c - 'A'); // Служебная строка логики DS18B20.
    if (value < 0) continue; // Проверяем условие корректности данных/состояния.
    current = static_cast<uint8_t>((current << 4) | value); // Служебная строка логики DS18B20.
    nibbleCount++; // Служебная строка логики DS18B20.
    if (nibbleCount == 2) { // Проверяем условие корректности данных/состояния.
      out[byteIndex++] = current; // Служебная строка логики DS18B20.
      current = 0; // Служебная строка логики DS18B20.
      nibbleCount = 0; // Служебная строка логики DS18B20.
    }
  }

  return byteIndex == 8; // Возвращаем рассчитанный результат вызывающему коду.
}

inline void loadSavedDs18Address(const char *key, DeviceAddress &target, String &label) { // Начало inline-функции для работы с адресами/состоянием DS18B20.
  String defaultAddress = formatDeviceAddress(target); // Служебная строка логики DS18B20.
  String stored = loadValue<String>(key, defaultAddress); // Служебная строка логики DS18B20.
  DeviceAddress parsed; // Служебная строка логики DS18B20.
  if (parseDeviceAddressString(stored, parsed)) { // Проверяем условие корректности данных/состояния.
    memcpy(target, parsed, sizeof(DeviceAddress)); // Копируем выбранный 8-байтовый адрес датчика в целевую переменную.
  }
  label = deviceAddressIsZero(target) ? "не назначен" : formatDeviceAddress(target); // Служебная строка логики DS18B20.

}




inline String scanDs18Sensors() { // Начало inline-функции для работы с адресами/состоянием DS18B20.
  String result; // Служебная строка логики DS18B20.
  sensors.begin(); // Выполняем действие библиотеки DallasTemperature для DS18B20.
  int qty = sensors.getDeviceCount(); // Служебная строка логики DS18B20.
  ds18FoundCount = 0; // Служебная строка логики DS18B20.

  result += "Найдено устройств: " + String(qty) + "\n"; // Служебная строка логики DS18B20.

  for (int i = 0; i < qty && ds18FoundCount < static_cast<int>(kMaxDs18Sensors); i++) { // Последовательно обрабатываем элементы адреса или список датчиков.
    DeviceAddress addr; // Служебная строка логики DS18B20.
    if (sensors.getAddress(addr, i)) { // Проверяем условие корректности данных/состояния.
      memcpy(ds18Found[ds18FoundCount], addr, sizeof(DeviceAddress)); // Копируем выбранный 8-байтовый адрес датчика в целевую переменную.
      result += "Индекс " + String(ds18FoundCount) + ": " + formatDeviceAddress(addr) + "\n"; // Служебная строка логики DS18B20.
      ds18FoundCount++; // Служебная строка логики DS18B20.
    }
  }

  if (qty > static_cast<int>(kMaxDs18Sensors)) { // Проверяем условие корректности данных/состояния.
    result += "Показаны только первые " + String(kMaxDs18Sensors) + " датчиков.\n"; // Служебная строка логики DS18B20.
  }

  if (ds18FoundCount == 0) { // Проверяем условие корректности данных/состояния.
    result += "Датчики DS18B20 не обнаружены."; // Служебная строка логики DS18B20.
  }

return result; // Возвращаем рассчитанный результат вызывающему коду.
}

inline bool assignDs18SensorFromIndex(DeviceAddress &target, int index, String &label, const char *storageKey, String &info) { // Начало inline-функции для работы с адресами/состоянием DS18B20.
  if (index == -1) { // Проверяем условие корректности данных/состояния.
    DeviceAddress emptyAddress = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Служебная строка логики DS18B20.
    memcpy(target, emptyAddress, sizeof(DeviceAddress)); // Копируем выбранный 8-байтовый адрес датчика в целевую переменную.
    label = "не назначен"; // Служебная строка логики DS18B20.
    saveValue<String>(storageKey, formatDeviceAddress(emptyAddress)); // Сохраняем значение в энергонезависимую память ESP32 (NVS).
    if (&target == &sensor0) { // Проверяем условие корректности данных/состояния.
      DS1Assigned = false; // Служебная строка логики DS18B20.
      DS1Available = false; // Служебная строка логики DS18B20.
    } else if (&target == &sensor1) { // Служебная строка логики DS18B20.
      DS2Assigned = false; // Служебная строка логики DS18B20.
      DS2Available = false; // Служебная строка логики DS18B20.
    }
    info = "Привязка датчика снята."; // Служебная строка логики DS18B20.
    return true; // Возвращаем рассчитанный результат вызывающему коду.
  }

    if (index < -1) { // Проверяем условие корректности данных/состояния.
    info = "Ожидание выбора датчика."; // Служебная строка логики DS18B20.
    return false; // Возвращаем рассчитанный результат вызывающему коду.
  }

  if (ds18FoundCount == 0) { // Проверяем условие корректности данных/состояния.
    info = "Сначала нажмите кнопку поиска датчиков."; // Служебная строка логики DS18B20.
    return false; // Возвращаем рассчитанный результат вызывающему коду.
  }

 if (index >= ds18FoundCount || index >= static_cast<int>(kMaxDs18Sensors)) { // Проверяем условие корректности данных/состояния.
    info = "Индекс " + String(index) + " недоступен. Сначала выполните поиск датчиков."; // Служебная строка логики DS18B20.
    return false; // Возвращаем рассчитанный результат вызывающему коду.
  }

  memcpy(target, ds18Found[index], sizeof(DeviceAddress)); // Копируем выбранный 8-байтовый адрес датчика в целевую переменную.
  if (deviceAddressIsZero(target)) { // Проверяем условие корректности данных/состояния.
    info = "Выбран пустой адрес. Повторите поиск датчиков."; // Служебная строка логики DS18B20.
    return false; // Возвращаем рассчитанный результат вызывающему коду.
  }

  label = formatDeviceAddress(target); // Служебная строка логики DS18B20.
  saveValue<String>(storageKey, label); // Сохраняем значение в энергонезависимую память ESP32 (NVS).
  if (&target == &sensor0) DS1Assigned = true; // Проверяем условие корректности данных/состояния.
  if (&target == &sensor1) DS2Assigned = true; // Проверяем условие корректности данных/состояния.
  info = "Выбран датчик " + String(index) + ": " + label; // Служебная строка логики DS18B20.
  return true; // Возвращаем рассчитанный результат вызывающему коду.
}

void setup_ds18(String &sensor0Label, String &sensor1Label) { // Начало функции инициализации/чтения DS18B20.
  sensors.begin(); // Выполняем действие библиотеки DallasTemperature для DS18B20.

  loadSavedDs18Address(kDs18Sensor0StorageKey, sensor0, sensor0Label); // Загружаем ранее сохранённый адрес датчика из NVS.
  loadSavedDs18Address(kDs18Sensor1StorageKey, sensor1, sensor1Label); // Загружаем ранее сохранённый адрес датчика из NVS.

  DS1Assigned = !deviceAddressIsZero(sensor0); // Служебная строка логики DS18B20.
  DS2Assigned = !deviceAddressIsZero(sensor1); // Служебная строка логики DS18B20.



  if (DS1Assigned) sensors.setResolution(sensor0, 12); // Проверяем условие корректности данных/состояния.
  if (DS2Assigned) sensors.setResolution(sensor1, 12); // Проверяем условие корректности данных/состояния.

  sensors.setWaitForConversion(true); // Выполняем действие библиотеки DallasTemperature для DS18B20.
}

void Temp_DS18B20(int interval_Temp_DS18B20) { // Начало функции инициализации/чтения DS18B20.
  static unsigned long timer; // Служебная строка логики DS18B20.
  if (millis() - timer < interval_Temp_DS18B20) return; // Проверяем условие корректности данных/состояния.
  timer = millis(); // Служебная строка логики DS18B20.

  sensors.requestTemperatures(); // Выполняем действие библиотеки DallasTemperature для DS18B20.

  DS1Assigned = !deviceAddressIsZero(sensor0); // Служебная строка логики DS18B20.
  DS1Available = false; // Служебная строка логики DS18B20.
  if (DS1Assigned) { // Проверяем условие корректности данных/состояния.
    float temp0 = sensors.getTempC(sensor0); // Служебная строка логики DS18B20.
    if (temp0 != DEVICE_DISCONNECTED_C && temp0 > -100 && temp0 < 150) { // Проверяем условие корректности данных/состояния.
      DS1 = roundf(temp0 * 10) / 10.0; // Служебная строка логики DS18B20.
      DS1Available = true; // Служебная строка логики DS18B20.
    } else { // Служебная строка логики DS18B20.
      DS1 = 0.0f; // Служебная строка логики DS18B20.
    }
  } else { // Служебная строка логики DS18B20.
    DS1 = 0.0f; // Служебная строка логики DS18B20.
  }

  DS2Assigned = !deviceAddressIsZero(sensor1); // Служебная строка логики DS18B20.
  DS2Available = false; // Служебная строка логики DS18B20.
  if (DS2Assigned) { // Проверяем условие корректности данных/состояния.
    float temp1 = sensors.getTempC(sensor1); // Служебная строка логики DS18B20.
    if (temp1 != DEVICE_DISCONNECTED_C && temp1 > -100 && temp1 < 150) { // Проверяем условие корректности данных/состояния.
      DS2 = roundf(temp1 * 10) / 10.0; // Служебная строка логики DS18B20.
      DS2Available = true; // Служебная строка логики DS18B20.
    } else { // Служебная строка логики DS18B20.
      DS2 = 0.0f; // Служебная строка логики DS18B20.
    }
  } else { // Служебная строка логики DS18B20.
    DS2 = 0.0f; // Служебная строка логики DS18B20.
  }
}
