//ui - JeeUI2.h
#pragma once

#include <Arduino.h>
#include <vector>
#include <functional>
#include <map>
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

    void range(const String &id, int minVal, int maxVal, float step, const String &label, bool dual=false){
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

    struct GraphConfig{
        String valueKey;
        int points;
        unsigned long maxPeriod;
        unsigned long step;
    };

    void resetDash(){
        dash.tabs.clear();
        dash.elements.clear();
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

#define UI_TIMER(id, label, callback) oab.timer(id, label, callback)
#define UI_TIME(id, label) oab.time(id, label)