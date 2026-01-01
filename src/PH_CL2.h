// // Функция для усреднения значений АЦП
// int readAnalog(int pin, int samples = 10) {
//     int sum = 0;
//     for (int i = 0; i < samples; i++) {
//         sum += analogRead(pin);
//         delay(5); // Небольшая задержка между измерениями
//     }
//     return sum / samples;
// }



// // Функция для усреднения значений АЦП
// int readAnalog_DS1x15(int ads_num, int adc) {
// // int16_t adcx;
// int32_t adcx = 0; int samples = 3;
// //int16_t adcx = ads.readADC_SingleEnded(adc); 
// for(uint8_t i = 0; i < samples; i++) {

// if(ads_num==1) {adcx += ads1.readADC_SingleEnded(adc);}
// else if ((ads_num==2)) {adcx += ads2.readADC_SingleEnded(adc);}

// delay(50); // Небольшая задержка между измерениями
// } 
// adcx = adcx / samples; // Вычисляем средние значения АЦП напряжения и тока

// //float voltage = adcx * (6.144 / 2048.0); //voltage = adc0*(6.144 / 32767.5); //16-бит от -32768 до +32767; 12-бит от 0 до 4095. 
// int mV = adcx * 0.1875; // / 1000.0; // или voltage = adc0*(6.144 / 32767.5); - 16-бит от -32768 до +32767;

// return mV;
// }

const uint8_t samples = 3;           // Количество выборок для усреднения аналоговых измерений
const uint8_t ads1Address = 0x48;     // I2C-адрес первого ADS1115 (pH)
const uint8_t ads2Address = 0x49;     // I2C-адрес второго ADS1115 (Cl2)
const uint16_t adsConversionTimeoutMs = 25; // Таймаут ожидания АЦП, чтобы не зависать в цикле
// --- Для pH-датчика (ADS1, канал A0) ---
uint8_t countPH = 0;                 // Счётчик выборок
int32_t totalPH = 0;                 // Сумма всех выборок для усреднения
int analogValuePH;

// --- Для хлора (ADS2, канал A1) ---
uint8_t countCL = 0;                 // Счётчик выборок
int32_t totalCL = 0;                 // Сумма всех выборок
int analogValueCL;



/************************* Настраиваем  АЦП модуль ADS1115 16-бит *********************************/
void setup_ADS1115_PH_CL2(){
  //Для подключения 16-битного АЦП ADS1115:
  // в базовом варианте платы NodeMCU-32S I2C интерфейс завязан на пины 21 и 22
  Wire.begin(4, 5); //10,11 или 8,9  // Инициализируем I2C с SDA  и SCL 

  //Serial.println("Getting single-ended readings from AIN0..3");
  //Serial.println("ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)");
  // Входной диапазон (или усиление) АЦП можно изменить с помощью следующих
  // функции, но будьте осторожны, чтобы не превышать VDD + 0,3 В макс или
  // превышать верхний и нижний пределы, если вы отрегулируете диапазон ввода! 
  //Неправильное изменение этих значений может привести к повреждению вашего АЦП!
  //                                                                ADS1015  ADS1115
  //                                                                -------  -------
  // ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default) //тут настраиваем опорное напряжение
  ads1.setGain(GAIN_TWOTHIRDS);
  ads2.setGain(GAIN_TWOTHIRDS);


  // ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
  // ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
  // ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
  // ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
  // ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
  
  // ads.begin(); //инициализируем модуль с ранее настроенными параметрами

  ads1.begin(ads1Address);
  ads2.begin(ads2Address);
}





// Проверка доступности устройства на I2C (быстрый "ping" без чтения данных).
bool isI2CDeviceReady(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}

// Безопасное чтение ADS1115: проверяем I2C, запускаем конверсию и ждём готовность с таймаутом.
// Возвращает false, если устройство недоступно или конверсия не завершилась вовремя.
bool readAdsSingleEndedSafe(Adafruit_ADS1115 &ads, uint8_t address, uint8_t channel, int16_t &result) {
    if (channel > 3) {
        return false;
    }

    if (!isI2CDeviceReady(address)) {
        return false;
    }

    ads.startADCReading(MUX_BY_CHANNEL[channel], false);
    unsigned long start = millis();
    while (!ads.conversionComplete()) {
        if (millis() - start > adsConversionTimeoutMs) {
            return false;
        }
        // Короткая задержка, чтобы не грузить CPU в плотном цикле ожидания.
        delay(1);
    }

    result = ads.getLastConversionResults();
    return true;
}



