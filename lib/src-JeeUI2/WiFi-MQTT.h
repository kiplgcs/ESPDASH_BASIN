// #include <WiFi.h>
// #include "esp_wifi.h"
String ipAddress;

#include <ESPmDNS.h>                    

String HostName = "basin"; // const char* HostName = "BASIN";//Для задания host имени сайту  xxx.local
String SID_STA; //Название WiFi из сохраненной переменной
String PasswordSTA;
String SID_AP;
String PasswordAP; //String PasswordAP = "0123456789";
bool Set_Wifi=false; //Флаг чтения параметров сети



int flag_WiFi = 1; // для switch - вывод информации в одно поле поочерди 


String WiFiset; // Записывам информацию IP, пароль и сеть для вывода на Nextion
int reasonCode; // Для записи статуса подключения к STA SID WIFI
//bool MQTT_Test =false;

String getStatusText(int status) {
    switch(status) {
        case WL_NO_SHIELD: return "No passwd WiFi";
        case WL_IDLE_STATUS: return "Ozhidanie";
        case WL_NO_SSID_AVAIL: return "Nedostupno SSID";
        case WL_SCAN_COMPLETED: return "Scan zaversheno";
        case WL_CONNECTED: return "Connected";
        case WL_CONNECT_FAILED: return "Connection Error";
        case WL_CONNECTION_LOST: return "Poteryano";
        case WL_DISCONNECTED: return "Disconnected";
        default: return "Neizvestniy status";
    }
}

int interval_Wifi_Connect =1000;

////////////////////////////******* Контроль мультиподключения к WiFi сети **********//////////////////////////////////////////

void ControlWiFi(int interval_Wifi_Con){static unsigned long timer;
if (interval_Wifi_Con + timer > millis()) return; 
timer = millis();
//---------------------------------------------------------------------------------

    

    if(!Set_Wifi){Set_Wifi = true;

        SID_STA = jee.param("ssid1");
        PasswordSTA = jee.param("pass1");
        HostName = SID_AP = jee.param("ap_ssid1");
        PasswordAP = jee.param("ap_ssid1");

        // if(SID_STA == F("")||SID_STA == F("null"))              jee.var(F("ssid1"), F("OAB-GeelyM")/*F("OAB_2.4G")*/);             SID_STA = "OAB-GeelyM";/*"OAB_2.4G";*/
        // if(PasswordSTA == F("")||PasswordSTA == F("null"))      jee.var(F("pass1"), "83913381"/*"OAB-245901:oab--245901"*/);  PasswordSTA = "83913381"/*"OAB-245901:oab--245901"*/;
         if(SID_STA == F("")||SID_STA == F("null"))              jee.var(F("ssid1"), F("OAB_2.4G_RPT"));             SID_STA = "OAB_2.4G_RPT";
        if(PasswordSTA == F("")||PasswordSTA == F("null"))      jee.var(F("pass1"), "OAB-245901:oab--245901");  PasswordSTA = "OAB-245901:oab--245901";
        if(SID_AP == F("")||SID_AP == F("null"))                jee.var(F("ap_ssid1"), F("basin"));                SID_AP = "basin";
        if(PasswordAP == F("")||PasswordAP == F("null"))        jee.var(F("ap_pass1"), "0123456789");            PasswordAP = "0123456789";

    }
    

  // Проверяем статус WiFi соединения
  if (WiFi.status() != WL_CONNECTED) {
    
     
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);

      interval_Wifi_Connect = 120000; // После перезагрузки подключение через 1000, и если уже подключились то увеличиваем время проверок до 120000
      
      // Если соединение отсутствует, пытаемся подключиться снова
      Serial.println("Потеряно соединение с WiFi. Повторное подключение...");
      Serial.println("Подключение к WiFi...");
      
      
      MDNS.begin(HostName.c_str());//Web имя при запросе: http://HostName

      WiFi.setHostname(HostName.c_str()); //Имя в сети и роутере

      WiFi.begin(SID_STA.c_str(), PasswordSTA.c_str()); 
    
  
      int attempts = 0;
      
      // Ждем установления соединения с WiFi
      while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
          attempts++;
          if (attempts >= 5) { // Максимальное количество попыток
              Serial.println("Не удалось подключиться к WiFi. Переход в режим точки доступа...");
              //WiFi.disconnect(); // Отключаемся от текущей сети
              //WiFi.softAP(HostName, PasswordAP, 7, false, 7); // Включаем режим точки доступа
            WiFi.mode(WIFI_AP);
            WiFi.softAP(SID_AP.c_str(), PasswordAP.c_str(), 7, false, 7); //, 7, false, 7); // Включаем режим точки доступа
                
              return;
          }

          WiFi.softAP(SID_AP.c_str(), PasswordAP.c_str(), 7, true, 7); // в режиме STA объявляем скрытую сеть AP

     }
      
      // Успешное подключение к WiFi
      Serial.println("\nПодключено к WiFi!");
      
      // Устанавливаем имя хоста
      //WiFi.setHostname(HostName);
      
        
      //ESP.restart();////// Необходимо присвоить переменным значение о подключении и перезагрузить ESP для работы MQTT

      

  } else {Serial.println("Проверка подключения к WiFi успешная - подключение есть");}

    
}


    void booootn(){
    MDNS.begin(jee.param("ap_ssid1").c_str());//Web имя при запросе: http://HostName
    WiFi.setHostname(jee.param("ap_ssid1").c_str()); //Имя в роутере
    WiFi.begin(jee.param("ssid1").c_str(), jee.param("pass1").c_str()); //   ESP.restart();
    }

    void btnAP(){
    MDNS.begin(jee.param("ssid1").c_str());//Web имя при запросе: http://HostName
    WiFi.softAP(jee.param("ap_ssid1").c_str(), jee.param("ap_ssid1").c_str(), 7, false, 7); // Включаем режим точки доступа
    }

    void RestESP(){
    ESP.restart(); // Перезагрузить ESP32
    }

  
