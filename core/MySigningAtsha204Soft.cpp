/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
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
 *
 * DESCRIPTION
 * Signing support created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
 * ATSHA204 emulated signing backend. The emulated ATSHA204 implementation offers pseudo random
 * number generation and HMAC-SHA256 authentication compatible with a "physical" ATSHA204.
 * NOTE: Key is stored in clear text in the Arduino firmware. Therefore, the use of this back-end
 * could compromise the key used in the signed message infrastructure if device is lost and its memory
 * dumped.
 *
 */

#include "MySigning.h"
#include "drivers/ATSHA204/sha256.h"

#define SIGNING_IDENTIFIER (1) //HMAC-SHA256

#if defined(MY_DEBUG_VERBOSE_SIGNING)
#define SIGN_DEBUG(x,...) hwDebugPrint(x, ##__VA_ARGS__)
#else
#define SIGN_DEBUG(x,...)
#endif

// Define MY_DEBUG_VERBOSE_SIGNING in your sketch to enable signing backend debugprints

Sha256Class _signing_sha256;
unsigned long _signing_timestamp;
bool _signing_verification_ongoing = false;
uint8_t _signing_verifying_nonce[32];
uint8_t _signing_signing_nonce[32];
uint8_t _signing_temp_message[32];
static uint8_t _signing_hmac_key[32];
uint8_t _signing_hmac[32];
extern uint8_t _doWhitelist[32];

static uint8_t _signing_node_serial_info[9];
#ifdef MY_SIGNING_NODE_WHITELISTING
const whitelist_entry_t _signing_whitelist[] = MY_SIGNING_NODE_WHITELISTING;
#endif

static void signerCalculateSignature(MyMessage &msg, bool signing);

static bool init_ok = false;
static const uint8_t reset_serial[9] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

