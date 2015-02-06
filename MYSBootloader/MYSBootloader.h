#ifndef MyOtaBootloader_H
#define MyOtaBootloader_H

#include <avr/power.h>
#include <stdint.h>
#include <string.h>
#include <util/crc16.h>

#include "MyMessage.h"
#include "MySensor.h"
#include "boot.h"
#include "MYSBootloaderRF24.h"


#define FIRMWARE_BLOCK_SIZE	16

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

static struct NodeConfig nc;
static NodeFirmwareConfig fc;
static MyMessage outMsg;
static MyMessage inMsg;

#ifdef PRESCALE
static clock_div_t orgClockDiv = 0;
#endif

static uint8_t progBuf[SPM_PAGESIZE];

#endif // MyOtaBootloader_H