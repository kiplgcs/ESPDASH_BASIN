#pragma once
#include <Arduino.h>
#include "web.h"
#include "NPT_Time.h"
#include "ModbusRTU_RS485.h"

// Функция для проверки времени в заданном интервале (минуты от 00:00)
bool checkTimeInInterval(int currentHour, int currentMinute, uint16_t startMinutes, uint16_t endMinutes)
{
  int current = currentHour * 60 + currentMinute;
  startMinutes %= 1440;
  endMinutes %= 1440;

  if (endMinutes < startMinutes)
  {
    return (current >= startMinutes) || (current < endMinutes);
  }

  return (current >= startMinutes) && (current < endMinutes);
}

  
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/

// // Форматирование времени в минутах или секундах
// String formatTime(unsigned long remainingMillis) {
//   unsigned long seconds = remainingMillis / 1000;
//   unsigned long minutes = seconds / 60;
//   seconds = seconds % 60;

//   if (minutes > 0) {
//     return String(minutes) + " m.";
//   } else {
//     return String(seconds) + " s.";
//   }
// }


// void manageTimer(int& mode,                 // Входной параметр: режим работы таймера (1,2,3,4,5,6,7)
//                  bool& power,               // Входной/выходной параметр: флаг, указывающий, включен ли таймер (true - включен, false - выключен)
//                  bool activation, //bool& activation,          // Входной параметр: флаг, указывающий, активирован ли таймер (true - активирован, false - неактивирован)
//                  unsigned long& lastMillis, // Входной/выходной параметр: переменная, содержащая последнее значение времени для таймера
//                  char* info) {              // Выходной параметр: массив символов для хранения информации о таймере
 
 

//   if (!activation || !Power_Filtr) {
//     power = false;  // Если активация таймера отключена, устанавливаем power в false и завершаем функцию
//     snprintf(info, 50, "OFF");  // Записываем информацию "OFF" в массив info
//     lastMillis = millis();  // Сбрасываем таймер
//     //lastMillis = currentMillis; //Обновляем временной таймер - сбраываем
//     return;
//   }

//   unsigned long timerDuration;
//   unsigned long timerWorkDuration = 5000;  // Время работы таймера всегда 5 секунд

