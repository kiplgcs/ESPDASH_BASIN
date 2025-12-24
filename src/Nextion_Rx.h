//https://wiki.iteadstudio.com/Nextion_Instruction_Set#wept:_write_data_in_hex_to_EEPROM_through_UART_-_Enhanced_Model_Only
//https://github.com/Seithan/EasyNextionLibrary
//https://nextion.tech/instruction-set/
//https://esphome.io/components/uart.html

void NextionDelay(int interval, int function);
bool triggerActivated_Nextion = false; // Флаг отложенного чтения из Nextion при сработке тригера
int Function_Nextion; //Исполняемая функция для отложенного чтения - привязана к "triggerActivated_Nextion"
int triggerRestartNextion = true; //Флаг функции для отложенного чтения всех необходимых переменных после перезагрузки ESP

//Работа  с монитором  Nextion через библиотеку - Easy Nextion Library

#include "EasyNextionLibrary.h"  // Include EasyNextionLibrary
#include <HardwareSerial.h>
HardwareSerial MySerial(1);
#define RXD1 6 //4 // 6 //8/15/16
#define TXD1 7 //5 //7 //9//14/17

EasyNex myNex(MySerial); // Create an object of EasyNex class with the name < myNex >
                       // Set as parameter the Hardware Serial you are going to use


//#define SERIAL_BUFFER_SIZE 2048 // Увеличьте размер буфера

//https://github.com/Seithan/EasyNextionLibrary
//const int LED_BUILTIN = 2; // Мигаем встроенным светодиодом на ESP32 при работе с Nextion
bool BUILTIN = false; // Для отслеживания состояния кнопки Nextion	

int flag = 1000; // для поочередного чтения с Nextion т.к. если много сразу отправлять в порт то ESP перезагружается.

int Nx_page_id = 0; //Текущий номер страницы открыты на Nextion
int Nx_dim_id = 50; //Текущее - считанное значение яркости Nextion экрана для изменеия скорости обновления данных на экране
int in_hours, in_minutes; char buffer[6];

// void ActivUARTInterrupt() {//Прерывание по Rx для получения данных от Nextion монитора - отключено потому что и так работает все хорошо.
//   myNex.NextionListen(); // Обработка данных при прерывании
// }


void trigger0(){
  //   // Чтобы вызвать эту пустоту, отправьте из события компонента Nextion: printh 23 02 54 00
  //  // В этом примере мы отправляем эту команду из события освобождения кнопки b0 (см. HMI этого примера)
  //  // Вы можете отправить одну и ту же команду `printh` для вызова одной и той же функции из более чем одного компонента, в зависимости от ваших потребностей

  //   Serial.print("Hello World");

  //   myNex.writeNum("b0.bco", 2016); // Установить цвет фона кнопки b0 на ЗЕЛЕНЫЙ (код цвета: 2016)
  //   //myNex.writeStr("b0.txt", "ON"); // Установить текст кнопки b0 на "ON"
  //   //myNex.writeNum("p1.pic", 1);    // Установить картинку 1 в качестве фоновой картинки для p0
  //   myNex.writeStr("t0.txt", "Hello World"); // Change t0 text to "Hello World"

  // BUILTIN = !BUILTIN; //Если BUILTIN включен, выключите его или наоборот
  // if(BUILTIN == true){
  //   myNex.writeNum("b0.bco", 2016); // Set button b0 background color to GREEN (color code: 2016)
  //   myNex.writeStr("b0.txt", "ON"); // Set button b0 text to "ON"
    
  // }else if(BUILTIN == false){
  //   myNex.writeNum("b0.bco", 63488); // Set button b0 background color to RED (color code: 63488)
  //   myNex.writeStr("b0.txt", "OFF"); // Set button b0 text to "ON"
  // }
}

