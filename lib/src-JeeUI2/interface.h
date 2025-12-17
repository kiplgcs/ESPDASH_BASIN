bool Slep = false; //Флаг режима сна
int Lumen_Ul, Saved_Lumen_Ul; //освещенность на улице
int Lumen_Ul_percent; //Освещенность в процентах

float PH, Saved_PH; //Кислотность воды
float  PH1, PH2;					//Точки калибровки PH
float  PH1_CAL, PH2_CAL; 	//Значения для данных точек калибровки PH от 0 до 4095 (12бит)
float Temper_PH; //Измеренная тепература для компенасации измерения PH
float Temper_Reference; // Температура при котором сенсор выдает свои параметры - 20.0 или 25.0 С для разных сенсоров PH;
bool Act_PH = false; //Активация калибровки
bool Act_Cl = false; //Активация калибровки
int analogValuePH_Comp; //корректированное значение АЦП после компенсации по температуре

int Saved_gmtOffset_correct, gmtOffset_correct; // Корректировка часового пояса призапросен времени из Интернета

//bool RestartESP32 = false; //флаг перезагрузки ESP32 по нажатию кнопки
bool CalCl = false; //флаг кнопки для запоминания значения калибровки по ОВП
unsigned long Timer_Callback; // Таймер опроса всех кнопок

/************************* Переменные для записи значений полученных по MQTT*******/

bool Lamp, Lamp1;		// Подсветка в бассейне -  Включение в ручную
bool Lamp_autosvet, Saved_Lamp_autosvet;
bool Power_Time1, Saved_Power_Time1;
String Lamp_timeON1, Lamp_timeOFF1; // Утавки времени включения-отключения LED ленты
String Saved_Lamp_timeON1, Saved_Lamp_timeOFF1;


bool Pow_WS2815, Pow_WS28151;		// Включение в ручную
bool Pow_WS2815_autosvet, Saved_Pow_WS2815_autosvet; 
bool WS2815_Time1, Saved_WS2815_Time1;
String timeON_WS2815, timeOFF_WS2815; // Утавки времени включения-отключения LED ленты
String Saved_timeON_WS2815, Saved_timeOFF_WS2815;
bool ColorRGB=false, Saved_ColorRGB=false;    //режим ручного задания цвета
int new_bright, new_bright1 = 255; //переменная с яркостью установленной в интерфейсе вручную
bool autoplay=true, autoplay1; // Автоматическая смена режимов работы LED
long long number, number1; //Цвет ленты
long long  r; 
long long  g; 
long long  b;
int autoplayDuration=30, autoplayDuration1; // Продолжительность автовоспроизведения программ
bool RANDOMNO, RANDOMNO1; // активация рандомного режима программ и яркости
int currentPatternIndex; //Номер цветовой программы в реальном времени

bool Index0,Index1,Index2,Index3,Index4,Index5,Index6,Index7,Index8,Index9,Index10,
Index11,Index12,Index13,Index14,Index15,Index16,Index17,Index18,Index19,Index20,
Index21,Index22,Index23,Index24;

	
bool Power_Filtr, Power_Filtr1;		// Фильтрация в бассейне -  Включение в ручную
bool Filtr_Time1, Filtr_Time2, Filtr_Time3; // Разрешения работы включения по времени
bool Saved_Filtr_Time1, Saved_Filtr_Time2, Saved_Filtr_Time3;
String Filtr_timeON1, Filtr_timeOFF1, Filtr_timeON2, Filtr_timeOFF2, Filtr_timeON3, Filtr_timeOFF3; // Утавки времени включения
String Saved_Filtr_timeON1, Saved_Filtr_timeOFF1, Saved_Filtr_timeON2, Saved_Filtr_timeOFF2, Saved_Filtr_timeON3, Saved_Filtr_timeOFF3; 

bool Power_Clean, Power_Clean1; // Промывка фильтра
bool Clean_Time1, Saved_Clean_Time1; // Разрешения работы включения по времени
String Clean_timeON1, Clean_timeOFF1; // Утавки времени включения
String Saved_Clean_timeON1, Saved_Clean_timeOFF1;
bool chk1, chk2, chk3, chk4, chk5, chk6, chk7; //Дни недели ПН, ВТ, СР, ЧТ, ПТ, СБ, ВС - для включения таймера в нужные дни
bool Saved_chk1, Saved_chk2, Saved_chk3, Saved_chk4, Saved_chk5, Saved_chk6, Saved_chk7;

