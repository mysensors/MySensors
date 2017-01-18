/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2016 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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
* |E| SYS	| SUB	| Message									| Comment
* |-|------|-------|-------------------------------------------|----------------------------------------------------------------------------
* | | OTA  | FWP	| UPDATE									| FW update initiated
* |!| OTA  | FWP	| FLASH INIT FAIL							| Failed to initialise flash
* | | OTA  | FWP	| UPDATE SKIPPED							| FW update skipped, no newer version available
* | | OTA  | FWP	| RECV B=%04X								| Received FW block (B)
* |!| OTA  | FWP	| WRONG FWB									| Wrong FW block received
* | | OTA  | FWP	| FW END									| FW received, proceed to CRC verification
* | | OTA  | FWP	| CRC OK									| FW CRC verification OK
* |!| OTA  | FWP	| CRC FAIL									| FW CRC verification failed
* | | OTA  | FRQ	| FW REQ,T=%04X,V=%04X,B=%04X				| Request FW update, FW type (T), version (V), block (B)
* |!| OTA  | FRQ	| FW UPD FAIL								| FW update failed
* | | OTA  | CRC	| B=%04X,C=%04X,F=%04X						| FW CRC verification. FW blocks (B), calculated CRC (C), FW CRC (F)
*
*
* @brief API declaration for MyOTAFirmwareUpdate
*/


#ifndef MyOTAFirmwareUpdate_h
#define MyOTAFirmwareUpdate_h

#include "MySensorsCore.h"

#define FIRMWARE_BLOCK_SIZE		(16u)				//!< Size of each firmware block
#define FIRMWARE_MAX_REQUESTS	(5u)				//!< Number of times a firmware block should be requested before giving up
#define MY_OTA_RETRY			(5u)				//!< Number of times to request a fw block before giving up
#define MY_OTA_RETRY_DELAY		(500u)				//!< Number of milliseconds before re-requesting a FW block
#define FIRMWARE_START_OFFSET	(10u)				//!< Start offset for firmware in flash (DualOptiboot wants to keeps a signature first)

#define MY_OTA_BOOTLOADER_MAJOR_VERSION (3u)		//!< Bootloader version major
#define MY_OTA_BOOTLOADER_MINOR_VERSION (0u)		//!< Bootloader version minor
#define MY_OTA_BOOTLOADER_VERSION (MY_OTA_BOOTLOADER_MINOR_VERSION * 256 + MY_OTA_BOOTLOADER_MAJOR_VERSION)	//!< Bootloader version

#if defined(MY_DEBUG)
#define OTA_DEBUG(x,...) hwDebugPrint(x, ##__VA_ARGS__)	//!< debug
#else
#define OTA_DEBUG(x,...)	//!< debug NULL
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
 * @brief Read firmware settings from EEPROM
 *
 * Current firmware settings (type, version, crc, blocks) are read into _fc
 */
void readFirmwareSettings(void);
/**
 * @brief Handle OTA FW update requests
 */
void firmwareOTAUpdateRequest(void);
/**
 * @brief Handle OTA FW update responses
 *
 * This function handles incoming OTA FW packets and stores them to external flash (Sensebender)
 */
bool firmwareOTAUpdateProcess(void);
/**
 * @brief Validate uploaded FW CRC
 *
 * This function verifies if uploaded FW CRC is valid
 */
bool transportIsValidFirmware(void);
/**
 * @brief Present bootloader/FW information upon startup
 */
void presentBootloaderInformation(void);

#endif

/** @}*/
