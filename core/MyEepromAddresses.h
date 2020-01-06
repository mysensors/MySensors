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
* @file MyEepromAddresses.h
* @brief Eeprom addresses for MySensors library data
*
* @defgroup MyEepromAddressesgrp MyEepromAddresses
* @ingroup internals
* @{
*
*/


#ifndef MyEepromAddresses_h
#define MyEepromAddresses_h

// EEPROM variable sizes, in bytes
#define SIZE_NODE_ID						(1u)		//!< Size node ID
#define SIZE_PARENT_NODE_ID					(1u)		//!< Size parent node ID
#define SIZE_DISTANCE						(1u)		//!< Size GW distance
#define SIZE_ROUTES							(256u)	//!< Size routing table
#define SIZE_CONTROLLER_CONFIG				(23u)	//!< Size controller config
#define SIZE_PERSONALIZATION_CHECKSUM	(1u)  //!< Size personalization checksum
#define SIZE_FIRMWARE_TYPE					(2u)		//!< Size firmware type
#define SIZE_FIRMWARE_VERSION				(2u)		//!< Size firmware version
#define SIZE_FIRMWARE_BLOCKS				(2u)		//!< Size firmware blocks
#define SIZE_FIRMWARE_CRC					(2u)		//!< Size firmware CRC
#define SIZE_SIGNING_REQUIREMENT_TABLE		(32u)	//!< Size signing requirement table
#define SIZE_WHITELIST_REQUIREMENT_TABLE	(32u)	//!< Size whitelist requirement table
#define SIZE_SIGNING_SOFT_HMAC_KEY			(32u)	//!< Size soft signing HMAC key
#define SIZE_SIGNING_SOFT_SERIAL			(9u)		//!< Size soft signing serial
#define SIZE_RF_ENCRYPTION_AES_KEY			(16u)	//!< Size RF AES encryption key
#define SIZE_NODE_LOCK_COUNTER				(1u)		//!< Size node lock counter


/** @brief EEPROM start address */
#define EEPROM_START (0u)
/** @brief Address node ID */
#define EEPROM_NODE_ID_ADDRESS EEPROM_START
/** @brief Address parent node ID */
#define EEPROM_PARENT_NODE_ID_ADDRESS (EEPROM_NODE_ID_ADDRESS + SIZE_NODE_ID)
/** @brief Address distance to GW */
#define EEPROM_DISTANCE_ADDRESS (EEPROM_PARENT_NODE_ID_ADDRESS + SIZE_PARENT_NODE_ID)
/** @brief Address routing table */
#define EEPROM_ROUTES_ADDRESS (EEPROM_DISTANCE_ADDRESS + SIZE_DISTANCE)
/** @brief Address configuration bytes sent by controller */
#define EEPROM_CONTROLLER_CONFIG_ADDRESS (EEPROM_ROUTES_ADDRESS + SIZE_ROUTES)
/** @brief Personalization checksum (set by SecurityPersonalizer.ino) */
#define EEPROM_PERSONALIZATION_CHECKSUM_ADDRESS (EEPROM_CONTROLLER_CONFIG_ADDRESS + SIZE_CONTROLLER_CONFIG)
/** @brief Address firmware type */
#define EEPROM_FIRMWARE_TYPE_ADDRESS (EEPROM_PERSONALIZATION_CHECKSUM_ADDRESS + SIZE_PERSONALIZATION_CHECKSUM)
/** @brief Address firmware version */
#define EEPROM_FIRMWARE_VERSION_ADDRESS (EEPROM_FIRMWARE_TYPE_ADDRESS + SIZE_FIRMWARE_TYPE)
/** @brief Address firmware blocks */
#define EEPROM_FIRMWARE_BLOCKS_ADDRESS (EEPROM_FIRMWARE_VERSION_ADDRESS + SIZE_FIRMWARE_VERSION)
/** @brief Address firmware CRC */
#define EEPROM_FIRMWARE_CRC_ADDRESS (EEPROM_FIRMWARE_BLOCKS_ADDRESS + SIZE_FIRMWARE_BLOCKS)
/** @brief Address signing requirement table */
#define EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS (EEPROM_FIRMWARE_CRC_ADDRESS + SIZE_FIRMWARE_CRC)
/** @brief Address whitelist requirement table */
#define EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS (EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS + SIZE_SIGNING_REQUIREMENT_TABLE)
/** @brief Address soft signing HMAC key. This is set with @ref SecurityPersonalizer.ino */
#define EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS (EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS + SIZE_WHITELIST_REQUIREMENT_TABLE)
/** @brief Address soft signing serial key. This is set with @ref SecurityPersonalizer.ino */
#define EEPROM_SIGNING_SOFT_SERIAL_ADDRESS (EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS + SIZE_SIGNING_SOFT_HMAC_KEY)
/** @brief Address RF AES encryption key. This is set with @ref SecurityPersonalizer.ino */
#define EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS (EEPROM_SIGNING_SOFT_SERIAL_ADDRESS + SIZE_SIGNING_SOFT_SERIAL)
/** @brief Address node lock counter. This is set with @ref SecurityPersonalizer.ino */
#define EEPROM_NODE_LOCK_COUNTER_ADDRESS (EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS + SIZE_RF_ENCRYPTION_AES_KEY)
/** @brief First free address for sketch static configuration */
#define EEPROM_LOCAL_CONFIG_ADDRESS (EEPROM_NODE_LOCK_COUNTER_ADDRESS + SIZE_NODE_LOCK_COUNTER)

#endif // MyEepromAddresses_h

/** @}*/
