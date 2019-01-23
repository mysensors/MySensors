/**
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

#include "MyHwTeensy3.h"


/*
int8_t pinIntTrigger = 0;
void wakeUp()	 //place to send the interrupts
{
	pinIntTrigger = 1;
}
void wakeUp2()	 //place to send the second interrupts
{
	pinIntTrigger = 2;
}

// Watchdog Timer interrupt service routine. This routine is required
// to allow automatic WDIF and WDIE bit clearance in hardware.
ISR (WDT_vect)
{
	// WDIE & WDIF is cleared in hardware upon entering this ISR
	wdt_disable();
}
*/

bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {}
#endif
#endif
	return true;
}

void hwWatchdogReset(void)
{
	// TODO: Not supported!
}

void hwReboot(void)
{
	SCB_AIRCR = 0x05FA0004;
	while (true);
}

int8_t hwSleep(uint32_t ms)
{
	// TODO: Not supported!
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	// TODO: Not supported!
	(void)interrupt;
	(void)mode;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const uint8_t mode2,
               uint32_t ms)
{
	// TODO: Not supported!
	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

bool hwUniqueID(unique_id_t *uniqueID)
{
#if defined(__MKL26Z64__)
	(void)memcpy((uint8_t *)uniqueID, &SIM_UIDMH, 12);
	(void)memset((uint8_t *)uniqueID + 12, MY_HWID_PADDING_BYTE, 4);
#else
	(void)memcpy((uint8_t *)uniqueID, &SIM_UIDH, 16);
#endif
	return true;
}

uint16_t hwCPUVoltage(void)
{
	analogReference(DEFAULT);
	analogReadResolution(12);
	analogReadAveraging(32);
#if defined(__MK20DX128__) || defined(__MK20DX256__)
	// Teensy 3.0/3.1/3.2
	return 1195 * 4096 / analogRead(39);
#elif defined(__MK64FX512__) || defined(__MK66FX1M0__)
	// Teensy 3.6
	return 1195 * 4096 / analogRead(71);
#elif defined(__MKL26Z64__)
	// Teensy LC
	// not supported
	return FUNCTION_NOT_SUPPORTED;
#else
	// not supported
	return FUNCTION_NOT_SUPPORTED;
#endif
}

uint16_t hwCPUFrequency(void)
{
	// TODO: currently reporting compile time frequency (in 1/10MHz)
	return F_CPU / 100000UL;
}

int8_t hwCPUTemperature(void)
{
	return -127; // not implemented yet
}

uint16_t hwFreeMem(void)
{
	// TODO: Not supported!
	return FUNCTION_NOT_SUPPORTED;
}

#if defined(MY_HW_HAS_GETENTROPY)
ssize_t hwGetentropy(void *__buffer, const size_t __length)
{
	SIM_SCGC6 |= SIM_SCGC6_RNGA; // enable RNG
	RNG_CR &= ~RNG_CR_SLP_MASK;
	RNG_CR |= RNG_CR_HA_MASK; //high assurance, not needed
	size_t pos = 0;
	while (pos < __length) {
		RNG_CR |= RNG_CR_GO_MASK;
		while (!(RNG_SR & RNG_SR_OREG_LVL(0xF)));
		const uint32_t rndVar = RNG_OR;
		const uint8_t bsize = (__length - pos) > sizeof(rndVar) ? sizeof(rndVar) : (__length - pos);
		(void)memcpy((uint8_t *)__buffer + pos, &rndVar, bsize);
		pos += bsize;
	}
	SIM_SCGC6 &= ~SIM_SCGC6_RNGA; // disable RNG
	return pos;
}
#endif

void hwRandomNumberInit(void)
{
#if defined(MY_HW_HAS_GETENTROPY)
	// use HW RNG present on Teensy3.5/3.6
	// init RNG
	uint32_t seed = 0;
	hwGetentropy(&seed, sizeof(seed));
	randomSeed(seed);
#else
	randomSeed(analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN));
#endif
}