bool signerAtsha204SoftInit(void)
{
	init_ok = true;

	// initialize pseudo-RNG
	hwRandomNumberInit();
	// Set secrets
	hwReadConfigBlock((void*)_signing_hmac_key, (void*)EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
	hwReadConfigBlock((void*)_signing_node_serial_info, (void*)EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
	if (!memcmp(_signing_node_serial_info, reset_serial, 9)) {
		unique_id_t uniqueID;
		// There is no serial, attempt to get unique ID for serial instead
		if (hwUniqueID(&uniqueID)) {
			// There is a unique serial, use that
			memcpy(_signing_node_serial_info, uniqueID, 9);
		}
	}
	return init_ok;
}

bool signerAtsha204SoftCheckTimer(void)
{
	if (!init_ok) {
		SIGN_DEBUG(PSTR("Backend not initialized\n"));
		return false;
	}
	if (_signing_verification_ongoing) {
		if (hwMillis() < _signing_timestamp ||
		        hwMillis() > _signing_timestamp + MY_VERIFICATION_TIMEOUT_MS) {
			SIGN_DEBUG(PSTR("Verification timeout\n"));
			// Purge nonce
			memset(_signing_signing_nonce, 0xAA, 32);
			memset(_signing_verifying_nonce, 0xAA, 32);
			_signing_verification_ongoing = false;
			return false;
		}
	}
	return true;
}

bool signerAtsha204SoftGetNonce(MyMessage &msg)
{
	if (!init_ok) {
		SIGN_DEBUG(PSTR("Backend not initialized\n"));
		return false;
	}
	SIGN_DEBUG(PSTR("Signing backend: ATSHA204Soft\n"));

	// We used a basic whitening technique that XORs a random byte with the current hwMillis() counter and then the byte is
	// hashed (SHA256) to produce the resulting nonce
	_signing_sha256.init();
	for (int i = 0; i < 32; i++) {
		_signing_sha256.write(random(256) ^ (hwMillis()&0xFF));
	}
	memcpy(_signing_verifying_nonce, _signing_sha256.result(), MAX_PAYLOAD);

	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&_signing_verifying_nonce[MAX_PAYLOAD], 0xAA, sizeof(_signing_verifying_nonce)-MAX_PAYLOAD);

	// Transfer the first part of the nonce to the message
	msg.set(_signing_verifying_nonce, MAX_PAYLOAD);
	_signing_verification_ongoing = true;
	_signing_timestamp = hwMillis(); // Set timestamp to determine when to purge nonce
	// Be a little fancy to handle turnover (prolong the time allowed to timeout after turnover)
	// Note that if message is "too" quick, and arrives before turnover, it will be rejected
	// but this is consider such a rare case that it is accepted and rejects are 'safe'
	if (_signing_timestamp + MY_VERIFICATION_TIMEOUT_MS < hwMillis()) {
		_signing_timestamp = 0;
	}
	return true;
}

void signerAtsha204SoftPutNonce(MyMessage &msg)
{
	if (!init_ok) {
		SIGN_DEBUG(PSTR("Backend not initialized\n"));
	}
	SIGN_DEBUG(PSTR("Signing backend: ATSHA204Soft\n"));

	memcpy(_signing_signing_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&_signing_signing_nonce[MAX_PAYLOAD], 0xAA, sizeof(_signing_signing_nonce)-MAX_PAYLOAD);
}

bool signerAtsha204SoftSignMsg(MyMessage &msg)
{
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		SIGN_DEBUG(PSTR("Message too large\n"));
		return false;
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	signerCalculateSignature(msg, true);

	if (DO_WHITELIST(msg.destination)) {
		// Salt the signature with the senders nodeId and the (hopefully) unique serial The Creator has provided
		_signing_sha256.init();
		for (int i=0; i<32; i++) {
			_signing_sha256.write(_signing_hmac[i]);
		}
		_signing_sha256.write(msg.sender);
		for (int i=0; i<SHA204_SERIAL_SZ; i++) {
			_signing_sha256.write(_signing_node_serial_info[i]);
		}
		memcpy(_signing_hmac, _signing_sha256.result(), 32);
		SIGN_DEBUG(PSTR("Signature salted with serial: %02X%02X%02X%02X%02X%02X%02X%02X%02X\n"),
		           _signing_node_serial_info[0], _signing_node_serial_info[1], _signing_node_serial_info[2],
		           _signing_node_serial_info[3], _signing_node_serial_info[4], _signing_node_serial_info[5],
		           _signing_node_serial_info[6], _signing_node_serial_info[7], _signing_node_serial_info[8]);
	}

	// Overwrite the first byte in the signature with the signing identifier
	_signing_hmac[0] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], _signing_hmac, MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool signerAtsha204SoftVerifyMsg(MyMessage &msg)
{
	if (!_signing_verification_ongoing) {
		SIGN_DEBUG(PSTR("No active verification session\n"));
		return false;
	} else {
		// Make sure we have not expired
		if (!signerCheckTimer()) {
			return false;
		}

		_signing_verification_ongoing = false;

		if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER) {
			SIGN_DEBUG(PSTR("Incorrect signing identifier\n"));
			return false;
		}

		signerCalculateSignature(msg, false); // Get signature of message

#ifdef MY_SIGNING_NODE_WHITELISTING
		// Look up the senders nodeId in our whitelist and salt the signature with that data
		size_t j;
		for (j=0; j < NUM_OF(_signing_whitelist); j++) {
			if (_signing_whitelist[j].nodeId == msg.sender) {
				SIGN_DEBUG(PSTR("Sender found in whitelist\n"));
				_signing_sha256.init();
				for (int i=0; i<32; i++) {
					_signing_sha256.write(_signing_hmac[i]);
				}
				_signing_sha256.write(msg.sender);
				for (int i=0; i<SHA204_SERIAL_SZ; i++) {
					_signing_sha256.write(_signing_whitelist[j].serial[i]);
				}
				memcpy(_signing_hmac, _signing_sha256.result(), 32);
				break;
			}
		}
		if (j == NUM_OF(_signing_whitelist)) {
			SIGN_DEBUG(PSTR("Sender not found in whitelist, message rejected!\n"));
			return false;
		}
#endif

		// Overwrite the first byte in the signature with the signing identifier
		_signing_hmac[0] = SIGNING_IDENTIFIER;

		// Compare the caluclated signature with the provided signature
		if (signerMemcmp(&msg.data[mGetLength(msg)], _signing_hmac, MAX_PAYLOAD-mGetLength(msg))) {
			SIGN_DEBUG(PSTR("Signature bad\n"));
#ifdef MY_SIGNING_NODE_WHITELISTING
			SIGN_DEBUG(PSTR("Is the sender whitelisted and serial correct?\n"));
#endif
			return false;
		} else {
			SIGN_DEBUG(PSTR("Signature OK\n"));
			return true;
		}
	}
}

