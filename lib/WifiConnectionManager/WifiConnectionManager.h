//
// Created by James Trotter on 4/6/22.
//

#ifndef WIFI_AC_DAIKIN_WIFICONNECTIONMANAGER_H
#define WIFI_AC_DAIKIN_WIFICONNECTIONMANAGER_H
#include <ESP8266WiFi.h>
#include "../DataManager/DataManager.h"
#include "EEPROMManager.h"

class WifiConnectionManager {
private:

    char ssid[20];
    char password[64];
    EEPROMManager<EEPROMData> eepromManager;
public:
    WifiConnectionManager(EEPROMManager<EEPROMData> manager);
    void setup();
    String scan();
    void connect(String ssid, String password);
    WifiCredentials getCredentials();
    bool testWifi();

};


#endif //WIFI_AC_DAIKIN_WIFICONNECTIONMANAGER_H
