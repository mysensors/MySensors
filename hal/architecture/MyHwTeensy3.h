/**
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2017 Sensnology AB
* Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* Radio wiring: Teensy3.x / LC
* MISO	12
* MOSI	11
* SCK	13
* CSN	10
* CE	9 (default)
*/

#ifndef MyHwTeensy3_h
#define MyHwTeensy3_h

#include "MyHw.h"
#ifdef __cplusplus
#include <Arduino.h>
#endif
#include "util/atomic.h"

#ifndef _BV
#define _BV(x) (1<<(x))
#endif

// Define these as macros to save valuable space
#define hwDigitalWrite(__pin, __value) digitalWriteFast(__pin, __value)
#define hwDigitalRead(__pin) digitalReadFast(__pin)
#define hwPinMode(__pin, __value) pinMode(__pin, __value)
#define hwMillis() millis()

// TODO: use hardware random number generator if Teensy 3.5 or 3.6
#define hwRandomNumberInit() randomSeed(analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN))

bool hwInit(void);
void hwWatchdogReset(void);
void hwReboot(void);

// Teensy 3.x implements avr-libc EEPROM API
#define hwReadConfig(__pos) eeprom_read_byte((uint8_t*)(__pos))
#define hwWriteConfig(__pos, __val) eeprom_update_byte((uint8_t*)(__pos), (__val))
#define hwReadConfigBlock(__buf, __pos, __length) eeprom_read_block((void*)(__buf), (void*)(__pos), (__length))
#define hwWriteConfigBlock(__buf, __pos, __length) eeprom_update_block((void*)(__buf), (void*)(__pos), (__length))

#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

#define MY_CRITICAL_SECTION ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

#endif