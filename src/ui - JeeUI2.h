//ui - JeeUI2.h
#pragma once

#include <Arduino.h>
#include <vector>
#include <functional>
#include <map>
#include <initializer_list>
#include <type_traits>
#include "web.h"

class OABuilder {
public:
      void registerGraphInput(const String &name, float &ref){ graphInputs[name] = &ref; }

    void app(const String &name){
        dashAppTitle = name;
        resetDash();
        dashInterfaceInitialized = true;
    }

    void menu(const String &title){ menus.push_back(title); }

    void page(){
        if(pageIndex >= menus.size()){
            menus.push_back(String(F("Page ")) + String(pageIndex + 1));
        }
        currentTabId = String(F("tab")) + String(pageIndex + 1);
        dash.addTab(currentTabId, menus[pageIndex]);
        pageIndex++;
    }

    void popupBegin(const String &popupId, const String &title, const String &buttonLabel){
        ensurePage();
        String popupTabId = String(F("popup_")) + popupId;
        dash.addPopup(popupId, title, popupTabId);
        addElement("popupButton", String(F("popup_btn_")) + popupId, buttonLabel, popupId);
        tabStack.push_back(currentTabId);
        currentTabId = popupTabId;
    }

    void popupEnd(){
        if(tabStack.empty()) return;
        currentTabId = tabStack.back();
        tabStack.pop_back();
    }


    void display(const String &id, const String &label, const String &placeholder=""){
        addElement("display", id, label, placeholder);
    }

    void text(const String &id, const String &label){ addElement("text", id, label, ""); }

    void text(const String &id, const String &valueOrLabel, const String &extra){
        if(isStyleString(extra)) addElement("displayStringAbsolute", id, valueOrLabel, extra);
        else addElement("text", id, valueOrLabel, extra);
    }

    void textarea(const String &id, const String &label, const String &defaultValue=""){
        addElement("textarea", id, label, defaultValue);
    }

    void number(const String &id, const String &label, bool allowFloat=false, const String &defaultValue=""){
        bool asFloat = allowFloat || containsIgnoreCase(id, F("float")) || containsIgnoreCase(label, F("float"));
        addElement(asFloat ? "float" : "int", id, label, defaultValue);
    }

    void time(const String &id, const String &label, const String &defaultValue=""){
        addElement("time", id, label, defaultValue);
    }
    
        void timer(const String &id, const String &label,
               const std::function<void(uint16_t, uint16_t)> &callback){
        ui.registerTimer(id, label, callback);
        addElement("timer", id, label, "");
    }


    void color(const String &id, const String &label, const String &defaultValue=""){
        addElement("color", id, label, defaultValue);
    }

  void image(const String &id, const String &filename, const String &style=""){
        auto ensureUnit = [](String raw) -> String {
            raw.trim();
            if(!raw.length()) return raw;
            for(size_t i = 0; i < raw.length(); i++){
                char c = raw[i];
                if(!((c >= '0' && c <= '9') || c == '.' || c == '-')) return raw;
            }
            return raw + "px";
        };

        String normalized = style;
        if(normalized.length() && normalized[normalized.length()-1] != ';') normalized += ';';

        String xRaw, yRaw, rebuilt;
        int start = 0;
        while(start < normalized.length()){
            int end = normalized.indexOf(';', start);
            if(end < 0) end = normalized.length();
            String token = normalized.substring(start, end);
            token.trim();
            if(token.length()){
                int sep = token.indexOf(':');
                if(sep > 0){
                    String key = token.substring(0, sep); key.trim();
                    String val = token.substring(sep + 1); val.trim();
                    if(key.equalsIgnoreCase("x")) xRaw = ensureUnit(val);
                    else if(key.equalsIgnoreCase("y")) yRaw = ensureUnit(val);
                    else rebuilt += key + ':' + val + ';';
                } else {
                    rebuilt += token + ';';
                }
            }
            start = end + 1;
        }

        if(xRaw.length() || yRaw.length()){
            rebuilt += "position:absolute;";
            rebuilt += "left:" + (xRaw.length() ? xRaw : String("0px")) + ';';
            rebuilt += "top:" + (yRaw.length() ? yRaw : String("0px")) + ';';
        }

        addElement("image", id, filename, rebuilt);
    }


