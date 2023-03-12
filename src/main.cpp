
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
#include <iterator>
#include "LittleFS.h"
#include "AsyncTimer.h"
#include <AsyncMqttClient.h>
#include <Ticker.h>
#include <TaskScheduler.h>


#define MQTT_HOST "hivemq.floretos.com"
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

RoomConditions roomConditions;
EEPROMManager eepromManager;
IRDaikinESP daikinir(D1);  // An IR LED is controlled by GPIO pin 4 (D2)
Config config;

AsyncWebServer server(80);

Scheduler runner;



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

WifiConnectionManager wifiConnectionManager(eepromManager, config);


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

daikinStruct acData = {
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
unsigned long start_time30 = 0;
unsigned long start_time60 = 0;

// save heartbeat time to variable
unsigned long heatbeattimestamp = 0;



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

void setAC(String payload) {
    Serial.println("payload to json");
    StaticJsonDocument<5120> data;
    deserializeJson(data, payload);
    Serial.println(data["temp"].as<String>());


    // convert to uint8_t
    const uint8_t temp = atou8(data["temp"].as<String>().c_str());
    const uint8_t fan = atou8(data["fan"].as<String>().c_str());
    const uint8_t power = atou8(data["power"].as<String>().c_str());
    const uint8_t powerful = atou8(data["powerful"].as<String>().c_str());
    const uint8_t quiet = atou8(data["quiet"].as<String>().c_str());
    const uint8_t swingh = atou8(data["swingh"].as<String>().c_str());
    const uint8_t swingv = atou8(data["swingv"].as<String>().c_str());
    const uint8_t mode = atou8(data["mode"].as<String>().c_str());

    daikinir.setTemp(temp);
    daikinir.setFan(fan);
    daikinir.setPower(power ? true : false);
    daikinir.setPowerful(powerful ? true : false);
    daikinir.setQuiet(quiet ? true : false);
    daikinir.setSwingHorizontal(swingh ? true : false);
    daikinir.setSwingVertical(swingv ? true : false);
    daikinir.setMode(mode);
    Serial.println("saving status");
    saveStatus();
    Serial.println("sending to AC");
    daikinir.send();
    Serial.println("sent to ac");

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

String getRoomConditions() {
    Conditions condition = roomConditions.getConditions();
    char buff[500];
    snprintf(buff, sizeof(buff), "{\"tempC\": %f, \"tempF\": %f, \"humidity\": %f}", condition.temperatureC,
             condition.temperatureF, condition.humidity);
    String resp(buff);
    return resp;
}

String handleRoomConditions() {
    Conditions condition = roomConditions.getConditions();

    char buff[500];
    snprintf(buff, sizeof(buff), "{\"tempC\": %f, \"tempF\": %f, \"humidity\": %f}", condition.temperatureC,
             condition.temperatureF, condition.humidity);
    String resp(buff);
    return resp;
}



void notFound(AsyncWebServerRequest *request) {
    request->send(404, "application/json", "{\"message\":\"Not found\"}");
}

void setupHandlers() {
    server.on("/setup/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
        String accessPointsJson = wifiConnectionManager.scan();
        request->send(200, "text/json", accessPointsJson);
    });
    server.on("/setup/connect", HTTP_POST, [](AsyncWebServerRequest * request){}, NULL, [](AsyncWebServerRequest * request, uint8_t *data, size_t len, size_t index, size_t total) {

        for (size_t i = 0; i < len; i++) {
            Serial.write(data[i]);
        }
        char* input = (char *) data;
        String strData = input;

        Serial.println();
        Serial.printf("result: %s", strData.c_str());

        StaticJsonDocument<5120> doc;
        DeserializationError error = deserializeJson(doc, strData.c_str());

        wifiConnectionManager.connect(doc["ssid"], doc["password"]);
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


void connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void onWifiConnect() {
    Serial.println("Connected to Wi-Fi.");
    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
    Serial.println("Disconnected from Wi-Fi.");
    mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
//    wifiReconnectTimer.once(2, wifiConnectionManager.);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("Disconnected from MQTT.");

    if (WiFi.isConnected()) {
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
    Serial.println("Subscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
    Serial.print("  qos: ");
    Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
    Serial.println("Unsubscribe acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void onMqttPublish(uint16_t packetId) {
    Serial.println("Publish acknowledged.");
    Serial.print("  packetId: ");
    Serial.println(packetId);
}

void publishRoomConditions() {
    String data = getRoomConditions();
    mqttClient.publish("roomconditions", 1, true, data.c_str());
}

void onMqttConnect(bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    Serial.print("Session present: ");
    Serial.println(sessionPresent);
    mqttClient.subscribe("ac", 1);
    mqttClient.subscribe("heartbeat-nodemcu", 1);

}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    Serial.println("Publish received.");
    Serial.print("  topic: ");
    Serial.println(topic);
    Serial.print("  qos: ");
    Serial.println(properties.qos);
    Serial.print("  dup: ");
    Serial.println(properties.dup);
    Serial.print("  retain: ");
    Serial.println(properties.retain);
    Serial.print("  len: ");
    Serial.println(len);
    Serial.print("  index: ");
    Serial.println(index);
    Serial.print("  total: ");
    Serial.println(total);
    Serial.println("  payload: ");
    // payload to string
    String payloadString = "";
    for (int i = 0; i < len; i++) {
        payloadString += (char)payload[i];
    }
    Serial.println(payloadString);
    // topic to string
    String str = "";
    for (int i = 0; i < strlen(topic); i++) {
        str += (char)topic[i];
    }
    Serial.println(str);
    if (str == "ac") {
        Serial.println("Setting AC");
        setAC(payloadString);
    }

    // if topic is heartbeat-nodemcu then set heartbeat to true
    if (str == "heartbeat-nodemcu") {
        Serial.println("Heartbeat received");
        heatbeattimestamp = millis();
    }
}

void checkHeartbeat() {
    Serial.println("Checking heartbeat");
    if (millis() - heatbeattimestamp > 30000) {
        Serial.println("Heartbeat not received for 30 sec");
        // reset
        ESP.restart();
    }
}

// one min
Task t1(60000, TASK_FOREVER, &publishRoomConditions);
// 30 sec
Task t2(30000, TASK_FOREVER, &checkHeartbeat);

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




    server.on("/cmd", HTTP_GET, handleCmd);
    server.on("/roomconditions", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(200, "text/plain", handleRoomConditions());
    });

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

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onSubscribe(onMqttSubscribe);
    mqttClient.onUnsubscribe(onMqttUnsubscribe);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.onPublish(onMqttPublish);
    mqttClient.setCredentials("admin-user", "admin-password");
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    wifiConnectionManager.setOnConnectCallback(onWifiConnect);
    wifiConnectionManager.setup();
    Serial.println("ALLOW_DELAY_CALLS");
    Serial.println(ALLOW_DELAY_CALLS);
    runner.init();
    runner.addTask(t1);
    runner.addTask(t2);
    t1.enable();
    t2.enable();
}

void loop(void) {
    roomConditions.setConditions();
    runner.execute();
    delay(300);
}
