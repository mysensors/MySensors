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
*/

#include "hmac_sha256.h"

void SHA256HMACInit(const uint8_t *key, size_t keyLength)
{
	(void)memset((void *)&SHA256keyBuffer, 0x00, BLOCK_LENGTH);
	if (keyLength > BLOCK_LENGTH) {
		// Hash long keys
		SHA256Init();
		SHA256Add(key, keyLength);
		SHA256Result(SHA256keyBuffer);
	} else {
		// Block length keys are used as is
		(void)memcpy((void *)SHA256keyBuffer, (const void *)key, keyLength);
	}
	// Start inner hash
	SHA256Init();
	for (uint8_t i = 0; i < BLOCK_LENGTH; i++) {
		SHA256Add(SHA256keyBuffer[i] ^ HMAC_IPAD);
	}
}

void SHA256HMACAdd(const uint8_t data)
{
	SHA256Add(data);
}

void SHA256HMACAdd(const uint8_t *data, size_t dataLength)
{
	SHA256Add(data, dataLength);
}

void SHA256HMACResult(uint8_t *dest)
{
	uint8_t innerHash[HASH_LENGTH];
	// Complete inner hash
	SHA256Result(innerHash);
	// Calculate outer hash
	SHA256Init();
	for (uint8_t i = 0; i < BLOCK_LENGTH; i++) {
		SHA256Add(SHA256keyBuffer[i] ^ HMAC_OPAD);
	}
	SHA256Add(innerHash, HASH_LENGTH);
	SHA256Result(dest);
}