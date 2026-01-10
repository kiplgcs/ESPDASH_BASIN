#pragma once // Защита от повторного включения заголовка Zigbee
#include <Arduino.h> // Базовые типы и Serial для ESP32
#include "ui - JeeUI2.h" // Доступ к реестру UI и регистрации обработчиков
// ===================================================== // Блок конфигурации GPIO Zigbee
// ZIGBEE GPIO CONFIGURATION (EXTERNAL CC2652 / CC2538) // Заголовок конфигурации
// // Пустой комментарий для визуального отделения блока
// ВАЖНО: // Важные правила выбора GPIO
// - Указать ТОЛЬКО свободные GPIO // Запрет конфликтов с проектом
// - Не использовать пины USB / JTAG / PSRAM // Запрет критических пинов
// - -1 означает "пин не используется" // Обозначение отключённого пина
// - Эти значения МОЖНО менять без правок логики // Отделение конфигурации от логики
// ===================================================== // Конец блока конфигурации GPIO
// UART Zigbee (ZNP) // Конфигурация UART для Zigbee
inline int8_t ZB_UART_RX_PIN   = -1; // 16; // RX ESP32  ← TX CC2652
inline int8_t ZB_UART_TX_PIN   = -1; // 15; // TX ESP32  → RX CC2652
inline uint32_t ZB_UART_BAUD   = 115200; // Скорость UART Zigbee
// Управляющие пины Zigbee-модуля // Конфигурация управляющих линий
inline int8_t ZB_RESET_PIN     = -1; //21; // RESET CC2652 (активный уровень зависит от модуля)
inline int8_t ZB_BOOT_PIN      = -1; // 22; // BOOT / BSL (опционально, -1 если не используется)
// ВЕСЬ существующий проект должен работать без изменений // Обязательная инструкция: обратная совместимость
// Zigbee реализован через ВНЕШНИЙ модуль CC2652 / CC2538 // Обязательная инструкция: внешний модуль
// ESP32-S3 используется только как управляющий MCU // Обязательная инструкция: MCU управляет модулем
// // Комментарий-пустышка для соблюдения формата
// Для Home Assistant: // Инструкция для пользователя
// 1. Включить Zigbee (zb_enable = 1) // Шаг 1: включение Zigbee
// 2. Нажать "Разрешить подключение" // Шаг 2: permit join
// 3. Добавить устройство в ZHA или Zigbee2MQTT // Шаг 3: добавление в HA
// Варианты UART-подключения CC2652/CC2538 к ESP32-S3 DevKitC-1: // Требование: минимум два варианта GPIO
// Вариант A: выбрать свободные GPIO, не задействованные USB/Wi-Fi/BT/JTAG и текущим проектом // Пояснение выбора
// Например, RX=GPIO16, TX=GPIO17, RESET=GPIO18, BOOT=GPIO19 при условии, что они свободны в проекте // Пример 1 без жёсткого задания
// Вариант B: альтернативные RX=GPIO4, TX=GPIO5, RESET=GPIO6, BOOT=GPIO7 при условии, что они свободны в проекте // Пример 2 без жёсткого задания
// Скорость UART: 115200 бод (типично для прошивки ZNP на CC2652/CC2538) // Требование: указать скорость UART
// RESET удерживаем в активном уровне при отключении Zigbee, BOOT/BSL держим в штатном уровне // Требование: полное выключение
// Внимание: конкретные GPIO НЕ задаются жёстко — задайте их через zigbeeConfigureUart() и zigbeeConfigureControlPins() // Требование: без жёстких GPIO
struct ZigbeeUartConfig { // Конфигурация UART для внешнего Zigbee-модуля
    int8_t rxPin = -1; // GPIO RX, задаётся пользователем, -1 означает "не настроен"
    int8_t txPin = -1; // GPIO TX, задаётся пользователем, -1 означает "не настроен"
    uint32_t baud = 115200; // Скорость UART для ZNP
}; // Конец структуры конфигурации UART
inline ZigbeeUartConfig zigbeeUartConfig; // Глобальная конфигурация UART Zigbee
struct ZigbeeControlPins { // Конфигурация управляющих GPIO для внешнего Zigbee-модуля
    int8_t resetPin = -1; // GPIO RESET, удерживается активным при отключении Zigbee
    int8_t bootPin = -1; // GPIO BOOT/BSL, опциональный, по умолчанию штатный уровень
}; // Конец структуры управляющих GPIO
inline ZigbeeControlPins zigbeeControlPins; // Глобальная конфигурация управляющих GPIO Zigbee
struct ZigbeeSettings { // Структура пользовательских настроек Zigbee из UI
    int enable = 0; // 0 = выключен, 1 = включен
    String role = "coordinator"; // Роль Zigbee: coordinator/router/end_device
    String channel = "auto"; // Канал Zigbee: "auto" или число в строке
    String panId = "auto"; // PAN ID: "auto" или hex строка
    int joined = 0; // Количество подключённых устройств (для UI DISPLAY)
}; // Конец структуры настроек
inline ZigbeeSettings zigbeeSettings; // Глобальные настройки Zigbee
inline bool zigbeeRunning = false; // Флаг запущенного Zigbee стека
inline bool zigbeePermitJoinRequested = false; // Запрос разрешения на подключение
inline bool zigbeeFactoryResetRequested = false; // Запрос сброса сети Zigbee
inline HardwareSerial &zigbeeSerial = Serial2; // Используем аппаратный UART2 для Zigbee
// Переменные для UI (используются напрямую в interface()) // Обязательная связка UI ↔ Zigbee
inline int zb_enable = 0; // UI: включение/выключение Zigbee
inline String zb_role = "coordinator"; // UI: роль Zigbee
inline String zb_channel = "auto"; // UI: канал Zigbee
inline String zb_panid = "auto"; // UI: PAN ID Zigbee
inline bool zb_permit_join = false; // UI: кнопка "Разрешить подключение"
inline bool zb_factory_reset = false; // UI: кнопка "Сброс Zigbee"
inline int zb_joined = 0; // UI: количество устройств в сети Zigbee
class ZigbeeBindingRegistry { // Реестр привязок переменных к Zigbee/HA
public: // Публичный интерфейс реестра привязок
    struct Binding { // Описание одной привязки
        String id; // Идентификатор UI/Zigbee
        void *ptr; // Указатель на переменную
        enum class Type { Int, Float, Bool, String } type; // Тип данных
    }; // Конец структуры Binding
    void add(const String &id, void *ptr, Binding::Type type){ // Добавление привязки
        bindings.push_back({id, ptr, type}); // Сохраняем привязку в список
    } // Конец add
    const std::vector<Binding> &all() const{ return bindings; } // Получение списка всех привязок
private: // Приватные данные реестра привязок
    std::vector<Binding> bindings; // Контейнер привязок
}; // Конец ZigbeeBindingRegistry
inline ZigbeeBindingRegistry zigbeeBindings; // Глобальный реестр привязок Zigbee
template <typename T> // Шаблон для универсальной привязки типов
inline void bind(const char *id, T &value){ // Универсальная привязка переменной к Zigbee/HA по id
    if constexpr (std::is_same<T, int>::value){ // Привязка int
        zigbeeBindings.add(id, &value, ZigbeeBindingRegistry::Binding::Type::Int); // Регистрируем int
    } else if constexpr (std::is_same<T, float>::value){ // Привязка float
        zigbeeBindings.add(id, &value, ZigbeeBindingRegistry::Binding::Type::Float); // Регистрируем float
    } else if constexpr (std::is_same<T, bool>::value){ // Привязка bool
        zigbeeBindings.add(id, &value, ZigbeeBindingRegistry::Binding::Type::Bool); // Регистрируем bool
    } else if constexpr (std::is_same<T, String>::value){ // Привязка String
        zigbeeBindings.add(id, &value, ZigbeeBindingRegistry::Binding::Type::String); // Регистрируем String
    } // Конец ветвлений типов
} // Конец bind
inline void zigbeeConfigureUart(int8_t rxPin, int8_t txPin, uint32_t baud){ // Настройка UART для Zigbee
    zigbeeUartConfig.rxPin = rxPin; // Сохраняем RX GPIO
    zigbeeUartConfig.txPin = txPin; // Сохраняем TX GPIO
    zigbeeUartConfig.baud = baud; // Сохраняем скорость UART
} // Конец zigbeeConfigureUart
inline void zigbeeConfigureControlPins(int8_t resetPin, int8_t bootPin){ // Настройка управляющих GPIO Zigbee
    zigbeeControlPins.resetPin = resetPin; // Сохраняем GPIO RESET
    zigbeeControlPins.bootPin = bootPin; // Сохраняем GPIO BOOT/BSL
    if(zigbeeControlPins.bootPin >= 0){ // Если задан BOOT/BSL
        pinMode(zigbeeControlPins.bootPin, OUTPUT); // Настраиваем BOOT/BSL как выход
        digitalWrite(zigbeeControlPins.bootPin, HIGH); // Устанавливаем штатный уровень BOOT/BSL
    } // Конец настройки BOOT/BSL
} // Конец zigbeeConfigureControlPins
inline void zigbeeApplyPinConfig(){ // Применяем выбранные пользователем GPIO к конфигурации Zigbee
    zigbeeConfigureUart(ZB_UART_RX_PIN, ZB_UART_TX_PIN, ZB_UART_BAUD); // Привязываем UART пины и скорость
    zigbeeConfigureControlPins(ZB_RESET_PIN, ZB_BOOT_PIN); // Привязываем RESET/BOOT пины
} // Конец zigbeeApplyPinConfig
inline void zigbeeAssertReset(){ // Удержание RESET внешнего модуля
    if(zigbeeControlPins.resetPin < 0) return; // Если RESET не задан — выходим
    pinMode(zigbeeControlPins.resetPin, OUTPUT); // Настраиваем RESET как выход
    digitalWrite(zigbeeControlPins.resetPin, LOW); // Удерживаем RESET в активном уровне
} // Конец zigbeeAssertReset
inline void zigbeeReleaseReset(){ // Снятие RESET внешнего модуля
    if(zigbeeControlPins.resetPin < 0) return; // Если RESET не задан — выходим
    pinMode(zigbeeControlPins.resetPin, OUTPUT); // Настраиваем RESET как выход
    digitalWrite(zigbeeControlPins.resetPin, HIGH); // Снимаем RESET, модуль готов к работе
} // Конец zigbeeReleaseReset
inline void zigbeeStartUart(){ // Запуск UART для Zigbee
    if(zigbeeUartConfig.rxPin < 0 || zigbeeUartConfig.txPin < 0) return; // Защита: UART не настроен
    zigbeeSerial.begin(zigbeeUartConfig.baud, SERIAL_8N1, zigbeeUartConfig.rxPin, zigbeeUartConfig.txPin); // Старт UART
} // Конец zigbeeStartUart
inline void zigbeeStopUart(){ // Остановка UART для Zigbee
    zigbeeSerial.end(); // Останавливаем UART для освобождения ресурсов
} // Конец zigbeeStopUart
inline void zigbeeApplySettingsFromUi(){ // Применение настроек из UI к Zigbee
    zigbeeSettings.enable = zb_enable; // Синхронизация включения
    zigbeeSettings.role = zb_role; // Синхронизация роли
    zigbeeSettings.channel = zb_channel; // Синхронизация канала
    zigbeeSettings.panId = zb_panid; // Синхронизация PAN ID
    zigbeeSettings.joined = zb_joined; // Синхронизация счётчика устройств
} // Конец zigbeeApplySettingsFromUi
inline void zigbeeStart(){ // Запуск Zigbee стека через внешний модуль
    if(zigbeeRunning) return; // Если уже запущен — ничего не делаем
    zigbeeApplyPinConfig(); // Применяем GPIO один раз перед запуском UART
    if(zigbeeUartConfig.rxPin < 0 || zigbeeUartConfig.txPin < 0) return; // Защита: UART не настроен, запуск невозможен
    zigbeeReleaseReset(); // Снимаем RESET перед запуском UART
    zigbeeStartUart(); // Запускаем UART
    zigbeeRunning = true; // Отмечаем запуск Zigbee
    // Здесь отправляются ZNP-команды на CC2652/CC2538 для старта координатора/роутера/конечного устройства // Комментарий: точка интеграции ZNP
} // Конец zigbeeStart
inline void zigbeeStop(){ // Остановка Zigbee стека и освобождение ресурсов
    if(!zigbeeRunning) return; // Если уже остановлен — ничего не делаем
    // Здесь отправляются ZNP-команды для остановки сети и выключения радиомодуля // Комментарий: точка интеграции ZNP
    zigbeeStopUart(); // Останавливаем UART и освобождаем ресурсы
    zigbeeAssertReset(); // Удерживаем RESET для полного отключения модуля
    zigbeeRunning = false; // Обновляем флаг состояния Zigbee
} // Конец zigbeeStop
inline void zigbeeApplyEnableState(){ // Применение состояния включения Zigbee
    if(zigbeeSettings.enable != 0){ // Если Zigbee включен
        zigbeeStart(); // Запускаем Zigbee
    } else { // Иначе Zigbee выключен
        zigbeeStopUart(); // Останавливаем UART для освобождения ресурсов
        zigbeeAssertReset(); // Удерживаем RESET для полного отключения модуля
        zigbeeRunning = false; // Сбрасываем флаг состояния Zigbee
    } // Конец ветвления enable
} // Конец zigbeeApplyEnableState
inline void zigbeeRequestPermitJoin(){ // Запрос разрешения подключения новых устройств
    zigbeePermitJoinRequested = true; // Ставим флаг permit join
} // Конец zigbeeRequestPermitJoin
inline void zigbeeRequestFactoryReset(){ // Запрос сброса Zigbee сети
    zigbeeFactoryResetRequested = true; // Ставим флаг сброса
} // Конец zigbeeRequestFactoryReset
inline void zigbeeProcessRequests(){ // Обработка запросов кнопок Zigbee
    if(zigbeePermitJoinRequested){ // Если требуется разрешить подключение
        zigbeePermitJoinRequested = false; // Сбрасываем флаг
        // Здесь отправляется ZNP-команда permit join на CC2652/CC2538 // Комментарий: точка интеграции ZNP
    } // Конец обработки permit join
    if(zigbeeFactoryResetRequested){ // Если требуется сброс сети
        zigbeeFactoryResetRequested = false; // Сбрасываем флаг
        // Здесь отправляется ZNP-команда factory reset на CC2652/CC2538 // Комментарий: точка интеграции ZNP
    } // Конец обработки factory reset
} // Конец zigbeeProcessRequests
inline void zigbeeApplyCurrentSettings(){ // Применение текущих настроек Zigbee
    zigbeeApplySettingsFromUi(); // Считываем настройки из UI
    zigbeeApplyEnableState(); // Включаем или выключаем Zigbee
} // Конец zigbeeApplyCurrentSettings
inline void zigbeeLoop(){ // Периодическая обработка Zigbee
    static bool initialized = false; // Флаг первичной инициализации сравнения
    static ZigbeeSettings lastSettings; // Предыдущее состояние настроек Zigbee
    static bool lastPermitJoin = false; // Предыдущее состояние кнопки permit join
    static bool lastFactoryReset = false; // Предыдущее состояние кнопки factory reset
    if(!initialized){ // Инициализация предыдущих значений при первом запуске
        lastSettings.enable = zb_enable; // Запоминаем текущее значение enable
        lastSettings.role = zb_role; // Запоминаем текущую роль
        lastSettings.channel = zb_channel; // Запоминаем текущий канал
        lastSettings.panId = zb_panid; // Запоминаем текущий PAN ID
        lastSettings.joined = zb_joined; // Запоминаем текущее число устройств
        lastPermitJoin = zb_permit_join; // Запоминаем состояние permit join
        lastFactoryReset = zb_factory_reset; // Запоминаем состояние factory reset
        initialized = true; // Отмечаем завершение инициализации
    } // Конец инициализации
    if(lastSettings.enable != zb_enable){ // Проверяем изменение состояния включения
        zigbeeApplyCurrentSettings(); // Применяем новое состояние включения
        lastSettings.enable = zb_enable; // Обновляем сохранённое значение enable
    } // Конец обработки enable
    if(lastSettings.role != zb_role || lastSettings.channel != zb_channel || lastSettings.panId != zb_panid){ // Проверяем изменения параметров сети
        zigbeeApplySettingsFromUi(); // Обновляем настройки Zigbee из переменных
        if(zigbeeRunning){ // Применяем параметры только при активном Zigbee
            // Здесь отправляются ZNP-команды изменения роли/канала/PAN ID // Комментарий: точка интеграции ZNP
        } // Конец применения параметров сети
        lastSettings.role = zb_role; // Обновляем сохранённую роль
        lastSettings.channel = zb_channel; // Обновляем сохранённый канал
        lastSettings.panId = zb_panid; // Обновляем сохранённый PAN ID
    } // Конец обработки параметров сети
    if(zb_permit_join && !lastPermitJoin){ // Отслеживаем нажатие permit join
        zigbeeRequestPermitJoin(); // Запрашиваем permit join
        zb_permit_join = false; // Сбрасываем кнопку после обработки
    } // Конец обработки permit join
    if(zb_factory_reset && !lastFactoryReset){ // Отслеживаем нажатие factory reset
        zigbeeRequestFactoryReset(); // Запрашиваем factory reset
        zb_factory_reset = false; // Сбрасываем кнопку после обработки
    } // Конец обработки factory reset
    lastPermitJoin = zb_permit_join; // Обновляем предыдущее состояние permit join
    lastFactoryReset = zb_factory_reset; // Обновляем предыдущее состояние factory reset
    zigbeeProcessRequests(); // Обрабатываем запросы кнопок Zigbee
    // Здесь можно читать события ZNP и обновлять zb_joined или другие привязки // Комментарий: точка интеграции событий
} // Конец zigbeeLoop



