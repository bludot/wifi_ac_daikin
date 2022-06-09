
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



RoomConditions roomConditions;
EEPROMManager eepromManager;
IRDaikinESP daikinir(D1);  // An IR LED is controlled by GPIO pin 4 (D2)

AsyncWebServer server(80);



WifiConnectionManager wifiConnectionManager(eepromManager);

setting_t fan_speeds[6] = { { "Auto", DAIKIN_FAN_AUTO },{ "Slowest", 1 },{ "Slow", 2 },{ "Medium", 3 },{ "Fast", 4 },{ "Fastest", 5 } };
setting_t modes[5] = { { "Cool", DAIKIN_COOL }, { "Heat", DAIKIN_HEAT }, { "Fan", DAIKIN_FAN }, { "Auto", DAIKIN_AUTO }, { "Dry", DAIKIN_DRY } };
setting_t on_off[2] = { { "On", 1 }, { "Off", 0 } };

const int ledPin = 2;
String ledState;

// Replaces placeholder with LED state value
String processor(const String& var){
    Serial.println(var);
    if(var == "GPIO_STATE"){
        if(digitalRead(ledPin)){
            ledState = "OFF";
        }
        else{
            ledState = "ON";
        }
        Serial.print(ledState);
        return ledState;
    }
    return String();
}

void saveStatus() {
    EEPROMData data_new;
    data_new.acData.temp = daikinir.getTemp();
    data_new.acData.fan = daikinir.getFan();
    data_new.acData.power = daikinir.getPower();
    data_new.acData.powerful = daikinir.getPowerful();
    data_new.acData.quiet = daikinir.getQuiet();
    data_new.acData.swingh = daikinir.getSwingHorizontal();
    data_new.acData.swingv = daikinir.getSwingVertical();
    data_new.acData.mode = daikinir.getMode();
    data_new.wifiCreds = wifiConnectionManager.getCredentials();
    eepromManager.saveData("/ac.json", data_new);
}

void restoreStatus() {
    EEPROMData data_stored = eepromManager.getData("/ac.json");
    if (data_stored.acData.power != false) {
        daikinir.setTemp(data_stored.acData.temp);
        daikinir.setFan(data_stored.acData.fan);
        daikinir.setPower(data_stored.acData.power);
        daikinir.setPowerful(data_stored.acData.powerful);
        daikinir.setQuiet(data_stored.acData.quiet);
        daikinir.setSwingHorizontal(data_stored.acData.swingh);
        daikinir.setSwingVertical(data_stored.acData.swingv);
        daikinir.setMode(data_stored.acData.mode);
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

String getSelection(const String& name, int min, int max, int selected, setting_t* list) {
    String ret = "<select name=\""+name+"\">";
    for (int i = min; i <= max; i++) {
        ret += "<option ";
        if (list[i].value == selected)
            ret += "selected ";
        ret += "value = " + String(list[i].value) + " > " + String(list[i].name) + "</option>";
    }
    return ret += "</select><br\>";
}

uint8_t atou8(const char *s)
{
    uint8_t v = 0;
    while (*s) { v = (v << 1) + (v << 3) + (*(s++) - '0'); }
    return v;
}

void handleCmd(AsyncWebServerRequest *request) {
    String argName;
    uint8_t arg;
    int params = request->params();
    for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);

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
    snprintf(buff, sizeof(buff), "{\"tempC\": %f, \"tempF\": %f, \"humidity\": %f}", condition.temperatureC, condition.temperatureF, condition.humidity);
    String resp(buff);
    return resp;
}


void connectWifi(AsyncWebServerRequest *request) {
    String postBody = request->getParam("plain")->value().c_str();
    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, postBody);
    if (error) {
        // if the file didn't open, print an error:
        Serial.print(F("Error parsing JSON "));
        Serial.println(error.c_str());

        String msg = error.c_str();

        request->send(400, F("text/html"),
                    "Error in parsin json body! <br>" + msg);

    } else {
        JsonObject postObj = doc.as<JsonObject>();
        wifiConnectionManager.connect(postObj["ssid"], postObj["password"]);
        // server.stop();
    }
}

void setupHandlers() {
    server.on("/setup/scan", HTTP_GET, [](AsyncWebServerRequest *request){
        String accessPointsJson = wifiConnectionManager.scan();
        request->send(200, "text/json", accessPointsJson);
    });

    server.on("/setup/connect", HTTP_POST, [](AsyncWebServerRequest *request){
        connectWifi(request);
    });

}

void setup(void) {

    if(!LittleFS.begin()){
        Serial.println("An Error has occurred while mounting LittleFS");
        return;
    }

    Serial.begin(115200);
    delay(3000);
    Serial.println();
    //Serial.println("Disconnecting previously connected WiFi");
    eepromManager.setup(1024);
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

    server.on("/cmd", HTTP_GET,  handleCmd);
    server.on("/roomconditions", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", handleRoomConditions());
    });
//    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
//        request->send(LittleFS, "/style.css", "text/css");
//    });
//    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//        request->send(LittleFS, "/index.html", String(), false, processor);
//    });
    server.serveStatic("/", LittleFS, "/");
    setupHandlers();

    server.begin();
    Serial.println("HTTP server started");

    if (MDNS.begin("ac", WiFi.localIP())) {
        Serial.println("MDNS responder started");
    }
}

void loop(void) {
    roomConditions.setConditions();
    EEPROMData data = eepromManager.getData();

    // wifiConnectionManager.scan();
    delay(300);
}