void trigger1(){
//   // Чтобы вызвать эту функцию, отправьте из события компонента Nextion: printh 23 02 54 01
//    // В этом примере мы отправляем эту команду из события освобождения кнопки b1 (см. HMI этого примера)
//    // Вы можете отправить одну и ту же команду `printh` для вызова одной и той же функции из более чем одного компонента, в зависимости от ваших потребностей
//    myNex.writeStr("t1.txt", "Hello World"); // Change t0 text to "Hello World"

//   //if(digitalRead(LED_BUILTIN) == HIGH){
//    // digitalWrite(LED_BUILTIN, LOW);  // Запускаем функцию при выключенном светодиоде
//     myNex.writeNum("b1.bco", 63488); // Установить цвет фона кнопки b0 на КРАСНЫЙ (код цвета: 63488)
//     myNex.writeStr("b1.txt", "OFF"); // Установить текст кнопки b0 на «ВЫКЛ.»
//     myNex.writeNum("p1.pic", 0);    // Установить изображение 0 в качестве фонового изображения для компонента изображения p0
//   //}
  
//   //myNex.writeStr("t0.txt", String(rand(0, 255)));
//  // for(int i = 0; i < 10; i++){
//    // for(int x = 0; x < 10; x++){
//      // digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Если LED_BUILTIN включен, выключите его или наоборот
//      // delay(50);
//     //}
//     //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));   // Если LED_BUILTIN включен, выключите его или наоборот
//     //delay(500);
//     //digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));   // Если LED_BUILTIN включен, выключите его или наоборот
//   //}
//   myNex.writeStr("t1.txt", "LED STROBE\\rOFF"); // Установка текста текстового поля t0 на "LED STROBE OFF"
//                                                  // \\r — это символ новой строки для Nextion.
//                                                  // Текст будет выглядеть так:
//                                                  // 1-я строка: LED STROBE
//                                                  // 2-я строка: ВЫКЛ.                                               
}


/////////////////////////************* page set_lamp  **************/////////////////////////////
////////////////////////************* page set_lamp  **************//////////////////////////////
///////////////////////************* page set_lamp  **************///////////////////////////////

//printh 23 02 54 04 - "set_lamp" Присвоить все кнопки подсветки
void read_lamp_sw0_sw1_sw2(){

    Lamp = myNex.readNumber("set_lamp.sw3.val"); 
    // jee.var("Lamp", Lamp ? "true" : "false");
    // Error err = RS485.addRequest(40001,1,0x05,0, Lamp ? devices[0].value : devices[1].value);

    Power_Time1 = myNex.readNumber("set_lamp.sw0.val"); delay(50);
    // jee.var("Power_Time1", Power_Time1 ? "true" : "false"); //jee.var("Power_Time1", String(Power_Time1));

    Lamp_autosvet = myNex.readNumber("set_lamp.sw1.val"); delay(50);
    // jee.var("Lamp_autosvet", Lamp_autosvet ? "true" : "false"); //jee.var("Lamp_autosvet", String(Lamp_autosvet));
}
void trigger4(){ read_lamp_sw0_sw1_sw2();}


// //printh 23 02 54 02  - "set_lamp" Присвоить n0 / n1 время включенния подсветки
// void read_lamp_n0_n1(){
//     in_hours = myNex.readNumber("set_lamp.n0.val"); in_minutes = myNex.readNumber("set_lamp.n1.val");
//     //Lamp_timeON1 = String(hours / 10) + String(hours % 10) + ":" + String(minutes/ 10) + String(minutes % 10);
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Lamp_timeON1=Lamp_timeON1 = buffer;
//     jee.var("Lamp_timeON1", Lamp_timeON1);
// }
// void trigger2(){read_lamp_n0_n1();}

// //printh 23 02 54 03 - "set_lamp" Присвоить n2 / n3 время отключения подсветки
// void read_lamp_n2_n3(){
//     in_hours = myNex.readNumber("set_lamp.n2.val");  in_minutes = myNex.readNumber("set_lamp.n3.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Lamp_timeOFF1=Lamp_timeOFF1 = buffer;
//     jee.var("Lamp_timeOFF1", Lamp_timeOFF1);
// }
// void trigger3(){read_lamp_n2_n3();}


// /////////////////////////************* page set_RGB  **************/////////////////////////////
// ////////////////////////************* page set_RGB  **************//////////////////////////////
// ///////////////////////************* page set_RGB  **************///////////////////////////////

// //printh 23 02 54 05 -  "set-RGB" Присвоить n0 / n1 время включенния RGB ленты
// void read_RGB_n0_n1(){
//     in_hours = myNex.readNumber("set_RGB.n0.val"); in_minutes = myNex.readNumber("set_RGB.n1.val");  
//     //Lamp_timeON1 = String(hours / 10) + String(hours % 10) + ":" + String(minutes/ 10) + String(minutes % 10);
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_timeON_WS2815=timeON_WS2815 = buffer;
//     jee.var("timeON_WS2815", timeON_WS2815);
// }
// void trigger5(){read_RGB_n0_n1();}


