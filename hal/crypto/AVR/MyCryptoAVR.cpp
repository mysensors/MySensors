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

#include "MyCryptoAVR.h"

// SHA256 implementation in ASM, see MyASM.S
void SHA256(uint8_t *dest, const uint8_t *data, size_t dataLength)
{
	sha256((sha256_hash_t *)dest, data, dataLength << 3);	// data length in bits
}

// SHA256HMAC
void SHA256HMAC(uint8_t *dest, const uint8_t *key, size_t keyLength, const uint8_t *data,
                size_t dataLength)
{
	hmac_sha256(dest, key, keyLength << 3, data, dataLength << 3);
}


// AES
AES_ctx aes_ctx;

void AES128CBCInit(const uint8_t *key)
{
	AES_init_ctx(&aes_ctx, key);
}

void AES128CBCEncrypt(uint8_t *iv, uint8_t *buffer, const size_t dataLength)
{
	AES_ctx_set_iv(&aes_ctx, iv);
	AES_CBC_encrypt_buffer(&aes_ctx, buffer, dataLength);
}

void AES128CBCDecrypt(uint8_t *iv, uint8_t *buffer, const size_t dataLength)
{
	AES_ctx_set_iv(&aes_ctx, iv);
	AES_CBC_decrypt_buffer(&aes_ctx, buffer, dataLength);
}


