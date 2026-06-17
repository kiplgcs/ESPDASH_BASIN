// bool controlTemperature(float temper,            //Температура измеренная
//                         int set_temper,          //Уставка
//                         bool activation_control) //Разрешение на регулировку
//                         { 
    
//   bool PowerHeat = false;
//   if (!activation_control) {/*Error err = RS485.addRequest(40001,1,0x05,4, devices[1].value );*/ return false;  /*pcfRelays.digitalWrite(P4, HIGH); return PowerHeat;*/} //               // Если нет разрешения контроля не контролируем и отключаем
//   //Serial.println("Выполнено 2" + String(activation_control)); //Для теста выполнения


//   if (temper != Saved_DS1 || set_temper != Sider_heat1) { // Если есть изменения уставки или температуры - выполняем далее

//       if (set_temper > temper + 0.1) {
//           PowerHeat = true;
//       } else if (set_temper < temper - 0.1) {
//            PowerHeat = false;
//       }
    

//       if(Power_Heat1 != Power_Heat){Power_Heat1 = Power_Heat;
//       myNex.writeNum("page0.b4.pic", PowerHeat ? 10 : 9);       //Обновляем включено или выключено в Nextion   
//       jee.var("Power_Heat", String(PowerHeat));                //Обновляем состояние включения обогрева на Web странице
//       }

//       Saved_DS1 = temper;  Sider_heat1 = set_temper;     // Приравниваем для отслеживания измениения
        
//       //pcfRelays.digitalWrite(P4, PowerHeat ? LOW : HIGH);
//       Error err = RS485.addRequest(40001,1,0x05,4, PowerHeat ? devices[0].value : devices[1].value);

//                                             //Возвращаем включение или отключение нагрева
//   } 
  
//   return PowerHeat;  
// }

// // //Функция для преобразования строки в байт данных
// // void stringToByteArray(const String& str, byte* byteArray) {
// //   int strLength = str.length();
  
// //   // Проходимся по каждому символу строки
// //   for (int i = 0; i < strLength; i++) {
// //     // Получаем ASCII-код символа
// //     byte asciiCode = str.charAt(i);
// //     // Сохраняем ASCII-код в байтовый массив
// //     byteArray[i] = asciiCode;
// //   }
// // }



// Функция для поиска в строке подстроки - необходима для преобразования времени в формат для передачи на Nextion
int getSubstring(const String& input, int start, int end) {
    // Проверка на корректность входных параметров
    if (start < 0 || end >= input.length() || start > end) {
        return 0; // Возвращаем 0 в случае ошибки
    }

    String substring = input.substring(start, end + 1);
    int result = substring.toInt();

    return result;
}




