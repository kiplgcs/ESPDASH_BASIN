// ------------------- web.h - веб-интерфейс и HTTP -------------------
#pragma once                    // Защищает от повторного включения этого заголовочного файла

#include <Arduino.h>             // Основная библиотека Arduino для ESP32
#include <AsyncTCP.h>            // Асинхронный TCP для ESP32 (не блокирующий)
#include <ESPAsyncWebServer.h>   // Асинхронный веб-сервер для ESP32
#include <esp_system.h>
#include <vector>                // STL вектор для хранения элементов UI
#include <esp_chip_info.h>
#include <esp_efuse.h>
#include "graph.h"               // Пользовательские графики (кастомные)
#include "fs_utils.h"            // Вспомогательные функции для работы с файловой системой
#include "wifi_utils.h"          // Вспомогательные функции для WiFi

using std::vector;              // Используем vector без указания std:: каждый раз

// ---------- Создание веб-сервера ----------
inline AsyncWebServer server(80);  // Создаем веб-сервер на порту 80

// ---------- Глобальные переменные для UI ----------
// Эти переменные управляют значениями элементов интерфейса
inline String ThemeColor;        // Цвет темы интерфейса
inline String LEDColor;          // Цвет LED-индикатора
inline int MotorSpeedSetting;    // Настройка скорости мотора (0-100)
inline int IntInput;             // Целочисленный ввод от пользователя
inline float FloatInput;         // Ввод с плавающей точкой
inline float Speed;              // Скорость (например, датчика)
inline float Temperatura;        // Температура
inline String Timer1;            // Таймер 1
inline String Comment;           // Текстовый комментарий
inline String CurrentTime;       // Текущее время
inline int RandomVal;            // Случайное значение (например, для демонстрации)
inline String InfoString;        // Информационная строка
inline String InfoString1;       // Вспомогательная информационная строка
inline String ModeSelect;        // Режим работы (например, Auto/Manual)
inline String DaysSelect;        // Выбор дней недели
inline String StoredAPSSID;      // Сохраненный SSID точки доступа
inline String StoredAPPASS;      // Сохраненный пароль точки доступа
inline int button1 = 0;          // Состояние кнопки 1
inline int button2 = 0;          // Состояние кнопки 2
inline int RangeMin = 10;        // Минимальное значение диапазонного слайдера
inline int RangeMax = 40;        // Максимальное значение диапазонного слайдера
inline int jpg = 1;              // Флаг JPG отображения (например, переключение картинок)
inline String dashAppTitle = "MiniDash"; // Название приложения
inline bool dashInterfaceInitialized = false; // Флаг, что интерфейс уже инициализирован


extern void interface();

// Безопасное экранирование строк для JSON ответов
String jsonEscape(const String &input){
  String output;
  output.reserve(input.length() + 8);
  for(size_t i = 0; i < input.length(); i++){
    char c = input.charAt(i);
    switch(c){
      case '\\': output += "\\\\"; break;
      case '\"': output += "\\\""; break;
      case '\b': output += "\\b"; break;
      case '\f': output += "\\f"; break;
      case '\n': output += "\\n"; break;
      case '\r': output += "\\r"; break;
      case '\t': output += "\\t"; break;
      default:
        if(static_cast<uint8_t>(c) < 0x20){
          char buf[7];
          snprintf(buf, sizeof(buf), "\\u%04x", static_cast<uint8_t>(c));
          output += buf;
        } else {
          output += c;
        }
    }
  }
  return output;
}

String chipSeriesName(const esp_chip_info_t &info){
  switch(info.model){
    case CHIP_ESP32: return "ESP32";
    case CHIP_ESP32S2: return "ESP32-S2";
    case CHIP_ESP32S3: return "ESP32-S3";
    case CHIP_ESP32C3: return "ESP32-C3";
#ifdef CHIP_ESP32C2
    case CHIP_ESP32C2: return "ESP32-C2";
#endif
#ifdef CHIP_ESP32C6
    case CHIP_ESP32C6: return "ESP32-C6";
#endif
    case CHIP_ESP32H2: return "ESP32-H2";
    default: return "ESP32";
  }
}

String buildChipIdentity(){
  esp_chip_info_t info;
  esp_chip_info(&info);

  const String series = chipSeriesName(info);
  char buffer[96];
  snprintf(buffer, sizeof(buffer), "%s rev %d", series.c_str(), info.revision);
  return String(buffer);
}


// ---------- Структуры UI ----------
struct Tab { String id; String title; };
struct Element { String type; String id; String label; String value; String tab; };





