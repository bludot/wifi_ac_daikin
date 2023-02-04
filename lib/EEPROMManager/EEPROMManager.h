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
        Serial.println("saving data...");
        Serial.println(data);
        // LittleFS.remove(fileName);
        File file = LittleFS.open(fileName, "w");
        file.print(data);
        delay(1);
        file.close();
    };
    String getData(String fileName) {
        Serial.println("Getting data");
        File file = LittleFS.open(fileName, "r");
        String result = file.readString();
        file.close();
        Serial.println("Got data");
        return result;


    };
private:
    String defaultFile = "config.json";
};


#endif //WIFI_AC_DAIKIN_EEPROMMANAGER_H
