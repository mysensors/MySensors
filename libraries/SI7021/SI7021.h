/*
  Copyright 2014 Marcus Sorensen <marcus@electron14.com>

This program is licensed under the GNU GPL v2
*/
#ifndef si7021_h
#define si7021_h

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#ifdef __AVR_ATtiny85__
 #include "TinyWireM.h"
 #define Wire TinyWireM
#else
 #include <avr/pgmspace.h>
 #include <Wire.h>
#endif

typedef struct si7021_env {
    int celsiusHundredths;
    int fahrenheitHundredths;
    unsigned int humidityPercent;
} si7021_env;

class SI7021
{
  public:
    SI7021();
    bool begin();
    bool sensorExists();
    int getFahrenheitHundredths();
    int getCelsiusHundredths();
    unsigned int getHumidityPercent();
    unsigned int getHumidityBasisPoints();
    struct si7021_env getHumidityAndTemperature();
    int getSerialBytes(byte * buf);
    int getDeviceId();
    void setHeater(bool on);
  private:
    void _command(byte * cmd, byte * buf );
    void _writeReg(byte * reg, int reglen);
    int _readReg(byte * reg, int reglen);
    int _getCelsiusPostHumidity();
};

#endif

