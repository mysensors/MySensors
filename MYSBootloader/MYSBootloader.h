#ifndef MyOtaBootloader_H
#define MyOtaBootloader_H

#include <avr/power.h>
#include <stdint.h>
#include <string.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "MyMessage.h"
#include "MySensor.h"
#include "boot.h"
#include "MYSBootloaderRF24.h"


#define FIRMWARE_BLOCK_SIZE	16
#define EEPROM_SIZE 1024			// 1024 bytes for ATMEGA328

typedef struct {
	union {
		uint16_t type;
		uint16_t bl_command;
	};
	union {
		uint16_t version;
		uint16_t bl_data
	};
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

static struct NodeConfig nc;
static NodeFirmwareConfig fc;
static MyMessage outMsg;
static MyMessage inMsg;

static boolean configuredParentFound = false;
static uint8_t configuredParentID = 0xFF;

//static clock_div_t orgClockDiv = 0;

static uint8_t progBuf[SPM_PAGESIZE];

#endif // MyOtaBootloader_H