void loop_PH(int interval) {
    static unsigned long timerPH = 0;
    if (millis() - timerPH < interval) return;  // Корректная проверка времени
    timerPH = millis();
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------
//---------------------------------------------------------------------------------

    //https://developer.alexanderklimov.ru/arduino/esp32/analog.php
    //https://microkontroller.ru/arduino-projects/ph-metr-izmeritel-kislotnosti-na-arduino-uno-i-zhk-displee/
    //int analogValuePH = readAnalog(34); //int analogValuePH = analogRead(34); // читаем значение с аналогового пина 34
    // int analogValuePH = readAnalog_DS1x15(1, 0);

    // ----------- pH СЕНСОР (ADS1, канал A0) -----------
 
    // if (!ads1.begin(0x48)) return; // если I2C не отвечает — выходим

     if (!isI2CDeviceReady(ads1Address)) { //if (!readAdsSingleEndedSafe(ads1, ads1Address, 0, rawPH)) {
        // Если АЦП недоступен/завис — сбрасываем накопление и выходим,
        // чтобы не держать старые данные и не блокировать основной loop().
        countPH = 0;
        totalPH = 0;
        return;
    }

    totalPH += ads1.readADC_SingleEnded(0); // Читаем значение с канала A0 и добавляем к сумме
    countPH++;                              // Увеличиваем счётчик выборок



    if (countPH < samples) return; // Проверяем  достигнуто нужное количество выборок для усреденения АЦП
      
        analogValuePH = (totalPH / samples) * 0.1875; // Усреднение и преобразование в милливольты   
        
        countPH = 0;// Сброс счётчиков
        totalPH = 0;
        
    // // --- Проверяем корректность данных ---
    // if (isnan(analogValuePH) || analogValuePH <= 0 || analogValuePH > 32000) {
    //     jee.var("PH", "Ошибка датчика");
    //     return; // Пропускаем расчёт, если данные неверны
    // }


    // Это изменение pH на единицу изменения АЦП - коэффициента наклона (k) для линейной функции
    float k = (PH2 - PH1) / (PH2_CAL - PH1_CAL);

    // Это значение pH, когда значение АЦП равно нулю, скорректированное на наклон
    float b = PH1 - k * PH1_CAL;

    // Вычисляем значение pH
    PH = k * analogValuePH + b;

    // Корректировка pH в зависимости от температуры - Коэффициент компенсации, зависит от сенсора
    float temperatureCoefficient = 0.03;
    //float Temper_Reference = 20.0; // Референсная температура (20°C)

    
    // --- Проверка корректности температуры ---
    /*if (isnan(Temper_PH) || Temper_PH <= -20 || Temper_PH >= 50 || Temper_PH == -127.0 || Temper_PH == 85.0) {
        jee.var("PH", "Ошибка температуры");
        return;  // Прерываем выполнение, если температура неверная
    } */Temper_PH = DS1; // Измеряемая температура

    PH -= temperatureCoefficient * (Temper_PH - Temper_Reference);

    // Рассчитываем корректированное значение АЦП после компенсации по температуре
    analogValuePH_Comp = (PH - b) / k;


    // Calibration feedback was previously sent via JeeUI; PH value is now provided through the dashboard JSON.                
}






/********************************************************************************************/
/********************************************************************************************/
/***************************     ОПРЕДЕЛЕНИЕ ОВП и CL      **********************************/
/********************************************************************************************/
/********************************************************************************************/

// Зависимость показаний сенсора ОВП от температуры
const int tempToMvTable[7][2] = {
    {10, 242}, //грС, mV
    {15, 235},
    {20, 227},
    {25, 222},
    {30, 215},
    {35, 209},
    {40, 201}
};