// //printh 23 02 54 06 - Присвоить n2 / n3 время отключения RGB ленты
// void read_RGB_n2_n3(){
//     in_hours = myNex.readNumber("set_RGB.n2.val"); in_minutes = myNex.readNumber("set_RGB.n3.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_timeOFF_WS2815=timeOFF_WS2815 = buffer;
//     jee.var("timeOFF_WS2815", timeOFF_WS2815);
// }
// void trigger6(){read_RGB_n2_n3();}


// //printh 23 02 54 07 - "set-RGB" Присвоить все кнопки подсветки RGB ленты
// void read_RGB_sw0_sw2_sw3(){
//     Pow_WS28151 = Pow_WS2815 = myNex.readNumber("set_RGB.sw3.val"); delay(50);
//     jee.var("Pow_WS2815", Pow_WS2815 ? "true" : "false");
//     Error err = RS485.addRequest(40001,1,0x05,1, Pow_WS2815 ? devices[0].value : devices[1].value);

//     Saved_WS2815_Time1=WS2815_Time1 = myNex.readNumber("set_RGB.sw0.val"); delay(50);
//     jee.var("WS2815_Time1", WS2815_Time1 ? "true" : "false"); 

//     Saved_Pow_WS2815_autosvet = Pow_WS2815_autosvet = myNex.readNumber("set_RGB.sw2.val"); 
//     jee.var("Pow_WS2815_autosvet", Pow_WS2815_autosvet ? "true" : "false"); 
// }
// void trigger7(){read_RGB_sw0_sw2_sw3();}


// /////////////////////////************* page set_filtr **************/////////////////////////////
// ////////////////////////************* page set_filtr **************//////////////////////////////
// ///////////////////////************* page set_filtr **************///////////////////////////////


// //printh 23 02 54 08 - "set-filtr" Присвоить n0 / n1 время вкл. по таймеру №1
// void read_filtr_n0_n1(){
//     in_hours = myNex.readNumber("set_filtr.n0.val"); in_minutes = myNex.readNumber("set_filtr.n1.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeON1=Filtr_timeON1 = buffer;
//     jee.var("Filtr_timeON1", Filtr_timeON1);
// }
// void trigger8(){read_filtr_n0_n1();} 



// //printh 23 02 54 09 - "set-filtr" Присвоить n0 / n1 время откл. по таймеру №1
// void read_filtr_n2_n3(){
//     in_hours = myNex.readNumber("set_filtr.n2.val"); in_minutes = myNex.readNumber("set_filtr.n3.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeOFF1=Filtr_timeOFF1 = buffer;
//     jee.var("Filtr_timeOFF1", Filtr_timeOFF1);
// }
// void trigger9(){read_filtr_n2_n3();}



// //printh 23 02 54 0A - "set-filtr" Присвоить n4 / n5 время вкл. по таймеру №2
// void read_filtr_n4_n5(){
//     in_hours = myNex.readNumber("set_filtr.n4.val"); in_minutes = myNex.readNumber("set_filtr.n5.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeON2=Filtr_timeON2 = buffer;
//     jee.var("Filtr_timeON2", Filtr_timeON2);
// }
// void trigger10(){read_filtr_n4_n5();}


// //printh 23 02 54 0B - "set-filtr" Присвоить n6 / n7 время откл. по таймеру №2
// void read_filtr_n6_n7(){
//     in_hours = myNex.readNumber("set_filtr.n6.val"); in_minutes = myNex.readNumber("set_filtr.n7.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeOFF2=Filtr_timeOFF2 = buffer;
//     jee.var("Filtr_timeOFF2", Filtr_timeOFF2);
// }
// void trigger11(){read_filtr_n6_n7();}


// //printh 23 02 54 0C - "set-filtr" Присвоить n8 / n9 время вкл. по таймеру №3
// void read_filtr_n8_n9(){
//     in_hours = myNex.readNumber("set_filtr.n8.val"); in_minutes = myNex.readNumber("set_filtr.n9.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeON3=Filtr_timeON3 = buffer;
//     jee.var("Filtr_timeON3", Filtr_timeON3);
// }
// void trigger12(){read_filtr_n8_n9();} 

