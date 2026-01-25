//ui - JeeUI2.h
#pragma once // защита от повторного включения заголовка

#include <Arduino.h> // базовые типы Arduino (String, millis и т.д.)
#include <vector> // контейнеры для хранения UI-элементов и меню
#include <functional> // std::function для callback'ов UI
#include <map> // отображение ключ → источник данных (графики)
#include <initializer_list> // списки опций для select
#include <type_traits> // проверки типов в шаблонах
#include "web.h" // backend/UI-инфраструктура (dash, ui, storage)

class OABuilder { // билдер, собирающий декларативное описание UI
public:
      void registerGraphInput(const String &name, float &ref){ graphInputs[name] = &ref; } // регистрирует источник данных для графиков по имени

    void app(const String &name){ // инициализирует UI-приложение
        dashAppTitle = name; // задаёт заголовок приложения
        resetDash(); // полностью сбрасывает предыдущее описание UI
        dashInterfaceInitialized = true; // помечает интерфейс как инициализированный
    }

    void menu(const String &title){ menus.push_back(title); } // добавляет пункт меню для последующих страниц

    void page(){ // создаёт новую страницу (tab) UI
        if(pageIndex >= menus.size()){ // если для страницы нет названия
            menus.push_back(String(F("Page ")) + String(pageIndex + 1)); // создаёт дефолтное имя страницы
        }
        currentTabId = String(F("tab")) + String(pageIndex + 1); // формирует уникальный id вкладки
        dash.addTab(currentTabId, menus[pageIndex]); // регистрирует вкладку в dash
        pageIndex++; // переходит к следующему индексу страницы
    }

    void popupBegin(const String &popupId, const String &title, const String &buttonLabel){ // начинает описание popup-окна
        ensurePage(); // гарантирует наличие активной страницы
        String popupTabId = String(F("popup_")) + popupId; // формирует id popup-вкладки
        dash.addPopup(popupId, title, popupTabId); // регистрирует popup в dash
        addElement("popupButton", String(F("popup_btn_")) + popupId, buttonLabel, popupId); // добавляет кнопку открытия popup
        tabStack.push_back(currentTabId); // сохраняет текущую вкладку в стек
        currentTabId = popupTabId; // переключается на popup как активную вкладку
    }

    void popupEnd(){ // завершает описание popup-окна
        if(tabStack.empty()) return; // если стек пуст — выход
        currentTabId = tabStack.back(); // возвращает предыдущую вкладку
        tabStack.pop_back(); // удаляет её из стека
    }


    void display(const String &id, const String &label, const String &placeholder=""){ // добавляет read-only отображаемое поле
        addElement("display", id, label, placeholder); // регистрирует display-элемент
    }

    void text(const String &id, const String &label){ addElement("text", id, label, ""); } // добавляет текстовое поле ввода

    void text(const String &id, const String &valueOrLabel, const String &extra){ // перегрузка text с поддержкой стиля
        if(isStyleString(extra)) addElement("displayStringAbsolute", id, valueOrLabel, extra); // если extra — стиль, создаёт styled-display
        else addElement("text", id, valueOrLabel, extra); // иначе обычное текстовое поле
    }

    void textarea(const String &id, const String &label, const String &defaultValue=""){ // многострочное текстовое поле
        addElement("textarea", id, label, defaultValue); // регистрирует textarea
    }

    void number(const String &id, const String &label, bool allowFloat=false, const String &defaultValue=""){ // числовое поле
        bool asFloat = allowFloat || containsIgnoreCase(id, F("float")) || containsIgnoreCase(label, F("float")); // определяет float или int
        addElement(asFloat ? "float" : "int", id, label, defaultValue); // добавляет соответствующий числовой элемент
    }

    void time(const String &id, const String &label, const String &defaultValue=""){ // поле выбора времени
        addElement("time", id, label, defaultValue); // регистрирует time-контрол
    }
    
        void timer(const String &id, const String &label,
               const std::function<void(uint16_t, uint16_t)> &callback){ // таймер с интервалами on/off
        ui.registerTimer(id, label, callback); // регистрирует таймер в backend UI
        addElement("timer", id, label, ""); // добавляет таймер в интерфейс
    }

    void color(const String &id, const String &label, const String &defaultValue=""){ // добавляет UI-элемент выбора цвета и связывает его с идентификатором
        addElement("color", id, label, defaultValue); // регистрирует color-контрол в текущей вкладке UI
    }

  void image(const String &id, const String &filename, const String &style=""){ // добавляет изображение с поддержкой позиционирования и inline-стилей
        auto ensureUnit = [](String raw) -> String { // лямбда для нормализации числовых значений CSS (добавляет px при необходимости)
            raw.trim(); // удаляет пробелы в начале и конце строки
            if(!raw.length()) return raw; // если значение пустое — возвращает его без изменений
            for(size_t i = 0; i < raw.length(); i++){ // проверяет каждый символ строки
                char c = raw[i]; // текущий символ
                if(!((c >= '0' && c <= '9') || c == '.' || c == '-')) return raw; // если есть нечисловые символы — считаем, что единицы уже заданы
            }
            return raw + "px"; // если строка — чистое число, добавляет единицы измерения px
        };

        String normalized = style; // копия строки стиля для безопасной модификации
        if(normalized.length() && normalized[normalized.length()-1] != ';') normalized += ';'; // гарантирует наличие ';' для корректного парсинга

        String xRaw, yRaw, rebuilt; // x/y — координаты, rebuilt — пересобранный CSS-стиль
        int start = 0; // начальная позиция парсинга строки стиля
        while(start < normalized.length()){ // проходит по всем CSS-токенам, разделённым ';'
            int end = normalized.indexOf(';', start); // ищет конец текущего токена
            if(end < 0) end = normalized.length(); // если ';' не найден — берёт конец строки
            String token = normalized.substring(start, end); // извлекает один CSS-токен
            token.trim(); // убирает лишние пробелы
            if(token.length()){ // если токен не пустой
                int sep = token.indexOf(':'); // ищет разделитель ключ:значение
                if(sep > 0){
                    String key = token.substring(0, sep); key.trim(); // извлекает имя CSS-свойства
                    String val = token.substring(sep + 1); val.trim(); // извлекает значение CSS-свойства
                    if(key.equalsIgnoreCase("x")) xRaw = ensureUnit(val); // сохраняет X-координату изображения
                    else if(key.equalsIgnoreCase("y")) yRaw = ensureUnit(val); // сохраняет Y-координату изображения
                    else rebuilt += key + ':' + val + ';'; // переносит остальные CSS-свойства без изменений
                } else {
                    rebuilt += token + ';'; // добавляет токен без ключа как есть
                }
            }
            start = end + 1; // переходит к следующему CSS-токену
        }

        if(xRaw.length() || yRaw.length()){ // если задана хотя бы одна координата
            rebuilt += "position:absolute;"; // включает абсолютное позиционирование изображения
            rebuilt += "left:" + (xRaw.length() ? xRaw : String("0px")) + ';'; // устанавливает X-позицию (по умолчанию 0)
            rebuilt += "top:" + (yRaw.length() ? yRaw : String("0px")) + ';'; // устанавливает Y-позицию (по умолчанию 0)
        }

        addElement("image", id, filename, rebuilt); // регистрирует image-элемент с пересобранным стилем
    }


