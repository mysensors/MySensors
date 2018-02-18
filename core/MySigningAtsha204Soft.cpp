/*
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
 * could compromise the key used in the signed message infrastructure if device is lost and its
 * memory dumped.
 *
 */

#include "MySigning.h"

#ifdef MY_SIGNING_SOFT
#include "drivers/ATSHA204/sha256.h"
#define SIGNING_IDENTIFIER (1) //HMAC-SHA256

#if defined(MY_DEBUG_VERBOSE_SIGNING)
#define SIGN_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)
static char printStr[65];
static char i2h(uint8_t i)
{
	uint8_t k = i & 0x0F;
	if (k <= 9) {
		return '0' + k;
	} else {
		return 'A' + k - 10;
	}
}

static void buf2str(const uint8_t* buf, size_t sz)
{
	uint8_t i;
	if (sz > 32) {
		sz = 32; //clamp to 32 bytes
	}
	for (i = 0; i < sz; i++) {
		printStr[i * 2] = i2h(buf[i] >> 4);
		printStr[(i * 2) + 1] = i2h(buf[i]);
	}
	printStr[sz * 2] = '\0';
}
#else
#define SIGN_DEBUG(x,...)
#endif

HmacClass _signing_sha256;
static unsigned long _signing_timestamp;
static bool _signing_verification_ongoing = false;
static uint8_t _signing_verifying_nonce[32+9+1];
static uint8_t _signing_signing_nonce[32+9+1];
static uint8_t _signing_temp_message[32];
static uint8_t _signing_hmac_key[32];
static uint8_t _signing_hmac[32];
static uint8_t _signing_node_serial_info[9];
#ifdef MY_SIGNING_NODE_WHITELISTING
static const whitelist_entry_t _signing_whitelist[] = MY_SIGNING_NODE_WHITELISTING;
#endif

static bool init_ok = false;
static const uint8_t reset_serial[9] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void signerCalculateSignature(MyMessage &msg, bool signing);
static uint8_t* signerAtsha204AHmac(const uint8_t* nonce, const uint8_t* data);
static uint8_t* signerSha256(const uint8_t* data, size_t sz);

bool signerAtsha204SoftInit(void)
{
	init_ok = true;

	// initialize pseudo-RNG
	hwRandomNumberInit();
	// Set secrets
#ifdef MY_SIGNING_SIMPLE_PASSWD
	if (strnlen(MY_SIGNING_SIMPLE_PASSWD, 32) < 8) {
		SIGN_DEBUG(PSTR("!SGN:BND:PWD<8\n")); //Password is too short to be acceptable
		init_ok = false;
	} else {
		memset(_signing_hmac_key, 0, 32);
		memcpy(_signing_hmac_key, MY_SIGNING_SIMPLE_PASSWD, strnlen(MY_SIGNING_SIMPLE_PASSWD, 32));
		memset(_signing_node_serial_info, 0, 9);
		memcpy(_signing_node_serial_info, MY_SIGNING_SIMPLE_PASSWD,
		       strnlen(MY_SIGNING_SIMPLE_PASSWD, 8));
		_signing_node_serial_info[8] = getNodeId();
	}
#else
	hwReadConfigBlock((void*)_signing_hmac_key, (void*)EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS, 32);
	hwReadConfigBlock((void*)_signing_node_serial_info, (void*)EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, 9);
#endif
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
		return false;
	}
	if (_signing_verification_ongoing) {
		unsigned long time_now = hwMillis();
		// If timestamp is taken so late a rollover can take place during the timeout,
		// offset both timestamp and current time to make sure no rollover takes place during the
		// timeout
		if (_signing_timestamp + MY_VERIFICATION_TIMEOUT_MS < _signing_timestamp) {
			_signing_timestamp += MY_VERIFICATION_TIMEOUT_MS;
			time_now += MY_VERIFICATION_TIMEOUT_MS;
		}
		if (time_now > _signing_timestamp + MY_VERIFICATION_TIMEOUT_MS) {
			SIGN_DEBUG(PSTR("!SGN:BND:TMR\n")); //Verification timeout
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
		return false;
	}

#ifdef MY_HW_HAS_GETENTROPY
	// Try to get MAX_PAYLOAD random bytes
	while (hwGetentropy(&_signing_verifying_nonce, MAX_PAYLOAD) != MAX_PAYLOAD);
#else
	// We used a basic whitening technique that XORs a random byte with the current hwMillis() counter
	// and then the byte is hashed (SHA256) to produce the resulting nonce
	_signing_sha256.init();
	for (int i = 0; i < 32; i++) {
		_signing_sha256.write(random(256) ^ (hwMillis()&0xFF));
	}
	memcpy(_signing_verifying_nonce, _signing_sha256.result(), MIN(MAX_PAYLOAD, 32));
#endif

	if (MAX_PAYLOAD < 32) {
		// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
		memset(&_signing_verifying_nonce[MAX_PAYLOAD], 0xAA, 32-MAX_PAYLOAD);
	}

	// Transfer the first part of the nonce to the message
	msg.set(_signing_verifying_nonce, MIN(MAX_PAYLOAD, 32));
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
		return;
	}

	memcpy(_signing_signing_nonce, (uint8_t*)msg.getCustom(), MIN(MAX_PAYLOAD, 32));
	if (MAX_PAYLOAD < 32) {
		// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
		memset(&_signing_signing_nonce[MAX_PAYLOAD], 0xAA, 32-MAX_PAYLOAD);
	}
}

