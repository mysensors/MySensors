/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyleft (c) 2016, Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwLinuxGeneric.h"

#include <sys/time.h>
#include <stdarg.h>
#include <string.h>

static const size_t _length = 1024;	// ATMega328 has 1024 bytes
//TODO store _config to a file
static uint8_t _config[_length];	
static unsigned long millis_at_start;

void hwInit()
{
	timeval curTime;

	for (size_t i = 0; i < _length; i++)
		_config[i] = 0xFF;

	gettimeofday(&curTime, NULL);
	millis_at_start = curTime.tv_sec;
}

void hwReadConfigBlock(void* buf, void* addr, size_t length)
{
	unsigned long int offs = reinterpret_cast<unsigned long int>(addr);

	if (length && offs + length <= _length) {
		memcpy(buf, _config+offs, length);
	}
}

void hwWriteConfigBlock(void* buf, void* addr, size_t length)
{
	unsigned long int offs = reinterpret_cast<unsigned long int>(addr);

	if (length && offs + length <= _length) {
		memcpy(_config+offs, buf, length);
	}
}

uint8_t hwReadConfig(int addr)
{
	if (addr >= 0 && (unsigned)addr < _length)
		return _config[addr];
	return 0xFF;
}

void hwWriteConfig(int addr, uint8_t value)
{
	if (addr >= 0 && (unsigned)addr < _length)
		_config[addr] = value;
}

unsigned long hwMillis()
{
	timeval curTime;

	gettimeofday(&curTime, NULL);
	return ((curTime.tv_sec - millis_at_start) * 1000) + (curTime.tv_usec / 1000);
}

int8_t hwSleep(unsigned long ms)
{
	// TODO: Not supported!
	return -2;
}

int8_t hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms)
{
	// TODO: Not supported!
	return -2;
}

int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms)
{
  // TODO: Not supported!
	return -2;
}

#ifdef MY_DEBUG
void hwDebugPrint(const char *fmt, ... )
{
	va_list arglist;
	va_start(arglist, fmt);
	vprintf(fmt, arglist);
	va_end(arglist);
}
#endif