    void checkbox(const String &id, const String &label, const String &defaultValue="0"){ // добавляет чекбокс, связанный с логическим состоянием
        addElement("checkbox", id, label, defaultValue); // регистрирует checkbox-контрол в UI
    }

    void button(const String &id, const String &color, const String &label, const String &layoutCfg=""){ // добавляет кнопку с цветом и конфигурацией
        String cfg = layoutCfg; // копирует конфигурацию layout кнопки
        if(cfg.length() && cfg[cfg.length()-1] != ';') cfg += ';'; // гарантирует корректный формат конфигурации
        if(color.length()) cfg += String(F("color=")) + normalizeColor(color) + ';'; // добавляет цвет кнопки в конфиг
        addElement("button", id, label, cfg); // регистрирует кнопку в UI
    }

        void range(const String &id, float minVal, float maxVal, float step, const String &label, bool dual=false){ // добавляет слайдер или диапазон
        String cfg = String(F("min=")) + String(minVal) + F(";max=") + String(maxVal) + F(";step=") + String(step); // формирует конфигурацию диапазона
        addElement(dual ? "range" : "slider", id, label, cfg); // создаёт одиночный или двойной диапазон
    }

    void selectDays(const String &id, const String &label){ addElement("selectdays", id, label, ""); } // добавляет специализированный выбор дней недели

    void option(const String &value, const String &label){ // добавляет один вариант в список опций select
        if(optionBuffer.length()) optionBuffer += '\n'; // добавляет перевод строки между вариантами
        optionBuffer += value + '=' + label; // сохраняет пару value=label во временном буфере
    }

    void select(const String &id, const String &label){ // завершает формирование select/dropdown
        addElement("dropdown", id, label, optionBuffer); // создаёт dropdown с накопленными опциями
        optionBuffer = ""; // очищает буфер опций для следующего select
    }

    void pub(const String &id, const String &label, const String &unit="", const String &bg="#6060ff", const String &textColor="#ffffff"){ // добавляет publish/индикатор значения
        String cfg = String(F("unit=")) + unit + F(";bg=") + normalizeColor(bg) + F(";fg=") + normalizeColor(textColor); // формирует конфиг отображения
        addElement("pub", id, label, cfg); // регистрирует pub-элемент в UI
    }

    void displayGraph(const String &id, const String &label, const String &config){ // добавляет график с источником данных по ключу
        GraphConfig cfg = parseGraphConfig(config, id); // парсит конфигурацию графика
        addElement("displayGraph", id, label, config); // регистрирует график в UI
        std::function<float()> getter = buildGetter(cfg, nullptr); // строит функцию получения данных по ключу
        registerGraphSource(id, getter, cfg.valueKey, updateInterval, cfg.points); // регистрирует источник данных графика
    }

    void displayGraph(const String &id, const String &label, const String &config, float &source){ // добавляет график с явным источником данных
        GraphConfig cfg = parseGraphConfig(config, id); // парсит конфигурацию графика
        registerGraphInput(cfg.valueKey, source); // регистрирует переменную как источник данных
        addElement("displayGraph", id, label, config); // добавляет график в UI
        std::function<float()> getter = buildGetter(cfg, &source); // строит getter напрямую от переменной
        registerGraphSource(id, getter, cfg.valueKey, updateInterval, cfg.points); // подключает источник к графику
    }

private:
    std::vector<String> menus; // список заголовков меню/страниц
    String currentTabId; // идентификатор текущей активной вкладки
    String optionBuffer; // временный буфер для накопления option у select
    size_t pageIndex = 0; // текущий индекс страницы при генерации UI
    std::map<String, float*> graphInputs; // реестр float-переменных для графиков
        std::vector<String> tabStack; // стек вкладок для возврата после popup

    struct GraphConfig{ // структура параметров графика
        String valueKey; // ключ источника данных
        int points; // количество точек на графике
        unsigned long maxPeriod; // максимальный период истории данных
        unsigned long step; // шаг обновления графика
    };

    void resetDash(){ // полностью очищает текущее описание UI
        dash.tabs.clear(); // удаляет все вкладки
        dash.elements.clear(); // удаляет все элементы UI
                dash.popups.clear(); // удаляет все popup-окна
        menus.clear(); // очищает список меню
        optionBuffer = ""; // сбрасывает буфер опций
        currentTabId = ""; // сбрасывает текущую вкладку
        pageIndex = 0; // сбрасывает счётчик страниц
    }

    bool containsIgnoreCase(String text, const __FlashStringHelper *needle){ // проверяет, содержит ли строка подстроку без учёта регистра
        text.toLowerCase(); // приводит исходный текст к нижнему регистру для регистронезависимого сравнения
        String token(needle); // копирует строку из flash-памяти в обычный String
        token.toLowerCase(); // приводит искомую подстроку к нижнему регистру
        return text.indexOf(token) >= 0; // возвращает true, если подстрока найдена
    }

    bool isStyleString(const String &text) const{ // определяет, является ли строка CSS-стилем, а не текстовым значением
        if(!text.length()) return false; // пустая строка не может быть стилем
        const char* markers[] = {"x:", "y:", "font", "color", "left:", "top:"}; // маркеры, характерные для style-строк
        for(const char* marker : markers){ // перебирает все известные маркеры стиля
            if(text.indexOf(marker) >= 0) return true; // если найден любой маркер — строка считается стилем
        }
        return false; // если ни один маркер не найден — это не style-строка
    }

    String normalizeColor(String color){ // нормализует цветовое значение к CSS-совместимому виду
        String trimmed = color; // создаёт копию строки цвета
        trimmed.trim(); // удаляет пробелы в начале и конце
        if(!trimmed.startsWith("#")){ // если цвет не задан в hex-формате
            if(trimmed.length()==3 || trimmed.length()==6){ // поддержка короткого и полного hex без #
                trimmed = "#" + trimmed; // добавляет символ # перед цветом
            }
        }
        return trimmed; // возвращает нормализованную строку цвета
    }

    void ensurePage(){ if(currentTabId.length()==0) page(); } // гарантирует наличие активной страницы перед добавлением элементов

    void addElement(const String &type, const String &id, const String &label, const String &value){ // добавляет UI-элемент в текущую вкладку
        ensurePage(); // автоматически создаёт страницу, если она ещё не была создана
        Element element{type, id, label, value, ""}; // формирует структуру элемента UI
        dash.addElement(currentTabId, element); // регистрирует элемент в dash для текущей вкладки
    }