bool signerAtsha204SoftSignMsg(MyMessage &msg)
{
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		SIGN_DEBUG(PSTR("!SGN:BND:SIG,SIZE,%" PRIu8 ">%" PRIu8 "\n"), mGetLength(msg),
		           MAX_PAYLOAD-2); //Message too large
		return false;
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	signerCalculateSignature(msg, true);

	if (DO_WHITELIST(msg.destination)) {
		// Salt the signature with the senders nodeId and the (hopefully) unique serial The Creator has
		// provided. We can reuse the nonce buffer now since it is no longer needed
		memcpy(_signing_signing_nonce, _signing_hmac, 32);
		_signing_signing_nonce[32] = msg.sender;
		memcpy(&_signing_signing_nonce[33], _signing_node_serial_info, 9);
		memcpy(_signing_hmac, signerSha256(_signing_signing_nonce, 32+1+9), 32);
		SIGN_DEBUG(PSTR("SGN:BND:SIG WHI,ID=%" PRIu8 "\n"), msg.sender);
#ifdef MY_DEBUG_VERBOSE_SIGNING
		buf2str(_signing_node_serial_info, 9);
		SIGN_DEBUG(PSTR("SGN:BND:SIG WHI,SERIAL=%s\n"), printStr);
#endif
	}

	// Overwrite the first byte in the signature with the signing identifier
	_signing_hmac[0] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], _signing_hmac, MIN(MAX_PAYLOAD-mGetLength(msg), 32));

	return true;
}

bool signerAtsha204SoftVerifyMsg(MyMessage &msg)
{
	if (!_signing_verification_ongoing) {
		SIGN_DEBUG(PSTR("!SGN:BND:VER ONGOING\n"));
		return false;
	} else {
		// Make sure we have not expired
		if (!signerCheckTimer()) {
			return false;
		}

		_signing_verification_ongoing = false;

		if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER) {
			SIGN_DEBUG(PSTR("!SGN:BND:VER,IDENT=%" PRIu8 "\n"), msg.data[mGetLength(msg)]);
			return false;
		}

		signerCalculateSignature(msg, false); // Get signature of message

#ifdef MY_SIGNING_NODE_WHITELISTING
		// Look up the senders nodeId in our whitelist and salt the signature with that data
		size_t j;
		for (j=0; j < NUM_OF(_signing_whitelist); j++) {
			if (_signing_whitelist[j].nodeId == msg.sender) {
				// We can reuse the nonce buffer now since it is no longer needed
				memcpy(_signing_verifying_nonce, _signing_hmac, 32);
				_signing_verifying_nonce[32] = msg.sender;
				memcpy(&_signing_verifying_nonce[33], _signing_whitelist[j].serial, 9);
				memcpy(_signing_hmac, signerSha256(_signing_verifying_nonce, 32+1+9), 32);
				SIGN_DEBUG(PSTR("SGN:BND:VER WHI,ID=%" PRIu8 "\n"), msg.sender);
#ifdef MY_DEBUG_VERBOSE_SIGNING
				buf2str(_signing_whitelist[j].serial, 9);
				SIGN_DEBUG(PSTR("SGN:BND:VER WHI,SERIAL=%s\n"), printStr);
#endif
				break;
			}
		}
		if (j == NUM_OF(_signing_whitelist)) {
			SIGN_DEBUG(PSTR("!SGN:BND:VER WHI,ID=%" PRIu8 " MISSING\n"), msg.sender);
			return false;
		}
#endif

		// Overwrite the first byte in the signature with the signing identifier
		_signing_hmac[0] = SIGNING_IDENTIFIER;

		// Compare the calculated signature with the provided signature
		if (signerMemcmp(&msg.data[mGetLength(msg)], _signing_hmac,
		                 MIN(MAX_PAYLOAD-mGetLength(msg), 32))) {
			return false;
		} else {
			return true;
		}
	}
}