    void checkbox(const String &id, const String &label, const String &defaultValue="0"){
        addElement("checkbox", id, label, defaultValue);
    }

    void button(const String &id, const String &color, const String &label, const String &layoutCfg=""){
        String cfg = layoutCfg;
        if(cfg.length() && cfg[cfg.length()-1] != ';') cfg += ';';
        if(color.length()) cfg += String(F("color=")) + normalizeColor(color) + ';';
        addElement("button", id, label, cfg);
    }

        void range(const String &id, float minVal, float maxVal, float step, const String &label, bool dual=false){
        String cfg = String(F("min=")) + String(minVal) + F(";max=") + String(maxVal) + F(";step=") + String(step);
        addElement(dual ? "range" : "slider", id, label, cfg);
    }

    void selectDays(const String &id, const String &label){ addElement("selectdays", id, label, ""); }

    void option(const String &value, const String &label){
        if(optionBuffer.length()) optionBuffer += '\n';
        optionBuffer += value + '=' + label;
    }

    void select(const String &id, const String &label){
        addElement("dropdown", id, label, optionBuffer);
        optionBuffer = "";
    }

    void pub(const String &id, const String &label, const String &unit="", const String &bg="#6060ff", const String &textColor="#ffffff"){
        String cfg = String(F("unit=")) + unit + F(";bg=") + normalizeColor(bg) + F(";fg=") + normalizeColor(textColor);
        addElement("pub", id, label, cfg);
    }

    void displayGraph(const String &id, const String &label, const String &config){
        GraphConfig cfg = parseGraphConfig(config, id);
        addElement("displayGraph", id, label, config);
        std::function<float()> getter = buildGetter(cfg, nullptr);
        registerGraphSource(id, getter, cfg.valueKey, updateInterval, cfg.points);
    }

    void displayGraph(const String &id, const String &label, const String &config, float &source){
        GraphConfig cfg = parseGraphConfig(config, id);
        registerGraphInput(cfg.valueKey, source);
        addElement("displayGraph", id, label, config);
        std::function<float()> getter = buildGetter(cfg, &source);
        registerGraphSource(id, getter, cfg.valueKey, updateInterval, cfg.points);
    }

private:
    std::vector<String> menus;
    String currentTabId;
    String optionBuffer;
    size_t pageIndex = 0;
    std::map<String, float*> graphInputs;
        std::vector<String> tabStack;

    struct GraphConfig{
        String valueKey;
        int points;
        unsigned long maxPeriod;
        unsigned long step;
    };

    void resetDash(){
        dash.tabs.clear();
        dash.elements.clear();
                dash.popups.clear();
        menus.clear();
        optionBuffer = "";
        currentTabId = "";
        pageIndex = 0;
    }

    bool containsIgnoreCase(String text, const __FlashStringHelper *needle){
        text.toLowerCase();
        String token(needle);
        token.toLowerCase();
        return text.indexOf(token) >= 0;
    }

    bool isStyleString(const String &text) const{
        if(!text.length()) return false;
        const char* markers[] = {"x:", "y:", "font", "color", "left:", "top:"};
        for(const char* marker : markers){
            if(text.indexOf(marker) >= 0) return true;
        }
        return false;
    }

    String normalizeColor(String color){
        String trimmed = color;
        trimmed.trim();
        if(!trimmed.startsWith("#")){
            if(trimmed.length()==3 || trimmed.length()==6){
                trimmed = "#" + trimmed;
            }
        }
        return trimmed;
    }

    void ensurePage(){ if(currentTabId.length()==0) page(); }

    void addElement(const String &type, const String &id, const String &label, const String &value){
        ensurePage();
        Element element{type, id, label, value, ""};
        dash.addElement(currentTabId, element);
    }