// //printh 23 02 54 0D - "set-filtr" Присвоить n10 / n11 время откл. по таймеру №3
// void read_filtr_n10_n11(){   
//     in_hours = myNex.readNumber("set_filtr.n10.val"); in_minutes = myNex.readNumber("set_filtr.n11.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeOFF3=Filtr_timeOFF3 = buffer;
//     jee.var("Filtr_timeOFF3", Filtr_timeOFF3);
// }
// void trigger13(){read_filtr_n10_n11();} 


// //printh 23 02 54 0E - "set-filtr" Присвоить все кнопки SW0, SW1, SW2
// void read_filtr_sw0_sw1_sw2(){
//     Saved_Filtr_Time1=Filtr_Time1 = myNex.readNumber("set_filtr.sw0.val");
//     jee.var("Filtr_Time1", Filtr_Time1 ? "true" : "false"); 

//     Saved_Filtr_Time2=Filtr_Time2 = myNex.readNumber("set_filtr.sw2.val");
//     jee.var("Filtr_Time2", Filtr_Time2 ? "true" : "false"); 

//     Saved_Filtr_Time3=Filtr_Time3 = myNex.readNumber("set_filtr.sw1.val");
//     jee.var("Filtr_Time3", Filtr_Time3 ? "true" : "false"); 

//     Power_Filtr1=Power_Filtr = myNex.readNumber("set_filtr.sw3.val");
//     jee.var("Power_Filtr", Power_Filtr ? "true" : "false");

//     Error err = RS485.addRequest(40001,1,0x05,8, Power_Filtr ? devices[0].value : devices[1].value);
// }
// void trigger14(){read_filtr_sw0_sw1_sw2();}


// /////////////////////////************* page Clean **************/////////////////////////////
// ////////////////////////************* page Clean **************//////////////////////////////
// ///////////////////////************* page Clean **************///////////////////////////////

// //printh 23 02 54 0F - "Clean"  считываем время таймера n0 / n1  насала промывки
// void read_Clean_n0_n1(){
//     in_hours = myNex.readNumber("Clean.n0.val"); in_minutes = myNex.readNumber("Clean.n1.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Clean_timeON1=Clean_timeON1 = buffer;
//     jee.var("Clean_timeON1", Clean_timeON1);
// }
// void trigger15(){read_Clean_n0_n1();}


// //printh 23 02 54 10 - "Clean" считываем время таймера n2 / n3  отключения промывки
// void read_Clean_n2_n3(){
//     in_hours = myNex.readNumber("Clean.n2.val"); in_minutes = myNex.readNumber("Clean.n3.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Clean_timeOFF1=Clean_timeOFF1 = buffer;
//     jee.var("Clean_timeOFF1", Clean_timeOFF1);
// }
// void trigger16(){read_Clean_n2_n3();} 


// //printh 23 02 54 11 - "Clean" считываем состояние кнопок промывки SW0, SW1
// void read_Clean_sw0_sw1(){
//     Saved_Clean_Time1=Clean_Time1 = myNex.readNumber("Clean.sw0.val");
//     jee.var("Clean_Time1", Clean_Time1 ? "true" : "false"); 

//     Power_Clean1=Power_Clean = myNex.readNumber("Clean.sw1.val");
//     jee.var("Power_Clean", Power_Clean ? "true" : "false"); 

//     Error err = RS485.addRequest(40001,1,0x05,3, Power_Clean ? devices[0].value : devices[1].value);
// }
// void trigger17(){read_Clean_sw0_sw1();} 

// //printh 23 02 54 12 - "Clean" Считываем кнопки ПН,ВТ,СР,ЧТ,ПТ,СБ,ВС для таймера
// void read_Clean_chk(){
//    Saved_chk1=chk1 = myNex.readNumber("Clean.bt0.val"); delay(50);
//    jee.var("chk1", chk1? "true" : "false"); 
//    Saved_chk2=chk2 = myNex.readNumber("Clean.bt1.val"); delay(50);
//    jee.var("chk2", chk2? "true" : "false");
//    Saved_chk3=chk3 = myNex.readNumber("Clean.bt2.val"); delay(50);
//    jee.var("chk3", chk3? "true" : "false");
//    Saved_chk4=chk4 = myNex.readNumber("Clean.bt3.val"); delay(50);
//    jee.var("chk4", chk4? "true" : "false");
//    Saved_chk5=chk5 = myNex.readNumber("Clean.bt4.val"); delay(50);
//    jee.var("chk5", chk5? "true" : "false");
//    Saved_chk6=chk6 = myNex.readNumber("Clean.bt5.val"); delay(50);
//    jee.var("chk6", chk6? "true" : "false");
//    Saved_chk7=chk7 = myNex.readNumber("Clean.bt6.val");
//    jee.var("chk7", chk7? "true" : "false");
// }
// void trigger18(){read_Clean_chk();} 