// Helper to calculate signature of msg (returned in _signing_hmac)
static void signerCalculateSignature(MyMessage &msg, bool signing)
{
	// Signature is calculated on everything expect the first byte in the header
	uint16_t bytes_left = mGetLength(msg)+HEADER_SIZE-1;
	int16_t current_pos = 1-(int16_t)HEADER_SIZE; // Start at the second byte in the header
	uint8_t* nonce = signing ? _signing_signing_nonce : _signing_verifying_nonce;

#ifdef MY_DEBUG_VERBOSE_SIGNING
	buf2str(nonce, 32);
	SIGN_DEBUG(PSTR("SGN:BND:NONCE=%s\n"), printStr);
#endif

	while (bytes_left) {
		uint16_t bytes_to_include = MIN(bytes_left, 32);

		memset(_signing_temp_message, 0, 32);
		memcpy(_signing_temp_message, (uint8_t*)&msg.data[current_pos], bytes_to_include);

		// We can 'void' signerAtsha204AHmac because the HMAC is already put in the correct place
		(void)signerAtsha204AHmac(nonce, _signing_temp_message);
		// Purge nonce when used
		memset(nonce, 0xAA, 32);

		bytes_left -= bytes_to_include;
		current_pos += bytes_to_include;

		if (bytes_left > 0) {
			// We will do another pass, use current HMAC as nonce for the next HMAC
			memcpy(nonce, _signing_hmac, 32);
		}
	}
#ifdef MY_DEBUG_VERBOSE_SIGNING
	buf2str(_signing_hmac, 32);
	SIGN_DEBUG(PSTR("SGN:BND:HMAC=%s\n"), printStr);
#endif
}

// Helper to calculate a ATSHA204A specific HMAC-SHA256 using provided 32 byte nonce and data
// (zero padded to 32 bytes)
// The pointer to the HMAC is returned, but the HMAC is also stored in _signing_hmac
static uint8_t* signerAtsha204AHmac(const uint8_t* nonce, const uint8_t* data)
{
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
		_signing_sha256.write(data[i]);
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
		_signing_sha256.write(nonce[i]);
	}
	memcpy(_signing_hmac, _signing_sha256.result(), 32);

	// Feed "message" to HMAC calculator
	_signing_sha256.initHmac(_signing_hmac_key, 32); // Set the key to use
	for (int i=0; i<32; i++) {
		_signing_sha256.write(0x00); // 32 bytes zeroes
	}
	for (int i=0; i<32; i++) {
		_signing_sha256.write(_signing_hmac[i]); // 32 bytes digest
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
	return _signing_hmac;
}

// Helper to calculate a generic SHA256 digest of provided buffer
// The pointer to the hash is returned
static uint8_t* signerSha256(const uint8_t* data, size_t sz)
{
	_signing_sha256.init();
	for (size_t i=0; i<sz; i++) {
		_signing_sha256.write(data[i]);
	}
	return _signing_sha256.result();
}
#endif //MY_SIGNING_SOFT
