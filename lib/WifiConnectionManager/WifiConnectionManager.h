//
// Created by James Trotter on 4/6/22.
//

#ifndef WIFI_AC_DAIKIN_WIFICONNECTIONMANAGER_H
#define WIFI_AC_DAIKIN_WIFICONNECTIONMANAGER_H
#include <ESP8266WiFi.h>
#include "../DataManager/DataManager.h"
#include "EEPROMManager.h"
#include "../Config/Config.h"

class WifiConnectionManager {
private:
    bool connected = false;
    char ssid[20];
    char password[64];
    EEPROMManager eepromManager;
    Config config;
    // define onConnectCallback function
    std::function<void()> onConnectCallback;
public:
    WifiConnectionManager(EEPROMManager manager, Config config);
    void setup();
    void setupAP();
    String scan();
    void connect(String ssid, String password);
    bool testWifi();
    void setOnConnectCallback(std::function<void()> callback);
};


#endif //WIFI_AC_DAIKIN_WIFICONNECTIONMANAGER_H
