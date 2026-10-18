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




#if 0 // Legacy sender: it wrote to hidden pages and skipped several active pages. The page-aware sender below replaces it.
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
#endif
#endif // End of the disabled legacy sender prefix.

inline void writeNextionTimerFields(const char *hourOnComponent,
                                    const char *minuteOnComponent,
                                    const char *hourOffComponent,
                                    const char *minuteOffComponent,
                                    uint16_t onMinutes,
                                    uint16_t offMinutes) {
  const uint16_t normalizedOn = onMinutes % (24U * 60U);
  const uint16_t normalizedOff = offMinutes % (24U * 60U);
  myNex.writeNum(hourOnComponent, normalizedOn / 60U);
  myNex.writeNum(minuteOnComponent, normalizedOn % 60U);
  myNex.writeNum(hourOffComponent, normalizedOff / 60U);
  myNex.writeNum(minuteOffComponent, normalizedOff % 60U);
}

inline String nextionCleanStatusText() {
  String text = CommentClean;
  text.replace(" ⏳ ", " | "); // The Nextion font has no hourglass glyph; avoid the visible question mark.
  text.replace("⏳", "");
  return nextionKoi8R(text);
}

struct NextionTimerPageCache {
  bool valid = false;
  bool power = false;
  bool timerEnabled = false;
  bool autoEnabled = false;
  uint16_t on = 0;
  uint16_t off = 0;
};

inline void syncNextionMainPage(bool force) {
  struct MainPageCache {
    bool valid = false;
    bool lamp = false;
    bool rgb = false;
    bool heat = false;
    bool topping = false;
    bool filtration = false;
    bool clean = false;
    bool h2o2 = false;
    bool aco = false;
    String h2o2Text;
    String acoText;
    String chlorineText;
    String phText;
    String poolTemperature;
    String heaterTemperature;
  };
  static MainPageCache cache;

  const bool relayFresh = rs485RelayDataFresh();
  const bool lampActive = relayFresh ? ReadRelayArray[0] : Lamp;
  const bool rgbActive = relayFresh ? ReadRelayArray[1] : Pow_WS2815;
  const bool heatActive = basinHeatingActive();
  const bool toppingActive = basinToppingActive();
  const bool filtrationActive = basinFiltrationActive();
  const bool cleanActive = CleanSequenceActive || Power_Clean || (relayFresh && ReadRelayArray[3]);
  const bool h2o2Active = Power_H2O2 || ManualPulse_H2O2_Active || (relayFresh && ReadRelayArray[5]);
  const bool acoActive = Power_ACO || ManualPulse_ACO_Active || (relayFresh && ReadRelayArray[6]);
  const String h2o2Text = Info_H2O2;
  const String acoText = Info_ACO;
  const String chlorineText = "Cl: " + String(ppmCl);
  const String phText = "PH: " + String(PH);
  const String poolTemperature = formatTemperatureString(DS1, DS1Available);
  const String heaterTemperature = formatTemperatureString(DS2, DS2Available);

  force = force || !cache.valid;
  if (force || cache.lamp != lampActive) {
    myNex.writeNum("page0.b0.pic", lampActive ? 2 : 1);
    cache.lamp = lampActive;
  }
  if (force || cache.rgb != rgbActive) {
    myNex.writeNum("page0.b12.pic", rgbActive ? 2 : 1);
    cache.rgb = rgbActive;
  }
  if (force || cache.heat != heatActive) {
    myNex.writeNum("page0.b4.pic", heatActive ? 10 : 9);
    cache.heat = heatActive;
  }
  if (force || cache.topping != toppingActive) {
    myNex.writeNum("page0.b3.pic", toppingActive ? 8 : 7);
    cache.topping = toppingActive;
  }
  if (force || cache.filtration != filtrationActive) {
    myNex.writeNum("page0.b1.pic", filtrationActive ? 4 : 3);
    cache.filtration = filtrationActive;
  }
  if (force || cache.clean != cleanActive) {
    myNex.writeNum("page0.b2.pic", cleanActive ? 6 : 5);
    cache.clean = cleanActive;
  }
  if (force || cache.h2o2 != h2o2Active) {
    myNex.writeNum("page0.b5.pic", h2o2Active ? 12 : 11);
    cache.h2o2 = h2o2Active;
  }
  if (force || cache.h2o2Text != h2o2Text) {
    myNex.writeStr("page0.b5.txt", h2o2Text);
    cache.h2o2Text = h2o2Text;
  }
  if (force || cache.aco != acoActive) {
    myNex.writeNum("page0.b6.pic", acoActive ? 12 : 11);
    cache.aco = acoActive;
  }
  if (force || cache.acoText != acoText) {
    myNex.writeStr("page0.b6.txt", acoText);
    cache.acoText = acoText;
  }
  if (force || cache.chlorineText != chlorineText) {
    myNex.writeStr("page0.b7.txt", chlorineText);
    cache.chlorineText = chlorineText;
  }
  if (force || cache.phText != phText) {
    myNex.writeStr("page0.b8.txt", phText);
    cache.phText = phText;
  }
  if (force || cache.poolTemperature != poolTemperature) {
    myNex.writeStr("page0.t0.txt", poolTemperature);
    myNex.writeStr("page0.t2.txt", poolTemperature); // Some HMI revisions use t2 as the pool-temperature duplicate.
    cache.poolTemperature = poolTemperature;
  }
  if (force || cache.heaterTemperature != heaterTemperature) {
    myNex.writeStr("page0.t1.txt", heaterTemperature);
    cache.heaterTemperature = heaterTemperature;
  }
  cache.valid = true;
}

