/**
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

#define SIGNING_IDENTIFIER (1)

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

#ifdef MY_DEBUG_VERBOSE_SIGNING
static char i2h(uint8_t i)
{
	uint8_t k = i & 0x0F;
	if (k <= 9) {
		return '0' + k;
	} else {
		return 'A' + k - 10;
	}
}

#ifdef __linux__
#define __FlashStringHelper char
#define MY_SERIALDEVICE.print debug
#endif

static void DEBUG_SIGNING_PRINTBUF(const __FlashStringHelper* str, uint8_t* buf, uint8_t sz)
{
	static char printBuffer[300];
	if (NULL == buf) {
		return;
	}
#if defined(MY_GATEWAY_FEATURE) && !defined(__linux__)
	// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
	snprintf_P(printBuffer, 299, PSTR("0;255;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
	MY_SERIALDEVICE.print(printBuffer);
#endif
	for (int i=0; i<sz; i++) {
		printBuffer[i * 2] = i2h(buf[i] >> 4);
		printBuffer[(i * 2) + 1] = i2h(buf[i]);
	}
	printBuffer[sz * 2] = '\0';
#if defined(MY_GATEWAY_FEATURE) && !defined(__linux__)
	// Truncate message if this is gateway node
	printBuffer[MY_GATEWAY_MAX_SEND_LENGTH-1-strlen_P((const char*)str)] = '\0';
#endif
	MY_SERIALDEVICE.print(str);
	if (sz > 0) {
		MY_SERIALDEVICE.print(printBuffer);
	}
#if !defined(__linux__)
	MY_SERIALDEVICE.println("");
#endif
}
#else
#define DEBUG_SIGNING_PRINTBUF(str, buf, sz)
#endif

void signerAtsha204SoftInit(void)
{
	// initialize pseudo-RNG
	hwRandomNumberInit();
	// Set secrets
	hwReadConfigBlock((void*)_signing_hmac_key, (void*)EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
	hwReadConfigBlock((void*)_signing_node_serial_info, (void*)EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
}

bool signerAtsha204SoftCheckTimer(void)
{
	if (_signing_verification_ongoing) {
		if (hwMillis() < _signing_timestamp ||
		        hwMillis() > _signing_timestamp + MY_VERIFICATION_TIMEOUT_MS) {
			DEBUG_SIGNING_PRINTBUF(F("Verification timeout"), NULL, 0);
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
	DEBUG_SIGNING_PRINTBUF(F("Signing backend: ATSHA204Soft"), NULL, 0);

	// We used a basic whitening technique that XORs a random byte with the current hwMillis() counter and then the byte is
	// hashed (SHA256) to produce the resulting nonce
	_signing_sha256.init();
	for (int i = 0; i < 32; i++) {
		_signing_sha256.write(random(256) ^ (hwMillis()&0xFF));
	}
	memcpy(_signing_verifying_nonce, _signing_sha256.result(), MAX_PAYLOAD);
	DEBUG_SIGNING_PRINTBUF(F("SHA256: "), _signing_verifying_nonce, 32);

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
	DEBUG_SIGNING_PRINTBUF(F("Signing backend: ATSHA204Soft"), NULL, 0);

	memcpy(_signing_signing_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&_signing_signing_nonce[MAX_PAYLOAD], 0xAA, sizeof(_signing_signing_nonce)-MAX_PAYLOAD);
}

bool signerAtsha204SoftSignMsg(MyMessage &msg)
{
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_SIGNING_PRINTBUF(F("Message too large"), NULL, 0);
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
		DEBUG_SIGNING_PRINTBUF(F("SHA256: "), _signing_hmac, 32);
		DEBUG_SIGNING_PRINTBUF(F("Signature salted with serial"), NULL, 0);
	}

	// Overwrite the first byte in the signature with the signing identifier
	_signing_hmac[0] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], _signing_hmac, MAX_PAYLOAD-mGetLength(msg));
	DEBUG_SIGNING_PRINTBUF(F("Signature in message: "), (uint8_t*)&msg.data[mGetLength(msg)],
	                       MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool signerAtsha204SoftVerifyMsg(MyMessage &msg)
{
	if (!_signing_verification_ongoing) {
		DEBUG_SIGNING_PRINTBUF(F("No active verification session"), NULL, 0);
		return false;
	} else {
		// Make sure we have not expired
		if (!signerCheckTimer()) {
			return false;
		}

		_signing_verification_ongoing = false;

		if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER) {
			DEBUG_SIGNING_PRINTBUF(F("Incorrect signing identifier"), NULL, 0);
			return false;
		}

		// Get signature of message
		DEBUG_SIGNING_PRINTBUF(F("Signature in message: "), (uint8_t*)&msg.data[mGetLength(msg)],
		                       MAX_PAYLOAD-mGetLength(msg));
		signerCalculateSignature(msg, false);

#ifdef MY_SIGNING_NODE_WHITELISTING
		// Look up the senders nodeId in our whitelist and salt the signature with that data
		size_t j;
		for (j=0; j < NUM_OF(_signing_whitelist); j++) {
			if (_signing_whitelist[j].nodeId == msg.sender) {
				DEBUG_SIGNING_PRINTBUF(F("Sender found in whitelist"), NULL, 0);
				_signing_sha256.init();
				for (int i=0; i<32; i++) {
					_signing_sha256.write(_signing_hmac[i]);
				}
				_signing_sha256.write(msg.sender);
				for (int i=0; i<SHA204_SERIAL_SZ; i++) {
					_signing_sha256.write(_signing_whitelist[j].serial[i]);
				}
				memcpy(_signing_hmac, _signing_sha256.result(), 32);
				DEBUG_SIGNING_PRINTBUF(F("SHA256: "), _signing_hmac, 32);
				break;
			}
		}
		if (j == NUM_OF(_signing_whitelist)) {
			DEBUG_SIGNING_PRINTBUF(F("Sender not found in whitelist, message rejected!"), NULL, 0);
			return false;
		}
#endif

		// Overwrite the first byte in the signature with the signing identifier
		_signing_hmac[0] = SIGNING_IDENTIFIER;

		// Compare the caluclated signature with the provided signature
		if (signerMemcmp(&msg.data[mGetLength(msg)], _signing_hmac, MAX_PAYLOAD-mGetLength(msg))) {
			DEBUG_SIGNING_PRINTBUF(F("Signature bad: "), _signing_hmac, MAX_PAYLOAD-mGetLength(msg));
#ifdef MY_SIGNING_NODE_WHITELISTING
			DEBUG_SIGNING_PRINTBUF(F("Is the sender whitelisted and serial correct?"), NULL, 0);
#endif
			return false;
		} else {
			DEBUG_SIGNING_PRINTBUF(F("Signature OK"), NULL, 0);
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
	DEBUG_SIGNING_PRINTBUF(F("Message to process: "), (uint8_t*)&msg.data[1-HEADER_SIZE],
	                       MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
	DEBUG_SIGNING_PRINTBUF(F("Current nonce: "),
	                       signing ? _signing_signing_nonce : _signing_verifying_nonce, 32);

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

	DEBUG_SIGNING_PRINTBUF(F("HMAC: "), _signing_hmac, 32);
}
