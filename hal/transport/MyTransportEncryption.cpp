/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * MyTransportEncryption implementation created by Olivier Mauti 2020 <olivier@mysensors.org>
 *
 */

#include "MyTransportEncryption.h"

static uint8_t _transportHMAC256_KEY[32];

// debug
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
#define TRANSPORT_ENCRYPTION_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug
#else
#define TRANSPORT_ENCRYPTION_DEBUG(x,...)	//!< debug NULL
#endif

static void transportEncryptionKeyDerivation(const uint8_t *salt, const uint8_t salt_len,
        const uint8_t *ikm,
        const uint8_t ikm_len, const uint8_t *info, uint8_t info_len, uint8_t *okm)
{
	uint8_t _temp_buffer[64];
	(void)memcpy((void *)_temp_buffer, (const void *)salt, salt_len);
	(void)memcpy((void *)&_temp_buffer[salt_len], (const void *)info, info_len);
	SHA256HMAC(okm, ikm, ikm_len, _temp_buffer, salt_len + info_len);
}

static void transportEncryptionInit(uint8_t *PSK, const uint8_t len)
{
#if defined(MY_ENCRYPTION_SIMPLE_PASSWD)
	(void)memset((void *)PSK, 0, len);
	(void)memcpy((void *)PSK, MY_ENCRYPTION_SIMPLE_PASSWD,
	             strnlen(MY_ENCRYPTION_SIMPLE_PASSWD, len));
#else
	hwReadConfigBlock((void *)PSK, (void *)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS,
	                  len);
#endif

#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)PSK, len);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:INIT:PSK=%s\n"), hwDebugPrintStr);
#endif
}


static uint8_t transportEncryptionInsecureAESEncryption(void *data, uint8_t len)
{
	// We use IV vector filled with zeros and randomize unused bytes in encryption block
	uint8_t IV[16] = { 0 };
	const uint8_t finalLength = len > 16 ? 32 : 16;
	// fill block with random data
	for (uint8_t i = len; i < finalLength; i++) {
		*((uint8_t *)data + i) = random(256);
	}
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)data, finalLength);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:IEX:PLAIN=%s\n"),
	                           hwDebugPrintStr);
#endif
	//encrypt data
	AES128CBCEncrypt(IV, (uint8_t *)data, finalLength);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)data, finalLength);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:IEX:CIP=%s\n"),
	                           hwDebugPrintStr); // IEX = insecure AES encryption
#endif
	return finalLength;
}

static void transportEncryptionInsecureAESDecryption(void *data, const uint8_t len)
{
	// has to be adjusted, WIP!
	uint8_t IV[16] = { 0 };
	// decrypt data
	AES128CBCDecrypt(IV, (uint8_t *)data, len);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)data, len);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:IDX:PLAIN=%s\n"), hwDebugPrintStr);
#endif
}

static uint8_t transportEncryptionSecureAESEncryption(uint8_t *outputBuffer,
        const uint8_t *inputBuffer,
        const uint8_t inputLen, const uint8_t *auxBuffer, const uint8_t auxBufferLen, const uint8_t IV_SIZE,
        const uint8_t HMAC_SIZE)
{
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:ENC:AEX\n"));
	uint8_t _IV[16];
	uint8_t _processedPayload[32];
	uint8_t _HMAC[32];
#if defined(MY_HW_HAS_GETENTROPY)
	while (hwGetentropy(&_IV, sizeof(_IV)) != sizeof(_IV));
#else
	// We use a basic whitening technique that XORs a random byte with the current hwMillis() counter
	// and then the byte is hashed (SHA256) to produce the resulting nonce
	for (uint8_t i = 0; i < sizeof(_HMAC); i++) {
		_HMAC[i] = random(256) ^ (hwMillis() & 0xFF);
	}
	SHA256(outputBuffer, _HMAC, sizeof(_HMAC));
	(void)memcpy((void *)_IV, (const void *)outputBuffer, IV_SIZE);
#endif
	// padding IV
	(void)memset((void *)&_IV[IV_SIZE], 0xAA, sizeof(_IV) - IV_SIZE);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)&_IV, sizeof(_IV));
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:ENC:IV=%s\n"), hwDebugPrintStr);
#endif
	const uint8_t finalLength = inputLen > 16 ? 32 : 16;
	(void)memcpy((void *)_processedPayload, (const void *)inputBuffer, inputLen);
	// add padding bytes, PKCS#7
	for (uint8_t i = inputLen; i < finalLength; i++) {
		_processedPayload[i] = finalLength - inputLen;
	}
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)&_processedPayload, finalLength);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:ENC:PAD=%s\n"), hwDebugPrintStr);
#endif
	(void)memcpy((void *)outputBuffer, (const void *)_IV, IV_SIZE);
	AES128CBCEncrypt(_IV, _processedPayload, finalLength);
	(void)memset((void *)_IV, 0, sizeof(_IV));
	// create IV + ciphertext + aux for HMAC256
	(void)memcpy((void *)&outputBuffer[IV_SIZE], (const void *)_processedPayload, finalLength);
	// add auxillary buffer (i.e. header) for HMAC
	(void)memcpy((void *)&outputBuffer[IV_SIZE + finalLength], (const void *)auxBuffer, auxBufferLen);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str(auxBuffer, auxBufferLen);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:ENC:AUX=%s\n"), hwDebugPrintStr);