inline void syncNextionLampPage(bool force) {
  static NextionTimerPageCache cache;
  UITimerEntry &timer = ui.timer("LampTimer");
  force = force || !cache.valid;
  if (force || cache.power != Lamp) myNex.writeNum("set_lamp.sw3.val", Lamp ? 1 : 0);
  if (force || cache.timerEnabled != Power_Time1) myNex.writeNum("set_lamp.sw0.val", Power_Time1 ? 1 : 0);
  if (force || cache.autoEnabled != Lamp_autosvet) myNex.writeNum("set_lamp.sw1.val", Lamp_autosvet ? 1 : 0);
  if (force || cache.on != timer.on || cache.off != timer.off) {
    writeNextionTimerFields("set_lamp.n0.val", "set_lamp.n1.val", "set_lamp.n2.val", "set_lamp.n3.val", timer.on, timer.off);
  }
  cache.power = Lamp;
  cache.timerEnabled = Power_Time1;
  cache.autoEnabled = Lamp_autosvet;
  cache.on = timer.on;
  cache.off = timer.off;
  cache.valid = true;
}

inline void syncNextionRgbPage(bool force) {
  static NextionTimerPageCache cache;
  UITimerEntry &timer = ui.timer("RgbTimer");
  force = force || !cache.valid;
  if (force || cache.power != Pow_WS2815) myNex.writeNum("set_RGB.sw3.val", Pow_WS2815 ? 1 : 0);
  if (force || cache.timerEnabled != WS2815_Time1) myNex.writeNum("set_RGB.sw0.val", WS2815_Time1 ? 1 : 0);
  if (force || cache.autoEnabled != Pow_WS2815_autosvet) myNex.writeNum("set_RGB.sw2.val", Pow_WS2815_autosvet ? 1 : 0);
  if (force || cache.on != timer.on || cache.off != timer.off) {
    writeNextionTimerFields("set_RGB.n0.val", "set_RGB.n1.val", "set_RGB.n2.val", "set_RGB.n3.val", timer.on, timer.off);
  }
  cache.power = Pow_WS2815;
  cache.timerEnabled = WS2815_Time1;
  cache.autoEnabled = Pow_WS2815_autosvet;
  cache.on = timer.on;
  cache.off = timer.off;
  cache.valid = true;
}

