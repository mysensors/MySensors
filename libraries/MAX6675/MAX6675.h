/*
  MAX6675.h - Library for reading temperature from a MAX6675.

  This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License.
  http://creativecommons.org/licenses/by-sa/3.0/
*/

#ifndef MAX6675_h
#define MAX6675_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

class MAX6675
{
  public:
    MAX6675(uint8_t CS_pin, uint8_t SO_pin, uint8_t SCK_pin, uint8_t units);
    float read_temp();
  private:
    uint8_t _CS_pin;
    uint8_t _SO_pin;
    uint8_t _SCK_pin;
    uint8_t _units;
    uint8_t chip_read(uint8_t CS_pin, uint8_t &error_tc);
};

#endif