#endif
	// calculate HMAC256
	SHA256HMAC(_HMAC, _transportHMAC256_KEY, sizeof(_transportHMAC256_KEY), outputBuffer,
	           IV_SIZE + finalLength + auxBufferLen);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	// padding HMAC, not needed
	(void)memset((void *)&_HMAC[HMAC_SIZE], 0xAA, sizeof(_HMAC) - HMAC_SIZE);
	hwDebugBuf2Str((const uint8_t *)&_HMAC, sizeof(_HMAC));
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:ENC:HMAC=%s\n"), hwDebugPrintStr);
#endif
	// create IV + HMAC
	(void)memcpy((void *)&outputBuffer[IV_SIZE], (const void *)_HMAC, HMAC_SIZE);
	// create IV + HMAC + ciphertext
	(void)memcpy((void *)&outputBuffer[IV_SIZE + HMAC_SIZE], (const void *)_processedPayload,
	             finalLength);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)&_processedPayload, finalLength);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:ENC:CIP=%s\n"), hwDebugPrintStr);
#endif
	return IV_SIZE + HMAC_SIZE + finalLength;
}

static bool transportEncryptionSecureAESDecryption(uint8_t *buffer, uint8_t bufferLen,
        const uint8_t *auxBuffer,
        const uint8_t auxBufferLen, const uint8_t IV_SIZE, const uint8_t HMAC_SIZE)
{
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:DEC:EAX\n"));
	bool result = false;
	uint8_t _IV[16];
	uint8_t _ciphertext[32];
	uint8_t _HMACin[32];
	const uint8_t len = bufferLen - (IV_SIZE + HMAC_SIZE);
	(void)memcpy((void *)_IV, (const void *)buffer, IV_SIZE);
	// padding IV
	(void)memset(&_IV[IV_SIZE], 0xAA, sizeof(_IV) - IV_SIZE);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)&_IV, sizeof(_IV));
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:DEC:IV=%s\n"), hwDebugPrintStr);
#endif
	(void)memcpy((void *)_HMACin, (const void *)&buffer[IV_SIZE],
	             HMAC_SIZE);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	// padding HMAC
	(void)memset((void *)&_HMACin[HMAC_SIZE], 0xAA, sizeof(_HMACin) - HMAC_SIZE);
	hwDebugBuf2Str((const uint8_t *)&_HMACin, sizeof(_HMACin));
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:DEC:HMACin=%s\n"), hwDebugPrintStr);
#endif
	(void)memcpy((void *)_ciphertext,
	             (const void *)&buffer[IV_SIZE + HMAC_SIZE],
	             len);
	// prepare HMAC verification: concatenate IV + ciphertext + auxBuffer
	(void)memcpy((void *)&buffer[IV_SIZE], (const void *)_ciphertext, len);
	// add auxillary buffer (i.e. header) for HMAC
	(void)memcpy((void *)&buffer[IV_SIZE + len], (const void *)auxBuffer, auxBufferLen);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str(auxBuffer, auxBufferLen);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:DEC:AUX=%s\n"), hwDebugPrintStr);
