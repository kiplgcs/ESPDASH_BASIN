//*****************************************************************************************************//
  #include "ModbusClientRTU.h"
  bool ReadRelayArray[16];// Массив для хранения прочитанных состояний реле - 16 бит - 16 реле в виде false/true
  bool ReadInputArray[16];// Массив для хранения прочитанных состояний входов - 16 бит - 16 входов в виде false/true
  bool AktualReadRelay = false; // Флаг актульных данных после прочтенния после перезагрузки или передаче данных.
  bool AktualReadInput = false; // Флаг актульных данных после прочтенния после перезагрузки или передаче данных.
  String InStr = ""; //Строка полученных данных из Modbus RTU
  char CharArray[16]; // Текстовый массив Char


// Создаем экземпляр клиента ModbusRTU
  ModbusClientRTU RS485(Serial2);    



  // Определение структуры для хранения данных для отправки
  struct DeviceData {
    //uint16_t address;   //ПЛК Адреса _
    // uint8_t param1; // Значение по умолчанию
    // uint8_t param2; // Поддерживаемые функциональные коды 01/03/05/06/15/16
    //uint8_t relay; // Номер реле 0-15 
    uint16_t value; // Значение 1=закрыть ； 0=открыть ；
  };

  // Массив для хранения данных устройств
  DeviceData devices[] = {
      {0xFF00},  // Включить реле
      {0x0000},  // Отключить реле
  };



//Определим функцию обратного вызова для поступающих ответов данных. 
void handleData(ModbusMessage msg, uint32_t token) 
  {
    Serial.printf("Response: serverID=%d, FC=%d, Token=%08X, length=%d:\n", msg.getServerID(), msg.getFunctionCode(), token, msg.size());

    byte InArray[8]; 
  

    memset(InArray, '\0', 8); // 8 байт не в HEX представлении, очистить массив перед записью данных 
    memset(CharArray, '\0', 16); //  16 символов так как дабавляем нули для HEX представления

    int Index=0;
  for (auto& byte : msg) {InArray[Index++] = byte;}

    //Временная функция для видуального отображения в Serial.print:
    sprintf(CharArray, "%02X%02X%02X%02X%02X%02X%02X%02X\n", InArray[0], InArray[1], InArray[2], InArray[3], InArray[4], InArray[5], InArray[6], InArray[7]);   
    Serial.print("Token: "); Serial.println(token);
    Serial.println(CharArray);

  
    //Периодически читаем для обратной сосояния входов и реле и записываем в массивы
    if(token ==40050){
    // Конвертируем InArray[3] и InArray[4] в логическое представление и объединяем в один массив
      for (int i = 0; i < 8; ++i) {
        ReadRelayArray[i] = ((InArray[4] >> i) & 0x01) == 0 ? false : true; // Записываем первые 8 бит из InArray[4]
        ReadRelayArray[i + 8] = ((InArray[3] >> i) & 0x01) == 0 ? false : true; // Записываем следующие 8 бит из InArray[5]
        AktualReadRelay=true; //Флаг что данные актульны
    }} else 
    if(token ==40060){
      for (int i = 0; i < 8; ++i) {
        ReadInputArray[i] = ((InArray[4] >> i) & 0x01) == 0 ? false : true; // Записываем первые 8 бит из InArray[4]
        ReadInputArray[i + 8] = ((InArray[3] >> i) & 0x01) == 0 ? false : true; // Записываем следующие 8 бит из InArray[5]
        AktualReadInput=true; //Флаг что данные актульны
    }}

  // InStr=String(CharArray);
  // Serial.println(InStr);

}



void setup_Modbus() {
 //Настраиваем Serial2, подключенный к Modbus RTU
  Serial2.begin(19200, SERIAL_8N1, GPIO_NUM_16, GPIO_NUM_17);

  //Настраиваем клиент ModbusClientRTU. предоставить функции обработчика onData и onError
  RS485.onDataHandler(&handleData);
  //RS485.onErrorHandler(&handleError);

  RS485.setTimeout(500);//Запустить фоновую задачу ModbusRTU
  
  RS485.begin();//Запускаем  фоновую задачу ModbusClientRTU
}



void loop_Modbus(int interval) {
  static unsigned long timer;
  if (interval + timer > millis()) return; 
  timer = millis();
//---------------------------------------------------------------------------------
  //https://emodbus.github.io/modbusclient-common-api
  //https://emodbus.github.io/modbusclient
  //https://github.com/eModbus/eModbus/issues/40
  //https://pastebin.com/VMmQpY27
//---------------------------------------------------------------------------------
  //Error err= RS485.addRequest(Tokin, адр.модуля, функция, адр.реле, вкл./выкл);
  //Error err = RS485.addRequest(Tokin, (должен быть свой для каждой функции, чтобы запрашиваемы знал кому и что отвечать
                                // адрес платы
                                // командная строка  0x03 -читать / 0x05 - писать и т.п.
                                // адрес устройств на плате - 0 - 16 реле / 4 - 16Iput
                                // передаваемые данные для реле 1/0 включение/отключение
  

}