  GraphConfig parseGraphConfig(const String &config, const String &fallbackId){ // разбирает строку конфигурации графика
        int defaultPoints = maxPoints > 0 ? maxPoints : 30; // количество точек по умолчанию или глобальное ограничение
        unsigned long defaultPeriod = 600000; // 10 минут по умолчанию
        unsigned long defaultStep = 1000;     // 1 секунда по умолчанию
        GraphConfig cfg{fallbackId, defaultPoints, defaultPeriod, defaultStep}; // инициализирует конфигурацию начальными значениями
        int start = 0; // позиция начала парсинга строки
        while(start < config.length()){ // проходит по всей строке конфигурации
            int end = config.indexOf(';', start); // ищет конец текущего параметра
            if(end < 0) end = config.length(); // если ; не найден — берёт конец строки
            String token = config.substring(start, end); // извлекает один параметр конфигурации
            int sep = token.indexOf(':'); // ищет разделитель ключ:значение
            if(sep > 0){
                String key = token.substring(0, sep); // извлекает имя параметра
                String val = token.substring(sep + 1); // извлекает значение параметра
                key.trim(); val.trim(); // очищает ключ и значение от пробелов
                if(key.equalsIgnoreCase("value")) cfg.valueKey = val; // задаёт ключ источника данных
                else if(key.equalsIgnoreCase("maxPoints")) cfg.points = val.toInt(); // переопределяет количество точек
                else if(key.equalsIgnoreCase("updatePeriod_of_Time")) cfg.maxPeriod = val.toInt(); // задаёт период хранения данных
                else if(key.equalsIgnoreCase("updateStep")) cfg.step = val.toInt(); // задаёт шаг обновления графика
            }
            start = end + 1; // переходит к следующему параметру
        }
        return cfg; // возвращает заполненную конфигурацию графика
    }

    std::function<float()> buildGetter(const GraphConfig &cfg, float *preferred){ // строит функцию получения значения для графика
        float *source = preferred; // приоритетный источник данных, если он передан явно
        if(!source){
            for(auto &entry : graphInputs){ // ищет источник по ключу среди зарегистрированных входов
                if(entry.first.equalsIgnoreCase(cfg.valueKey)){
                    source = entry.second; // находит подходящую переменную-источник
                    break;
                }
            }
        }

        if(!source){ // если источник всё ещё не найден
            if(cfg.valueKey.equalsIgnoreCase("Speed")) source = &Speed; // fallback на глобальную переменную Speed
            else if(cfg.valueKey.equalsIgnoreCase("Temperatura")) source = &Temperatura; // fallback на глобальную Temperatura
        }

        if(source) return [source](){ return *source; }; // возвращает лямбду, читающую значение напрямую из переменной
        return [](){ return 0.0f; }; // если источник не найден — возвращает 0 как безопасное значение
    }
};

inline OABuilder oab; // глобальный экземпляр билдера для декларативного описания UI

struct UIOption { // структура одного варианта выбора для select
    const char *value; // значение, передаваемое в backend
    const char *label; // отображаемый текст в UI
};

inline bool uiIsStyleString(const String &text){ // глобальная версия проверки style-строки
    if(!text.length()) return false; // пустая строка не может быть стилем
    const char* markers[] = {"x:", "y:", "font", "color", "left:", "top:"}; // маркеры CSS-позиционирования и оформления
    for(const char* marker : markers){ // перебирает все маркеры
        if(text.indexOf(marker) >= 0) return true; // если найден маркер — это style-строка
    }
    return false; // иначе обычный текст
}

class UIDeclarativeElement { // базовый класс для всех декларативных UI-элементов
public:
    UIDeclarativeElement(const String &elementId, const String &elementLabel)
        : id(elementId), label(elementLabel) {} // сохраняет идентификатор и подпись элемента
    virtual ~UIDeclarativeElement() = default; // виртуальный деструктор для корректного удаления

    const String id; // уникальный идентификатор UI-элемента
    const String label; // подпись или текст элемента
    bool registered = false; // флаг регистрации элемента в UI-реестре

    virtual void build(OABuilder &builder) = 0; // добавляет элемент в UI через билдер
    virtual void load() = 0; // загружает сохранённое состояние элемента
    virtual void save() const = 0; // сохраняет текущее состояние элемента
    virtual String valueString() const = 0; // возвращает значение элемента в строковом виде
    virtual void setFromString(const String &value) = 0; // обновляет состояние элемента из строки
};

// СКРЫТЫЙ ЭЛЕМЕНТ ДЛЯ СОХРАНЕНИЯ И ВОССТАНОВЛЕНИЯ ДАННЫХ ИЗ EEPROM / NVS
template <typename T>
class UIHiddenElement : public UIDeclarativeElement {
public:
    // elementId — имя (ключ) в EEPROM/NVS
    // storageRef — переменная в RAM, которую нужно сохранять
    UIHiddenElement(const String &elementId, T &storageRef)
        : UIDeclarativeElement(elementId, String()), storage(storageRef) {}

    // В UI не отображается
    void build(OABuilder &builder) override { (void)builder; }

    // ===== ЧТЕНИЕ ИЗ EEPROM / NVS =====
    void load() override {
        // 1. Берём ключ id (строка)
        // 2. Читаем сохранённое значение из памяти
        // 3. Если записи нет — берётся текущее значение переменной (по умолчанию)
        // 4. Записываем результат в переменную storage

        if constexpr (std::is_same<T, bool>::value) {
            // bool читаем как int (0 или 1)
            storage = loadValue<int>(id.c_str(), storage ? 1 : 0) != 0;
        } else {
            // остальные типы читаем напрямую
            storage = loadValue<T>(id.c_str(), storage);
        }
    }

    // ===== ЗАПИСЬ В EEPROM / NVS =====
    void save() const override {
        // 1. Берём ключ id (строка)
        // 2. Берём текущее значение переменной storage
        // 3. Записываем значение в энергонезависимую память

        if constexpr (std::is_same<T, bool>::value) {
            // bool сохраняем как 0 или 1
            saveValue<int>(id.c_str(), storage ? 1 : 0);
        } else {
            // остальные типы сохраняем напрямую
            saveValue<T>(id.c_str(), storage);
        }
    }

    // Преобразование значения в строку (для логов / UI)
    String valueString() const override { return toString(storage); }

    // Установка значения из строки (например из Web UI)
    void setFromString(const String &value) override {
        storage = fromString(value);
    }

private:
    T &storage; // ссылка на переменную в RAM

    static String toString(const String &v) { return v; }
    static String toString(const char *v)   { return String(v); }
    static String toString(int v)           { return String(v); }
    static String toString(float v)         { return String(v); }
    static String toString(bool v)          { return v ? "1" : "0"; }

    static T fromString(const String &v) {
        if constexpr (std::is_same<T, String>::value) return v;
        else if constexpr (std::is_same<T, bool>::value) return v.toInt() != 0;
        else if constexpr (std::is_same<T, float>::value) return v.toFloat();
        else return static_cast<T>(v.toInt());
    }
};



class UIDeclarativeRegistry { // реестр всех декларативных UI-элементов
public:
    void registerElement(UIDeclarativeElement *element){ // регистрирует элемент, если он ещё не зарегистрирован
        if(!element) return; // защита от nullptr
        for(auto *existing : elements){ // проверяет, не был ли элемент уже зарегистрирован
            if(existing && existing->id == element->id){
                element->registered = true; // помечает элемент как зарегистрированный
                return;
            }
        }
        element->load(); // загружает сохранённое состояние элемента
        element->registered = true; // помечает элемент как зарегистрированный
        elements.push_back(element); // добавляет элемент в реестр
    }

    UIDeclarativeElement *find(const String &id) const{ // ищет элемент по идентификатору
        for(auto *element : elements){
            if(element && element->id == id) return element; // возвращает найденный элемент
        }
        return nullptr; // если элемент не найден
    }

    bool applyValue(const String &id, const String &value){ // применяет значение к элементу по id
        UIDeclarativeElement *element = find(id); // ищет элемент в реестре
        if(!element) return false; // если элемент не найден — операция не выполнена
        element->setFromString(value); // обновляет состояние элемента
        element->save(); // сохраняет новое состояние
        return true; // подтверждает успешное применение значения
    }

