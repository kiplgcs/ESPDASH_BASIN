
//////////////////////////////////////////////*************slow()**************///////////////////////////////////////////////////////////////
//////////////////////////////////////////////*************slow()**************///////////////////////////////////////////////////////////////
//////////////////////////////////////////////*************slow()**************///////////////////////////////////////////////////////////////
int flag_slow = 0; //Флаг общей отправки данных обратной связи
int period_slow_Time = 2000; //Период обновления данных - зависит от "Nx_dim_id" nекущеuj - считанного значения яркости Nextion
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
period_slow_Time = 2000;
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
  Lumen_Ul_percent = map(Lumen_Ul, 4095, 0, 100, 0); // Переводим диаппазон люменов в диаппазон процентов

  InfoString2 = "Освещенность = " + String(Lumen_Ul_percent) + " %";  

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



  }
