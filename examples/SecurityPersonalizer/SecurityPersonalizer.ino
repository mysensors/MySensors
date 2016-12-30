/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 */
/**
 * @ingroup MySigninggrp
 * @{
 * @file SecurityPersonalizer.ino
 * @brief Security personalization sketch
 *
 * REVISION HISTORY
 *  - See git log (git log libraries/MySensors/examples/SecurityPersonalizer/SecurityPersonalizer.ino)
 */

/**
 * @example SecurityPersonalizer.ino
 * This sketch will personalize either none-volatile memory or ATSHA204A for security functions
 * available in the MySensors library.
 *
 * For ATSHA204A:
 * It will write factory default settings to the configuration zone
 * and then lock it.<br>
 * It will then either<br>
 * -# Generate a random value to use as a key which will be stored in
 * slot 0. The key is printed on UART (115200) in clear text for the user to be
 * able to use it as a user-supplied key in other personalization executions
 * where the same key is needed.
 * -# Use a user-supplied value to use as a key which will be stored in
 * slot 0.
 * Finally it will lock the data zone.
 *
 * By default, no locking is performed. User have to manually enable the flags that
 * turn on the locking. Furthermore, user have to send a SPACE character on serial
 * console when prompted to do any locking. On boards that does not provide UART
 * input it is possible to configure the sketch to skip this confirmation.
 * Default settings use ATSHA204A on @ref MY_SIGNING_ATSHA204_PIN.
 *
 * For Soft signing:
 * It will<br>
 * -# Generate a random value to use as a key which will be stored in EEPROM.
 * The key is printed on UART (115200) in clear text for the user to be ablle to
 * use it as a user-supplied key in other personalization executions where the same
 * key is needed.
 * -# Use a user-supplied value to use as a key which will be stored in EEPROM.
 * -# Generate a random value to use as a serial number which will be stored in EEPROM.
 * The serial number is printed on UART (115200) in clear text for the user to be ablle to
 * use it as a user-supplied serial number in other personalization executions where the
 * serial is needed (typically for a whitelist).
 * -# Use a user-supplied value to use as a serial which will be stored in EEPROM.
 *
 * For Encryption support:
 * -# Generate a random value to use as a AES key which will be stored in EEPROM.
 * The AES key is printed on UART (115200) in clear text for the user to be ablle to
 * use it as a user-supplied AES key in other personalization executions where the
 * AES key is needed (typically for RF encryption).
 * -# Use a user-supplied value to use as a AES key which will be stored in EEPROM.
 *
 * Personalizing EEPROM or ATSHA204A still require the appropriate configuration of the
 * library to actually have an effect. There is no problem personalizing EEPROM and
 * ATSHA204A at the same time. There is however a security risk with using the same
 * data for EEPROM and ATSHA204A so it is recommended to use different serial and HMAC
 * keys on the same device for ATSHA204A vs soft signing settings.
 *
 * Details on personalization procedure is given in @ref personalization.
 */

#include "sha204_library.h"
#include "sha204_lib_return_codes.h"
#define MY_CORE_ONLY
#include <MySensors.h>

// Doxygen specific constructs, not included when built normally
// This is used to enable disabled macros/definitions to be included in the documentation as well.
#if DOXYGEN
#define LOCK_CONFIGURATION
#define LOCK_DATA
#define SKIP_KEY_STORAGE
#define USER_KEY
#define SKIP_UART_CONFIRMATION
#define USE_SOFT_SIGNING
#define STORE_SOFT_KEY
#define USER_SOFT_KEY
#define STORE_SOFT_SERIAL
#define USER_SOFT_SERIAL
#define STORE_AES_KEY
#define USER_AES_KEY
#endif

/**
 * @def LOCK_CONFIGURATION
 * @brief Uncomment this to enable locking the configuration zone.
 *
 * It is still possible to change the key, and this also enable random key generation.
 * @warning BE AWARE THAT THIS PREVENTS ANY FUTURE CONFIGURATION CHANGE TO THE CHIP
 */
