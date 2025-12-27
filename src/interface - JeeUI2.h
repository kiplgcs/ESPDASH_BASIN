//interface - JeeUI2
#pragma once

#include "ui - JeeUI2.h"

inline void interface(){
    oab.app("Система управления бассейном");

    oab.menu("Общая информация по бассейну");
    oab.menu("Controls");
    oab.menu("Настройка фильтрации, Промывка фильтра");
    oab.menu("Управление лампой");
    oab.menu("Управление RGB подсветкой");
    oab.menu("Контроль уровня воды");
    oab.menu("Контроль температуры");
    oab.menu("Контроль PH (NaOCl)");
    oab.menu("Контроль хлора CL (ACO)");

    // Общая информация по бассейну
    oab.page();
    oab.image("Image1", "/anim1.gif", "width:40%;height:200; x:25%;y:50%;");

    oab.display("RandomVal", "Random Number", "0");
    oab.text("InfoString", InfoString, "x:10%;y:10%;fontSize:12;color:#00ff00");
    oab.text("InfoString1", InfoString1, "x:20%;y:50%;fontSize:12;color:#00ff00");
    oab.button("button1", "gray", "My Button");
    oab.button("button2", "gray", "My Button1");

    // Controls tab
    oab.page();
    
    oab.range("MotorSpeed", 0, 100, 1, "Motor Speed");
  
    oab.range("RangeSlider", 10, 40, 1, "Range Min-Max", true);
    oab.number("IntInput", "Enter Integer");
    oab.number("FloatInput", "Enter Float", true);
    oab.time("Timer1", "Start Time");
    oab.text("Comment", "Comment");

    // Настройка фильтрации, Промывка фильтра
    oab.page();
    oab.selectDays("DaysSelect", "Select Days");
    // oab.number("IntInput", "Enter Integer");
    // oab.number("FloatInput", "Enter Float", true);

    

    
    // Управление лампой
    oab.page();
    oab.text("InfoString2", InfoString2, "x:45%;y:1%;fontSize:22;color:#00ff00");
    oab.button("button_Lamp", "red", "Включить / Отключить : Лампу в бассейне ");
    oab.option("button_Lamp", "Лампа отключена постоянно");
    oab.option("button_Lamp", "Лампа включена постоянно");
    oab.option("Lamp_autosvet", "Включение по датчику освещенности");
    oab.option("Power_Time1", "Включение по таймеру");
    oab.select("SetLamp", "Режим света");
    oab.text("Lumen_Ul", "Освещенность на улице, %");
    oab.checkbox("Power_Time1", "Таймер лампы"); //Галочка - активания/деактивация таймера
	oab.time("Lamp_timeON1", "Время включения по таймеру"); //Задать время включения часы/минуты
	oab.time("Lamp_timeOFF1", "Время отключения по таймеру"); //Задать время отключения часы/минуты

    // Управление RGB подсветкой
    oab.page();
    oab.button("button_WS2815", "gray", "Включить / Отключить : RGB ленту WS2815");
    oab.checkbox("WS2815_Time1", "Таймер RGB ленты"); //Галочка - активания/деактивация таймера
	oab.time("timeON_WS2815", "Время включения по таймеру"); //Задать время включения часы/минуты
	oab.time("timeOFF_WS2815", "Время отключения по таймеру"); //Задать время отключения часы/минуты
    oab.color("LEDColor", "Цвет подсветки");
    oab.option("auto", "Автоматически");
    oab.option("manual", "Ручной цвет");
    oab.select("LedColorMode", "Режим цвета");
    oab.range("LedBrightness", 10, 255, 1, "Яркость");
    oab.option("rainbow", "Радуга");
    oab.option("pulse", "Пульс");
    oab.option("chase", "Шлейф");
    oab.option("comet", "Комета");
    oab.option("color_wipe", "Цветовая заливка");
    oab.option("theater_chase", "Театр");
    oab.option("scanner", "Сканер");
    oab.option("sparkle", "Искры");
    oab.option("twinkle", "Мерцание");
    oab.option("confetti", "Конфетти");
    oab.option("waves", "Волны");
    oab.option("breathe", "Дыхание");
    oab.option("firefly", "Светлячки");
    oab.option("ripple", "Рябь");
    oab.option("dots", "Бегущие точки");
    oab.option("gradient", "Градиент");
    oab.option("meteor", "Метеоры");
    oab.option("juggle", "Жонглирование");
    oab.option("aurora", "Северное сияние");
    oab.option("candy", "Карамель");
    oab.option("twirl", "Завихрение");
    oab.option("sparkle_trails", "Искровые шлейфы");
    oab.option("neon_flow", "Неоновый поток");
    oab.option("calm_sea", "Спокойное море");
    oab.select("LedPattern", "Режим подсветки");
    oab.range("LedAutoplayDuration", 5, 180, 5, "Смена режима (сек)");
    oab.option("1", "Автомат");
    oab.option("0", "Вручную");
    oab.select("LedAutoplay", "Автосмена");
    oab.option("GRB", "WS2815 / WS2812 (GRB)");
    oab.option("RGB", "WS2811 (RGB)");
    oab.option("GBR", "GBR");
    oab.option("RBG", "RBG");
    oab.option("BRG", "BRG");
    oab.option("BGR", "BGR");
    oab.select("LedColorOrder", "Порядок цветов ленты");



    // oab.time("Timer1", "Start Time");
    // oab.text("Comment", "Comment");

    // Контроль уровня воды
    oab.page();

   // Контроль температуры
    oab.page();
        oab.displayGraph("SpeedTrend", "Speed Trend",
        "value:Speed;updatePeriod_of_Time:180;updateStep:3;maxPoints:20;width:100%;height:240;"
        "xLabel:Time;yLabel:Speed;pointColor:#ffd166;lineColor:#4CAF50;"
        "lineWidth:1;pointRadius:3;smooth:false", Speed);

    // Контроль PH (NaOCl)
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

    //Контроль хлора CL (ACO)
    oab.page();
    oab.displayGraph("FloatTrend2", "Temperature2 Trend",
    "value:Temperatura;updatePeriod_of_Time:60;updateStep:5;maxPoints:30;width:100%;height:400;"
    "xLabel:Time;yLabel:Temperature;pointColor:#6b66ff;lineColor:#ff5e5e;"
    "lineWidth:1;pointRadius:3;smooth:false", Temperatura);

}
