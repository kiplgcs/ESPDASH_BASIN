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

constexpr uint16_t NX_COLOR_GREEN = 2016; // Зеленый RGB565 для нормального состояния уровнемера.
constexpr uint16_t NX_COLOR_YELLOW = 65504; // Желтый RGB565 для предупреждения по верхнему уровню.
constexpr uint16_t NX_COLOR_RED = 63488; // Красный RGB565 для аварийного/сработавшего уровня.
constexpr uint16_t NX_COLOR_WHITE = 65535; // Белый RGB565 для символа внутри индикатора.

inline void writeNextionLevelIndicator(const String &component, bool active, uint16_t activeColor, uint16_t normalColor) {
  myNex.writeNum("set_topping." + component + ".val", active ? 1 : 0); // Галочка/состояние самого индикатора.
  myNex.writeNum("set_topping." + component + ".bco", active ? activeColor : normalColor); // Цвет фона делает пиктограмму понятной.
  myNex.writeNum("set_topping." + component + ".pco", NX_COLOR_WHITE); // Символ индикатора оставляем контрастным.
}

inline void writeNextionLevelIndicators() {
  const bool upperBelow = WaterLevelSensorUpper; // Верхний контакт замкнут, когда уровень ниже верхнего датчика.
  const bool lowerLow = WaterLevelSensorLower; // Нижний контакт замкнут, когда уровень ниже нижнего датчика.
  const bool drainFull = DrainPitFullConfirmed; // Яма заполнена, когда семантический верхний уровень подтвержден.
  writeNextionLevelIndicator("c0", upperBelow, NX_COLOR_YELLOW, NX_COLOR_GREEN); // c0: верхний датчик бассейна.
  writeNextionLevelIndicator("c1", lowerLow, NX_COLOR_RED, NX_COLOR_GREEN); // c1: нижний датчик бассейна.
  writeNextionLevelIndicator("c2", drainFull, NX_COLOR_RED, NX_COLOR_GREEN); // c2: верхний датчик сливной ямы.
}




