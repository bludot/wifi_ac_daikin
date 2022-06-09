//
// Created by James Trotter on 4/6/22.
//

#ifndef WIFI_AC_DAIKIN_EEPROMMANAGER_H
#define WIFI_AC_DAIKIN_EEPROMMANAGER_H

#include <WString.h>
#include "LittleFS.h"
#include <ArduinoJson.h>

class EEPROMManager {
public:
    void saveData(String fileName, String data) {
        LittleFS.remove(fileName);
        File file = LittleFS.open(fileName, "w");
        file.print(data);
        delay(1);
        file.close();

    };
    template <typename T>
    JsonObject getData() {
        StaticJsonDocument<1024> doc;
        DeserializationError error = deserializeJson(doc, json);
    };
private:
    String defaultFile = "config.json"
};


#endif //WIFI_AC_DAIKIN_EEPROMMANAGER_H