// Helper to calculate signature of msg (returned in hmac)
static void signerCalculateSignature(MyMessage &msg, bool signing)
{
	memset(_signing_temp_message, 0, 32);
	memcpy(_signing_temp_message, (uint8_t*)&msg.data[1-HEADER_SIZE],
	       MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
	SIGN_DEBUG(PSTR("Current nonce: "
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n"),
	           signing ? _signing_signing_nonce[0]  : _signing_verifying_nonce[0],
	           signing ? _signing_signing_nonce[1]  : _signing_verifying_nonce[1],
	           signing ? _signing_signing_nonce[2]  : _signing_verifying_nonce[2],
	           signing ? _signing_signing_nonce[3]  : _signing_verifying_nonce[3],
	           signing ? _signing_signing_nonce[4]  : _signing_verifying_nonce[4],
	           signing ? _signing_signing_nonce[5]  : _signing_verifying_nonce[5],
	           signing ? _signing_signing_nonce[6]  : _signing_verifying_nonce[6],
	           signing ? _signing_signing_nonce[7]  : _signing_verifying_nonce[7],
	           signing ? _signing_signing_nonce[8]  : _signing_verifying_nonce[8],
	           signing ? _signing_signing_nonce[9]  : _signing_verifying_nonce[9],
	           signing ? _signing_signing_nonce[10] : _signing_verifying_nonce[10],
	           signing ? _signing_signing_nonce[11] : _signing_verifying_nonce[11],
	           signing ? _signing_signing_nonce[12] : _signing_verifying_nonce[12],
	           signing ? _signing_signing_nonce[13] : _signing_verifying_nonce[13],
	           signing ? _signing_signing_nonce[14] : _signing_verifying_nonce[14],
	           signing ? _signing_signing_nonce[15] : _signing_verifying_nonce[15],
	           signing ? _signing_signing_nonce[16] : _signing_verifying_nonce[16],
	           signing ? _signing_signing_nonce[17] : _signing_verifying_nonce[17],
	           signing ? _signing_signing_nonce[18] : _signing_verifying_nonce[18],
	           signing ? _signing_signing_nonce[19] : _signing_verifying_nonce[19],
	           signing ? _signing_signing_nonce[20] : _signing_verifying_nonce[20],
	           signing ? _signing_signing_nonce[21] : _signing_verifying_nonce[21],
	           signing ? _signing_signing_nonce[22] : _signing_verifying_nonce[22],
	           signing ? _signing_signing_nonce[23] : _signing_verifying_nonce[23],
	           signing ? _signing_signing_nonce[24] : _signing_verifying_nonce[24],
	           signing ? _signing_signing_nonce[25] : _signing_verifying_nonce[25],
	           signing ? _signing_signing_nonce[26] : _signing_verifying_nonce[26],
	           signing ? _signing_signing_nonce[27] : _signing_verifying_nonce[27],
	           signing ? _signing_signing_nonce[28] : _signing_verifying_nonce[28],
	           signing ? _signing_signing_nonce[29] : _signing_verifying_nonce[29],
	           signing ? _signing_signing_nonce[30] : _signing_verifying_nonce[30],
	           signing ? _signing_signing_nonce[31] : _signing_verifying_nonce[31]
	          );

	// ATSHA204 calculates the HMAC with a PSK and a SHA256 digest of the following data:
	// 32 bytes zeroes
	// 32 bytes digest,
	// 1 byte OPCODE (0x11)
	// 1 byte Mode (0x04)
	// 2 bytes SlotID (0x0000)
	// 11 bytes zeroes
	// SN[8] (0xEE)
	// 4 bytes zeroes
	// SN[0:1] (0x0123)
	// 2 bytes zeroes

	// The digest is calculated as a SHA256 digest of the following:
	// 32 bytes message
	// 1 byte OPCODE (0x15)
	// 1 byte param1 (0x02)
	// 2 bytes param2 (0x0800)
	// SN[8] (0xEE)
	// SN[0:1] (0x0123)
	// 25 bytes zeroes
	// 32 bytes nonce

	// Calculate message digest first
	_signing_sha256.init();
	for (int i=0; i<32; i++) {
		_signing_sha256.write(_signing_temp_message[i]);
	}
	_signing_sha256.write(0x15); // OPCODE
	_signing_sha256.write(0x02); // param1
	_signing_sha256.write(0x08); // param2(1)
	_signing_sha256.write(0x00); // param2(2)
	_signing_sha256.write(0xEE); // SN[8]
	_signing_sha256.write(0x01); // SN[0]
	_signing_sha256.write(0x23); // SN[1]
	for (int i=0; i<25; i++) {
		_signing_sha256.write(0x00);
	}
	for (int i=0; i<32; i++) {
		_signing_sha256.write(signing ? _signing_signing_nonce[i] : _signing_verifying_nonce[i]);
	}
	// Purge nonce when used
	memset(signing ? _signing_signing_nonce : _signing_verifying_nonce, 0xAA, 32);
	memcpy(_signing_temp_message, _signing_sha256.result(), 32);

	// Feed "message" to HMAC calculator
	_signing_sha256.initHmac(_signing_hmac_key,32); // Set the key to use
	for (int i=0; i<32; i++) {
		_signing_sha256.write(0x00); // 32 bytes zeroes
	}
	for (int i=0; i<32; i++) {
		_signing_sha256.write(_signing_temp_message[i]); // 32 bytes digest
	}
	_signing_sha256.write(0x11); // OPCODE
	_signing_sha256.write(0x04); // Mode
	_signing_sha256.write(0x00); // SlotID(1)
	_signing_sha256.write(0x00); // SlotID(2)
	for (int i=0; i<11; i++) {
		_signing_sha256.write(0x00); // 11 bytes zeroes
	}
	_signing_sha256.write(0xEE); // SN[8]
	for (int i=0; i<4; i++) {
		_signing_sha256.write(0x00); // 4 bytes zeroes
	}
	_signing_sha256.write(0x01); // SN[0]
	_signing_sha256.write(0x23); // SN[1]
	for (int i=0; i<2; i++) {
		_signing_sha256.write(0x00); // 2 bytes zeroes
	}

	memcpy(_signing_hmac, _signing_sha256.resultHmac(), 32);

	SIGN_DEBUG(PSTR("HMAC: "
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n"),
	           _signing_hmac[0], _signing_hmac[1], _signing_hmac[2], _signing_hmac[3],
	           _signing_hmac[4], _signing_hmac[5], _signing_hmac[6], _signing_hmac[7],
	           _signing_hmac[8], _signing_hmac[9], _signing_hmac[10], _signing_hmac[11],
	           _signing_hmac[12], _signing_hmac[13], _signing_hmac[14], _signing_hmac[15],
	           _signing_hmac[16], _signing_hmac[17], _signing_hmac[18], _signing_hmac[19],
	           _signing_hmac[20], _signing_hmac[21], _signing_hmac[22], _signing_hmac[23],
	           _signing_hmac[24], _signing_hmac[25], _signing_hmac[26], _signing_hmac[27],
	           _signing_hmac[28], _signing_hmac[29], _signing_hmac[30], _signing_hmac[31]);
}