//#define LOCK_CONFIGURATION

/**
 * @def LOCK_DATA
 * @brief Uncomment this to enable locking the data zone.
 *
 * It is not required to lock data, key cannot be retrieved anyway, but by locking
 * data, it can be guaranteed that nobody even with physical access to the chip,
 * will be able to change the key.
 * @warning BE AWARE THAT THIS PREVENTS THE KEY TO BE CHANGED
 */
//#define LOCK_DATA

/**
 * @def SKIP_KEY_STORAGE
 * @brief Uncomment this to skip key storage (typically once key has been written once)
 */
#define SKIP_KEY_STORAGE

/**
 * @def USER_KEY
 * @brief Uncomment this to skip key generation and use @ref user_key_data as key instead.
 */
//#define USER_KEY

/**
 * @def SKIP_UART_CONFIRMATION
 * @brief Uncomment this for boards that lack UART
 *
 * @b Important<br> No confirmation will be required for locking any zones with this configuration!
 * Also, key generation is not permitted in this mode as there is no way of presenting the generated key.
 */
//#define SKIP_UART_CONFIRMATION

/**
 * @def USE_SOFT_SIGNING
 * @brief Uncomment this to store data to EEPROM instead of ATSHA204A
 */
//#define USE_SOFT_SIGNING

/**
 * @def STORE_SOFT_KEY
 * @brief Uncomment this to store soft HMAC key to EEPROM
 */
//#define STORE_SOFT_KEY

/**
 * @def USER_SOFT_KEY
 * @brief Uncomment this to skip soft HMAC key generation and use @ref user_soft_key_data as HMAC key instead.
 */
//#define USER_SOFT_KEY

/**
 * @def STORE_SOFT_SERIAL
 * @brief Uncomment this to store soft serial to EEPROM
 */
//#define STORE_SOFT_SERIAL

/**
 * @def USER_SOFT_SERIAL
 * @brief Uncomment this to skip soft serial generation and use @ref user_soft_serial as serial instead.
 */
//#define USER_SOFT_SERIAL

/**
 * @def STORE_AES_KEY
 * @brief Uncomment this to store AES key to EEPROM
 */
//#define STORE_AES_KEY

/**
 * @def USER_AES_KEY
 * @brief Uncomment this to skip AES key generation and use @ref user_aes_key as key instead.
 */
//#define USER_AES_KEY

#if defined(SKIP_UART_CONFIRMATION) && !defined(USER_KEY)
#error You have to define USER_KEY for boards that does not have UART
#endif

#ifdef USER_KEY
/** @brief The user-defined HMAC key to use for personalization */
#define MY_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
/** @brief The data to store in key slot 0 */
const uint8_t user_key_data[32] = {MY_HMAC_KEY};
#endif

#ifdef USER_SOFT_KEY
/** @brief The user-defined soft HMAC key to use for EEPROM personalization */
#define MY_SOFT_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
/** @brief The data to store as soft HMAC key in EEPROM */
const uint8_t user_soft_key_data[32] = {MY_SOFT_HMAC_KEY};
#endif

#ifdef USER_SOFT_SERIAL
/** @brief The user-defined soft serial to use for EEPROM personalization */
#define MY_SOFT_SERIAL 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
/** @brief The data to store as soft serial in EEPROM */
const uint8_t user_soft_serial[9] = {MY_SOFT_SERIAL};
#endif

#ifdef USER_AES_KEY
/** @brief The user-defined AES key to use for EEPROM personalization */
#define MY_AES_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
/** @brief The data to store as AES key in EEPROM */
const uint8_t user_aes_key[16] = {MY_AES_KEY};
#endif

#ifndef USE_SOFT_SIGNING
const int sha204Pin = MY_SIGNING_ATSHA204_PIN; //!< The IO pin to use for ATSHA204A
atsha204Class sha204(sha204Pin);
#endif

/** @brief Print a error notice and halt the execution */
void halt()
{
	Serial.println(F("Halting!"));
	while(1);
}

