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

#ifndef MyEepromAddresses_h
#define MyEepromAddresses_h
// EEPROM start address for mysensors library data
#define EEPROM_START 0
// EEPROM location of node id
#define EEPROM_NODE_ID_ADDRESS EEPROM_START
// EEPROM location of parent id
#define EEPROM_PARENT_NODE_ID_ADDRESS (EEPROM_START+1)
// EEPROM location of distance to gateway
#define EEPROM_DISTANCE_ADDRESS (EEPROM_PARENT_NODE_ID_ADDRESS+1)
#define EEPROM_ROUTES_ADDRESS (EEPROM_DISTANCE_ADDRESS+1) // Where to start storing routing information in EEPROM. Will allocate 256 bytes.
#define EEPROM_CONTROLLER_CONFIG_ADDRESS (EEPROM_ROUTES_ADDRESS+256) // Location of controller sent configuration (we allow one payload of config data from controller)
#define EEPROM_FIRMWARE_TYPE_ADDRESS (EEPROM_CONTROLLER_CONFIG_ADDRESS+24)
#define EEPROM_FIRMWARE_VERSION_ADDRESS (EEPROM_FIRMWARE_TYPE_ADDRESS+2)
#define EEPROM_FIRMWARE_BLOCKS_ADDRESS (EEPROM_FIRMWARE_VERSION_ADDRESS+2)
#define EEPROM_FIRMWARE_CRC_ADDRESS (EEPROM_FIRMWARE_BLOCKS_ADDRESS+2)
#define EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS (EEPROM_FIRMWARE_CRC_ADDRESS+2)
#define EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS (EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS+32)
#define EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS (EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS+32) // This is set with SecurityPersonalizer.ino
#define EEPROM_SIGNING_SOFT_SERIAL_ADDRESS (EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS+32) // This is set with SecurityPersonalizer.ino
#define EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS (EEPROM_SIGNING_SOFT_SERIAL_ADDRESS+9) // This is set with SecurityPersonalizer.ino
#define EEPROM_NODE_LOCK_COUNTER (EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS+16)
#define EEPROM_LOCAL_CONFIG_ADDRESS (EEPROM_NODE_LOCK_COUNTER+1) // First free address for sketch static configuration

#endif
