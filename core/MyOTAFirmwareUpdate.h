/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
* @file MyOTAFirmwareUpdate.h
*
* @defgroup MyOTAFirmwaregrp MyOTAFirmwareUpdate
* @ingroup internals
* @{
*
* MyOTAFirmwareUpdate-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
* - [!] Exclamation mark is prepended in case of error or warning
* - SYSTEM:
*  - <b>OTA</b> messages emitted by MyOTAFirmwareUpdate
* - SUB SYSTEMS:
*  - OTA:<b>FRQ</b>	from @ref firmwareOTAUpdateRequest()
*  - OTA:<b>FWP</b>	from @ref firmwareOTAUpdateProcess()
*
* MyOTAFirmwareUpdate debug log messages:
*
* |E| SYS | SUB | Message                     | Comment
* |-|-----|-----|-----------------------------|----------------------------------------------------------------------------
* | | OTA | FWP | UPDATE                      | FW update initiated
* |!| OTA | FWP | UPDO                        | FW config response received, FW update already ongoing
* |!| OTA | FWP | FLASH INIT FAIL             | Failed to initialise flash
* | | OTA | FWP | UPDATE SKIPPED              | FW update skipped, no newer version available
* | | OTA | FWP | RECV B=%04X                 | Received FW block (B)
* |!| OTA | FWP | WRONG FWB                   | Wrong FW block received
* | | OTA | FWP | FW END                      | FW received, proceed to CRC verification
* | | OTA | FWP | CRC OK                      | FW CRC verification OK
* |!| OTA | FWP | CRC FAIL                    | FW CRC verification failed
* | | OTA | FRQ | FW REQ,T=%04X,V=%04X,B=%04X | Request FW update, FW type (T), version (V), block (B)
* |!| OTA | FRQ | FW UPD FAIL                 | FW update failed
* | | OTA | CRC | B=%04X,C=%04X,F=%04X        | FW CRC verification. FW blocks (B), calculated CRC (C), FW CRC (F)
*
*
* @brief API declaration for MyOTAFirmwareUpdate
*/


#ifndef MyOTAFirmwareUpdate_h
#define MyOTAFirmwareUpdate_h

#include "MySensorsCore.h"
#ifdef MCUBOOT_PRESENT
#include "generated_dts_board.h"
#define FIRMWARE_PROTOCOL_31
#endif

#define LOCAL static //!< static

#if MAX_PAYLOAD >= 22
#define FIRMWARE_BLOCK_SIZE		(16u)				//!< Size of each firmware block
#else
#define FIRMWARE_BLOCK_SIZE		(8u)				//!< Size of each firmware block
#ifndef FIRMWARE_PROTOCOL_31
#define FIRMWARE_PROTOCOL_31
#endif
#endif
#ifndef MY_OTA_RETRY
#define MY_OTA_RETRY			(5u)				//!< Number of times to request a fw block before giving up
#endif
#ifndef MY_OTA_RETRY_DELAY
#define MY_OTA_RETRY_DELAY		(500u)				//!< Number of milliseconds before re-requesting a FW block
#endif
#ifndef MCUBOOT_PRESENT
#define FIRMWARE_START_OFFSET	(10u)				//!< Start offset for firmware in flash (DualOptiboot wants to keeps a signature first)
#else
#define	FIRMWARE_START_OFFSET	(FLASH_AREA_IMAGE_1_OFFSET_0)	//!< Use offset from generated_dts_board.h (mcuboot)
#endif

#define MY_OTA_BOOTLOADER_MAJOR_VERSION (3u)		//!< Bootloader version major
#ifdef FIRMWARE_PROTOCOL_31
#define MY_OTA_BOOTLOADER_MINOR_VERSION (1u)		//!< Bootloader version minor
#else
#define MY_OTA_BOOTLOADER_MINOR_VERSION (0u)		//!< Bootloader version minor
#endif
#define MY_OTA_BOOTLOADER_VERSION (MY_OTA_BOOTLOADER_MINOR_VERSION * 256 + MY_OTA_BOOTLOADER_MAJOR_VERSION)	//!< Bootloader version

#if defined(MY_DEBUG_VERBOSE_OTA_UPDATE)
#define OTA_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug
//#define OTA_EXTRA_FLASH_DEBUG	//!< Dumps flash after each FW block
#else
#define OTA_DEBUG(x,...)	//!< debug NULL
#endif

#if defined(DOXYGEN) && !defined(FIRMWARE_PROTOCOL_31)
/**
 * @brief Enabled FOTA 3.1 protocol extensions
 *
 * Supports smaller FIRMWARE_BLOCK_SIZE, RLE and NVM for nRF5 with mcuboot. The
 * extension is enabled per default when mcuboot is present or full FIRMWARE_BLOCK_SIZE
 * exeeds MAX_PAYLOAD.
 */
#define FIRMWARE_PROTOCOL_31
#endif

/**
* @brief FW config structure, stored in eeprom
*/
typedef struct {
	uint16_t type;								//!< Type of config
	uint16_t version;							//!< Version of config
	uint16_t blocks;							//!< Number of blocks
	uint16_t crc;								//!< CRC of block data
} __attribute__((packed)) nodeFirmwareConfig_t;

/**
* @brief FW config request structure
*/
typedef struct {
	uint16_t type;								//!< Type of config
	uint16_t version;							//!< Version of config
	uint16_t blocks;							//!< Number of blocks
	uint16_t crc;								//!< CRC of block data
	uint16_t BLVersion;							//!< Bootloader version
#ifdef FIRMWARE_PROTOCOL_31
	uint8_t  blockSize;							//!< Blocksize, when protocol version >= 3.1 is reported. Otherwhise the blocksize is 16
	uint8_t  img_commited;							//!< mcuboot image_ok attribute commited firmware=0x01(mcuboot)|0x02(DualOptiboot), when protocol version >= 3.1 is reported
	uint16_t img_revision;							//!< mcuboot revision attribute, when protocol version >= 3.1 is reported
	uint32_t img_build_num;							//!< mcuboot build_num attribute, when protocol version >= 3.1 is reported
#endif
} __attribute__((packed)) requestFirmwareConfig_t;

/**
* @brief FW block request structure
*/
typedef struct {
	uint16_t type;								//!< Type of config
	uint16_t version;							//!< Version of config
	uint16_t block;								//!< Block index
} __attribute__((packed)) requestFirmwareBlock_t;

/**
* @brief  FW block reply structure
*/
typedef struct {
	uint16_t type;								//!< Type of config
	uint16_t version;							//!< Version of config
	uint16_t block;								//!< Block index
	uint8_t data[FIRMWARE_BLOCK_SIZE];			//!< Block data
} __attribute__((packed)) replyFirmwareBlock_t;

/**
* @brief  FW block reply structure (RLE)
*/
typedef struct {
	uint16_t type;								//!< Type of config
	uint16_t version;							//!< Version of config
	uint16_t block;								//!< Block index
	uint16_t number_of_blocks;						//!< Number of blocks to fill with data
	uint8_t data;								//!< Block data
} __attribute__((packed)) replyFirmwareBlockRLE_t;

/**
 * @brief Read firmware settings from EEPROM
 *
 * Current firmware settings (type, version, crc, blocks) are read into _fc
 */
LOCAL void readFirmwareSettings(void);
/**
 * @brief Handle OTA FW update requests
 */
LOCAL void firmwareOTAUpdateRequest(void);
/**
 * @brief Handle OTA FW update responses
 *
 * This function handles incoming OTA FW packets and stores them to external flash (Sensebender)
 */
LOCAL bool firmwareOTAUpdateProcess(void);
/**
 * @brief Validate uploaded FW CRC
 *
 * This function verifies if uploaded FW CRC is valid
 */
LOCAL bool transportIsValidFirmware(void);
/**
 * @brief Present bootloader/FW information upon startup
 */
LOCAL void presentBootloaderInformation(void);

#endif

/** @}*/
