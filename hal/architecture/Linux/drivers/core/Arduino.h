/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef Arduino_h
#define Arduino_h

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>
#include <algorithm>
#include "stdlib_noniso.h"

#ifdef LINUX_ARCH_RASPBERRYPI
#include "RPi.h"
#define pinMode(pin, direction) RPi.pinMode(pin, direction)
#define digitalWrite(pin, value) RPi.digitalWrite(pin, value)
#define digitalRead(pin) RPi.digitalRead(pin)
#define digitalPinToInterrupt(pin) RPi.digitalPinToInterrupt(pin)
#else
#include "GPIO.h"
#define pinMode(pin, direction) GPIO.pinMode(pin, direction)
#define digitalWrite(pin, value) GPIO.digitalWrite(pin, value)
#define digitalRead(pin) GPIO.digitalRead(pin)
#define digitalPinToInterrupt(pin) GPIO.digitalPinToInterrupt(pin)
#endif

#include "interrupt.h"

#undef PSTR
#define PSTR(x) (x)
#undef F
#define F(x) (x)
#define PROGMEM __attribute__(( section(".progmem.data") ))
#define vsnprintf_P(...) vsnprintf( __VA_ARGS__ )
#define snprintf_P(...) snprintf( __VA_ARGS__ )
#define memcpy_P memcpy
#define pgm_read_byte(p) (*(p))
#define pgm_read_dword(p) (*(p))
#define pgm_read_byte_near(p) (*(p))

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define bit(b) (1UL << (b))

#define GET_MACRO(_0, _1, _2, NAME, ...) NAME
#define random(...) GET_MACRO(_0, ##__VA_ARGS__, randMinMax, randMax, rand)(__VA_ARGS__)

#ifndef delay
#define delay _delay_milliseconds
#endif

#ifndef delayMicroseconds
#define delayMicroseconds _delay_microseconds
#endif

using std::string;
using std::min;
using std::max;
using std::abs;

typedef uint8_t byte;
typedef string String;
typedef char __FlashStringHelper;

void yield(void);
unsigned long millis(void);
unsigned long micros(void);
void _delay_milliseconds(unsigned int millis);
void _delay_microseconds(unsigned int micro);
void randomSeed(unsigned long seed);
long randMax(long howbig);
long randMinMax(long howsmall, long howbig);

#endif