bool Power_Heat, Power_Heat1;
bool Activation_Heat, Activation_Heat1; // Включение и Активация контроля включения нагрева воды
int Sider_heat,  Sider_heat1; 			// Sider_heat1; bool Sider_Heat; // Переменная для получения или передачи из в  Nextion c  сидера монитора уставки нагрева воды
//float temper1; int set_temper1;  //Для котроля измениения
//bool Relay4_Time1, Relay4_Time2; // Разрешения работы включения по времени
//String Relay4_timeON1, Relay4_timeOFF1, Relay4_timeON2, Relay4_timeOFF2; // Утавки времени включения
	
bool Pow_Ul_light, Pow_Ul_light1; // Промывка фильтра
bool Ul_light_Time, Saved_Ul_light_Time; // Разрешения работы включения по времени
String Ul_light_timeON, Ul_light_timeOFF; // Утавки времени включения
// String Saved_Ul_light_timeON, Saved_Ul_light_timeOFF;

bool Power_Topping, Power_Topping1; // Долив воды по уровню


bool Saved_Power_H2O, Power_H2O2 = false; //Дозация перекеси водороода 
bool Saved_Power_ACO, Power_ACO = false; 	//Дозация Активное Каталитическое Окисление «Active Catalytic Oxidation» ACO
//bool Saved_Power_APF, Power_APF = false;		//Высокоэффективный коагулянт и флокулянт «All Poly Floc» APF

// bool Saved_Test_Pump, Test_Pump; //Разрешение на работу тамера дозации ACO
// bool Saved_Activation_Timer_H2O2, Activation_Timer_H2O2;//Разрешение на работу тамера дозации H2O2
//bool Saved_Activation_Timer_APF, Activation_Timer_APF; //Разрешение на работу тамера дозации APF

//String Info_H2O2_H2O2_APF;  //Таймера включения дозаторов  H2O2, H2O2, APF

// String Saved_Timer_H2O2_Start, Timer_H2O2_Start;
// String Saved_Timer_H2O2_Work, Timer_H2O2_Work;

// String Saved_Timer_ACO_Start, Timer_ACO_Start;
// String Saved_Timer_ACO_Work, Timer_ACO_Work;

bool PH_Control_ACO, Saved_PHControlACO; // Флаг для отслеживания предыдущего состояния PH_Control_ACO
int ACO_Work, Saved_ACO_Work;


bool NaOCl_H2O2_Control, Saved_NaOCl_H2O2_Control;
int H2O2_Work, Saved_H2O2_Work;

int corr_ORP_temper_mV; 		// ОРП с учетом калибровки по температуре
int CalRastvor256mV	= 256;	//Калибровочный раствор
int Calibration_ORP_mV = 0; //Калибровочный кооффициент - разница между колибровочным раствором 256mV 25C	 и показаниями сенсора
int corrected_ORP_Eh_mV;		// ОРП с учетом калибровки по  калибровочному раствору 	
float Saved_ppmCl, ppmCl = 1.3; //Свободный Хлор CL2 -  млг/литр, норма: 1,3млг/литр


//Строки для хранения информации о таймерах
char Info_H2O2[50]; String Saved_Info_H2O2;
char Info_ACO[50];  String Saved_Info_ACO;

// ===== Настройки пределов pH =====
float PH_setting; // = 7.2;  // Верхний предел для включения дозирования


// ===== Настройки пределов ORP =====
int ORP_setting; // = 500;   // Нижний предел для включения дозирования

//Получение по ModbusRTU Объявляем переменные Input1, ..., Input16
//bool Input1, Input2, Input3, Input4, Input5, Input6, Input7, Input8, Input9, Input10, Input11, Input12, Input13, Input14, Input15, Input16;



