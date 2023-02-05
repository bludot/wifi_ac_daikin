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


void WifiConnectionManager::setup() {
    Serial.println("setting up wifi");
    WifiCreds creds = this->config.getWifiCreds();


    Serial.println(creds.ssid);
    Serial.println(creds.password);


    if ((creds.ssid == "" || creds.ssid == NULL || creds.ssid == "null") && (creds.password == "" || creds.password == NULL || creds.password == "null")) {
        this->setupAP();
    } else {
        this->connect(creds.ssid, creds.password);
        if (testWifi()) {
            WiFi.softAP(defaultSsid, defaultPassword);
            Serial.println("Initializing_Wifi_accesspoint");
            Serial.print("Local IP: ");
            Serial.println(WiFi.localIP());
            Serial.print("SoftAP IP: ");
            Serial.println(WiFi.softAPIP());
            this->connected = true;
            this->onConnectCallback();
        }
    }


}

void WifiConnectionManager::setupAP() {
    WiFi.softAP(defaultSsid, defaultPassword);
    Serial.println("Initializing_Wifi_accesspoint");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("SoftAP IP: ");
    Serial.println(WiFi.softAPIP());
}

WifiConnectionManager::WifiConnectionManager(EEPROMManager manager, Config config) {
    this->eepromManager = manager;
    this->config = config;
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
        if(WiFi.scanComplete() == -2){
            WiFi.scanNetworks(true);
        }
    }
    String output;
    serializeJsonPretty(doc, output);
    return output;
}

void WifiConnectionManager::connect(String ssid, String password) {
    Serial.println(ssid);
    Serial.println(password);
    Serial.println("preparing to write data");
    WifiCreds creds = WifiCreds();
    creds.ssid = ssid;
    creds.password = password;
    this->config.setWifiCreds(creds);


    Serial.println("Wrote data");

//    WiFi.persistent(false);
//    if(WiFi.getMode() & WIFI_STA){
//        WiFi.mode(WIFI_OFF);
//        int timeout = millis()+1200;
//        // async loop for mode change
//        while(WiFi.getMode()!= WIFI_OFF && millis()<timeout){
//            delay(0);
//        }
//    }
    WiFi.persistent(true);
    WiFi.begin(ssid, password);
//    if (testWifi()) {

//    }
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
    this->setupAP();
    Serial.println("opened AP or Hotspot");
    return false;
}

void WifiConnectionManager::setOnConnectCallback(std::function<void()> callback) {
    this->onConnectCallback = callback;
}