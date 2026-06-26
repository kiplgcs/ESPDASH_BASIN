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
#define RXD1 6  // 6 //8/15/16
#define TXD1 7  //7 //9//14/17

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

constexpr unsigned long NEXTION_IDLE_DIM_MS = 5UL * 60UL * 1000UL; // Через 5 минут простоя приглушаем экран Nextion.
constexpr uint8_t NEXTION_DIM_ACTIVE = 45; // Рабочая яркость Nextion после касания.
constexpr uint8_t NEXTION_DIM_IDLE = 3; // Яркость Nextion при простое.
unsigned long NextionLastActivityMs = 0; // Время последнего события от сенсорного экрана Nextion.
bool NextionScreenDimmed = false; // Флаг нужен, чтобы не слать dim=3 постоянно.

constexpr uint8_t NX_ASYNC_GROUP_NONE = 0;
constexpr uint8_t NX_ASYNC_GROUP_DISPENSERS = 1;
constexpr uint8_t NX_ASYNC_GROUP_FILTR_SWITCHES = 2;

struct NextionAsyncNumberRead {
  bool active = false;
  uint8_t group = NX_ASYNC_GROUP_NONE;
  uint8_t target = 255;
  uint8_t state = 0;
  uint8_t dataIndex = 0;
  uint8_t ffCount = 0;
  uint32_t value = 0;
  unsigned long sentAt = 0;
};

NextionAsyncNumberRead NxAsyncNumberReadState;
uint32_t NxDispensersRawValues[8] = {0, 0, 0, 0, 0, 0, 0, 0};
bool NxDispensersRawValid[8] = {false, false, false, false, false, false, false, false};
unsigned long NxLastDispensersAsyncPollMs = 0;
uint8_t NxDispensersAsyncPollIndex = 0;
unsigned long NxFiltrSwitchPollRequestUntil = 0;
unsigned long NxFiltrWriteHoldUntil = 0;
unsigned long NxLastFiltrSwitchPollMs = 0;
uint8_t NxFiltrSwitchPollIndex = 0;

inline bool nextionAsyncReadActive() {
  return NxAsyncNumberReadState.active;
}

inline void markNextionActivity() { // Любой trigger/page от Nextion считаем касанием или активностью пользователя.
  NextionLastActivityMs = millis(); // Обновляем момент последней активности без блокирующих чтений dim/dp.
  if (NextionScreenDimmed) { // Если экран уже был приглушен, сразу возвращаем рабочую яркость.
    myNex.writeStr("dim=" + String(NEXTION_DIM_ACTIVE)); // Возвращаем яркость Nextion до 45%.
    Nx_dim_id = NEXTION_DIM_ACTIVE; // Держим локальное состояние яркости синхронным с отправленной командой.
    NextionScreenDimmed = false; // Повторно dim=45 не отправляем, пока экран снова не приглушится.
  }
}

inline void serviceNextionIdleBrightness() { // Неблокирующий контроль затемнения Nextion при простое.
  if (NextionLastActivityMs == 0) NextionLastActivityMs = millis(); // Первый запуск считаем активностью после старта ESP32.
  if (NxAsyncNumberReadState.active || MySerial.available() > 0) return; // Не вмешиваем dim-команду в активный обмен UART.
  if (!NextionScreenDimmed && static_cast<unsigned long>(millis() - NextionLastActivityMs) >= NEXTION_IDLE_DIM_MS) {
    myNex.writeStr("dim=" + String(NEXTION_DIM_IDLE)); // Через 5 минут без событий опускаем яркость до 3%.
    Nx_dim_id = NEXTION_DIM_IDLE; // Запоминаем отправленную яркость локально.
    NextionScreenDimmed = true; // До следующего касания больше не шлем dim=3.
  }
}

inline bool consumeNextionNativeTouchEvent() { // Обрабатываем штатные touch-пакеты Nextion, если они включены в HMI/sendxy.
  if (!MySerial.available()) return false; // Нет байтов для разбора.
  const uint8_t firstByte = static_cast<uint8_t>(MySerial.peek()); // Смотрим тип пакета, не удаляя его из буфера.
  uint8_t packetLength = 0; // Длина известного штатного пакета Nextion.
  if (firstByte == 0x65) packetLength = 7; // Touch Event: 0x65 page component event FF FF FF.
  else if (firstByte == 0x67 || firstByte == 0x68) packetLength = 9; // Touch Coordinate: 0x67/0x68 xH xL yH yL event FF FF FF.
  else return false; // Остальные байты обрабатываются обычным протоколом или отбрасываются ниже.
  if (MySerial.available() < packetLength) return false; // Ждем полный пакет, чтобы не съесть половину события.
  for (uint8_t i = 0; i < packetLength; ++i) MySerial.read(); // Удаляем штатный touch-пакет из RX-буфера.
  markNextionActivity(); // Любой штатный touch-пакет возвращает яркость и сбрасывает таймер простоя.
  return true;
}

inline void resetNextionAsyncNumberRead() {
  NxAsyncNumberReadState.active = false;
  NxAsyncNumberReadState.group = NX_ASYNC_GROUP_NONE;
  NxAsyncNumberReadState.target = 255;
  NxAsyncNumberReadState.state = 0;
  NxAsyncNumberReadState.dataIndex = 0;
  NxAsyncNumberReadState.ffCount = 0;
  NxAsyncNumberReadState.value = 0;
  NxAsyncNumberReadState.sentAt = 0;
}

inline void holdNextionFiltrWrites(unsigned long holdMs = 1500) {
  NxFiltrWriteHoldUntil = millis() + holdMs;
}

inline bool nextionFiltrWriteHoldActive() {
  return NxFiltrWriteHoldUntil != 0 &&
         static_cast<long>(millis() - NxFiltrWriteHoldUntil) < 0;
}

inline void requestNextionFiltrSwitchPoll(unsigned long pollMs = 1800) {
  NxFiltrSwitchPollRequestUntil = millis() + pollMs;
  NxFiltrSwitchPollIndex = 0;
}

inline bool nextionFiltrSwitchPollRequested() {
  return NxFiltrSwitchPollRequestUntil != 0 &&
         static_cast<long>(millis() - NxFiltrSwitchPollRequestUntil) < 0;
}