inline void syncNextionFiltrationPage(bool force) {
  struct FiltrationPageCache {
    bool valid = false;
    bool power = false;
    bool enabled1 = false;
    bool enabled2 = false;
    bool enabled3 = false;
    uint16_t on1 = 0, off1 = 0, on2 = 0, off2 = 0, on3 = 0, off3 = 0;
  };
  static FiltrationPageCache cache;
  if (force) cache.valid = false;
  if (nextionFiltrWriteHoldActive()) return;
  UITimerEntry &timer1 = ui.timer("FiltrTimer1");
  UITimerEntry &timer2 = ui.timer("FiltrTimer2");
  UITimerEntry &timer3 = ui.timer("FiltrTimer3");
  const bool power = basinFiltrationActive();
  force = force || !cache.valid;
  if (force || cache.power != power) myNex.writeNum("set_filtr.sw3.val", power ? 1 : 0);
  if (force || cache.enabled1 != Filtr_Time1) myNex.writeNum("set_filtr.sw0.val", Filtr_Time1 ? 1 : 0);
  if (force || cache.enabled2 != Filtr_Time2) myNex.writeNum("set_filtr.sw2.val", Filtr_Time2 ? 1 : 0);
  if (force || cache.enabled3 != Filtr_Time3) myNex.writeNum("set_filtr.sw1.val", Filtr_Time3 ? 1 : 0);
  if (force || cache.on1 != timer1.on || cache.off1 != timer1.off) {
    writeNextionTimerFields("set_filtr.n0.val", "set_filtr.n1.val", "set_filtr.n2.val", "set_filtr.n3.val", timer1.on, timer1.off);
  }
  if (force || cache.on2 != timer2.on || cache.off2 != timer2.off) {
    writeNextionTimerFields("set_filtr.n4.val", "set_filtr.n5.val", "set_filtr.n6.val", "set_filtr.n7.val", timer2.on, timer2.off);
  }
  if (force || cache.on3 != timer3.on || cache.off3 != timer3.off) {
    writeNextionTimerFields("set_filtr.n8.val", "set_filtr.n9.val", "set_filtr.n10.val", "set_filtr.n11.val", timer3.on, timer3.off);
  }
  cache.power = power;
  cache.enabled1 = Filtr_Time1;
  cache.enabled2 = Filtr_Time2;
  cache.enabled3 = Filtr_Time3;
  cache.on1 = timer1.on; cache.off1 = timer1.off;
  cache.on2 = timer2.on; cache.off2 = timer2.off;
  cache.on3 = timer3.on; cache.off3 = timer3.off;
  cache.valid = true;
}

inline void syncNextionCleanPage(bool force) {
  struct CleanPageCache {
    bool valid = false;
    bool power = false;
    bool timerEnabled = false;
    uint16_t on = 0, off = 0;
    bool days[7] = {false, false, false, false, false, false, false};
    String status;
  };
  static CleanPageCache cache;
  UITimerEntry &timer = ui.timer("CleanTimer1");
  const bool days[7] = {chk1, chk2, chk3, chk4, chk5, chk6, chk7};
  const String status = nextionCleanStatusText();
  force = force || !cache.valid;
  if (force || cache.power != Power_Clean) myNex.writeNum("Clean.sw1.val", Power_Clean ? 1 : 0);
  if (force || cache.timerEnabled != Clean_Time1) myNex.writeNum("Clean.sw0.val", Clean_Time1 ? 1 : 0);
  if (force || cache.on != timer.on || cache.off != timer.off) {
    writeNextionTimerFields("Clean.n0.val", "Clean.n1.val", "Clean.n2.val", "Clean.n3.val", timer.on, timer.off);
  }
  for (uint8_t day = 0; day < 7; ++day) {
    if (force || cache.days[day] != days[day]) {
      myNex.writeNum("Clean.bt" + String(day) + ".val", days[day] ? 1 : 0);
      cache.days[day] = days[day];
    }
  }
  if (force || cache.status != status) {
    myNex.writeStr("Clean.t3.txt", status);
    cache.status = status;
  }
  cache.power = Power_Clean;
  cache.timerEnabled = Clean_Time1;
  cache.on = timer.on;
  cache.off = timer.off;
  cache.valid = true;
}

