//https://github.com/ayushsharma82/AsyncElegantOTA

//Загрузка: http://IP-адрес:порт/update 


/***************************************************Раскоментировать для ESP32******************************************/
//#include <WiFi.h>
//#include <Hash.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

AsyncWebServer server1 (8080);

void WebUpdate_setup (void){
server1.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32, IP:"+ ipAddress);
  });

  AsyncElegantOTA.begin(&server1);    // Start ElegantOTA
  server1.begin();
  Serial.println("HTTP server started");
}

// void loop(void) {
//   AsyncElegantOTA.loop();
// }

// //***************************************************Раскоментировать для ESP8266*****************************************/

// #include <ESP8266WiFi.h>
// #include <Hash.h>
// #include <ESPAsyncTCP.h>
// #include <ESPAsyncWebServer.h>
// #include <AsyncElegantOTA.h>

// AsyncWebServer server1 (8080);

// void WebUpdate_setup (void){

//   server1.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
//     request->send(200, "text/plain", "Hi! I am ESP8266.");
//   });

//   AsyncElegantOTA.begin(&server1);    // Start ElegantOTA
//   server.begin();
//   Serial.println("HTTP server started");

// }


// // void loop(void) {
// //   AsyncElegantOTA.loop();
// // }