// Функция для интерполяции мВ в зависимости от температуры
int interpolateMvByTemperature(float temperature) {
    // Если температура ниже минимального значения в таблице, возвращаем значение мВ для минимальной температуры
    if (temperature <= tempToMvTable[0][0]) {
        return tempToMvTable[0][1];
    } 
    // Если температура выше максимального значения в таблице, возвращаем значение мВ для максимальной температуры
    else if (temperature >= tempToMvTable[6][0]) {
        return tempToMvTable[6][1];
    }

    // Определяем индексы для температурных интервалов, между которыми находится текущая температура
    int tempIndex1 = 0, tempIndex2 = 0;
    for (int i = 0; i < 6; i++) {
        // Если температура находится в интервале между текущим и следующим значением в таблице, запоминаем индексы
        if (temperature <= tempToMvTable[i + 1][0]) {
            tempIndex1 = i;
            tempIndex2 = i + 1;
            break;
        }
    }

    // Получаем значения мВ для найденных температурных интервалов
    int mv1 = tempToMvTable[tempIndex1][1];
    int mv2 = tempToMvTable[tempIndex2][1];
    // Получаем температуры для интервала
    float temp1 = tempToMvTable[tempIndex1][0];
    float temp2 = tempToMvTable[tempIndex2][0];

    // Интерполируем значение мВ на основе текущей температуры
    return mv1 + (mv2 - mv1) * (temperature - temp1) / (temp2 - temp1);
}

// Функция корректировки измерения сенсора в зависимости от температуры:
int correctValueByTemperature(int analogValue, float temperature) {
    int tempMv = interpolateMvByTemperature(temperature);
    return analogValue + (242 - tempMv); // 242 - базовое значение мВ при 10°C
}



// Таблица ORP (мВ) для каждого значения pH
const int orpTable[23][13] = {
// 6.9  7.0  7.2  7.3  7.4  7.5  7.6  7.7  7.8  7.9  8.0  8.1  8.2
  {507, 505, 502, 500, 499, 497, 496, 494, 493, 491, 490, 488, 487}, // 0.2
  {561, 558, 553, 550, 548, 546, 544, 541, 539, 536, 534, 532, 529}, // 0.3
  {599, 596, 590, 586, 583, 580, 577, 574, 571, 568, 565, 562, 559}, // 0.4
  {629, 625, 618, 615, 611, 607, 604, 600, 597, 593, 590, 586, 583}, // 0.5
  {652, 648, 640, 637, 632, 629, 625, 621, 617, 613, 610, 605, 602}, // 0.6
  {663, 658, 650, 646, 642, 638, 634, 630, 626, 622, 618, 614, 610}, // 0.65
  {673, 669, 660, 656, 651, 647, 643, 639, 635, 630, 626, 622, 618}, // 0.67
  {682, 677, 668, 664, 660, 655, 651, 647, 642, 638, 634, 629, 625}, // 0.75
  {690, 686, 677, 672, 668, 663, 659, 654, 650, 645, 641, 636, 632}, // 0.8
  {698, 694, 684, 680, 675, 670, 666, 661, 657, 652, 647, 643, 638}, // 0.85
  {706, 702, 692, 687, 682, 677, 673, 668, 663, 658, 654, 649, 644}, // 0.9
  {713, 708, 698, 694, 689, 684, 679, 674, 669, 664, 659, 654, 650}, // 0.95
  {720, 715, 705, 700, 695, 690, 685, 680, 675, 670, 665, 660, 655}, // 1
  {733, 727, 717, 712, 707, 701, 696, 691, 686, 680, 675, 670, 665}, // 1.1
  {744, 739, 728, 722, 717, 712, 706, 701, 695, 690, 685, 679, 674}, // 1.2
  {755, 749, 738, 732, 727, 721, 716, 710, 705, 699, 694, 688, 682}, // 1.3
  {765, 759, 747, 742, 736, 730, 724, 719, 713, 707, 702, 696, 690}, // 1.4 
  {774, 768, 756, 750, 744, 738, 732, 727, 721, 715, 709, 703, 697}, // 1.5
  {790, 784, 771, 765, 759, 753, 747, 741, 735, 728, 722, 716, 710}, // 1.7
  {798, 792, 779, 773, 766, 760, 754, 748, 741, 735, 729, 722, 716}, // 1.8
  {812, 805, 792, 785, 779, 773, 766, 760, 753, 747, 740, 734, 727}, // 2.0
  {824, 818, 804, 797, 791, 784, 777, 771, 764, 757, 751, 744, 737}, // 2.2
  {841, 834, 826, 813, 806, 800, 792, 785, 778, 771, 764, 757, 751}  // 2.5
};

