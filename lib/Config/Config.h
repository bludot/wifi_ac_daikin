//
// Created by James Trotter on 4/2/23.
//

#ifndef WIFI_AC_DAIKIN_CONFIG_H
#define WIFI_AC_DAIKIN_CONFIG_H

#include "EEPROMManager.h"

struct WifiCreds {
    String ssid;
    String password;
};

class Config {
private:
    WifiCreds wifiCreds;
    WifiCreds getFromEEPROM();
    bool setToEEPROM();
    EEPROMManager eepromManager;

public:
    Config();
    bool setWifiCreds(WifiCreds creds);
    WifiCreds getWifiCreds();
};

#endif //WIFI_AC_DAIKIN_CONFIG_H