  GraphConfig parseGraphConfig(const String &config, const String &fallbackId){
        int defaultPoints = maxPoints > 0 ? maxPoints : 30;
        unsigned long defaultPeriod = 600000; // 10 минут по умолчанию
        unsigned long defaultStep = 1000;     // 1 секунда по умолчанию
        GraphConfig cfg{fallbackId, defaultPoints, defaultPeriod, defaultStep};
        int start = 0;
        while(start < config.length()){
            int end = config.indexOf(';', start);
            if(end < 0) end = config.length();
            String token = config.substring(start, end);
            int sep = token.indexOf(':');
            if(sep > 0){
                String key = token.substring(0, sep);
                String val = token.substring(sep + 1);
                key.trim(); val.trim();
                if(key.equalsIgnoreCase("value")) cfg.valueKey = val;
                else if(key.equalsIgnoreCase("maxPoints")) cfg.points = val.toInt();
                else if(key.equalsIgnoreCase("updatePeriod_of_Time")) cfg.maxPeriod = val.toInt();
                else if(key.equalsIgnoreCase("updateStep")) cfg.step = val.toInt();
            }
            start = end + 1;
        }
        return cfg;
    }

    std::function<float()> buildGetter(const GraphConfig &cfg, float *preferred){
        float *source = preferred;
        if(!source){
            for(auto &entry : graphInputs){
                if(entry.first.equalsIgnoreCase(cfg.valueKey)){
                    source = entry.second;
                    break;
                }
            }
        }

        if(!source){
            if(cfg.valueKey.equalsIgnoreCase("Speed")) source = &Speed;
            else if(cfg.valueKey.equalsIgnoreCase("Temperatura")) source = &Temperatura;
        }

        if(source) return [source](){ return *source; };
        return [](){ return 0.0f; };
    }
};

inline OABuilder oab;

struct UIOption {
    const char *value;
    const char *label;
};

inline bool uiIsStyleString(const String &text){
    if(!text.length()) return false;
    const char* markers[] = {"x:", "y:", "font", "color", "left:", "top:"};
    for(const char* marker : markers){
        if(text.indexOf(marker) >= 0) return true;
    }
    return false;
}

class UIDeclarativeElement {
public:
    UIDeclarativeElement(const String &elementId, const String &elementLabel)
        : id(elementId), label(elementLabel) {}
    virtual ~UIDeclarativeElement() = default;

    const String id;
    const String label;
    bool registered = false;

    virtual void build(OABuilder &builder) = 0;
    virtual void load() = 0;
    virtual void save() const = 0;
    virtual String valueString() const = 0;
    virtual void setFromString(const String &value) = 0;
};

class UIDeclarativeRegistry {
public:
    void registerElement(UIDeclarativeElement *element){
        if(!element) return;
        for(auto *existing : elements){
            if(existing && existing->id == element->id){
                element->registered = true;
                return;
            }
        }
        element->load();
        element->registered = true;
        elements.push_back(element);
    }

    UIDeclarativeElement *find(const String &id) const{
        for(auto *element : elements){
            if(element && element->id == id) return element;
        }
        return nullptr;
    }

    bool applyValue(const String &id, const String &value){
        UIDeclarativeElement *element = find(id);
        if(!element) return false;
        element->setFromString(value);
        element->save();
        return true;
    }

private:
    std::vector<UIDeclarativeElement*> elements;
};

inline UIDeclarativeRegistry uiRegistry;

class UICheckboxElement : public UIDeclarativeElement {
public:
    UICheckboxElement(const String &elementId, bool &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {}

    void build(OABuilder &builder) override{
        builder.checkbox(id, label, storage ? "1" : "0");
    }

    void load() override{
        storage = loadValue<int>(id.c_str(), storage ? 1 : 0) != 0;
    }

    void save() const override{
        saveValue<int>(id.c_str(), storage ? 1 : 0);
    }

    String valueString() const override{ return storage ? "1" : "0"; }

    void setFromString(const String &value) override{
        storage = value.toInt() != 0;
    }

private:
    bool &storage;
};

template <typename T>
class UIButtonElement : public UIDeclarativeElement {
public:
    UIButtonElement(const String &elementId, T &storageRef, const String &elementColor, const String &elementLabel, int defaultState)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), color(elementColor),
          defaultValue(defaultState) {}

    void build(OABuilder &builder) override{
        builder.button(id, color, label);
    }