const float ppmClValues[] = {0.2, 0.3, 0.4, 0.5, 0.6, 0.65, 0.67, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0, 1.1, 1.2,
  1.3, 1.4, 1.5, 1.7, 1.8, 2.0, 2.2, 2.5
};
// Таблица значений pH, соответствующих столбцам
const float pHValues[] = {6.9, 7.0, 7.2, 7.3, 7.4, 7.5, 7.6, 7.7, 7.8, 7.9, 8.0, 8.1, 8.2};


// Функция для интерполяции значения ppm Cl на основе pH и ORP
float interpolateClValue(float pH, int orp) {
    // Ограничиваем pH в пределах допустимых значений
    if (pH < pHValues[0]) {
        pH = pHValues[0];
    } else if (pH > pHValues[12]) {
        pH = pHValues[12];
    }

    // Ограничиваем значение ORP в пределах допустимых значений
    if (orp < orpTable[0][0]) {
        orp = orpTable[0][0]; // Минимальное значение ORP
    } else if (orp > orpTable[22][12]) {
        orp = orpTable[22][12]; // Максимальное значение ORP
    }

    int pH_index1 = 0, pH_index2 = 0;

    // Поиск ближайших индексов pH
    for (int i = 0; i < 12; i++) {
        if (pHValues[i] <= pH && pHValues[i + 1] > pH) {
            pH_index1 = i;
            pH_index2 = i + 1;
            break;
        }
    }

    float ppmCl1 = 0.0, ppmCl2 = 0.0;

    // Интерполяция по первому индексу pH
    for (int i = 0; i < 22; i++) {
        if (orp >= orpTable[i][pH_index1] && orp <= orpTable[i + 1][pH_index1]) {
            ppmCl1 = ppmClValues[i] + (ppmClValues[i + 1] - ppmClValues[i]) * (orp - orpTable[i][pH_index1]) / (orpTable[i + 1][pH_index1] - orpTable[i][pH_index1]);
            break;
        }
    }

    // Интерполяция по второму индексу pH
    for (int i = 0; i < 22; i++) {
        if (orp >= orpTable[i][pH_index2] && orp <= orpTable[i + 1][pH_index2]) {
            ppmCl2 = ppmClValues[i] + (ppmClValues[i + 1] - ppmClValues[i]) * (orp - orpTable[i + 1][pH_index2]) / (orpTable[i + 1][pH_index2] - orpTable[i][pH_index2]);
            break;
        }
    }

    // Финальная интерполяция по pH
    float ppmCl = ppmCl1 + (ppmCl2 - ppmCl1) * (pH - pHValues[pH_index1]) / (pHValues[pH_index2] - pHValues[pH_index1]);

    return ppmCl;
}


// Функция для калибровки ORP сенсора
int calibrateSensor(int ORP_mV) {
    return ORP_mV + Calibration_ORP_mV; // Применяем калибровочное смещение
}

 void btnCalCl(){
    //ESP.restart(); // Перезагрузить ESP32
    }


//Определение ОВП-сенсор измеряет редокс-потенциал в милливольтах (мВ)
//ОВП-сенсор измеряет окислительно-восстановительный потенциал (редокс-потенциал), 
//который отражает способность воды окислять или восстанавливать химические вещества.
//Высокие значения ОВП (обычно выше 650 мВ) указывают на наличие окислителей, таких как хлор, в воде.