// ИНСТРУКЦИЯ ПО ДОБАВЛЕНИЮ ПЕРЕМЕННЫХ ДЛЯ ZIGBEE ↔ HOME ASSISTANT // Заголовок инструкции
// ============================================================================ // Разделительная линия инструкции
// // Пустой комментарий для визуального разделения
// Данный файл (Zigbee.h) является ЕДИНСТВЕННЫМ местом, // Правило единой точки
// где добавляются: // Уточнение области применения
// // Пустой комментарий для списка
// 1) Переменные, доступные из Home Assistant // Тип 1 переменных
// 2) Переменные, передающие данные в Home Assistant // Тип 2 переменных
// 3) Привязки UI ↔ Zigbee ↔ HA // Тип 3 привязок
// // Пустой комментарий для раздела правил
// --------------------------------------------------------------------------- // Разделитель общих правил
// ОБЩИЕ ПРАВИЛА // Заголовок раздела
// --------------------------------------------------------------------------- // Разделитель общих правил
// // Пустой комментарий для правил
// ❗ НИКОГДА не добавляйте HA-переменные в: // Запрет на размещение переменных
//    - main.cpp // Запрещённый файл
//    - interface() // Запрещённое место UI
//    - другие .h / .cpp файлы // Запрет на другие файлы
// // Пустой комментарий для запрета механизмов
// ❗ НЕ используйте: // Запрещённые механизмы
//    - if(id == "...") // Запрет id-логики
//    - switch(id) // Запрет switch по id
//    - server.on() // Запрет server.on
//    - UIApplyHandlerRegistry // Запрет legacy-механизма
// // Пустой комментарий для разрешённых принципов
// ❗ ВСЯ логика должна работать через: // Разрешённые основы
//    - обычные переменные // Только переменные
//    - декларативный bind() // Только bind()
// // Пустой комментарий для раздела шага 1
// --------------------------------------------------------------------------- // Разделитель шага 1
// КАК ДОБАВИТЬ НОВУЮ ПЕРЕМЕННУЮ ДЛЯ HOME ASSISTANT // Заголовок шага 1
// --------------------------------------------------------------------------- // Разделитель шага 1
// // Пустой комментарий для шага 1
// ШАГ 1. Объявить переменную ЗДЕСЬ, в Zigbee.h // Инструкция шага 1
// // Пустой комментарий для примеров
// Примеры для ВАШИХ типов переменных: // Примеры из проекта
// // Пустой комментарий перед примерами
// inline bool  ha_filter_enable = false;   // HA: switch  | Фильтрация (Power_Filtr)
// inline int   ha_heat_target   = 28;      // HA: number  | Уставка нагрева (Sider_heat)
// inline float ha_pool_ph       = 7.2f;    // HA: sensor  | PH воды (PH)
// inline String ha_lamp_mode    = "off";   // HA: text    | Режим лампы (SetLamp)
// // Пустой комментарий для рекомендаций
// РЕКОМЕНДАЦИЯ: // Рекомендация по именованию
// - Все HA-переменные начинайте с префикса "ha_" // Префикс для переменных
// // Пустой комментарий для раздела шага 2
// --------------------------------------------------------------------------- // Разделитель шага 2
// ШАГ 2. Зарегистрировать переменную через bind() // Заголовок шага 2
// --------------------------------------------------------------------------- // Разделитель шага 2
// // Пустой комментарий для указаний
// Добавьте привязку в функцию регистрации биндингов: // Инструкция по привязке
// // Пустой комментарий для примера функции
// inline void zigbeeRegisterHaBindings(){ // Пример функции регистрации
// // Пустой комментарий для привязок
//     bind("ha_filter_enable", ha_filter_enable); // Пример bind
//     bind("ha_heat_target",   ha_heat_target); // Пример bind
//     bind("ha_pool_ph",       ha_pool_ph); // Пример bind
//     bind("ha_lamp_mode",     ha_lamp_mode); // Пример bind
// // Пустой комментарий для конца примера
// } // Конец примера функции
// // Пустой комментарий для запрета
// ❗ bind() — это ЕДИНСТВЕННЫЙ допустимый способ связи с HA // Правило bind
// // Пустой комментарий для раздела управления
// --------------------------------------------------------------------------- // Разделитель управления
// КАК РАБОТАЕТ УПРАВЛЕНИЕ // Заголовок управления
// --------------------------------------------------------------------------- // Разделитель управления
// // Пустой комментарий для направления HA → ESP32
// HA → ESP32: // Направление потока данных
// - Home Assistant изменяет атрибут // Шаг HA
// - Zigbee обновляет значение // Шаг Zigbee
// - bind() обновляет переменную // Шаг bind
// - Ваш код просто читает переменную // Шаг кода
// // Пустой комментарий для примера
// Пример: // Пример использования
// // Пустой комментарий перед примером кода
// if(ha_filter_enable){ // Пример проверки
//     // включить фильтрацию // Пример действия
// } // Конец примера
// // Пустой комментарий для направления ESP32 → HA
// --------------------------------------------------------------------------- // Разделитель направления
// ESP32 → HA: // Направление потока данных
// - Код изменяет значение переменной // Шаг кода
// - Zigbee автоматически отправляет обновление // Шаг Zigbee
// - Home Assistant получает новое состояние // Шаг HA
// // Пустой комментарий для примера
// Пример: // Пример обновления
// // Пустой комментарий перед примером
// ha_pool_ph = PH; // Пример передачи значения
// // Пустой комментарий для раздела UI/HA
// --------------------------------------------------------------------------- // Разделитель UI/HA
// UI И HOME ASSISTANT // Заголовок раздела
// --------------------------------------------------------------------------- // Разделитель UI/HA
// // Пустой комментарий для правил UI/HA
// UI и Home Assistant НЕ зависят друг от друга напрямую. // Разделение UI и HA
// // Пустой комментарий для сценариев
// Возможны сценарии: // Перечень сценариев
// - Переменная ТОЛЬКО для HA (без UI) // Сценарий 1
// - Переменная ТОЛЬКО для UI // Сценарий 2
// - Переменная общая (UI + HA) // Сценарий 3
// // Пустой комментарий для общего сценария
// Для общего сценария используется ОДНА переменная: // Принцип общей переменной
// // Пустой комментарий для примера
// inline bool ha_light_enable; // Пример общей переменной
// // Пустой комментарий для примера UI
// UI_BUTTON("light", ha_light_enable, ...); // Пример UI-кнопки
// bind("ha_light_enable", ha_light_enable); // Пример bind для общей переменной
// // Пустой комментарий для раздела важно
// --------------------------------------------------------------------------- // Разделитель важно
// ВАЖНО // Заголовок важного раздела
// --------------------------------------------------------------------------- // Разделитель важно
// // Пустой комментарий для списка важно
// ✔ interface() — ТОЛЬКО описание UI // Правило UI
// ✔ Zigbee.h — логика, переменные, HA // Правило Zigbee.h
// ✔ 1 переменная = 1 источник истины // Правило источника истины
// ✔ Никакой id-логики вне UI // Правило запрета id-логики
// ✔ Проект остаётся полностью обратимо совместимым // Правило совместимости
// // Пустой комментарий для конца инструкции
// ============================================================================ // Конец раздела инструкции
// КОНЕЦ ИНСТРУКЦИИ // Завершение инструкции
// ============================================================================ // Конец инструкции