    const std::vector<UIDeclarativeElement*> &all() const{ return elements; } // возвращает список всех зарегистрированных элементов

private:
    std::vector<UIDeclarativeElement*> elements; // контейнер всех зарегистрированных UI-элементов
};

inline UIDeclarativeRegistry uiRegistry; // глобальный реестр декларативных UI-элементов

inline void appendUiRegistryValues(JsonDocument &doc){
    for(auto *element : uiRegistry.all()){
        if(!element) continue;
        if(doc.containsKey(element->id)) continue;
        doc[element->id] = element->valueString();
    }
}

class UICheckboxElement : public UIDeclarativeElement { // UI-элемент чекбокса, связанный с bool-переменной
public:
    UICheckboxElement(const String &elementId, bool &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {} // связывает чекбокс с переменной состояния

    void build(OABuilder &builder) override{ // добавляет чекбокс в UI
        builder.checkbox(id, label, storage ? "1" : "0"); // передаёт начальное состояние чекбокса
    }

    void load() override{ // загружает сохранённое состояние чекбокса из постоянного хранилища
        storage = loadValue<int>(id.c_str(), storage ? 1 : 0) != 0; // читает int и преобразует его в bool-состояние
    }

    void save() const override{ // сохраняет текущее состояние чекбокса
        saveValue<int>(id.c_str(), storage ? 1 : 0); // записывает bool как 0/1 для совместимости с хранилищем
    }

    String valueString() const override{ return resolveUiValueOverride(id, storage ? "1" : "0"); } // результат чекбокса с возможным переопределением

    void setFromString(const String &value) override{ // обновляет состояние чекбокса из строкового значения UI
        storage = value.toInt() != 0; // интерпретирует любое ненулевое значение как true
    }

private:
    bool &storage; // ссылка на внешнюю переменную состояния, связанную с чекбоксом
};

template <typename T>
class UIButtonElement : public UIDeclarativeElement { // универсальный UI-элемент кнопки для bool/int состояний
public:
    UIButtonElement(const String &elementId, T &storageRef, const String &elementColor, const String &elementLabel, int defaultState)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), color(elementColor),
          defaultValue(defaultState) {} // связывает кнопку с переменной состояния и значением по умолчанию

    void build(OABuilder &builder) override{ // добавляет кнопку в UI
        builder.button(id, color, label); // регистрирует кнопку с заданным цветом и подписью
    }

    void load() override{ // загружает сохранённое состояние кнопки
        int stored = loadButtonState(id.c_str(), defaultValue); // читает сохранённое состояние или default
        storage = fromInt(stored); // преобразует int в целевой тип T
    }

    void save() const override{ // сохраняет текущее состояние кнопки
        saveButtonState(id.c_str(), asInt(storage)); // приводит состояние к int и записывает в хранилище
    }

    String valueString() const override{ return resolveUiValueOverride(id, String(asInt(storage))); } // состояние кнопки с учётом провайдера

    void setFromString(const String &value) override{ // применяет значение, пришедшее из UI
        storage = fromInt(value.toInt()); // преобразует строку в int, затем в целевой тип
    }

private:
    T &storage; // ссылка на переменную состояния, управляемую кнопкой
    String color; // цвет кнопки в UI
    int defaultValue; // значение по умолчанию при отсутствии сохранённых данных

    static int asInt(int value){ return value; } // passthrough для int-значений
    static int asInt(bool value){ return value ? 1 : 0; } // преобразует bool в 0/1 для хранения

    static T fromInt(int value){ // преобразует сохранённое int-значение в тип T
        if constexpr (std::is_same<T, bool>::value) return value != 0; // для bool — любое ненулевое true
        else return static_cast<T>(value); // для остальных типов — прямое приведение
    }
};

template <typename T>
class UINumberElement : public UIDeclarativeElement { // числовой UI-элемент для int/float значений
public:
    UINumberElement(const String &elementId, T &storageRef, const String &elementLabel, bool allowFloat)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), asFloat(allowFloat) {} // связывает элемент с переменной и режимом float/int

    void build(OABuilder &builder) override{ // добавляет числовое поле ввода в UI
        builder.number(id, label, asFloat, String(storage)); // передаёт текущее значение как начальное
    }

    void load() override{ // загружает сохранённое числовое значение
        storage = loadValue<T>(id.c_str(), storage); // читает значение из хранилища с fallback
    }

    void save() const override{ // сохраняет текущее числовое значение
        saveValue<T>(id.c_str(), storage); // записывает значение в постоянное хранилище
    }

    String valueString() const override{ return resolveUiValueOverride(id, String(storage)); } // число с возможным форматором

    void setFromString(const String &value) override{ // применяет значение из UI
        if(asFloat) storage = static_cast<T>(value.toFloat()); // для float — парсит как float
        else storage = static_cast<T>(value.toInt()); // для int — парсит как int
    }

private:
    T &storage; // ссылка на числовую переменную состояния
    bool asFloat; // флаг, определяющий режим float или int
};

template <typename T>
class UIRangeElement : public UIDeclarativeElement { // UI-элемент диапазона (slider/range)
public:
    UIRangeElement(const String &elementId, T &storageRef, int minVal, int maxVal, float stepVal,
                   const String &elementLabel, bool dual, const std::function<void(const T &)> &cb)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), minValue(minVal), maxValue(maxVal),
          step(stepVal), dualRange(dual), callback(cb) {} // инициализирует диапазон и callback

    void build(OABuilder &builder) override{ // добавляет range/slider в UI
        builder.range(id, minValue, maxValue, step, label, dualRange); // регистрирует диапазон с параметрами
    }

    void load() override{ // загружает сохранённое значение диапазона
        storage = loadValue<T>(id.c_str(), storage); // читает значение из хранилища
        clamp(); // ограничивает значение допустимыми пределами
        notify(); // уведомляет callback об обновлении
    }

    void save() const override{ // сохраняет текущее значение диапазона
        saveValue<T>(id.c_str(), storage); // записывает значение в хранилище
    }

    String valueString() const override{ return resolveUiValueOverride(id, String(storage)); } // значение диапазона с возможным переопределением

    void setFromString(const String &value) override{ // применяет значение, полученное из UI
        storage = static_cast<T>(value.toFloat()); // парсит значение как float (универсально)
        clamp(); // ограничивает диапазоном
        notify(); // вызывает callback
    }

private:
    T &storage; // ссылка на управляемую переменную
    T minValue; // минимально допустимое значение
    T maxValue; // максимально допустимое значение
    float step; // шаг изменения значения
    bool dualRange; // флаг одиночного или двойного диапазона
    std::function<void(const T &)> callback; // callback при изменении значения

    void clamp(){ // приводит значение в допустимые границы
        if(storage < static_cast<T>(minValue)) storage = static_cast<T>(minValue); // защита от выхода ниже минимума
        if(storage > static_cast<T>(maxValue)) storage = static_cast<T>(maxValue); // защита от выхода выше максимума
    }

    void notify(){ // уведомляет подписчика об изменении значения
        if(callback) callback(storage); // вызывает callback, если он задан
    }
};