    void load() override{
        int stored = loadButtonState(id.c_str(), defaultValue);
        storage = fromInt(stored);
    }

    void save() const override{
        saveButtonState(id.c_str(), asInt(storage));
    }

    String valueString() const override{ return String(asInt(storage)); }

    void setFromString(const String &value) override{
        storage = fromInt(value.toInt());
    }

private:
    T &storage;
    String color;
    int defaultValue;

    static int asInt(int value){ return value; }
    static int asInt(bool value){ return value ? 1 : 0; }

    static T fromInt(int value){
        if constexpr (std::is_same<T, bool>::value) return value != 0;
        else return static_cast<T>(value);
    }
};

template <typename T>
class UINumberElement : public UIDeclarativeElement {
public:
    UINumberElement(const String &elementId, T &storageRef, const String &elementLabel, bool allowFloat)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), asFloat(allowFloat) {}

    void build(OABuilder &builder) override{
        builder.number(id, label, asFloat, String(storage));
    }

    void load() override{
        storage = loadValue<T>(id.c_str(), storage);
    }

    void save() const override{
        saveValue<T>(id.c_str(), storage);
    }

    String valueString() const override{ return String(storage); }

    void setFromString(const String &value) override{
        if(asFloat) storage = static_cast<T>(value.toFloat());
        else storage = static_cast<T>(value.toInt());
    }

private:
    T &storage;
    bool asFloat;
};

template <typename T>
class UIRangeElement : public UIDeclarativeElement {
public:
    UIRangeElement(const String &elementId, T &storageRef, int minVal, int maxVal, float stepVal,
                   const String &elementLabel, bool dual, const std::function<void(const T &)> &cb)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), minValue(minVal), maxValue(maxVal),
          step(stepVal), dualRange(dual), callback(cb) {}

    void build(OABuilder &builder) override{
        builder.range(id, minValue, maxValue, step, label, dualRange);
    }

    void load() override{
        storage = loadValue<T>(id.c_str(), storage);
        clamp();
        notify();
    }

    void save() const override{
        saveValue<T>(id.c_str(), storage);
    }

    String valueString() const override{ return String(storage); }

    void setFromString(const String &value) override{
        storage = static_cast<T>(value.toFloat());
        clamp();
        notify();
    }

private:
    T &storage;
    T minValue;
    T maxValue;
    float step;
    bool dualRange;
    std::function<void(const T &)> callback;

    void clamp(){
        if(storage < static_cast<T>(minValue)) storage = static_cast<T>(minValue);
        if(storage > static_cast<T>(maxValue)) storage = static_cast<T>(maxValue);
    }

    void notify(){
        if(callback) callback(storage);
    }
};

template <typename T>
class UIDualRangeElement : public UIDeclarativeElement {
public:
      UIDualRangeElement(const String &elementId, T &minRef, T &maxRef, const String &minKey, const String &maxKey, T minVal, T maxVal, float stepVal, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), minStorage(minRef), maxStorage(maxRef),
          minStorageKey(minKey), maxStorageKey(maxKey), minValue(minVal), maxValue(maxVal), step(stepVal) {}

    void build(OABuilder &builder) override{
        builder.range(id, static_cast<float>(minValue), static_cast<float>(maxValue), step, label, true);
    }

    void load() override{
        minStorage = loadValue<T>(minStorageKey.c_str(), minStorage);
        maxStorage = loadValue<T>(maxStorageKey.c_str(), maxStorage);
        clamp();
    }

    void save() const override{
        saveValue<T>(minStorageKey.c_str(), minStorage);
        saveValue<T>(maxStorageKey.c_str(), maxStorage);
    }

    String valueString() const override{ return String(minStorage) + '-' + String(maxStorage); }

    void setFromString(const String &value) override{
        int sep = value.indexOf('-');
        if(sep < 0) return;
        minStorage = parseValue(value.substring(0, sep));
        maxStorage = parseValue(value.substring(sep + 1));
        clamp();
    }

private:
    T &minStorage;
    T &maxStorage;
    String minStorageKey;
    String maxStorageKey;
    int minValue;
    int maxValue;
    float step;