// Основная функция чтения и расчёта ppm Cl (переписана)
void loop_CL2(int interval) {
  static unsigned long timer;
  if (interval + timer > millis()) return;  // Проверяем, прошло ли нужное время
  timer = millis(); // Обновляем таймер

  // ----------- ХЛОР (ADS2, канал A1) -----------
//    if (!ads2.begin(0x49)) return; // если I2C не отвечает — выходим

  if (!isI2CDeviceReady(ads2Address)) { //if (!readAdsSingleEndedSafe(ads2, ads2Address, 1, rawCL)) {
      // Если АЦП недоступен/завис — сбрасываем накопление и выходим,
      // чтобы не держать старые данные и не блокировать основной loop().
      countCL = 0;
      totalCL = 0;
      return;
  }


    totalCL += ads2.readADC_SingleEnded(1);  // Считываем значение с ADS2 (канал 1)
    countCL++;                              // Увеличиваем счётчик

  if (countCL < samples) return;           // Ждём, пока не наберётся 3 выборки

  // Усреднение и перевод в милливольты
  analogValueCL = (float(totalCL) / samples) * 0.1875; //(totalCL / samples) * 0.1875;

  // Сброс суммирующих переменных
  countCL = 0;
  totalCL = 0;

    // // --- Проверяем корректность данных ---
    // if (isnan(analogValueCL) || analogValueCL <= 0 || analogValueCL > 32000) {
    //     jee.var("Cl", "Ошибка датчика");
    //     return; // Пропускаем расчёт, если данные неверны
    // }

  // Температура из датчика DS18B20
  float temperature = DS1;

  // Коррекция ORP по температуре воды (мВ)
  corr_ORP_temper_mV = correctValueByTemperature(analogValueCL, temperature);

  // Применение калибровочного смещения (мВ)
  corrected_ORP_Eh_mV = calibrateSensor(corr_ORP_temper_mV);

    // Расчёт ppm Cl по ORP и pH
    ppmCl = interpolateClValue(PH, corrected_ORP_Eh_mV);
    //ppmCl = getClFromORP(PH, corrected_ORP_Eh_mV);
    

      // Ограничение
  if (ppmCl < 0.001) ppmCl = 0.001;
  if (ppmCl > 10.0) ppmCl = 10.0;


  // Отображение в интерфейсе
  // Calibration/status UI updates are handled via the dashboard JSON now.
}


    //Косвенное определение ОВП:
                    // //ОВП и рН действительно связаны между собой по формуле rH2 = Eh/0,029 + 2 pH. 
                    // //ОВП зависит не только от рН, но и от равновесного окислительно-восстановительный потенциала 
                    // //в текущих условиях - Eh, который в свою очередь нелинейно зависит от рН
                    // //https://dzagigrow.ru/blog/zhestkost-i-okislitelno-vosstanovitelnyy-potentsial-vody-kak-primenit-v-rastenievodstve/#:~:text=%D0%9E%D0%92%D0%9F%20%D0%B8%20%D1%80%D0%9D%20%D0%B4%D0%B5%D0%B9%D1%81%D1%82%D0%B2%D0%B8%D1%82%D0%B5%D0%BB%D1%8C%D0%BD%D0%BE%20%D1%81%D0%B2%D1%8F%D0%B7%D0%B0%D0%BD%D1%8B,%D0%BE%D1%87%D0%B5%D1%80%D0%B5%D0%B4%D1%8C%20%D0%BD%D0%B5%D0%BB%D0%B8%D0%BD%D0%B5%D0%B9%D0%BD%D0%BE%20%D0%B7%D0%B0%D0%B2%D0%B8%D1%81%D0%B8%D1%82%20%D0%BE%D1%82%20%D1%80%D0%9D.

                    // //Примерное значение rH2 (можно корректировать по необходимости)
                    // //rH2 = Eh/0,029 + 2 pH (или redox potential) является безразмерным показателем, который отражает совокупное окислительно-восстановительное состояние среды.
                    // float rH2 = 25.0; // Это значение зависит от состава воды и должно быть уточнено

                    // // Вычисление Eh на основе формулы rH2 = Eh/0.029 + 2 * pH
                    // float Eh = (rH2 - 2 * PH) * 0.029; // Преобразуем формулу для вычисления Eh
                    // float Eh_mV = Eh * 1000;// Преобразование Eh ОВП в милливольты

                    // //Eh представляет собой ОВП окислительно-восстановительный потенциал воды и измеряется в вольтах (или милливольтах). 
                    // //Этот потенциал отражает способность среды окислять или восстанавливать вещества.
                    //  jee.var("Eh_ORP", String(Eh_mV) + " mV"); 
                    // //Расчет концентрации хлора (Cl2) в миллиграммах на литр (mg/L)
                    // float k_chlorine = 0.1; // Эмпирический коэффициент для хлора - базовый множитель в формуле для расчета концентрации хлора
                    // float b_chlorine = 0.05; // Эмпирический коэффициент - определяет чувствительность концентрации хлора к изменениям pH
                    // chlorineConcentration = k_chlorine * pow(10, (Eh - b_chlorine * PH));
                    // jee.var("CL2_Kosv", String(chlorineConcentration, 2) + " млг/литр"); // Косвенное содержание хлора CL2 млг/литр, норма: 3млг/литр