template <typename T>
class UIDualRangeElement : public UIDeclarativeElement { // двойной диапазон с независимыми min/max значениями
public:
      UIDualRangeElement(const String &elementId, T &minRef, T &maxRef, const String &minKey, const String &maxKey, T minVal, T maxVal, float stepVal, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), minStorage(minRef), maxStorage(maxRef),
          minStorageKey(minKey), maxStorageKey(maxKey), minValue(minVal), maxValue(maxVal), step(stepVal) {} // связывает диапазон с двумя переменными состояния

    void build(OABuilder &builder) override{ // добавляет двойной range в UI
        builder.range(id, static_cast<float>(minValue), static_cast<float>(maxValue), step, label, true); // всегда dual=true
    }

    void load() override{ // загружает сохранённые значения min/max
        minStorage = loadValue<T>(minStorageKey.c_str(), minStorage); // читает минимальное значение
        maxStorage = loadValue<T>(maxStorageKey.c_str(), maxStorage); // читает максимальное значение
        clamp(); // приводит значения в допустимые границы
    }

    void save() const override{ // сохраняет значения min/max
        saveValue<T>(minStorageKey.c_str(), minStorage); // сохраняет минимальное значение
        saveValue<T>(maxStorageKey.c_str(), maxStorage); // сохраняет максимальное значение
    }

    String valueString() const override{ return resolveUiValueOverride(id, String(minStorage) + '-' + String(maxStorage)); } // диапазон min-max с учётом провайдера

    void setFromString(const String &value) override{ // применяет строку диапазона из UI
        int sep = value.indexOf('-'); // ищет разделитель значений
        if(sep < 0) return; // если формат неверный — игнорирует
        minStorage = parseValue(value.substring(0, sep)); // парсит минимальное значение
        maxStorage = parseValue(value.substring(sep + 1)); // парсит максимальное значение
        clamp(); // нормализует диапазон
    }

private:
    T &minStorage; // ссылка на минимальное значение
    T &maxStorage; // ссылка на максимальное значение
    String minStorageKey; // ключ хранения минимального значения
    String maxStorageKey; // ключ хранения максимального значения
    int minValue; // допустимый минимум диапазона
    int maxValue; // допустимый максимум диапазона
    float step; // шаг изменения значений

    void clamp(){ // приводит min/max в корректное состояние
        if(minStorage < minValue) minStorage = minValue; // ограничивает минимум
        if(minStorage > maxValue) minStorage = maxValue; // ограничивает минимум сверху
        if(maxStorage < minValue) maxStorage = minValue; // ограничивает максимум снизу
        if(maxStorage > maxValue) maxStorage = maxValue; // ограничивает максимум сверху
        if(minStorage > maxStorage){ // гарантирует корректный порядок значений
            int temp = minStorage;
            minStorage = maxStorage;
            maxStorage = temp;
        }
    }
    
    static T parseValue(const String &value){ // универсальный парсер строкового значения
        if constexpr(std::is_floating_point<T>::value){
            return static_cast<T>(value.toFloat()); // для float — парсит как float
        }
        return static_cast<T>(value.toInt()); // для целых типов — парсит как int
    }
};

class UITextElement : public UIDeclarativeElement { // текстовый UI-элемент, связанный со строковой переменной
public:
    UITextElement(const String &elementId, String &storageRef, const String &elementLabelOrStyle)
        : UIDeclarativeElement(elementId, elementLabelOrStyle), storage(storageRef) {} // связывает текстовый элемент со строкой состояния

    void build(OABuilder &builder) override{ // добавляет текстовый элемент в UI
        if(uiIsStyleString(label)) builder.text(id, storage, label); // если label — стиль, отображает значение со стилем
        else builder.text(id, label, storage); // иначе обычное текстовое поле ввода
    }

    void load() override{ // загружает сохранённое текстовое значение
        storage = loadValue<String>(id.c_str(), storage); // читает строку из хранилища
    }

    void save() const override{ // сохраняет текущее текстовое значение
        saveValue<String>(id.c_str(), storage); // записывает строку в хранилище
    }

    String valueString() const override{ return resolveUiValueOverride(id, storage); } // текст с возможным прокси-генератором

    void setFromString(const String &value) override{ storage = value; } // обновляет строковое состояние из UI

private:
    String &storage; // ссылка на строковую переменную состояния
};

class UIDisplayElement : public UIDeclarativeElement { // UI-элемент только для отображения строкового значения без редактирования
public:
    UIDisplayElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {} // связывает display-элемент с внешней строковой переменной

    void build(OABuilder &builder) override{ builder.display(id, label, storage); } // добавляет отображаемое поле в UI с текущим значением
    void load() override{ storage = loadValue<String>(id.c_str(), storage); } // загружает отображаемое значение из постоянного хранилища
    void save() const override{ saveValue<String>(id.c_str(), storage); } // сохраняет значение, если оно было обновлено программно
    String valueString() const override{ return resolveUiValueOverride(id, storage); } // отображение строки с возможным переопределением
    void setFromString(const String &value) override{ storage = value; } // позволяет обновить отображаемое значение извне

private:
    String &storage; // ссылка на отображаемое строковое состояние
};

class UIDisplayIntElement : public UIDeclarativeElement { // display-элемент для целочисленных значений
public:
    UIDisplayIntElement(const String &elementId, int &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {} // связывает UI с целочисленной переменной

    void build(OABuilder &builder) override{ builder.display(id, label, String(storage)); } // отображает текущее int-значение
    void load() override{ storage = loadValue<int>(id.c_str(), storage); } // загружает сохранённое значение
    void save() const override{ saveValue<int>(id.c_str(), storage); } // сохраняет целочисленное значение
    String valueString() const override{ return resolveUiValueOverride(id, String(storage)); } // int-значение с возможной подменой
    void setFromString(const String &value) override{ storage = value.toInt(); } // обновляет значение из UI

private:
    int &storage; // ссылка на целочисленную переменную состояния
};

class UIDisplayFloatElement : public UIDeclarativeElement { // display-элемент для вещественных значений
public:
    UIDisplayFloatElement(const String &elementId, float &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {} // связывает UI с float-переменной

    void build(OABuilder &builder) override{ builder.display(id, label, String(storage)); } // отображает текущее float-значение
    void load() override{ storage = loadValue<float>(id.c_str(), storage); } // загружает сохранённое значение
    void save() const override{ saveValue<float>(id.c_str(), storage); } // сохраняет float-значение
    String valueString() const override{ return resolveUiValueOverride(id, String(storage)); } // float-значение с возможным форматированием
    void setFromString(const String &value) override{ storage = value.toFloat(); } // обновляет значение из строкового представления

private:
    float &storage; // ссылка на вещественную переменную состояния
};

class UIDisplayBoolElement : public UIDeclarativeElement { // display-элемент для bool с текстовыми состояниями
public:
    UIDisplayBoolElement(const String &elementId, bool &storageRef, const String &elementLabel,
                         const String &onLabel, const String &offLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef),
          onText(onLabel), offText(offLabel) {} // задаёт текст для true/false состояний