inline void syncNextionHeatPage(bool force) {
  struct HeatPageCache {
    bool valid = false;
    bool enabled = false;
    int setpoint = 0;
    String poolTemperature;
    String heaterTemperature;
  };
  static HeatPageCache cache;
  const String poolTemperature = formatTemperatureString(DS1, DS1Available);
  const String heaterTemperature = formatTemperatureString(DS2, DS2Available);
  force = force || !cache.valid;
  if (force || cache.enabled != Activation_Heat) myNex.writeNum("heat.sw0.val", Activation_Heat ? 1 : 0);
  if (force || cache.setpoint != Sider_heat) {
    myNex.writeNum("heat.h0.val", Sider_heat);
    myNex.writeNum("heat.n0.val", Sider_heat);
  }
  if (force || cache.poolTemperature != poolTemperature) {
    myNex.writeStr("heat.t1.txt", poolTemperature);
    myNex.writeStr("heat.t2.txt", poolTemperature);
    cache.poolTemperature = poolTemperature;
  }
  if (force || cache.heaterTemperature != heaterTemperature) {
    myNex.writeStr("heat.t0.txt", heaterTemperature);
    cache.heaterTemperature = heaterTemperature;
  }
  cache.enabled = Activation_Heat;
  cache.setpoint = Sider_heat;
  cache.valid = true;
}

inline void syncNextionWaterPage(bool force) {
  struct WaterPageCache {
    bool valid = false;
    bool automatic = false;
    bool drain = false;
    bool topping = false;
    bool upper = false;
    bool lower = false;
    bool pit = false;
    String status;
  };
  static WaterPageCache cache;
  const String status = waterLevelNextionText();
  force = force || !cache.valid;
  if (force || cache.automatic != Activation_Water_Level) myNex.writeNum("set_topping.sw0.val", Activation_Water_Level ? 1 : 0);
  if (force || cache.drain != Power_Drain) myNex.writeNum("set_topping.sw1.val", Power_Drain ? 1 : 0);
  if (force || cache.topping != Power_Topping) myNex.writeNum("set_topping.sw2.val", Power_Topping ? 1 : 0);
  if (force || cache.upper != WaterLevelSensorUpper || cache.lower != WaterLevelSensorLower || cache.pit != DrainPitFullConfirmed) {
    writeNextionLevelIndicators();
  }
  if (force || cache.status != status) myNex.writeStr("set_topping.t0.txt", nextionKoi8R(status));
  cache.automatic = Activation_Water_Level;
  cache.drain = Power_Drain;
  cache.topping = Power_Topping;
  cache.upper = WaterLevelSensorUpper;
  cache.lower = WaterLevelSensorLower;
  cache.pit = DrainPitFullConfirmed;
  cache.status = status;
  cache.valid = true;
}

inline void syncNextionRtcPage(bool force) {
  static bool valid = false;
  static int savedOffset = 0;
  if (force || !valid || savedOffset != gmtOffset_correct) {
    myNex.writeNum("pageRTC.n5.val", gmtOffset_correct);
    savedOffset = gmtOffset_correct;
    valid = true;
  }
}

inline void syncNextionServicePage(bool force) {
  static String savedText;
  static unsigned long lastRefreshAt = 0;
  if (!force && millis() - lastRefreshAt < 1000UL) return;
  const String text = serviceNextionText();
  if (force || savedText != text) {
    myNex.writeStr("Service.t0.txt", text);
    savedText = text;
  }
  lastRefreshAt = millis();
}