inline float nextionComposeTenths(uint8_t wholeIndex, uint8_t tenthIndex, float fallback) {
  if (!NxDispensersRawValid[wholeIndex] || !NxDispensersRawValid[tenthIndex]) return fallback;
  int whole = static_cast<int>(NxDispensersRawValues[wholeIndex]);
  int tenth = static_cast<int>(NxDispensersRawValues[tenthIndex]);
  if (tenth < 0) tenth = 0;
  if (tenth > 9) tenth = 9;
  return static_cast<float>(whole) + static_cast<float>(tenth) / 10.0f;
}

inline bool nextionFloatChangedLocal(float oldValue, float newValue) {
  float delta = oldValue > newValue ? oldValue - newValue : newValue - oldValue;
  return delta >= 0.05f;
}

inline void applyNextionDispensersAsyncValue(uint8_t target, uint32_t value) {
  if (target >= 8) return;
  const float oldPHLower = PH_Lower;
  const float oldPHUpper = PH_Upper;
  const float oldCLLower = CL_Lower;
  const float oldCLUpper = CL_Upper;
  NxDispensersRawValues[target] = value;
  NxDispensersRawValid[target] = true;

  bool changed = false;
  float nextPHLower = nextionComposeTenths(0, 1, PH_Lower);
  float nextPHUpper = nextionComposeTenths(2, 3, PH_Upper);
  float nextCLLower = nextionComposeTenths(4, 5, CL_Lower);
  float nextCLUpper = nextionComposeTenths(6, 7, CL_Upper);

  if ((target == 0 || target == 1) && nextionFloatChangedLocal(PH_Lower, nextPHLower)) {
    PH_Lower = nextPHLower;
    changed = true;
  }
  if ((target == 2 || target == 3) && nextionFloatChangedLocal(PH_Upper, nextPHUpper)) {
    PH_Upper = nextPHUpper;
    changed = true;
  }
  if ((target == 4 || target == 5) && nextionFloatChangedLocal(CL_Lower, nextCLLower)) {
    CL_Lower = nextCLLower;
    changed = true;
  }
  if ((target == 6 || target == 7) && nextionFloatChangedLocal(CL_Upper, nextCLUpper)) {
    CL_Upper = nextCLUpper;
    changed = true;
  }

  if (changed) {
    normalizeChemicalLimits();
    persistChangedChemicalLimits(oldPHLower, oldPHUpper, oldCLLower, oldCLUpper);
    holdNextionDispensersWrites(4000);
  }
}

inline void applyNextionFiltrSwitchAsyncValue(uint8_t target, uint32_t value) {
  const bool nextValue = value != 0;
  if (target == 0) {
    const bool changed = Filtr_Time1 != nextValue;
    Filtr_Time1 = nextValue;
    Saved_Filtr_Time1 = nextValue;
    if (changed) saveValue<int>("Filtr_Time1", Filtr_Time1 ? 1 : 0);
  } else if (target == 1) {
    const bool changed = Filtr_Time2 != nextValue;
    Filtr_Time2 = nextValue;
    Saved_Filtr_Time2 = nextValue;
    if (changed) saveValue<int>("Filtr_Time2", Filtr_Time2 ? 1 : 0);
  } else if (target == 2) {
    const bool changed = Filtr_Time3 != nextValue;
    Filtr_Time3 = nextValue;
    Saved_Filtr_Time3 = nextValue;
    if (changed) saveValue<int>("Filtr_Time3", Filtr_Time3 ? 1 : 0);
  } else if (target == 3) {
    const bool changed = Power_Filtr != nextValue;
    Power_Filtr = nextValue;
    Power_Filtr1 = nextValue;
    if (changed) saveValue<int>("Power_Filtr", Power_Filtr ? 1 : 0);
  }
}

inline void beginNextionAsyncNumberRead(const char* component, uint8_t target, uint8_t group = NX_ASYNC_GROUP_DISPENSERS) {
  if (NxAsyncNumberReadState.active || MySerial.available() > 0) return;
  NxAsyncNumberReadState.active = true;
  NxAsyncNumberReadState.group = group;
  NxAsyncNumberReadState.target = target;
  NxAsyncNumberReadState.state = 0;
  NxAsyncNumberReadState.dataIndex = 0;
  NxAsyncNumberReadState.ffCount = 0;
  NxAsyncNumberReadState.value = 0;
  NxAsyncNumberReadState.sentAt = millis();
  MySerial.print("get ");
  MySerial.print(component);
  MySerial.print("\xFF\xFF\xFF");
}

inline bool processNextionAsyncNumberByte() {
  if (!NxAsyncNumberReadState.active || !MySerial.available()) return false;
  if (static_cast<uint8_t>(MySerial.peek()) == '#') return false;
  uint8_t byteValue = static_cast<uint8_t>(MySerial.read());

  if (NxAsyncNumberReadState.state == 0) {
    if (byteValue == 0x71) {
      NxAsyncNumberReadState.state = 1;
      NxAsyncNumberReadState.dataIndex = 0;
      NxAsyncNumberReadState.value = 0;
    }
    return true;
  }

  if (NxAsyncNumberReadState.state == 1) {
    NxAsyncNumberReadState.value |= static_cast<uint32_t>(byteValue) << (8 * NxAsyncNumberReadState.dataIndex);
    NxAsyncNumberReadState.dataIndex++;
    if (NxAsyncNumberReadState.dataIndex >= 4) {
      NxAsyncNumberReadState.state = 2;
      NxAsyncNumberReadState.ffCount = 0;
    }
    return true;
  }

  if (NxAsyncNumberReadState.state == 2) {
    if (byteValue == 0xFF) {
      NxAsyncNumberReadState.ffCount++;
      if (NxAsyncNumberReadState.ffCount >= 3) {
        uint8_t group = NxAsyncNumberReadState.group;
        uint8_t target = NxAsyncNumberReadState.target;
        uint32_t readValue = NxAsyncNumberReadState.value;
        resetNextionAsyncNumberRead();
        if (group == NX_ASYNC_GROUP_DISPENSERS) {
          applyNextionDispensersAsyncValue(target, readValue);
        } else if (group == NX_ASYNC_GROUP_FILTR_SWITCHES) {
          applyNextionFiltrSwitchAsyncValue(target, readValue);
        }
      }
    } else {
      resetNextionAsyncNumberRead();
    }
    return true;
  }

  return true;
}

