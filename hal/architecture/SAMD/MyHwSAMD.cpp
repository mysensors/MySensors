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
 */

#include "MyHwSAMD.h"


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


void hwReadConfigBlock(void* buf, void* adr, size_t length)
{
	uint8_t* dst = static_cast<uint8_t*>(buf);
	const int offs = reinterpret_cast<int>(adr);
	(void)eep.read(offs, dst, length);

}

void hwWriteConfigBlock(void* buf, void* adr, size_t length)
{
	uint8_t* src = static_cast<uint8_t*>(buf);
	const int offs = reinterpret_cast<int>(adr);
	// use update() instead of write() to reduce e2p wear off
	(void)eep.update(offs, src, length);
}

uint8_t hwReadConfig(int adr)
{
	return eep.read(adr);
}

void hwWriteConfig(int adr, uint8_t value)
{
	(void)eep.update(adr, value);
}

bool hwInit(void)
{
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {}
#endif

	const uint8_t eepInit = eep.begin(MY_EXT_EEPROM_TWI_CLOCK);
#if defined(SENSEBENDER_GW_SAMD_V1)
	// check connection to external EEPROM - only sensebender GW
	return eepInit==0;
#else
	(void)eepInit;
	return true;
#endif
}

void hwWatchdogReset(void)
{
	// TODO: Not supported!
}

void hwReboot(void)
{
	NVIC_SystemReset();
	while (true);
}

int8_t hwSleep(uint32_t ms)
{
	// TODO: Not supported!
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(uint8_t interrupt, uint8_t mode, uint32_t ms)
{
	// TODO: Not supported!
	(void)interrupt;
	(void)mode;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2,
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
	(void)memcpy((uint8_t*)uniqueID, (uint32_t *)0x0080A00C, 4);
	(void)memcpy((uint8_t*)uniqueID + 4, (uint32_t *)0x0080A040, 12);
	return true;
}

uint16_t hwCPUVoltage()
{

	// disable ADC
	while (ADC->STATUS.bit.SYNCBUSY);
	ADC->CTRLA.bit.ENABLE = 0x00;

	// internal 1V reference (default)
	analogReference(AR_INTERNAL1V0);
	// 12 bit resolution (default)
	analogWriteResolution(12);
	// MUXp 0x1B = SCALEDIOVCC/4 => connected to Vcc
	ADC->INPUTCTRL.bit.MUXPOS = 0x1B ;

	// enable ADC
	while (ADC->STATUS.bit.SYNCBUSY);
	ADC->CTRLA.bit.ENABLE = 0x01;
	// start conversion
	while (ADC->STATUS.bit.SYNCBUSY);
	ADC->SWTRIG.bit.START = 1;
	// clear the Data Ready flag
	ADC->INTFLAG.bit.RESRDY = 1;
	// start conversion again, since The first conversion after the reference is changed must not be used.
	while (ADC->STATUS.bit.SYNCBUSY);
	ADC->SWTRIG.bit.START = 1;

	// waiting for conversion to complete
	while (!ADC->INTFLAG.bit.RESRDY);
	const uint32_t valueRead = ADC->RESULT.reg;

	// disable ADC
	while (ADC->STATUS.bit.SYNCBUSY);
	ADC->CTRLA.bit.ENABLE = 0x00;

	return valueRead * 4;
}

uint16_t hwCPUFrequency()
{
	// TODO: currently reporting compile time frequency (in 1/10MHz)
	return F_CPU / 100000UL;
}

uint16_t hwFreeMem()
{
	// TODO: Not supported!
	return FUNCTION_NOT_SUPPORTED;
}

void hwDebugPrint(const char *fmt, ... )
{
#ifndef MY_DEBUGDEVICE
#define MY_DEBUGDEVICE MY_SERIALDEVICE
#endif
#ifndef MY_DISABLED_SERIAL
	if (MY_DEBUGDEVICE) {
		char fmtBuffer[MY_SERIAL_OUTPUT_SIZE];
#ifdef MY_GATEWAY_SERIAL
		// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
		snprintf(fmtBuffer, sizeof(fmtBuffer), PSTR("0;255;%" PRIu8 ";0;%" PRIu8 ";%" PRIu32 " "),
		         C_INTERNAL, I_LOG_MESSAGE, hwMillis());
		MY_DEBUGDEVICE.print(fmtBuffer);
#else
		// prepend timestamp
		MY_DEBUGDEVICE.print(hwMillis());
		MY_DEBUGDEVICE.print(" ");
#endif
		va_list args;
		va_start (args, fmt );
		vsnprintf(fmtBuffer, sizeof(fmtBuffer), fmt, args);
#ifdef MY_GATEWAY_SERIAL
		// Truncate message if this is gateway node
		fmtBuffer[sizeof(fmtBuffer) - 2] = '\n';
		fmtBuffer[sizeof(fmtBuffer) - 1] = '\0';
#endif
		va_end (args);
		MY_DEBUGDEVICE.print(fmtBuffer);
		//	MY_SERIALDEVICE.flush();
	}
#else
	(void)fmt;
#endif
}
