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
    StaticJsonDocument<5120> getData(String fileName) {
        File file = LittleFS.open(fileName, "r");
        std::vector<String> v;
        while (file.available()) {
            v.push_back(file.readStringUntil('\n'));
        }
        file.close();

        String result;
        for (auto const& s : v) { result += s; }

        StaticJsonDocument<5120> doc;
        DeserializationError error = deserializeJson(doc, result);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());

        }
        return doc;
    };
private:
    String defaultFile = "config.json";
};


#endif //WIFI_AC_DAIKIN_EEPROMMANAGER_H
