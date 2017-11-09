/*
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
 */

#ifndef MyHwSTM32F1_h
#define MyHwSTM32F1_h

#include <libmaple/iwdg.h>
#include <itoa.h>
#include <EEPROM.h>
#ifdef __cplusplus
#include <Arduino.h>
#endif

// SS default
#ifndef SS
#define SS PA4
#endif

// mapping
#define snprintf_P snprintf
#define vsnprintf_P vsnprintf
#define strncpy_P strncpy
#define printf_P printf
#define yield()				  // not defined

#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

#define MIN(a,b) min(a,b)
#define MAX(a,b) max(a,b)

#ifndef digitalPinToInterrupt
#define digitalPinToInterrupt(__pin) (__pin)
#endif

#define hwDigitalWrite(__pin, __value) digitalWrite(__pin, __value)
#define hwDigitalRead(__pin) digitalRead(__pin)
#define hwPinMode(__pin, __value) pinMode(__pin, __value)
#define hwWatchdogReset() iwdg_feed()
#define hwReboot() nvic_sys_reset()
#define hwMillis() millis()

extern void serialEventRun(void) __attribute__((weak));
//void (*serialEventRun)() = NULL;
bool hwInit(void);
void hwRandomNumberInit(void);
void hwReadConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfigBlock(void* buf, void* adr, size_t length);
void hwWriteConfig(const int addr, uint8_t value);
uint8_t hwReadConfig(const int addr);

#ifndef DOXYGEN
#define MY_CRITICAL_SECTION
#endif  /* DOXYGEN */

#endif
