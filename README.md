# AC DAIKIN program for ESP8266

This is a program for ESP8266 to control AC Daikin.
Schematics not yet provided.


### notes:
* There is a bug in a library used [see here](https://github.com/crankyoldgit/IRremoteESP8266/issues/430)
  * you must change the file `src/IRsend.cpp`
  * line 120: 
  ```c++
  uint16_t wholes =  usec / 1000UL;
  while(wholes--){
    delayMicroseconds(1000);
  }
  delayMicroseconds(static_cast<uint16_t>(usec % 1000UL));
  ```