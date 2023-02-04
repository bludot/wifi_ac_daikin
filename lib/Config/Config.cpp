//
// Created by James Trotter on 4/2/23.
//

#include "Config.h"

Config::Config() {
    this->wifiCreds.ssid = "";
    this->wifiCreds.password = "";
}

bool Config::setWifiCreds(WifiCreds creds) {
    this->wifiCreds = creds;
    return this->setToEEPROM();
}

WifiCreds Config::getWifiCreds() {
    if (this->wifiCreds.ssid == "" && this->wifiCreds.password == "") {
        return this->getFromEEPROM();
    } else {
        return this->wifiCreds;
    }
}

WifiCreds Config::getFromEEPROM() {
    String dataString = this->eepromManager.getData("/config.json");
    StaticJsonDocument<5120> data;
    deserializeJson(data, dataString);

    WifiCreds creds;
    creds.ssid = data["wificreds"]["ssid"].as<String>();
    creds.password = data["wificreds"]["password"].as<String>();
    return creds;
}

bool Config::setToEEPROM() {
    // convert WifiCreds to json string
    DynamicJsonDocument doc(8000);
    doc["wificreds"]["ssid"] = this->wifiCreds.ssid;
    doc["wificreds"]["password"] = this->wifiCreds.password;
    String jsonString;
    serializeJson(doc, jsonString);
    // save json string to eeprom
    this->eepromManager.saveData(String("/config.json"), jsonString);
    return true;
}