#endif
	uint8_t _HMACout[32];
	SHA256HMAC(_HMACout, _transportHMAC256_KEY, sizeof(_transportHMAC256_KEY),
	           buffer,
	           IV_SIZE + len + auxBufferLen);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	// padding HMACout
	(void)memset((void *)&_HMACout[HMAC_SIZE], 0xAA, sizeof(_HMACin) - HMAC_SIZE);
	hwDebugBuf2Str((const uint8_t *)&_HMACout, sizeof(_HMACout));
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:DEC:HMACout=%s\n"), hwDebugPrintStr);
#endif
	if (timingneutralMemcmp(_HMACin, _HMACout, (size_t)HMAC_SIZE)) {
		// HMAC failed
		TRANSPORT_ENCRYPTION_DEBUG(PSTR("!TEX:DEC:HMAC\n"));
	} else {
		result = true;
	}
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)&_ciphertext, len);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:DEC:CIP=%s\n"), hwDebugPrintStr);
#endif
	AES128CBCDecrypt(_IV, (uint8_t *)_ciphertext, len);
	// copy plaintext back
	(void)memcpy((void *)buffer, _ciphertext, len);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)&_ciphertext, len);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:DEC:PLAIN=%s\n"), hwDebugPrintStr);
#endif
	return result;
}



static uint8_t transportEncryptionSignData(uint8_t *outputBuffer,
        const uint8_t *inputBuffer,
        const uint8_t inputLen, const uint8_t *auxBuffer, const uint8_t auxBufferLen,
        const uint8_t NONCE_SIZE,
        const uint8_t HMAC_SIZE)
{
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:SDA:SIGN\n"));
	uint8_t _NONCE[32];
	uint8_t _HMAC[32];
#if defined(MY_HW_HAS_GETENTROPY)
	while (hwGetentropy(&_NONCE, sizeof(_NONCE)) != sizeof(_NONCE));
#else
	// We use a basic whitening technique that XORs a random byte with the current hwMillis() counter
	// and then the byte is hashed (SHA256) to produce the resulting nonce
	for (uint8_t i = 0; i < sizeof(_HMAC); i++) {
		_HMAC[i] = random(256) ^ (hwMillis() & 0xFF);
	}
	SHA256(outputBuffer, _HMAC, sizeof(_HMAC));
	(void)memcpy((void *)_NONCE, (const void *)outputBuffer, NONCE_SIZE);
#endif
	// padding IV
	(void)memset((void *)&_NONCE[NONCE_SIZE], 0xAA, sizeof(_NONCE) - NONCE_SIZE);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str((const uint8_t *)&_NONCE, sizeof(_NONCE));
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:SDA:NONCE=%s\n"), hwDebugPrintStr);
#endif
	(void)memcpy((void *)outputBuffer, (const void *)_NONCE, NONCE_SIZE);
	(void)memset((void *)_NONCE, 0, sizeof(_NONCE));
	// create NONCE + cleartext + auxBuffer for HMAC256
	(void)memcpy((void *)&outputBuffer[NONCE_SIZE], (const void *)inputBuffer, inputLen);
	// add auxillary buffer (i.e. header) for HMAC
	(void)memcpy((void *)&outputBuffer[NONCE_SIZE + inputLen], (const void *)auxBuffer, auxBufferLen);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	hwDebugBuf2Str(auxBuffer, auxBufferLen);
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:SDA:AUX=%s\n"), hwDebugPrintStr);
#endif
	// calculate HMAC256
	SHA256HMAC(_HMAC, _transportHMAC256_KEY, sizeof(_transportHMAC256_KEY), outputBuffer,
	           NONCE_SIZE + inputLen + auxBufferLen);
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_ENCRYPTION)
	// padding HMAC, not needed
	(void)memset((void *)&_HMAC[HMAC_SIZE], 0xAA, sizeof(_HMAC) - HMAC_SIZE);
	hwDebugBuf2Str((const uint8_t *)&_HMAC, sizeof(_HMAC));
	TRANSPORT_ENCRYPTION_DEBUG(PSTR("TEX:SDA:HMAC=%s\n"), hwDebugPrintStr);
#endif
	// create IV + HMAC
	(void)memcpy((void *)&outputBuffer[NONCE_SIZE], (const void *)_HMAC, HMAC_SIZE);
	// create IV + HMAC + cleartext
	(void)memcpy((void *)&outputBuffer[NONCE_SIZE + HMAC_SIZE], (const void *)inputBuffer,
	             inputLen);
	return NONCE_SIZE + HMAC_SIZE + inputLen;
}
