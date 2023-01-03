
/* esp_ir_ac.ino */
/*
* IRDaikinServer : Simple http interface to control Daikin A/C units via IR
* Version 0.1 July, 2017
* Copyright 2017 Andrea Cucci
*
* Dependencies:
*  - Arduino core for ESP8266 WiFi chip (https://github.com/esp8266/Arduino)
* - IRremote ESP8266 Library (https://github.com/markszabo/IRremoteESP8266)
*
*/

#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ir_Daikin.h>
#include "../lib/RoomConditions/RoomConditions.h"
#include "../lib/EEPROMManager/EEPROMManager.h"
#include "../lib/WifiConnectionManager/WifiConnectionManager.h"
#include <ArduinoJson.h>
#include "LittleFS.h"
#include "AsyncJson.h"


RoomConditions roomConditions;
EEPROMManager eepromManager;
IRDaikinESP daikinir(D1);  // An IR LED is controlled by GPIO pin 4 (D2)

AsyncWebServer server(80);

struct daikinStruct {
    uint8_t temp;
    uint8_t fan;
    uint8_t power;
    uint8_t powerful;
    uint8_t quiet;
    uint8_t swingh;
    uint8_t swingv;
    uint8_t mode;
};

WifiConnectionManager wifiConnectionManager(eepromManager);

setting_t fan_speeds[6] = {{"Auto", DAIKIN_FAN_AUTO},
                           {"Slowest", 1},
                           {"Slow",    2},
                           {"Medium",  3},
                           {"Fast",    4},
                           {"Fastest", 5}};
setting_t modes[5] = {{"Cool", DAIKIN_COOL},
                      {"Heat", DAIKIN_HEAT},
                      {"Fan",  DAIKIN_FAN},
                      {"Auto", DAIKIN_AUTO},
                      {"Dry",  DAIKIN_DRY}};
setting_t on_off[2] = {{"On",  1},
                       {"Off", 0}};

EEPROM_data acData = {
        temp: 0,
        fan: 0,
        power: 0,
        powerful: 0,
        quiet: 0,
        swingh: 0,
        swingv: 0,
        mode: 0,
};

const int ledPin = 2;
String ledState;

// Replaces placeholder with LED state value
String processor(const String &var) {
    Serial.println(var);
    if (var == "GPIO_STATE") {
        if (digitalRead(ledPin)) {
            ledState = "OFF";
        } else {
            ledState = "ON";
        }
        Serial.print(ledState);
        return ledState;
    }
    return String();
}

void saveStatus() {
    acData.temp = daikinir.getTemp();
    acData.fan = daikinir.getFan();
    acData.power = daikinir.getPower();
    acData.powerful = daikinir.getPowerful();
    acData.quiet = daikinir.getQuiet();
    acData.swingh = daikinir.getSwingHorizontal();
    acData.swingv = daikinir.getSwingVertical();
    acData.mode = daikinir.getMode();

}

void restoreStatus() {

    if (acData.power != false) {
        daikinir.setTemp(acData.temp);
        daikinir.setFan(acData.fan);
        daikinir.setPower(acData.power);
        daikinir.setPowerful(acData.powerful);
        daikinir.setQuiet(acData.quiet);
        daikinir.setSwingHorizontal(acData.swingh);
        daikinir.setSwingVertical(acData.swingv);
        daikinir.setMode(acData.mode);
        daikinir.send();
    } else {
        daikinir.setTemp(25);
        daikinir.setFan(2);
        daikinir.setPower(false);
        daikinir.setPowerful(false);
        daikinir.setQuiet(false);
        daikinir.setSwingHorizontal(false);
        daikinir.setSwingVertical(false);
        daikinir.setMode(DAIKIN_COOL);
        daikinir.send();
        saveStatus();
    }
}

String getSelection(const String &name, int min, int max, int selected, setting_t *list) {
    String ret = "<select name=\"" + name + "\">";
    for (int i = min; i <= max; i++) {
        ret += "<option ";
        if (list[i].value == selected)
            ret += "selected ";
        ret += "value = " + String(list[i].value) + " > " + String(list[i].name) + "</option>";
    }
    return ret += "</select><br\>";
}

uint8_t atou8(const char *s) {
    uint8_t v = 0;
    while (*s) { v = (v << 1) + (v << 3) + (*(s++) - '0'); }
    return v;
}

void handleCmd(AsyncWebServerRequest *request) {
    String argName;
    uint8_t arg;
    int params = request->params();
    for (int i = 0; i < params; i++) {
        AsyncWebParameter *p = request->getParam(i);

        arg = atou8(p->value().c_str());
        if (p->name().c_str() == "temp") { daikinir.setTemp(arg); }
        else if (p->name().c_str() == "fan") { daikinir.setFan(arg); }
        else if (p->name().c_str() == "power") { daikinir.setPower(arg); }
        else if (p->name().c_str() == "powerful") { daikinir.setPowerful(arg); }
        else if (p->name().c_str() == "quiet") { daikinir.setQuiet(arg); }
        else if (p->name().c_str() == "swingh") { daikinir.setSwingHorizontal(arg); }
        else if (p->name().c_str() == "swingv") { daikinir.setSwingVertical(arg); }
        else if (p->name().c_str() == "mode") { daikinir.setMode(arg); }
        Serial.print(argName);
        Serial.print(" ");
        Serial.println(arg);
    }
    saveStatus();
    daikinir.send();
    //handleRoot();
    request->send(200, "text/plaintext", "OK");
}