void parameters(){

	jee.var("Act_PH", "false"); // Деактивируем после перезагрузки калибровку PH
	jee.var("Act_Cl", "false"); // Деактивируем после перезагрузки калибровку 
	

	if(String(Sider_heat) == F("")||String(Sider_heat) == F("null")|| Sider_heat < 5 || Sider_heat > 30)  { jee.var(F("Sider_heat"), "25"); Sider_heat=25;
	} else {
	Sider_heat1 = Sider_heat; Activation_Heat1=Activation_Heat; jee.var("Activation_Heat", String(Activation_Heat));}


	jee.var("new_bright", "255"); // Яркость
	jee.var("autoplay", "true");
	jee.var("autoplayDuration", "30"); // таймер смены режима
	jee.var("ColorLED", "#ff1bb9"); //Цвет по умолчанию
	jee.var("ColorRGB", "false"); //Отключаем выбор цвета
	jee.var("RANDOMNO", "true");

	//Если все режимы отключены, то включаем хотябы несколько, чтобы обеспечить работу ленты
	if(!Index0&&!Index1&&!Index2&&!Index3&&!Index4&&!Index5&&!Index6&&!Index7&&!Index8&&!Index9&&!Index10&&
	!Index11&&!Index12&&!Index13&&!Index14&&!Index15&&!Index16&&!Index17&&!Index18&&!Index19&&!Index20&&!Index21&&!Index22&&!Index23&&!Index24){
		jee.var("Index0", "true");
	 	jee.var("Index1", "true");
	 	jee.var("Index2", "true");
	 	jee.var("Index3", "true");
	 	jee.var("Index4", "true");
	 	jee.var("Index5", "true");
	 	jee.var("Index6", "true");
		jee.var("Index7", "true");
		jee.var("Index8", "true");
		jee.var("Index9", "true");
		jee.var("Index10", "true");
		jee.var("Index11", "true");
		jee.var("Index12", "true");
		jee.var("Index13", "true");
		jee.var("Index14", "true");
		jee.var("Index15", "true");
		jee.var("Index16", "true");
		jee.var("Index17", "true");
		jee.var("Index18", "true");
		jee.var("Index19", "true");
		jee.var("Index20", "true");
		jee.var("Index21", "true");
	 	jee.var("Index22", "true");
	 	jee.var("Index23", "true");
		jee.var("Index24", "true");
	}

	// Power_Filtr=Power_Filtr1 = jee.param(F("Power_Filtr")); // насос - Приравниваем, чтобы после перезагрузки не было случаного запуска насоса фильтра

	// Power_Time1 = Saved_Power_Time1 = jee.param(F("Power_Time1")); // - таймер подсветки -  переписывкм данные из памяти в переменные - пробно чтобы не отваливался тамер ...


	if(String(PH1) == F("")||String(PH1) == F("null"))      {jee.var(F("PH1"), "4.1"); PH1 = 4.1;}
	if(String(PH2) == F("")||String(PH2) == F("null"))      {jee.var(F("PH1"), "6.86"); PH2 = 6.86;}
	if(String(PH1_CAL) == F("")||String(PH1_CAL) == F("null"))      {jee.var(F("PH1_CAL"), "3471"); PH1_CAL = 3471;} //jee.var(F("PH1_CAL"), "3471"); PH1_CAL = 3471;
	if(String(PH2_CAL) == F("")||String(PH2_CAL) == F("null"))      {jee.var(F("PH2_CAL"), "2948"); PH2_CAL = 2948;} //jee.var(F("PH2_CAL"), "3125"); PH2_CAL = 3125;
	if(String(Temper_PH) == F("")||String(Temper_PH) == F("null"))      {jee.var(F("Temper_PH"), "20"); Temper_PH=20.00;}
	if(String(Temper_Reference) == F("")||String(Temper_Reference) == F("null"))      {jee.var(F("Temper_Reference"), "20.00"); Temper_Reference=20.00;}

	if(String(H2O2_Work) == F("")||String(H2O2_Work) == F("null"))      {jee.var(F("H2O2_Work"), "7"); H2O2_Work = 7;} // Таймер дозтирования кислоты
	if(String(ACO_Work) == F("")||String(ACO_Work) == F("null"))      {jee.var(F("ACO_Work"), "7"); ACO_Work = 7;} // Таймер дозтирования хлора

	if(String(Calibration_ORP_mV) == F("")||String(Calibration_ORP_mV) == F("null"))    {jee.var(F("Calibration_ORP_mV"), "0"); Calibration_ORP_mV = 0;}
	

	if(String(PH_setting) == F("")||String(PH_setting) == F("null"))      {jee.var(F("PH_Upper"), "7.2"); PH_setting = 7.2;}

	if(String(ORP_setting) == F("")||String(ORP_setting) == F("null"))      {jee.var(F("ORP_Upper"), "500"); ORP_setting = 500;}


}




