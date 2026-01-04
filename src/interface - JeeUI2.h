//interface - JeeUI2
#pragma once

#include "ui - JeeUI2.h"

inline void interface(){ // Декларатиынве функции интерфейса
    UI_APP("🏊 Система управления бассейном");

    UI_MENU("📊 Общая информация по бассейну");
    UI_MENU("🧰 Controls");
    UI_MENU("🧹 Настройка фильтрации, Промывка фильтра");
    UI_MENU("💡 Управление лампой");
    UI_MENU("🌈 Управление RGB подсветкой");
    UI_MENU("📏 Контроль уровня воды");
    UI_MENU("🌡 Контроль температуры");
    UI_MENU("🧪 Контроль PH (NaOCl)");
    UI_MENU("🧴 Контроль хлора CL (ACO)");
    UI_MENU("🏠 Контроль температуры в помещении");
    UI_MENU("🚏 Уличное освещение");

    // Общая информация по бассейну
    UI_PAGE();
    UI_IMAGE("Image1", "/Basin.jpg", "width:150%;height:550; x:-30%;y:95%;");
    UI_IMAGE("FilterImage", "/anim1.gif", "width:200;height:200; x:23%;y:823%;");
    UI_TEXT("OverlayPoolTemp", OverlayPoolTemp, "x:55%;y:150;fontSize:14;color:#00ff00");
    UI_TEXT("OverlayHeaterTemp", OverlayHeaterTemp, "x:70%;y:150;fontSize:14;color:#00ff00");
    UI_TEXT("OverlayLevelUpper", OverlayLevelUpper, "x:36%;y:130;fontSize:13;color:#00ff00");
    UI_TEXT("OverlayLevelLower", OverlayLevelLower, "x:36%;y:170;fontSize:13;color:#00ff00");
   
    UI_TEXT("OverlayPh", OverlayPh, "x:680;y:250;fontSize:13;color:#00ff00");
    UI_TEXT("OverlayChlorine", OverlayChlorine, "x:780;y:250;fontSize:13;color:#00ff00");
    UI_TEXT("OverlayFilterState", OverlayFilterState, "x:300;y:400;fontSize:12;color:#00ff00");

    
    // Controls tab
    UI_PAGE();
    
    UI_RANGE("MotorSpeed", MotorSpeedSetting, 0, 100, 1, "⚙️ Motor Speed");
    UI_DUAL_RANGE_KEYS("RangeSlider", RangeMin, RangeMax, "RangeMin", "RangeMax", 10, 40, 1, "🎚️ Range Min-Max");
    UI_NUMBER("IntInput", IntInput, "🔢 Enter Integer", false);
    UI_NUMBER("FloatInput", FloatInput, "🔣 Enter Float", true);
    UI_TIME("Timer1", Timer1, "⏰ Start Time");
    UI_TEXT("Comment", Comment, "💬 Comment");

    UI_DISPLAY_INT("RandomVal", RandomVal, "🔢 Random Number");
  
    UI_BUTTON("button1", button1, "gray", "🔘 My Button");
    UI_BUTTON("button2", button2, "gray", "🔘 My Button1");

    static String PopupComment;
    UI_POPUP_BEGIN("DataEntry", "📝 Ввод данных", "🪟 Открыть окно");
    UI_TEXT("PopupComment", PopupComment, "💬 Комментарий");

    // UI_TEXT("InfoString", InfoString, "x:30%;y:40%;fontSize:12;color:#00ff00");
    // UI_TEXT("InfoString1", InfoString1, "x:70%;y:70%;fontSize:12;color:#00ff00");


    UI_POPUP_END();

    // Настройка фильтрации, Промывка фильтра
    UI_PAGE();
    UI_BUTTON("Power_Filtr", Power_Filtr, "gray", "🧽 Фильтрация (вручную)");
    // UI_CHECKBOX("Power_Filtr", Power_Filtr, "Фильтрация (вручную)");
    UI_CHECKBOX("Filtr_Time1", Filtr_Time1, "⏱️ Таймер фильтрации №1");
    UI_TIMER("FiltrTimer1", "⏱️ Таймер фильтрации №1", noopTimerCallback);
    UI_CHECKBOX("Filtr_Time2", Filtr_Time2, "⏱️ Таймер фильтрации №2");
    UI_TIMER("FiltrTimer2", "⏱️ Таймер фильтрации №2", noopTimerCallback);
    UI_CHECKBOX("Filtr_Time3", Filtr_Time3, "⏱️ Таймер фильтрации №3");
    UI_TIMER("FiltrTimer3", "⏱️ Таймер фильтрации №3", noopTimerCallback);
    UI_BUTTON("Power_Clean", Power_Clean, "gray", "🧼 Промывка фильтра (вручную)");
    // UI_CHECKBOX("Power_Clean", Power_Clean, "Промывка фильтра (вручную)");
    UI_CHECKBOX("Clean_Time1", Clean_Time1, "🗓️ Таймер промывки");
    UI_TIMER("CleanTimer1", "🗓️ Таймер промывки", noopTimerCallback);
    UI_SELECT_DAYS("DaysSelect", DaysSelect, "📅 Дни промывки");

    

    
    // Управление лампой
    UI_PAGE();
    UI_TEXT("InfoString2", InfoString2, "x:45%;y:1%;fontSize:22;color:#00ff00");
    UI_SELECT_CB("SetLamp", SetLamp, (std::initializer_list<UIOption>{{"off", "Лампа отключена постоянно"},
                                     {"on", "Лампа включена постоянно"},
                                     {"auto", "Включение по датчику освещенности (<20%)"},
                                     {"timer", "Включение по таймеру"}}), "💡 Режим света", onSetLampChange);
    // UI_NUMBER("Lumen_Ul", Lumen_Ul, "Освещенность на улице, %", false);
    static String Lumen_Ul_str = String(Lumen_Ul); 
    UI_TEXT("Lumen_Ul", Lumen_Ul_str, "🔆 Освещенность на улице, %");

    UI_TIMER("LampTimer", "⏲️ Таймер лампы", onLampTimerChange);

    // Управление RGB подсветкой
    UI_PAGE();
    UI_BUTTON_DEFAULT("button_WS2815", Pow_WS2815, "gray", "🌈 Включить / Отключить : RGB ленту WS2815", 1);
    UI_CHECKBOX("WS2815_Time1", WS2815_Time1, "⏲️ Таймер RGB ленты"); //Галочка - активания/деактивация таймера
    UI_SELECT_CB("SetRGB", SetRGB, (std::initializer_list<UIOption>{{"off", "RGB подсветка отключена постоянно"},
                                   {"on", "RGB подсветка включена постоянно"},
                                   {"auto", "Включение по датчику освещенности (<20%)"},
                                   {"timer", "Включение по таймеру"}}), "🎛️ Режим управления RGB подсветкой", onSetRgbChange);
    UI_TIMER("RgbTimer", "⏲️ Таймер RGB ленты", noopTimerCallback);
    UI_COLOR("LEDColor", LEDColor, "🎨 Цвет подсветки");
    UI_SELECT_CB("LedColorMode", LedColorMode, (std::initializer_list<UIOption>{{"auto", "Автоматически"},
                                               {"manual", "Ручной цвет"}}), "🎨 Режим цвета", onLedColorModeChange);
    UI_RANGE_CB("LedBrightness", LedBrightness, 10, 255, 1, "🔆 Яркость", onLedBrightnessChange);
    UI_SELECT("LedPattern", LedPattern, (std::initializer_list<UIOption>{{"rainbow", "Радуга"},
                                         {"pulse", "Пульс"},
                                         {"chase", "Шлейф"},
                                         {"comet", "Комета"},
                                         {"color_wipe", "Цветовая заливка"},
                                         {"theater_chase", "Театр"},
                                         {"scanner", "Сканер"},
                                         {"sparkle", "Искры"},
                                         {"twinkle", "Мерцание"},
                                         {"confetti", "Конфетти"},
                                         {"waves", "Волны"},
                                         {"breathe", "Дыхание"},
                                         {"firefly", "Светлячки"},
                                         {"ripple", "Рябь"},
                                         {"dots", "Бегущие точки"},
                                         {"gradient", "Градиент"},
                                         {"meteor", "Метеоры"},
                                         {"juggle", "Жонглирование"},
                                         {"aurora", "Северное сияние"},
                                         {"candy", "Карамель"},
                                         {"twirl", "Завихрение"},
                                         {"sparkle_trails", "Искровые шлейфы"},
                                         {"neon_flow", "Неоновый поток"},
                                         {"calm_sea", "Спокойное море"}}), "✨ Режим подсветки");
    UI_RANGE("LedAutoplayDuration", LedAutoplayDuration, 5, 180, 5, "⏳ Смена режима (сек)");
    UI_SELECT("LedAutoplay", LedAutoplay, (std::initializer_list<UIOption>{{"1", "Автомат"},
                                           {"0", "Вручную"}}), "🔁 Автосмена");
    UI_SELECT("LedColorOrder", LedColorOrder, (std::initializer_list<UIOption>{{"GRB", "WS2815 / WS2812 (GRB)"},
                                               {"RGB", "WS2811 (RGB)"},
                                               {"GBR", "GBR"},
                                               {"RBG", "RBG"},
                                               {"BRG", "BRG"},
                                               {"BGR", "BGR"}}), "🎚️ Порядок цветов ленты");




    // Контроль уровня воды
    oab.page();
    UI_CHECKBOX("Activation_Water_Level", Activation_Water_Level, "✅ Контроль уровня воды");
    UI_DISPLAY_BOOL("WaterLevelSensorUpper", WaterLevelSensorUpper, "🛟🔼 Датчик уровня (верхний)", "сработал уровень", "нет уровня");
    UI_DISPLAY_BOOL("WaterLevelSensorLower", WaterLevelSensorLower, "🛟🔽 Датчик уровня (нижний)", "сработал уровень", "нет уровня");
    UI_DISPLAY_BOOL("Power_Topping_State", Power_Topping_State, "🚰 Состояние соленоида долива воды", "✅ Включен", "⏹️ Откл.");
    UI_BUTTON("Power_Topping", Power_Topping, "gray", "🚰 Включить/Отключить соленоид долива воды");
          
    UI_TEXT("InfoStringDIN", InfoStringDIN, "x:50%;y:130%;fontSize:14;color:#00ff00;white-space:pre-line");

   
    // Контроль температуры
    UI_PAGE();
    UI_DISPLAY_FLOAT("DS1", DS1, "🌡 Температура воды, °C");
    UI_RANGE("Sider_heat", Sider_heat, 5, 30, 1, "🎯 Уставка нагрева");
    UI_NUMBER("Sider_heat", Sider_heat, "🎯 Уставка нагрева, °C", false);
    UI_CHECKBOX("Activation_Heat", Activation_Heat, "🔥 Контроль нагрева");
    UI_DISPLAY_BOOL("Power_Heat", Power_Heat, "♨️ Состояние нагрева", "🔥 Нагрев", "⏹️ Откл.");
    
    // UI_GRAPH_SOURCE("PoolTempTrend", "📈 Температура бассейна",
    // "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:40;width:100%;height:240;"
    // "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
    // "lineWidth:1;pointRadius:3;smooth:false", DS1);

    UI_GRAPH_SOURCE("FloatTrend3", "📈 Температура бассейна",
    "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:40;width:100%;height:240;"
    "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
    "lineWidth:1;pointRadius:3;smooth:false", DS1);
        

    // Контроль PH (NaOCl)
    UI_PAGE();
 

    UI_DISPLAY_FLOAT("PH", PH, "🧪 pH (текущее)");




    UI_DISPLAY_BOOL("Power_ACO", Power_ACO, "🧴 Дозатор ACO", "✅ Работа", "⏹️ Откл.");
    UI_CHECKBOX("PH_Control_ACO", PH_Control_ACO, "🧪 Контроль pH (ACO)");
    UI_NUMBER("PH_setting", PH_setting, "⬆️ Верхний предел pH", true);



   static const std::initializer_list<UIOption> dosingOptions{  {"1", "15 сек"},
                                                                {"2", "60 сек"},
                                                                {"3", "5 мин"},
                                                                {"4", "15 мин"},
                                                                {"5", "30 мин"},
                                                                {"6", "1 час"},
                                                                {"8", "2 часа"},
                                                                {"9", "3 часа"},
                                                                {"10", "4 часа"},
                                                                {"11", "6 часов"},
                                                                {"12", "8 часов"},
                                                                {"13", "12 часов"},
                                                                {"7", "24 часа"}};
       UI_SELECT("ACO_Work", ACO_Work, dosingOptions, "⏳ Период дозирования ACO");

        // График тренда измеренной температуры:
    //  - "FloatTrend1" — внутреннее имя источника данных (ID графика), которым библиотека связывает график с данными.
    //  - "Temperature1 Trend" — заголовок графика, показываемый в веб-интерфейсе.
    //  - Строка настроек:
    //      value:Temperatura          — имя переменной/ключа данных, выводимой на график.
    //      updatePeriod_of_Time:60    — максимальный период обновления, задаётся в минутах (значение конвертируется в мс и
    //                                     ограничивает выпадающий список Update Interval; отдельно добавляется пункт 1 секунда).
    //      updateStep:10               — шаг изменения периода обновления в выпадающем списке, задаётся в минутах (переводится в
    //                                     миллисекунды; минимальная опция в списке всё равно остаётся 1 секунда).
    //      maxPoints:30               — максимальное количество точек на графике по умолчанию и верхняя граница выбора в UI.
    //      width:100%                 — ширина графика относительно контейнера.
    //      height:240                 — высота графика в пикселях.
    //      xLabel:Time                — подпись оси X.
    //      yLabel:Temperature         — подпись оси Y.
    //      pointColor:#6b66ff         — цвет точек.
    //      lineColor:#ff5e5e          — цвет линии.
    //      lineWidth:1                — толщина линии.
    //      pointRadius:3              — радиус точек.
    //      smooth:false               — отключено сглаживание линий (ступенчатый вывод).
    //  - Temperatura — переменная-источник, из которой читается значение для построения графика.
    UI_GRAPH_SOURCE("FloatPH", "📊 PH воды",
    "value:PH;updatePeriod_of_Time:60;updateStep:5;maxPoints:50;width:100%;height:400;"
    "xLabel:Time;yLabel:PH;pointColor:#6b66ff;lineColor:#ff5e5e;"
    "lineWidth:1;pointRadius:3;smooth:false", PH);
        
     UI_POPUP_BEGIN("Cal_PH", "🧪 Калибровка датчика PH", "🪟 Открыть окно калибровки датчика PH");

    //var("PH_CAL", "PH"+ String(PH, 2) + " : " + String(analogValuePH_Comp)+"mV");
                UI_DISPLAY_INT("analogValuePH", analogValuePH_Comp, "📟 Данные с АЦП от датчика PH");
                // UI_NUMBER("PH_Min", PH1, "Min CAL PH1 (4.1)", true);
                // UI_NUMBER("PH_Max", PH2, "Max CAL PH2 (6.86)", true);
                UI_DUAL_RANGE_KEYS("Float_PH_Slider", PH1, PH2, "PH1_MIN", "PH2_MAX", 4.0, 10.0, 0.1, "🎚️ Range PH Min-Max");
                UI_NUMBER("PH1_CAL", PH1_CAL, "📉 АЦП_mV для PH1 (Примерно 3500)", false);
                UI_NUMBER("PH2_CAL", PH2_CAL, "📉 АЦП_mV для PH2 (Примерно 2900)", false);
                // UI_DUAL_RANGE_KEYS("Int_PH_Slider", PH1_CAL, PH2_CAL, "PH1_CAL", "PH2_CAL", 100.0, 5000.0, 1, "Range АЦП_mV Min-Max");

                UI_NUMBER("Temper_Reference", Temper_Reference, "🌡 Температура референсная", true);
                UI_NUMBER("Temper_PH", Temper_PH, "🌡 Измеренная тепература для компенасации измерения PH", true);

                UI_BUTTON("Power_H2O2_Button", Power_H2O2, "gray", "🧪 Проверка работы перельстатического насоса подачи кислоты (вручную)");
        UI_POPUP_END();


    //Контроль хлора CL (ACO)
    UI_PAGE();
    
    UI_DISPLAY_FLOAT("ppmCl", ppmCl, "🧴 Свободный хлор, мг/л");
    UI_DISPLAY_INT("corrected_ORP_Eh_mV", corrected_ORP_Eh_mV, "📟 ORP, мВ");
    UI_DISPLAY_BOOL("Power_H2O2", Power_H2O2, "🧴 Дозатор NaOCl", "✅ Работа", "⏹️ Откл.");
    UI_CHECKBOX("NaOCl_H2O2_Control", NaOCl_H2O2_Control, "🧪 Контроль хлора (NaOCl)");
    UI_NUMBER("ORP_setting", ORP_setting, "⬇️ Нижний предел ORP, мВ", false);
    UI_SELECT("H2O2_Work", H2O2_Work, dosingOptions, "⏳ Период дозирования NaOCl");

    UI_GRAPH_SOURCE("FloatСl", "📊 Хлор в воде, ppmCl",
    "value:ppmCl;updatePeriod_of_Time:60;updateStep:5;maxPoints:50;width:100%;height:400;"
    "xLabel:Time;yLabel:Хлор,мг/л;pointColor:#6b66ff;lineColor:#ff5e5e;"
    "lineWidth:1;pointRadius:3;smooth:false", ppmCl);

        UI_POPUP_BEGIN("CL", "Калибровка датчика хлора", "Открыть окно калибровки датчика CL хлора}");
            static String Cl_Cal_str = String(corrected_ORP_Eh_mV) + "-" + String(CalRastvor256mV) + "=" + String(CalRastvor256mV - corrected_ORP_Eh_mV);   
            UI_NUMBER("Cl_Cal", Cl_Cal_str, "📐 ORP - ORPCal = калибровочный коэффициент", true);
            UI_NUMBER("CalRastvor256mV", CalRastvor256mV, "🧪 ОВП калибровочного раствора - мВ", false);
            UI_NUMBER("Calibration_ORP_mV", Calibration_ORP_mV, "📏 Калибровочный коэффициент - мВ", false);
            UI_BUTTON("Power_ACO_Button", Power_ACO, "gray", "🧴 Проверка работы перельстатического насоса подачи хлора (вручную)");
        UI_POPUP_END();

    // Контроль температуры в помещении
    UI_PAGE();
    UI_DISPLAY_FLOAT("RoomTemp", DS1, "🌡 Температура в помещении, °C");
    UI_DUAL_RANGE_KEYS("RoomTempRange", RoomTempOn, RoomTempOff, "RoomTempOn", "RoomTempOff", 1.0, 30.0, 0.5, "🎚️ Включение/выключение обогрева, °C");
    UI_CHECKBOX("RoomTemper", RoomTemper, "✅ Контроль температуры в помещении");
    UI_DISPLAY_BOOL("Power_Warm_floor_heating", Power_Warm_floor_heating, "♨️ Обогрев пола", "🔥 Включен", "⏹️ Откл.");

    // Уличное освещение
    UI_PAGE();
    UI_BUTTON("Pow_Ul_light", Pow_Ul_light, "gray", "🚏 Включить/Отключить освещение вручную");
    UI_CHECKBOX("Ul_light_Time", Ul_light_Time, "⏱️ Таймер уличного освещения");
    UI_TIMER("UlLightTimer", "⏲️ Таймер уличного освещения", noopTimerCallback);
}