    void build(OABuilder &builder) override{ builder.display(id, label, valueString()); } // отображает текстовое состояние вместо 0/1
    void load() override{ storage = loadValue<int>(id.c_str(), storage ? 1 : 0) != 0; } // загружает bool из сохранённого int
    void save() const override{ saveValue<int>(id.c_str(), storage ? 1 : 0); } // сохраняет bool как 0/1
    String valueString() const override{ return resolveUiValueOverride(id, storage ? onText : offText); } // bool-лента с кастомным текстом
    void setFromString(const String &value) override{
        storage = value.toInt() != 0; // позволяет управлять состоянием через UI
    }

private:
    bool &storage; // ссылка на логическую переменную
    String onText; // текст для состояния true
    String offText; // текст для состояния false
};

class UITimeElement : public UIDeclarativeElement { // UI-элемент для выбора времени (HH:MM)
public:
    UITimeElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {} // связывает time-контрол со строкой времени

    void build(OABuilder &builder) override{ builder.time(id, label, storage); } // добавляет time picker в UI
    void load() override{ storage = loadValue<String>(id.c_str(), storage); } // загружает сохранённое время
    void save() const override{ saveValue<String>(id.c_str(), storage); } // сохраняет выбранное время
    String valueString() const override{ return resolveUiValueOverride(id, storage); } // время, проходящее через провайдер
    void setFromString(const String &value) override{ storage = value; } // обновляет время из UI

private:
    String &storage; // строка, содержащая время
};

class UITimerElement : public UIDeclarativeElement { // UI-элемент таймера с интервалами включения/выключения
public:
    UITimerElement(const String &elementId, const String &elementLabel, int *onRef, int *offRef,
                   const std::function<void(uint16_t, uint16_t)> &cb = nullptr)
        : UIDeclarativeElement(elementId, elementLabel), callback(cb), onStorage(onRef), offStorage(offRef) {} // callback вызывается при изменении таймера и можно привязать внешние переменные

    void build(OABuilder &builder) override{
        builder.timer(id, label, callback); // регистрирует таймер в UI и backend
    }

    void load() override{ // при регистрации таймеров синхронизируем внешние значения с backend
        UITimerEntry &entry = ui.timer(id); // обеспечивает существование таймера и его сохранённого состояния
        if(onStorage) *onStorage = entry.on; // обновляем значение включения в минутах
        if(offStorage) *offStorage = entry.off; // обновляем значение отключения
    }
    void save() const override{ // сохраняем значения из backend в привязанные переменные
        UITimerEntry &entry = ui.timer(id);
        if(onStorage) *onStorage = entry.on;
        if(offStorage) *offStorage = entry.off;
    }
    String valueString() const override{ return resolveUiValueOverride(id, String(ui.timer(id).on) + '-' + String(ui.timer(id).off)); } // интервалы таймера с опциональным провайдером
    void setFromString(const String &value) override{
        int sep = value.indexOf('-'); // ищет разделитель интервалов
        if(sep < 0) return; // игнорирует неверный формат
        uint16_t onMinutes = static_cast<uint16_t>(value.substring(0, sep).toInt()); // парсит время включения
        uint16_t offMinutes = static_cast<uint16_t>(value.substring(sep + 1).toInt()); // парсит время выключения
        ui.setTimerMinutes(id, onMinutes, offMinutes, true); // применяет таймер и активирует его
        if(onStorage) *onStorage = onMinutes; // синхронизируем внешние переменные
        if(offStorage) *offStorage = offMinutes;
    }

private:
    std::function<void(uint16_t, uint16_t)> callback; // пользовательский обработчик изменения таймера
    int *onStorage; // ссылка на внешнюю переменную времени включения (в минутах)
    int *offStorage; // ссылка на внешнюю переменную времени отключения
};

template <typename T>
class UISelectElement : public UIDeclarativeElement { // универсальный UI-элемент выпадающего списка (select) для разных типов данных
public:
    UISelectElement(const String &elementId, T &storageRef, const String &elementLabel,
                    std::initializer_list<UIOption> optionsList,
                    const std::function<void(const T &)> &cb)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), options(optionsList), callback(cb) {} // связывает select с переменной, набором опций и callback

    void build(OABuilder &builder) override{ // формирует select-элемент в UI
        for(const auto &opt : options){ builder.option(opt.value, opt.label); } // добавляет все варианты выбора в буфер билдера
        builder.select(id, label); // завершает формирование выпадающего списка
    }

    void load() override{ // загружает сохранённое значение select
        if constexpr (std::is_same<T, bool>::value){ // специальная обработка для bool
            storage = loadValue<int>(id.c_str(), storage ? 1 : 0) != 0; // читает bool как 0/1 из хранилища
        } else {
            storage = loadValue<T>(id.c_str(), storage); // читает значение напрямую для остальных типов
        }
        notify(); // уведомляет callback о восстановленном значении
    }
    void save() const override{ // сохраняет текущее выбранное значение
        if constexpr (std::is_same<T, bool>::value){ // bool сохраняется как int
            saveValue<int>(id.c_str(), storage ? 1 : 0); // запись bool в виде 0/1
        } else {
            saveValue<T>(id.c_str(), storage); // запись значения без преобразований
        }
    }
    String valueString() const override{ return resolveUiValueOverride(id, toString(storage)); } // select с возможной кастомной строкой

    void setFromString(const String &value) override{ // применяется значение, выбранное пользователем в UI
        storage = fromString(value); // преобразует строку в тип T
        notify(); // вызывает callback при изменении значения
    }

private:
    T &storage; // ссылка на переменную, в которой хранится выбранное значение
    std::vector<UIOption> options; // список доступных вариантов выбора
    std::function<void(const T &)> callback; // callback, вызываемый при изменении значения

    void notify(){ // уведомляет внешнюю логику о смене значения
        if(callback) callback(storage); // вызывает callback, если он задан
    }

    static String toString(const String &value){ return value; } // преобразование String → String
    static String toString(const char *value){ return String(value); } // преобразование C-строки → String
    static String toString(int value){ return String(value); } // преобразование int → String
    static String toString(bool value){ return value ? "1" : "0"; } // bool отображается как 1/0
    static String toString(float value){ return String(value); } // float → String

    static T fromString(const String &value){ // универсальный парсер значения из строки
        if constexpr (std::is_same<T, String>::value) return value; // для String возвращает значение напрямую
        else if constexpr (std::is_same<T, bool>::value) return value.toInt() != 0; // bool определяется как 0/1
        else if constexpr (std::is_same<T, float>::value) return value.toFloat(); // float парсится как float
        else return static_cast<T>(value.toInt()); // остальные типы парсятся как int
    }
};

class UISelectDaysElement : public UIDeclarativeElement { // специализированный UI-элемент выбора дней недели
public:
    UISelectDaysElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {} // связывает выбор дней со строкой состояния

    void build(OABuilder &builder) override{ builder.selectDays(id, label); } // добавляет UI-контрол выбора дней
    void load() override{
        storage = loadValue<String>(id.c_str(), storage); // загружает сохранённую строку выбранных дней
        syncCleanDaysFromSelection(); // синхронизирует внутренние флаги дней с выбранным значением
    }
    void save() const override{ saveValue<String>(id.c_str(), storage); } // сохраняет выбранные дни
    String valueString() const override{ return resolveUiValueOverride(id, storage); } // дни недели с возможным переопределением
    void setFromString(const String &value) override{
        storage = value; // обновляет выбранные дни из UI
        syncCleanDaysFromSelection(); // обновляет внутреннюю логику после изменения
    }

private:
    String &storage; // строка, кодирующая выбранные дни недели
};

class UIColorElement : public UIDeclarativeElement { // UI-элемент выбора цвета
public:
    UIColorElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {} // связывает color picker со строкой цвета