#ifndef USE_SOFT_SIGNING
/**
 * @brief Write default configuration and return CRC of the configuration bits
 * @returns CRC over the configuration bits
 */
uint16_t write_config_and_get_crc()
{
	uint16_t crc = 0;
	uint8_t config_word[4];
	uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
	uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
	uint8_t ret_code;
	bool do_write;

	// We will set default settings from datasheet on all slots. This means that we can use slot 0 for the key
	// as that slot will not be readable (key will therefore be secure) and slot 8 for the payload digest
	// calculationon as that slot can be written in clear text even when the datazone is locked.
	// Other settings which are not relevant are kept as is.

	for (int i=0; i < 88; i += 4) {
		do_write = true;
		if (i == 20) {
			config_word[0] = 0x8F;
			config_word[1] = 0x80;
			config_word[2] = 0x80;
			config_word[3] = 0xA1;
		} else if (i == 24) {
			config_word[0] = 0x82;
			config_word[1] = 0xE0;
			config_word[2] = 0xA3;
			config_word[3] = 0x60;
		} else if (i == 28) {
			config_word[0] = 0x94;
			config_word[1] = 0x40;
			config_word[2] = 0xA0;
			config_word[3] = 0x85;
		} else if (i == 32) {
			config_word[0] = 0x86;
			config_word[1] = 0x40;
			config_word[2] = 0x87;
			config_word[3] = 0x07;
		} else if (i == 36) {
			config_word[0] = 0x0F;
			config_word[1] = 0x00;
			config_word[2] = 0x89;
			config_word[3] = 0xF2;
		} else if (i == 40) {
			config_word[0] = 0x8A;
			config_word[1] = 0x7A;
			config_word[2] = 0x0B;
			config_word[3] = 0x8B;
		} else if (i == 44) {
			config_word[0] = 0x0C;
			config_word[1] = 0x4C;
			config_word[2] = 0xDD;
			config_word[3] = 0x4D;
		} else if (i == 48) {
			config_word[0] = 0xC2;
			config_word[1] = 0x42;
			config_word[2] = 0xAF;
			config_word[3] = 0x8F;
		} else if (i == 52 || i == 56 || i == 60 || i == 64) {
			config_word[0] = 0xFF;
			config_word[1] = 0x00;
			config_word[2] = 0xFF;
			config_word[3] = 0x00;
		} else if (i == 68 || i == 72 || i == 76 || i == 80) {
			config_word[0] = 0xFF;
			config_word[1] = 0xFF;
			config_word[2] = 0xFF;
			config_word[3] = 0xFF;
		} else {
			// All other configs are untouched
			ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, i);
			if (ret_code != SHA204_SUCCESS) {
				Serial.print(F("Failed to read config. Response: "));
				Serial.println(ret_code, HEX);
				halt();
			}
			// Set config_word to the read data
			config_word[0] = rx_buffer[SHA204_BUFFER_POS_DATA+0];
			config_word[1] = rx_buffer[SHA204_BUFFER_POS_DATA+1];
			config_word[2] = rx_buffer[SHA204_BUFFER_POS_DATA+2];
			config_word[3] = rx_buffer[SHA204_BUFFER_POS_DATA+3];
			do_write = false;
		}

		// Update crc with CRC for the current word
		crc = sha204.calculateAndUpdateCrc(4, config_word, crc);

		// Write config word
		if (do_write) {
			ret_code = sha204.sha204m_execute(SHA204_WRITE, SHA204_ZONE_CONFIG,
			                                  i >> 2, 4, config_word, 0, NULL, 0, NULL,
			                                  WRITE_COUNT_SHORT, tx_buffer, WRITE_RSP_SIZE, rx_buffer);
			if (ret_code != SHA204_SUCCESS) {
				Serial.print(F("Failed to write config word at address "));
				Serial.print(i);
				Serial.print(F(". Response: "));
				Serial.println(ret_code, HEX);
				halt();
			}
		}
	}
	return crc;
}

/**
 * @brief Write provided key to slot 0
 * @param key The key data to write
 */