void interface(){

  // // Проверяем аутентификацию перед отображением других страниц
  // 	if (!authenticated) {
		

	// 	jee.app("Аутентификация"); // название программы (отображается в веб интерфейсе)
	// 	jee.menu("Введите пароль доступа");

  //   // Если не аутентифицированы, отображаем страницу ввода пароля
  //   jee.page();
  //   //jee.text("password_input", "Введите пароль:");
  //   jee.password("enteredPassword", "Пароль");
  //   // Кнопка для отправки пароля на проверку
  //   //jee.button(F("begin"), F("gray"), "Войти");
   		
	// 	jee.page(); //Закрываем страницы
		
	// 	return;  // Завершаем выполнение функции, чтобы не отображать другие страницы

  // } else {




	jee.app("Система управления бассейном"); // название программы (отображается в веб интерфейсе)
	jee.menu("Управление подсветкой");
	jee.menu("Управление RGB подстветкой WS2815");
	jee.menu("Настройка фильтрации");
	jee.menu("Промывка фильтра");
	jee.menu("Контроль температуры");
	jee.menu("Долив воды");
	jee.menu("Контроль PH (NaOCl)");
	jee.menu("Контроль хлора CL (ACO)");
	jee.menu("Уличное освещение");
	jee.menu("Настройки Nextion");
	jee.menu("Настройки MQTT");
  jee.menu("Настройки Wi-Fi");

jee.page(); //Управление подсветкой - лампами
 	
		
	//jee.number("temp_in", "Интервал замера PZEM004T (U/I/W/E)");
  // jee.range("temp_in", -30, +45, 0.5, "Темпереатура внутри"); //добавляем в web интерфейс ползунок - "range" с фргументами (значение, мин.порог, макс.порог, шаг, "название")
  //jee.range("his_in", 0, 5, 0.1, "Гистерезис");

  //jee.color("Color1", "Цвет"); //Панель выбора цвета
 

  // jee.checkbox("chk1", "Чекбокс");                        //Выбор состояния true или false
  // jee.password("pswd1", "Пароль");              //Поле ввода пароля    
  // jee.number("num1", "цифры");                  //Поле ввода цифр
  // jee.time("time1", "Время");
  // jee.email("email1", "Почта"); 
  // jee.range("temp_in", -10, +10, 0.2, "Ползунок");

		//jee.pub("Data", "Время:::", " ", "#080808", "#f2f2f0");
		
		if(!Act_PH && !Act_Cl) {jee.pub("Time", "", " ", "#383269", "#02c8fa");}
		
		
		jee.checkbox("Lamp", "Lamp"); // Свет в бассейне
		jee.checkbox("Lamp_autosvet", "Lamp_autosvet"); // Автосвет в бассейне
		jee.text("Lumen_Ul",  "Освещенность %");
		
		

	
		jee.checkbox("Power_Time1", "Power_Time1"); 
		jee.time("Lamp_timeON1", "Время_Вкл1");
		jee.time("Lamp_timeOFF1", "Время_Выкл1");


		

		// jee.checkbox("Power_Time2", "Power_Time2"); 
		// jee.time("Lamp_timeON2", "Время_Вкл2");
		// jee.time("Lamp_timeOFF2", "Время_Выкл2");
	
jee.page(); //Управление RGB подстветкой - лентой WS2815

		jee.checkbox("Pow_WS2815", "Pow_WS2815"); 

		jee.checkbox("Pow_WS2815_autosvet", "Pow_WS2815_autosvet");
		jee.text("Lumen_Ul",  "Освещенность %");

		jee.checkbox("WS2815_Time1", "WS2815_Time1"); 
		jee.time("timeON_WS2815", "Время_Вкл1");
		jee.time("timeOFF_WS2815", "Время_Выкл1");


		jee.checkbox("ColorRGB", "Задать цвет");  //Вкл. разрешения задавать цвет в ручную
		jee.color("ColorLED", "Выбрать цвет"); //Панель выбора цвета
		
		jee.range("new_bright", 5, 255, 1, "Яркость свечения"); // ползунок с заданными минимальными и максимальными значениями и шагом
		jee.checkbox("autoplay", "AUTOPLAY:"); 
		jee.checkbox("RANDOMNO", "RANDOMNO"); // активация рандомного режима программ и яркости
		jee.range("autoplayDuration", 5, 60, 1, "Через сколько секунд менять режим:");
		
		jee.checkbox("Index0", "Index0");jee.checkbox("Index1", "Index1");jee.checkbox("Index2", "Index2");jee.checkbox("Index3", "Index3");jee.checkbox("Index4", "Index4");
		jee.checkbox("Index5", "Index5");	jee.checkbox("Index6", "Index6");	jee.checkbox("Index7", "Index7");jee.checkbox("Index8", "Index8");jee.checkbox("Index9", "Index9");
		jee.checkbox("Index10", "Index10");jee.checkbox("Index11", "Index11");jee.checkbox("Index12", "Index12");jee.checkbox("Index13", "Index13");	jee.checkbox("Index14", "Index14");
		jee.checkbox("Index15", "Index15");jee.checkbox("Index16", "Index16");jee.checkbox("Index17", "Index17");	jee.checkbox("Index18", "Index18");jee.checkbox("Index19", "Index19");
		jee.checkbox("Index20", "Index20");	jee.checkbox("Index21", "Index21");jee.checkbox("Index22", "Index22");jee.checkbox("Index23", "Index23");jee.checkbox("Index24", "Index24");
	
		
		
jee.page(); //Фильтр


		//jee.pub("Temp_DS_18B20", "Temp_DS_18B20", " ", "#6060ff", "#f5f5f5");

		

		jee.checkbox("Power_Filtr", "Power_Filtr"); //jee.checkbox("Filtr", "Filtr"); 

		//jee.checkbox("DOZHD_OFF_POLIV", "DOZHD");

		//jee.text("GET_API",  "GET_API");


		jee.checkbox("Filtr_Time1", "Filtr_Time1"); 
		jee.time("Filtr_timeON1", "Время_Вкл1");
		jee.time("Filtr_timeOFF1", "Время_Выкл1");

		jee.checkbox("Filtr_Time2", "Filtr_Time2"); 
		jee.time("Filtr_timeON2", "Время_Вкл2");
		jee.time("Filtr_timeOFF2", "Время_Выкл2");

		jee.checkbox("Filtr_Time3", "Filtr_Time3"); 
		jee.time("Filtr_timeON3", "Время_Вкл3");
		jee.time("Filtr_timeOFF3", "Время_Выкл3");


jee.page();	//Промывка фильтра

		jee.checkbox("Power_Clean", "Power_Clean"); 

		// jee.checkbox("DOZHD_OFF_POLIV", "DOZHD");
		// jee.text("GET_API",  "GET_API");


		jee.checkbox("Clean_Time1", "Clean_Time1"); 

		jee.time("Clean_timeON1", "Время_Вкл1");
		jee.time("Clean_timeOFF1", "Время_Выкл1");

		jee.checkbox("chk1", "Понедельник");
		jee.checkbox("chk2", "Вторник");
		jee.checkbox("chk3", "Среда"); 
		jee.checkbox("chk4", "Четверг"); 
		jee.checkbox("chk5", "Пятница"); 
		jee.checkbox("chk6", "Суббота"); 
		jee.checkbox("chk7", "Воскресенье");


		

jee.page();	//Контроль температуры

		
	jee.checkbox("Activation_Heat", "Activation_Heat");

	if(!Act_PH && !Act_Cl) {jee.pub("Heat","T_inp, T_out, T_nos","", "#383269", "#02c8fa");}

	jee.range("Sider_heat", 5, 30, 1, "Температура нагрева воды в бассейне"); // ползунок с заданными минимальными и максимальными значениями и шагом

	jee.text("DS1",  "Температура в бассейне  - перед нагревателем");
	jee.text("DS2",  "Температура после нагревателя - падача нагретой воды в бассейн");
	

	//jee.checkbox("DOZHD_OFF_POLIV", "DOZHD");
	//jee.text("GET_API",  "GET_API");


	//jee.checkbox("Relay4_Time1", "Relay4_Time1"); 

	//jee.time("Relay4_timeON1", "Время_Вкл1");
	//jee.time("Relay4_timeOFF1", "Время_Выкл1");

	//jee.checkbox("Relay4_Time2", "Relay4_Time2"); 

	//jee.time("Relay4_timeON2", "Время_Вкл2");
	//jee.time("Relay4_timeOFF2", "Время_Выкл2");

jee.page();	//Долив воды

jee.checkbox("Power_Topping", "Power_Topping"); 

jee.page(); //Химия (H2O2, ACO, APF)

	//jee.pub("Timer_H2O2_H2O2_APF", "Timer Start/Work-сек.: H2O2, ACO", "", "#383269", "#02c8fa"); //Таймера включения дозаторов  H2O2, H2O2, APF

	//jee.pub("Timer_H2O2_H2O2_APF", "", "", "#383269", "#02c8fa"); //Таймера включения дозаторов  H2O2, H2O2, APF
	//jee.text("Info_H2O2_H2O2_APF",  "Timer H2O2, ACO");

	// jee.datetime("XXXX",  "НННН");
	// jee.date("XXXX",  "НННН");

	

	
	
	if(Act_PH) {
	jee.pub("PH_CAL", "PH1-PH2 : АЦП0-5000mV", "", "#383269", "#02c8fa");
	jee.text("PH1",  "Min_CAL PH1:4.1:::" +  String(PH1));
	jee.text("PH2",  "Max_CAL PH2:6.86::: " + String(PH2));
	jee.text("PH1_CAL",  "АЦП_mV для PH1: " + String(PH1_CAL));
	jee.text("PH2_CAL",  "АЦП_mV для PH2: " + String(PH2_CAL));
	jee.text("Temper_Reference",  "Темп. референсная: " +  String(Temper_Reference));
	jee.text("Temper_PH",  "Темп. компенс. PH: " + String(Temper_PH)); //Измеренная тепература для компенасации измерения PH
	} else {

	jee.text("PH",  "РH—показатель кислотно-щелочных свойств воды, 4:кисл. / норма:7-7,4 / 14:щелочь");
	jee.checkbox("PH_Control_ACO", "PH.контроль");

	// 	// ========================= Динамические значения PH/ORP =========================
	// jee.text("PH_Current",  "PH: 0.00");    // <- здесь динамически обновляем
	// jee.text("ORP_Current", "ORP: 0 mV");   // <- здесь динамически обновляем
	// jee.checkbox("Pump_ACO", "Насос ACO");  // <- можно динамически включать/выключать
	
	//jee.checkbox("Power_ACO", "Power_ACO"); 	//Дозация Активное Каталитическое Окисление «Active Catalytic Oxidation» ACO
	
	jee.option("3","работать 5 секунд через 5 минут - 2.46 мл/мин или 0,15 л/час");
	jee.option("1","работать 5 секунд через 15 секунд - 37.5 мл/мин или 2,2 л/час");
	jee.option("2","работать 5 секунд через 60 секунд - 11.54 мл/мин или 0,7 л/час");
	jee.option("4","работать 5 секунд через 15 минут - 1.238 мл/мин или 0.07428 л/час");
	jee.option("5","работать 5 секунд через 30 минут - 0.412 мл/мин или 0.02472 л/час ");
	jee.option("6","работать 5 секунд через 1 час - 0.213 мл/мин или 0.01278 л/час ");
	jee.option("7","работать 5 секунд через 24 часа - 0.213 мл/мин или 0.01278 л/день ");
  jee.select("ACO_Work","Режим пожачи окислителя ACO"); //Выбор значений - "Select"

	//jee.checkbox("Test_Pump_PH", "Тест_Насос"); //ACO (Advanced Copper Oxidizer) — это продвинутый окислитель на основе меди
	}

	jee.checkbox("Act_PH", "КалибровкаPH"); //Активация калибровки

	jee.text("PH_setting",  "Уставка PH  - 7.2"); 
	
	
jee.page(); //Химия Контроль хлора


	
	
	
	if(Act_Cl) {
		jee.pub("Cl_Cal", "ORP - ORPCal = калибровочный коэффициент", "", "#383269", "#02c8fa");
		
		jee.text("CalRastvor256mV",  "ОВП калибровочного раствора - мВ");
		
		jee.text("Calibration_ORP_mV",  "Калибровочный коэффициент - мВ");

		jee.button("CalCl", "ff00ff", "Калибровать ОВП по раствору");


	} else {

	jee.text("Cl",  "ОВП:(норма 740мВ - pH=7.2 - Cl: 1.3млг/л)");

	
	
	
	// jee.text("Eh_ORP",  "Косв. ОВП (по PH)  - окислительно-восстановительный потенциал,(норма > 650 мВ)");
	// jee.text("CL2_Kosv",  "Косв. хлор CL2 (по PH) -  млг/литр, норма: 3млг/литр");
	
	jee.checkbox("NaOCl_H2O2_Control", "Хлор.контрль");
	
	//jee.checkbox("Power_H2O2", "Power_H2O2"); //Дозация перекеси водороода
	
	jee.option("3","работать 5 секунд через 5 минут - 2.46 мл/мин или 0,15 л/час");
	jee.option("1","работать 5 секунд через 15 секунд - 37.5 мл/мин или 2,2 л/час");
	jee.option("2","работать 5 секунд через 60 секунд - 11.54 мл/мин или 0,7 л/час");
	jee.option("4","работать 5 секунд через 15 минут - 1.238 мл/мин или 0.07428 л/час");
	jee.option("5","работать 5 секунд через 30 минут - 0.412 мл/мин или 0.02472 л/час ");
	jee.option("6","работать 5 секунд через 1 час - 0.213 мл/мин или 0.01278 л/час ");
	jee.option("7","работать 5 секунд через 24 часа - 0.213 мл/мин или 0.01278 л/день ");
  jee.select("H2O2_Work","Режим пожачи хлора - Гипохлорита Натрия (NaOCl)"); //Выбор значений - "Select"

	//jee.checkbox("Activation_Timer_H2O2", "Насос_Cl"); //Дозация перекеси водороода или Гипохлорит натрия (NaOCl) "Activ_H2O2_NaOCl"
	}
	
	jee.checkbox("Act_Cl", "КалибровкаCl"); //Активация калибровки


	jee.text("ORP_setting",  "Уставка Хлора  - 500 mV"); 
jee.page();
//Уличное освещение

		jee.checkbox("Pow_Ul_light", "Pow_Ul_light"); //jee.checkbox("Filtr", "Filtr"); 
		jee.checkbox("Ul_light_Time", "Ul_light_Time"); 
		jee.time("Ul_light_timeON", "Время_Вкл");
		jee.time("Ul_light_timeOFF", "Время_Выкл");




jee.page(); //Nextion
		
	//jee.number("gmtOffset_correct", "Часовой пояс"); // Наверное не надо отправлять из ESP32 в Nextion часовой пояс - только принимать
		
	jee.textarea("info",  "Инструкция:");
	
  // text(F("m_host"), F("MQTT host"));
  // number(F("m_port"), F("MQTT port"));
  // text(F("m_user"), F("User"));
  // text(F("m_pass"), F("Password"));
  // button(F("bWF"), F("gray"), F("Reconnect"));

jee.page(); //MQTT

	jee.formMqtt();


jee.page(); //Wi-Fi

	// jee.formWifi();

	jee.text(F("ssid1"), F("SSID"));
  jee.password(F("pass1"), F("Password"));
	jee.button("ConnectWiFi", "b38bff", "Подключиться к WiFi");

	jee.text(F("ap_ssid1"), F("AP_SSID"));
  jee.password(F("ap_pass1"), F("Password"));
	
	jee.button("ConnectAP", "b38bff", "Перейти в режим точки доступа");

	jee.text("RSSI_WiFi",  "RSSI_WiFi");
	//booootn(F("bWF"), F("blue"), F("Connect"));


	jee.button("RestartESP32", "ff00ff", "Рестарт ESP32");

jee.page(); //Закрываем страницы
}//}







