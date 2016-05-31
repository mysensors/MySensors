/*
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

#ifndef MyTransport_h
#define MyTransport_h

#include <stdint.h>
#include "MySensorCore.h"

// Search for a new parent node after this many transmission failures
#define SEARCH_FAILURES  5

#define AUTO 0xFF // 0-254. Id 255 is reserved for auto initialization of nodeId.

#define BROADCAST_ADDRESS ((uint8_t)0xFF)

// invalid distance when searching for parent
#define DISTANCE_INVALID (0xFF)

// Common functions in all radio drivers
#ifdef MY_OTA_FIRMWARE_FEATURE
	// do a crc16 on the whole received firmware
	bool transportIsValidFirmware();
#endif


void transportProcess();
void transportRequestNodeId();
void transportPresentNode();
void transportFindParentNode();
boolean transportSendRoute(MyMessage &message);
boolean transportSendWrite(uint8_t to, MyMessage &message);

// "Interface" functions for radio driver
bool transportInit();
void transportSetAddress(uint8_t address);
uint8_t transportGetAddress();
bool transportSend(uint8_t to, const void* data, uint8_t len);
bool transportAvailable(uint8_t *to);
uint8_t transportReceive(void* data);
void transportPowerDown();

#endif
