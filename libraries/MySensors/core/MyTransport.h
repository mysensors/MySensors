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

#ifndef MyTransport_h
#define MyTransport_h

#include <stdint.h>
#include "MySensorCore.h"

// Size of each firmware block
#define FIRMWARE_BLOCK_SIZE	16
// Number of times a firmware block should be requested before giving up
#define FIRMWARE_MAX_REQUESTS 5
// Number of times to request a fw block before giving up
#define MY_OTA_RETRY 5
// Number of millisecons before re-request a fw block
#define MY_OTA_RETRY_DELAY 500
// Start offset for firmware in flash (DualOptiboot wants to keeps a signature first)
#define FIRMWARE_START_OFFSET 10
// Bootloader version
#define MY_OTA_BOOTLOADER_MAJOR_VERSION 3
#define MY_OTA_BOOTLOADER_MINOR_VERSION 0
#define MY_OTA_BOOTLOADER_VERSION (MY_OTA_BOOTLOADER_MINOR_VERSION * 256 + MY_OTA_BOOTLOADER_MAJOR_VERSION)

// Search for a new parent node after this many transmission failures
#define SEARCH_FAILURES  5

// Status when waiting for signing nonce in process
enum { SIGN_WAITING_FOR_NONCE = 0, SIGN_OK = 1 };


// FW config structure, stored in eeprom
typedef struct {
	uint16_t type;
	uint16_t version;
	uint16_t blocks;
	uint16_t crc;
} __attribute__((packed)) NodeFirmwareConfig;

typedef struct {
	uint16_t type;
	uint16_t version;
	uint16_t blocks;
	uint16_t crc;
	uint16_t BLVersion;
} __attribute__((packed)) RequestFirmwareConfig;

typedef struct {
	uint16_t type;
	uint16_t version;
	uint16_t block;
} __attribute__((packed)) RequestFWBlock;

typedef struct {
	uint16_t type;
	uint16_t version;
	uint16_t block;
	uint8_t data[FIRMWARE_BLOCK_SIZE];
} __attribute__((packed)) ReplyFWBlock;



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