// /////////////////////////************* page heat  **************/////////////////////////////
// ////////////////////////************* page heat **************//////////////////////////////
// ///////////////////////************* page heat  **************///////////////////////////////


// //printh 23 02 54 13  -"heat" - свчитываем уставку (n0.val=h0.val) темепературы воды в бассейне
// void read_heat_h0(){

//     int Sider_heat_on = myNex.readNumber("heat.h0.val");

//     if(Sider_heat_on < 31 && Sider_heat_on >= 0) { // Sider_heat  != 777777){ 
//         Sider_heat1 = Sider_heat = Sider_heat_on;
//         jee.var("Sider_heat", String(Sider_heat));
//         jee.var("Heat", String(Power_Heat ? "Нагрев" : "Откл.")+ ", " + "T:" + String(DS1)+ ", " + "T_s:" + String(Sider_heat));
//     } 
// }
// void trigger19(){ read_heat_h0();}


// //printh 23 02 54 14 -"heat" - свчитываем состояние кнопки SW0
// void read_heat_sw0(){
//     Activation_Heat1=Activation_Heat = myNex.readNumber("heat.sw0.val");
//     jee.var("Activation_Heat", Activation_Heat ? "true" : "false"); 
// }
// void trigger20(){read_heat_sw0();}



// //////////////////////////******** Дозаторы - Dispensers **********///////////////////////////
// /////////////////////////************* Dispensers  **************/////////////////////////////
// ////////////////////////************* Dispensers **************//////////////////////////////
// ///////////////////////************* Dispensers **************///////////////////////////////

// //printh 23 02 54 15 
// void trigger21(){
//    ESP.restart();
// } 


// // printh 23 02 54 16 - Dispensers  свчитываем состояние sw0 / sw1
// void read_Dispensers_sw0_sw1(){
//     Saved_PHControlACO = PH_Control_ACO = myNex.readNumber("Dispensers.sw0.val"); delay(10);
//     jee.var("PH_Control_ACO", PH_Control_ACO ? "true" : "false");

//     // Saved_Test_Pump = Test_Pump = myNex.readNumber("Dispensers.sw1.val"); delay(10);
//     // jee.var("Test_Pump", Test_Pump ? "true" : "false"); 


//     // in_hours = myNex.readNumber("Dispensers.n0.val"); in_minutes = myNex.readNumber("Dispensers.n1.val");
//     // sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Timer_H2O2_Start=Timer_H2O2_Start = buffer;
//     // jee.var("Timer_H2O2_Start", Timer_H2O2_Start);
// }
// void trigger22(){read_Dispensers_sw0_sw1();} 

// // printh 23 02 54 17  - Dispensers  свчитываем состояние ComboBox cb0.txt
// void read_Dispensers_cb0(){
//     //Отложенное повторное выполнение через 2 секунды - выполняем NextionDelay();
//     triggerActivated_Nextion=true; Function_Nextion = 23; // читаем - Dispensers.cb0.val
// }
// void trigger23(){read_Dispensers_cb0();
//  Saved_ACO_Work = ACO_Work = myNex.readNumber("Dispensers.cb0.val") +1;  //Отложенное повторное выполнение через 1 секунду
//  jee.var("ACO_Work", String(ACO_Work));
// } 

// // printh 23 02 54 18 - Dispensers  свчитываем состояние n4 / n5
// void read_Dispensers_sw2_sw3(){
//     Saved_NaOCl_H2O2_Control = NaOCl_H2O2_Control = myNex.readNumber("Dispensers.sw2.val"); delay(10);
//     jee.var("NaOCl_H2O2_Control", NaOCl_H2O2_Control ? "true" : "false");

