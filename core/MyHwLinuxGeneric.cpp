/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyright (C) 2016 Marcelo Aquino
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

#include <sys/time.h>
#include <stdarg.h>
#include <string.h>
#include <fstream>
#include <sys/stat.h>

static const char* CONFIG_FILE = MY_LINUX_CONFIG_FILE;
static const size_t _length = 1024;	// ATMega328 has 1024 bytes
static uint8_t _config[_length];	
static unsigned long millis_at_start;

bool CheckConfigFile() {
    struct stat fileInfo;
	
	if(stat(CONFIG_FILE, &fileInfo) != 0)
	{
		//File does not exist.  Create it.
		debug("Config file %s does not exist, creating new config file.\n", CONFIG_FILE);
		ofstream myFile(CONFIG_FILE, ios::out | ios::binary);
		if(!myFile)
		{
			debug("Unable to create config file %s.\n", CONFIG_FILE);
			return false;
		}
		myFile.write((const char*)_config, _length);
		myFile.close();
	}
	else if(fileInfo.st_size < 0 || (size_t)fileInfo.st_size != _length)
	{
		debug("Config file %s is not the correct size of %i.  Please remove the file and a new one will be created.\n", CONFIG_FILE, _length);
		return false;
	}
	else
	{
		//Read config into local memory.
		ifstream myFile(CONFIG_FILE, ios::in | ios::binary);
		if(!myFile)
		{
			debug("Unable to open config to file %s for reading.\n", CONFIG_FILE);
			return false;
		}
		myFile.read((char*)_config, _length);
		myFile.close();
	}
	
	return true;
}

void hwInit()
{
	timeval curTime;

	for (size_t i = 0; i < _length; i++)
		_config[i] = 0xFF;
		
	if (!CheckConfigFile())
	{
		exit(1);
	}

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
		
		ofstream myFile(CONFIG_FILE, ios::out | ios::binary);
		if(!myFile)
		{
			debug("Unable to write config to file %s.\n", CONFIG_FILE);
			return;
		}
		myFile.write((const char*)buf+offs, _length);
		myFile.close();
	}
}

uint8_t hwReadConfig(int adr)
{
	uint8_t value = 0xFF;
	hwReadConfigBlock(&value, reinterpret_cast<void*>(adr), 1);
	return value;
}

void hwWriteConfig(int adr, uint8_t value)
{
	uint8_t curr = hwReadConfig(adr);
	if (curr != value)
	{
		hwWriteConfigBlock(&value, reinterpret_cast<void*>(adr), 1);
	}
}

unsigned long hwMillis()
{
	timeval curTime;

	gettimeofday(&curTime, NULL);
	return ((curTime.tv_sec - millis_at_start) * 1000) + (curTime.tv_usec / 1000);
}

// TODO: Not supported!
int8_t hwSleep(unsigned long ms)
{
	(void)ms;

	return -2;
}

// TODO: Not supported!
int8_t hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms)
{
	(void)interrupt;
	(void)mode;
	(void)ms;

	return -2;
}

// TODO: Not supported!
int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms)
{
	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;
	
	return -2;
}

uint16_t hwCPUVoltage() {
	// TODO: Not supported!
	return 0;
}
 
uint16_t hwCPUFrequency() {
	// TODO: Not supported!
	return 0;
}
 
uint16_t hwFreeMem() {
	// TODO: Not supported!
	return 0;
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
