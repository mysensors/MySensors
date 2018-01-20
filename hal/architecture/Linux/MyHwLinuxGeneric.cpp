/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwLinuxGeneric.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>
#include <unistd.h>
#include "SoftEeprom.h"
#include "log.h"

static SoftEeprom eeprom = SoftEeprom(MY_LINUX_CONFIG_FILE, 1024);	// ATMega328 has 1024 bytes
static FILE *randomFp = NULL;

bool hwInit(void)
{
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#ifdef MY_GATEWAY_SERIAL
#ifdef MY_LINUX_SERIAL_GROUPNAME
	if (!MY_SERIALDEVICE.setGroupPerm(MY_LINUX_SERIAL_GROUPNAME)) {
		logError("Unable to change permission for serial port device.\n");
		exit(1);
	}
#endif
#endif

	return true;
}

void hwReadConfigBlock(void* buf, void* addr, size_t length)
{
	eeprom.readBlock(buf, addr, length);
}

void hwWriteConfigBlock(void* buf, void* addr, size_t length)
{
	eeprom.writeBlock(buf, addr, length);
}

uint8_t hwReadConfig(int addr)
{
	return eeprom.readByte(addr);
}

void hwWriteConfig(int addr, uint8_t value)
{
	eeprom.writeByte(addr, value);
}

void hwRandomNumberInit()
{
	uint32_t seed=0;

	if (randomFp != NULL) {
		fclose(randomFp);
	}
	if (!(randomFp = fopen("/dev/urandom", "r"))) {
		logError("Cannot open '/dev/urandom'.\n");
		exit(2);
	}

	while (hwGetentropy(&seed, sizeof(seed)) != sizeof(seed));
	randomSeed(seed);
}

ssize_t hwGetentropy(void *__buffer, size_t __length)
{
	return(fread(__buffer, 1, __length, randomFp));
}

uint32_t hwMillis()
{
	return millis();
}

bool hwUniqueID(unique_id_t *uniqueID)
{
	// not implemented yet
	(void)uniqueID;
	return false;
}

// Not supported!
int8_t hwSleep(uint32_t ms)
{
	(void)ms;

	return MY_SLEEP_NOT_POSSIBLE;
}

// Not supported!
int8_t hwSleep(uint8_t interrupt, uint8_t mode, uint32_t ms)
{
	(void)interrupt;
	(void)mode;
	(void)ms;

	return MY_SLEEP_NOT_POSSIBLE;
}

// Not supported!
int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2,
               uint32_t ms)
{
	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;

	return MY_SLEEP_NOT_POSSIBLE;
}

uint16_t hwCPUVoltage()
{
	// TODO: Not supported!
	return FUNCTION_NOT_SUPPORTED;
}

uint16_t hwCPUFrequency()
{
	// TODO: Not supported!
	return FUNCTION_NOT_SUPPORTED;
}

uint16_t hwFreeMem()
{
	// TODO: Not supported!
	return FUNCTION_NOT_SUPPORTED;
}

void hwDigitalWrite(uint8_t pin, uint8_t value)
{
	digitalWrite(pin, value);
}

int hwDigitalRead(uint8_t pin)
{
	return digitalRead(pin);
}

void hwPinMode(uint8_t pin, uint8_t mode)
{
	pinMode(pin, mode);
}

void hwDebugPrint(const char *fmt, ...)
{
#ifndef MY_DISABLED_SERIAL
#ifdef MY_DEBUGDEVICE
	char fmtBuffer[MY_SERIAL_OUTPUT_SIZE];
#ifdef MY_GATEWAY_SERIAL
	// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
	snprintf_P(fmtBuffer, sizeof(fmtBuffer), PSTR("0;255;%" PRIu8 ";0;%" PRIu8 ";%" PRIu32 " "),
	           C_INTERNAL, I_LOG_MESSAGE, hwMillis());
	MY_DEBUGDEVICE.print(fmtBuffer);
#else
	// prepend timestamp
	MY_DEBUGDEVICE.print(hwMillis());
	MY_DEBUGDEVICE.print(" ");
#endif
	va_list args;
	va_start (args, fmt );
	vsnprintf_P(fmtBuffer, sizeof(fmtBuffer), fmt, args);
#ifdef MY_GATEWAY_SERIAL
	// Truncate message if this is gateway node
	fmtBuffer[sizeof(fmtBuffer) - 2] = '\n';
	fmtBuffer[sizeof(fmtBuffer) - 1] = '\0';
#endif
	va_end (args);
	MY_DEBUGDEVICE.print(fmtBuffer);
	MY_DEBUGDEVICE.flush();
#else
	va_list args;
	va_start(args, fmt);
	vlogDebug(fmt, args);
	va_end(args);
#endif
#else
	(void)fmt;
#endif
}