//     // Saved_Activation_Timer_H2O2 = Activation_Timer_H2O2 = myNex.readNumber("Dispensers.sw3.val"); delay(10);
//     // jee.var("Activation_Timer_H2O2", Activation_Timer_H2O2 ? "true" : "false"); 

// }
// void trigger24(){read_Dispensers_sw2_sw3();} 


// // printh 23 02 54 19 - Dispensers  свчитываем состояние n4 / n5
// void read_Dispensers_cb1(){
//    //Отложенное повторное выполнение через 2 секунды - выполняем NextionDelay();
//     triggerActivated_Nextion=true; Function_Nextion = 25; // читаем - Dispensers.cb1.val
// }
// void trigger25(){read_Dispensers_cb1();

// Saved_H2O2_Work = H2O2_Work = myNex.readNumber("Dispensers.cb1.val") +1;
// jee.var("H2O2_Work", String(H2O2_Work));

// } 

// // printh 23 02 54 1A -
// void trigger26(){
//     //////////// 
// } 


// // printh 23 02 54 1B - 
// void trigger27(){
//   //////////// 
// } 

// // printh 23 02 54 1C - Dispensers  свчитываем состояние n10 / n11
// void trigger28(){
//     // Saved_Activation_Timer_H2O2 = Activation_Timer_H2O2 = myNex.readNumber("Dispensers.sw0.val"); delay(50);
//     // jee.var("Activation_Timer_H2O2", Activation_Timer_H2O2 ? "true" : "false");




// //     Saved_Activation_Timer_ACO = Activation_Timer_ACO = myNex.readNumber("Dispensers.sw1.val"); delay(50);
// //     jee.var("Activation_Timer_ACO", Activation_Timer_ACO ? "true" : "false"); 
//  } 



// // printh 23 02 54 1D - Dispensers  свчитываем состояние SW0/SW1/SW2
// void trigger29(){

// }

// // printh 23 02 54 1E - Dispensers  ----
// void trigger30(){
   
// } 

// /////////////////////////************* page pageRTC **************/////////////////////////////
// ////////////////////////************* page pageRTC **************//////////////////////////////

// // printh printh 23 02 54 1F - pageRTC - Читаем часовой пояс
// void read_RTC_n5(){
//     Saved_gmtOffset_correct = gmtOffset_correct = myNex.readNumber("pageRTC.n5.val"); delay(100);
//     if(gmtOffset_correct > 1 && gmtOffset_correct < 10){jee.var("gmtOffset_correct", String(gmtOffset_correct)); }
// }
// void trigger31(){read_RTC_n5();} 


// // printh 23 02 54 20
// // printh 23 02 54 21
// // printh 23 02 54 22





// //  void Nextion_Read (int interval) {
// //   static unsigned long timer;
// //   if (interval + timer > millis()) return; 
// //   timer = millis();
// // // //---------------------------------------------------------------------------------
// // // //---------------------------------------------------------------------------------
// // // //---------------------------------------------------------------------------------
// // // //---------------------------------------------------------------------------------


// // switch (flag) {
// //   case 0:
 
// //     flag = 1000; 
// //     break;
    
// //   case 1:
  
// //     flag = 1000; 
// //     break;
    
// //   case 2:

    

// //     flag = 1000; 
// //     break;
    
// // }




// // }




//Отложенное чтение переменных из Nextion, связанное с поздним обновление в Nextion информации.
void NextionDelay(void){
// void NextionDelay(int interval) {
// static unsigned long timer;
// if (interval + timer > millis()) return; 
// timer = millis();
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

triggerActivated_Nextion = false; //Деактивируем вызов из loop - выполнение данной функции

// Используем switch для вызова соответствующей функции
  switch(Function_Nextion) {
    case 0:  break;
    case 1:  break;
    case 2:  break;
    case 3:  break;
    case 4:  break;
    case 5:  break;
    case 6:  break;
    case 7:  break;
    case 8:  break;
    case 9:  break;
    case 10:  break;
    case 11:  break;
    case 12:  break;
    case 13:  break;
    case 14:  break;
    case 15:  break;
    case 16:  break;
    case 17:  break;
    case 18:  break;
    case 19:  break;
    case 20:  break;
    case 21:  break;
    case 22:  break;
    case 23: 
    Saved_ACO_Work = ACO_Work = myNex.readNumber("Dispensers.cb0.val") +1;  //Отложенное повторное выполнение через 1 секунду
    //Serial.println(ACO_Work);
    // jee.var("ACO_Work", String(ACO_Work));
    break;
    case 24:  break;
    case 25: 
    Saved_H2O2_Work = H2O2_Work = myNex.readNumber("Dispensers.cb1.val") +1;  //Отложенное повторное выполнение через 1 секунду
    //Serial.println(H2O2_Work);
    // jee.var("H2O2_Work", String(H2O2_Work));
    break;
    default: break; // Ничего не делаем, если значение функции вне диапазона
  }
}