// ----------------------------------------------------- // Инструкция по прошивке CC2652 / CC2538
// Прошивка CC2652: используйте Zigbee2MQTT coordinator firmware (Z-Stack ZNP) от Koenkk // Источник: Zigbee2MQTT
// - Прямая ссылка на релизы HEX: https://github.com/Koenkk/Z-Stack-firmware/releases // Скачивание .hex
// - Файл прошивки для CC2652 (например, CC2652P): CC2652P2_CC2652P_launchpad_coordinator_*.hex // Пример имени
// Прошивка CC2538: используйте Zigbee2MQTT Z-Stack ZNP firmware для CC2538 // Источник: Zigbee2MQTT
// - Прямая ссылка на релизы HEX: https://github.com/Koenkk/Z-Stack-firmware/releases // Скачивание .hex
// Прошивка через CC Debugger (TI): // Программатор
// 1) Подключите CC Debugger к плате модуля: // Подключения CC Debugger
//    - VCC (3.3V) -> питание модуля // Питание
//    - GND -> GND модуля // Земля
//    - RESET -> RESET модуля (RST) // Сброс
//    - DD (DIO) -> DD (Debug Data) // Линия данных отладчика
//    - DC (CLK) -> DC (Debug Clock) // Линия тактирования отладчика
// 2) Используйте TI SmartRF Flash Programmer 2 / UniFlash для заливки .hex // ПО прошивки
//    - Выберите устройство (CC2652/CC2538) и файл прошивки .hex, затем выполните Program/Flash // Шаг прошивки
// ----------------------------------------------------- // Подключение CC2652/CC2538 к ESP32 для работы
// UART-пины: // Соединения UART
// - TX CC2652/CC2538 -> RX ESP32 (ZB_UART_RX_PIN) // Вход ESP32
// - RX CC2652/CC2538 -> TX ESP32 (ZB_UART_TX_PIN) // Выход ESP32
// Управляющие пины: // Управляющие линии
// - RESET CC2652/CC2538 -> ZB_RESET_PIN (ESP32) // Управление сбросом
// - BOOT/BSL (если есть) -> ZB_BOOT_PIN (ESP32) // Управление загрузчиком, иначе оставить не подключённым
// Общая земля обязательна: GND ESP32 -> GND CC2652/CC2538 // Общая земля
// Точные пины по умолчанию этого проекта: // Значения по умолчанию
// - RX ESP32 = GPIO16, TX ESP32 = GPIO15, RESET = GPIO21, BOOT/BSL = GPIO22 // Соответствие ZB_*_PIN