void Nextion_Transmit (int interval) {
  static unsigned long timer;
  if (interval + timer > millis()) return; 
  timer = millis();
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------

// /////////////////////////************* page set_lamp  **************/////////////////////////////
// ////////////////////////************* page set_lamp  **************//////////////////////////////
// ///////////////////////************* page set_lamp  **************///////////////////////////////

#if 0 // Старый блок принудительно переключал страницы Nextion и тормозил ввод; синхронизация ниже идет порциями без page.
if (Lamp != Lamp1 && !triggerRestartNextion){Lamp1 = Lamp;
  myNex.writeStr("dim=50");
  myNex.writeNum("page0.b0.pic", Lamp ? 2 : 1); 
  myNex.writeStr("page set_lamp");
  myNex.writeNum("set_lamp.sw3.val", Lamp ? 1 : 0);

  // Error err = RS485.addRequest(40001,1,0x05,0, Lamp ? devices[0].value : devices[1].value);

}//pcfRelays.digitalWrite(P0, Lamp ? LOW : HIGH);
  





if (Saved_Lamp_autosvet != Lamp_autosvet && !triggerRestartNextion){Saved_Lamp_autosvet = Lamp_autosvet;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_lamp");
  myNex.writeNum("set_lamp.sw1.val", Lamp_autosvet ? 1 : 0); 
}

if (Saved_Power_Time1 != Power_Time1 && !triggerRestartNextion){ Saved_Power_Time1 = Power_Time1;
  myNex.writeStr("dim=50");  
  myNex.writeStr("page set_lamp");
  myNex.writeNum("set_lamp.sw0.val", Power_Time1 ? 1 : 0); 
}

UITimerEntry &lampTimer = ui.timer("LampTimer");
if (Saved_Lamp_timeON1 != lampTimer.on && !triggerRestartNextion) {Saved_Lamp_timeON1 = lampTimer.on;  myNex.writeStr("dim=50");
  myNex.writeStr("page set_lamp");
  String lampOnStr = formatMinutesToTime(lampTimer.on);
  myNex.writeNum("set_lamp.n0.val", getSubstring(lampOnStr, 0, 1));
  myNex.writeNum("set_lamp.n1.val", getSubstring(lampOnStr, 3, 4));
}

if (Saved_Lamp_timeOFF1 != lampTimer.off && !triggerRestartNextion) {Saved_Lamp_timeOFF1 = lampTimer.off;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_lamp");
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
    myNex.writeStr("page set_RGB"); 
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
  myNex.writeStr("page set_RGB"); 
  myNex.writeNum("set_RGB.sw2.val", Pow_WS2815_autosvet ? 1 : 0); 
}

if (Saved_WS2815_Time1 != WS2815_Time1 && !triggerRestartNextion){ Saved_WS2815_Time1 = WS2815_Time1;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_RGB"); //отобразить страницу
  myNex.writeNum("set_RGB.sw0.val", WS2815_Time1); 
}


UITimerEntry &rgbTimer = ui.timer("RgbTimer");
if (Saved_timeON_WS2815 != rgbTimer.on && !triggerRestartNextion){Saved_timeON_WS2815 = rgbTimer.on;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_RGB"); //отобразить страницу
  String rgbOnStr = formatMinutesToTime(rgbTimer.on);
  myNex.writeNum("set_RGB.n0.val", getSubstring(rgbOnStr, 0, 1));
  myNex.writeNum("set_RGB.n1.val", getSubstring(rgbOnStr, 3, 4));
} 

if (Saved_timeOFF_WS2815 != rgbTimer.off && !triggerRestartNextion){Saved_timeOFF_WS2815 = rgbTimer.off;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_RGB");//отобразить страницу
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
    myNex.writeStr("page set_filtr");
    myNex.writeNum("set_filtr.sw3.val", Power_Filtr ? 1 : 0);
}   //pcfRelays.digitalWrite(P2, Power_Filtr ? LOW : HIGH);

// if (Saved_Filtr_Time1 != Filtr_Time1 && !triggerRestartNextion){Saved_Filtr_Time1 = Filtr_Time1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.sw0.val", Filtr_Time1 ? 1 : 0); 
// }

if (Saved_Filtr_Time1 != Filtr_Time1 && !triggerRestartNextion){Saved_Filtr_Time1 = Filtr_Time1;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_filtr");
  myNex.writeNum("set_filtr.sw0.val", Filtr_Time1 ? 1 : 0); 
}

// if (Saved_Filtr_Time2 != Filtr_Time2 && !triggerRestartNextion){Saved_Filtr_Time2 = Filtr_Time2;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.sw2.val", Filtr_Time2 ? 1 : 0); 
// }

if (Saved_Filtr_Time2 != Filtr_Time2 && !triggerRestartNextion){Saved_Filtr_Time2 = Filtr_Time2;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_filtr");
  myNex.writeNum("set_filtr.sw2.val", Filtr_Time2 ? 1 : 0); 
}

// if (Saved_Filtr_Time3 != Filtr_Time3 && !triggerRestartNextion){Saved_Filtr_Time3 = Filtr_Time3;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page set_filtr");
//   myNex.writeNum("set_filtr.sw1.val", Filtr_Time3 ? 1 : 0); 
// }

if (Saved_Filtr_Time3 != Filtr_Time3 && !triggerRestartNextion){Saved_Filtr_Time3 = Filtr_Time3;
  myNex.writeStr("dim=50");
  myNex.writeStr("page set_filtr");
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
  myNex.writeStr("page set_filtr");
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
  myNex.writeStr("page set_filtr");
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
  myNex.writeStr("page set_filtr");
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
  myNex.writeStr("page set_filtr");
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
  myNex.writeStr("page set_filtr");
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
  myNex.writeStr("page set_filtr");
  String filtrOff3Str = formatMinutesToTime(filtrTimer3.off);
  myNex.writeNum("set_filtr.n10.val", getSubstring(filtrOff3Str, 0, 1));
  myNex.writeNum("set_filtr.n11.val", getSubstring(filtrOff3Str, 3, 4));
}
#endif // Конец отключенного legacy-блока set_lamp/set_RGB/set_filtr с принудительными переходами страниц.

// Активная синхронизация фильтрации без принудительного перехода на страницу.
// Показываем факт работы насоса: команда Power_Filtr или уже включенное RS485-реле.
const bool nextionFiltrActualState = Power_Filtr || ReadRelayArray[8];
static bool savedNextionFiltrActualState = false;
static int savedNextionFiltrPageId = -1;
if (Power_Filtr1 != nextionFiltrActualState && !triggerRestartNextion && !nextionFiltrWriteHoldActive()) {
  Power_Filtr1 = nextionFiltrActualState;
  myNex.writeNum("page0.b1.pic", nextionFiltrActualState ? 4 : 3);
}
if (Nx_page_id != 3) savedNextionFiltrPageId = -1;
if (Nx_page_id == 3 && !triggerRestartNextion && !nextionFiltrWriteHoldActive() &&
    (savedNextionFiltrPageId != 3 || savedNextionFiltrActualState != nextionFiltrActualState)) {
  savedNextionFiltrPageId = 3;
  savedNextionFiltrActualState = nextionFiltrActualState;
  myNex.writeNum("set_filtr.sw3.val", nextionFiltrActualState ? 1 : 0);
}

static int savedNextionFiltrTimerPageId = -1;
static bool savedNextionFiltrTime1 = false;
static bool savedNextionFiltrTime2 = false;
static bool savedNextionFiltrTime3 = false;
if (Nx_page_id != 3) savedNextionFiltrTimerPageId = -1;
if (Nx_page_id == 3 && !triggerRestartNextion && !nextionFiltrWriteHoldActive()) {
  const bool forceFiltrTimerSync = savedNextionFiltrTimerPageId != 3;
  if (forceFiltrTimerSync || savedNextionFiltrTime1 != Filtr_Time1) {
    savedNextionFiltrTime1 = Filtr_Time1;
    myNex.writeNum("set_filtr.sw0.val", Filtr_Time1 ? 1 : 0);
  }
  if (forceFiltrTimerSync || savedNextionFiltrTime2 != Filtr_Time2) {
    savedNextionFiltrTime2 = Filtr_Time2;
    myNex.writeNum("set_filtr.sw2.val", Filtr_Time2 ? 1 : 0);
  }
  if (forceFiltrTimerSync || savedNextionFiltrTime3 != Filtr_Time3) {
    savedNextionFiltrTime3 = Filtr_Time3;
    myNex.writeNum("set_filtr.sw1.val", Filtr_Time3 ? 1 : 0);
  }
    static uint16_t savedNextionFiltrOn1 = 65535;
  static uint16_t savedNextionFiltrOff1 = 65535;
  static uint16_t savedNextionFiltrOn2 = 65535;
  static uint16_t savedNextionFiltrOff2 = 65535;
  static uint16_t savedNextionFiltrOn3 = 65535;
  static uint16_t savedNextionFiltrOff3 = 65535;
  auto writeFiltrTimeToNextion = [](const char *hourComponent, const char *minuteComponent, uint16_t minutes) {
    String timeText = formatMinutesToTime(minutes);
    myNex.writeNum(hourComponent, getSubstring(timeText, 0, 1));
    myNex.writeNum(minuteComponent, getSubstring(timeText, 3, 4));
  };
  UITimerEntry &filtrTimer1Active = ui.timer("FiltrTimer1");
  UITimerEntry &filtrTimer2Active = ui.timer("FiltrTimer2");
  UITimerEntry &filtrTimer3Active = ui.timer("FiltrTimer3");
  if (forceFiltrTimerSync || savedNextionFiltrOn1 != filtrTimer1Active.on) {
    savedNextionFiltrOn1 = filtrTimer1Active.on;
    writeFiltrTimeToNextion("set_filtr.n0.val", "set_filtr.n1.val", filtrTimer1Active.on);
  }
  if (forceFiltrTimerSync || savedNextionFiltrOff1 != filtrTimer1Active.off) {
    savedNextionFiltrOff1 = filtrTimer1Active.off;
    writeFiltrTimeToNextion("set_filtr.n2.val", "set_filtr.n3.val", filtrTimer1Active.off);
  }
  if (forceFiltrTimerSync || savedNextionFiltrOn2 != filtrTimer2Active.on) {
    savedNextionFiltrOn2 = filtrTimer2Active.on;
    writeFiltrTimeToNextion("set_filtr.n4.val", "set_filtr.n5.val", filtrTimer2Active.on);
  }
  if (forceFiltrTimerSync || savedNextionFiltrOff2 != filtrTimer2Active.off) {
    savedNextionFiltrOff2 = filtrTimer2Active.off;
    writeFiltrTimeToNextion("set_filtr.n6.val", "set_filtr.n7.val", filtrTimer2Active.off);
  }
  if (forceFiltrTimerSync || savedNextionFiltrOn3 != filtrTimer3Active.on) {
    savedNextionFiltrOn3 = filtrTimer3Active.on;
    writeFiltrTimeToNextion("set_filtr.n8.val", "set_filtr.n9.val", filtrTimer3Active.on);
  }
  if (forceFiltrTimerSync || savedNextionFiltrOff3 != filtrTimer3Active.off) {
    savedNextionFiltrOff3 = filtrTimer3Active.off;
    writeFiltrTimeToNextion("set_filtr.n10.val", "set_filtr.n11.val", filtrTimer3Active.off);
  }
  savedNextionFiltrTimerPageId = 3;
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

if (Power_Clean1 != Power_Clean && !triggerRestartNextion) {
    Power_Clean1 = Power_Clean; // Запоминаем состояние промывки, чтобы не отправлять одно и то же постоянно.
    myNex.writeNum("page0.b2.pic", Power_Clean ? 6 : 5); // Обновляем кнопку главной страницы без delay.
    if (Nx_page_id == 4) myNex.writeNum("Clean.sw1.val", Power_Clean ? 1 : 0); // Переключатель Clean обновляем только на открытой странице.
}  //pcfRelays.digitalWrite(P3, Power_Clean ? LOW : HIGH);

// if (Saved_Clean_Time1 != Clean_Time1 && !triggerRestartNextion) {Saved_Clean_Time1=Clean_Time1;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.sw0.val", Clean_Time1 ? 1 : 0);
// }

if (Saved_Clean_Time1 != Clean_Time1 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_Clean_Time1=Clean_Time1; // Запоминаем состояние таймера промывки после записи на открытую страницу.
    myNex.writeNum("Clean.sw0.val", Clean_Time1 ? 1 : 0); // Пишем sw0 без принудительного перехода страницы.
}

// if (Saved_Clean_timeON1 != Clean_timeON1 && !triggerRestartNextion) {Saved_Clean_timeON1 = Clean_timeON1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page Clean");
//   myNex.writeNum("Clean.n0.val", getSubstring(Clean_timeON1, 0, 1));
//   myNex.writeNum("Clean.n1.val", getSubstring(Clean_timeON1, 3, 4));
// }

UITimerEntry &cleanTimer = ui.timer("CleanTimer1");
if (Saved_Clean_timeON1 != cleanTimer.on && !triggerRestartNextion && Nx_page_id == 4) {
  Saved_Clean_timeON1 = cleanTimer.on; // Запоминаем время старта промывки после записи на открытую страницу.
  String cleanOnStr = formatMinutesToTime(cleanTimer.on); // Переводим минуты в формат HH:MM.
  myNex.writeNum("Clean.n0.val", getSubstring(cleanOnStr, 0, 1)); // Пишем часы старта без page Clean.
  myNex.writeNum("Clean.n1.val", getSubstring(cleanOnStr, 3, 4)); // Пишем минуты старта без page Clean.
}

// if (Saved_Clean_timeOFF1 != Clean_timeOFF1 && !triggerRestartNextion) {Saved_Clean_timeOFF1 = Clean_timeOFF1;
//   myNex.writeStr("dim=50");
//   myNex.writeStr("page Clean");
//   myNex.writeNum("Clean.n2.val", getSubstring(Clean_timeOFF1, 0, 1));
//   myNex.writeNum("Clean.n3.val", getSubstring(Clean_timeOFF1, 3, 4));
// }
if (Saved_Clean_timeOFF1 != cleanTimer.off && !triggerRestartNextion && Nx_page_id == 4) {
  Saved_Clean_timeOFF1 = cleanTimer.off; // Запоминаем время окончания промывки после записи на открытую страницу.
  String cleanOffStr = formatMinutesToTime(cleanTimer.off); // Переводим минуты в формат HH:MM.
  myNex.writeNum("Clean.n2.val", getSubstring(cleanOffStr, 0, 1)); // Пишем часы окончания без page Clean.
  myNex.writeNum("Clean.n3.val", getSubstring(cleanOffStr, 3, 4)); // Пишем минуты окончания без page Clean.
}

// if (Saved_chk1 != chk1 && !triggerRestartNextion) {Saved_chk1 = chk1;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt0.val", chk1? 1 : 0);
// }

if (Saved_chk1 != chk1 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_chk1 = chk1; // Запоминаем понедельник после записи на открытую страницу.
    myNex.writeNum("Clean.bt0.val", chk1? 1 : 0); // Пишем день без переключения страницы.
}

// if (Saved_chk2 != chk2 && !triggerRestartNextion) {Saved_chk2 = chk2;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt1.val", chk2? 1 : 0);
// }

if (Saved_chk2 != chk2 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_chk2 = chk2; // Запоминаем вторник после записи на открытую страницу.
    myNex.writeNum("Clean.bt1.val", chk2? 1 : 0); // Пишем день без переключения страницы.
}

// if (Saved_chk3 != chk3 && !triggerRestartNextion) {Saved_chk3 = chk3;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt2.val", chk3? 1 : 0);
// }

if (Saved_chk3 != chk3 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_chk3 = chk3; // Запоминаем среду после записи на открытую страницу.
    myNex.writeNum("Clean.bt2.val", chk3? 1 : 0); // Пишем день без переключения страницы.
}

// if (Saved_chk4 != chk4 && !triggerRestartNextion) {Saved_chk4 = chk4;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt3.val", chk4? 1 : 0);
// }

if (Saved_chk4 != chk4 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_chk4 = chk4; // Запоминаем четверг после записи на открытую страницу.
    myNex.writeNum("Clean.bt3.val", chk4? 1 : 0); // Пишем день без переключения страницы.
}

// if (Saved_chk5 != chk5 && !triggerRestartNextion) {Saved_chk5 = chk5;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt4.val", chk5? 1 : 0);
// }
if (Saved_chk5 != chk5 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_chk5 = chk5; // Запоминаем пятницу после записи на открытую страницу.
    myNex.writeNum("Clean.bt4.val", chk5? 1 : 0); // Пишем день без переключения страницы.
}


// if (Saved_chk6 != chk6 && !triggerRestartNextion) {Saved_chk6 = chk6;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt5.val", chk6? 1 : 0);
// }
if (Saved_chk6 != chk6 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_chk6 = chk6; // Запоминаем субботу после записи на открытую страницу.
    myNex.writeNum("Clean.bt5.val", chk6? 1 : 0); // Пишем день без переключения страницы.
}

// if (Saved_chk7 != chk7 && !triggerRestartNextion) {Saved_chk7 = chk7;
//     myNex.writeStr("dim=50");
//     myNex.writeStr("page Clean");
//     myNex.writeNum("Clean.bt6.val", chk7? 1 : 0);
// }

if (Saved_chk7 != chk7 && !triggerRestartNextion && Nx_page_id == 4) {
    Saved_chk7 = chk7; // Запоминаем воскресенье после записи на открытую страницу.
    myNex.writeNum("Clean.bt6.val", chk7? 1 : 0); // Пишем день без переключения страницы.
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
  if (Sider_heat != Sider_heat1 && !triggerRestartNextion && Nx_page_id == 5){
    Sider_heat1 = Sider_heat; // Запоминаем уставку после записи на открытую страницу нагрева.
    myNex.writeNum("heat.h0.val", Sider_heat); // Обновляем слайдер нагрева без delay.
    myNex.writeNum("heat.n0.val", Sider_heat); // Обновляем число нагрева без обратного чтения.
  }

  if (Activation_Heat != Activation_Heat1 && !triggerRestartNextion && Nx_page_id == 5) {
    Activation_Heat1 = Activation_Heat; // Запоминаем режим нагрева после записи на открытую страницу.
    myNex.writeNum("heat.sw0.val", Activation_Heat); // Пишем переключатель нагрева без page heat и delay.
  }

// /////////////////////////************* set_topping  **************/////////////////////////////
// ////////////////////////************* set_topping **************//////////////////////////////
// ///////////////////////************* set_topping **************///////////////////////////////

// if (Power_Topping1 != Power_Topping && !triggerRestartNextion) {Power_Topping1 = Power_Topping; // При изменении обновляем картинку долива воды на Nextion
//     myNex.writeStr("dim=50");
//     myNex.writeNum("page0.b3.pic", Power_Topping ? 8 : 7); 
// }

#if 1 // Синхронизация set_topping включена только на открытой странице, без принудительного page set_topping.
static bool SavedNxActivationWaterLevel = false; // Запоминаем последнее отправленное состояние контроля уровня.
static bool SavedNxPowerDrain = false; // Запоминаем последнее отправленное состояние кнопки слива.
static bool SavedNxPowerTopping = false; // Запоминаем последнее отправленное состояние клапана долива.
static bool SavedNxUpperLevel = false; // Запоминаем последний отправленный верхний датчик бассейна.
static bool SavedNxLowerLevel = false; // Запоминаем последний отправленный нижний датчик бассейна.
static bool SavedNxDrainLevel = false; // Запоминаем последний отправленный датчик сливной ямы.
static int SavedNxWaterPageId = -1; // Запоминаем вход на страницу set_topping для принудительной первичной синхронизации.
static String SavedNxWaterText = ""; // Запоминаем последнюю информационную строку уровня воды.
String currentWaterText = waterLevelNextionText(); // Формируем текущий короткий статус уровня воды.
if (Nx_page_id != 6) SavedNxWaterPageId = -1; // При уходе со страницы разрешаем новый полный sync при следующем входе.
if (Nx_page_id == 6 && !triggerRestartNextion &&
    (SavedNxWaterPageId != 6 ||
     SavedNxActivationWaterLevel != Activation_Water_Level ||
     SavedNxPowerDrain != Power_Drain ||
     SavedNxPowerTopping != Power_Topping ||
     SavedNxUpperLevel != WaterLevelSensorUpper ||
     SavedNxLowerLevel != WaterLevelSensorLower ||
     SavedNxDrainLevel != WaterLevelSensorDrain ||
     SavedNxWaterText != currentWaterText)) { // Отправляем в Nextion только при изменениях, чтобы не забивать UART.
  SavedNxWaterPageId = 6; // Отмечаем, что текущий вход на страницу уже синхронизирован.
  SavedNxActivationWaterLevel = Activation_Water_Level; // Обновляем сохраненное состояние контроля уровня.
  SavedNxPowerDrain = Power_Drain; // Обновляем сохраненное состояние слива.
  SavedNxPowerTopping = Power_Topping; // Обновляем сохраненное состояние долива.
  SavedNxUpperLevel = WaterLevelSensorUpper; // Обновляем сохраненный верхний датчик.
  SavedNxLowerLevel = WaterLevelSensorLower; // Обновляем сохраненный нижний датчик.
  SavedNxDrainLevel = WaterLevelSensorDrain; // Обновляем сохраненный датчик ямы.
  SavedNxWaterText = currentWaterText; // Обновляем сохраненный текст.
  myNex.writeNum("set_topping.sw0.val", Activation_Water_Level ? 1 : 0); // sw0: контроль уровня.
  myNex.writeNum("set_topping.sw1.val", Power_Drain ? 1 : 0); // sw1: слив.
  myNex.writeNum("set_topping.sw2.val", Power_Topping ? 1 : 0); // sw2: клапан долива.
  writeNextionLevelIndicators(); // c0/c1/c2: меняем именно пиктограммы уровнемеров, а не только текст.
  myNex.writeStr("set_topping.t0.txt", nextionKoi8R(currentWaterText)); // t0: понятная строка этапа работы.
}
#endif // Конец синхронизации set_topping без фонового переключения страниц.

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
static int SavedNxDispensersPageId = -1; // Запоминаем вход на страницу дозаторов для первичной синхронизации.
if (Nx_page_id != 9) SavedNxDispensersPageId = -1; // При уходе со страницы разрешаем новый полный sync при следующем входе.
const bool forceDispensersSync = (Nx_page_id == 9 && SavedNxDispensersPageId != 9); // true только при первом проходе после входа.
if (Nx_page_id == 9 && !nextionDispensersWriteHoldActive() && (forceDispensersSync || Saved_PHControlACO != PH_Control_ACO) && !triggerRestartNextion) {Saved_PHControlACO = PH_Control_ACO;
    myNex.writeNum("Dispensers.sw0.val", PH_Control_ACO ? 1 : 0);
}

if (Nx_page_id == 9 && !nextionDispensersWriteHoldActive() && (forceDispensersSync || ACO_Work != Saved_ACO_Work) && !triggerRestartNextion) {Saved_ACO_Work = ACO_Work;
  myNex.writeNum("Dispensers.cb0.val", nextionDosingComboIndexFromMode(ACO_Work)); // В Nextion пишем индекс строки, а не внутренний код периода.
}

if (Nx_page_id == 9 && !nextionDispensersWriteHoldActive() && (forceDispensersSync || Saved_NaOCl_H2O2_Control != NaOCl_H2O2_Control) && !triggerRestartNextion) {
    Saved_NaOCl_H2O2_Control = NaOCl_H2O2_Control;
    myNex.writeNum("Dispensers.sw2.val", NaOCl_H2O2_Control ? 1 : 0);
}

if (Nx_page_id == 9 && !nextionDispensersWriteHoldActive() && (forceDispensersSync || H2O2_Work != Saved_H2O2_Work) && !triggerRestartNextion) {Saved_H2O2_Work = H2O2_Work;
  myNex.writeNum("Dispensers.cb1.val", nextionDosingComboIndexFromMode(H2O2_Work)); // В Nextion пишем индекс строки, а не внутренний код периода.
}

static float Saved_PH_Lower_Nextion = -100.0f;
static float Saved_PH_Upper_Nextion = -100.0f;
static float Saved_CL_Lower_Nextion = -100.0f;
static float Saved_CL_Upper_Nextion = -100.0f;
if (Nx_page_id == 9 && !nextionDispensersWriteHoldActive() && !triggerRestartNextion && (forceDispensersSync || Saved_PH_Lower_Nextion != PH_Lower || Saved_PH_Upper_Nextion != PH_Upper || Saved_CL_Lower_Nextion != CL_Lower || Saved_CL_Upper_Nextion != CL_Upper)) {
  normalizeChemicalLimits(); // Перед выводом на Nextion гарантируем допустимые пределы pH/CL.
  Saved_PH_Lower_Nextion = PH_Lower;
  Saved_PH_Upper_Nextion = PH_Upper;
  Saved_CL_Lower_Nextion = CL_Lower;
  Saved_CL_Upper_Nextion = CL_Upper;
  myNex.writeNum("Dispensers.n0.val", nextionTenthsWhole(PH_Lower));
  myNex.writeNum("Dispensers.n1.val", nextionTenthsDigit(PH_Lower));
  myNex.writeNum("Dispensers.n2.val", nextionTenthsWhole(PH_Upper));
  myNex.writeNum("Dispensers.n3.val", nextionTenthsDigit(PH_Upper));
  myNex.writeNum("Dispensers.n6.val", nextionTenthsWhole(CL_Lower));
  myNex.writeNum("Dispensers.n7.val", nextionTenthsDigit(CL_Lower));
  myNex.writeNum("Dispensers.n4.val", nextionTenthsWhole(CL_Upper));
  myNex.writeNum("Dispensers.n5.val", nextionTenthsDigit(CL_Upper));
}
if (Nx_page_id == 9 && !triggerRestartNextion) SavedNxDispensersPageId = 9; // После всех записей считаем страницу дозаторов синхронизированной.


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