//   // Определяем длительность таймера в зависимости от режима
//   switch (mode) {
//     case 1:
//       timerDuration = 1000 * 15; // 15 сек
//       break;
//     case 2:
//       timerDuration = 1000 * 60; // 60 сек
//       break;
//     case 3:
//       timerDuration = 1000 * 60 * 5; // 5 минут
//       break;
//     case 4:
//       timerDuration = 1000 * 60 * 15; // 10 минут
//       break;
//     case 5:
//       timerDuration = 1000 * 60 * 30; // 30 минут
//       break;
//     case 6:
//       timerDuration = 1000 * 60 * 60; // 1 час
//       break;
//     case 7:
//       timerDuration = 1000 * 60 * 60 * 24; // 24 часа
//       break;
//     default:
//       timerDuration = 0;
//       break;
//   }
void manageTimer(int& mode,                 // Входной параметр: режим работы таймера (1,2,3,4,5,6,7)
                 bool& power,               // Входной/выходной параметр: флаг, указывающий, включен ли таймер (true - включен, false - выключен)
                 bool activation,           // Входной параметр: флаг, указывающий, активирован ли таймер (true - активирован, false - неактивирован)
                 unsigned long& lastMillis, // Входной/выходной параметр: переменная, содержащая последнее значение времени для таймера
                 char* info) {              // Выходной параметр: массив символов для хранения информации о таймере
  if (!activation || !Power_Filtr) {
    power = false;
    snprintf(info, 50, "OFF");
    lastMillis = millis();
    return;
  }

//     unsigned long currentMillis = millis();  // Получаем текущее время в миллисекундах
//     unsigned long elapsedMillis = currentMillis - lastMillis;  // Вычисляем прошедшее время с момента последнего обновления
unsigned long timerDuration;
  const unsigned long timerWorkDuration = 5000;  // Время работы таймера всегда 5 секунд

  switch (mode) {
    case 1: timerDuration = 1000UL * 15; break;             // 15 сек
    case 2: timerDuration = 1000UL * 60; break;             // 60 сек
    case 3: timerDuration = 1000UL * 60 * 5; break;         // 5 минут
    case 4: timerDuration = 1000UL * 60 * 15; break;        // 15 минут
    case 5: timerDuration = 1000UL * 60 * 30; break;        // 30 минут
    case 6: timerDuration = 1000UL * 60 * 60; break;        // 1 час
    case 8: timerDuration = 1000UL * 60 * 60 * 2; break;    // 2 часа
    case 9: timerDuration = 1000UL * 60 * 60 * 3; break;    // 3 часа
    case 10: timerDuration = 1000UL * 60 * 60 * 4; break;   // 4 часа
    case 11: timerDuration = 1000UL * 60 * 60 * 6; break;   // 6 часов
    case 12: timerDuration = 1000UL * 60 * 60 * 8; break;   // 8 часов
    case 13: timerDuration = 1000UL * 60 * 60 * 12; break;  // 12 часов
    case 7: timerDuration = 1000UL * 60 * 60 * 24; break;   // 24 часа
    default: timerDuration = 0; break;
  }
//   if (!power) {
//     // Режим ожидания (таймер не работает)
//     if (elapsedMillis >= timerDuration) {
//       power = true;  // Если прошло достаточно времени, включаем таймер
//       lastMillis = currentMillis;  // Обновляем последнее время
//     }
//   } else {
//     // Режим работы таймера
//     if (elapsedMillis >= timerWorkDuration) {
//       power = false;  // Если прошло достаточно времени работы, выключаем таймер
//       lastMillis = currentMillis;  // Обновляем последнее время
//     }
//   }
  const unsigned long currentMillis = millis();
  const unsigned long elapsedMillis = currentMillis - lastMillis;

  if (!power) {
    if (elapsedMillis >= timerDuration) {
      power = true;
      lastMillis = currentMillis;
    }
  } else {
    if (elapsedMillis >= timerWorkDuration) {
      power = false;
      lastMillis = currentMillis;
    }
  }
//   // Формируем информацию о таймере для последующей передачи в Nextion или вывода в порт
//   if (power && activation) {
//     // Если таймер включен и активирован, вычисляем оставшееся время работы
//     unsigned long remainingMillis = timerWorkDuration - elapsedMillis;
//     //snprintf(info, 44, "Work: %s", /*formatTime(remainingMillis).c_str()*/ String(timerWorkDuration/1000) + " sec");
//     snprintf(info, 50, "Work");
    
//   } else if (!power && activation) {
//     // Если таймер выключен и активирован, вычисляем оставшееся время до старта
//     unsigned long remainingMillis = timerDuration - elapsedMillis;
//     snprintf(info, 44, "Start: %s", formatTime(remainingMillis).c_str());
//   }
    
// }
  if (power && activation) {
    snprintf(info, 50, "Work");
  } else if (!power && activation) {
    snprintf(info, 50, "Start");
  }
}

// Включение перельстатических насосов в ручную кнопкой на 1 сек для проверки
inline void updateManualPumpPulses(){
  const unsigned long now = millis();
  if(ManualPulse_ACO_Active && (now - ManualPulse_ACO_StartedAt >= 1000UL)){
    ManualPulse_ACO_Active = false;
  }
  if(ManualPulse_H2O2_Active && (now - ManualPulse_H2O2_StartedAt >= 1000UL)){
    ManualPulse_H2O2_Active = false;
  }
}
/**************************************************************************************/
/**************************************************************************************/
/**************************************************************************************/









