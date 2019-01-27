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

#include "MyCryptoESP32.h"

// ESP32 SHA256
void SHA256(uint8_t *dest, const uint8_t *data, size_t dataLength)
{
	mbedtls_md_context_t ctx;
	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
	mbedtls_md_starts(&ctx);
	mbedtls_md_update(&ctx, (const unsigned char *)data, dataLength);
	mbedtls_md_finish(&ctx, dest);
}


// ESP32 SHA256HMAC
void SHA256HMAC(uint8_t *dest, const uint8_t *key, size_t keyLength, const uint8_t *data,
                size_t dataLength)
{
	mbedtls_md_context_t ctx;
	mbedtls_md_init(&ctx);
	mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 1);
	mbedtls_md_starts(&ctx);
	mbedtls_md_hmac_starts(&ctx, (const unsigned char *)key, keyLength);
	mbedtls_md_hmac_update(&ctx, (const unsigned char *)data, dataLength);
	mbedtls_md_hmac_finish(&ctx, dest);
}

/* not tested yet
// ESP32 AES128 CBC
esp_aes_context aes_ctx;

void AES128CBCInit(const uint8_t *key)
{
	esp_aes_init(&aes_ctx);
	(void)esp_aes_setkey(&aes_ctx, key, 128);
}

void AES128CBCEncrypt(uint8_t *dest, const uint8_t *data, size_t dataLength)
{
	uint8_t iv[16] = { 0 };
	esp_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, dataLength, iv, (const unsigned char *)data, (unsigned char *)dest);
}

void AES128CBCDecrypt(uint8_t *dest, const uint8_t *data, size_t dataLength)
{
	uint8_t iv[16] = { 0 };
	esp_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, dataLength, iv, (const unsigned char *)data, (unsigned char *)dest);
}

void AES128CBCFree(void)
{
	esp_aes_free(&aes_ctx);
}
*/