    void clamp(){
        if(minStorage < minValue) minStorage = minValue;
        if(minStorage > maxValue) minStorage = maxValue;
        if(maxStorage < minValue) maxStorage = minValue;
        if(maxStorage > maxValue) maxStorage = maxValue;
        if(minStorage > maxStorage){
            int temp = minStorage;
            minStorage = maxStorage;
            maxStorage = temp;
        }
    }
    
    static T parseValue(const String &value){
        if constexpr(std::is_floating_point<T>::value){
            return static_cast<T>(value.toFloat());
        }
        return static_cast<T>(value.toInt());
    }
};

class UITextElement : public UIDeclarativeElement {
public:
    UITextElement(const String &elementId, String &storageRef, const String &elementLabelOrStyle)
        : UIDeclarativeElement(elementId, elementLabelOrStyle), storage(storageRef) {}

    void build(OABuilder &builder) override{
        if(uiIsStyleString(label)) builder.text(id, storage, label);
        else builder.text(id, label, storage);
    }

    void load() override{
        storage = loadValue<String>(id.c_str(), storage);
    }

    void save() const override{
        saveValue<String>(id.c_str(), storage);
    }

    String valueString() const override{ return storage; }

    void setFromString(const String &value) override{ storage = value; }

private:
    String &storage;
};

class UIDisplayElement : public UIDeclarativeElement {
public:
    UIDisplayElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {}

    void build(OABuilder &builder) override{ builder.display(id, label, storage); }
    void load() override{ storage = loadValue<String>(id.c_str(), storage); }
    void save() const override{ saveValue<String>(id.c_str(), storage); }
    String valueString() const override{ return storage; }
    void setFromString(const String &value) override{ storage = value; }

private:
    String &storage;
};

class UIDisplayIntElement : public UIDeclarativeElement {
public:
    UIDisplayIntElement(const String &elementId, int &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {}

    void build(OABuilder &builder) override{ builder.display(id, label, String(storage)); }
    void load() override{ storage = loadValue<int>(id.c_str(), storage); }
    void save() const override{ saveValue<int>(id.c_str(), storage); }
    String valueString() const override{ return String(storage); }
    void setFromString(const String &value) override{ storage = value.toInt(); }

private:
    int &storage;
};

class UIDisplayFloatElement : public UIDeclarativeElement {
public:
    UIDisplayFloatElement(const String &elementId, float &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {}

    void build(OABuilder &builder) override{ builder.display(id, label, String(storage)); }
    void load() override{ storage = loadValue<float>(id.c_str(), storage); }
    void save() const override{ saveValue<float>(id.c_str(), storage); }
    String valueString() const override{ return String(storage); }
    void setFromString(const String &value) override{ storage = value.toFloat(); }

private:
    float &storage;
};

class UIDisplayBoolElement : public UIDeclarativeElement {
public:
    UIDisplayBoolElement(const String &elementId, bool &storageRef, const String &elementLabel,
                         const String &onLabel, const String &offLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef),
          onText(onLabel), offText(offLabel) {}

    void build(OABuilder &builder) override{ builder.display(id, label, valueString()); }
    void load() override{ storage = loadValue<int>(id.c_str(), storage ? 1 : 0) != 0; }
    void save() const override{ saveValue<int>(id.c_str(), storage ? 1 : 0); }
    String valueString() const override{ return storage ? onText : offText; }
    void setFromString(const String &value) override{
        storage = value.toInt() != 0;
    }

private:
    bool &storage;
    String onText;
    String offText;
};

class UITimeElement : public UIDeclarativeElement {
public:
    UITimeElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {}

    void build(OABuilder &builder) override{ builder.time(id, label, storage); }
    void load() override{ storage = loadValue<String>(id.c_str(), storage); }
    void save() const override{ saveValue<String>(id.c_str(), storage); }
    String valueString() const override{ return storage; }
    void setFromString(const String &value) override{ storage = value; }

private:
    String &storage;
};

class UITimerElement : public UIDeclarativeElement {
public:
    UITimerElement(const String &elementId, const String &elementLabel, const std::function<void(uint16_t, uint16_t)> &cb)
        : UIDeclarativeElement(elementId, elementLabel), callback(cb) {}