void TimerControlRelay(int interval) {
  static unsigned long timer;
  if (interval + timer > millis()) return; 
  timer = millis();
  int currentHour = 0;
  int currentMinute = 0;
  if (!getCurrentHourMinute(currentHour, currentMinute)) {
    return;
  }
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------

                // //Проверяем, находится ли текущее время в диапазоне времени включения/выключения
                //   if (checkTimeInInterval(currentHour, currentMinute, Lamp_timeON1, Lamp_timeOFF1)&&Power_Time1==true) {
                //     Lamp=true;
                //   } else if (Power_Time1==true) {Lamp=false; } // Выключаем
            //Проверяем режим освещения и применяем логику включения лампы
                UITimerEntry &lampTimer = ui.timer("LampTimer");
                UITimerEntry &rgbTimer = ui.timer("RgbTimer");
                UITimerEntry &filtrTimer1 = ui.timer("FiltrTimer1");
                UITimerEntry &filtrTimer2 = ui.timer("FiltrTimer2");
                UITimerEntry &filtrTimer3 = ui.timer("FiltrTimer3");
                UITimerEntry &cleanTimer = ui.timer("CleanTimer1");
                UITimerEntry &ulLightTimer = ui.timer("UlLightTimer");

                 if (SetLamp == "off") {
                  Lamp = false;
                } else if (SetLamp == "on") {
                  Lamp = true;
                } else if (SetLamp == "timer") {
                  Lamp = checkTimeInInterval(currentHour, currentMinute, lampTimer.on, lampTimer.off);
                } else if (SetLamp == "auto") {
                  Lamp = Lumen_Ul < 20;
                } else {
                  // Резервная логика для совместимости со старым управлением
                  if (Power_Time1) {
                    Lamp = checkTimeInInterval(currentHour, currentMinute, lampTimer.on, lampTimer.off);
                  } else if (Lamp_autosvet) {
                    Lamp = Lumen_Ul < 20;
                  }
                }    
   
                    if (checkTimeInInterval(currentHour, currentMinute, rgbTimer.on, rgbTimer.off)&&WS2815_Time1==true) {
                      Pow_WS2815=true;
                    } else if (WS2815_Time1==true) {Pow_WS2815=false;} // Выключаем
              
                    if (Ul_light_Time) {
                      Pow_Ul_light = checkTimeInInterval(currentHour, currentMinute, ulLightTimer.on, ulLightTimer.off);
                    }        
    
//   if (checkTimeInInterval(hours, minutes, Filtr_timeON1, Filtr_timeOFF1)&&Filtr_Time1==true || checkTimeInInterval(hours, minutes, Filtr_timeON2, Filtr_timeOFF2)&&Filtr_Time2==true || checkTimeInInterval(hours, minutes, Filtr_timeON3, Filtr_timeOFF3)&&Filtr_Time3==true) {
//      if(!Power_Clean){Power_Filtr=true;} //преимущество очистки - отключаем фильтрацию в любом случае (даже если включен по таймерам), если подошло время очистки фильтра
//     } else if (Filtr_Time1==true || Filtr_Time2==true || Filtr_Time3==true) {
//       Power_Filtr=false;// Выключаем
//     }
  
//     //Проверяем, находится ли текущее время в диапазоне времени включения/выключения освещения
//     if (checkTimeInInterval(hours, minutes, Ul_light_timeON, Ul_light_timeOFF)&&Ul_light_Time==true) {
//       Pow_Ul_light=true;
//     } else if (Ul_light_Time==true) {Pow_Ul_light=false; } // Выключаем
    bool anyFiltrTimer = Filtr_Time1 || Filtr_Time2 || Filtr_Time3;
    bool filtrActive = (Filtr_Time1 && checkTimeInInterval(currentHour, currentMinute, filtrTimer1.on, filtrTimer1.off))
                    || (Filtr_Time2 && checkTimeInInterval(currentHour, currentMinute, filtrTimer2.on, filtrTimer2.off))
                    || (Filtr_Time3 && checkTimeInInterval(currentHour, currentMinute, filtrTimer3.on, filtrTimer3.off));
//     //текущий день недели "DayOfWeek" - будет указывать в массиве "chk_Array[DayOfWeek - 1]"" на "chk1...chk7" который имее значение "true" или "false"
//     bool chk_Array[] = {chk1, chk2, chk3, chk4, chk5, chk6, chk7}; 
    if (anyFiltrTimer) {
      Power_Filtr = filtrActive;
    }
//     if (checkTimeInInterval(hours, minutes, Clean_timeON1, Clean_timeOFF1) && Clean_Time1 == true && chk_Array[DayOfWeek - 1]) {
//         Power_Clean = true; // Включаем промывку
//     } else if (Clean_Time1 == true) {
//         Power_Clean = false; // Выключаем
//     }
    bool chk_Array[] = {chk1, chk2, chk3, chk4, chk5, chk6, chk7};
    bool cleanDayEnabled = (DayOfWeek >= 1 && DayOfWeek <= 7) ? chk_Array[DayOfWeek - 1] : false;
    if (Clean_Time1) {
      Power_Clean = cleanDayEnabled && checkTimeInInterval(currentHour, currentMinute, cleanTimer.on, cleanTimer.off);
    }

    if (Power_Clean) {
      Power_Filtr = false;
    }





//   // // Дозация ACO (кислоты) по pH: только если включен контроль и pH >= 7.3
//   // static unsigned long lastMillisACO = 0;  // переменная хранения времени последней дозировки ACO
//   // bool needDosePH = PH_Control_ACO && (PH >= 7.3);// Условие запуска дозатора: включен контроль + значение pH выше допустимого
//   // manageTimer(ACO_Work, Power_ACO, needDosePH, lastMillisACO, Info_ACO);

//   // // Дозация NaOCl (хлора) по ORP: только если включен контроль и ORP < 600
//   // static unsigned long lastMillisH2O2 = 0;
//   // bool needDoseORP = NaOCl_H2O2_Control && (corrected_ORP_Eh_mV < 600);
//   // manageTimer(H2O2_Work, Power_H2O2, needDoseORP, lastMillisH2O2, Info_H2O2);

// //   // Дозация ACO (кислоты)
// // static unsigned long lastMillisACO = 0;
// // if (PH_Control_ACO && PH >= 7.3) {
// //   bool activationPH = true;
// //   manageTimer(ACO_Work, Power_ACO, activationPH, lastMillisACO, Info_ACO);
// // } else {
// //   // Если не требуется дозация, принудительно сбрасываем таймер
// //   Power_ACO = false;
// //   lastMillisACO = 0;
// //   strcpy(Info_ACO, "OFF"); // strcpy(куда_копировать, что_копировать);
// // }

// // // Дозация NaOCl (хлора)
// // static unsigned long lastMillisH2O2 = 0;
// // if (NaOCl_H2O2_Control && corrected_ORP_Eh_mV < 600) {
// // bool activationORP = true;
// // manageTimer(H2O2_Work, Power_H2O2, activationORP, lastMillisH2O2, Info_H2O2);
// // } else {
// //   Power_H2O2 = false;
// //   lastMillisH2O2 = 0;
// //   strcpy(Info_H2O2, "OFF");
// // }



// // static unsigned long lastMillisACO = 0;            // Время последнего включения таймера дозации ACO (для manageTimer)
// // static bool phDosingActive = false;                // Флаг активной фазы дозации pH, сохраняется между циклами

// // if (PH_Control_ACO) {                              // Если включён контроль по pH (активирована дозация)
  
// //   if (!phDosingActive && PH > PH_Upper) {          // Если дозация ещё не активна и pH превышает верхний предел
// //     phDosingActive = true;                         // Включаем фазу дозации
// //   } else if (phDosingActive && PH <= PH_Lower) {   // Если дозация активна и pH опустился до нижнего предела
// //     phDosingActive = false;                        // Завершаем фазу дозации
// //   }

// //   if (phDosingActive && Power_Filtr) {             // Если фаза дозации активна И фильтрация включена
// //     bool activationPH = true;                      // Временная переменная для передачи по ссылке в manageTimer
// //     manageTimer(ACO_Work, Power_ACO, activationPH, lastMillisACO, Info_ACO);  // Управляем таймером дозации кислоты
// //   } else {                                         // Если фаза дозации завершена или фильтрация выключена
// //     Power_ACO = false;                             // Отключаем дозацию ACO (реле)
// //     lastMillisACO = 0;                             // Сбрасываем время последней активации
// //     strcpy(Info_ACO, "OFF");                       // Устанавливаем текстовое состояние таймера для дисплея/лога
// //   }

// // } else {                                           // Если контроль pH выключен
// //   phDosingActive = false;                          // Сбрасываем флаг дозации
// //   Power_ACO = false;                               // Отключаем реле
// //   lastMillisACO = 0;                               // Сбрасываем таймер
// //   strcpy(Info_ACO, "OFF");                         // Устанавливаем текст "OFF" для отображения
// // }


// // // ===== Дозация NaOCl (хлора) =====

// // static unsigned long lastMillisH2O2 = 0;           // Время последнего включения таймера дозации хлора
// // static bool orpDosingActive = false;               // Флаг активной фазы дозации по ORP (хлору)

// // if (NaOCl_H2O2_Control) {                          // Если включён контроль хлора по ORP
  
// //   if (!orpDosingActive && corrected_ORP_Eh_mV < ORP_Lower) {   // Если дозация ещё не активна и ORP ниже порога
// //     orpDosingActive = true;                        // Активируем фазу дозации
// //   } else if (orpDosingActive && corrected_ORP_Eh_mV >= ORP_Upper) {  // Если ORP поднялся до верхнего порога
// //     orpDosingActive = false;                       // Завершаем фазу дозации
// //   }

// //   if (orpDosingActive && Power_Filtr) {            // Если дозация активна и включена фильтрация
// //     bool activationORP = true;                     // Временная переменная для передачи по ссылке
// //     manageTimer(H2O2_Work, Power_H2O2, activationORP, lastMillisH2O2, Info_H2O2); // Запускаем/обслуживаем таймер дозации хлора
// //   } else {                                         // Если дозация неактивна или фильтрации нет
// //     Power_H2O2 = false;                            // Отключаем дозирующий насос
// //     lastMillisH2O2 = 0;                            // Сбрасываем таймер
// //     strcpy(Info_H2O2, "OFF");                      // Пишем "OFF" для отображения
// //   }

// // } else {                                           // Если контроль по ORP выключен
// //   orpDosingActive = false;                         // Сбрасываем флаг
// //   Power_H2O2 = false;                              // Отключаем дозирующее реле
// //   lastMillisH2O2 = 0;                              // Обнуляем таймер
// //   strcpy(Info_H2O2, "OFF");                        // Обновляем текстовое состояние
// // }

        // // Активация дозации ACO - кислоты по датчику PH
        //     static unsigned long lastMillisACO = 0;
        //     if (PH > PH_setting  && PH_Control_ACO && Power_Filtr) {
        //       manageTimer(ACO_Work, Power_ACO, PH_Control_ACO, lastMillisACO, Info_ACO);
        //         //Activation_Timer_ACO = true;
        //     } else if (PH <= PH_setting || !PH_Control_ACO || !Power_Filtr) {
        //       manageTimer(ACO_Work, Power_ACO=false, false, lastMillisACO, Info_ACO);
        //     }
          
        //     // Активация дозации NaOCl по датчику хлора
        //     // Условие запуска таймера дозации хлора:
        //     // Если ORP ниже нижнего предела (ORP_Lower), pH в норме,
        //     // контроль включён, фильтрация активна
        //     static unsigned long lastMillisH2O2 = 0;
        //     if(corrected_ORP_Eh_mV < ORP_setting && PH <= PH_setting + 0.1 && NaOCl_H2O2_Control && Power_Filtr) {
        //       manageTimer(H2O2_Work, Power_H2O2, NaOCl_H2O2_Control, lastMillisH2O2, Info_H2O2);
        //     } else if (corrected_ORP_Eh_mV > ORP_setting || PH > PH_setting  || !NaOCl_H2O2_Control || !Power_Filtr){
        //       manageTimer(H2O2_Work, Power_H2O2 = false, false, lastMillisH2O2, Info_H2O2);
        //     }
    // Активация дозации ACO - кислоты по датчику PH
    static unsigned long lastMillisACO = 0;
    if (PH > PH_setting && PH_Control_ACO && Power_Filtr) {
      manageTimer(ACO_Work, Power_ACO, PH_Control_ACO, lastMillisACO, Info_ACO);
    } else {
      manageTimer(ACO_Work, Power_ACO = false, false, lastMillisACO, Info_ACO);
    }

    // Активация дозации NaOCl по датчику хлора
    static unsigned long lastMillisH2O2 = 0;
    if (corrected_ORP_Eh_mV < ORP_setting && PH <= PH_setting + 0.1 && NaOCl_H2O2_Control && Power_Filtr) {
      manageTimer(H2O2_Work, Power_H2O2, NaOCl_H2O2_Control, lastMillisH2O2, Info_H2O2);
    } else {
      manageTimer(H2O2_Work, Power_H2O2 = false, false, lastMillisH2O2, Info_H2O2);
    }

// } // End TimerControlRelay






// void ControlModbusRelay(int interval) {
//   static unsigned long timer;
//   if (interval + timer > millis()) return; 
//   timer = millis();
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
// //---------------------------------------------------------------------------------
} // End TimerControlRelay

