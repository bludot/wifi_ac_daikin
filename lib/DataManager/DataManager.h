//
// Created by James Trotter on 4/6/22.
//

#ifndef WIFI_AC_DAIKIN_DATAMANAGER_H
#define WIFI_AC_DAIKIN_DATAMANAGER_H

#include "../WifiConnectionManager/WifiConnectionManager.h"
#include "../../include/IRDaikinServer.h"


struct WifiCredentials {
    char ssid[20];
    char password[64];
};

struct EEPROMData {
    struct WifiCredentials wifiCreds;
    struct EEPROM_data acData;
};

#endif //WIFI_AC_DAIKIN_DATAMANAGER_H