    void build(OABuilder &builder) override{
        builder.timer(id, label, callback);
    }

    void load() override{}
    void save() const override{}
    String valueString() const override{ return String(ui.timer(id).on) + '-' + String(ui.timer(id).off); }
    void setFromString(const String &value) override{
        int sep = value.indexOf('-');
        if(sep < 0) return;
        uint16_t onMinutes = static_cast<uint16_t>(value.substring(0, sep).toInt());
        uint16_t offMinutes = static_cast<uint16_t>(value.substring(sep + 1).toInt());
        ui.setTimerMinutes(id, onMinutes, offMinutes, true);
    }

private:
    std::function<void(uint16_t, uint16_t)> callback;
};

template <typename T>
class UISelectElement : public UIDeclarativeElement {
public:
    UISelectElement(const String &elementId, T &storageRef, const String &elementLabel,
                    std::initializer_list<UIOption> optionsList,
                    const std::function<void(const T &)> &cb)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef), options(optionsList), callback(cb) {}

    void build(OABuilder &builder) override{
        for(const auto &opt : options){ builder.option(opt.value, opt.label); }
        builder.select(id, label);
    }

    void load() override{
        if constexpr (std::is_same<T, bool>::value){
            storage = loadValue<int>(id.c_str(), storage ? 1 : 0) != 0;
        } else {
            storage = loadValue<T>(id.c_str(), storage);
        }
        notify();
    }
    void save() const override{
        if constexpr (std::is_same<T, bool>::value){
            saveValue<int>(id.c_str(), storage ? 1 : 0);
        } else {
            saveValue<T>(id.c_str(), storage);
        }
    }
    String valueString() const override{ return toString(storage); }

    void setFromString(const String &value) override{
        storage = fromString(value);
        notify();
    }

private:
    T &storage;
    std::vector<UIOption> options;
    std::function<void(const T &)> callback;

    void notify(){
        if(callback) callback(storage);
    }

    static String toString(const String &value){ return value; }
    static String toString(const char *value){ return String(value); }
    static String toString(int value){ return String(value); }
    static String toString(bool value){ return value ? "1" : "0"; }
    static String toString(float value){ return String(value); }

    static T fromString(const String &value){
        if constexpr (std::is_same<T, String>::value) return value;
        else if constexpr (std::is_same<T, bool>::value) return value.toInt() != 0;
        else if constexpr (std::is_same<T, float>::value) return value.toFloat();
        else return static_cast<T>(value.toInt());
    }
};

class UISelectDaysElement : public UIDeclarativeElement {
public:
    UISelectDaysElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {}

    void build(OABuilder &builder) override{ builder.selectDays(id, label); }
    void load() override{
        storage = loadValue<String>(id.c_str(), storage);
        syncCleanDaysFromSelection();
    }
    void save() const override{ saveValue<String>(id.c_str(), storage); }
    String valueString() const override{ return storage; }
    void setFromString(const String &value) override{
        storage = value;
        syncCleanDaysFromSelection();
    }

private:
    String &storage;
};

class UIColorElement : public UIDeclarativeElement {
public:
    UIColorElement(const String &elementId, String &storageRef, const String &elementLabel)
        : UIDeclarativeElement(elementId, elementLabel), storage(storageRef) {}

    void build(OABuilder &builder) override{ builder.color(id, label, storage); }
    void load() override{ storage = loadValue<String>(id.c_str(), storage); }
    void save() const override{ saveValue<String>(id.c_str(), storage); }
    String valueString() const override{ return storage; }
    void setFromString(const String &value) override{ storage = value; }

private:
    String &storage;
};

class UIImageElement : public UIDeclarativeElement {
public:
    UIImageElement(const String &elementId, const String &filename, const String &style)
        : UIDeclarativeElement(elementId, filename), imageStyle(style) {}

    void build(OABuilder &builder) override{ builder.image(id, label, imageStyle); }
    void load() override{}
    void save() const override{}
    String valueString() const override{ return label; }
    void setFromString(const String &value) override{ (void)value; }

private:
    String imageStyle;
};

class UIGraphElement : public UIDeclarativeElement {
public:
    UIGraphElement(const String &elementId, const String &elementLabel, const String &config)
        : UIDeclarativeElement(elementId, elementLabel), configString(config), source(nullptr) {}