void Nextion_Transmit (int interval) {
  static unsigned long timer;
  if (interval + timer > millis()) return; 
  timer = millis();
  uint32_t currentPage = myNex.readNumber("dp");
  if (currentPage != 777777) {
    Nx_page_id = currentPage;
  }
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------

// /////////////////////////************* page set_lamp  **************/////////////////////////////
// ////////////////////////************* page set_lamp  **************//////////////////////////////
// ///////////////////////************* page set_lamp  **************///////////////////////////////

if (Lamp != Lamp1 && !triggerRestartNextion){Lamp1 = Lamp;
  myNex.writeStr("dim=50");
  myNex.writeNum("page0.b0.pic", Lamp ? 2 : 1); 
  //   myNex.writeStr("page set_lamp");
  myNex.writeNum("set_lamp.sw3.val", Lamp ? 1 : 0);

  // Error err = RS485.addRequest(40001,1,0x05,0, Lamp ? devices[0].value : devices[1].value);

}//pcfRelays.digitalWrite(P0, Lamp ? LOW : HIGH);
  





if (Saved_Lamp_autosvet != Lamp_autosvet && !triggerRestartNextion){Saved_Lamp_autosvet = Lamp_autosvet;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_lamp");
  myNex.writeNum("set_lamp.sw1.val", Lamp_autosvet ? 1 : 0); 
}

if (Saved_Power_Time1 != Power_Time1 && !triggerRestartNextion){ Saved_Power_Time1 = Power_Time1;
  myNex.writeStr("dim=50");  
  //   myNex.writeStr("page set_lamp");
  myNex.writeNum("set_lamp.sw0.val", Power_Time1 ? 1 : 0); 
}

UITimerEntry &lampTimer = ui.timer("LampTimer");
if (Saved_Lamp_timeON1 != lampTimer.on && !triggerRestartNextion) {Saved_Lamp_timeON1 = lampTimer.on;  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_lamp");
  String lampOnStr = formatMinutesToTime(lampTimer.on);
  myNex.writeNum("set_lamp.n0.val", getSubstring(lampOnStr, 0, 1));
  myNex.writeNum("set_lamp.n1.val", getSubstring(lampOnStr, 3, 4));
}

if (Saved_Lamp_timeOFF1 != lampTimer.off && !triggerRestartNextion) {Saved_Lamp_timeOFF1 = lampTimer.off;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_lamp");
  String lampOffStr = formatMinutesToTime(lampTimer.off);
  myNex.writeNum("set_lamp.n2.val", getSubstring(lampOffStr, 0, 1));
  myNex.writeNum("set_lamp.n3.val", getSubstring(lampOffStr, 3, 4));
}


// /////////////////////////************* page set_RGB  **************/////////////////////////////
// ////////////////////////************* page set_RGB  **************//////////////////////////////
// ///////////////////////************* page set_RGB  **************///////////////////////////////

  if (Pow_WS28151 != Pow_WS2815 && !triggerRestartNextion){Pow_WS28151 = Pow_WS2815;
    myNex.writeStr("dim=50");
    myNex.writeNum("page0.b12.pic", Pow_WS2815 ? 2 : 1);
    //     myNex.writeStr("page set_RGB"); 
    myNex.writeNum("set_RGB.sw3.val", Pow_WS2815 ? 1 : 0); 

  // //if(Pow_WS2815 == true) {/*loop_i2c(String("LED___ON"));} else if  (Pow_WS2815 ==false) {loop_i2c(String("LED__OFF"));*/}
  //   Error err = RS485.addRequest(40001,1,0x05,1, Pow_WS2815 ? devices[0].value : devices[1].value);
  }//pcfRelays.digitalWrite(P1, Pow_WS2815 ? LOW : HIGH);

// // if(Color_RGB != Saved_Color_RGB) {Saved_Color_RGB = Color_RGB;
// // 		//if(Color_RGB == true) {/*loop_i2c(String("CRGB__ON"));} else if  (Color_RGB ==false) {loop_i2c(String("CRGB_OFF"));*/}
// //   }

// if(new_bright1 != new_bright) {new_bright1 = new_bright;
// 		//loop_i2c("Brig=" + String(new_bright));
		
// 	}

// if(number1 != number) {number1= number;
// 		//Serial.println(String(number, HEX)); // Преобразуем number в строку в шестнадцатеричной системе и выводим
// 		//loop_i2c("C="+String(number, HEX));	
// 	}


if (Saved_Pow_WS2815_autosvet != Pow_WS2815_autosvet && !triggerRestartNextion){Saved_Pow_WS2815_autosvet = Pow_WS2815_autosvet;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_RGB"); 
  myNex.writeNum("set_RGB.sw2.val", Pow_WS2815_autosvet ? 1 : 0); 
}

if (Saved_WS2815_Time1 != WS2815_Time1 && !triggerRestartNextion){ Saved_WS2815_Time1 = WS2815_Time1;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_RGB"); //отобразить страницу
  myNex.writeNum("set_RGB.sw0.val", WS2815_Time1); 
}


UITimerEntry &rgbTimer = ui.timer("RgbTimer");
if (Saved_timeON_WS2815 != rgbTimer.on && !triggerRestartNextion){Saved_timeON_WS2815 = rgbTimer.on;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_RGB"); //отобразить страницу
  String rgbOnStr = formatMinutesToTime(rgbTimer.on);
  myNex.writeNum("set_RGB.n0.val", getSubstring(rgbOnStr, 0, 1));
  myNex.writeNum("set_RGB.n1.val", getSubstring(rgbOnStr, 3, 4));
} 

if (Saved_timeOFF_WS2815 != rgbTimer.off && !triggerRestartNextion){Saved_timeOFF_WS2815 = rgbTimer.off;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_RGB");//отобразить страницу
  String rgbOffStr = formatMinutesToTime(rgbTimer.off);
  myNex.writeNum("set_RGB.n2.val", getSubstring(rgbOffStr, 0, 1));
  myNex.writeNum("set_RGB.n3.val", getSubstring(rgbOffStr, 3, 4));
} 

// /////////////////////////************* page set_filtr **************/////////////////////////////
// ////////////////////////************* page set_filtr **************//////////////////////////////
// ///////////////////////************* page set_filtr **************///////////////////////////////

// if (Power_Filtr1 != Power_Filtr && !triggerRestartNextion) {Power_Filtr1 = Power_Filtr;
//     myNex.writeStr("dim=50");
//     myNex.writeNum("page0.b1.pic", Power_Filtr ? 4 : 3);

//     jee.var("Power_Filtr", Power_Filtr ? "true" : "false");
    
//     myNex.writeStr("page set_filtr");
//     myNex.writeNum("set_filtr.sw3.val", Power_Filtr ? 1 : 0);
//     Error err = RS485.addRequest(40001,1,0x05,8, Power_Filtr ? devices[0].value : devices[1].value);
// }   //pcfRelays.digitalWrite(P2, Power_Filtr ? LOW : HIGH);

if (Power_Filtr1 != Power_Filtr && !triggerRestartNextion) {Power_Filtr1 = Power_Filtr;
    myNex.writeStr("dim=50");
    myNex.writeNum("page0.b1.pic", Power_Filtr ? 4 : 3);
    //     myNex.writeStr("page set_filtr");
    myNex.writeNum("set_filtr.sw3.val", Power_Filtr ? 1 : 0);
}   //pcfRelays.digitalWrite(P2, Power_Filtr ? LOW : HIGH);

// if (Saved_Filtr_Time1 != Filtr_Time1 && !triggerRestartNextion){Saved_Filtr_Time1 = Filtr_Time1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.sw0.val", Filtr_Time1 ? 1 : 0); 
// }

if (Saved_Filtr_Time1 != Filtr_Time1 && !triggerRestartNextion){Saved_Filtr_Time1 = Filtr_Time1;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  myNex.writeNum("set_filtr.sw0.val", Filtr_Time1 ? 1 : 0); 
}

// if (Saved_Filtr_Time2 != Filtr_Time2 && !triggerRestartNextion){Saved_Filtr_Time2 = Filtr_Time2;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.sw2.val", Filtr_Time2 ? 1 : 0); 
// }

if (Saved_Filtr_Time2 != Filtr_Time2 && !triggerRestartNextion){Saved_Filtr_Time2 = Filtr_Time2;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  myNex.writeNum("set_filtr.sw2.val", Filtr_Time2 ? 1 : 0); 
}

// if (Saved_Filtr_Time3 != Filtr_Time3 && !triggerRestartNextion){Saved_Filtr_Time3 = Filtr_Time3;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.sw1.val", Filtr_Time3 ? 1 : 0); 
// }

if (Saved_Filtr_Time3 != Filtr_Time3 && !triggerRestartNextion){Saved_Filtr_Time3 = Filtr_Time3;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  myNex.writeNum("set_filtr.sw1.val", Filtr_Time3 ? 1 : 0); 
}

// if (Saved_Filtr_timeON1 != Filtr_timeON1 && !triggerRestartNextion) {Saved_Filtr_timeON1 = Filtr_timeON1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.n0.val", getSubstring(Filtr_timeON1, 0, 1));
//   myNex.writeNum("set_filtr.n1.val", getSubstring(Filtr_timeON1, 3, 4));
// }

UITimerEntry &filtrTimer1 = ui.timer("FiltrTimer1");
if (Saved_Filtr_timeON1 != filtrTimer1.on && !triggerRestartNextion) {Saved_Filtr_timeON1 = filtrTimer1.on;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  String filtrOn1Str = formatMinutesToTime(filtrTimer1.on);
  myNex.writeNum("set_filtr.n0.val", getSubstring(filtrOn1Str, 0, 1));
  myNex.writeNum("set_filtr.n1.val", getSubstring(filtrOn1Str, 3, 4));
}

// if (Saved_Filtr_timeOFF1 != Filtr_timeOFF1 && !triggerRestartNextion) {Saved_Filtr_timeOFF1 = Filtr_timeOFF1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.n2.val", getSubstring(Filtr_timeOFF1, 0, 1));
//   myNex.writeNum("set_filtr.n3.val", getSubstring(Filtr_timeOFF1, 3, 4));
// }

if (Saved_Filtr_timeOFF1 != filtrTimer1.off && !triggerRestartNextion) {Saved_Filtr_timeOFF1 = filtrTimer1.off;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  String filtrOff1Str = formatMinutesToTime(filtrTimer1.off);
  myNex.writeNum("set_filtr.n2.val", getSubstring(filtrOff1Str, 0, 1));
  myNex.writeNum("set_filtr.n3.val", getSubstring(filtrOff1Str, 3, 4));
}

// if (Saved_Filtr_timeON2 != Filtr_timeON2 && !triggerRestartNextion) {Saved_Filtr_timeON2 = Filtr_timeON2;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.n4.val", getSubstring(Filtr_timeON2, 0, 1));
//   myNex.writeNum("set_filtr.n5.val", getSubstring(Filtr_timeON2, 3, 4));
// }

UITimerEntry &filtrTimer2 = ui.timer("FiltrTimer2");
if (Saved_Filtr_timeON2 != filtrTimer2.on && !triggerRestartNextion) {Saved_Filtr_timeON2 = filtrTimer2.on;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  String filtrOn2Str = formatMinutesToTime(filtrTimer2.on);
  myNex.writeNum("set_filtr.n4.val", getSubstring(filtrOn2Str, 0, 1));
  myNex.writeNum("set_filtr.n5.val", getSubstring(filtrOn2Str, 3, 4));
}

// if (Saved_Filtr_timeOFF2 != Filtr_timeOFF2 && !triggerRestartNextion) {Saved_Filtr_timeOFF2 = Filtr_timeOFF2;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.n6.val", getSubstring(Filtr_timeOFF2, 0, 1));
//   myNex.writeNum("set_filtr.n7.val", getSubstring(Filtr_timeOFF2, 3, 4));
// }

if (Saved_Filtr_timeOFF2 != filtrTimer2.off && !triggerRestartNextion) {Saved_Filtr_timeOFF2 = filtrTimer2.off;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  String filtrOff2Str = formatMinutesToTime(filtrTimer2.off);
  myNex.writeNum("set_filtr.n6.val", getSubstring(filtrOff2Str, 0, 1));
  myNex.writeNum("set_filtr.n7.val", getSubstring(filtrOff2Str, 3, 4));
}

// if (Saved_Filtr_timeON3 != Filtr_timeON3 && !triggerRestartNextion) {Saved_Filtr_timeON3 = Filtr_timeON3;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.n8.val", getSubstring(Filtr_timeON3, 0, 1));
//   myNex.writeNum("set_filtr.n9.val", getSubstring(Filtr_timeON3, 3, 4));
// }

UITimerEntry &filtrTimer3 = ui.timer("FiltrTimer3");
if (Saved_Filtr_timeON3 != filtrTimer3.on && !triggerRestartNextion) {Saved_Filtr_timeON3 = filtrTimer3.on;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  String filtrOn3Str = formatMinutesToTime(filtrTimer3.on);
  myNex.writeNum("set_filtr.n8.val", getSubstring(filtrOn3Str, 0, 1));
  myNex.writeNum("set_filtr.n9.val", getSubstring(filtrOn3Str, 3, 4));
}

// if (Saved_Filtr_timeOFF3 != Filtr_timeOFF3 && !triggerRestartNextion) {Saved_Filtr_timeOFF3 = Filtr_timeOFF3;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.n10.val", getSubstring(Filtr_timeOFF3, 0, 1));
//   myNex.writeNum("set_filtr.n11.val", getSubstring(Filtr_timeOFF3, 3, 4));
// }

if (Saved_Filtr_timeOFF3 != filtrTimer3.off && !triggerRestartNextion) {Saved_Filtr_timeOFF3 = filtrTimer3.off;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page set_filtr");
  String filtrOff3Str = formatMinutesToTime(filtrTimer3.off);
  myNex.writeNum("set_filtr.n10.val", getSubstring(filtrOff3Str, 0, 1));
  myNex.writeNum("set_filtr.n11.val", getSubstring(filtrOff3Str, 3, 4));
}
// /////////////////////////************* page Clean **************/////////////////////////////
// ////////////////////////************* page Clean **************//////////////////////////////
// ///////////////////////************* page Clean **************///////////////////////////////

// if (Power_Clean1 != Power_Clean && !triggerRestartNextion) {Power_Clean1 = Power_Clean;
//     myNex.writeStr("dim=50");
//     myNex.writeNum("page0.b2.pic", Power_Clean ? 6 : 5); delay(50);
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.sw1.val", Power_Clean ? 1 : 0);
//   Error err = RS485.addRequest(40001,1,0x05,3, Power_Clean ? devices[0].value : devices[1].value);
// }  //pcfRelays.digitalWrite(P3, Power_Clean ? LOW : HIGH);

if (Power_Clean1 != Power_Clean && !triggerRestartNextion) {Power_Clean1 = Power_Clean;
    myNex.writeStr("dim=50");
    myNex.writeNum("page0.b2.pic", Power_Clean ? 6 : 5); delay(50);
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.sw1.val", Power_Clean ? 1 : 0);
}  //pcfRelays.digitalWrite(P3, Power_Clean ? LOW : HIGH);

// if (Saved_Clean_Time1 != Clean_Time1 && !triggerRestartNextion) {Saved_Clean_Time1=Clean_Time1;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.sw0.val", Clean_Time1 ? 1 : 0);
// }

if (Saved_Clean_Time1 != Clean_Time1 && !triggerRestartNextion) {Saved_Clean_Time1=Clean_Time1;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.sw0.val", Clean_Time1 ? 1 : 0);
}

// if (Saved_Clean_timeON1 != Clean_timeON1 && !triggerRestartNextion) {Saved_Clean_timeON1 = Clean_timeON1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page Clean");
//   myNex.writeNum("Clean.n0.val", getSubstring(Clean_timeON1, 0, 1));
//   myNex.writeNum("Clean.n1.val", getSubstring(Clean_timeON1, 3, 4));
// }

UITimerEntry &cleanTimer = ui.timer("CleanTimer1");
if (Saved_Clean_timeON1 != cleanTimer.on && !triggerRestartNextion) {Saved_Clean_timeON1 = cleanTimer.on;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page Clean");
  String cleanOnStr = formatMinutesToTime(cleanTimer.on);
  myNex.writeNum("Clean.n0.val", getSubstring(cleanOnStr, 0, 1));
  myNex.writeNum("Clean.n1.val", getSubstring(cleanOnStr, 3, 4));
}

// if (Saved_Clean_timeOFF1 != Clean_timeOFF1 && !triggerRestartNextion) {Saved_Clean_timeOFF1 = Clean_timeOFF1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page Clean");
//   myNex.writeNum("Clean.n2.val", getSubstring(Clean_timeOFF1, 0, 1));
//   myNex.writeNum("Clean.n3.val", getSubstring(Clean_timeOFF1, 3, 4));
// }
if (Saved_Clean_timeOFF1 != cleanTimer.off && !triggerRestartNextion) {Saved_Clean_timeOFF1 = cleanTimer.off;
  myNex.writeStr("dim=50");
  //   myNex.writeStr("page Clean");
  String cleanOffStr = formatMinutesToTime(cleanTimer.off);
  myNex.writeNum("Clean.n2.val", getSubstring(cleanOffStr, 0, 1));
  myNex.writeNum("Clean.n3.val", getSubstring(cleanOffStr, 3, 4));
}

// if (Saved_chk1 != chk1 && !triggerRestartNextion) {Saved_chk1 = chk1;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt0.val", chk1? 1 : 0);
// }

if (Saved_chk1 != chk1 && !triggerRestartNextion) {Saved_chk1 = chk1;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.bt0.val", chk1? 1 : 0);
}

// if (Saved_chk2 != chk2 && !triggerRestartNextion) {Saved_chk2 = chk2;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt1.val", chk2? 1 : 0);
// }

if (Saved_chk2 != chk2 && !triggerRestartNextion) {Saved_chk2 = chk2;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.bt1.val", chk2? 1 : 0);
}

// if (Saved_chk3 != chk3 && !triggerRestartNextion) {Saved_chk3 = chk3;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt2.val", chk3? 1 : 0);
// }

if (Saved_chk3 != chk3 && !triggerRestartNextion) {Saved_chk3 = chk3;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.bt2.val", chk3? 1 : 0);
}

// if (Saved_chk4 != chk4 && !triggerRestartNextion) {Saved_chk4 = chk4;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt3.val", chk4? 1 : 0);
// }

if (Saved_chk4 != chk4 && !triggerRestartNextion) {Saved_chk4 = chk4;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.bt3.val", chk4? 1 : 0);
}

// if (Saved_chk5 != chk5 && !triggerRestartNextion) {Saved_chk5 = chk5;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt4.val", chk5? 1 : 0);
// }
if (Saved_chk5 != chk5 && !triggerRestartNextion) {Saved_chk5 = chk5;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.bt4.val", chk5? 1 : 0);
}


// if (Saved_chk6 != chk6 && !triggerRestartNextion) {Saved_chk6 = chk6;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt5.val", chk6? 1 : 0);
// }
if (Saved_chk6 != chk6 && !triggerRestartNextion) {Saved_chk6 = chk6;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.bt5.val", chk6? 1 : 0);
}

// if (Saved_chk7 != chk7 && !triggerRestartNextion) {Saved_chk7 = chk7;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt6.val", chk7? 1 : 0);
// }

if (Saved_chk7 != chk7 && !triggerRestartNextion) {Saved_chk7 = chk7;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page Clean");
    myNex.writeNum("Clean.bt6.val", chk7? 1 : 0);
}

static bool cleanLabelWritten = false;
if (Nx_page_id == 4 && !triggerRestartNextion) {
  if (!cleanLabelWritten) {
    myNex.writeStr("Clean.t3.txt", "Clean stage");
    cleanLabelWritten = true;
  }
} else {
  cleanLabelWritten = false;
}

// /////////////////////////************* page heat  **************/////////////////////////////
// ////////////////////////************* page heat **************//////////////////////////////
// ///////////////////////************* page heat  **************///////////////////////////////

//   if (Sider_heat != Sider_heat1 && !triggerRestartNextion){//Sider_heat1 = Sider_heat;
//   //myNex.writeStr("wept 149,5", byteArray); //записывает по адресам с 149 длиной 5 байтов в EEPROM из последовательного порта
//   //seconds = myNex.readNumber("rept 149,5"); // Читаем данные из EEPROM с адресов
//   myNex.writeStr("dim=50"); delay(50); 
//   myNex.writeStr("page heat");delay(50); 
//   myNex.writeNum("heat.h0.val", Sider_heat); delay(50); 
//   myNex.writeNum("heat.n0.val", Sider_heat); delay(50); 
//   //jee.var("Heat", String(Power_Heat ? "Нагрев" : "Откл.")+ ", " + "T:" + String(DS1)+ ", " + "T_s:" + String(Sider_heat));

//   //Проверяем записалось ли значение:
//   Sider_heat1 = myNex.readNumber("heat.n0.val"); //if(Sider_heat   < 31 && Sider_heat  >= 0) {jee.var("Sider_heat", String(Sider_heat));}
//   } 

//   if (Activation_Heat != Activation_Heat1 && !triggerRestartNextion) { Activation_Heat1 = Activation_Heat; //выполняем если изменилась уставка и прошло время после презагрузки микросхемы.
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page heat");delay(150); 
//   myNex.writeNum("heat.sw0.val", Activation_Heat); delay(150); 
//   }

//   Power_Heat = controlTemperature (DS1, Sider_heat, Activation_Heat); //Контроль температуры и отправка myNex.writeNum("page0.va0.val", Heat_ON_OFF ? 1 : 0);
  if (Sider_heat != Sider_heat1 && !triggerRestartNextion){
    myNex.writeStr("dim=50"); delay(50);
    //     myNex.writeStr("page heat"); delay(50);
    myNex.writeNum("heat.h0.maxval", 35); delay(20);
    myNex.writeNum("heat.h0.val", Sider_heat); delay(50);
    myNex.writeNum("heat.n0.val", Sider_heat); delay(50);

    // Проверяем записалось ли значение:
    Sider_heat1 = myNex.readNumber("heat.n0.val");
  }

  if (Activation_Heat != Activation_Heat1 && !triggerRestartNextion) {
    Activation_Heat1 = Activation_Heat;
    myNex.writeStr("dim=50");
    //     myNex.writeStr("page heat"); delay(150);
    myNex.writeNum("heat.sw0.val", Activation_Heat); delay(150);
  }

// /////////////////////////************* set_topping  **************/////////////////////////////
// ////////////////////////************* set_topping **************//////////////////////////////
// ///////////////////////************* set_topping **************///////////////////////////////

// if (Power_Topping1 != Power_Topping && !triggerRestartNextion) {Power_Topping1 = Power_Topping; // При изменении обновляем картинку долива воды на Nextion
//     myNex.writeStr("dim=50");
//     myNex.writeNum("page0.b3.pic", Power_Topping ? 8 : 7); 
// }

static bool setToppingPageWasOpen = false;
static unsigned long lastSetToppingSyncMs = 0;
static bool lastNxActivationWaterLevel = false;
static bool lastNxPowerDrain = false;
static bool lastNxPowerTopping = false;
static bool lastNxWaterLevelUpper = false;
static bool lastNxWaterLevelLower = false;
static bool lastNxWaterLevelDrain = false;
static String lastNxWaterLevelStatus;
if (Nx_page_id == 6 && !triggerRestartNextion) {
  const unsigned long nowMs = millis();
  const bool changed =
    !setToppingPageWasOpen ||
    Activation_Water_Level != lastNxActivationWaterLevel ||
    Power_Drain != lastNxPowerDrain ||
    Power_Topping != lastNxPowerTopping ||
    WaterLevelSensorUpper != lastNxWaterLevelUpper ||
    WaterLevelSensorLower != lastNxWaterLevelLower ||
    WaterLevelSensorDrain != lastNxWaterLevelDrain ||
    WaterLevelStatus != lastNxWaterLevelStatus;
  if (changed || nowMs - lastSetToppingSyncMs >= 5000UL) {
    myNex.writeNum("set_topping.sw0.val", Activation_Water_Level ? 1 : 0);
    myNex.writeNum("set_topping.sw1.val", Power_Drain ? 1 : 0);
    myNex.writeNum("set_topping.sw2.val", Power_Topping ? 1 : 0);
    myNex.writeNum("set_topping.c1.val", WaterLevelSensorUpper ? 1 : 0);
    myNex.writeNum("set_topping.c2.val", WaterLevelSensorLower ? 1 : 0);
    myNex.writeNum("set_topping.c3.val", WaterLevelSensorDrain ? 1 : 0);
    myNex.writeStr("set_topping.t0.txt", WaterLevelStatus);
    lastNxActivationWaterLevel = Activation_Water_Level;
    lastNxPowerDrain = Power_Drain;
    lastNxPowerTopping = Power_Topping;
    lastNxWaterLevelUpper = WaterLevelSensorUpper;
    lastNxWaterLevelLower = WaterLevelSensorLower;
    lastNxWaterLevelDrain = WaterLevelSensorDrain;
    lastNxWaterLevelStatus = WaterLevelStatus;
    lastSetToppingSyncMs = nowMs;
  }
  setToppingPageWasOpen = true;
} else {
  setToppingPageWasOpen = false;
}

// /////////////////////////************* pageRTC  **************/////////////////////////////
// ////////////////////////************* pageRTC **************//////////////////////////////
// ///////////////////////************* pageRTC **************///////////////////////////////


// // Наверное не надо отправлять из ESP32 в Nextion часовой пояс - только принимать
// // if(Saved_gmtOffset_correct != gmtOffset_correct){Saved_gmtOffset_correct = gmtOffset_correct;
// //   myNex.writeStr("dim=50");
// //   myNex.writeStr("page pageRTC");
// //   myNex.writeNum("pageRTC.n5.val", gmtOffset_correct);
// // }

// //////////////////////////******** Дозаторы - Dispensers **********///////////////////////////
// /////////////////////////************* Dispensers  **************/////////////////////////////
// ////////////////////////************* Dispensers **************//////////////////////////////
// ///////////////////////************* Dispensers **************///////////////////////////////




// if (Saved_PH != PH) {Saved_PH = PH;
//     //myNex.writeStr("dim=50");
//     //myNex.writeStr("page Dispensers");
//     myNex.writeStr("page0.b8.txt", "PH: " + String(PH)); //Кислотность воды
// }

// if (Saved_Power_ACO != Power_ACO) {Saved_Power_ACO = Power_ACO;
//     myNex.writeStr("page0.b6.txt", Info_ACO); //Информация о таймере подачи кислоты
// }


// if (Saved_PHControlACO != PH_Control_ACO && !triggerRestartNextion) {Saved_PHControlACO = PH_Control_ACO;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Dispensers");
//     myNex.writeNum("Dispensers.sw0.val", PH_Control_ACO? 1 : 0);
//     jee.var("PH_Control_ACO", PH_Control_ACO ? "true" : "false"); 
// }

// // if (Saved_Test_Pump != Test_Pump) {Saved_Test_Pump = Test_Pump;
// //     // myNex.writeStr("dim=50");
// //     // myNex.writeStr("page Dispensers");
// //     myNex.writeNum("Dispensers.sw1.val", Test_Pump? 1 : 0);
// //     jee.var("Test_Pump", Test_Pump ? "true" : "false"); 
// // }

// // if (ACO_Work != Saved_ACO_Work) {Saved_ACO_Work = ACO_Work;
// //   myNex.writeStr("dim=50");
// //   myNex.writeStr("page Dispensers");
// //   myNex.writeNum("Dispensers.cb0.val", ACO_Work-1); 
// // }
if (Saved_PHControlACO != PH_Control_ACO && Nx_page_id == 9 && !triggerRestartNextion) {Saved_PHControlACO = PH_Control_ACO;
    myNex.writeStr("dim=50");
    myNex.writeNum("Dispensers.sw0.val", PH_Control_ACO ? 1 : 0);
}

if (ACO_Work != Saved_ACO_Work && Nx_page_id == 9 && !triggerRestartNextion) {Saved_ACO_Work = ACO_Work;
  myNex.writeStr("dim=50");
  myNex.writeNum("Dispensers.cb0.val", ACO_Work - 1); 
}

if (Saved_NaOCl_H2O2_Control != NaOCl_H2O2_Control && Nx_page_id == 9 && !triggerRestartNextion) {
    Saved_NaOCl_H2O2_Control = NaOCl_H2O2_Control;
    myNex.writeStr("dim=50");
    myNex.writeNum("Dispensers.sw2.val", NaOCl_H2O2_Control ? 1 : 0);
}

if (H2O2_Work != Saved_H2O2_Work && Nx_page_id == 9 && !triggerRestartNextion) {Saved_H2O2_Work = H2O2_Work;
  myNex.writeStr("dim=50");
  myNex.writeNum("Dispensers.cb1.val", H2O2_Work - 1); 
}

static float lastNxPhSetting = -1000.0f;
static float lastNxPhLower = -1000.0f;
static float lastNxPhUpper = -1000.0f;
static float lastNxClLower = -1000.0f;
static float lastNxClUpper = -1000.0f;
static int lastNxOrpSetting = -100000;
if (Nx_page_id == 9 && !triggerRestartNextion) {
  float phSettingDelta = PH_setting - lastNxPhSetting;
  if (phSettingDelta < 0) phSettingDelta = -phSettingDelta;
  if (phSettingDelta > 0.001f || abs(PH_Lower - lastNxPhLower) > 0.001f || abs(PH_Upper - lastNxPhUpper) > 0.001f) {
    myNex.writeStr("Dispensers.t2.txt", "pH " + String(PH_Lower, 1) + "-" + String(PH_Upper, 1));
    lastNxPhSetting = PH_setting;
  }
  if (ORP_setting != lastNxOrpSetting || abs(CL_Lower - lastNxClLower) > 0.001f || abs(CL_Upper - lastNxClUpper) > 0.001f) {
    myNex.writeStr("Dispensers.t3.txt", "CL " + String(CL_Lower, 2) + "-" + String(CL_Upper, 2));
    lastNxOrpSetting = ORP_setting;
  }

  if (abs(PH_Lower - lastNxPhLower) > 0.001f || abs(PH_Upper - lastNxPhUpper) > 0.001f) {
    int phLowInt = static_cast<int>(PH_Lower);
    int phLowDec = static_cast<int>((PH_Lower - phLowInt) * 10.0f + 0.5f);
    if (phLowDec > 9) { phLowInt++; phLowDec = 0; }
    int phHighInt = static_cast<int>(PH_Upper);
    int phHighDec = static_cast<int>((PH_Upper - phHighInt) * 10.0f + 0.5f);
    if (phHighDec > 9) { phHighInt++; phHighDec = 0; }
    myNex.writeNum("Dispensers.n0.val", phLowInt);
    myNex.writeNum("Dispensers.n1.val", phLowDec);
    myNex.writeNum("Dispensers.n2.val", phHighInt);
    myNex.writeNum("Dispensers.n3.val", phHighDec);
    lastNxPhLower = PH_Lower;
    lastNxPhUpper = PH_Upper;
  }

  if (abs(CL_Lower - lastNxClLower) > 0.001f || abs(CL_Upper - lastNxClUpper) > 0.001f) {
    int clLowInt = static_cast<int>(CL_Lower);
    int clLowDec = static_cast<int>((CL_Lower - clLowInt) * 100.0f + 0.5f);
    if (clLowDec > 99) { clLowInt++; clLowDec = 0; }
    int clHighInt = static_cast<int>(CL_Upper);
    int clHighDec = static_cast<int>((CL_Upper - clHighInt) * 100.0f + 0.5f);
    if (clHighDec > 99) { clHighInt++; clHighDec = 0; }
    myNex.writeNum("Dispensers.n6.val", clLowInt);
    myNex.writeNum("Dispensers.n7.val", clLowDec);
    myNex.writeNum("Dispensers.n4.val", clHighInt);
    myNex.writeNum("Dispensers.n5.val", clHighDec);
    lastNxClLower = CL_Lower;
    lastNxClUpper = CL_Upper;
  }
}


// if (Saved_ppmCl != ppmCl) {Saved_ppmCl = ppmCl;
//     //myNex.writeStr("dim=50");
//     //myNex.writeStr("page Dispensers");
//     myNex.writeStr("page0.b7.txt", "Cl: " + String(ppmCl)); //Содержание хлора
// }

// if (Saved_Power_H2O != Power_H2O2) {Saved_Power_H2O = Power_H2O2;
//     myNex.writeStr("page0.b5.txt", Info_H2O2);  //Информация о таймере подачи хлора
// }

// if (Saved_NaOCl_H2O2_Control != NaOCl_H2O2_Control && !triggerRestartNextion) {Saved_NaOCl_H2O2_Control = NaOCl_H2O2_Control;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Dispensers");
//     myNex.writeNum("Dispensers.sw2.val", NaOCl_H2O2_Control? 1 : 0);
//     jee.var("NaOCl_H2O2_Control", NaOCl_H2O2_Control ? "true" : "false");
// }

// // if (Saved_Activation_Timer_H2O2 != Activation_Timer_H2O2) {Saved_Activation_Timer_H2O2 = Activation_Timer_H2O2;
// //     // myNex.writeStr("dim=50");
// //     // myNex.writeStr("page Dispensers");
// //     myNex.writeNum("Dispensers.sw3.val", Activation_Timer_H2O2? 1 : 0);
// //     jee.var("Activation_Timer_H2O2", Activation_Timer_H2O2 ? "true" : "false"); 
// // }

// // if (H2O2_Work != Saved_H2O2_Work) {Saved_H2O2_Work = H2O2_Work;
// //   myNex.writeStr("dim=50");
// //   myNex.writeStr("page Dispensers");
// //   myNex.writeNum("Dispensers.cb1.val", H2O2_Work-1); 
// // }







// // if (Saved_Timer_H2O2_Start != Timer_H2O2_Start) {Saved_Timer_H2O2_Start = Timer_H2O2_Start;
// //   myNex.writeStr("dim=50");
// //   myNex.writeStr("page Dispensers");
// //   myNex.writeNum("Dispensers.n0.val", getSubstring(Timer_H2O2_Start, 0, 1));
// //   myNex.writeNum("Dispensers.n1.val", getSubstring(Timer_H2O2_Start, 3, 4)); delay(50);
// //   // //Проверяем:
// //   // in_hours = myNex.readNumber("Dispensers.n0.val"); in_minutes = myNex.readNumber("Dispensers.n1.val");
// //   // sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Timer_H2O2_Start = buffer;

// // }


// // if (Saved_Timer_H2O2_Work != Timer_H2O2_Work) {Saved_Timer_H2O2_Work = Timer_H2O2_Work;
// //   myNex.writeStr("dim=50");
// //   myNex.writeStr("page Dispensers");
// //   myNex.writeNum("Dispensers.n2.val", getSubstring(Timer_H2O2_Work, 0, 1));
// //   myNex.writeNum("Dispensers.n3.val", getSubstring(Timer_H2O2_Work, 3, 4)); delay(50);
// //   // //Проверяем:
// //   // in_hours = myNex.readNumber("Dispensers.n2.val"); in_minutes = myNex.readNumber("Dispensers.n3.val");
// //   // sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Timer_H2O2_Work = buffer;

// // }


// // if (Saved_Timer_ACO_Start != Timer_ACO_Start) {Saved_Timer_ACO_Start = Timer_ACO_Start;
// //   myNex.writeStr("dim=50");
// //   myNex.writeStr("page Dispensers");
// //   myNex.writeNum("Dispensers.n4.val", getSubstring(Timer_ACO_Start, 0, 1));
// //   myNex.writeNum("Dispensers.n5.val", getSubstring(Timer_ACO_Start, 3, 4)); delay(50);
// //   // //Проверяем:
// //   // in_hours = myNex.readNumber("Dispensers.n4.val"); in_minutes = myNex.readNumber("Dispensers.n5.val");
// //   // sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Timer_ACO_Start = buffer;

// // }



// // if (Saved_Timer_ACO_Work != Timer_ACO_Work) {Saved_Timer_ACO_Work = Timer_ACO_Work;
// //   myNex.writeStr("dim=50");
// //   myNex.writeStr("page Dispensers");
// //   myNex.writeNum("Dispensers.n6.val", getSubstring(Timer_ACO_Work, 0, 1));
// //   myNex.writeNum("Dispensers.n7.val", getSubstring(Timer_ACO_Work, 3, 4)); delay(50);
// //   // //Проверяем:
// //   // in_hours = myNex.readNumber("Dispensers.n6.val"); in_minutes = myNex.readNumber("Dispensers.n7.val");
// //   // sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Timer_ACO_Work = buffer;

// // }






}