void write_key(uint8_t* key)
{
	uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
	uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
	uint8_t ret_code;

	// Write key to slot 0
	ret_code = sha204.sha204m_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG,
	                                  0, SHA204_ZONE_ACCESS_32, key, 0, NULL, 0, NULL,
	                                  WRITE_COUNT_LONG, tx_buffer, WRITE_RSP_SIZE, rx_buffer);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Failed to write key to slot 0. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	}
}
#endif // not USE_SOFT_SIGNING

/** @brief Dump current configuration to UART */
void dump_configuration()
{
	uint8_t buffer[32];
#ifndef USE_SOFT_SIGNING
	Serial.println(F("EEPROM DATA:"));
#endif
	hwReadConfigBlock((void*)buffer, (void*)EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
	Serial.print(F("SOFT_HMAC_KEY | "));
	for (int j=0; j<32; j++) {
		if (buffer[j] < 0x10) {
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(buffer[j], HEX);
	}
	Serial.println();
	hwReadConfigBlock((void*)buffer, (void*)EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
	Serial.print(F("SOFT_SERIAL   | "));
	for (int j=0; j<9; j++) {
		if (buffer[j] < 0x10) {
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(buffer[j], HEX);
	}
	Serial.println();
	hwReadConfigBlock((void*)buffer, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
	Serial.print(F("AES_KEY       | "));
	for (int j=0; j<16; j++) {
		if (buffer[j] < 0x10) {
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(buffer[j], HEX);
	}
	Serial.println();
#ifndef USE_SOFT_SIGNING
	uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
	uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
	uint8_t ret_code;
	Serial.println(F("ATSHA204A DATA:"));
	for (int i=0; i < 88; i += 4) {
		ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, i);
		if (ret_code != SHA204_SUCCESS) {
			Serial.print(F("Failed to read config. Response: "));
			Serial.println(ret_code, HEX);
			halt();
		}
		if (i == 0x00) {
			Serial.print(F("           SN[0:1]           |         SN[2:3]           | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x04) {
			Serial.print(F("                          Revnum                         | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				Serial.print(F("   "));
			}
			Serial.println();
		} else if (i == 0x08) {
			Serial.print(F("                          SN[4:7]                        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				Serial.print(F("   "));
			}
			Serial.println();
		} else if (i == 0x0C) {
			Serial.print(F("    SN[8]    |  Reserved13   | I2CEnable | Reserved15    | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j < 3) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x10) {
			Serial.print(F("  I2CAddress |  TempOffset   |  OTPmode  | SelectorMode  | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j < 3) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x14) {
			Serial.print(F("         SlotConfig00        |       SlotConfig01        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x18) {
			Serial.print(F("         SlotConfig02        |       SlotConfig03        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x1C) {
			Serial.print(F("         SlotConfig04        |       SlotConfig05        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x20) {
			Serial.print(F("         SlotConfig06        |       SlotConfig07        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x24) {
			Serial.print(F("         SlotConfig08        |       SlotConfig09        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x28) {
			Serial.print(F("         SlotConfig0A        |       SlotConfig0B        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x2C) {
			Serial.print(F("         SlotConfig0C        |       SlotConfig0D        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x30) {
			Serial.print(F("         SlotConfig0E        |       SlotConfig0F        | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j == 1) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x34) {
			Serial.print(F("  UseFlag00  | UpdateCount00 | UseFlag01 | UpdateCount01 | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j < 3) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x38) {
			Serial.print(F("  UseFlag02  | UpdateCount02 | UseFlag03 | UpdateCount03 | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j < 3) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x3C) {
			Serial.print(F("  UseFlag04  | UpdateCount04 | UseFlag05 | UpdateCount05 | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j < 3) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x40) {
			Serial.print(F("  UseFlag06  | UpdateCount06 | UseFlag07 | UpdateCount07 | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j < 3) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		} else if (i == 0x44) {
			Serial.print(F("                      LastKeyUse[0:3]                    | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				Serial.print(F("   "));
			}
			Serial.println();
		} else if (i == 0x48) {
			Serial.print(F("                      LastKeyUse[4:7]                    | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				Serial.print(F("   "));
			}
			Serial.println();
		} else if (i == 0x4C) {
			Serial.print(F("                      LastKeyUse[8:B]                    | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				Serial.print(F("   "));
			}
			Serial.println();
		} else if (i == 0x50) {
			Serial.print(F("                      LastKeyUse[C:F]                    | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				Serial.print(F("   "));
			}
			Serial.println();
		} else if (i == 0x54) {
			Serial.print(F("  UserExtra  |    Selector   | LockValue |  LockConfig   | "));
			for (int j=0; j<4; j++) {
				if (rx_buffer[SHA204_BUFFER_POS_DATA+j] < 0x10) {
					Serial.print('0'); // Because Serial.print does not 0-pad HEX
				}
				Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+j], HEX);
				if (j < 3) {
					Serial.print(F(" | "));
				} else {
					Serial.print(F("   "));
				}
			}
			Serial.println();
		}
	}
#endif // not USE_SOFT_SIGNING
}

/** @brief Sketch setup code */
void setup()
{
	// Delay startup a bit for serial consoles to catch up
	unsigned long enter = hwMillis();
	while (hwMillis() - enter < (unsigned long)500);
#ifndef USE_SOFT_SIGNING
	uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
	uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
	uint8_t ret_code;
	uint8_t lockConfig = 0;
	uint8_t lockValue = 0;
	uint16_t crc;
	(void)crc;
#else
	// initialize pseudo-RNG
	randomSeed(analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN));
#endif
	uint8_t key[32];
	(void)key;

	Serial.begin(115200);
	hwInit();
	Serial.println(F("Personalization sketch for MySensors usage."));
	Serial.println(F("-------------------------------------------"));

#ifndef USE_SOFT_SIGNING
	// Wake device before starting operations
	ret_code = sha204.sha204c_wakeup(rx_buffer);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Failed to wake device. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	}
	// Read out lock config bits to determine if locking is possible
	ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Failed to determine device lock status. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	} else {
		lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
		lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
	}
#endif

#ifdef STORE_SOFT_KEY
#ifdef USER_SOFT_KEY
	memcpy(key, user_soft_key_data, 32);
	Serial.println(F("Using this user supplied soft HMAC key:"));
#else
	// Retrieve random value to use as soft HMAC key
#ifdef USE_SOFT_SIGNING
	for (int i = 0; i < 32; i++) {
		key[i] = random(256) ^ micros();
		unsigned long enter = hwMillis();
		while (hwMillis() - enter < (unsigned long)2);
	}
	Serial.println(F("This value will be stored in EEPROM as soft HMAC key:"));
#else
	ret_code = sha204.sha204m_random(tx_buffer, rx_buffer, RANDOM_SEED_UPDATE);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Random key generation failed. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	} else {
		memcpy(key, rx_buffer+SHA204_BUFFER_POS_DATA, 32);
	}
	if (lockConfig == 0x00) {
		Serial.println(F("This value will be stored in EEPROM as soft HMAC key:"));
	} else {
		Serial.println(F("Key is not randomized (configuration not locked):"));
	}
#endif // not USE_SOFT_SIGNING
#endif // not USER_SOFT_KEY
	Serial.print("#define MY_SOFT_HMAC_KEY ");
	for (int i=0; i<32; i++) {
		Serial.print("0x");
		if (key[i] < 0x10) {
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(key[i], HEX);
		if (i < 31) {
			Serial.print(',');
		}
	}
	Serial.println();
	hwWriteConfigBlock((void*)key, (void*)EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
#endif // STORE_SOFT_KEY

#ifdef STORE_SOFT_SERIAL
#ifdef USER_SOFT_SERIAL
	memcpy(key, user_soft_serial, 9);
	Serial.println(F("Using this user supplied soft serial:"));
#else
	// Retrieve random value to use as serial
#ifdef USE_SOFT_SIGNING
	for (int i = 0; i < 9; i++) {
		key[i] = random(256) ^ micros();
		unsigned long enter = hwMillis();
		while (hwMillis() - enter < (unsigned long)2);
	}
	Serial.println(F("This value will be stored in EEPROM as soft serial:"));
#else
	ret_code = sha204.sha204m_random(tx_buffer, rx_buffer, RANDOM_SEED_UPDATE);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Random serial generation failed. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	} else {
		memcpy(key, rx_buffer+SHA204_BUFFER_POS_DATA, 9);
	}
	if (lockConfig == 0x00) {
		Serial.println(F("This value will be stored in EEPROM as soft serial:"));
	} else {
		Serial.println(F("Serial is not randomized (configuration not locked):"));
	}
#endif // not USE_SOFT_SIGNING
#endif // not USER_SOFT_SERIAL
	Serial.print("#define MY_SOFT_SERIAL ");
	for (int i=0; i<9; i++) {
		Serial.print("0x");
		if (key[i] < 0x10) {
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(key[i], HEX);
		if (i < 8) {
			Serial.print(',');
		}
	}
	Serial.println();
	hwWriteConfigBlock((void*)key, (void*)EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
#endif // STORE_SOFT_SERIAL

#ifdef STORE_AES_KEY
#ifdef USER_AES_KEY
	memcpy(key, user_aes_key, 16);
	Serial.println(F("Using this user supplied AES key:"));
#else
	// Retrieve random value to use as key
#ifdef USE_SOFT_SIGNING
	for (int i = 0; i < 16; i++) {
		key[i] = random(256) ^ micros();
		unsigned long enter = hwMillis();
		while (hwMillis() - enter < (unsigned long)2);
	}
	Serial.println(F("This key will be stored in EEPROM as AES key:"));
#else
	ret_code = sha204.sha204m_random(tx_buffer, rx_buffer, RANDOM_SEED_UPDATE);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Random key generation failed. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	} else {
		memcpy(key, rx_buffer+SHA204_BUFFER_POS_DATA, 32);
	}
	if (lockConfig == 0x00) {
		Serial.println(F("This key will be stored in EEPROM as AES key:"));
	} else {
		Serial.println(F("Key is not randomized (configuration not locked):"));
	}
#endif // not USE_SOFT_SIGNING
#endif // not USER_AES_KEY
	Serial.print("#define MY_AES_KEY ");
	for (int i=0; i<16; i++) {
		Serial.print("0x");
		if (key[i] < 0x10) {
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(key[i], HEX);
		if (i < 15) {
			Serial.print(',');
		}
	}
	Serial.println();
	hwWriteConfigBlock((void*)key, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
#endif // STORE_AES_KEY

#ifdef USE_SOFT_SIGNING
	Serial.println(F("EEPROM configuration:"));
	dump_configuration();
#else
	// Output device revision on console
	ret_code = sha204.sha204m_dev_rev(tx_buffer, rx_buffer);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Failed to determine device revision. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	} else {
		Serial.print(F("Device revision: "));
		for (int i=0; i<4; i++) {
			if (rx_buffer[SHA204_BUFFER_POS_DATA+i] < 0x10) {
				Serial.print('0'); // Because Serial.print does not 0-pad HEX
			}
			Serial.print(rx_buffer[SHA204_BUFFER_POS_DATA+i], HEX);
		}
		Serial.println();
	}

	// Output serial number on console
	ret_code = sha204.getSerialNumber(rx_buffer);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Failed to obtain device serial number. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	} else {
		Serial.print(F("Device serial:   "));
		Serial.print('{');
		for (int i=0; i<9; i++) {
			Serial.print(F("0x"));
			if (rx_buffer[i] < 0x10) {
				Serial.print('0'); // Because Serial.print does not 0-pad HEX
			}
			Serial.print(rx_buffer[i], HEX);
			if (i < 8) {
				Serial.print(',');
			}
		}
		Serial.print('}');
		Serial.println();
		for (int i=0; i<9; i++) {
			if (rx_buffer[i] < 0x10) {
				Serial.print('0'); // Because Serial.print does not 0-pad HEX
			}
			Serial.print(rx_buffer[i], HEX);
		}
		Serial.println();
	}

	if (lockConfig != 0x00) {
		// Write config and get CRC for the updated config
		crc = write_config_and_get_crc();

		// List current configuration before attempting to lock
		Serial.println(F("Chip configuration:"));
		dump_configuration();

#ifdef LOCK_CONFIGURATION
		// Purge serial input buffer
#ifndef SKIP_UART_CONFIRMATION
		while (Serial.available()) {
			Serial.read();
		}
		Serial.println(F("Send SPACE character now to lock the configuration..."));

		while (Serial.available() == 0);
		if (Serial.read() == ' ')
#endif //not SKIP_UART_CONFIRMATION
		{
			Serial.println(F("Locking configuration..."));

			// Correct sequence, resync chip
			ret_code = sha204.sha204c_resync(SHA204_RSP_SIZE_MAX, rx_buffer);
			if (ret_code != SHA204_SUCCESS && ret_code != SHA204_RESYNC_WITH_WAKEUP) {
				Serial.print(F("Resync failed. Response: "));
				Serial.println(ret_code, HEX);
				halt();
			}

			// Lock configuration zone
			ret_code = sha204.sha204m_execute(SHA204_LOCK, SHA204_ZONE_CONFIG,
			                                  crc, 0, NULL, 0, NULL, 0, NULL,
			                                  LOCK_COUNT, tx_buffer, LOCK_RSP_SIZE, rx_buffer);
			if (ret_code != SHA204_SUCCESS) {
				Serial.print(F("Configuration lock failed. Response: "));
				Serial.println(ret_code, HEX);
				halt();
			} else {
				Serial.println(F("Configuration locked."));

				// Update lock flags after locking
				ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
				if (ret_code != SHA204_SUCCESS) {
					Serial.print(F("Failed to determine device lock status. Response: "));
					Serial.println(ret_code, HEX);
					halt();
				} else {
					lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
					lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
				}
			}
		}
#ifndef SKIP_UART_CONFIRMATION
		else {
			Serial.println(F("Unexpected answer. Skipping lock."));
		}
#endif //not SKIP_UART_CONFIRMATION
#else //LOCK_CONFIGURATION
		Serial.println(F("Configuration not locked. Define LOCK_CONFIGURATION to lock for real."));
#endif
	} else {
		Serial.println(F("Skipping configuration write and lock (configuration already locked)."));
		Serial.println(F("Chip configuration:"));
		dump_configuration();
	}

#ifdef SKIP_KEY_STORAGE
	Serial.println(F("Disable SKIP_KEY_STORAGE to store key."));
#else
#ifdef USER_KEY
	memcpy(key, user_key_data, 32);
	Serial.println(F("Using this user supplied HMAC key:"));
#else
	// Retrieve random value to use as key
	ret_code = sha204.sha204m_random(tx_buffer, rx_buffer, RANDOM_SEED_UPDATE);
	if (ret_code != SHA204_SUCCESS) {
		Serial.print(F("Random key generation failed. Response: "));
		Serial.println(ret_code, HEX);
		halt();
	} else {
		memcpy(key, rx_buffer+SHA204_BUFFER_POS_DATA, 32);
	}
	if (lockConfig == 0x00) {
		Serial.println(F("Take note of this key, it will never be the shown again:"));
	} else {
		Serial.println(F("Key is not randomized (configuration not locked):"));
	}
#endif
	Serial.print("#define MY_HMAC_KEY ");
	for (int i=0; i<32; i++) {
		Serial.print("0x");
		if (key[i] < 0x10) {
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(key[i], HEX);
		if (i < 31) {
			Serial.print(',');
		}
		if (i+1 == 16) {
			Serial.print("\\\n                    ");
		}
	}
	Serial.println();

	// It will not be possible to write the key if the configuration zone is unlocked
	if (lockConfig == 0x00) {
		// Write the key to the appropriate slot in the data zone
		Serial.println(F("Writing key to slot 0..."));
		write_key(key);
	} else {
		Serial.println(F("Skipping key storage (configuration not locked)."));
		Serial.println(F("The configuration must be locked to be able to write a key."));
	}
#endif

	if (lockValue != 0x00) {
#ifdef LOCK_DATA
#ifndef SKIP_UART_CONFIRMATION
		while (Serial.available()) {
			Serial.read();
		}
		Serial.println(F("Send SPACE character to lock data..."));
		while (Serial.available() == 0);
		if (Serial.read() == ' ')
#endif //not SKIP_UART_CONFIRMATION
		{
			// Correct sequence, resync chip
			ret_code = sha204.sha204c_resync(SHA204_RSP_SIZE_MAX, rx_buffer);
			if (ret_code != SHA204_SUCCESS && ret_code != SHA204_RESYNC_WITH_WAKEUP) {
				Serial.print(F("Resync failed. Response: "));
				Serial.println(ret_code, HEX);
				halt();
			}

			// If configuration is unlocked, key is not updated. Locking data in this case will cause
			// slot 0 to contain an unknown (or factory default) key, and this is in practically any
			// usecase not the desired behaviour, so ask for additional confirmation in this case.
			if (lockConfig != 0x00) {
				while (Serial.available()) {
					Serial.read();
				}
				Serial.println(F("*** ATTENTION ***"));
				Serial.println(F("Configuration is not locked. Are you ABSULOUTELY SURE you want to lock data?"));
				Serial.println(F("Locking data at this stage will cause slot 0 to contain a factory default key"));
				Serial.println(
				    F("which cannot be change after locking is done. This is in practically any usecase"));
				Serial.println(F("NOT the desired behavour. Send SPACE character now to lock data anyway..."));
				while (Serial.available() == 0);
				if (Serial.read() != ' ') {
					Serial.println(F("Unexpected answer. Skipping lock."));
					halt();
				}
			}

			// Lock data zone
			ret_code = sha204.sha204m_execute(SHA204_LOCK, SHA204_ZONE_DATA | LOCK_ZONE_NO_CRC,
			                                  0x0000, 0, NULL, 0, NULL, 0, NULL,
			                                  LOCK_COUNT, tx_buffer, LOCK_RSP_SIZE, rx_buffer);
			if (ret_code != SHA204_SUCCESS) {
				Serial.print(F("Data lock failed. Response: "));
				Serial.println(ret_code, HEX);
				halt();
			} else {
				Serial.println(F("Data locked."));

				// Update lock flags after locking
				ret_code = sha204.sha204m_read(tx_buffer, rx_buffer, SHA204_ZONE_CONFIG, 0x15<<2);
				if (ret_code != SHA204_SUCCESS) {
					Serial.print(F("Failed to determine device lock status. Response: "));
					Serial.println(ret_code, HEX);
					halt();
				} else {
					lockConfig = rx_buffer[SHA204_BUFFER_POS_DATA+3];
					lockValue = rx_buffer[SHA204_BUFFER_POS_DATA+2];
				}
			}
		}
#ifndef SKIP_UART_CONFIRMATION
		else {
			Serial.println(F("Unexpected answer. Skipping lock."));
		}
#endif //not SKIP_UART_CONFIRMATION
#else //LOCK_DATA
		Serial.println(F("Data not locked. Define LOCK_DATA to lock for real."));
#endif
	} else {
		Serial.println(F("Skipping OTP/data zone lock (zone already locked)."));
	}
#endif // not USE_SOFT_SIGNING

	Serial.println(F("--------------------------------"));
	Serial.println(F("Personalization is now complete."));
#ifndef USE_SOFT_SIGNING
	Serial.print(F("Configuration is "));
	if (lockConfig == 0x00) {
		Serial.println("LOCKED");
	} else {
		Serial.println("UNLOCKED");
	}
	Serial.print(F("Data is "));
	if (lockValue == 0x00) {
		Serial.println("LOCKED");
	} else {
		Serial.println("UNLOCKED");
	}
#endif
}

/** @brief Sketch execution code */
void loop()
{
}