// ---------- Класс MiniDash ----------
class MiniDash {
public:
    vector<Tab> tabs;         // Список вкладок
    vector<Element> elements; // Список элементов UI
  void addTab(const String &id, const String &title){ tabs.push_back({id,title}); }
  void addElement(const String &tab, const Element &e){ Element x=e; x.tab=tab; elements.push_back(x); }
  void begin(){
    if(!dashInterfaceInitialized){
      interface();
      dashInterfaceInitialized = true;
    }
    setupServer();
  }

private:
// Настройка веб-сервера
  void setupServer(){
    MiniDash *self=this;

    server.on("/", HTTP_GET, [self](AsyncWebServerRequest *r){
       // Формируем HTML-страницу
      String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='Content-Type' content='text/html; charset=UTF-8'><title>";
      html += dashAppTitle;
      html += "</title>"
      "<style>"
      "body{margin:0;background:"+ThemeColor+";color:#ddd;font-family:'Inter', 'Segoe UI', 'Roboto', Arial, sans-serif;} "
      "#sidebar{width:230px;position:fixed;left:0;top:0;background:#151515;height:100vh;padding:10px;box-sizing:border-box;transition:all 0.3s;overflow:auto;box-shadow:2px 0 12px rgba(0,0,0,0.45);} "
      "#sidebar.collapsed{width:0;padding:0;overflow:hidden;opacity:0;pointer-events:none;} "
      "#main{margin-left:230px;padding:20px;overflow:auto;height:100vh;box-sizing:border-box;transition:margin-left 0.3s;} "
      "body.sidebar-hidden #main{margin-left:20px;} "
      "#toggleBtn{position:fixed;top:10px;left:230px;width:36px;height:48px;background:rgba(255,255,255,0.12);border:none;border-radius:0 6px 6px 0;cursor:pointer;z-index:1000;transition:left 0.3s, opacity 0.3s, background 0.3s;box-shadow:2px 2px 10px rgba(0,0,0,0.4);} "
      "body.sidebar-hidden #toggleBtn{left:16px;opacity:0.65;} "
      "#sidebar button{width:100%;padding:10px;margin:6px 0;border:none;border-radius:8px;background:#2a2a2a;color:#ccc;text-align:left;cursor:pointer;font-weight:600;transition:0.2s;} "
      "#sidebar button.active{background:#4CAF50;color:#fff;} "
      ".card{background:#1d1d1f;padding:14px;border-radius:14px;margin-bottom:14px;box-shadow:0 4px 14px rgba(0,0,0,0.45);border:1px solid rgba(255,255,255,0.06);display:flex;flex-direction:column;gap:6px;} "
      ".card.compact{padding:10px 12px;margin-bottom:10px;} "
      ".card.compact label{font-size:0.78em;color:#b0b0b0;margin-bottom:2px;text-transform:uppercase;letter-spacing:0.3px;} "
      ".card.compact input,.card.compact select,.card.compact .display-value{font-size:0.95em;padding:6px 8px;border-radius:8px;background:#111;border:1px solid #262626;color:#f6f6f6;} "
      ".card.compact .display-value{background:transparent;border:none;padding:2px 0;} "
      ".select-days{display:grid; grid-template-columns: repeat(4, auto); gap:5px; padding:5px; background:#222; border-radius:6px;} "
      ".select-days label{display:flex;align-items:center;gap:5px;cursor:pointer;} "
      ".select-days.compact{gap:6px;padding:6px;background:#141416;border:1px solid #242424;} "
      "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:center;} "
      "th{background:#333;} "
      ".dash-btn{display:inline-block;margin-top:6px;padding:8px 16px;border-radius:10px;border:1px solid rgba(255,255,255,0.18);background:#222;color:#ddd;font-weight:600;cursor:pointer;box-shadow:0 4px 10px rgba(0,0,0,0.35);transition:transform 0.15s ease, box-shadow 0.15s ease, background 0.15s ease, color 0.15s ease;letter-spacing:0.03em;text-transform:uppercase;font-size:0.78rem;} "
      ".dash-btn.on{background:linear-gradient(135deg,#3a7bd5,#00d2ff);color:#fff;} "
      ".dash-btn.off{background:#222;color:#ddd;opacity:0.9;} "
      ".dash-btn:hover{transform:translateY(-1px);box-shadow:0 6px 14px rgba(0,0,0,0.45);} "
      ".page{display:none;grid-template-columns: repeat(auto-fill, minmax(250px, 1fr)); gap:15px;} "
      ".page.active{display:block;} "
      "label{display:block;margin-bottom:5px;font-size:0.9em;} "
      "input,select{width:100%;padding:7px;margin-bottom:10px;background:#111;border:1px solid #333;color:#ddd;border-radius:6px;} "
      "input[type=range]{width:100%;} "
      ".select-days{display:grid; grid-template-columns: repeat(4, auto); gap:5px; padding:5px; background:#222; border-radius:6px;} "
      ".select-days label{display:flex;align-items:center;gap:5px;cursor:pointer;} "
      "table{width:100%;border-collapse:collapse;margin-top:10px;} th,td{border:1px solid #444;padding:6px;text-align:center;} "
      "th{background:#333;} "
      ".card.pro-card{background:linear-gradient(135deg,#0f0f12,#151a2d);border:1px solid rgba(129,193,255,0.4);} "
      ".card.pro-card label{color:#9ae7ff;text-transform:uppercase;letter-spacing:0.08em;font-size:0.75em;} "
      ".card.pro-card input,.card.pro-card select{background:#090c10;border-color:#252f40;color:#eff6ff;} "
      ".graph-controls{display:flex;flex-direction:row;flex-wrap:nowrap;gap:18px;align-items:center;} "
      ".graph-controls .control-group{flex:1;min-width:220px;display:flex;align-items:center;gap:10px;} "
      ".graph-controls label{display:inline-flex;font-size:0.72em;color:#9fb4c8;letter-spacing:0.08em;text-transform:uppercase;margin-bottom:0;white-space:nowrap;} "
      ".graph-controls select,.graph-controls input{margin-bottom:0;flex:1;} "
      ".dash-graph{width:100%;background:#05070a;}"
      ".card:has(#ModeSelect),.card:has(#LEDColor),.card:has(#Timer1),"
      ".card:has(#FloatInput),.card:has(#IntInput),.card:has(#CurrentTime),"
      ".card:has(#RandomVal),.card:has(#DaysSelect){"
      "background:linear-gradient(145deg,#111,#080b13);border:1px solid rgba(159,180,255,0.25);"
      "box-shadow:0 18px 34px rgba(0,0,0,0.65);border-radius:18px;padding:14px 16px;"
      "transition:transform 0.18s ease,box-shadow 0.18s ease;"
      "position:relative;overflow:hidden;"
      "} "
      ".card:has(#ModeSelect):hover,.card:has(#LEDColor):hover,.card:has(#Timer1):hover,"
      ".card:has(#FloatInput):hover,.card:has(#IntInput):hover,.card:has(#CurrentTime):hover,"
      ".card:has(#RandomVal):hover,.card:has(#DaysSelect):hover{"
      "transform:translateY(-2px);box-shadow:0 22px 40px rgba(0,0,0,0.7);}"
      ".card:has(#ModeSelect) label,.card:has(#LEDColor) label,.card:has(#Timer1) label,"
      ".card:has(#FloatInput) label,.card:has(#IntInput) label,.card:has(#CurrentTime) label,"
      ".card:has(#RandomVal) label,.card:has(#DaysSelect) label{"
      "font-size:0.72em;letter-spacing:0.08em;text-transform:uppercase;color:#94b4d6;"
      "} "
      ".card:has(#ModeSelect) select,.card:has(#LEDColor) input,"
      ".card:has(#Timer1) input,.card:has(#FloatInput) input,.card:has(#IntInput) input{"
      "background:#06070c;border:1px solid rgba(255,255,255,0.12);border-radius:10px;"
      "color:#eef2ff;font-weight:600;padding:10px 12px;box-shadow:inset 0 0 0 1px rgba(255,255,255,0.03);"
      "width:100%;} "

      ".card:has(#LEDColor){"
      "--led-color:#33b8ff;"
      "background:linear-gradient(135deg,rgba(15,18,35,0.98),rgba(11,16,28,0.98));"
      "border:1px solid rgba(110,173,255,0.35);"
      "box-shadow:0 16px 32px rgba(0,0,0,0.7),0 0 0 1px rgba(255,255,255,0.05);"
      "padding:16px 18px;gap:10px;"
      "}"
      ".card:has(#LEDColor)::before{"
      "content:'';position:absolute;inset:8px;"
      "background:radial-gradient(circle at 30% 25%,rgba(255,255,255,0.08),transparent 55%);"
      "pointer-events:none;z-index:0;"
      "}"
      ".card:has(#LEDColor)::after{"
      "content:'';position:absolute;inset:-50%;"
      "background:radial-gradient(circle at 50% 50%,var(--led-color),transparent 50%);"
      "opacity:0.25;filter:blur(32px);pointer-events:none;z-index:0;"
      "}"
      ".card:has(#LEDColor) label{"
      "display:flex;align-items:center;justify-content:space-between;gap:10px;"
      "letter-spacing:0.09em;color:#b4d6ff;position:relative;z-index:1;"
      "}"
      ".card:has(#LEDColor) label::after{"
      "content:attr(data-color);font-family:'JetBrains Mono','SFMono-Regular',monospace;"
      "font-size:0.82em;padding:4px 9px;border-radius:9px;"
      "background:rgba(255,255,255,0.06);color:#eaf4ff;"
      "box-shadow:inset 0 0 0 1px rgba(255,255,255,0.08);"
      "}"
      "#sidebar .card:has(#ThemeColor){"
      "--theme-color:#6dd5ed;"
      "background:linear-gradient(145deg,rgba(16,20,32,0.96),rgba(10,14,24,0.96));"
      "border:1px solid rgba(126,193,255,0.32);"
      "box-shadow:0 14px 26px rgba(0,0,0,0.65),0 0 0 1px rgba(255,255,255,0.05);"
      "padding:14px 16px;position:relative;overflow:hidden;gap:8px;"
      "}"
      "#sidebar .card:has(#ThemeColor)::before{"
      "content:'';position:absolute;inset:10px;"
      "background:radial-gradient(circle at 22% 30%,rgba(255,255,255,0.08),transparent 55%);"
      "pointer-events:none;"
      "}"
      "#sidebar .card:has(#ThemeColor)::after{"
      "content:'';position:absolute;inset:-55%;"
      "background:radial-gradient(circle at 50% 45%,var(--theme-color),transparent 52%);"
      "opacity:0.2;filter:blur(28px);pointer-events:none;"
      "}"
      "#sidebar .card:has(#ThemeColor) label{"
      "display:flex;align-items:center;justify-content:space-between;gap:10px;"
      "letter-spacing:0.09em;color:#b7dbff;font-size:0.72em;text-transform:uppercase;"
      "position:relative;z-index:1;"
      "}"
      "#sidebar .card:has(#ThemeColor) label::after{"
      "content:attr(data-color);font-family:'JetBrains Mono','SFMono-Regular',monospace;"
      "font-size:0.78em;padding:4px 8px;border-radius:9px;"
      "background:rgba(255,255,255,0.06);color:#eaf4ff;"
      "box-shadow:inset 0 0 0 1px rgba(255,255,255,0.08);"
      "}"
      "#ThemeColor{"
      "height:50px;padding:0 10px;border-radius:12px;"
      "background:linear-gradient(145deg,#0c101b,#10182a);"
      "border:1px solid rgba(255,255,255,0.16);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.1),0 12px 22px rgba(0,0,0,0.52);"
      "cursor:pointer;position:relative;z-index:1;"
      "}"
      "#ThemeColor::-webkit-color-swatch-wrapper{padding:6px;border-radius:10px;}"
      "#ThemeColor::-webkit-color-swatch{border-radius:9px;border:1px solid rgba(255,255,255,0.16);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.22);}"
      "#ThemeColor::-moz-color-swatch{border-radius:9px;border:1px solid rgba(255,255,255,0.16);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.22);}"
      "#LEDColor{"
      "height:58px;padding:0 10px;border-radius:14px;"
      "background:linear-gradient(145deg,#0b101a,#101727);"
      "border:1px solid rgba(255,255,255,0.18);"
      "box-shadow:inset 0 1px 0 rgba(255,255,255,0.12),0 14px 26px rgba(0,0,0,0.55);"
      "cursor:pointer;position:relative;z-index:1;"
      "}"
      "#LEDColor::-webkit-color-swatch-wrapper{padding:6px;border-radius:12px;}"
      "#LEDColor::-webkit-color-swatch{border-radius:10px;border:1px solid rgba(255,255,255,0.18);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.25);}"
      "#LEDColor::-moz-color-swatch{border-radius:10px;border:1px solid rgba(255,255,255,0.18);"
      "box-shadow:inset 0 0 0 1px rgba(0,0,0,0.25);}"

      ".card:has(#CurrentTime) #CurrentTime,.card:has(#RandomVal) #RandomVal{"
      "font-size:1.5em;font-weight:700;color:#ffffff;text-shadow:0 4px 12px rgba(0,0,0,0.45);margin-top:6px;"
      "} "
      ".card:has(#DaysSelect) .select-days{gap:8px;padding:8px 6px;background:rgba(255,255,255,0.02);"
      "border-radius:12px;border:1px solid rgba(255,255,255,0.12);box-shadow: inset 0 0 0 1px rgba(255,255,255,0.02);}"
      ".card:has(#DaysSelect) .select-days label{border:1px solid rgba(255,255,255,0.08);"
      "padding:5px 10px;border-radius:9px;background:rgba(255,255,255,0.02);transition:background 0.2s ease,color 0.2s ease;}"
      ".card:has(#DaysSelect) .select-days input[type=checkbox]{accent-color:#4CAF50;}"
      
      ".stats-card{display:flex;flex-direction:column;gap:14px;} "
      ".stat-group{display:flex;flex-direction:column;gap:8px;} "
      ".stat-heading{font-size:0.82em;color:#9fb4c8;letter-spacing:0.05em;text-transform:uppercase;padding-left:2px;} "
      ".stat-list{list-style:none;padding:0;margin:0;display:flex;flex-direction:column;gap:8px;} "
      ".stat-list li{display:flex;justify-content:space-between;align-items:center;padding:8px 10px;border-radius:10px;background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.06);} "
      ".stat-list span{color:#9fb4c8;font-size:0.9em;} "
      ".stat-list strong{color:#fff;font-family:'JetBrains Mono','SFMono-Regular',monospace;font-size:0.95em;} "
      ".btn-primary{display:inline-flex;align-items:center;gap:8px;padding:10px 18px;border-radius:12px;border:1px solid rgba(255,255,255,0.12);background:linear-gradient(135deg,#3a7bd5,#00d2ff);color:#fff;font-weight:700;letter-spacing:0.03em;text-transform:uppercase;box-shadow:0 12px 26px rgba(0,0,0,0.35),0 0 0 1px rgba(255,255,255,0.08);cursor:pointer;transition:transform 0.12s ease,box-shadow 0.12s ease;} "
      ".btn-primary:hover{transform:translateY(-1px);box-shadow:0 16px 30px rgba(0,0,0,0.45);} "
      ".btn-secondary{padding:8px 14px;border-radius:10px;border:1px solid rgba(255,255,255,0.12);background:rgba(255,255,255,0.05);color:#e2e6f0;font-weight:600;cursor:pointer;transition:background 0.15s ease,transform 0.12s ease;} "
      ".btn-secondary:hover{background:rgba(255,255,255,0.1);transform:translateY(-1px);} "
      ".btn-secondary:disabled{opacity:0.6;cursor:progress;} "
      ".btn-danger{padding:8px 14px;border-radius:10px;border:1px solid rgba(255,87,87,0.25);background:linear-gradient(135deg,#ff5f6d,#ffc371);color:#0c0f16;font-weight:700;cursor:pointer;box-shadow:0 8px 18px rgba(255,95,109,0.35);transition:transform 0.12s ease,box-shadow 0.12s ease;} "
      ".btn-danger:hover{transform:translateY(-1px);box-shadow:0 12px 26px rgba(255,95,109,0.45);} "
      ".btn-danger:disabled{opacity:0.65;cursor:progress;} "
      ".stats-actions{display:flex;gap:10px;flex-wrap:wrap;margin-bottom:10px;} "
      ".wifi-card{display:flex;flex-direction:column;gap:10px;} "
      ".wifi-field label{margin-bottom:6px;font-size:0.85em;color:#9fb4c8;text-transform:uppercase;letter-spacing:0.06em;} "
      ".input-with-action{display:flex;gap:8px;align-items:center;} "
      ".wifi-actions{display:flex;align-items:center;gap:14px;margin-top:10px;flex-wrap:wrap;} "
      ".wifi-hint{color:#9fb4c8;font-size:0.9em;} "
      ".wifi-status-card .stat-list li{background:rgba(0,0,0,0.3);} "
      ".section-title{margin-top:10px;margin-bottom:6px;font-size:1em;color:#cfd7e0;} "
      ".wifi-modal{position:fixed;inset:0;background:rgba(0,0,0,0.55);display:flex;align-items:flex-start;justify-content:center;padding-top:60px;z-index:1200;} "
      ".wifi-modal.hidden{display:none;} "
      ".wifi-modal-content{width:min(480px,90vw);background:#1a1c22;border:1px solid rgba(255,255,255,0.08);border-radius:14px;box-shadow:0 18px 40px rgba(0,0,0,0.65);overflow:hidden;} "
      ".wifi-modal-header{display:flex;align-items:center;justify-content:space-between;padding:12px 14px;border-bottom:1px solid rgba(255,255,255,0.06);} "
      ".wifi-scan-list{max-height:320px;overflow:auto;display:flex;flex-direction:column;} "
      ".network-row{display:flex;flex-direction:column;align-items:flex-start;gap:4px;padding:12px 14px;background:transparent;border:none;border-bottom:1px solid rgba(255,255,255,0.05);color:#e9ecf4;text-align:left;cursor:pointer;transition:background 0.12s ease;} "
      ".network-row:hover{background:rgba(255,255,255,0.04);} "
      ".network-ssid{font-weight:700;} "
      ".network-meta{color:#9fb4c8;font-size:0.9em;} "
      ".empty-row{padding:16px;color:#9fb4c8;text-align:center;} "
      ".icon-btn{background:none;border:none;color:#fff;font-size:1.4em;cursor:pointer;line-height:1;} "

      "</style></head><body>";

      // Sidebar
      html += "<div id='sidebar'>";
      bool first = true;
      for(auto &t : self->tabs){
        html += "<button onclick=\"showPage('"+t.id+"',this)\"";
        if(first){ html += " class='active'"; first=false; }
        html += ">"+t.title+"</button>";
      }
      html += "<hr><div class='card'><label>Theme color</label>"
              "<input id='ThemeColor' type='color' value='"+ThemeColor+"'></div>";
      html += "<button onclick=\"showPage('wifi',this)\">WiFi Settings</button>";
      html += "<button onclick=\"showPage('stats',this)\">Statistics</button>";
      html += "</div>";

      html += "<button id='toggleBtn' onclick='toggleSidebar()'>?</button>";

      // Основной контент
      html += "<div id='main'>";

      bool firstPage = true;
      for(auto &t : self->tabs){
          html += "<div id='"+t.id+"' class='page"+String(firstPage?" active":"")+"'><h3>"+t.title+"</h3>";

// Пример для первой вкладки с GIF
if(t.id == "tab1"){
    html += "<div class=\"card\" style=\"position:relative; text-align:center;\">";

    html += "<img id=\"dynImg\" src=\"/getImage\""
            "style=\"display:block; margin:0 auto; max-width:80%; height:auto;"
            "border-radius:12px; box-shadow:0 4px 12px rgba(0,0,0,0.5);"
            "position:relative; z-index:1;\"/>";

    // Генерация элементов UI
    for(auto &e : self->elements){
        if(e.tab == t.id && e.type == "displayStringAbsolute"){
            String styleStr = e.value;
            // Чтение свойства из строки стилей, например "fontSize:24;color:#00ff00;"
            auto readProp = [&](const String &prop)->String {
                String key = prop + ":";
                int idx = styleStr.indexOf(key);
                if(idx < 0) return "";
                int start = idx + key.length();
                int end = styleStr.indexOf(";", start);
                if(end < 0) end = styleStr.length();
                return styleStr.substring(start, end);
            };
            // Нормализация координат: если нет единиц, добавляем "px"
            auto normalizeCoord = [&](const String &raw)->String {
                if(raw.length() == 0) return raw;
                bool hasUnit = false;
                for(int i=0;i<raw.length();i++){
                    char c = raw[i];
                    if((c>='0' && c<='9') || c=='-' || c=='.') continue;
                    hasUnit = true;
                    break;
                }
                return hasUnit ? raw : raw + "px";
            };

            // Получение параметров стиля с дефолтными значениями
            String fontSizeStr = readProp("fontSize");
            int fontSize = fontSizeStr.length() ? fontSizeStr.toInt() : 24;
            String color = readProp("color"); if(color.length() == 0) color = "#00ff00";
            String bgColor = readProp("bg"); if(bgColor.length() == 0) bgColor = "rgba(0,0,0,0.65)";
            String padding = readProp("padding"); if(padding.length() == 0) padding = "6px 12px";
            String borderRadius = readProp("borderRadius"); if(borderRadius.length() == 0) borderRadius = "14px";
            // Координаты и трансформация для центрирования
            String leftRaw = readProp("x");
            String topRaw = readProp("y");
            bool hasLeft = leftRaw.length();
            bool hasTop = topRaw.length();
            String leftValue = hasLeft ? normalizeCoord(leftRaw) : "50%";
            String topValue = hasTop ? normalizeCoord(topRaw) : "50%";
            String translateX = hasLeft ? "0%" : "-50%";
            String translateY = hasTop ? "0%" : "-50%";
            String transform = "translate("+translateX+", "+translateY+")";
            // Формируем inline CSS для элемента
            String panelStyle = "position:absolute; left:"+leftValue+"; top:"+topValue+"; transform:"+transform+"; "
                                 "background:"+bgColor+"; color:"+color+"; font-size:"+String(fontSize)+"px; padding:"+padding+"; "
                                 "border-radius:"+borderRadius+"; display:inline-flex; align-items:center; justify-content:center; "
                                 "text-align:center; box-sizing:border-box; white-space:nowrap; max-width:90%; "
                                 "box-shadow:0 10px 20px rgba(0,0,0,0.45); z-index:2;";

            html += "<div id='"+e.id+"' style='"+panelStyle+"'>"+e.label+"</div>";
        }
    }

    // Создаем контейнер для кнопок выбора картинок, с CSS для выравнивания и отступов
    html += "<div style=\"display:flex;gap:10px; justify-content:center; margin-top:10px;\">"
            "<button onclick=\"setImg(1)\">Картинка 1</button>"  // Кнопка для выбора первой картинки
            "<button onclick=\"setImg(2)\">Картинка 2</button>"  // Кнопка для выбора второй картинки
            "</div>"; // Конец контейнера кнопок

    html += "</div>"; // Закрываем .card контейнер для этого элемента интерфейса
}

          for(auto &e : self->elements){
              if(e.tab != t.id) continue;
              if(t.id == "tab1" && e.type=="displayStringAbsolute") continue;
              if(e.type=="displayGraph" || e.type=="displayGraphJS"){
                  String config = e.value;
                  auto readSetting = [&](const String &prop)->String {
                      String key = prop + ":";
                      int idx = config.indexOf(key);
                      if(idx < 0) return "";
                      int start = idx + key.length();
                      int end = config.indexOf(";", start);
                      if(end < 0) end = config.length();
                      return config.substring(start, end);
                  };
                  auto ensureUnit = [&](const String &raw)->String {
                      if(raw.length() == 0) return "";
                      bool hasUnit = false;
                      for(int i=0;i<raw.length();i++){
                          char c = raw[i];
                          if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                      }
                      return hasUnit ? raw : raw + "px";
                  };
                  String widthRaw = readSetting("width");
                  String heightRaw = readSetting("height");
                  String valueName = readSetting("value");
                  if(valueName.length()==0) valueName = readSetting("source");
                  if(valueName.length()==0) valueName = e.id;
                  String seriesName = e.id;
                  int canvasWidth = widthRaw.length() ? widthRaw.toInt() : 320;
                  int canvasHeight = heightRaw.length() ? heightRaw.toInt() : 220;
                  String widthStyle = widthRaw.length() ? ensureUnit(widthRaw) : "100%";
                  String heightStyle = heightRaw.length() ? ensureUnit(heightRaw) : "220px";
                  String left = ensureUnit(readSetting("left"));
                  String top = ensureUnit(readSetting("top"));
                  String xLabel = readSetting("xLabel"); if(xLabel.length()==0) xLabel = "X Axis";
                  String yLabel = readSetting("yLabel"); if(yLabel.length()==0) yLabel = "Y Axis";
                  String pointColor = readSetting("pointColor"); if(pointColor.length()==0) pointColor = "#ff8c42";
                  String lineColor = readSetting("lineColor"); if(lineColor.length()==0) lineColor = "#4CAF50";
                    String updatePeriodRaw = readSetting("updatePeriod_of_Time");
                    String updateStepRaw = readSetting("updateStep");
                    // unsigned long maxUpdatePeriod = updatePeriodRaw.length() ? updatePeriodRaw.toInt() : 600000; // 10 минут
                    // unsigned long updateStep = updateStepRaw.length() ? updateStepRaw.toInt() : 1000;            // 1 секунда
                    unsigned long maxUpdatePeriodMinutes = updatePeriodRaw.length() ? updatePeriodRaw.toInt() : 10; // минуты
                    unsigned long updateStepMinutes = updateStepRaw.length() ? updateStepRaw.toInt() : 1;           // минуты
                    unsigned long maxUpdatePeriod = maxUpdatePeriodMinutes * 60000UL;
                    unsigned long updateStep = updateStepMinutes * 60000UL;

                    if(maxUpdatePeriod < minGraphUpdateInterval) maxUpdatePeriod = minGraphUpdateInterval;
                    if(updateStep < minGraphUpdateInterval) updateStep = minGraphUpdateInterval;
                    if(updateStep > maxUpdatePeriod) updateStep = maxUpdatePeriod;
                    const size_t maxUpdateOptions = 120; // ограничиваем длину выпадающего списка, чтобы не переполнять память
                    unsigned long optionCount = maxUpdatePeriod / updateStep;
                    if(optionCount > maxUpdateOptions){
                      updateStep = maxUpdatePeriod / maxUpdateOptions;
                      if(updateStep < minGraphUpdateInterval) updateStep = minGraphUpdateInterval;
                    }
                    String maxPointsRaw = readSetting("maxPoints");
                    int defaultGraphMax = maxPointsRaw.length() ? maxPointsRaw.toInt() : (maxPoints > 0 ? maxPoints : 30);
                    // unsigned long defaultUpdate = updateInterval > 0 ? updateInterval : 1000;
                    // unsigned long defaultUpdate = updateInterval > 0 ? updateInterval : updateStep;
                    // GraphSettings seriesSettings{defaultUpdate, defaultGraphMax};
                    if(defaultGraphMax < minGraphPoints) defaultGraphMax = minGraphPoints;
                    int maxSelectablePoints = defaultGraphMax;
                    if(maxSelectablePoints > maxGraphPoints) maxSelectablePoints = maxGraphPoints;
                    unsigned long defaultUpdate = updateInterval > 0 ? updateInterval : updateStep;
                    GraphSettings seriesSettings{defaultUpdate, maxSelectablePoints};



                    if(!loadGraphSettings(seriesName, seriesSettings)){
                      loadGraphSettings(valueName, seriesSettings);
                    }
                    int graphUpdateInterval = seriesSettings.updateInterval;
                    if(graphUpdateInterval < (int)minGraphUpdateInterval) graphUpdateInterval = minGraphUpdateInterval;
                    if(graphUpdateInterval > (int)maxUpdatePeriod) graphUpdateInterval = maxUpdatePeriod;
                    int graphMaxPoints = seriesSettings.maxPoints;
                    if(graphMaxPoints < minGraphPoints) graphMaxPoints = minGraphPoints;
                    // if(graphMaxPoints > maxGraphPoints) graphMaxPoints = maxGraphPoints;
                    if(graphMaxPoints > maxSelectablePoints) graphMaxPoints = maxSelectablePoints;
                    String graphUpdateStr = String(graphUpdateInterval);
                    String graphMaxStr = String(graphMaxPoints);
                  String tableId = "graphTable_"+e.id;
                  String containerStyle = "width:"+widthStyle+";max-width:100%;height:auto;padding:10px 12px;";
                  containerStyle += "display:flex;flex-direction:column;";
                  if(left.length() || top.length()){
                      containerStyle += "position:absolute;";
                      if(left.length()) containerStyle += "left:"+left+";";
                      if(top.length()) containerStyle += "top:"+top+";";
                  }
                  html += "<div class='card graph-card' style='"+containerStyle+"'>";
                  html += "<div class='graph-heading'>"+e.label+"</div>";
                  // html += "<canvas id='graph_"+e.id+"' class='dash-graph' data-graph-id='"+e.id+"' width='"+String(canvasWidth)+"' height='"+String(canvasHeight)+"' data-series='"+valueName+"' data-table-id='"+tableId+"' data-update-interval='"+String(graphUpdateInterval)+"' data-max-points='"+String(graphMaxPoints)+"' data-line-color='"+lineColor+"' data-point-color='"+pointColor+"' data-x-label='"+xLabel+"' data-y-label='"+yLabel+"' style='border:1px solid rgba(255,255,255,0.08);height:"+heightStyle+";'></canvas>";
                  html += "<canvas id='graph_"+e.id+"' class='dash-graph' data-graph-id='"+e.id+"' width='"+String(canvasWidth)+"' height='"+String(canvasHeight)+"' data-series='"+seriesName+"' data-table-id='"+tableId+"' data-update-interval='"+String(graphUpdateInterval)+"' data-max-points='"+String(graphMaxPoints)+"' data-line-color='"+lineColor+"' data-point-color='"+pointColor+"' data-x-label='"+xLabel+"' data-y-label='"+yLabel+"' style='border:1px solid rgba(255,255,255,0.08);height:"+heightStyle+";'></canvas>";
                  html += "<div class='graph-axes'><span class='axis-name'>"+xLabel+"</span><span class='axis-name'>"+yLabel+"</span></div>";
                  html += "</div>";
                  html += "<div class='card graph-controls' style='margin-bottom:0;'>";
                  html += "<div class='control-group'><label>Update Interval</label>";
                  html += "<select class='graph-update-interval' data-graph='"+e.id+"'>";
                  auto formatIntervalLabel = [](unsigned long valMs){
                      if(valMs % 60000 == 0){
                          unsigned long minutes = valMs / 60000;
                          return String(minutes) + " min";
                      }
                      if(valMs % 1000 == 0){
                          unsigned long seconds = valMs / 1000;
                          return String(seconds) + " sec";
                      }
                      return String(valMs) + " ms";
                  };
                  if(maxUpdatePeriod >= 1000){
                      const unsigned long oneSecond = 1000;
                      String val = String(oneSecond);
                      html += String("<option value='") + val + "'" + (graphUpdateStr==val ? " selected" : "") + ">" + formatIntervalLabel(oneSecond) + "</option>";
                  }

                  for(unsigned long opt = updateStep; opt <= maxUpdatePeriod; opt += updateStep){
                      String val = String(opt);
                      html += String("<option value='") + val + "'" + (graphUpdateStr==val ? " selected" : "") + ">" + formatIntervalLabel(opt) + "</option>";
                      if(maxUpdatePeriod - opt < updateStep) break; // защита от переполнения
                  }
                  html += "</select></div>";
                  html += "<div class='control-group'><label>Max Points</label>";
                  // html += "<input class='graph-max-points' data-graph='"+e.id+"' type='number' min='1' max='50' value='"+graphMaxStr+"'>";
                  html += "<input class='graph-max-points' data-graph='"+e.id+"' type='number' min='1' max='"+String(maxSelectablePoints)+"' value='"+graphMaxStr+"'>";
                  html += "</div>";
                  html += "</div>";
                  html += "<div class='card' style='overflow-x:auto;'>";
                  html += "<table id='"+tableId+"' style='min-width:400px;'>";
                  html += "<thead><tr><th>#</th><th>Time</th><th>Value</th></tr></thead><tbody></tbody>";
                  html += "</table></div>";
                  continue;
              }
              String val;

              if(e.id=="ThemeColor") val = ThemeColor;
              else if(e.id=="LEDColor") val = LEDColor;
              else if(e.id=="MotorSpeed") val = String(MotorSpeedSetting);
              else if(e.id=="IntInput") val = String(IntInput);
              else if(e.id=="FloatInput") val = String(FloatInput);
              else if(e.id=="Timer1") val = Timer1;
              else if(e.id=="Comment") val = Comment;
              else if(e.id=="CurrentTime") val = CurrentTime;
              else if(e.id=="RandomVal") val = String(RandomVal);
              else if(e.id=="ModeSelect") val = ModeSelect;
              else if(e.id=="DaysSelect") val = DaysSelect;
              else if(e.id=="RangeSlider") {
                  RangeMin = loadValue<int>("RangeMin", RangeMin);
                  RangeMax = loadValue<int>("RangeMax", RangeMax);
                  val = String(RangeMin) + "-" + String(RangeMax);
              }

              html += "<div class='card'>";

              if(e.type=="text") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='text' value='"+val+"'>";
              else if(e.type=="int") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='number' value='"+val+"'>";
              else if(e.type=="float") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='number' step='0.1' value='"+val+"'>";
              else if(e.type=="time") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='time' value='"+val+"'>";
              else if(e.type=="color") html += "<label>"+e.label+"</label><input id='"+e.id+"' type='color' value='"+val+"'>";
              else if(e.type=="slider"){
                  String cfg = e.value;
                  auto readSliderCfg = [&](const String &prop)->String {
                      String token = prop + "=";
                      int start = cfg.indexOf(token);
                      if(start < 0) return "";
                      start += token.length();
                      int end = cfg.indexOf(';', start);
                      if(end < 0) end = cfg.length();
                      return cfg.substring(start, end);
                  };
                  float minCfg = 0;
                  float maxCfg = 100;
                  float stepCfg = 1;
                  String minStr = readSliderCfg("min");
                  if(minStr.length()) minCfg = minStr.toFloat();
                  String maxStr = readSliderCfg("max");
                  if(maxStr.length()) maxCfg = maxStr.toFloat();
                  String stepStr = readSliderCfg("step");
                  if(stepStr.length()) stepCfg = stepStr.toFloat();
                  if(!val.length()) val = readSliderCfg("value");
                  if(!val.length()){
                      int stepInt = static_cast<int>(stepCfg);
                      bool integerStep = (static_cast<float>(stepInt) == stepCfg);
                      val = String(minCfg, integerStep ? 0 : 2);
                  }
                  html += "<label>"+e.label+"</label><input id='"+e.id+"' type='range' min='"+String(minCfg)+"' max='"+String(maxCfg)+"' step='"+String(stepCfg)+"' value='"+val+"' oninput='updateSlider(this)'><span id='"+e.id+"Val'> "+val+"</span>";
              }
              else if(e.type=="button"){
                  int currentState = loadButtonState(e.id.c_str(), 0);
                  String state = String(currentState);
                  String text = (state == "1") ? "ON" : "OFF";
                  String cssState = (state == "1") ? " on" : " off";

                  // val (e.value) can contain layout config: x:..;y:..;width:..;height:..
                  String cfg = e.value;
                  auto readProp = [&](const String &prop)->String {
                      String key = prop + ":";
                      int idx = cfg.indexOf(key);
                      if(idx < 0) return "";
                      int start = idx + key.length();
                      int end = cfg.indexOf(";", start);
                      if(end < 0) end = cfg.length();
                      return cfg.substring(start, end);
                  };
                  auto ensureUnit = [&](const String &raw)->String {
                      if(raw.length() == 0) return "";
                      bool hasUnit = false;
                      for(int i=0;i<raw.length();i++){
                          char c = raw[i];
                          if(!((c>='0' && c<='9') || c=='-' || c=='.')){ hasUnit = true; break; }
                      }
                      return hasUnit ? raw : raw + "px";
                  };
                  String x = readProp("x");
                  String y = readProp("y");
                  String w = readProp("width");
                  String h = readProp("height");
                  String btnColor = readProp("color");
                  String styleExtra;
                  if(x.length()) styleExtra += "margin-left:"+ensureUnit(x)+";";
                  if(y.length()) styleExtra += "margin-top:"+ensureUnit(y)+";";
                  if(w.length()) styleExtra += "width:"+ensureUnit(w)+";";
                  if(h.length()) styleExtra += "height:"+ensureUnit(h)+";";
                  if(btnColor.length()){
                      String normalized = btnColor;
                      normalized.trim();
                      if(!normalized.startsWith("#") && (normalized.length()==3 || normalized.length()==6)){
                          normalized = "#" + normalized;
                      }
                      styleExtra += "background:"+normalized+";border-color:"+normalized+";";
                  }
                  String styleAttr = styleExtra.length() ? " style='"+styleExtra+"'" : "";

                  html += "<label>"+e.label+"</label><button id='"+e.id+"' class='dash-btn"+cssState+"' data-type='dashButton' data-state='"+state+"'"+styleAttr+">"+text+"</button>";
              }
            else if(e.type=="display" || e.type=="displayString") 
              html += "<label>"+e.label+"</label><div id='"+e.id+"' style='font-size:1.2em; min-height:1.5em; color:#fff;'>"+val+"</div>";
            else if(e.type=="displayStringAbsolute") {
                // Стиль задается в e.value в формате: : x:20;y:20;fontSize:18;color:#fff
                int x=0, y=0, fontSize=16;
                String color="#ffffff";
                String style = e.value; // "x:20;y:20;fontSize:18;color:#ffffff"
                int idx;

                if((idx = style.indexOf("x:"))!=-1) x = style.substring(idx+2, style.indexOf(";", idx)).toInt();
                if((idx = style.indexOf("y:"))!=-1) y = style.substring(idx+2, style.indexOf(";", idx)).toInt();
                if((idx = style.indexOf("fontSize:"))!=-1) fontSize = style.substring(idx+9, style.indexOf(";", idx)).toInt();
                if((idx = style.indexOf("color:"))!=-1) color = style.substring(idx+6);

                html += "<div id='"+e.id+"' style='position:absolute; left:"+String(x)+"px; top:"+String(y)+"px; font-size:"+String(fontSize)+"px; color:"+color+";'>"+e.label+"</div>";
            }

              else if(e.type=="dropdown"){
                  html += "<label>"+e.label+"</label><select id='"+e.id+"'>";
                  if(e.value.length()){
                      int start = 0;
                      while(start < e.value.length()){
                          int end = e.value.indexOf('\n', start);
                          if(end < 0) end = e.value.length();
                          String line = e.value.substring(start, end);
                          line.trim();
                          if(line.length()){
                              int sep = line.indexOf('=');
                              String optValue = sep >= 0 ? line.substring(0, sep) : line;
                              String optLabel = sep >= 0 ? line.substring(sep + 1) : line;
                              html += String("<option value='") + optValue + "'" + (val==optValue?" selected":"") + ">" + optLabel + "</option>";
                          }
                          start = end + 1;
                      }
                  } else {
                      html += String("<option value='Normal'") + (val=="Normal"?" selected":"") + ">Normal</option>";
                      html += String("<option value='Eco'") + (val=="Eco"?" selected":"") + ">Eco</option>";
                      html += String("<option value='Turbo'") + (val=="Turbo"?" selected":"") + ">Turbo</option>";
                  }
                  html += "</select>";
              }
              else if(e.type=="selectdays"){
                  html += "<label>"+e.label+"</label>";
                  html += "<div id='"+e.id+"' class='select-days'>";
                  const char* days[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
                  for(int i=0;i<7;i++){
                      bool checked = val.indexOf(days[i])>=0;
                      html += "<label><input type='checkbox' value='"+String(days[i])+"' "+(checked?"checked":"")+">"+days[i]+"</label>";
                  }
                  html += "</div>"; // ????????? select-days
              }
else if(e.type=="range"){ // Этот блок создает HTML для диапазонного слайдера с двумя ползунками (минимум и максимум)
    // Загружаем минимальное и максимальное значения слайдера из NVS
    // Если значения отсутствуют, используются переменные RangeMin и RangeMax
    int minVal = loadValue<int>("RangeMin", RangeMin); 
    int maxVal = loadValue<int>("RangeMax", RangeMax);

    // Подготавливаем подпись для слайдера. Если e.label пустой, используем "Range Slider"
    String rangeLabel = e.label.length() ? e.label : "Range Slider";
    // Создаем HTML-код для слайдера:
    html += "<label>"+rangeLabel+"</label>"
            "<div class='range-slider' id='RangeSlider'>"
            "<input type='range' id='RangeSliderMin' min='0' max='100' value='" + String(minVal) + "'>"
            "<input type='range' id='RangeSliderMax' min='0' max='100' value='" + String(maxVal) + "'>"
            "<div class='slider-track'></div>"
            "</div>"
            "<div>Range: <span id='RangeSliderVal'>" + String(minVal) + " - " + String(maxVal) + "</span>%</div>"
    // CSS-стили для слайдера
            "<style>"
            ".range-slider { position: relative; width: 100%; height: 10px; }"
            ".range-slider input[type=range] { position: absolute; width: 100%; pointer-events: none; -webkit-appearance: none; background: none; }"
            ".range-slider input[type=range]::-webkit-slider-thumb { pointer-events: all; width: 20px; height: 20px; border-radius: 50%; background: #1e88e5; -webkit-appearance: none; cursor: pointer; position: relative; z-index: 3; }"
            ".slider-track { position: absolute; height: 6px; top: 50%; transform: translateY(-50%); background: #444; border-radius: 3px; width: 100%; }"
            "</style>"
    // JavaScript для управления слайдером
            "<script>"
            "function updateRangeSlider(){"
            "  const minEl = document.getElementById('RangeSliderMin');"
            "  const maxEl = document.getElementById('RangeSliderMax');"
            "  let min = parseInt(minEl.value);"
            "  let max = parseInt(maxEl.value);"
            "  if(min > max) { [min, max] = [max, min]; minEl.value=min; maxEl.value=max; }"
            "  document.getElementById('RangeSliderVal').innerText = min + ' - ' + max;"
            "  document.querySelector('#RangeSlider .slider-track').style.background = "
            "    `linear-gradient(to right, #444 ${min}%, #1e88e5 ${min}%, #1e88e5 ${max}%, #444 ${max}%)`;"
            "  fetch('/save?key=RangeSliderMin&val='+min);"
            "  fetch('/save?key=RangeSliderMax&val='+max);"
            "}"
            "document.getElementById('RangeSliderMin').addEventListener('input', updateRangeSlider);"
            "document.getElementById('RangeSliderMax').addEventListener('input', updateRangeSlider);"
            "updateRangeSlider();"
            "</script>";
}

              html += "</div>"; // Закрываем контейнер .card — завершение текущей карточки UI элемента
          }
          html += "</div>"; // Закрываем контейнер .page — завершение текущей страницы UI
          firstPage = false;
      }

      // ====== WiFi страница ======
      html += "<div id='wifi' class='page'><h3>WiFi Settings</h3>"
              "<div class='card compact wifi-card'>"
              "<div class='wifi-field'><label>SSID</label><div class='input-with-action'>"
              "<input id='ssid' value='"+loadValue<String>("ssid",defaultSSID)+"'>"
              "<button class='btn-secondary' id='scan-btn' onclick='scanWiFi()'>Scan WiFi</button>"
              "</div></div>"
              "<div class='wifi-field'><label>Password</label><input id='pass' type='password' value='"+loadValue<String>("pass",defaultPASS)+"'></div>"
              "</div>"
              "<h4 class='section-title'>AP Settings</h4>"
              "<div class='card compact wifi-card'>"
              "<div class='wifi-field'><label>AP SSID</label><input id='ap_ssid' value='"+loadValue<String>("apSSID", String(apSSID))+"'></div>"
              "<div class='wifi-field'><label>AP Password</label><input id='ap_pass' type='password' value='"+loadValue<String>("apPASS", String(apPASS))+"'></div>"
              "</div>"
              "<div class='wifi-actions'><button class='btn-primary' onclick='saveWiFi()'>Save WiFi</button>"
              "<div class='wifi-hint'>Текущее состояние сети обновляется автоматически</div></div>"
              "<div class='card compact stats-card wifi-status-card'>"
              "<div class='stat-group'><div class='stat-heading'>Сеть</div><ul class='stat-list'>"
              "<li><span>Wi-Fi Mode (текущий режим STA/AP)</span><strong id='wifi-mode'>--</strong></li>"
              "<li><span>SSID (имя Wi-Fi сети)</span><strong id='wifi-ssid'>--</strong></li>"
              "<li><span>Local IP (текущий IP-адрес)</span><strong id='wifi-ip'>--</strong></li>"
              "<li><span>Signal Strength (RSSI) (уровень сигнала Wi-Fi)</span><strong id='wifi-rssi'>--</strong></li>"
              "</ul></div></div>"
              "<div id='wifi-scan-modal' class='wifi-modal hidden'>"
              "<div class='wifi-modal-content'>"
              "<div class='wifi-modal-header'><h4>Доступные сети</h4><button class='icon-btn' onclick='closeWifiModal()'>&times;</button></div>"
              "<div id='wifi-scan-list' class='wifi-scan-list'></div>"
              "</div></div>"
              "</div>";

      // ====== Statistics страница ======
      html += "<div id='stats' class='page'><h3>Statistics</h3>"
              "<div class='stats-actions'>"
              "<button class='btn-secondary' id='refresh-stats-btn' onclick='fetchStats(true)'>Обновить информацию</button>"
              "<button class='btn-danger' id='reboot-btn' onclick='rebootEsp()'>Перезагрузить ESP</button>"
              "</div>"
              "<div class='card compact stats-card'>"
              "<div class='stat-group'><div class='stat-heading'>Система</div><ul class='stat-list'>"
              "<li><span>Chip Model / Revision (модель и ревизия чипа)</span><strong id='stat-chip'>--</strong></li>"
              "<li><span>CPU Frequency (MHz) (текущая частота процессора)</span><strong id='stat-cpu'>--</strong></li>"
              "<li><span>Temperature (температура чипа)</span><strong id='stat-temp'>--</strong></li>"
              "<li><span>Vcc (напряжение питания)</span><strong id='stat-vcc'>--</strong></li>"
              "<li><span>Uptime (время непрерывной работы устройства)</span><strong id='stat-uptime'>--</strong></li>"
              "<li><span>MAC Address (уникальный адрес)</span><strong id='stat-mac'>--</strong></li>"
              "</ul></div>"
              "<div class='stat-group'><div class='stat-heading'>Память и хранилище</div><ul class='stat-list'>"
              "<li><span>Free Heap (свободная оперативная память)</span><strong id='stat-heap'>--</strong></li>"
              "<li><span>Free PSRAM (свободный объём PSRAM)</span><strong id='stat-psram'>--</strong></li>"
              "<li><span>SPIFFS Used / Free (использовано / свободно в SPIFFS)</span><strong id='stat-spiffs'>--</strong></li>"
              "</ul></div>"
              "</div></div>";

      // ====== Основной скрипт страницы ======
      html += R"rawliteral(
<script>
  let wifiStatusInterval = null;

  // Показ выбранной страницы и скрытие остальных
  function showPage(id,btn){
    document.querySelectorAll('.page').forEach(p=>p.classList.remove('active'));
    document.getElementById(id).classList.add('active'); // Отображаем выбранную страницу
    document.querySelectorAll('#sidebar button').forEach(b=>b.classList.remove('active'));
    if(btn) btn.classList.add('active'); // Активируем кнопку меню
    if(id.startsWith('tab')){
      resizeCustomGraphs(); // Подгоняем размеры графиков под контейнер
      customGraphCanvases.forEach(canvas=>{
        const page = canvas.closest('.page');
        if(page && page.id===id){
          fetchCustomGraph(canvas); // Загружаем данные для графика
          restartCustomGraphInterval(canvas); // Запускаем интервал обновления
        }
      });
   }
   if(id === 'stats') startStatsUpdates();
   else stopStatsUpdates();
   if(id === 'wifi') startWifiStatusUpdates();
   else stopWifiStatusUpdates();
  }

 // Скрыть/показать боковую панель
function toggleSidebar(){
 let sb=document.getElementById('sidebar');
 sb.classList.toggle('collapsed');
 document.body.classList.toggle('sidebar-hidden', sb.classList.contains('collapsed'));
}

// Отметить, что пользователь изменил элемент вручную
  const markManualChange = el=>{
    if(!el) return;
    el.dataset.manual = '1';
  };

  const formatBytes = (value)=>{
    const num = Number(value);
    if(isNaN(num)) return String(value || 'N/A');
    const units = ['B','KB','MB','GB'];
    let idx = 0; let val = num;
    while(val >= 1024 && idx < units.length-1){ val /= 1024; idx++; }
    const decimals = val >= 10 ? 1 : 2;
    return `${val.toFixed(decimals)} ${units[idx]}`;
  };

  const renderSpiffs = (used, free, total)=>{
    if(typeof used === 'undefined' || typeof free === 'undefined') return '--';
    const percent = total ? ((Number(used)/Number(total))*100).toFixed(1) : '0.0';
    return `${formatBytes(used)} used / ${formatBytes(free)} free (${percent}%)`;
  };

  const updateStat = (id, value)=>{
    const el = document.getElementById(id);
    if(el) el.innerText = value;
  };

  function fetchStats(manual=false){
    const refreshBtn = document.getElementById('refresh-stats-btn');
    if(manual && refreshBtn){
      refreshBtn.disabled = true;
      refreshBtn.innerText = 'Обновление...';
    }
    fetch('/stats').then(r=>r.json()).then(data=>{
      updateStat('stat-chip', data.chipModelRevision || '--');
      const cpuFreq = typeof data.cpuFreqMHz !== 'undefined' ? data.cpuFreqMHz : null;
      updateStat('stat-cpu', cpuFreq !== null ? `${cpuFreq} MHz` : 'N/A');
      const tempVal = (typeof data.temperature !== 'undefined' && data.temperature !== null) ? data.temperature : 'N/A';
      const tempText = isNaN(Number(tempVal)) ? String(tempVal) : `${Number(tempVal).toFixed(1)} °C`;
      updateStat('stat-temp', tempText);
      const vccVal = (typeof data.vcc !== 'undefined' && data.vcc !== null) ? data.vcc : 'N/A';
      const vccText = isNaN(Number(vccVal)) ? String(vccVal) : `${(Number(vccVal)/1000).toFixed(3)} V`;
      updateStat('stat-vcc', vccText);
      updateStat('stat-uptime', data.uptime || '--');
      updateStat('stat-mac', data.mac || 'N/A');
      updateStat('stat-heap', typeof data.freeHeap !== 'undefined' ? formatBytes(data.freeHeap) : '--');
      const psramVal = (typeof data.freePsram === 'undefined' || data.freePsram === null) ? 'N/A' : data.freePsram;
      updateStat('stat-psram', isNaN(Number(psramVal)) ? String(psramVal) : formatBytes(psramVal));
      updateStat('stat-spiffs', renderSpiffs(data.spiffsUsed, data.spiffsFree, data.spiffsTotal));
    }).catch(()=>{}).finally(()=>{
      if(manual && refreshBtn){
        refreshBtn.disabled = false;
        refreshBtn.innerText = 'Обновить информацию';
      }
    });
  }

  function startStatsUpdates(){
    fetchStats();
  }

  function stopStatsUpdates(){
  }

  function rebootEsp(){
    const rebootBtn = document.getElementById('reboot-btn');
    const original = rebootBtn ? rebootBtn.innerText : '';
    if(rebootBtn){
      rebootBtn.disabled = true;
      rebootBtn.innerText = 'Перезагрузка...';
    }
    fetch('/restart',{method:'POST'}).then(()=>{
      setTimeout(()=>location.reload(), 4000);
    }).catch(()=>{
      if(rebootBtn){
        rebootBtn.disabled = false;
        rebootBtn.innerText = original || 'Перезагрузить ESP';
      }
      alert('Не удалось отправить команду перезагрузки');
    });
  }

  const updateWifiStatus = (data)=>{
    updateStat('wifi-mode', data.wifiMode || 'N/A');
    updateStat('wifi-ssid', data.ssid && data.ssid.length ? data.ssid : 'N/A');
    updateStat('wifi-ip', data.localIp && data.localIp.length ? data.localIp : 'N/A');
    const rssiVal = (typeof data.rssi !== 'undefined' && data.rssi !== null) ? data.rssi : 'N/A';
    const rssiText = isNaN(Number(rssiVal)) ? String(rssiVal) : `${rssiVal} dBm`;
    updateStat('wifi-rssi', rssiText);
  };

  function fetchWifiStatus(){
    fetch('/stats').then(r=>r.json()).then(updateWifiStatus).catch(()=>{});
  }

  function startWifiStatusUpdates(){
    if(wifiStatusInterval) return;
    fetchWifiStatus();
    wifiStatusInterval = setInterval(fetchWifiStatus, 3000);
  }

  function stopWifiStatusUpdates(){
    if(!wifiStatusInterval) return;
    clearInterval(wifiStatusInterval);
    wifiStatusInterval = null;
  }

// Подсветка и отображение текущего значения для панели LED Color
const refreshLedColorUI = (val)=>{
  const ledInput = document.getElementById('LEDColor');
  if(!ledInput) return;
  let colorValue = String(val || ledInput.value || '').toUpperCase();
  if(!colorValue.startsWith('#')) colorValue = '#' + colorValue.replace(/^#?/, '');
  if(colorValue === '#') colorValue = '#33B8FF';
  const card = ledInput.closest('.card');
  if(card) card.style.setProperty('--led-color', colorValue);
  const label = card ? card.querySelector('label') : null;
  if(label) label.setAttribute('data-color', colorValue);
};
// Подсветка и отображение текущего значения для панели Theme Color
const refreshThemeColorUI = (val)=>{
  const themeInput = document.getElementById('ThemeColor');
  if(!themeInput) return;
  let colorValue = String(val || themeInput.value || '').toUpperCase();
  if(!colorValue.startsWith('#')) colorValue = '#' + colorValue.replace(/^#?/, '');
  if(colorValue === '#') colorValue = '#6DD5ED';
  const card = themeInput.closest('.card');
  if(card) card.style.setProperty('--theme-color', colorValue);
  const label = card ? card.querySelector('label') : null;
  if(label) label.setAttribute('data-color', colorValue);
  document.body.style.background = colorValue;
};

// Очистка флага ручного изменения через delay (по умолчанию 600 мс)
const clearManualFlag = (el, delay=600)=>{
  if(!el) return;
  setTimeout(()=>{
    if(el.dataset.manual === '1') el.dataset.manual = '';
  }, delay);
};
// Обновление слайдера и отправка значения на сервер
function updateSlider(el){
 document.getElementById(el.id+'Val').innerText=' '+el.value;
 fetch('/save?key='+el.id+'&val='+encodeURIComponent(el.value));
}
// Обновление значения input (текст, число и т.д.) с проверкой ручного ввода
function updateInputValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || el.dataset.manual === '1') return;
  const text = String(value);
  if(id==='LEDColor') refreshLedColorUI(text);
  else if(id==='ThemeColor') refreshThemeColorUI(text);
}

function updateSelectValue(id, value){
  if(typeof value === 'undefined') return;
  const el = document.getElementById(id);
  if(!el || el.dataset.manual === '1') return;
  const text = String(value);
  if(el.value != text) el.value = text;
}

function updateDaysSelection(id, value){
  const container = document.getElementById(id);
  if(!container || container.dataset.manual === '1') return;
  const tokens = (value||"").split(',').map(s=>s.trim()).filter(Boolean);
  const selected = new Set(tokens);
  container.querySelectorAll('input[type=checkbox]').forEach(chk=>{
    chk.checked = selected.has(chk.value);
  });
}

function updateSliderDisplay(id, value){
  const sl = document.getElementById(id);
  if(sl && sl.value != value) sl.value = value;
  const label = document.getElementById(id+'Val');
  if(label) label.innerText = ' ' + value;
}
function setRangeSliderUI(id, minVal, maxVal){
  const minEl = document.getElementById(id+"Min");
  const maxEl = document.getElementById(id+"Max");
  if(!minEl || !maxEl) return;
  if(minVal > maxVal) [minVal, maxVal] = [maxVal, minVal];
  minEl.value = minVal;
  maxEl.value = maxVal;
  const display = document.getElementById(id+"Val");
  if(display) display.innerText = minVal + ' - ' + maxVal;
  const track = document.querySelector('#'+id+' .slider-track');
  if(track) track.style.background = `linear-gradient(to right, #444 ${minVal}%, #1e88e5 ${minVal}%, #1e88e5 ${maxVal}%, #444 ${maxVal}%)`;
}

function syncDashButton(id, state){
  const btn = document.getElementById(id);
  if(!btn || typeof state === 'undefined') return;
  const isOn = state == 1 || state === true || state === "1";
  btn.dataset.state = isOn ? "1" : "0";
  btn.classList.toggle('on', isOn);
  btn.classList.toggle('off', !isOn);
  btn.innerText = isOn ? "ON" : "OFF";
}

function toggleButton(id){
  const btn = document.getElementById(id);
  if(!btn) return;
  let newState = btn.dataset.state == "1" ? 0 : 1;
  btn.dataset.state = newState;
  btn.classList.toggle('on', newState == 1);
  btn.classList.toggle('off', newState == 0);
  fetch('/button?id=' + encodeURIComponent(id) + '&state=' + newState)
    .then(resp => {
      btn.innerText = newState == 1 ? "ON" : "OFF";
    });
}

document.querySelectorAll('button[data-type="dashButton"]').forEach(b=>{
  b.addEventListener('click', ()=>toggleButton(b.id));
});

// document.getElementById('ThemeColor').addEventListener('change', (e)=>{
//   const c = e.target.value;
//   document.body.style.background = c;
//   fetch('/save?key=ThemeColor&val='+encodeURIComponent(c));
// });
const listenToManual = el=>{
  if(!el) return;
  el.addEventListener('focus', ()=> markManualChange(el));
  el.addEventListener('blur', ()=> clearManualFlag(el));
};


const themeInput = document.getElementById('ThemeColor');
if(themeInput){
  listenToManual(themeInput);
  refreshThemeColorUI(themeInput.value);
  themeInput.addEventListener('change', (e)=>{
    markManualChange(themeInput);
    const c = e.target.value;
    refreshThemeColorUI(c);
    fetch('/save?key=ThemeColor&val='+encodeURIComponent(c));
  });
}


document.querySelectorAll('input[type=text],input[type=number],input[type=time],input[type=color],select').forEach(el=>{
  if(el.id=='ThemeColor') return;
  el.addEventListener('change', ()=>{
    markManualChange(el);
    if(el.id==='LEDColor') refreshLedColorUI(el.value);
    fetch('/save?key='+el.id+'&val='+encodeURIComponent(el.value));
  });
  listenToManual(el);
  if(el.id==='LEDColor') refreshLedColorUI(el.value);
});


document.querySelectorAll('div[id$="DaysSelect"]').forEach(el=>{
  el.querySelectorAll('input[type=checkbox]').forEach(chk=>{
    chk.addEventListener('change', ()=>{
      let selected = Array.from(el.querySelectorAll('input[type=checkbox]:checked')).map(c=>c.value).join(',');
      el.dataset.manual = '1';
      fetch('/save?key='+el.id+'&val='+encodeURIComponent(selected));
      clearManualFlag(el);
    });
  });
});


const escapeHtml = (text='')=>String(text)
  .replace(/&/g,'&amp;')
  .replace(/</g,'&lt;')
  .replace(/>/g,'&gt;')
  .replace(/"/g,'&quot;')
  .replace(/'/g,'&#039;');

const authLabel = (auth)=>{
  const val = (auth || '').toString().toUpperCase();
  if(val.includes('WPA3')) return 'WPA3';
  if(val.includes('WPA2')) return 'WPA2';
  if(val.includes('WPA')) return 'WPA';
  if(val.includes('WEP')) return 'WEP';
  return 'open';
};

function closeWifiModal(){
  const modal = document.getElementById('wifi-scan-modal');
  if(modal) modal.classList.add('hidden');
}

function renderWifiScanList(networks){
  const list = document.getElementById('wifi-scan-list');
  const modal = document.getElementById('wifi-scan-modal');
  if(!list || !modal) return;
  list.innerHTML = '';
  if(!networks || !networks.length){
    list.innerHTML = '<div class="empty-row">Сети не найдены</div>';
  } else {
    networks.forEach(net=>{
      const btn = document.createElement('button');
      btn.className = 'network-row';
      const ssid = escapeHtml(net.ssid || '(hidden)');
      const rssi = typeof net.rssi !== 'undefined' ? `${net.rssi} dBm` : 'n/a';
      const auth = authLabel(net.auth);
      btn.innerHTML = `<div class='network-ssid'>${ssid}</div><div class='network-meta'>${rssi} · ${auth}</div>`;
      btn.addEventListener('click', ()=>{
        const ssidInput = document.getElementById('ssid');
        if(ssidInput) ssidInput.value = net.ssid || '';
        closeWifiModal();
      });
      list.appendChild(btn);
    });
  }
  modal.classList.remove('hidden');
  modal.onclick = (e)=>{ if(e.target === modal) closeWifiModal(); };
}

function scanWiFi(){
  const btn = document.getElementById('scan-btn');
  if(btn){ btn.disabled = true; btn.innerText = 'Scanning...'; }
  fetch('/wifi/scan').then(r=>r.json()).then(data=>{
    renderWifiScanList(data);
  }).catch(()=>{
    alert('Scan failed');
  }).finally(()=>{
    if(btn){ btn.disabled = false; btn.innerText = 'Scan WiFi'; }
  });
}

function saveWiFi(){
 let s=document.getElementById('ssid').value;
 let p=document.getElementById('pass').value;
 let aps=document.getElementById('ap_ssid').value;
 let app=document.getElementById('ap_pass').value;
 fetch('/save?key=ssid&val='+encodeURIComponent(s));
 fetch('/save?key=pass&val='+encodeURIComponent(p));
 fetch('/save?key=apSSID&val='+encodeURIComponent(aps));
 fetch('/save?key=apPASS&val='+encodeURIComponent(app));
 alert('WiFi saved! Reboot ESP.');
}

// ====== ������� ��� ����� � live ======
let customGraphCanvases = Array.from(document.querySelectorAll("canvas[id^='graph_']"));
const customGraphTimers = new Map();

function resizeCustomGraphs(){
  customGraphCanvases.forEach(canvas=>{
    if(!canvas.parentElement) return;
    const w = canvas.parentElement.clientWidth;
    if(w && w > 0) canvas.width = w;
    const h = parseFloat(getComputedStyle(canvas).height);
    if(!isNaN(h) && h > 0) canvas.height = h;
  });
}

function populateGraphTable(tableId, data){
  if(!tableId || !data) return;
  const table = document.getElementById(tableId);
  if(!table) return;
  const tbody = table.querySelector('tbody');
  if(!tbody) return;
  tbody.innerHTML = '';
  data.forEach((point, index)=>{
    const tr = document.createElement('tr');
    tr.innerHTML = '<td>'+(index+1)+'</td><td>'+point.time+'</td><td>'+point.value+'</td>';
    tbody.appendChild(tr);
  });
}

function drawCustomGraph(canvas,data){
  if(!canvas || !canvas.getContext || !data.length) return;
  const ctx = canvas.getContext('2d');
  if(!ctx) return;
  const width = canvas.width;
  const height = canvas.height;
  ctx.clearRect(0,0,width,height);
  ctx.fillStyle = '#05070a';
  ctx.fillRect(0,0,width,height);

  ctx.strokeStyle = 'rgba(255,255,255,0.07)';
  ctx.lineWidth = 1;
  for(let i=0;i<=4;i++){
    let y = i*(height/4);
    ctx.beginPath();
    ctx.moveTo(0,y);
    ctx.lineTo(width,y);
    ctx.stroke();
  }

  const maxPointsAttr = parseInt(canvas.dataset.maxPoints);
  const maxPoints = !isNaN(maxPointsAttr) && maxPointsAttr > 0
    ? maxPointsAttr
    : 10;
  const pointsToDraw = data.slice(-maxPoints);
  const lineColor = canvas.dataset.lineColor || '#4CAF50';
  const pointColor = canvas.dataset.pointColor || '#ff0000';
  ctx.strokeStyle = lineColor;
  ctx.lineWidth = 2;
  ctx.beginPath();
  for(let i=0;i<pointsToDraw.length;i++){
    const x = i*(width/(pointsToDraw.length || 1));
    const y = height - (pointsToDraw[i].value/50.0)*height;
    if(i==0) ctx.moveTo(x,y); else ctx.lineTo(x,y);
  }
  ctx.stroke();

  ctx.fillStyle = pointColor;
  for(let i=0;i<pointsToDraw.length;i++){
    const x = i*(width/(pointsToDraw.length || 1));
    const y = height - (pointsToDraw[i].value/50.0)*height;
    ctx.beginPath();
    ctx.arc(x,y,3,0,2*Math.PI);
    ctx.fill();
  }
  populateGraphTable(canvas.dataset.tableId, pointsToDraw);
}

function fetchCustomGraph(canvas){
  if(!canvas) return;
  const series = canvas.dataset.series || canvas.id;
  fetch('/graphData?series='+encodeURIComponent(series))
    .then(r=>r.json())
    .then(j=>drawCustomGraph(canvas,j));
}

function restartCustomGraphInterval(canvas){
  if(!canvas) return;
  if(customGraphTimers.has(canvas)) clearInterval(customGraphTimers.get(canvas));
  const interval = parseInt(canvas.dataset.updateInterval);
  const delay = (!isNaN(interval) && interval > 0) ? interval : 1000;
  const timer = setInterval(()=>fetchCustomGraph(canvas), delay);
  customGraphTimers.set(canvas, timer);
}

resizeCustomGraphs();
customGraphCanvases.forEach(canvas=>{
  fetchCustomGraph(canvas);
  restartCustomGraphInterval(canvas);
});


document.querySelectorAll('.graph-update-interval').forEach(select=>{
  const graphId = select.dataset.graph;
  const canvas = document.getElementById('graph_'+graphId);
  const series = canvas ? (canvas.dataset.series || graphId) : graphId;
  if(canvas && canvas.dataset.updateInterval) select.value = canvas.dataset.updateInterval;
  select.addEventListener('change', ()=>{
    let value = parseInt(select.value);
    if(isNaN(value) || value < 100) value = 100;
    select.value = value;
    const target = document.getElementById('graph_'+graphId);
    if(!target) return;
    target.dataset.updateInterval = value;
    // fetch('/save?key=graphUpdateInterval_'+graphId+'&val='+encodeURIComponent(value));
    fetch('/save?key=graphUpdateInterval_'+graphId+'&series='+encodeURIComponent(series)+'&val='+encodeURIComponent(value));
    fetchCustomGraph(target);
    restartCustomGraphInterval(target);
  });
});

document.querySelectorAll('.graph-max-points').forEach(input=>{
  const graphId = input.dataset.graph;
  const canvas = document.getElementById('graph_'+graphId);
  const series = canvas ? (canvas.dataset.series || graphId) : graphId;
  if(canvas && canvas.dataset.maxPoints) input.value = canvas.dataset.maxPoints;
  input.addEventListener('change', ()=>{
    let value = parseInt(input.value);
    if(isNaN(value) || value < 1) value = 1;
    if(value > 50) value = 50;
    input.value = value;
    const target = document.getElementById('graph_'+graphId);
    if(!target) return;
    target.dataset.maxPoints = value;
    // fetch('/save?key=graphMaxPoints_'+graphId+'&val='+encodeURIComponent(value));
    fetch('/save?key=graphMaxPoints_'+graphId+'&series='+encodeURIComponent(series)+'&val='+encodeURIComponent(value));
    fetchCustomGraph(target);
  });
});

window.addEventListener('resize', ()=>{
  resizeCustomGraphs();
  customGraphCanvases.forEach(canvas=>{
    fetchCustomGraph(canvas);
  });
});

function fetchLive(){
  fetch('/live').then(r=>r.json()).then(j=>{
    if(document.getElementById('CurrentTime')) document.getElementById('CurrentTime').innerText=j.CurrentTime;
    if(document.getElementById('RandomVal')) document.getElementById('RandomVal').innerText=j.RandomVal;
    if(document.getElementById('InfoString')) document.getElementById('InfoString').innerText=j.InfoString;
    if(document.getElementById('InfoString1')) document.getElementById('InfoString1').innerText=j.InfoString1;
    syncDashButton('button1', j.button1);
    syncDashButton('button2', j.button2);
    if(typeof j.MotorSpeed !== 'undefined') updateSliderDisplay('MotorSpeed', j.MotorSpeed);
    if(typeof j.RangeMin !== 'undefined' && typeof j.RangeMax !== 'undefined') setRangeSliderUI('RangeSlider', j.RangeMin, j.RangeMax);
    if(typeof j.LEDColor !== 'undefined') updateInputValue('LEDColor', j.LEDColor);
    if(typeof j.ModeSelect !== 'undefined') updateSelectValue('ModeSelect', j.ModeSelect);
    if(typeof j.DaysSelect !== 'undefined') updateDaysSelection('DaysSelect', j.DaysSelect);
    if(typeof j.IntInput !== 'undefined') updateInputValue('IntInput', j.IntInput);
    if(typeof j.FloatInput !== 'undefined') updateInputValue('FloatInput', j.FloatInput);

    if(typeof j.Timer1 !== 'undefined') updateInputValue('Timer1', j.Timer1);
    if(typeof j.Comment !== 'undefined') updateInputValue('Comment', j.Comment);
  });
}
setInterval(fetchLive, 1000);

// ====== ??????? ???????????? ??????????? (???????? /setjpg ? ????????????? ????????) ======
function setImg(x){
  fetch('/setjpg?val='+x).then(resp=>{
    // ??????? ??????????? ? ???????????, ????? ???????? ????
    const img = document.getElementById('dynImg');
    if(img) img.src = '/getImage?ts=' + Date.now();
  });
}
</script></body></html>
)rawliteral";

      r->send(200,"text/html",html);
    });

    // ---------------- SAVE ----------------
    server.on("/save", HTTP_GET, [](AsyncWebServerRequest *r){
      if(r->hasParam("key") && r->hasParam("val")){
        String key = r->getParam("key")->value();
        String valStr = r->getParam("val")->value();
        if(key=="ThemeColor") { ThemeColor = valStr; saveValue<String>(key.c_str(), valStr); }
        else if(key=="LEDColor") { LEDColor = valStr; saveValue<String>(key.c_str(), valStr); }
        else if(key=="MotorSpeed") { MotorSpeedSetting = valStr.toInt(); saveValue<int>(key.c_str(), MotorSpeedSetting); }
        else if(key=="IntInput") { IntInput = valStr.toInt(); saveValue<int>(key.c_str(), IntInput); }
        else if(key=="FloatInput") { FloatInput = valStr.toFloat(); saveValue<float>(key.c_str(), FloatInput); }
        else if(key=="Timer1") { Timer1 = valStr; saveValue<String>(key.c_str(), Timer1); }
        else if(key=="Comment") { Comment = valStr; saveValue<String>(key.c_str(), Comment); }
        else if(key=="ModeSelect") { ModeSelect = valStr; saveValue<String>(key.c_str(), ModeSelect); }
        else if(key=="DaysSelect") { DaysSelect = valStr; saveValue<String>(key.c_str(), DaysSelect); }
        else if(key=="graphMainMaxPoints") {
          int valInt = valStr.toInt();
          if(valInt < minGraphPoints) valInt = minGraphPoints;
          if(valInt > maxGraphPoints) valInt = maxGraphPoints;
          maxPoints = valInt;
          trimGraphPoints(graphPoints, maxPoints);
          saveGraphSeries("main", graphPoints);
          saveGraphSettings("main", GraphSettings{updateInterval, maxPoints});
          seriesConfig["main"].maxPoints = maxPoints;
        }
        else if(key=="graphMainUpdateInterval") {
          int valInt = valStr.toInt();
          if(valInt < minGraphUpdateInterval) valInt = minGraphUpdateInterval;
          updateInterval = valInt;
          saveGraphSettings("main", GraphSettings{updateInterval, maxPoints});
          seriesConfig["main"].updateInterval = updateInterval;
          seriesLastUpdate["main"] = 0;
        }
        else if(key.startsWith("graphUpdateInterval_")) {
          String series = key.substring(String("graphUpdateInterval_").length());
          if(r->hasParam("series")) series = r->getParam("series")->value();
          int valInt = valStr.toInt();
          if(valInt < minGraphUpdateInterval) valInt = minGraphUpdateInterval;
          GraphSettings cfg{static_cast<unsigned long>(valInt), maxPoints};
          loadGraphSettings(series, cfg);
          cfg.updateInterval = valInt;
          saveGraphSettings(series, cfg);
          seriesConfig[series].updateInterval = cfg.updateInterval;
          if(seriesConfig[series].maxPoints == 0) seriesConfig[series].maxPoints = cfg.maxPoints;
          seriesLastUpdate[series] = 0;
        }
        else if(key.startsWith("graphMaxPoints_")) {
          String series = key.substring(String("graphMaxPoints_").length());
          if(r->hasParam("series")) series = r->getParam("series")->value();
          int valInt = valStr.toInt();
          if(valInt < minGraphPoints) valInt = minGraphPoints;
          if(valInt > maxGraphPoints) valInt = maxGraphPoints;
          GraphSettings cfg{updateInterval, valInt};
          loadGraphSettings(series, cfg);
          cfg.maxPoints = valInt;
          saveGraphSettings(series, cfg);
          seriesConfig[series].maxPoints = cfg.maxPoints;
          if(seriesConfig[series].updateInterval == 0) seriesConfig[series].updateInterval = cfg.updateInterval;
          auto it = customGraphSeries.find(series);
          if(it != customGraphSeries.end()){
            trimGraphPoints(it->second, valInt);
            saveGraphSeries(it->first, it->second);
          }
        }
        else if(key=="ssid") saveValue<String>(key.c_str(), valStr);
        else if(key=="pass") saveValue<String>(key.c_str(), valStr);
        else if(key=="RangeSliderMin") { RangeMin = valStr.toInt(); saveValue<int>("RangeMin", RangeMin); }
        else if(key=="RangeSliderMax") { RangeMax = valStr.toInt(); saveValue<int>("RangeMax", RangeMax); }
        else if(key=="apSSID") { StoredAPSSID = valStr; saveValue<String>(key.c_str(), valStr); }
        else if(key=="apPASS") { StoredAPPASS = valStr; saveValue<String>(key.c_str(), valStr); }
      }
      r->send(200,"text/plain","OK");
    });

    server.on("/button", HTTP_GET, [](AsyncWebServerRequest *r){
      if(r->hasParam("id") && r->hasParam("state")){
        String id = r->getParam("id")->value();
        int state = r->getParam("state")->value().toInt();
        if(id == "button1") button1 = state;
        else if(id == "button2") button2 = state;
        saveButtonState(id.c_str(), state);
        r->send(200, "text/plain", "OK");
      } else {
        r->send(400, "text/plain", "Missing params");
      }
    });

    server.on("/live", HTTP_GET, [](AsyncWebServerRequest *r){
    String s = "{\"CurrentTime\":\""+CurrentTime+"\",\"RandomVal\":"+String(RandomVal)
               +",\"InfoString\":\""+InfoString+"\",\"InfoString1\":\""+InfoString1
               +"\",\"button1\":"+String(button1)+",\"button2\":"+String(button2)
               +",\"MotorSpeed\":"+String(MotorSpeedSetting)
               +",\"RangeMin\":"+String(RangeMin)+",\"RangeMax\":"+String(RangeMax)
               +",\"LEDColor\":\""+LEDColor+"\",\"ModeSelect\":\""+ModeSelect+"\",\"DaysSelect\":\""+DaysSelect+"\""
               +",\"IntInput\":"+String(IntInput)+",\"FloatInput\":"+String(FloatInput)
               +",\"Timer1\":\""+Timer1+"\",\"Comment\":\""+Comment+"\"}";
    r->send(200, "application/json", s);
    });

    server.on("/stats", HTTP_GET, [](AsyncWebServerRequest *r){
      auto formatUptime = [](){
        unsigned long seconds = millis() / 1000;
        unsigned long hours = seconds / 3600;
        unsigned long minutes = (seconds % 3600) / 60;
        unsigned long secs = seconds % 60;
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", hours, minutes, secs);
        return String(buffer);
      };

      size_t total = SPIFFS.totalBytes();
      size_t used = SPIFFS.usedBytes();
      size_t freeSpace = total > used ? (total - used) : 0;

      String chipModelRevision = buildChipIdentity();
      uint32_t cpuFreq = getCpuFrequencyMhz();
#ifdef ARDUINO_ARCH_ESP32
      float temperatureVal = temperatureRead();
#else
      float temperatureVal = NAN;
#endif
      String temperature = isnan(temperatureVal) ? String("N/A") : String(temperatureVal, 1);
      String vcc = String("N/A");
      String mac = WiFi.macAddress();

      wifi_mode_t mode = WiFi.getMode();
      String wifiMode = "Unknown";
      if(mode == WIFI_MODE_STA) wifiMode = "STA";
      else if(mode == WIFI_MODE_AP) wifiMode = "AP";
      else if(mode == WIFI_MODE_APSTA) wifiMode = "STA+AP";

      bool staActive = (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) && WiFi.isConnected();
      String ssid = (staActive && WiFi.SSID().length()) ? WiFi.SSID() : String("N/A");
      String localIp = staActive ? WiFi.localIP().toString() : String("N/A");
      String rssi = staActive ? String(WiFi.RSSI()) : String("N/A");
      String freePsram = psramFound() ? String(ESP.getFreePsram()) : String("N/A");

      String json = "{";
      json += "\"chipModelRevision\":\""+jsonEscape(chipModelRevision)+"\",";
      json += "\"cpuFreqMHz\":"+String(cpuFreq)+",";
      json += "\"temperature\":\""+jsonEscape(temperature)+"\",";
      json += "\"vcc\":\""+jsonEscape(vcc)+"\",";
      json += "\"uptime\":\""+jsonEscape(formatUptime())+"\",";
      json += "\"mac\":\""+jsonEscape(mac)+"\",";
      json += "\"freeHeap\":"+String(ESP.getFreeHeap())+",";
      json += "\"freePsram\":\""+jsonEscape(freePsram)+"\",";
      json += "\"spiffsUsed\":"+String(used)+",";
      json += "\"spiffsFree\":"+String(freeSpace)+",";
      json += "\"spiffsTotal\":"+String(total)+",";
      json += "\"wifiMode\":\""+jsonEscape(wifiMode)+"\",";
      json += "\"ssid\":\""+jsonEscape(ssid)+"\",";
      json += "\"localIp\":\""+jsonEscape(localIp)+"\",";
      json += "\"rssi\":\""+jsonEscape(rssi)+"\"";
      json += "}";

      r->send(200, "application/json", json);
    });

    server.on("/wifi/scan", HTTP_GET, [](AsyncWebServerRequest *r){
      int16_t n = WiFi.scanNetworks(/*async*/false, /*show_hidden*/false);
      String json = "[";
      for(int i=0;i<n;i++){
        String auth = "open";
        wifi_auth_mode_t enc = WiFi.encryptionType(i);
        switch(enc){
          case WIFI_AUTH_WEP: auth = "WEP"; break;
          case WIFI_AUTH_WPA_PSK: auth = "WPA"; break;
          case WIFI_AUTH_WPA2_PSK: auth = "WPA2"; break;
          case WIFI_AUTH_WPA_WPA2_PSK: auth = "WPA/WPA2"; break;
          case WIFI_AUTH_WPA3_PSK: auth = "WPA3"; break;
          case WIFI_AUTH_WPA2_WPA3_PSK: auth = "WPA2/WPA3"; break;
          default: auth = "open"; break;
        }
        json += "{\"ssid\":\""+jsonEscape(WiFi.SSID(i))+"\",";
        json += "\"rssi\":"+String(WiFi.RSSI(i))+",";
        json += "\"auth\":\""+auth+"\"}";
        if(i < n-1) json += ",";
      }
      json += "]";
      WiFi.scanDelete();
      r->send(200, "application/json", json);
    });

    server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *r){
      r->send(200, "text/plain", "Restarting");
      r->client()->stop();
      delay(100);
      ESP.restart();
    });


    server.on("/graphData", HTTP_GET, [](AsyncWebServerRequest *r){
      String series = "main";
      if(r->hasParam("series")) series = r->getParam("series")->value();
      const vector<GraphPoint> *points = &graphPoints;
      if(series != "main"){
        static const vector<GraphPoint> emptySeries;
        auto it = customGraphSeries.find(series);
        if(it != customGraphSeries.end() && !it->second.empty()) points = &it->second;
        else points = &emptySeries;
      }
      String s="[";
      for(int i=0;i<points->size();i++){
        s+="{\"time\":\""+(*points)[i].time+"\",\"value\":"+String((*points)[i].value)+"}";
        if(i<points->size()-1) s+=",";
      }
      s+="]";
      r->send(200,"application/json",s);
    });

server.on("/getImage", HTTP_GET, [](AsyncWebServerRequest *r){
    String path = (jpg == 1) ? "/anim1.gif" : "/anim2.gif";
    Serial.println("GET IMAGE -> " + path);

    if(!SPIFFS.exists(path)){
        Serial.println("Image not found: " + path);
        r->send(404, "text/plain", "Image not found");
        return;
    }

    File f = SPIFFS.open(path, "r");
    if(!f){
        Serial.println("Failed to open file: " + path);
        r->send(500, "text/plain", "Failed to open file");
        return;
    }

    Serial.printf("Serving file %s, size %u bytes\n", path.c_str(), f.size());
    r->send(SPIFFS, path, "image/gif");
    f.close();
});


    // ---------------- SET JPG (??????????? jpg ??????????) ----------------
    server.on("/setjpg", HTTP_GET, [](AsyncWebServerRequest *r){
      if(r->hasParam("val")){
        String v = r->getParam("val")->value();
        int newv = v.toInt();
        if(newv != 1 && newv != 2) newv = 1;
        jpg = newv;
        saveValue<int>("jpg", jpg);
        Serial.printf("jpg set to %d\n", jpg);
        r->send(200, "text/plain", "OK");
      } else {
        r->send(400, "text/plain", "Missing val");
      }
    });

    // ?????? ?????? ???? ? ?????? (???????????)
    server.serveStatic("/img1.jpg", SPIFFS, "/img1.jpg");
    server.serveStatic("/img2.jpg", SPIFFS, "/img2.jpg");

    server.begin();
  }
} dash;
