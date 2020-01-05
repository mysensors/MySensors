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

/*
* all lengths in bits!
*/
void hmac_sha256(void *dest, const void *key, uint16_t keylength_b, const void *msg,
                 uint32_t msglength_b)
{
	sha256_ctx_t s;
	uint8_t buffer[HMAC_SHA256_BLOCK_BYTES];

	(void)memset((void *)buffer, 0x00, HMAC_SHA256_BLOCK_BYTES);

	/* if key is larger than a block we have to hash it*/
	if (keylength_b > SHA256_BLOCK_BITS) {
		sha256((sha256_hash_t *)buffer, key, keylength_b);
	} else {
		(void)memcpy((void *)buffer, (const void *)key, (keylength_b + 7) / 8);
	}

	for (uint8_t i = 0; i < SHA256_BLOCK_BYTES; ++i) {
		buffer[i] ^= IPAD;
	}
	sha256_init(&s);
	sha256_nextBlock(&s, buffer);
	while (msglength_b >= HMAC_SHA256_BLOCK_BITS) {
		sha256_nextBlock(&s, msg);
		msg = (uint8_t *)msg + HMAC_SHA256_BLOCK_BYTES;
		msglength_b -= HMAC_SHA256_BLOCK_BITS;
	}
	sha256_lastBlock(&s, msg, msglength_b);
	/* since buffer still contains key xor ipad we can do ... */
	for (uint8_t i = 0; i < HMAC_SHA256_BLOCK_BYTES; ++i) {
		buffer[i] ^= IPAD ^ OPAD;
	}
	sha256_ctx2hash((sha256_hash_t *)dest, &s); /* save inner hash temporary to dest */
	sha256_init(&s);
	sha256_nextBlock(&s, buffer);
	sha256_lastBlock(&s, dest, SHA256_HASH_BITS);
	sha256_ctx2hash((sha256_hash_t *)dest, &s);
}