//   // Error err = RS485.addRequest(40001,1,0x05,0, Lamp ? devices[0].value : devices[1].value); // реле№1 для Lamp -  RS485.addRequest(Tokin, адр.модуля, функция, адр.реле, вкл./выкл);

//   // err = RS485.addRequest(40001,1,0x05,1, Pow_WS2815 ? devices[0].value : devices[1].value); //реле№2 для Pow_WS2815

//   // err = RS485.addRequest(40001,1,0x05,8, Power_Filtr ? devices[0].value : devices[1].value); //реле№3 для Power_Filtr

//   // err = RS485.addRequest(40001,1,0x05,3, Power_Clean ? devices[0].value : devices[1].value); //реле№4 для Power_Clean

//   // err = RS485.addRequest(40001,1,0x05,4, PowerHeat ? devices[0].value : devices[1].value); //реле№5 для PowerHeat

//   if (!Activation_Heat) {Error err = RS485.addRequest(40001,1,0x05,4, devices[1].value );} //реле№5 для PowerHeat
void ControlModbusRelay(int interval) {
  static unsigned long timer;
  if (interval + timer > millis()) return;
  timer = millis();
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

  // if (RoomTemper && DS1 < RoomTempOn) {
  //   Power_Warm_floor_heating = true;
  // } else if (RoomTemper && DS1 > RoomTempOff) {
  //   Power_Warm_floor_heating = false;
  // } else if (!RoomTemper) {
  //   Power_Warm_floor_heating = false;
  // }

const bool roomTempInRange = DS1 >= RoomTempOn && DS1 <= RoomTempOff; // true, если текущая температура DS1 находится в зоне гистерезиса между порогом включения и порогом выключения

if (RoomTemper && !roomTempInRange && DS1 < RoomTempOn) {             // если режим температурного контроля включён, температура вышла из допустимого диапазона и опустилась ниже порога включения
    // здесь должно происходить включение тёплого пола (условие "холодно, нужно греть")
} else {
    Power_Warm_floor_heating = false;                                 // во всех остальных случаях подогрев принудительно выключается, чтобы исключить перегрев и ложные включения
}


if (AktualReadInput) {
    WaterLevelSensorUpper = ReadInputArray[0]; // Датчик верхнего уровня (вход №1)
    WaterLevelSensorLower = ReadInputArray[1]; // Датчик нижнего уровня (вход №2)
  }

  if (AktualReadRelay) {
    Power_Topping_State = ReadRelayArray[10]; // Состояние реле №11 (соленоид долива воды)
  } else {
    Power_Topping_State = Power_Topping;
  }

  if (Activation_Water_Level) {
    if (!WaterLevelSensorLower) {
      Power_Topping = true;
    }
    if (WaterLevelSensorUpper) {
      Power_Topping = false;
    }
  }


  Error err = RS485.addRequest(40001, 1, 0x05, 0, Lamp ? devices[0].value : devices[1].value); // реле№1 для Lamp
  err = RS485.addRequest(40001, 1, 0x05, 1, Pow_WS2815 ? devices[0].value : devices[1].value); //реле№2 для Pow_WS2815
  err = RS485.addRequest(40001, 1, 0x05, 8, Power_Filtr ? devices[0].value : devices[1].value); //реле№3 для Power_Filtr
  err = RS485.addRequest(40001, 1, 0x05, 3, Power_Clean ? devices[0].value : devices[1].value); //реле№4 для Power_Clean

  if (!Activation_Heat) {
    err = RS485.addRequest(40001, 1, 0x05, 4, devices[1].value); //реле№5 для PowerHeat
  } else {
    err = RS485.addRequest(40001, 1, 0x05, 4, Power_Heat ? devices[0].value : devices[1].value);
  }

  err = RS485.addRequest(40001, 1, 0x05, 14, Power_Warm_floor_heating ? devices[0].value : devices[1].value); // реле№15 теплый пол
  err = RS485.addRequest(40001, 1, 0x05, 15, Pow_Ul_light ? devices[0].value : devices[1].value); //Уличное освещение на столбе
  err = RS485.addRequest(40001, 1, 0x05, 10, Power_Topping ? devices[0].value : devices[1].value); // реле№11 соленоид долива воды
//   Error err = RS485.addRequest(40001,1,0x05,5, Power_H2O2 ? devices[0].value : devices[1].value); //реле№6 для Power_H2O2
  // err = RS485.addRequest(40001, 1, 0x05, 5, Power_H2O2 ? devices[0].value : devices[1].value); //реле№6 для Power_H2O2
  // err = RS485.addRequest(40001, 1, 0x05, 6, Power_ACO ? devices[0].value : devices[1].value); //реле№7 для Power_ACO
  const bool powerH2O2Active = Power_H2O2 || ManualPulse_H2O2_Active; //Переменная powerH2O2Active будет равна true, если хотя бы одно из условий -  true
  const bool powerAcoActive = Power_ACO || ManualPulse_ACO_Active;
  err = RS485.addRequest(40001, 1, 0x05, 5, powerH2O2Active ? devices[0].value : devices[1].value); //реле№6 для Power_H2O2
  err = RS485.addRequest(40001, 1, 0x05, 6, powerAcoActive ? devices[0].value : devices[1].value); //реле№7 для Power_ACO
//   err = RS485.addRequest(40001,1,0x05,6, Power_ACO ? devices[0].value : devices[1].value);//реле№7 для Power_ACO
    
   
//   err = RS485.addRequest(40050, 1, 0x03, 0, 1); // Чтение всех реле в бинарный массив ReadRelayArray[16] - читать при совпадении token ==40050
//   err = RS485.addRequest(40060, 1, 0x03, 4, 1); // Чтение всех входов в бинарный массив ReadInputArray[16] - читать при совпадении token ==40060
  
//   AktualReadRelay=false; // перед прочтением говорим всем что данные в массивах не актуальны пока
//   AktualReadInput=false;
  err = RS485.addRequest(40050, 1, 0x03, 0, 1); // Чтение всех реле в бинарный массив ReadRelayArray[16]
  err = RS485.addRequest(40060, 1, 0x03, 4, 1); // Чтение всех входов в бинарный массив ReadInputArray[16]
//   // Если активирован автосвет - то свет включаем и отключаем по освещенности на улице
//   if(Lamp_autosvet && Lumen_Ul_percent < 20){Lamp=true;}  else if (Lamp_autosvet && Lumen_Ul_percent > 30) {Lamp=false;} 
  AktualReadRelay = false;
  AktualReadInput = false;
}
//   err = RS485.addRequest(40001,1,0x05,15, Pow_Ul_light ? devices[0].value : devices[1].value); //Уличное освещение на столбе
 
// }