    UIGraphElement(const String &elementId, const String &elementLabel, const String &config, float &sourceRef)
        : UIDeclarativeElement(elementId, elementLabel), configString(config), source(&sourceRef) {}

    void build(OABuilder &builder) override{
        if(source) builder.displayGraph(id, label, configString, *source);
        else builder.displayGraph(id, label, configString);
    }
    void load() override{}
    void save() const override{}
    String valueString() const override{ return String(); }
    void setFromString(const String &value) override{ (void)value; }

private:
    String configString;
    float *source;
};

inline void onSetLampChange(const String &value){
    SetLamp = value;
    if(SetLamp == "on"){
        Lamp = true;
        Lamp_autosvet = false;
        Power_Time1 = false;
    } else if(SetLamp == "auto"){
        Lamp = false;
        Lamp_autosvet = true;
        Power_Time1 = false;
    } else if(SetLamp == "timer"){
        Lamp = false;
        Lamp_autosvet = false;
        Power_Time1 = true;
    } else {
        Lamp = false;
        Lamp_autosvet = false;
        Power_Time1 = false;
    }
    saveButtonState("button_Lamp", Lamp ? 1 : 0);
    saveValue<int>("Lamp_autosvet", Lamp_autosvet ? 1 : 0);
    saveValue<int>("Power_Time1", Power_Time1 ? 1 : 0);
}

inline void onSetRgbChange(const String &value){
    SetRGB = value;
    if(SetRGB == "on"){
        Pow_WS2815 = true;
        Pow_WS2815_autosvet = false;
        WS2815_Time1 = false;
    } else if(SetRGB == "auto"){
        Pow_WS2815 = false;
        Pow_WS2815_autosvet = true;
        WS2815_Time1 = false;
    } else if(SetRGB == "timer"){
        Pow_WS2815 = false;
        Pow_WS2815_autosvet = false;
        WS2815_Time1 = true;
    } else {
        Pow_WS2815 = false;
        Pow_WS2815_autosvet = false;
        WS2815_Time1 = false;
    }
    saveButtonState("button_WS2815", Pow_WS2815 ? 1 : 0);
    saveValue<int>("Pow_WS2815_autosvet", Pow_WS2815_autosvet ? 1 : 0);
    saveValue<int>("WS2815_Time1", WS2815_Time1 ? 1 : 0);
}

inline void onLedColorModeChange(const String &value){
    ColorRGB = value.equalsIgnoreCase("manual");
}

inline void onLedBrightnessChange(const int &value){
    new_bright = value;
}

inline String uiValueForId(const String &id){
    UIDeclarativeElement *element = uiRegistry.find(id);
    if(!element) return String();
    return element->valueString();
}

inline bool uiApplyValueForId(const String &id, const String &value){
    return uiRegistry.applyValue(id, value);
}

#define UI_REGISTER_ELEMENT(instance) \
    do { \
        if(!(instance).registered){ uiRegistry.registerElement(&(instance)); } \
        (instance).build(oab); \
    } while(false)

#define UI_APP(title) oab.app(title)
#define UI_MENU(title) oab.menu(title)
#define UI_PAGE() oab.page()

#define UI_POPUP_BEGIN(id, title, buttonLabel) oab.popupBegin(id, title, buttonLabel)
#define UI_POPUP_END() oab.popupEnd()

#define UI_CONCAT_INNER(a, b) a##b
#define UI_CONCAT(a, b) UI_CONCAT_INNER(a, b)
#define UI_UNIQUE_NAME(prefix) UI_CONCAT(prefix, __LINE__)

#define UI_BUTTON(id, state, color, label) \
    do { static UIButtonElement<decltype(state)> UI_UNIQUE_NAME(ui_button_)(id, state, color, label, static_cast<int>(state)); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_button_)); } while(false)

#define UI_BUTTON_DEFAULT(id, state, color, label, defaultState) \
    do { static UIButtonElement<decltype(state)> UI_UNIQUE_NAME(ui_button_)(id, state, color, label, defaultState); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_button_)); } while(false)

