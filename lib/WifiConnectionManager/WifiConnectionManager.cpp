//
// Created by James Trotter on 4/6/22.
//

#include "WifiConnectionManager.h"
#include "EEPROMManager.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>

const String defaultSsid = "esp8266";
const String defaultPassword = "password";


String getEncryptionType(uint8_t type) {
    switch (type) {
        case ENC_TYPE_WEP:
            return "WEP";
        case ENC_TYPE_TKIP:
            return "TKIP";
        case ENC_TYPE_CCMP:
            return "CCMP";
        case ENC_TYPE_NONE:
            return "NONE";
        default:
            return "UNKNOWN";
    }
}

String convertToString(char* a, int size) {
    int i;
    String s = "";
    for (i = 0; i < size; i++) {
        s = s + a[i];
    }
    return s;
}

void WifiConnectionManager::setup() {
    Serial.println("setting up wifi");

    StaticJsonDocument<5120> data = this->eepromManager.getData("/config.json");

    String ssid = data["wificreds"]["ssid"].as<String>();
    String password = data["wificreds"]["password"].as<String>();

    Serial.println(ssid);
    Serial.println(password);


    if (ssid == "" && password == "") {
        // this->server.on("/setup");
        WiFi.softAP(defaultSsid, defaultPassword);
        Serial.println("Initializing_Wifi_accesspoint");
        Serial.print("Local IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("SoftAP IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        this->connect(ssid, password);
        if (testWifi()) {
            WiFi.softAP(defaultSsid, defaultPassword);
            Serial.println("Initializing_Wifi_accesspoint");
            Serial.print("Local IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("SoftAP IP: ");
            Serial.println(WiFi.softAPIP());
        }
    }


}

WifiConnectionManager::WifiConnectionManager(EEPROMManager manager) {
    this->eepromManager = manager;
}

String WifiConnectionManager::scan() {
    String st;
    int n = WiFi.scanComplete();
    StaticJsonDocument<5120> doc;
    JsonArray accessPoints = doc.createNestedArray("accessPoints");

    if(n == -2) {
        WiFi.scanNetworks(true);
    } else if(n) {
        Serial.println("Scanning access points...");
        if (n != 0) {
            for (int i = 0; i < n; ++i) {
                String name = WiFi.SSID(i);
                int32_t strength = WiFi.RSSI(i);
                String mac = WiFi.BSSIDstr(i);
                Serial.print(i + 1);  //Sr. No
                Serial.print(": ");
                Serial.print(name); //SSID
                Serial.print(" (");
                Serial.print(strength); //Signal Strength
                Serial.print(") MAC:");
                Serial.print(mac);
                Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " Unsecured" : " Secured");
                // Print SSID and RSSI for each network found
                JsonObject accessPoint = accessPoints.createNestedObject();
                accessPoint["ssid"] = name;
                accessPoint["mac"] = mac;
                accessPoint["security"] = (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " Unsecured" : " Secured";
                // accessPoints.add(accessPoint);
            }
        } else {
            Serial.println("No Networks Found");
        }
        WiFi.scanDelete();
        /*if(WiFi.scanComplete() == -2){
            WiFi.scanNetworks(true);
        }*/
    }
    String output;
    serializeJsonPretty(doc, output);
    return output;
}

void WifiConnectionManager::connect(String ssid, String password) {
    Serial.println(ssid);
    Serial.println(password);
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (testWifi()) {
        Serial.println("preparing to write data");
        StaticJsonDocument<5120> data = this->eepromManager.getData("/config.json");

        data["wificreds"]["ssid"] = ssid;
        data["wificreds"]["password"] = password;

        Serial.println(ssid);
        Serial.println(password);

        String output;
        serializeJsonPretty(data, output);
        this->eepromManager.saveData("/config.json", output);
        Serial.println("Wrote data");
    }
}

bool WifiConnectionManager::testWifi() {
    int c = 0;
    Serial.println("Waiting for WiFi to connect");
    while ( c < 20 ) {
        if (WiFi.status() == WL_CONNECTED)
        {
            return true;
        }
        delay(500);
        Serial.print("*");
        c++;
    }
    Serial.println("");
    Serial.println("Connection timed out, opening AP or Hotspot");
    return false;
}