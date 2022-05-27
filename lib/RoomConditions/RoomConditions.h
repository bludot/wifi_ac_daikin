//
// Created by James Trotter on 27/5/22.
//

#include "DHT.h"

DHT dht(D4,DHT22);

#ifndef WIFI_AC_DAIKIN_ROOMCONDITIONS_H
#define WIFI_AC_DAIKIN_ROOMCONDITIONS_H

namespace temperature {
    struct Conditions {
        float temperatureC = 0.0;
        float temperatureF = 0.0;
        float humidity = 0.0;
    };
    class RoomConditions {
    private:
        float temperatureC = 0.0;;
        float temperatureF = 0.0;;
        float humidity = 0.0;;
    public:
        RoomConditions() {
        };
        Conditions getConditions() {
            Conditions conditions;
            conditions.temperatureC = this->temperatureC;
            conditions.temperatureF = this->temperatureF;
            conditions.humidity = this->humidity;
            return conditions;
        };
        void setConditions() {
            this->humidity = dht.readHumidity();
            this->temperatureC = dht.readTemperature(); // or dht.readTemperature(true) for Fahrenheit

            if (isnan(this->temperatureC) || isnan(this->humidity)) {
                Serial.println("Failed to read from DHT sensor!");
                return;
            }
            this->temperatureF =(9.0/5.0) * this->temperatureC + 32;
        };
        void setup() {
            dht.begin();
            this->setConditions();
        }
    };

} // temperature

#endif //WIFI_AC_DAIKIN_ROOMCONDITIONS_H