inline void serviceNextionSerial(uint8_t maxOperations = 24) {
  if (NxAsyncNumberReadState.active &&
      static_cast<unsigned long>(millis() - NxAsyncNumberReadState.sentAt) > 250UL) {
    resetNextionAsyncNumberRead();
  }

  for (uint8_t i = 0; i < maxOperations && MySerial.available(); ++i) {
    if (!NxAsyncNumberReadState.active) { // Штатные touch-пакеты разбираем только вне числового ответа get.
      const uint8_t firstByte = static_cast<uint8_t>(MySerial.peek()); // Проверяем, не начинается ли буфер с touch-события.
      if (firstByte == 0x65 || firstByte == 0x67 || firstByte == 0x68) {
        if (consumeNextionNativeTouchEvent()) continue; // Касания Nextion без printh тоже считаем активностью.
        break; // Если пакет еще не полный, не съедаем первый байт и ждем следующий проход.
      }
    }

    if (NxAsyncNumberReadState.active && static_cast<uint8_t>(MySerial.peek()) != '#') {
      processNextionAsyncNumberByte();
      continue;
    }

    if (static_cast<uint8_t>(MySerial.peek()) == '#') {
      if (MySerial.available() <= 2) break;
      if (NxAsyncNumberReadState.active) resetNextionAsyncNumberRead();
      myNex.NextionListen();
      markNextionActivity(); // После обработки события Nextion обновляем таймер простоя и возвращаем яркость при касании.
    } else {
      MySerial.read();
    }
  }

  if (myNex.currentPageId >= 0 && myNex.currentPageId < 50) {
    Nx_page_id = myNex.currentPageId;
  }
  serviceNextionIdleBrightness(); // Проверяем затемнение без чтения dim из Nextion.
}

inline void pollNextionDispensersSettingsAsync() {
  if (Nx_page_id != 9 || nextionDispensersReadHoldActive() || NxAsyncNumberReadState.active || MySerial.available() > 0) return;
  if (static_cast<unsigned long>(millis() - NxLastDispensersAsyncPollMs) < 250UL) return;
  static const char* components[] = {
    "Dispensers.n0.val", "Dispensers.n1.val",
    "Dispensers.n2.val", "Dispensers.n3.val",
    "Dispensers.n6.val", "Dispensers.n7.val",
    "Dispensers.n4.val", "Dispensers.n5.val"
  };
  NxLastDispensersAsyncPollMs = millis();
  beginNextionAsyncNumberRead(components[NxDispensersAsyncPollIndex], NxDispensersAsyncPollIndex, NX_ASYNC_GROUP_DISPENSERS);
  NxDispensersAsyncPollIndex = (NxDispensersAsyncPollIndex + 1) % 8;
}

inline void pollNextionFiltrSwitchesAsync() {
  const bool urgentPoll = nextionFiltrSwitchPollRequested();
  if ((Nx_page_id != 3 && !urgentPoll) || NxAsyncNumberReadState.active || MySerial.available() > 0) return;
  const unsigned long pollInterval = urgentPoll ? 120UL : 1000UL;
  if (static_cast<unsigned long>(millis() - NxLastFiltrSwitchPollMs) < pollInterval) return;
  static const char* components[] = {
    "set_filtr.sw0.val",
    "set_filtr.sw2.val",
    "set_filtr.sw1.val",
    "set_filtr.sw3.val"
  };
  NxLastFiltrSwitchPollMs = millis();
  beginNextionAsyncNumberRead(components[NxFiltrSwitchPollIndex], NxFiltrSwitchPollIndex, NX_ASYNC_GROUP_FILTR_SWITCHES);
  NxFiltrSwitchPollIndex = (NxFiltrSwitchPollIndex + 1) % 4;
}

// void ActivUARTInterrupt() {//Прерывание по Rx для получения данных от Nextion монитора - отключено потому что и так работает все хорошо.
//   myNex.NextionListen(); // Обработка данных при прерывании
// }

  /************************* инициализируем монитор Nextion*********************************/
void setup_Nextion(){
  myNex.currentPageId = 0;

  MySerial.begin(115200, SERIAL_8N1, RXD1, TXD1); // Инициализируем порт со своими пинами


  myNex.lastCurrentPageId = 1;  // При первом запуске цикла currentPageId и lastCurrentPageId
                                // должны иметь разные значения из-за запуска функции firstRefresh()
  myNex.writeStr("page 0");     // Для синхронизации страницы Nextion в случае сброса на Arduino
  myNex.writeStr("sendxy=1");   // Просим Nextion присылать координаты касаний, чтобы любое касание возвращало яркость экрана.
  myNex.writeStr("dim=" + String(NEXTION_DIM_ACTIVE)); // После старта держим экран Nextion на рабочей яркости 45%.
  Nx_dim_id = NEXTION_DIM_ACTIVE; // Синхронизируем локальную переменную яркости с отправленной командой.
  NextionLastActivityMs = millis(); // Старт ESP считаем активностью, чтобы затемнение началось только через 5 минут.
  NextionScreenDimmed = false; // После старта экран не считается приглушенным.
  //triggerRestartNextion = true; //Флаг чтения всех необходимых переменных из Nextion, после перезагрузки контроллера.

  //Прерываем по пину  RX порта для выполнения функции получения данных для монитора Nextion
  //attachInterrupt(digitalPinToInterrupt(RXD1), ActivUARTInterrupt, RISING); //CHANGE //FALLING //RISING); 
  /************************* инициализируем и получаем время*********************************/
  //setup_rtc(); - отключена, т.к. настройка серверов идет в основной функции запроса
}



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

    Power_Time1 = myNex.readNumber("set_lamp.sw0.val"); // Читаем таймер лампы без блокирующего delay.
    // jee.var("Power_Time1", Power_Time1 ? "true" : "false"); //jee.var("Power_Time1", String(Power_Time1));

    Lamp_autosvet = myNex.readNumber("set_lamp.sw1.val"); // Читаем авто-свет лампы без блокирующего delay.
    // jee.var("Lamp_autosvet", Lamp_autosvet ? "true" : "false"); //jee.var("Lamp_autosvet", String(Lamp_autosvet));


    bool powerChanged = Power_Time1 != Saved_Power_Time1;
    bool autoChanged = Lamp_autosvet != Saved_Lamp_autosvet;
    if (Power_Time1 && Lamp_autosvet) {
      if (powerChanged && !autoChanged) {
        Lamp_autosvet = false;
      } else if (autoChanged && !powerChanged) {
        Power_Time1 = false;
      } else {
        Lamp_autosvet = false;
      }
    }

    if (Power_Time1 != Saved_Power_Time1 || Lamp_autosvet != Saved_Lamp_autosvet) {
      myNex.writeStr("page set_lamp");
      if (Power_Time1 != Saved_Power_Time1) {
        myNex.writeNum("set_lamp.sw0.val", Power_Time1 ? 1 : 0);
        Saved_Power_Time1 = Power_Time1;
      }
      if (Lamp_autosvet != Saved_Lamp_autosvet) {
        myNex.writeNum("set_lamp.sw1.val", Lamp_autosvet ? 1 : 0);
        Saved_Lamp_autosvet = Lamp_autosvet;
      }
    }

    if (Power_Time1) {
      SetLamp = "timer";
    } else if (Lamp_autosvet) {
      SetLamp = "auto";
    } else {
      SetLamp = Lamp ? "on" : "off";
    }

    saveButtonState("button_Lamp", Lamp ? 1 : 0);
    saveValue<int>("Lamp_autosvet", Lamp_autosvet ? 1 : 0);
    saveValue<int>("Power_Time1", Power_Time1 ? 1 : 0);
    saveValue<String>("SetLamp", SetLamp);

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