String handleRoomConditions() {
    Conditions condition = roomConditions.getConditions();

    char buff[500];
    snprintf(buff, sizeof(buff), "{\"tempC\": %f, \"tempF\": %f, \"humidity\": %f}", condition.temperatureC,
             condition.temperatureF, condition.humidity);
    String resp(buff);
    return resp;
}


void connectWifi(StaticJsonDocument<1024> doc) {


    wifiConnectionManager.connect(doc["ssid"], doc["password"]);
    // server.stop();


}

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "application/json", "{\"message\":\"Not found\"}");
}

void setupHandlers() {
    server.on("/setup/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        String accessPointsJson = wifiConnectionManager.scan();
        request->send(200, "text/json", accessPointsJson);
    });
    server.on("/setup/connect", HTTP_POST, [](AsyncWebServerRequest *request) {
        int params = request->params();
        Serial.printf("Save settings, %d params", params);
        for(int i = 0; i < params; i++) {
            AsyncWebParameter* p = request->getParam(i);
            if(p->isFile()){
                Serial.printf("_FILE[%s]: %s, size: %u", p->name().c_str(), p->value().c_str(), p->size());
            } else if(p->isPost()){
                Serial.printf("_POST[%s]: %s", p->name().c_str(), p->value().c_str());
            } else {
                Serial.printf("_GET[%s]: %s", p->name().c_str(), p->value().c_str());
            }
        }
        if(request->hasParam("body", true))
        {
            AsyncWebParameter* p = request->getParam("body", true);
            String json = p->value();
            request->send(200, "text/json", json);
        }
        if(request->hasParam("plain", true))
        {
            AsyncWebParameter* p = request->getParam("body", true);
            String json = p->value();
            request->send(200, "text/json", json);
        }
        request->send(200, "text/json", "OK");
    });
//    AsyncCallbackJsonWebHandler *handler = new AsyncCallbackJsonWebHandler("/setup/connect", [](AsyncWebServerRequest *request, JsonVariant &json) {
//        StaticJsonDocument<1024> data;
//        if (json.is<JsonArray>())
//        {
//            data = json.as<JsonArray>();
//        }
//        else if (json.is<JsonObject>())
//        {
//            data = json.as<JsonObject>();
//        }
//        String response;
//        serializeJson(data, response);
//        request->send(200, "application/json", response);
//        Serial.println(response);
//        // connectWifi(data);
//        // request->send(200, "text/plain", "OK");
//    });
//    server.addHandler(handler);

}

void setup(void) {

    if (!LittleFS.begin()) {
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    Serial.begin(115200);
    delay(3000);
    Serial.println();
    //Serial.println("Disconnecting previously connected WiFi");

//    EEPROMData data;
//    data.wifiCreds.ssid = "";
//    data.wifiCreds.password = "";
//    eepromManager.saveData(data);

    roomConditions.setup();


    // analogReference(INTERNAL);
    pinMode(A0, INPUT);
    daikinir.begin();
    restoreStatus();

    wifiConnectionManager.setup();
//    WiFi.mode(WIFI_STA);
//    WiFi.begin(ssid, password);
//    DEBUG_PRINTLN("");
//
//    while (WiFi.status() != WL_CONNECTED) {
//        delay(500);
//        DEBUG_PRINT(".");
//    }
//    DEBUG_PRINTLN("");
//    DEBUG_PRINT("Connected to ");
//    DEBUG_PRINTLN(ssid);
//    DEBUG_PRINT("IP address: ");
//    DEBUG_PRINTLN(WiFi.localIP());

    server.on("/cmd", HTTP_GET, handleCmd);
    server.on("/roomconditions", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", handleRoomConditions());
    });
//    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
//        request->send(LittleFS, "/style.css", "text/css");
//    });
//    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//        request->send(LittleFS, "/index.html", String(), false, processor);
//    });

    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");;
    server.onNotFound(notFound);
    setupHandlers();
    server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        if (!index) {
            Serial.printf("BodyStart: %u", total);
        }
        Serial.printf("%s", (const char *) data);
        if (index + len == total) {
            Serial.printf("BodyEnd: %u", total);
        }
    });


    server.begin();
    Serial.println("HTTP server started");

    if (MDNS.begin("ac", WiFi.localIP())) {
        Serial.println("MDNS responder started");
    }
}

void loop(void) {
    roomConditions.setConditions();
    // wifiConnectionManager.scan();
    delay(300);
}
