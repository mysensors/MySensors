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

#ifndef MyCryptoHAL_h
#define MyCryptoHAL_h

/**
* @brief SHA256 calculation
*
* The returned hash size is always 32 bytes.
*
* @param dest Buffer to return 32-byte hash.
* @param data Buffer with data to add.
* @param dataLength Size of data buffer.
*/
void SHA256(uint8_t *dest, const uint8_t *data, size_t dataLength);

/**
* @brief SHA256 HMAC calculation
*
* The returned hash size is always 32 bytes.
*
* @param dest Buffer to return 32-byte hash.
* @param key Buffer with HMAC key.
* @param keyLength Size of HMAC key.
* @param data Buffer with data to add.
* @param dataLength Size of data buffer.
*/
void SHA256HMAC(uint8_t *dest, const uint8_t *key, size_t keyLength, const uint8_t *data,
                size_t dataLength);

/**
* @brief AES128CBCInit
* @param key AES encryption key, 16 bytes
*/
void AES128CBCInit(const uint8_t *key);
/**
* @brief AES128CBCEncrypt
* @param iv Initialization vector, 16 bytes
* @param buffer Buffer to enctypt
* @param dataLength Buffer length
*/
void AES128CBCEncrypt(uint8_t *iv, uint8_t *buffer, const size_t dataLength);
/**
* @brief AES128CBCDecrypt
* @param iv Initialization vector, 16 bytes
* @param buffer Buffer to decrypt
* @param dataLength Buffer length
*/
void AES128CBCDecrypt(uint8_t *iv, uint8_t *buffer, const size_t dataLength);

#endif
