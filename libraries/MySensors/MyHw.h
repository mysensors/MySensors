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

#ifndef MyHw_h
#define MyHw_h

#include <stdint.h>
#include <stddef.h>
#include "MyConfig.h"

/**
 * The Hw contains all hardware specific details of the platform
 */

class MyHw
{ 
public:
	// MyHw constructor
	MyHw();
/*	// The following is defined as macros to save space
  	virtual void init() = 0;
	virtual void watchdogReset() = 0;
	virtual void reboot() = 0;
	virtual unsigned long millisec() = 0;
	virtual uint8_t readConfig(uint8_t pos) = 0;
	virtual void writeConfig(uint8_t pos, uint8_t value) = 0;
	virtual void readConfigBlock(void* buf, void * pos, size_t length) = 0;
	virtual void writeConfigBlock(void* pos, void* buf, uint16_t length) = 0; */
	virtual void sleep(unsigned long ms) = 0;
	virtual bool sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) = 0;
	virtual uint8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) = 0;
	#ifdef DEBUG
	virtual void debugPrint(bool isGW, const char *fmt, ... ) = 0;
	#endif

};

#endif