//Отложенное чтение всех необходимых переменных из Nextion, после перезагрузки контроллера - 1 раз после перезагрузки
void RestartNextionDelay(void) {
// void RestartNextionDelay(int interval) {
// static unsigned long timer;
// if (interval + timer > millis()) return; 
// timer = millis();
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
triggerRestartNextion = false;

// //////////////////////////***************** set_lamp **********///////////////////////////
// //////////////////////////******** Управление подсветкой - лампами  *****/////////////////
// read_lamp_n0_n1();
// read_lamp_n2_n3();
read_lamp_sw0_sw1_sw2();
// //////////////////////////***************** set_RGB **********///////////////////////////
// //////////////////////////******** Управление RGB подстветкой - лентой WS2815  *****/////////////////
// in_hours = myNex.readNumber("set_RGB.n0.val"); read_RGB_n0_n1();
// read_RGB_n2_n3();
// read_RGB_sw0_sw2_sw3();
// //////////////////////////***************** set_filtr **********///////////////////////////
// //////////////////////////************* Фильтр  *****/////////////////
// read_filtr_n0_n1();
// read_filtr_n2_n3();
// read_filtr_n4_n5();
// read_filtr_n6_n7();
// read_filtr_n8_n9();
// read_filtr_n10_n11();   
// read_filtr_sw0_sw1_sw2();

// /////////////////////////************* page Clean **************/////////////////////////////
// ////////////////////////************* page Clean **************//////////////////////////////
// read_Clean_n0_n1();
// read_Clean_n2_n3();
// read_Clean_sw0_sw1();
// read_Clean_chk();

// /////////////////////////************* page heat  **************/////////////////////////////
// ////////////////////////************* page heat **************//////////////////////////////
// read_heat_sw0();
// read_heat_h0();
// //////////////////////////******** Дозаторы - Dispensers **********///////////////////////////
// //////////////////////////******** menu("Химия (NaOCl, ACO, APF)  *****/////////////////
// read_Dispensers_sw0_sw1();
// read_Dispensers_cb0();
// read_Dispensers_sw2_sw3();
// read_Dispensers_cb1();
// // Saved_PHControlACO = PH_Control_ACO = myNex.readNumber("Dispensers.sw0.val"); jee.var("PH_Control_ACO", PH_Control_ACO ? "true" : "false"); delay(10);//Контроль PH  
// // Saved_Activation_Timer_ACO = Activation_Timer_ACO = myNex.readNumber("Dispensers.sw1.val"); jee.var("Activation_Timer_ACO", String(Activation_Timer_ACO ? "true" : "false" ));  delay(10);  //Работа перельстатического насоса
// // Saved_ACO_Work = ACO_Work = myNex.readNumber("Dispensers.cb0.val") +1; jee.var("ACO_Work", String(ACO_Work));  delay(10);  // Как часто включается перельстатический насос

     
// // Saved_NaOCl_H2O2_Control = NaOCl_H2O2_Control = myNex.readNumber("Dispensers.sw2.val"); jee.var("NaOCl_H2O2_Control", String(NaOCl_H2O2_Control ? "true" : "false")); delay(10); //Контроль Хлора
// // Saved_Activation_Timer_H2O2 = Activation_Timer_H2O2 = myNex.readNumber("Dispensers.sw3.val"); jee.var("Activation_Timer_H2O2", String(Activation_Timer_H2O2 ? "true" : "false")); delay(10); //Работа перельстатического насоса
// // Saved_H2O2_Work = H2O2_Work = myNex.readNumber("Dispensers.cb1.val") +1;  jee.var("H2O2_Work", String(H2O2_Work)); // Как часто включается перельстатический насос

// /////////////////////////************* page pageRTC **************/////////////////////////////
// ////////////////////////************* page pageRTC **************//////////////////////////////
// read_RTC_n5();



}


