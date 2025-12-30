//interface - JeeUI2
#pragma once

#include "ui - JeeUI2.h"

inline void interface(){
    UI_APP("Система управления бассейном");

    UI_MENU("Общая информация по бассейну");
    UI_MENU("Controls");
    UI_MENU("Настройка фильтрации, Промывка фильтра");
    UI_MENU("Управление лампой");
    UI_MENU("Управление RGB подсветкой");
    UI_MENU("Контроль уровня воды");
    UI_MENU("Контроль температуры");
    UI_MENU("Контроль PH (NaOCl)");
    UI_MENU("Контроль хлора CL (ACO)");

    // Общая информация по бассейну
    UI_PAGE();
    UI_IMAGE("Image1", "/anim1.gif", "width:40%;height:200; x:25%;y:50%;");

    UI_DISPLAY_INT("RandomVal", RandomVal, "Random Number");
    UI_TEXT("InfoString", InfoString, "x:10%;y:10%;fontSize:12;color:#00ff00");
    UI_TEXT("InfoString1", InfoString1, "x:20%;y:50%;fontSize:12;color:#00ff00");
    UI_BUTTON("button1", button1, "gray", "My Button");
    UI_BUTTON("button2", button2, "gray", "My Button1");

    // Controls tab
    UI_PAGE();
    
    UI_RANGE("MotorSpeed", MotorSpeedSetting, 0, 100, 1, "Motor Speed");
    UI_DUAL_RANGE_KEYS("RangeSlider", RangeMin, RangeMax, "RangeMin", "RangeMax", 10, 40, 1, "Range Min-Max");
    UI_NUMBER("IntInput", IntInput, "Enter Integer", false);
    UI_NUMBER("FloatInput", FloatInput, "Enter Float", true);
    UI_TIME("Timer1", Timer1, "Start Time");
    UI_TEXT("Comment", Comment, "Comment");

    // Настройка фильтрации, Промывка фильтра
    UI_PAGE();
    UI_CHECKBOX("Power_Filtr", Power_Filtr, "Фильтрация (вручную)");
    UI_CHECKBOX("Filtr_Time1", Filtr_Time1, "Таймер фильтрации №1");
    UI_TIMER("FiltrTimer1", "Таймер фильтрации №1", noopTimerCallback);
    UI_CHECKBOX("Filtr_Time2", Filtr_Time2, "Таймер фильтрации №2");
    UI_TIMER("FiltrTimer2", "Таймер фильтрации №2", noopTimerCallback);
    UI_CHECKBOX("Filtr_Time3", Filtr_Time3, "Таймер фильтрации №3");
    UI_CHECKBOX("Power_Clean", Power_Clean, "Промывка фильтра (вручную)");
    UI_CHECKBOX("Clean_Time1", Clean_Time1, "Таймер промывки");
    UI_TIMER("CleanTimer1", "Таймер промывки", noopTimerCallback);
    UI_SELECT_DAYS("DaysSelect", DaysSelect, "Дни промывки");

    

    
    // Управление лампой
    UI_PAGE();
    UI_TEXT("InfoString2", InfoString2, "x:45%;y:1%;fontSize:22;color:#00ff00");
    UI_SELECT_CB("SetLamp", SetLamp, (std::initializer_list<UIOption>{{"off", "Лампа отключена постоянно"},
                                     {"on", "Лампа включена постоянно"},
                                     {"auto", "Включение по датчику освещенности (<20%)"},
                                     {"timer", "Включение по таймеру"}}), "Режим света", onSetLampChange);
    UI_NUMBER("Lumen_Ul", Lumen_Ul, "Освещенность на улице, %", false);
    UI_TIMER("LampTimer", "Таймер лампы", onLampTimerChange);

    // Управление RGB подсветкой
    UI_PAGE();
    UI_BUTTON_DEFAULT("button_WS2815", Pow_WS2815, "gray", "Включить / Отключить : RGB ленту WS2815", 1);
    UI_CHECKBOX("WS2815_Time1", WS2815_Time1, "Таймер RGB ленты"); //Галочка - активания/деактивация таймера
    UI_SELECT_CB("SetRGB", SetRGB, (std::initializer_list<UIOption>{{"off", "RGB подсветка отключена постоянно"},
                                   {"on", "RGB подсветка включена постоянно"},
                                   {"auto", "Включение по датчику освещенности (<20%)"},
                                   {"timer", "Включение по таймеру"}}), "Режим управления RGB подсветкой", onSetRgbChange);
        UI_TIMER("RgbTimer", "Таймер RGB ленты", noopTimerCallback);
    UI_COLOR("LEDColor", LEDColor, "Цвет подсветки");
    UI_SELECT_CB("LedColorMode", LedColorMode, (std::initializer_list<UIOption>{{"auto", "Автоматически"},
                                               {"manual", "Ручной цвет"}}), "Режим цвета", onLedColorModeChange);
    UI_RANGE_CB("LedBrightness", LedBrightness, 10, 255, 1, "Яркость", onLedBrightnessChange);
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
                                         {"calm_sea", "Спокойное море"}}), "Режим подсветки");
    UI_RANGE("LedAutoplayDuration", LedAutoplayDuration, 5, 180, 5, "Смена режима (сек)");
    UI_SELECT("LedAutoplay", LedAutoplay, (std::initializer_list<UIOption>{{"1", "Автомат"},
                                           {"0", "Вручную"}}), "Автосмена");
    UI_SELECT("LedColorOrder", LedColorOrder, (std::initializer_list<UIOption>{{"GRB", "WS2815 / WS2812 (GRB)"},
                                               {"RGB", "WS2811 (RGB)"},
                                               {"GBR", "GBR"},
                                               {"RBG", "RBG"},
                                               {"BRG", "BRG"},
                                               {"BGR", "BGR"}}), "Порядок цветов ленты");




    // Контроль уровня воды
    oab.page();

   // Контроль температуры
    UI_PAGE();
    UI_DISPLAY_FLOAT("DS1", DS1, "Температура воды, °C");
    UI_RANGE("Sider_heat", Sider_heat, 5, 30, 1, "Уставка нагрева");
    UI_NUMBER("Sider_heat", Sider_heat, "Уставка нагрева, °C", false);
    UI_CHECKBOX("Activation_Heat", Activation_Heat, "Контроль нагрева");
    UI_DISPLAY_BOOL("Power_Heat", Power_Heat, "Состояние нагрева", "Нагрев", "Откл.");
    UI_GRAPH_SOURCE("PoolTempTrend", "Температура бассейна",
        "value:Temperatura;updatePeriod_of_Time:180;updateStep:3;maxPoints:20;width:100%;height:240;"
        "xLabel:Time;yLabel:Temperature;pointColor:#ffd166;lineColor:#4CAF50;"
        "lineWidth:1;pointRadius:3;smooth:false", Temperatura);
    // Контроль PH (NaOCl)
      UI_PAGE();

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
    UI_GRAPH_SOURCE("FloatTrend1", "Temperature1 Trend",
    "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:40;width:100%;height:240;"
    "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
    "lineWidth:1;pointRadius:3;smooth:false", Temperatura);

    //Контроль хлора CL (ACO)
    UI_PAGE();
    UI_GRAPH_SOURCE("FloatTrend2", "Temperature2 Trend",
    "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:30;width:100%;height:400;"
    "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
    "lineWidth:1;pointRadius:3;smooth:false", Temperatura);

}
