
//////////////////////////////////////////////*************slow()**************///////////////////////////////////////////////////////////////
//////////////////////////////////////////////*************slow()**************///////////////////////////////////////////////////////////////
//////////////////////////////////////////////*************slow()**************///////////////////////////////////////////////////////////////
int flag_slow = 0; //Флаг общей отправки данных обратной связи
int period_slow_Time = 10000; //Период обновления данных - зависит от "Nx_dim_id" nекущеuj - считанного значения яркости Nextion
int flag_WiFi = 1; // для switch - вывод информации в одно поле поочерди 

void slow(int interval){ // Обратная связь для редкого обновления данных - раз в XXX сек 
  static unsigned long timer=0;
  if (interval + timer > millis()) return;
  timer = millis();
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------

Nx_page_id = myNex.readNumber("dp"); //Текущий номер страницы открыты на Nextion

Nx_dim_id = myNex.readNumber("dim"); //Текущее - считанное значение яркости Nextion экрана для изменеия скорости обновления данных на экране

if(Nx_dim_id < 20 /*&& !Act_PH && !Act_Cl*/) {period_slow_Time = 20000;//} //меняем скорость обновления данных
// else if (Act_PH || Act_Cl){
// period_slow_Time = 1000;
 }else{
period_slow_Time = 10000;
}  


//Если находимся на "page0" то обновляем компоненты только на этой страницу
if(Nx_page_id == 0){
  myNex.writeNum("page0.b0.pic", Lamp ? 2 : 1);
  //jee.var("Lamp", Lamp ? "true" : "false"); 
  myNex.writeNum("page0.b12.pic", Pow_WS2815 ? 2 : 1);
  //jee.var("Pow_WS2815", Pow_WS2815 ? "true" : "false");

  myNex.writeNum("page0.b4.pic", Power_Heat ? 10 : 9); //Обновляем включено или выключено в Nextion   
  //jee.var("Sider_heat", String(Sider_heat));
  myNex.writeNum("page0.b3.pic", Power_Topping ? 8 : 7); //Уровень и долив воды
  //jee.var("Power_Topping", Power_Topping ? "true" : "false");

  
  myNex.writeNum("page0.b5.pic", Power_H2O2 ? 12 : 11); //Дозация перекеси водороода 
  myNex.writeStr("page0.b5.txt", /*String(Info_H2O2).substring(6)*/Info_H2O2); // Установить текст кнопки после XXXго символа

 
  myNex.writeNum("page0.b6.pic", Power_ACO ? 12 : 11); //Дозация Активное Каталитическое Окисление «Active Catalytic Oxidation» ACO
  myNex.writeStr("page0.b6.txt", Info_ACO); // Установить текст кнопки после XXXго символа
  //myNex.writeNum("page0.b7.pic", Power_APF ? 12 : 11); //Высокоэффективный коагулянт и флокулянт «All Poly Floc» APF
  myNex.writeStr("page0.b7.txt", "Cl: " + String(ppmCl)); //Содержание хлора
  myNex.writeStr("page0.b8.txt", "PH: " + String(PH)); //Кислотность воды

  // myNex.writeStr("page0.t0.txt", String(DS1) + " C"); // Температура в бассейне
  // myNex.writeStr("page0.t1.txt", String(DS2) + " C"); // Температура после нагревателя - падача нагретой воды в бассейн
  // myNex.writeStr("page0.t2.txt", String(DS1+30) + " C"); // Температура теплоносителя в нагреватель для пологрева воды

  myNex.writeNum("page0.b1.pic", Power_Filtr ? 4 : 3); // Фильтрация в бассейне -  Включение
  //jee.var("Power_Filtr", Power_Filtr ? "true" : "false");

  myNex.writeNum("page0.b2.pic", Power_Clean ? 6 : 5); //Промывка фильтра
  //jee.var("Power_Clean", Power_Clean ? "true" : "false");

}

//Если находимся на странице "heat"
if(Nx_page_id == 5){
  myNex.writeNum("heat.sw0.val", Activation_Heat);  //Отправляем отктивирован ли режим контроля температуры
  
  // myNex.writeStr("heat.t1.txt", String(DS1) + " C"); // Температура в бассейне  - перед нагревателем
  // myNex.writeStr("heat.t0.txt", String(DS2) + " C"); // Температура после нагревателя - падача нагретой воды в бассейн
  // myNex.writeStr("heat.t2.txt", String(DS1+30) + " C"); // Температура теплоносителя в нагреватель для пологрева воды
}



//Если находимся на странице "RTC"
if(Nx_page_id == 7){
// наверное не надо лишний раз отправлят часовой пояс - только при изминении - myNex.writeNum("pageRTC.n5.val", gmtOffset_correct); //Отправляем на Nextion монитора установленный часовой пояс

}




//Если находимся на странице "Service"
if(Nx_page_id == 8){


switch (flag_WiFi) {

  case 0:
  // WiFiset = "STA: " + String(jee.param("ssid1")) + "/"+ String(WiFi.RSSI()) + ", Pass: " + String(jee.param("pass1")) + ", IP: " + WiFi.localIP().toString();

  // myNex.writeStr("Service.t0.txt", WiFiset);
  flag_WiFi++;
  break;

  case 1:
  // if (WiFi.getMode() != WIFI_STA) {
  // WiFiset = "  AP: " + String(jee.param("ap_ssid")) + ", Pass: " +  String(jee.param("ap_pass")) + ", IP: " + WiFi.softAPIP().toString();
  // myNex.writeStr("Service.t0.txt", WiFiset);
  // }
  flag_WiFi++;
  break;

  case 2:
  // myNex.writeStr("Service.t0.txt", getStatusText(WiFi.status()));
  flag_WiFi=0;
  break;
}
  }

//Если находимся на странице "Dispensers"
if(Nx_page_id == 9){

  myNex.writeStr("Dispensers.t0.txt", "PH: " + String(PH)); delay(20);          //Кислотность воды
  myNex.writeStr("Dispensers.t1.txt", "Cl: " + String(ppmCl)); //Содержание хлора
  
  //myNex.writeNum("Dispensers.sw1.val", Test_Pump? 1 : 0); //Тест работы перельстатического насоса
  myNex.writeNum("Dispensers.cb0.val", ACO_Work-1); // Как часто включается перельстатический насос

  //myNex.writeNum("Dispensers.sw3.val", Activation_Timer_H2O2? 1 : 0);//Работа перельстатического насоса
  myNex.writeNum("Dispensers.cb1.val", H2O2_Work-1); // Как часто включается перельстатический насос

}



//Даже если не находися на нужной странице - по очереди обновляем компоненты необходимые для работы
//Pазбиваем на пакеты отправки данных, чтобы не перегружать страницу
// IPAddress ip;//Инициализируем

switch (flag_slow) {

  case 0:

  //Периодически считывам значение уставки темепературы - на случай ошибки чтения при пердыдущем запросе
  // jee.var("Power_Heat", String(Power_Heat));                //Обновляем состояние включения обогрева на Web странице
  // jee.var("Heat", String(Power_Heat ? "Нагрев" : "Откл.")+ ", " + "T:" + String(DS1)+ ", " + "T_s:" + String(Sider_heat));
  // jee.var("DS1", String(DS1));
  // jee.var("DS2", String(DS2));

  
  flag_slow++;
  break; 
  case 1: 

  // ip = WiFi.localIP();
  // ipAddress = ip.toString();
  // jee.var("RSSI_WiFi", "IP адрес: " + ipAddress + ", Сигнал: " + String(WiFi.RSSI()) + ", Host: http://" + HostName); // выводим мощность принимаемого сигнала
  
  flag_slow++;
  break;
  case 2: 

  //Lumen_Ul = readAnalog(36);
  Lumen_Ul = analogRead(3); // GPIO1  GPIO2  GPIO3  GPIO4  GPIO5  GPIO6  GPIO7  GPIO8  GPIO9  GPIO10
  //jee.var("Lumen_Ul", String(Lumen_Ul));
  // if(Lumen_Ul != Saved_Lumen_Ul) {Saved_Lumen_Ul = Lumen_Ul;
  Lumen_Ul  = map(Lumen_Ul, 4095, 0, 100, 0); // Переводим диаппазон люменов в диаппазон процентов

  InfoString2 = "Освещенность = " + String(Lumen_Ul) + " %";  

    //  jee.var("Lumen_Ul", ""); //освещенность на улице в процентах
    //  delay(5);
    //  jee.var("Lumen_Ul", String(Lumen_Ul_percent)); 
    // } //освещенность на улице в процентах

  flag_slow++;
  break;

  case 3: 

  // jee.var("info", ""); 
     
  flag_slow++;
  break;



  case 4:


  flag_slow=0;
  break;

    }

// Медленная поочередная синхронизация с Nextion на случай рассинхронизации.
// Отправляем небольшими порциями, чтобы не перегружать канал связи и CPU.
static uint8_t sync_step = 0; // ync_step объявлен как static, чтобы его значение сохранялось между вызовами slow()
const bool onSetLampPage = (Nx_page_id == 1); // Страница настройки лампы активна
const bool onSetRgbPage = (Nx_page_id == 2); // Страница настройки RGB активна
const bool onSetFiltrPage = (Nx_page_id == 3); // Страница таймеров фильтрации активна
const bool onCleanPage = (Nx_page_id == 4); // Страница таймера промывки активна
const bool onHeatPage = (Nx_page_id == 5); // Страница нагрева активна
const bool onDispensersPage = (Nx_page_id == 9); // Страница дозаторов активна


Lamp          = ReadRelayArray[0];
Pow_WS2815    = ReadRelayArray[1];
Power_Filtr  = ReadRelayArray[8];
Power_Clean  = ReadRelayArray[3];
Power_Heat   = ReadRelayArray[4];
Power_H2O2   = ReadRelayArray[5];
Power_ACO    = ReadRelayArray[6];
Power_Topping= ReadRelayArray[10];


switch (sync_step) {
  case 0:
    myNex.writeNum("page0.b0.pic", Lamp ? 2 : 1);
    myNex.writeNum("page0.b12.pic", Pow_WS2815 ? 2 : 1);
    sync_step++;
    break;
  case 1:
    myNex.writeNum("page0.b4.pic", Power_Heat ? 10 : 9);
    myNex.writeNum("page0.b3.pic", Power_Topping ? 8 : 7);
    sync_step++;
    break;
  case 2:
    myNex.writeNum("page0.b1.pic", Power_Filtr ? 4 : 3);
    myNex.writeNum("page0.b2.pic", Power_Clean ? 6 : 5);
    sync_step++;
    break;
  case 3:
    myNex.writeNum("page0.b5.pic", Power_H2O2 ? 12 : 11);
    myNex.writeStr("page0.b5.txt", Info_H2O2);
    sync_step++;
    break;
  case 4:
    myNex.writeNum("page0.b6.pic", Power_ACO ? 12 : 11);
    myNex.writeStr("page0.b6.txt", Info_ACO);
    sync_step++;
    break;
  case 5:
    myNex.writeStr("page0.b7.txt", "Cl: " + String(ppmCl));
    myNex.writeStr("page0.b8.txt", "PH: " + String(PH));
    sync_step++;
    break;
   case 6:
    if (!onHeatPage) { // Не перезаписываем уставку при открытом экране нагрева
      myNex.writeNum("heat.sw0.val", Activation_Heat);
      myNex.writeNum("heat.h0.val", Sider_heat);
    }
    sync_step++;
    break;
  case 7:
    if (!onHeatPage) { // Не мешаем редактированию уставки нагрева
      myNex.writeNum("heat.n0.val", Sider_heat);
    }
    if (!onDispensersPage) { // Не перезаписываем настройки дозаторов
      myNex.writeNum("Dispensers.cb0.val", ACO_Work - 1);
    }
    sync_step++;
    break;
  case 8:
    if (!onDispensersPage) { // Не перезаписываем настройки дозаторов
      myNex.writeNum("Dispensers.cb1.val", H2O2_Work - 1);
      myNex.writeNum("Dispensers.sw0.val", PH_Control_ACO ? 1 : 0);
    }
    sync_step++;
    break;
  case 9:
    if (!onDispensersPage) { // Не перезаписываем настройки дозаторов
      myNex.writeNum("Dispensers.sw2.val", NaOCl_H2O2_Control ? 1 : 0);
    }
    if (!onSetLampPage) { // Не мешаем редактированию таймера лампы
      myNex.writeNum("set_lamp.sw3.val", Lamp ? 1 : 0);
    }
    sync_step++;
    break;
  case 10:
    if (!onSetLampPage) { // Не мешаем редактированию таймера лампы
      myNex.writeNum("set_lamp.sw1.val", Lamp_autosvet ? 1 : 0);
      myNex.writeNum("set_lamp.sw0.val", Power_Time1 ? 1 : 0);
    }
    sync_step++;
    break;
  case 11: {
    if (!onSetLampPage) { // Не мешаем редактированию времени лампы
      UITimerEntry &lampTimer = ui.timer("LampTimer");
      String lampOnStr = formatMinutesToTime(lampTimer.on);
      myNex.writeNum("set_lamp.n0.val", getSubstring(lampOnStr, 0, 1));
      myNex.writeNum("set_lamp.n1.val", getSubstring(lampOnStr, 3, 4));
    }
    sync_step++;
    break;
  }
  case 12: {
    if (!onSetLampPage) { // Не мешаем редактированию времени лампы
      UITimerEntry &lampTimer = ui.timer("LampTimer");
      String lampOffStr = formatMinutesToTime(lampTimer.off);
      myNex.writeNum("set_lamp.n2.val", getSubstring(lampOffStr, 0, 1));
      myNex.writeNum("set_lamp.n3.val", getSubstring(lampOffStr, 3, 4));
    }
    sync_step++;
    break;
  }
  case 13:
    if (!onSetRgbPage) { // Не мешаем редактированию настроек RGB
      myNex.writeNum("set_RGB.sw3.val", Pow_WS2815 ? 1 : 0);
      myNex.writeNum("set_RGB.sw2.val", Pow_WS2815_autosvet ? 1 : 0);
    }
    sync_step++;
    break;
  case 14:
    if (!onSetRgbPage) { // Не мешаем редактированию таймера RGB
      myNex.writeNum("set_RGB.sw0.val", WS2815_Time1 ? 1 : 0);
    }
    sync_step++;
    break;
  case 15: {
    if (!onSetRgbPage) { // Не мешаем редактированию времени RGB
      UITimerEntry &rgbTimer = ui.timer("RgbTimer");
      String rgbOnStr = formatMinutesToTime(rgbTimer.on);
      myNex.writeNum("set_RGB.n0.val", getSubstring(rgbOnStr, 0, 1));
      myNex.writeNum("set_RGB.n1.val", getSubstring(rgbOnStr, 3, 4));
    }
    sync_step++;
    break;
  }
  case 16: {
    if (!onSetRgbPage) { // Не мешаем редактированию времени RGB
      UITimerEntry &rgbTimer = ui.timer("RgbTimer");
      String rgbOffStr = formatMinutesToTime(rgbTimer.off);
      myNex.writeNum("set_RGB.n2.val", getSubstring(rgbOffStr, 0, 1));
      myNex.writeNum("set_RGB.n3.val", getSubstring(rgbOffStr, 3, 4));
    }
    sync_step++;
    break;
  }
  case 17:
    if (!onSetFiltrPage) { // Не мешаем редактированию таймеров фильтрации
      myNex.writeNum("set_filtr.sw3.val", Power_Filtr ? 1 : 0);
      myNex.writeNum("set_filtr.sw0.val", Filtr_Time1 ? 1 : 0);
    }
    sync_step++;
    break;
  case 18:
    if (!onSetFiltrPage) { // Не мешаем редактированию таймеров фильтрации
      myNex.writeNum("set_filtr.sw2.val", Filtr_Time2 ? 1 : 0);
      myNex.writeNum("set_filtr.sw1.val", Filtr_Time3 ? 1 : 0);
    }
    sync_step++;
    break;
  case 19: {
    if (!onSetFiltrPage) { // Не мешаем редактированию времени фильтра №1
      UITimerEntry &filtrTimer1 = ui.timer("FiltrTimer1");
      String filtrOn1Str = formatMinutesToTime(filtrTimer1.on);
      myNex.writeNum("set_filtr.n0.val", getSubstring(filtrOn1Str, 0, 1));
      myNex.writeNum("set_filtr.n1.val", getSubstring(filtrOn1Str, 3, 4));
    }
    sync_step++;
    break;
  }
  case 20: {
    if (!onSetFiltrPage) { // Не мешаем редактированию времени фильтра №1
      UITimerEntry &filtrTimer1 = ui.timer("FiltrTimer1");
      String filtrOff1Str = formatMinutesToTime(filtrTimer1.off);
      myNex.writeNum("set_filtr.n2.val", getSubstring(filtrOff1Str, 0, 1));
      myNex.writeNum("set_filtr.n3.val", getSubstring(filtrOff1Str, 3, 4));
    }
    sync_step++;
    break;
  }
  case 21: {
    if (!onSetFiltrPage) { // Не мешаем редактированию времени фильтра №2
      UITimerEntry &filtrTimer2 = ui.timer("FiltrTimer2");
      String filtrOn2Str = formatMinutesToTime(filtrTimer2.on);
      myNex.writeNum("set_filtr.n4.val", getSubstring(filtrOn2Str, 0, 1));
      myNex.writeNum("set_filtr.n5.val", getSubstring(filtrOn2Str, 3, 4));
    }
    sync_step++;
    break;
  }
  case 22: {
    if (!onSetFiltrPage) { // Не мешаем редактированию времени фильтра №2
      UITimerEntry &filtrTimer2 = ui.timer("FiltrTimer2");
      String filtrOff2Str = formatMinutesToTime(filtrTimer2.off);
      myNex.writeNum("set_filtr.n6.val", getSubstring(filtrOff2Str, 0, 1));
      myNex.writeNum("set_filtr.n7.val", getSubstring(filtrOff2Str, 3, 4));
    }
    sync_step++;
    break;
  }
  case 23: {
    if (!onSetFiltrPage) { // Не мешаем редактированию времени фильтра №3
      UITimerEntry &filtrTimer3 = ui.timer("FiltrTimer3");
      String filtrOn3Str = formatMinutesToTime(filtrTimer3.on);
      myNex.writeNum("set_filtr.n8.val", getSubstring(filtrOn3Str, 0, 1));
      myNex.writeNum("set_filtr.n9.val", getSubstring(filtrOn3Str, 3, 4));
    }
    sync_step++;
    break;
  }
  case 24: {
    if (!onSetFiltrPage) { // Не мешаем редактированию времени фильтра №3
      UITimerEntry &filtrTimer3 = ui.timer("FiltrTimer3");
      String filtrOff3Str = formatMinutesToTime(filtrTimer3.off);
      myNex.writeNum("set_filtr.n10.val", getSubstring(filtrOff3Str, 0, 1));
      myNex.writeNum("set_filtr.n11.val", getSubstring(filtrOff3Str, 3, 4));
    }
    sync_step++;
    break;
  }
  case 25:
    if (!onCleanPage) { // Не мешаем редактированию таймера промывки
      myNex.writeNum("Clean.sw1.val", Power_Clean ? 1 : 0);
      myNex.writeNum("Clean.sw0.val", Clean_Time1 ? 1 : 0);
    }
    sync_step++;
    break;
  case 26: {
    if (!onCleanPage) { // Не мешаем редактированию времени промывки
      UITimerEntry &cleanTimer = ui.timer("CleanTimer1");
      String cleanOnStr = formatMinutesToTime(cleanTimer.on);
      myNex.writeNum("Clean.n0.val", getSubstring(cleanOnStr, 0, 1));
      myNex.writeNum("Clean.n1.val", getSubstring(cleanOnStr, 3, 4));
    }
    sync_step++;
    break;
  }
  case 27: {
    if (!onCleanPage) { // Не мешаем редактированию времени промывки
      UITimerEntry &cleanTimer = ui.timer("CleanTimer1");
      String cleanOffStr = formatMinutesToTime(cleanTimer.off);
      myNex.writeNum("Clean.n2.val", getSubstring(cleanOffStr, 0, 1));
      myNex.writeNum("Clean.n3.val", getSubstring(cleanOffStr, 3, 4));
    }
    sync_step++;
    break;
  }
  case 28:
    if (!onCleanPage) { // Не мешаем редактированию дней промывки
      myNex.writeNum("Clean.bt0.val", chk1 ? 1 : 0);
      myNex.writeNum("Clean.bt1.val", chk2 ? 1 : 0);
    }
    sync_step++;
    break;
  case 29:
    if (!onCleanPage) { // Не мешаем редактированию дней промывки
      myNex.writeNum("Clean.bt2.val", chk3 ? 1 : 0);
      myNex.writeNum("Clean.bt3.val", chk4 ? 1 : 0);
    }
    sync_step++;
    break;
  case 30:
    if (!onCleanPage) { // Не мешаем редактированию дней промывки
      myNex.writeNum("Clean.bt4.val", chk5 ? 1 : 0);
      myNex.writeNum("Clean.bt5.val", chk6 ? 1 : 0);
    }
    sync_step++;
    break;
  case 31:
    if (!onCleanPage) { // Не мешаем редактированию дней промывки
      myNex.writeNum("Clean.bt6.val", chk7 ? 1 : 0);
    }
    sync_step = 0;
    break;
  default:
    sync_step = 0;
    break;
}

  }
