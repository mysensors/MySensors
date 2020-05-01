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

#ifndef MyTransportEncryption_h
#define MyTransportEncryption_h

/**
* @brief transportEncryptionKeyDerivation
* @param salt
* @param salt_len
* @param ikm
* @param ikm_len
* @param info
* @param info_len
* @param okm
* @return
*/
static void transportEncryptionKeyDerivation(const uint8_t *salt, const uint8_t salt_len,
        const uint8_t *ikm,
        const uint8_t ikm_len, const uint8_t *info, uint8_t info_len, uint8_t *okm) __attribute__((unused));

/**
* @brief transportEncryptionInsecureAESEncryption
* @param data
* @param len
* @return final length
*/

static uint8_t transportEncryptionInsecureAESEncryption(void *data,
        uint8_t len) __attribute__((unused));

/**
* @brief transportEncryptionInsecureAESDecryption
* @param data
* @param len
*/
static void transportEncryptionInsecureAESDecryption(void *data,
        const uint8_t len) __attribute__((unused));

/**
* @brief transportEncryptionSecureAESEncryption
* @param outputBuffer
* @param inputBuffer
* @param inputLen
* @param auxBuffer
* @param aufBufferLen
* @param IV_SIZE
* @param HMAC_SIZE
* @return
*/
static uint8_t transportEncryptionSecureAESEncryption(uint8_t *outputBuffer,
        const uint8_t *inputBuffer,
        const uint8_t inputLen, const uint8_t *auxBuffer, const uint8_t auxBufferLen, const uint8_t IV_SIZE,
        const uint8_t HMAC_SIZE) __attribute__((unused));

/**
* @brief transportEncryptionSecureAESDecryption
* @param buffer
* @param bufferLen
* @param auxBuffer
* @param auxBufferLen
* @param IV_SIZE
* @param HMAC_SIZE
* @return
*/
static bool transportEncryptionSecureAESDecryption(uint8_t *buffer, uint8_t bufferLen,
        const uint8_t *auxBuffer,
        const uint8_t auxBufferLen, const uint8_t IV_SIZE, const uint8_t HMAC_SIZE) __attribute__((unused));

/**
* @brief transportEncryptionSignData
* @param outputBuffer
* @param inputBuffer
* @param inputLen
* @param auxBuffer
* @param auxBufferLen
* @param NONCE_SIZE
* @param HMAC_SIZE
* @return
*/
static uint8_t transportEncryptionSignData(uint8_t *outputBuffer,
        const uint8_t *inputBuffer,
        const uint8_t inputLen, const uint8_t *auxBuffer, const uint8_t auxBufferLen,
        const uint8_t NONCE_SIZE,
        const uint8_t HMAC_SIZE) __attribute__((unused));

#endif // MyTransportEncryption_h