    void build(OABuilder &builder) override{ builder.color(id, label, storage); } // добавляет color picker в UI
    void load() override{ storage = loadValue<String>(id.c_str(), storage); } // загружает сохранённый цвет
    void save() const override{ saveValue<String>(id.c_str(), storage); } // сохраняет выбранный цвет
    String valueString() const override{ return resolveUiValueOverride(id, storage); } // цвет с учётом провайдера
    void setFromString(const String &value) override{ storage = value; } // обновляет цвет из UI

private:
    String &storage; // строка с текущим цветом (HEX или CSS)
};

class UIImageElement : public UIDeclarativeElement { // UI-элемент для отображения изображения
public:
    UIImageElement(const String &elementId, const String &filename, const String &style)
        : UIDeclarativeElement(elementId, filename), imageStyle(style) {} // filename используется как label для image

    void build(OABuilder &builder) override{ builder.image(id, label, imageStyle); } // добавляет изображение с заданным стилем
    void load() override{} // изображение не имеет состояния для загрузки
    void save() const override{} // изображение не сохраняет состояние
    String valueString() const override{ return label; } // возвращает имя файла изображения
    void setFromString(const String &value) override{ (void)value; } // image не реагирует на входящие значения

private:
    String imageStyle; // CSS-стиль позиционирования и отображения изображения
};

class UIGraphElement : public UIDeclarativeElement { // UI-элемент графика значений
public:
    UIGraphElement(const String &elementId, const String &elementLabel, const String &config)
        : UIDeclarativeElement(elementId, elementLabel), configString(config), source(nullptr) {} // график с автоисточником данных

    UIGraphElement(const String &elementId, const String &elementLabel, const String &config, float &sourceRef)
        : UIDeclarativeElement(elementId, elementLabel), configString(config), source(&sourceRef) {} // график с явным источником данных

    void build(OABuilder &builder) override{
        if(source) builder.displayGraph(id, label, configString, *source); // график с прямой ссылкой на переменную
        else builder.displayGraph(id, label, configString); // график с источником по имени
    }
    void load() override{} // график не загружает состояние
    void save() const override{} // график не сохраняет состояние
    String valueString() const override{ return String(); } // график не возвращает единичное значение
    void setFromString(const String &value) override{ (void)value; } // график не принимает значения из UI

private:
    String configString; // строка конфигурации графика (points, step, source и т.д.)
    float *source; // указатель на источник данных, если задан явно
};

inline void onSetLampChange(const String &value){ // обработчик изменения режима лампы из UI
    SetLamp = value; // сохраняет выбранный режим
    if(SetLamp == "on"){ // режим постоянного включения
        Lamp = true;
        Lamp_autosvet = false;
        Power_Time1 = false;
    } else if(SetLamp == "auto"){ // режим автоматического управления
        Lamp = false;
        Lamp_autosvet = true;
        Power_Time1 = false;
    } else if(SetLamp == "timer"){ // режим работы по таймеру
        Lamp = false;
        Lamp_autosvet = false;
        Power_Time1 = true;
    } else { // любой другой режим — всё выключено
        Lamp = false;
        Lamp_autosvet = false;
        Power_Time1 = false;
    }
    saveButtonState("button_Lamp", Lamp ? 1 : 0); // сохраняет состояние кнопки лампы
    saveValue<int>("Lamp_autosvet", Lamp_autosvet ? 1 : 0); // сохраняет режим автоосвещения
    saveValue<int>("Power_Time1", Power_Time1 ? 1 : 0); // сохраняет флаг таймера
}

inline void onSetRgbChange(const String &value){ // обработчик изменения режима RGB-подсветки
    SetRGB = value; // сохраняет выбранный режим
    if(SetRGB == "on"){ // постоянное включение
        Pow_WS2815 = true;
        Pow_WS2815_autosvet = false;
        WS2815_Time1 = false;
    } else if(SetRGB == "auto"){ // автоматический режим
        Pow_WS2815 = false;
        Pow_WS2815_autosvet = true;
        WS2815_Time1 = false;
    } else if(SetRGB == "timer"){ // режим по таймеру
        Pow_WS2815 = false;
        Pow_WS2815_autosvet = false;
        WS2815_Time1 = true;
    } else { // любое другое значение — всё выключено
        Pow_WS2815 = false;
        Pow_WS2815_autosvet = false;
        WS2815_Time1 = false;
    }
    saveButtonState("button_WS2815", Pow_WS2815 ? 1 : 0); // сохраняет состояние кнопки RGB
    saveValue<int>("Pow_WS2815_autosvet", Pow_WS2815_autosvet ? 1 : 0); // сохраняет режим авто-RGB
    saveValue<int>("WS2815_Time1", WS2815_Time1 ? 1 : 0); // сохраняет таймер RGB
}

inline void onLedColorModeChange(const String &value){ // обработчик выбора режима цвета LED
    ColorRGB = value.equalsIgnoreCase("manual"); // включает ручной режим выбора цвета
}

inline void onLedBrightnessChange(const int &value){ // обработчик изменения яркости LED из UI
    new_bright = value; // обновляет глобальную переменную яркости для последующего применения
}

// ВНИМАНИЕ: // Предупреждение по архитектуре
// UIApplyHandlerRegistry используется ТОЛЬКО для legacy-кода. // Требование: только для старой логики
// В новой декларативной архитектуре (сервисные loop’ы) данный механизм НЕ используется и не должен расширяться. // Требование: запрет на новую логику
class UIApplyHandlerRegistry { // Реестр обработчиков применения значений UI
public: // Открытая секция для регистрации обработчиков
    void add(const String &id, const std::function<void(const String &)> &handler){ // Добавляет обработчик по id
        handlers.push_back({id, handler}); // Сохраняет пару id и callback
    } // Конец add
    void apply(const String &id, const String &value) const{ // Вызывает обработчик для id, если он зарегистрирован
        for(const auto &entry : handlers){ // Перебирает все зарегистрированные обработчики
            if(entry.id == id){ // Проверяет совпадение id
                if(entry.handler) entry.handler(value); // Вызывает обработчик, если он задан
                break; // Прерывает поиск после первого совпадения
            } // Конец проверки id
        } // Конец цикла обработчиков
    } // Конец apply
private: // Приватная секция данных
    struct Entry { // Элемент записи обработчика
        String id; // Идентификатор UI-элемента
        std::function<void(const String &)> handler; // Callback обработки значения
    }; // Конец структуры Entry
    std::vector<Entry> handlers; // Список всех обработчиков
}; // Конец UIApplyHandlerRegistry
inline UIApplyHandlerRegistry uiApplyHandlers; // Глобальный реестр обработчиков применения UI
inline void registerUiApplyHandler(const String &id, const std::function<void(const String &)> &handler){ // Регистрация обработчика UI
    uiApplyHandlers.add(id, handler); // Добавляет обработчик в реестр
} // Конец registerUiApplyHandler


inline String uiValueForId(const String &id){ // универсальная функция получения текущего значения UI-элемента по id
    UIDeclarativeElement *element = uiRegistry.find(id); // ищет зарегистрированный UI-элемент по идентификатору
    if(!element) return String(); // если элемент не найден — возвращает пустую строку
    return element->valueString(); // возвращает строковое представление текущего значения элемента
}

inline bool uiApplyValueForId(const String &id, const String &value){ // применяет значение к UI-элементу
    bool applied = uiRegistry.applyValue(id, value); // Применяет значение через реестр декларативных элементов
    if(!applied) return false; // Если элемент не найден — сообщает об ошибке применения
    return true; // Сообщает об успешном применении значения
}


// макрос регистрации и построения UI-элемента
// гарантирует, что элемент регистрируется только один раз
// и добавляется в текущую структуру UI
#define UI_REGISTER_ELEMENT(instance) \
    do { \
        if(!(instance).registered){ uiRegistry.registerElement(&(instance)); } \
        (instance).build(oab); \
    } while(false)


#define UI_APP(title) oab.app(title) // объявляет и инициализирует UI-приложение
#define UI_MENU(title) oab.menu(title) // добавляет пункт меню
#define UI_PAGE() oab.page() // создаёт новую страницу UI

#define UI_POPUP_BEGIN(id, title, buttonLabel) oab.popupBegin(id, title, buttonLabel) // начинает описание popup-окна
#define UI_POPUP_END() oab.popupEnd() // завершает описание popup-окна

#define UI_CONCAT_INNER(a, b) a##b // склеивание токенов препроцессора
#define UI_CONCAT(a, b) UI_CONCAT_INNER(a, b) // двухэтапная конкатенация для корректной подстановки
#define UI_UNIQUE_NAME(prefix) UI_CONCAT(prefix, __LINE__) // создаёт уникальное имя на основе номера строки


// объявляет кнопку с состоянием по умолчанию
#define UI_BUTTON(id, state, color, label) \
    do { static UIButtonElement<decltype(state)> UI_UNIQUE_NAME(ui_button_)(id, state, color, label, static_cast<int>(state)); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_button_)); } while(false)

