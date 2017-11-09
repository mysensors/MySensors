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
* Radio wiring Teensy3.x: RF24, RFM69, RFM95:
* MISO	12
* MOSI	11
* SCK	13
* CSN	10
* CE	9 (RF24)
* IRQ	8 (opt. RF24, RFM69, RFM95)
*/

#ifndef MyHwTeensy3_h
#define MyHwTeensy3_h

#ifdef __cplusplus
#include <Arduino.h>
#endif
#include "util/atomic.h"

#ifndef _BV
#define _BV(x) (1<<(x))
#endif

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define RNG_CR_GO_MASK			0x1u
#define RNG_CR_HA_MASK			0x2u
#define RNG_CR_INTM_MASK		0x4u
#define RNG_CR_CLRI_MASK		0x8u
#define RNG_CR_SLP_MASK			0x10u
#define RNG_SR_OREG_LVL_MASK	0xFF00u
#define RNG_SR_OREG_LVL_SHIFT	8
#define RNG_SR_OREG_LVL(x)		(((uint32_t)(((uint32_t)(x))<<RNG_SR_OREG_LVL_SHIFT))&RNG_SR_OREG_LVL_MASK)
#define SIM_SCGC6_RNGA			((uint32_t)0x00000200)
#endif

#define MIN(a,b) min(a,b)
#define MAX(a,b) max(a,b)

// Define these as macros to save valuable space
#define hwDigitalWrite(__pin, __value) digitalWriteFast(__pin, __value)
#define hwDigitalRead(__pin) digitalReadFast(__pin)
#define hwPinMode(__pin, __value) pinMode(__pin, __value)
#define hwMillis() millis()

void hwRandomNumberInit(void);
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

#if defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define MY_HW_HAS_GETENTROPY
#endif

#define MY_CRITICAL_SECTION ATOMIC_BLOCK(ATOMIC_RESTORESTATE)

#endif
