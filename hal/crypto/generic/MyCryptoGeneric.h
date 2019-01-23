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

// This code is taken from https://github.com/Cathedrow/Cryptosuite (Peter Knight) and is lightly modified for MySensors use

#ifndef MyCryptoESP32_h
#define MyCryptoESP32_h

#include "hal/crypto/MyCryptoHAL.h"

#define HASH_LENGTH 32	//!< HASH_LENGTH
#define BLOCK_LENGTH 64	//!< BLOCK_LENGTH

/**
* @brief buffer for SHA256 calculator
*/
union _SHA256buffer_t {
	uint8_t b[BLOCK_LENGTH];			//!< SHA256 b
	uint32_t w[BLOCK_LENGTH / 4];	//!< SHA256 w
};

/**
* @brief state variables for SHA256 calculator
*/
union _SHA256state_t {
	uint8_t b[HASH_LENGTH];	//!< SHA256 b
	uint32_t w[HASH_LENGTH / 4]; //!< SHA256 w
};

// SHA256 HMAC padding bytes
#define HMAC_IPAD 0x36	//!< HMAC_IPAD
#define HMAC_OPAD 0x5c	//!< HMAC_OPAD

#endif
