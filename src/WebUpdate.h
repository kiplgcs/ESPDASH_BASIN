#pragma once
#define ELEGANTOTA_USE_ASYNC_WEBSERVER 1  // ensure ElegantOTA uses AsyncWebServer API
#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

inline AsyncWebServer otaServer(8080);

inline void beginWebUpdate() {
    otaServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "ESP32 OTA Ready at /update");
    });

    ElegantOTA.begin(&otaServer);  // async mode now supported
    otaServer.begin();
}