void update(){ //Присвоеваем значения введенные в интерфейсе в переменные программы, чтобы постоянно не читать в цикле из EEPROM

	
	Lamp = jee.param("Lamp") == "true";
	Lamp_autosvet = jee.param("Lamp_autosvet") == "true";
	Power_Time1 = jee.param("Power_Time1") == "true"; 
	Lamp_timeON1 = jee.param("Lamp_timeON1");
	Lamp_timeOFF1 = jee.param("Lamp_timeOFF1");

	Pow_WS2815 = jee.param("Pow_WS2815") == "true";
	Pow_WS2815_autosvet = jee.param("Pow_WS2815_autosvet") == "true";
	WS2815_Time1 = jee.param("WS2815_Time1") == "true"; 
	timeON_WS2815 = jee.param("timeON_WS2815");
	timeOFF_WS2815 = jee.param("timeOFF_WS2815");
	ColorRGB = jee.param("ColorRGB") == "true"; //Разрешение за задание цвета - конвертируем текст в bolean 
	new_bright = jee.param("new_bright").toInt(); // Яркость RGB ленты - конвертируем текст в iteger
  number = strtoll( &jee.param("ColorLED")[1], NULL, 16); //Избавляемся от первого символа # строки #e31c57
	autoplay = jee.param("autoplay") == "true";
	autoplayDuration = jee.param("autoplayDuration").toInt();
	RANDOMNO = jee.param("RANDOMNO") == "true";
	Index0 = jee.param("Index0") == "true";  Index1 = jee.param("Index1") == "true"; Index2 = jee.param("Index2") == "true"; Index3 = jee.param("Index3") == "true"; Index4 = jee.param("Index4") == "true"; Index5 = jee.param("Index5") == "true"; 
	Index6 = jee.param("Index6") == "true"; Index7 = jee.param("Index7") == "true"; Index8 = jee.param("Index8") == "true"; Index9 = jee.param("Index9") == "true"; Index10 = jee.param("Index10") == "true"; 
	Index11 = jee.param("Index11") == "true"; Index12 = jee.param("Index12") == "true"; Index13 = jee.param("Index13") == "true"; Index14 = jee.param("Index14") == "true"; Index15 = jee.param("Index15") == "true"; 
	Index16 = jee.param("Index16") == "true"; Index17 = jee.param("Index17") == "true"; Index18 = jee.param("Index18") == "true"; Index19 = jee.param("Index19") == "true"; Index20 = jee.param("Index20") == "true"; 
	Index21 = jee.param("Index21") == "true"; Index22 = jee.param("Index22") == "true"; Index23 = jee.param("Index23") == "true"; Index24 = jee.param("Index24") == "true"; 
	


	//DOZHD = jee.param("DOZHD") == "true"; // Активация запрета полив по прогнозу погоды  - если определено что дождь

	Power_Filtr = jee.param("Power_Filtr") == "true";  //Фильтрация
	Filtr_Time1 = jee.param("Filtr_Time1") == "true"; 
	Filtr_timeON1 = jee.param("Filtr_timeON1");
	Filtr_timeOFF1 = jee.param("Filtr_timeOFF1");
	Filtr_Time2 = jee.param("Filtr_Time2") == "true"; 
	Filtr_timeON2 = jee.param("Filtr_timeON2");
	Filtr_timeOFF2 = jee.param("Filtr_timeOFF2");
	Filtr_Time3 = jee.param("Filtr_Time3") == "true"; 
	Filtr_timeON3 = jee.param("Filtr_timeON3");
	Filtr_timeOFF3 = jee.param("Filtr_timeOFF3");
	

	Pow_Ul_light = jee.param("Pow_Ul_light") == "true";  //Освещение
	Ul_light_Time = jee.param("Ul_light_Time") == "true";
	Ul_light_timeON = jee.param("Ul_light_timeON");
	Ul_light_timeOFF = jee.param("Ul_light_timeOFF");

	
	Power_Clean = jee.param("Power_Clean") == "true"; //Очистка фильтра
	Clean_Time1 = jee.param("Clean_Time1") == "true"; 
	Clean_timeON1 = jee.param("Clean_timeON1");
	Clean_timeOFF1 = jee.param("Clean_timeOFF1");

	chk1 = jee.param("chk1") == "true"; //ПН
	chk2 = jee.param("chk2") == "true"; //ВТ
	chk3 = jee.param("chk3") == "true"; //СР
	chk4 = jee.param("chk4") == "true"; //ЧТ
	chk5 = jee.param("chk5") == "true"; //ПТ
	chk6 = jee.param("chk6") == "true"; //СБ
	chk7 = jee.param("chk7") == "true"; //ВС


	Activation_Heat = jee.param("Activation_Heat") == "true";
	Power_Heat = jee.param("Power_Heat") == "true"; //Нагрев воды
	Sider_heat = jee.param("Sider_heat").toInt(); 	//уставка нагрева воды


 	Power_Topping = jee.param("Power_Topping") == "true"; //Долив воды в бассейн



	PH1 = jee.param("PH1").toFloat();
	PH2 = jee.param("PH2").toFloat();
	PH1_CAL = jee.param("PH1_CAL").toFloat();
	PH2_CAL = jee.param("PH2_CAL").toFloat();
	Temper_PH = jee.param("Temper_PH").toFloat();
	Temper_Reference = jee.param("Temper_Reference").toFloat();
	Act_PH = jee.param("Act_PH") == "true";
	Act_Cl = jee.param("Act_Cl") == "true";
	
	
	//Power_H2O2 = jee.param("Power_H2O2") == "true"; //Дозация перекеси водороода 
	NaOCl_H2O2_Control = jee.param("NaOCl_H2O2_Control") == "true"; //Хлор контроль - автоматическое включение дозации хлора по датчику
	//Activation_Timer_H2O2 = jee.param("Activation_Timer_H2O2") == "true"; //Дозация перекеси водороода или хлора
	H2O2_Work= jee.param("H2O2_Work").toInt();
	// Timer_H2O2_Start = jee.param("Timer_H2O2_Start");
	// Timer_H2O2_Work = jee.param("Timer_H2O2_Work");

	CalRastvor256mV = jee.param("CalRastvor256mV").toInt(); // ОВП калибровочного раствора
	Calibration_ORP_mV = jee.param("Calibration_ORP_mV").toInt(); //Калибровочный кооффициент - разница между колибровочным раствором 256mV 25C	 и показаниями сенсора
	CalCl = jee.param("CalCl") == "true"; 


	//Power_ACO = jee.param("Power_ACO") == "true"; //Дозация Активное Каталитическое Окисление «Active Catalytic Oxidation» ACO
	PH_Control_ACO = jee.param("PH_Control_ACO") == "true"; //PH контроль - включение дозации кислоты по PH
	//Test_Pump = jee.param("Test_Pump") == "true"; //Дозация Активное Каталитическое Окисление «Active Catalytic Oxidation» ACO
	ACO_Work= jee.param("ACO_Work").toInt();
	// Timer_ACO_Start  = jee.param("Timer_ACO_Start");
	// Timer_ACO_Work  = jee.param("Timer_ACO_Work");

	PH_setting = jee.param("PH_setting").toFloat();

	ORP_setting = jee.param("ORP_setting").toInt();
	

	// //Power_APF = jee.param("Power_APF") == "true"; //Высокоэффективный коагулянт и флокулянт «All Poly Floc» APF
	// Activation_Timer_APF = jee.param("Activation_Timer_APF") == "true"; //Высокоэффективный коагулянт и флокулянт «All Poly Floc» APF
	// Timer_APF_Start = jee.param("Timer_APF_Start");
	// Timer_APF_Work = jee.param("Timer_APF_Work");


	gmtOffset_correct= jee.param("gmtOffset_correct").toInt(); // Корректировка часового пояса призапросен времени из Интернета
	

	//RestartESP32 = jee.param("RestartESP32") == "true"; 

}