// кнопка с явно заданным состоянием по умолчанию
#define UI_BUTTON_DEFAULT(id, state, color, label, defaultState) \
    do { static UIButtonElement<decltype(state)> UI_UNIQUE_NAME(ui_button_)(id, state, color, label, defaultState); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_button_)); } while(false)

// объявляет чекбокс, связанный с bool-переменной
#define UI_CHECKBOX(id, state, label) \
    do { static UICheckboxElement UI_UNIQUE_NAME(ui_checkbox_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_checkbox_)); } while(false)

// одиночный range/slider без callback
#define UI_RANGE(id, state, minVal, maxVal, stepVal, label) \
    do { static UIRangeElement<decltype(state)> UI_UNIQUE_NAME(ui_range_)(id, state, minVal, maxVal, stepVal, label, false, nullptr); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_range_)); } while(false)

// range/slider с callback при изменении значения
#define UI_RANGE_CB(id, state, minVal, maxVal, stepVal, label, callback) \
    do { static UIRangeElement<decltype(state)> UI_UNIQUE_NAME(ui_range_)(id, state, minVal, maxVal, stepVal, label, false, callback); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_range_)); } while(false)

// двойной диапазон с раздельным хранением min/max
#define UI_DUAL_RANGE_KEYS(id, minState, maxState, minKey, maxKey, minVal, maxVal, stepVal, label) \
    do { static UIDualRangeElement<std::remove_reference_t<decltype(minState)>> UI_UNIQUE_NAME(ui_dual_range_)(id, minState, maxState, minKey, maxKey, minVal, maxVal, stepVal, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_dual_range_)); } while(false)

// числовое поле ввода (int/float)
#define UI_NUMBER(id, state, label, allowFloat) \
    do { static UINumberElement<decltype(state)> UI_UNIQUE_NAME(ui_number_)(id, state, label, allowFloat); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_number_)); } while(false)

// текстовый элемент (ввод или styled-display)
#define UI_TEXT(id, state, labelOrStyle) \
    do { static UITextElement UI_UNIQUE_NAME(ui_text_)(id, state, labelOrStyle); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_text_)); } while(false)

// display-элемент для строковых значений
#define UI_DISPLAY(id, state, label) \
    do { static UIDisplayElement UI_UNIQUE_NAME(ui_display_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_)); } while(false)

// display-элемент для int
#define UI_DISPLAY_INT(id, state, label) \
    do { static UIDisplayIntElement UI_UNIQUE_NAME(ui_display_int_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_int_)); } while(false)

// display-элемент для float
#define UI_DISPLAY_FLOAT(id, state, label) \
    do { static UIDisplayFloatElement UI_UNIQUE_NAME(ui_display_float_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_float_)); } while(false)

// display-элемент для bool с текстовыми состояниями
#define UI_DISPLAY_BOOL(id, state, label, onLabel, offLabel) \
    do { static UIDisplayBoolElement UI_UNIQUE_NAME(ui_display_bool_)(id, state, label, onLabel, offLabel); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_bool_)); } while(false)

// скрытый элемент для хранения значения без отображения в UI
#define UI_HIDDEN(id, state) \
    do { static UIHiddenElement<decltype(state)> UI_UNIQUE_NAME(ui_hidden_)(id, state); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_hidden_)); } while(false)



// элемент выбора времени
#define UI_TIME(id, state, label) \
    do { static UITimeElement UI_UNIQUE_NAME(ui_time_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_time_)); } while(false)

// элемент таймера с on/off интервалами и явной привязкой внешних переменных времени включения/выключения
#define UI_TIMER(id, label, onValue, offValue, ...) \
    do { static UITimerElement UI_UNIQUE_NAME(ui_timer_)(id, label, &(onValue), &(offValue), ##__VA_ARGS__); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_timer_)); } while(false)

// выпадающий список без callback
#define UI_SELECT(id, state, options, label) \
    do { static UISelectElement<decltype(state)> UI_UNIQUE_NAME(ui_select_)(id, state, label, options, nullptr); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_select_)); } while(false)

// выпадающий список с callback при изменении
#define UI_SELECT_CB(id, state, options, label, callback) \
    do { static UISelectElement<decltype(state)> UI_UNIQUE_NAME(ui_select_)(id, state, label, options, callback); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_select_)); } while(false)

// специализированный select выбора дней недели
// Сохраняет дни как строку вида "Mon,Tue" в переменной состояния; используйте syncCleanDaysFromSelection/syncDaysSelectionFromClean для согласованности
#define UI_SELECT_DAYS(id, state, label) \
    do { static UISelectDaysElement UI_UNIQUE_NAME(ui_select_days_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_select_days_)); } while(false)

// color picker для выбора цвета
#define UI_COLOR(id, state, label) \
    do { static UIColorElement UI_UNIQUE_NAME(ui_color_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_color_)); } while(false)

// отображение изображения с CSS-стилем
#define UI_IMAGE(id, file, style) \
    do { static UIImageElement UI_UNIQUE_NAME(ui_image_)(id, file, style); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_image_)); } while(false)

// график с источником данных по имени
#define UI_GRAPH(id, label, config) \
    do { static UIGraphElement UI_UNIQUE_NAME(ui_graph_)(id, label, config); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_graph_)); } while(false)

// график с прямой ссылкой на переменную-источник
#define UI_GRAPH_SOURCE(id, label, config, source) \
    do { static UIGraphElement UI_UNIQUE_NAME(ui_graph_source_)(id, label, config, source); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_graph_source_)); } while(false)
