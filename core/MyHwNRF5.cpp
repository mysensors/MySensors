/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list:
 * https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwNRF5.h"

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

void hwReadConfigBlock(void *buf, void *adr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(adr);
	(void)NVRAM.read_block(dst, offs, length);
}

void hwWriteConfigBlock(void *buf, void *adr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(adr);
	// use update() instead of write() to reduce e2p wear off
	(void)NVRAM.write_block(src, offs, length);
}

uint8_t hwReadConfig(int adr)
{
	return NVRAM.read(adr);
}

void hwWriteConfig(int adr, uint8_t value)
{
	(void)NVRAM.write(adr, value);
}

bool hwInit()
{
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {
	}
#endif
	return true;
}

void hwRandomNumberInit()
{
	// Start HWRNG
#ifdef NRF51
	NRF_RNG->POWER = 1;
#endif
	NRF_RNG->TASKS_START = 1;
	NRF_RNG->EVENTS_VALRDY = 0;
	NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos;

	uint32_t seed = 0;
	for (uint8_t i = 0; i < 4; i++) {
		// Wait for an random number
		while (NRF_RNG->EVENTS_VALRDY == 0) {
			yield();
		}
		seed = (seed << 8) | (uint32_t)NRF_RNG->VALUE;
		NRF_RNG->EVENTS_VALRDY = 0;
	}
	randomSeed(seed);

	// Stop HWRNG
	NRF_RNG->TASKS_STOP = 1;
#ifdef NRF51
	NRF_RNG->POWER = 0;
#endif
}

void hwWatchdogReset()
{
	NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void hwReboot()
{
	NVIC_SystemReset();
	while (true)
		;
}

int8_t hwSleep(unsigned long ms)
{
	// TODO: Not supported!
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms)
{
	// TODO: Not supported!
	(void)interrupt;
	(void)mode;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2,
               uint8_t mode2, unsigned long ms)
{
	// TODO: Not supported!
	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

#if defined(MY_DEBUG) || defined(MY_SPECIAL_DEBUG)
uint16_t hwCPUVoltage()
{
	return 0;
}

uint16_t hwCPUFrequency()
{
	return F_CPU / 100000UL;
}

uint16_t hwFreeMem()
{
	// TODO: Not supported!
	return 0;
}
#endif

#ifdef MY_DEBUG
void hwDebugPrint(const char *fmt, ...)
{
	if (MY_SERIALDEVICE) {
		char fmtBuffer[MY_SERIAL_OUTPUT_SIZE];
#ifdef MY_GATEWAY_FEATURE
		// prepend debug message to be handled correctly by controller (C_INTERNAL,
		// I_LOG_MESSAGE)
		snprintf(fmtBuffer, sizeof(fmtBuffer), PSTR("0;255;%d;0;%d;"), C_INTERNAL,
		         I_LOG_MESSAGE);
		MY_SERIALDEVICE.print(fmtBuffer);
#endif
		va_list args;
		va_start(args, fmt);
#ifdef MY_GATEWAY_FEATURE
		// Truncate message if this is gateway node
		vsnprintf(fmtBuffer, sizeof(fmtBuffer), fmt, args);
		fmtBuffer[sizeof(fmtBuffer) - 2] = '\n';
		fmtBuffer[sizeof(fmtBuffer) - 1] = '\0';
#else
		vsnprintf(fmtBuffer, sizeof(fmtBuffer), fmt, args);
#endif
		va_end(args);
		MY_SERIALDEVICE.print(fmtBuffer);
		//	MY_SERIALDEVICE.flush();

		// MY_SERIALDEVICE.write(freeRam());
	}
}
#endif
