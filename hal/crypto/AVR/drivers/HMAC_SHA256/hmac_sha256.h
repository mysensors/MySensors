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
*
* ======================================================================
*
* HMAC SHA256 implementation for AVR:
*
* This file is part of the AVR-Crypto-Lib.
* Copyright (C) 2006-2015 Daniel Otte (bg@nerilex.org)
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* Author:	Daniel Otte
*
* License: GPLv3 or later
*
* ======================================================================
*/


#ifndef _HMAC_SHA256_
#define _HMAC_SHA256_

#define IPAD 0x36 //!< HMAC, inner hash, xor byte
#define OPAD 0x5C //!< HMAC, outer hash, xor byte

#define HMAC_SHA256_BITS        SHA256_HASH_BITS //!< Defines the size of a SHA-256 HMAC hash value in bits
#define HMAC_SHA256_BYTES       SHA256_HASH_BYTES //!< Defines the size of a SHA-256 HMAC hash value in bytes
#define HMAC_SHA256_BLOCK_BITS  SHA256_BLOCK_BITS //!< Defines the size of a SHA-256 HMAC input block in bits
#define HMAC_SHA256_BLOCK_BYTES SHA256_BLOCK_BYTES //!< Defines the size of a SHA-256 HMAC input block in bytes

/**
* @brief hash context structure
*/
typedef struct {
	sha256_ctx_t a; //!< a
	sha256_ctx_t b; //!< b
} hmac_sha256_ctx_t;

/**
* @brief SHA256 HMAC function
*
* @param dest pointer to the location where the hash value is going to be written to
* @param key pointer to the key that's is needed for the HMAC calculation
* @param keylength_b length of the key
* @param msg pointer to the message that's going to be hashed
* @param msglength_b length of the message
*/
void hmac_sha256(void *dest, const void *key, uint16_t keylength_b, const void *msg,
                 uint32_t msglength_b);

#endif
