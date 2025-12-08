//interface - JeeUI2
#pragma once

#include "ui - JeeUI2.h"

inline void interface(){
    oab.app("MiniDash");

    oab.menu("Main");
    oab.menu("Controls");
    oab.menu("Monitor");
    oab.menu("Advanced");
    oab.menu("Metrics");
    oab.menu("Signals");
    oab.menu("Hello");

    // Main tab
    oab.page();
    oab.display("CurrentTime", "Current Time", "--:--:--");
    oab.display("RandomVal", "Random Number", "0");
    oab.text("InfoString", InfoString, "x:44%;y:60%;fontSize:12;color:#00ff00");
    oab.text("InfoString1", InfoString1, "x:44%;y:90%;fontSize:12;color:#00ff00");
    oab.button("button1", "gray", "My Button");
    oab.button("button2", "gray", "My Button1");

    // Controls tab
    oab.page();
    oab.button("button3", "gray", "My Button3");
    oab.range("MotorSpeed", 0, 100, 1, "Motor Speed");
    oab.color("LEDColor", "LED Color");
    oab.option("Normal", "Normal");
    oab.option("Eco", "Eco");
    oab.option("Turbo", "Turbo");
    oab.select("ModeSelect", "Mode");
    oab.selectDays("DaysSelect", "Select Days");
    oab.range("RangeSlider", 10, 40, 1, "Range Min-Max", true);

    // Monitor tab
    oab.page();
    oab.number("IntInput", "Enter Integer");
    oab.number("FloatInput", "Enter Float", true);

    // Advanced tab
    oab.page();
    oab.time("Timer1", "Start Time");
    oab.text("Comment", "Comment");

   // Metrics tab
    oab.page();
        oab.displayGraph("SpeedTrend", "Speed Trend",
        "value:Speed;updatePeriod_of_Time:180;updateStep:3;maxPoints:20;width:100%;height:240;"
        "xLabel:Time;yLabel:Speed;pointColor:#ffd166;lineColor:#4CAF50;"
        "lineWidth:1;pointRadius:3;smooth:false", Speed);

    // Signals tab
    oab.page();

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
    oab.displayGraph("FloatTrend1", "Temperature1 Trend",
    "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:40;width:100%;height:240;"
    "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
    "lineWidth:1;pointRadius:3;smooth:false", Temperatura);

  
}
