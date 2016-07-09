/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
#ifndef MyHwESP8266_h
#define MyHwESP8266_h

#include "MyHw.h"

#ifdef __cplusplus
#include <Arduino.h>
#endif

#define MY_SERIALDEVICE Serial
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))


// Define these as macros to save valuable space

#define hwDigitalWrite(__pin, __value) (digitalWrite(__pin, __value))
#define hwInit() MY_SERIALDEVICE.begin(MY_BAUD_RATE); MY_SERIALDEVICE.setDebugOutput(true)
#define hwWatchdogReset() wdt_reset()
#define hwReboot() ESP.restart();
#define hwMillis() millis()

void hwReadConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfig(int adr, uint8_t value);
uint8_t hwReadConfig(int adr);


#endif // #ifdef ARDUINO_ARCH_ESP8266