#define UI_CHECKBOX(id, state, label) \
    do { static UICheckboxElement UI_UNIQUE_NAME(ui_checkbox_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_checkbox_)); } while(false)

#define UI_RANGE(id, state, minVal, maxVal, stepVal, label) \
    do { static UIRangeElement<decltype(state)> UI_UNIQUE_NAME(ui_range_)(id, state, minVal, maxVal, stepVal, label, false, nullptr); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_range_)); } while(false)

#define UI_RANGE_CB(id, state, minVal, maxVal, stepVal, label, callback) \
    do { static UIRangeElement<decltype(state)> UI_UNIQUE_NAME(ui_range_)(id, state, minVal, maxVal, stepVal, label, false, callback); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_range_)); } while(false)

#define UI_DUAL_RANGE_KEYS(id, minState, maxState, minKey, maxKey, minVal, maxVal, stepVal, label) \
    do { static UIDualRangeElement<std::remove_reference_t<decltype(minState)>> UI_UNIQUE_NAME(ui_dual_range_)(id, minState, maxState, minKey, maxKey, minVal, maxVal, stepVal, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_dual_range_)); } while(false)
#define UI_NUMBER(id, state, label, allowFloat) \
    do { static UINumberElement<decltype(state)> UI_UNIQUE_NAME(ui_number_)(id, state, label, allowFloat); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_number_)); } while(false)

#define UI_TEXT(id, state, labelOrStyle) \
    do { static UITextElement UI_UNIQUE_NAME(ui_text_)(id, state, labelOrStyle); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_text_)); } while(false)

#define UI_DISPLAY(id, state, label) \
    do { static UIDisplayElement UI_UNIQUE_NAME(ui_display_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_)); } while(false)

#define UI_DISPLAY_INT(id, state, label) \
    do { static UIDisplayIntElement UI_UNIQUE_NAME(ui_display_int_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_int_)); } while(false)

#define UI_DISPLAY_FLOAT(id, state, label) \
    do { static UIDisplayFloatElement UI_UNIQUE_NAME(ui_display_float_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_float_)); } while(false)

#define UI_DISPLAY_BOOL(id, state, label, onLabel, offLabel) \
    do { static UIDisplayBoolElement UI_UNIQUE_NAME(ui_display_bool_)(id, state, label, onLabel, offLabel); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_display_bool_)); } while(false)

#define UI_TIME(id, state, label) \
    do { static UITimeElement UI_UNIQUE_NAME(ui_time_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_time_)); } while(false)

#define UI_TIMER(id, label, callback) \
    do { static UITimerElement UI_UNIQUE_NAME(ui_timer_)(id, label, callback); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_timer_)); } while(false)

#define UI_SELECT(id, state, options, label) \
    do { static UISelectElement<decltype(state)> UI_UNIQUE_NAME(ui_select_)(id, state, label, options, nullptr); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_select_)); } while(false)

#define UI_SELECT_CB(id, state, options, label, callback) \
    do { static UISelectElement<decltype(state)> UI_UNIQUE_NAME(ui_select_)(id, state, label, options, callback); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_select_)); } while(false)

#define UI_SELECT_DAYS(id, state, label) \
    do { static UISelectDaysElement UI_UNIQUE_NAME(ui_select_days_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_select_days_)); } while(false)

#define UI_COLOR(id, state, label) \
    do { static UIColorElement UI_UNIQUE_NAME(ui_color_)(id, state, label); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_color_)); } while(false)

#define UI_IMAGE(id, file, style) \
    do { static UIImageElement UI_UNIQUE_NAME(ui_image_)(id, file, style); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_image_)); } while(false)

#define UI_GRAPH(id, label, config) \
    do { static UIGraphElement UI_UNIQUE_NAME(ui_graph_)(id, label, config); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_graph_)); } while(false)

#define UI_GRAPH_SOURCE(id, label, config, source) \
    do { static UIGraphElement UI_UNIQUE_NAME(ui_graph_source_)(id, label, config, source); UI_REGISTER_ELEMENT(UI_UNIQUE_NAME(ui_graph_source_)); } while(false)