inline void syncNextionDispensersPage(bool force) {
  struct DispensersPageCache {
    bool valid = false;
    String phText;
    String chlorineText;
    bool phControl = false;
    bool chlorineControl = false;
    int acoMode = 0;
    int chlorineMode = 0;
    int phLowerWhole = 0, phLowerTenth = 0, phUpperWhole = 0, phUpperTenth = 0;
    int clLowerWhole = 0, clLowerTenth = 0, clUpperWhole = 0, clUpperTenth = 0;
  };
  static DispensersPageCache cache;
  const String phText = "PH: " + String(PH);
  const String chlorineText = "Cl: " + String(ppmCl);
  if (force) cache.valid = false;
  if (force || cache.phText != phText) myNex.writeStr("Dispensers.t0.txt", phText);
  if (force || cache.chlorineText != chlorineText) myNex.writeStr("Dispensers.t1.txt", chlorineText);
  cache.phText = phText;
  cache.chlorineText = chlorineText;

  if (nextionDispensersWriteHoldActive()) return;
  force = force || !cache.valid;
  normalizeChemicalLimits();
  const int acoMode = nextionDosingComboIndexFromMode(ACO_Work);
  const int chlorineMode = nextionDosingComboIndexFromMode(H2O2_Work);
  const int phLowerWhole = nextionTenthsWhole(PH_Lower);
  const int phLowerTenth = nextionTenthsDigit(PH_Lower);
  const int phUpperWhole = nextionTenthsWhole(PH_Upper);
  const int phUpperTenth = nextionTenthsDigit(PH_Upper);
  const int clLowerWhole = nextionTenthsWhole(CL_Lower);
  const int clLowerTenth = nextionTenthsDigit(CL_Lower);
  const int clUpperWhole = nextionTenthsWhole(CL_Upper);
  const int clUpperTenth = nextionTenthsDigit(CL_Upper);
  if (force || cache.phControl != PH_Control_ACO) myNex.writeNum("Dispensers.sw0.val", PH_Control_ACO ? 1 : 0);
  if (force || cache.acoMode != acoMode) myNex.writeNum("Dispensers.cb0.val", acoMode);
  if (force || cache.chlorineControl != NaOCl_H2O2_Control) myNex.writeNum("Dispensers.sw2.val", NaOCl_H2O2_Control ? 1 : 0);
  if (force || cache.chlorineMode != chlorineMode) myNex.writeNum("Dispensers.cb1.val", chlorineMode);
  if (force || cache.phLowerWhole != phLowerWhole) myNex.writeNum("Dispensers.n0.val", phLowerWhole);
  if (force || cache.phLowerTenth != phLowerTenth) myNex.writeNum("Dispensers.n1.val", phLowerTenth);
  if (force || cache.phUpperWhole != phUpperWhole) myNex.writeNum("Dispensers.n2.val", phUpperWhole);
  if (force || cache.phUpperTenth != phUpperTenth) myNex.writeNum("Dispensers.n3.val", phUpperTenth);
  if (force || cache.clLowerWhole != clLowerWhole) myNex.writeNum("Dispensers.n6.val", clLowerWhole);
  if (force || cache.clLowerTenth != clLowerTenth) myNex.writeNum("Dispensers.n7.val", clLowerTenth);
  if (force || cache.clUpperWhole != clUpperWhole) myNex.writeNum("Dispensers.n4.val", clUpperWhole);
  if (force || cache.clUpperTenth != clUpperTenth) myNex.writeNum("Dispensers.n5.val", clUpperTenth);
  cache.phControl = PH_Control_ACO;
  cache.chlorineControl = NaOCl_H2O2_Control;
  cache.acoMode = acoMode;
  cache.chlorineMode = chlorineMode;
  cache.phLowerWhole = phLowerWhole; cache.phLowerTenth = phLowerTenth;
  cache.phUpperWhole = phUpperWhole; cache.phUpperTenth = phUpperTenth;
  cache.clLowerWhole = clLowerWhole; cache.clLowerTenth = clLowerTenth;
  cache.clUpperWhole = clUpperWhole; cache.clUpperTenth = clUpperTenth;
  cache.valid = true;
}

void Nextion_Transmit(int interval) {
  static unsigned long lastTransmitAt = 0;
  static int lastPage = -1;
  const unsigned long now = millis();
  if (now - lastTransmitAt < static_cast<unsigned long>(interval)) return;
  if (nextionAsyncReadActive() || nextionPageReadBatchPending() || MySerial.available() > 0) return; // Never mix writes into an active request/response exchange.
  lastTransmitAt = now;

  if (myNex.currentPageId >= 0 && myNex.currentPageId < 50) Nx_page_id = myNex.currentPageId;
  const bool force = Nx_page_id != lastPage;
  lastPage = Nx_page_id;

  switch (Nx_page_id) {
    case 0: syncNextionMainPage(force); break;
    case 1: syncNextionLampPage(force); break;
    case 2: syncNextionRgbPage(force); break;
    case 3: syncNextionFiltrationPage(force); break;
    case 4: syncNextionCleanPage(force); break;
    case 5: syncNextionHeatPage(force); break;
    case 6: syncNextionWaterPage(force); break;
    case 7: syncNextionRtcPage(force); break;
    case 8: syncNextionServicePage(force); break;
    case 9: syncNextionDispensersPage(force); break;
    default: break;
  }
}

#if 0 // Remaining body of the legacy sender; retained only as migration history.
#if 0 // Balances the original legacy subsection's #endif below.

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
#endif // End of the remaining disabled legacy sender body.