void read_lamp_n0_n1(){
    in_hours = myNex.readNumber("set_lamp.n0.val"); in_minutes = myNex.readNumber("set_lamp.n1.val");
    //Lamp_timeON1 = String(hours / 10) + String(hours % 10) + ":" + String(minutes/ 10) + String(minutes % 10);
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("LampTimer", minutes, ui.timer("LampTimer").off);
    Saved_Lamp_timeON1 = minutes;
    // jee.var("Lamp_timeON1", Lamp_timeON1);
}
void trigger2(){read_lamp_n0_n1();}

// //printh 23 02 54 03 - "set_lamp" Присвоить n2 / n3 время отключения подсветки
// void read_lamp_n2_n3(){
//     in_hours = myNex.readNumber("set_lamp.n2.val");  in_minutes = myNex.readNumber("set_lamp.n3.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Lamp_timeOFF1=Lamp_timeOFF1 = buffer;
//     jee.var("Lamp_timeOFF1", Lamp_timeOFF1);
// }
// void trigger3(){read_lamp_n2_n3();}
void read_lamp_n2_n3(){
    in_hours = myNex.readNumber("set_lamp.n2.val");  in_minutes = myNex.readNumber("set_lamp.n3.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("LampTimer", ui.timer("LampTimer").on, minutes);
    Saved_Lamp_timeOFF1 = minutes;
    // jee.var("Lamp_timeOFF1", Lamp_timeOFF1);
}
void trigger3(){read_lamp_n2_n3();}

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
void read_RGB_n0_n1(){
    in_hours = myNex.readNumber("set_RGB.n0.val"); in_minutes = myNex.readNumber("set_RGB.n1.val");  
    //Lamp_timeON1 = String(hours / 10) + String(hours % 10) + ":" + String(minutes/ 10) + String(minutes % 10);
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("RgbTimer", minutes, ui.timer("RgbTimer").off);
    Saved_timeON_WS2815 = minutes;
    // jee.var("timeON_WS2815", timeON_WS2815);
}
void trigger5(){read_RGB_n0_n1();}

// //printh 23 02 54 06 - Присвоить n2 / n3 время отключения RGB ленты
// void read_RGB_n2_n3(){
//     in_hours = myNex.readNumber("set_RGB.n2.val"); in_minutes = myNex.readNumber("set_RGB.n3.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_timeOFF_WS2815=timeOFF_WS2815 = buffer;
//     jee.var("timeOFF_WS2815", timeOFF_WS2815);
// }
// void trigger6(){read_RGB_n2_n3();}
void read_RGB_n2_n3(){
    in_hours = myNex.readNumber("set_RGB.n2.val"); in_minutes = myNex.readNumber("set_RGB.n3.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("RgbTimer", ui.timer("RgbTimer").on, minutes);
    Saved_timeOFF_WS2815 = minutes;
    // jee.var("timeOFF_WS2815", timeOFF_WS2815);
}
void trigger6(){read_RGB_n2_n3();}

// //printh 23 02 54 07 - "set-RGB" Присвоить все кнопки подсветки RGB ленты
// void read_RGB_sw0_sw2_sw3(){
//     Pow_WS28151 = Pow_WS2815 = myNex.readNumber("set_RGB.sw3.val"); delay(50);
//     jee.var("Pow_WS2815", Pow_WS2815 ? "true" : "false");
//     Error err = RS485.addRequest(40001,1,0x05,1, Pow_WS2815 ? devices[0].value : devices[1].value);
void read_RGB_sw0_sw2_sw3(){
    Pow_WS2815 = myNex.readNumber("set_RGB.sw3.val"); // Читаем питание RGB без блокирующего delay.
    WS2815_Time1 = myNex.readNumber("set_RGB.sw0.val"); // Читаем таймер RGB без блокирующего delay.
    Pow_WS2815_autosvet = myNex.readNumber("set_RGB.sw2.val"); // Читаем авто-свет RGB без блокирующего delay.

    bool timerChanged = WS2815_Time1 != Saved_WS2815_Time1;
    bool autoChanged = Pow_WS2815_autosvet != Saved_Pow_WS2815_autosvet;
    if (WS2815_Time1 && Pow_WS2815_autosvet) {
      if (timerChanged && !autoChanged) {
        Pow_WS2815_autosvet = false;
      } else if (autoChanged && !timerChanged) {
        WS2815_Time1 = false;
      } else {
        Pow_WS2815_autosvet = false;
      }
    }

    if (WS2815_Time1 != Saved_WS2815_Time1 || Pow_WS2815_autosvet != Saved_Pow_WS2815_autosvet) {
      myNex.writeStr("page set_RGB");
      if (WS2815_Time1 != Saved_WS2815_Time1) {
        myNex.writeNum("set_RGB.sw0.val", WS2815_Time1 ? 1 : 0);
        Saved_WS2815_Time1 = WS2815_Time1;
      }
      if (Pow_WS2815_autosvet != Saved_Pow_WS2815_autosvet) {
        myNex.writeNum("set_RGB.sw2.val", Pow_WS2815_autosvet ? 1 : 0);
        Saved_Pow_WS2815_autosvet = Pow_WS2815_autosvet;
      }
    }

//     Saved_WS2815_Time1=WS2815_Time1 = myNex.readNumber("set_RGB.sw0.val"); delay(50);
//     jee.var("WS2815_Time1", WS2815_Time1 ? "true" : "false"); 
    if (WS2815_Time1) {
      SetRGB = "timer";
    } else if (Pow_WS2815_autosvet) {
      SetRGB = "auto";
    } else {
      SetRGB = Pow_WS2815 ? "on" : "off";
    }
//     Saved_Pow_WS2815_autosvet = Pow_WS2815_autosvet = myNex.readNumber("set_RGB.sw2.val"); 
//     jee.var("Pow_WS2815_autosvet", Pow_WS2815_autosvet ? "true" : "false"); 
// }
// void trigger7(){read_RGB_sw0_sw2_sw3();}
    saveButtonState("button_WS2815", Pow_WS2815 ? 1 : 0);
    saveValue<int>("Pow_WS2815_autosvet", Pow_WS2815_autosvet ? 1 : 0);
    saveValue<int>("WS2815_Time1", WS2815_Time1 ? 1 : 0);
    saveValue<String>("SetRGB", SetRGB);
}
void trigger7(){read_RGB_sw0_sw2_sw3();}

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

//printh 23 02 54 08 - "set-filtr" Присвоить n0 / n1 время вкл. по таймеру №1
void read_filtr_n0_n1(){
    in_hours = myNex.readNumber("set_filtr.n0.val"); in_minutes = myNex.readNumber("set_filtr.n1.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("FiltrTimer1", minutes, ui.timer("FiltrTimer1").off);
    Saved_Filtr_timeON1 = minutes;
}
void trigger8(){read_filtr_n0_n1();}


// //printh 23 02 54 0A - "set-filtr" Присвоить n4 / n5 время вкл. по таймеру №2
// void read_filtr_n4_n5(){
//     in_hours = myNex.readNumber("set_filtr.n4.val"); in_minutes = myNex.readNumber("set_filtr.n5.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeON2=Filtr_timeON2 = buffer;
//     jee.var("Filtr_timeON2", Filtr_timeON2);
// }
// void trigger10(){read_filtr_n4_n5();}

//printh 23 02 54 09 - "set-filtr" Присвоить n0 / n1 время откл. по таймеру №1
void read_filtr_n2_n3(){
    in_hours = myNex.readNumber("set_filtr.n2.val"); in_minutes = myNex.readNumber("set_filtr.n3.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("FiltrTimer1", ui.timer("FiltrTimer1").on, minutes);
    Saved_Filtr_timeOFF1 = minutes;
}
void trigger9(){read_filtr_n2_n3();}

// //printh 23 02 54 0B - "set-filtr" Присвоить n6 / n7 время откл. по таймеру №2
// void read_filtr_n6_n7(){
//     in_hours = myNex.readNumber("set_filtr.n6.val"); in_minutes = myNex.readNumber("set_filtr.n7.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Filtr_timeOFF2=Filtr_timeOFF2 = buffer;
//     jee.var("Filtr_timeOFF2", Filtr_timeOFF2);
// }
// void trigger11(){read_filtr_n6_n7();}

//printh 23 02 54 0A - "set-filtr" Присвоить n4 / n5 время вкл. по таймеру №2
void read_filtr_n4_n5(){
    in_hours = myNex.readNumber("set_filtr.n4.val"); in_minutes = myNex.readNumber("set_filtr.n5.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("FiltrTimer2", minutes, ui.timer("FiltrTimer2").off);
    Saved_Filtr_timeON2 = minutes;
}
void trigger10(){read_filtr_n4_n5();}

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

//printh 23 02 54 0B - "set-filtr" Присвоить n6 / n7 время откл. по таймеру №2
void read_filtr_n6_n7(){
    in_hours = myNex.readNumber("set_filtr.n6.val"); in_minutes = myNex.readNumber("set_filtr.n7.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("FiltrTimer2", ui.timer("FiltrTimer2").on, minutes);
    Saved_Filtr_timeOFF2 = minutes;
}
void trigger11(){read_filtr_n6_n7();}

// //printh 23 02 54 0E - "set-filtr" Присвоить все кнопки SW0, SW1, SW2
// void read_filtr_sw0_sw1_sw2(){
//     Saved_Filtr_Time1=Filtr_Time1 = myNex.readNumber("set_filtr.sw0.val");
//     jee.var("Filtr_Time1", Filtr_Time1 ? "true" : "false"); 

//printh 23 02 54 0C - "set-filtr" Присвоить n8 / n9 время вкл. по таймеру №3
void read_filtr_n8_n9(){
    in_hours = myNex.readNumber("set_filtr.n8.val"); in_minutes = myNex.readNumber("set_filtr.n9.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("FiltrTimer3", minutes, ui.timer("FiltrTimer3").off);
    Saved_Filtr_timeON3 = minutes;
}
void trigger12(){read_filtr_n8_n9();}

//     Saved_Filtr_Time2=Filtr_Time2 = myNex.readNumber("set_filtr.sw2.val");
//     jee.var("Filtr_Time2", Filtr_Time2 ? "true" : "false"); 

//printh 23 02 54 0D - "set-filtr" Присвоить n10 / n11 время откл. по таймеру №3
void read_filtr_n10_n11(){   
    in_hours = myNex.readNumber("set_filtr.n10.val"); in_minutes = myNex.readNumber("set_filtr.n11.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("FiltrTimer3", ui.timer("FiltrTimer3").on, minutes);
    Saved_Filtr_timeOFF3 = minutes;
}
void trigger13(){read_filtr_n10_n11();}

//     Saved_Filtr_Time3=Filtr_Time3 = myNex.readNumber("set_filtr.sw1.val");
//     jee.var("Filtr_Time3", Filtr_Time3 ? "true" : "false"); 

//     Power_Filtr1=Power_Filtr = myNex.readNumber("set_filtr.sw3.val");
//     jee.var("Power_Filtr", Power_Filtr ? "true" : "false");

//printh 23 02 54 0E - "set-filtr" Присвоить все кнопки SW0, SW1, SW2
void read_filtr_sw0_sw1_sw2(){
    requestNextionFiltrSwitchPoll();
}
void trigger14(){holdNextionFiltrWrites(2500); requestNextionFiltrSwitchPoll(2500);}

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

//printh 23 02 54 0F - "Clean"  считываем время таймера n0 / n1  насала промывки
void read_Clean_n0_n1(){
    in_hours = myNex.readNumber("Clean.n0.val"); in_minutes = myNex.readNumber("Clean.n1.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("CleanTimer1", minutes, ui.timer("CleanTimer1").off);
    Saved_Clean_timeON1 = minutes;
}
void trigger15(){read_Clean_n0_n1();}

// //printh 23 02 54 10 - "Clean" считываем время таймера n2 / n3  отключения промывки
// void read_Clean_n2_n3(){
//     in_hours = myNex.readNumber("Clean.n2.val"); in_minutes = myNex.readNumber("Clean.n3.val");
//     sprintf(buffer, "%02d:%02d", in_hours, in_minutes); Saved_Clean_timeOFF1=Clean_timeOFF1 = buffer;
//     jee.var("Clean_timeOFF1", Clean_timeOFF1);
// }
// void trigger16(){read_Clean_n2_n3();} 

//printh 23 02 54 10 - "Clean" считываем время таймера n2 / n3  отключения промывки
void read_Clean_n2_n3(){
    in_hours = myNex.readNumber("Clean.n2.val"); in_minutes = myNex.readNumber("Clean.n3.val");
    uint16_t minutes = static_cast<uint16_t>(in_hours * 60 + in_minutes);
    ui.setTimerMinutes("CleanTimer1", ui.timer("CleanTimer1").on, minutes);
    Saved_Clean_timeOFF1 = minutes;
}
void trigger16(){read_Clean_n2_n3();} 

// //printh 23 02 54 11 - "Clean" считываем состояние кнопок промывки SW0, SW1
// void read_Clean_sw0_sw1(){
//     Saved_Clean_Time1=Clean_Time1 = myNex.readNumber("Clean.sw0.val");
//     jee.var("Clean_Time1", Clean_Time1 ? "true" : "false"); 

//     Power_Clean1=Power_Clean = myNex.readNumber("Clean.sw1.val");
//     jee.var("Power_Clean", Power_Clean ? "true" : "false"); 

//printh 23 02 54 11 - "Clean" считываем состояние кнопок промывки SW0, SW1
void read_Clean_sw0_sw1(){
    Saved_Clean_Time1=Clean_Time1 = myNex.readNumber("Clean.sw0.val");
    Power_Clean1=Power_Clean = myNex.readNumber("Clean.sw1.val");

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


    saveValue<int>("Clean_Time1", Clean_Time1 ? 1 : 0);
    saveValue<int>("Power_Clean", Power_Clean ? 1 : 0);
}
void trigger17(){read_Clean_sw0_sw1();} 

//printh 23 02 54 12 - "Clean" Считываем кнопки ПН,ВТ,СР,ЧТ,ПТ,СБ,ВС для таймера
void read_Clean_chk(){
   Saved_chk1=chk1 = myNex.readNumber("Clean.bt0.val"); // Читаем понедельник без delay.
   Saved_chk2=chk2 = myNex.readNumber("Clean.bt1.val"); // Читаем вторник без delay.
   Saved_chk3=chk3 = myNex.readNumber("Clean.bt2.val"); // Читаем среду без delay.
   Saved_chk4=chk4 = myNex.readNumber("Clean.bt3.val"); // Читаем четверг без delay.
   Saved_chk5=chk5 = myNex.readNumber("Clean.bt4.val"); // Читаем пятницу без delay.
   Saved_chk6=chk6 = myNex.readNumber("Clean.bt5.val"); // Читаем субботу без delay.
   Saved_chk7=chk7 = myNex.readNumber("Clean.bt6.val");
   syncDaysSelectionFromClean();
   saveValue<String>("DaysSelect", DaysSelect);
}
void trigger18(){read_Clean_chk();} 
// /////////////////////////************* page heat  **************/////////////////////////////
// ////////////////////////************* page heat **************//////////////////////////////
// ///////////////////////************* page heat  **************///////////////////////////////


// //printh 23 02 54 13  -"heat" - свчитываем уставку (n0.val=h0.val) темепературы воды в бассейне
// void read_heat_h0(){
// printh 23 02 54 13  -"heat" - свчитываем уставку (n0.val=h0.val) темепературы воды в бассейне
void read_heat_h0(){
    int Sider_heat_on = myNex.readNumber("heat.h0.val");
//     int Sider_heat_on = myNex.readNumber("heat.h0.val");

//     if(Sider_heat_on < 31 && Sider_heat_on >= 0) { // Sider_heat  != 777777){ 
//         Sider_heat1 = Sider_heat = Sider_heat_on;
//         jee.var("Sider_heat", String(Sider_heat));
//         jee.var("Heat", String(Power_Heat ? "Нагрев" : "Откл.")+ ", " + "T:" + String(DS1)+ ", " + "T_s:" + String(Sider_heat));
//     } 
// }
// void trigger19(){ read_heat_h0();}
    if(Sider_heat_on < 31 && Sider_heat_on >= 0) {
        Sider_heat1 = Sider_heat = Sider_heat_on;
        saveValue<int>("Sider_heat", Sider_heat);
    }
}
void trigger19(){ read_heat_h0();}

// //printh 23 02 54 14 -"heat" - свчитываем состояние кнопки SW0
// void read_heat_sw0(){
//     Activation_Heat1=Activation_Heat = myNex.readNumber("heat.sw0.val");
//     jee.var("Activation_Heat", Activation_Heat ? "true" : "false"); 
// }
// void trigger20(){read_heat_sw0();}
// printh 23 02 54 14 -"heat" - свчитываем состояние кнопки SW0
void read_heat_sw0(){
    Activation_Heat1 = Activation_Heat = myNex.readNumber("heat.sw0.val");
    saveValue<int>("Activation_Heat", Activation_Heat ? 1 : 0);
}
void trigger20(){read_heat_sw0();}


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
void read_Dispensers_sw0_sw1(){
    uint32_t rawValue = myNex.readNumber("Dispensers.sw0.val");
    if(rawValue == 777777) return;
    Saved_PHControlACO = PH_Control_ACO = rawValue != 0;
    holdNextionDispensersWrites();
    saveValue<int>("PH_Control_ACO", PH_Control_ACO ? 1 : 0);
}
void trigger22(){holdNextionDispensersWrites(); read_Dispensers_sw0_sw1();}
// // printh 23 02 54 17  - Dispensers  свчитываем состояние ComboBox cb0.txt
// void read_Dispensers_cb0(){
//     //Отложенное повторное выполнение через 2 секунды - выполняем NextionDelay();
//     triggerActivated_Nextion=true; Function_Nextion = 23; // читаем - Dispensers.cb0.val
// }
// void trigger23(){read_Dispensers_cb0();
//  Saved_ACO_Work = ACO_Work = myNex.readNumber("Dispensers.cb0.val") +1;  //Отложенное повторное выполнение через 1 секунду
//  jee.var("ACO_Work", String(ACO_Work));
// } 

void read_Dispensers_cb0(){ // Колбэк-функция обработки значения cb0
    uint32_t rawValue = myNex.readNumber("Dispensers.cb0.val"); // Читаем индекс выбранной строки ComboBox Nextion.
    if(rawValue == 777777) return; // Выход из функции при получении служебного/ошибочного значения - чтобы не помещало правильной работе логики
    int nextValue = dosingModeFromNextionComboIndex(static_cast<int>(rawValue)); // Индекс строки переводим в код периода ESP32.
    Saved_ACO_Work = ACO_Work = nextValue; // Сохраняем итоговое корректное значение
    holdNextionDispensersWrites(6000); // Даем Nextion время закрыть список, чтобы ESP32 не вернул старое значение.
    saveValue<int>("ACO_Work", ACO_Work); // Записываем значение в энергонезависимую память
}
void trigger23(){holdNextionDispensersWrites(); read_Dispensers_cb0();}
// // printh 23 02 54 18 - Dispensers  свчитываем состояние n4 / n5
// void read_Dispensers_sw2_sw3(){
//     Saved_NaOCl_H2O2_Control = NaOCl_H2O2_Control = myNex.readNumber("Dispensers.sw2.val"); delay(10);
//     jee.var("NaOCl_H2O2_Control", NaOCl_H2O2_Control ? "true" : "false");

//     // Saved_Activation_Timer_H2O2 = Activation_Timer_H2O2 = myNex.readNumber("Dispensers.sw3.val"); delay(10);
//     // jee.var("Activation_Timer_H2O2", Activation_Timer_H2O2 ? "true" : "false"); 

// }
// void trigger24(){read_Dispensers_sw2_sw3();} 
void read_Dispensers_sw2_sw3(){
    uint32_t rawValue = myNex.readNumber("Dispensers.sw2.val");
    if(rawValue == 777777) return;
    Saved_NaOCl_H2O2_Control = NaOCl_H2O2_Control = rawValue != 0;
    holdNextionDispensersWrites();
    saveValue<int>("NaOCl_H2O2_Control", NaOCl_H2O2_Control ? 1 : 0);
}
void trigger24(){holdNextionDispensersWrites(); read_Dispensers_sw2_sw3();}

// // printh 23 02 54 19 - Dispensers  свчитываем состояние n4 / n5
// void read_Dispensers_cb1(){
//    //Отложенное повторное выполнение через 2 секунды - выполняем NextionDelay();
//     triggerActivated_Nextion=true; Function_Nextion = 25; // читаем - Dispensers.cb1.val
// }
// void trigger25(){read_Dispensers_cb1();

// Saved_H2O2_Work = H2O2_Work = myNex.readNumber("Dispensers.cb1.val") +1;
// jee.var("H2O2_Work", String(H2O2_Work));

// } 

void read_Dispensers_cb1(){ // Колбэк-функция обработки значения cb1
    uint32_t rawValue = myNex.readNumber("Dispensers.cb1.val"); // Читаем индекс выбранной строки ComboBox Nextion.
    if(rawValue == 777777) return; // Выходим из функции при служебном/ошибочном значении
    int nextValue = dosingModeFromNextionComboIndex(static_cast<int>(rawValue)); // Индекс строки переводим в код периода ESP32.
    Saved_H2O2_Work = H2O2_Work = nextValue; // Сохраняем скорректированное значение в рабочие переменные
    holdNextionDispensersWrites(6000); // Даем Nextion время закрыть список, чтобы ESP32 не вернул старое значение.
    saveValue<int>("H2O2_Work", H2O2_Work); // Записываем значение в энергонезависимую память
}

void trigger25(){holdNextionDispensersWrites(); read_Dispensers_cb1();}

inline float readDispensersTenths(const char* wholeComponent, const char* tenthComponent, float fallback){
    uint32_t wholeRaw = myNex.readNumber(wholeComponent); // Читаем целую часть без дополнительного delay.
    uint32_t tenthRaw = myNex.readNumber(tenthComponent); // Читаем десятые без дополнительного delay.
    if(wholeRaw == 777777 || tenthRaw == 777777) return fallback;
    int whole = static_cast<int>(wholeRaw);
    int tenth = static_cast<int>(tenthRaw);
    if(tenth < 0) tenth = 0;
    if(tenth > 9) tenth = 9;
    return static_cast<float>(whole) + (static_cast<float>(tenth) / 10.0f);
}

void read_Dispensers_PH_CL_limits(){
    holdNextionDispensersWrites(4000);
    const float oldPHLower = PH_Lower;
    const float oldPHUpper = PH_Upper;
    const float oldCLLower = CL_Lower;
    const float oldCLUpper = CL_Upper;
    PH_Lower = readDispensersTenths("Dispensers.n0.val", "Dispensers.n1.val", PH_Lower); // Читаем нижний предел pH.
    PH_Upper = readDispensersTenths("Dispensers.n2.val", "Dispensers.n3.val", PH_Upper); // Читаем верхний предел pH.
    CL_Lower = readDispensersTenths("Dispensers.n6.val", "Dispensers.n7.val", CL_Lower); // Читаем нижний предел CL.
    CL_Upper = readDispensersTenths("Dispensers.n4.val", "Dispensers.n5.val", CL_Upper); // Читаем верхний предел CL.
    normalizeChemicalLimits(); // Исправляем диапазоны после ввода с Nextion.
    persistChangedChemicalLimits(oldPHLower, oldPHUpper, oldCLLower, oldCLUpper); // Сохраняем только реально измененные поля.
}
void trigger26(){holdNextionDispensersWrites(4000); read_Dispensers_PH_CL_limits();}

inline bool floatSettingChanged(float oldValue, float newValue) { // Проверяем изменение десятичной настройки с защитой от шума float.
    float delta = oldValue > newValue ? oldValue - newValue : newValue - oldValue; // Считаем модуль разницы без дополнительной math-библиотеки.
    return delta >= 0.05f; // Для шагов 0.1 считаем изменением половину шага и больше.
}

void poll_Dispensers_PH_CL_limits(){ // Периодически читаем pH/CL с открытой страницы Nextion, даже если trigger не пришел.
    static unsigned long lastPollMs = 0; // Запоминаем время последнего опроса, чтобы не забивать UART.
    static uint8_t pollStep = 0; // Читаем только одну настройку за проход, чтобы экран не тормозил loop.
    if(Nx_page_id != 9) return; // Читаем только когда реально открыта страница Dispensers.
    if(millis() - lastPollMs < 1000UL) return; // Опрос не чаще одного раза в секунду.
    lastPollMs = millis(); // Обновляем время опроса.
    bool changed = false; // Флаг нужен, чтобы не писать NVS без реального изменения.
    const float oldPHLower = PH_Lower;
    const float oldPHUpper = PH_Upper;
    const float oldCLLower = CL_Lower;
    const float oldCLUpper = CL_Upper;

    if(pollStep == 0){ // Первый проход читает нижний предел pH.
      float nextValue = readDispensersTenths("Dispensers.n0.val", "Dispensers.n1.val", PH_Lower); // Получаем pH lower.
      changed = floatSettingChanged(PH_Lower, nextValue); // Проверяем изменение с шагом 0.1.
      if(changed) PH_Lower = nextValue; // Применяем новое значение только если оно реально изменилось.
    } else if(pollStep == 1){ // Второй проход читает верхний предел pH.
      float nextValue = readDispensersTenths("Dispensers.n2.val", "Dispensers.n3.val", PH_Upper); // Получаем pH upper.
      changed = floatSettingChanged(PH_Upper, nextValue); // Проверяем изменение с шагом 0.1.
      if(changed) PH_Upper = nextValue; // Применяем новое значение только если оно реально изменилось.
    } else if(pollStep == 2){ // Третий проход читает нижний предел CL.
      float nextValue = readDispensersTenths("Dispensers.n6.val", "Dispensers.n7.val", CL_Lower); // Получаем CL lower.
      changed = floatSettingChanged(CL_Lower, nextValue); // Проверяем изменение с шагом 0.1.
      if(changed) CL_Lower = nextValue; // Применяем новое значение только если оно реально изменилось.
    } else { // Четвертый проход читает верхний предел CL.
      float nextValue = readDispensersTenths("Dispensers.n4.val", "Dispensers.n5.val", CL_Upper); // Получаем CL upper.
      changed = floatSettingChanged(CL_Upper, nextValue); // Проверяем изменение с шагом 0.1.
      if(changed) CL_Upper = nextValue; // Применяем новое значение только если оно реально изменилось.
    }

    pollStep = (pollStep + 1) % 4; // Следующий раз читаем следующее поле.
    if(!changed) return; // Если ничего не изменилось, NVS не трогаем.
    normalizeChemicalLimits(); // Исправляем перевернутые или выходящие за пределы значения.
    persistChangedChemicalLimits(oldPHLower, oldPHUpper, oldCLLower, oldCLUpper); // Сохраняем только реально измененные поля.
}

inline bool readNextionBoolSafe(const char* component, bool fallback) { // Безопасно читаем bool-компонент Nextion.
    uint32_t rawValue = myNex.readNumber(component); // Получаем числовое значение компонента.
    if(rawValue == 777777) return fallback; // При ошибке чтения оставляем текущее состояние.
    return rawValue != 0; // Любое ненулевое значение считаем включенным.
}

void read_set_topping_controls(){ // Читаем переключатели страницы контроля уровня воды.
    bool nextLevelControl = readNextionBoolSafe("set_topping.sw0.val", Activation_Water_Level); // sw0: автоматический контроль уровня.
    bool nextDrain = readNextionBoolSafe("set_topping.sw1.val", Power_Drain); // sw1: режим слива воды.
    bool nextTopping = readNextionBoolSafe("set_topping.sw2.val", Power_Topping); // sw2: ручной клапан долива.

    if(nextDrain) { // Если пользователь включил слив.
      applyWaterControlRequest("Power_Drain", true); // Включаем слив через общую защитную логику.
    } else { // Если слив выключен.
      applyWaterControlRequest("Power_Drain", false); // Выключаем слив через общую защитную логику.
      applyWaterControlRequest("Activation_Water_Level", nextLevelControl); // Применяем контроль уровня с учетом запрета при сливе.
      applyWaterControlRequest("Power_Topping", nextTopping); // Применяем ручной долив с учетом верхнего датчика.
    }
}

void trigger32(){read_set_topping_controls();} // Триггер Nextion для sw0 на странице set_topping.
void trigger33(){read_set_topping_controls();} // Триггер Nextion для sw1 на странице set_topping.
void trigger34(){read_set_topping_controls();} // Триггер Nextion для sw2 на странице set_topping.



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
void trigger31(){
  GmtOffsetSyncResult gmtOffsetSync = applyGmtOffsetFromNextion(true); // Nextion изменил GMT: сразу применяем часовой пояс в ESP и сдвигаем локальное время.
  if (gmtOffsetSync.changed) {
    syncNextionRtcFromEpoch(getCurrentEpoch()); // После сдвига GMT сразу возвращаем в Nextion пересчитанное локальное RTC-время.
  }
}


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
    {
      uint32_t rawValue = myNex.readNumber("Dispensers.cb0.val"); // Старый отложенный путь оставлен совместимым с корректным маппингом.
      if(rawValue != 777777) {
        Saved_ACO_Work = ACO_Work = dosingModeFromNextionComboIndex(static_cast<int>(rawValue)); // Индекс ComboBox переводим через таблицу.
        saveValue<int>("ACO_Work", ACO_Work);
      }
    }
    //Serial.println(ACO_Work);
    // jee.var("ACO_Work", String(ACO_Work));
    break;
    case 24:  break;
    case 25: 
    {
      uint32_t rawValue = myNex.readNumber("Dispensers.cb1.val"); // Старый отложенный путь оставлен совместимым с корректным маппингом.
      if(rawValue != 777777) {
        Saved_H2O2_Work = H2O2_Work = dosingModeFromNextionComboIndex(static_cast<int>(rawValue)); // Индекс ComboBox переводим через таблицу.
        saveValue<int>("H2O2_Work", H2O2_Work);
      }
    }
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
return; // При старте не читаем поля Nextion, чтобы дисплей не перетирал сохраненные в ESP32/NVS настройки.

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
read_Dispensers_sw0_sw1();
read_Dispensers_cb0();
read_Dispensers_sw2_sw3();
read_Dispensers_cb1();
read_Dispensers_PH_CL_limits();
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
