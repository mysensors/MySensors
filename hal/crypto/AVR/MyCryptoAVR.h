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
* SHA256 and SHA256 implementation for AVR:
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

#ifndef MyCryptoGeneric_h
#define MyCryptoGeneric_h

#include "hal/crypto/MyCryptoHAL.h"

#define MY_CRYPTO_SHA256_ASM	//!< Switch to define correct variable scope for ASM SHA256 implementation

#define SHA256_HASH_BITS  256	//!< Defines the size of a SHA-256 hash value in bits
#define SHA256_HASH_BYTES (SHA256_HASH_BITS/8) //!< Defines the size of a SHA-256 hash value in bytes
#define SHA256_BLOCK_BITS 512 //!< Defines the size of a SHA-256 input block in bits
#define SHA256_BLOCK_BYTES (SHA256_BLOCK_BITS/8) //!< Defines the size of a SHA-256 input block in bytes

/**
* @brief SHA-256 context type
*
* A variable of this type may hold the state of a SHA-256 hashing process
*/
typedef struct {
	uint32_t h[8]; 	//!< h
	uint64_t length; 	//!< length
} sha256_ctx_t;

/**
* @brief SHA-256 hash value type
*
* A variable of this type may hold the hash value produced by the
* sha256_ctx2hash(sha256_hash_t *dest, const sha256_ctx_t *state) function.
*/
typedef uint8_t sha256_hash_t[SHA256_HASH_BYTES];

extern "C" {	// ASM implementation, see MyASM.S

/**
* @brief initialise a SHA-256 context
*
* This function sets a ::sha256_ctx_t to the initial values for hashing.
* @param state pointer to the SHA-256 hashing context
*/
void sha256_init(sha256_ctx_t *state);

/**
* @brief update the context with a given block
*
* This function updates the SHA-256 hash context by processing the given block
* of fixed length.
* @param state pointer to the SHA-256 hash context
* @param block pointer to the block of fixed length (512 bit = 64 byte)
*/
void sha256_nextBlock(sha256_ctx_t *state, const void *block);

/**
* @brief finalize the context with the given block
*
* This function finalizes the SHA-256 hash context by processing the given block
* of variable length.
* @param state pointer to the SHA-256 hash context
* @param block pointer to the block of fixed length (512 bit = 64 byte)
* @param length_b the length of the block in bits
*/
void sha256_lastBlock(sha256_ctx_t *state, const void *block, uint16_t length_b);

/**
* @brief convert the hash state into the hash value
*
* This function reads the context and writes the hash value to the destination
* @param dest pointer to the location where the hash value should be written
* @param state pointer to the SHA-256 hash context
*/
void sha256_ctx2hash(sha256_hash_t *dest, const sha256_ctx_t *state);

/**
* @brief simple SHA-256 hashing function for direct hashing
*
* This function automaticaly hashes a given message of arbitary length with
* the SHA-256 hashing algorithm.
* @param dest pointer to the location where the hash value is going to be written to
* @param msg pointer to the message thats going to be hashed
* @param length_b length of the message in bits
*/
void sha256(sha256_hash_t *dest, const void *msg, uint32_t length_b);
}
// MHAC SHA256

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
* @param msg pointer to the message thats going to be hashed
* @param msglength_b length of the message
*/
void hmac_sha256(void *dest, const void *key, uint16_t keylength_b, const void *msg,
                 uint32_t msglength_b);

#endif
