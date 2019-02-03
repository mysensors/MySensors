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
 *******************************
 *
 * DESCRIPTION
 * Signing support created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
 * ATSHA204 signing backend. The Atmel ATSHA204 offers true random number generation and
 * HMAC-SHA256 authentication with a readout-protected key.
 *
 */

#include "MySigning.h"
#include "MyHelperFunctions.h"

#ifdef MY_SIGNING_ATSHA204
#define SIGNING_IDENTIFIER (1) //HMAC-SHA256

#if defined(MY_DEBUG_VERBOSE_SIGNING)
#define SIGN_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)
static char printStr[65];
static void buf2str(const uint8_t* buf, size_t sz)
{
	uint8_t i;
	if (sz > 32) {
		sz = 32; //clamp to 32 bytes
	}
	for (i = 0; i < sz; i++) {
		printStr[i * 2] = convertI2H(buf[i] >> 4);
		printStr[(i * 2) + 1] = convertI2H(buf[i]);
	}
	printStr[sz * 2] = '\0';
}
#else
#define SIGN_DEBUG(x,...)
#endif

static unsigned long _signing_timestamp;
static bool _signing_verification_ongoing = false;
static uint8_t _signing_verifying_nonce[32+9+1];
static uint8_t _signing_signing_nonce[32+9+1];
static uint8_t _signing_temp_message[SHA_MSG_SIZE];
static uint8_t _signing_rx_buffer[SHA204_RSP_SIZE_MAX];
static uint8_t _signing_tx_buffer[SHA204_CMD_SIZE_MAX];
static uint8_t* const _signing_hmac = &_signing_rx_buffer[SHA204_BUFFER_POS_DATA];
static uint8_t _signing_node_serial_info[9];
#ifdef MY_SIGNING_NODE_WHITELISTING
static const whitelist_entry_t _signing_whitelist[] = MY_SIGNING_NODE_WHITELISTING;
#endif

static bool init_ok = false;

static void signerCalculateSignature(MyMessage &msg, bool signing);
static uint8_t* signerAtsha204AHmac(const uint8_t* nonce, const uint8_t* data);
static uint8_t* signerSha256(const uint8_t* data, size_t sz);

bool signerAtsha204Init(void)
{
	init_ok = true;
	atsha204_init(MY_SIGNING_ATSHA204_PIN);

	(void)atsha204_wakeup(_signing_temp_message);
	// Read the configuration lock flag to determine if device is personalized or not
	if (atsha204_read(_signing_tx_buffer, _signing_rx_buffer,
	                  SHA204_ZONE_CONFIG, 0x15<<2) != SHA204_SUCCESS) {
		SIGN_DEBUG(PSTR("!SGN:BND:INIT FAIL\n")); //Could not read ATSHA204A lock config
		init_ok = false;
	} else if (_signing_rx_buffer[SHA204_BUFFER_POS_DATA+3] != 0x00) {
		SIGN_DEBUG(PSTR("!SGN:BND:PER\n")); //ATSHA204A not personalized
		init_ok = false;
	}
	if (init_ok) {
		// Get and cache the serial of the ATSHA204A
		if (atsha204_getSerialNumber(_signing_node_serial_info) != SHA204_SUCCESS) {
			SIGN_DEBUG(PSTR("!SGN:BND:SER\n")); //Could not get ATSHA204A serial
			init_ok = false;
		}
	}
	return init_ok;
}

bool signerAtsha204CheckTimer(void)
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

bool signerAtsha204GetNonce(MyMessage &msg)
{
	if (!init_ok) {
		return false;
	}

	// We used a basic whitening technique that XORs each byte in a 32byte random value with current
	// hwMillis() counter. This 32-byte random value is then hashed (SHA256) to produce the resulting
	// nonce
	(void)atsha204_wakeup(_signing_temp_message);
	if (atsha204_execute(SHA204_RANDOM, RANDOM_SEED_UPDATE, 0, 0, NULL,
	                     RANDOM_COUNT, _signing_tx_buffer, RANDOM_RSP_SIZE, _signing_rx_buffer) !=
	        SHA204_SUCCESS) {
		return false;
	}
	for (int i = 0; i < 32; i++) {
		_signing_verifying_nonce[i] = _signing_rx_buffer[SHA204_BUFFER_POS_DATA+i] ^ (hwMillis()&0xFF);
	}
	memcpy(_signing_verifying_nonce, signerSha256(_signing_verifying_nonce, 32),
	       min(MAX_PAYLOAD, 32));

	// We just idle the chip now since we expect to use it soon when the signed message arrives
	atsha204_idle();

	if (MAX_PAYLOAD < 32) {
		// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
		memset(&_signing_verifying_nonce[MAX_PAYLOAD], 0xAA, 32-MAX_PAYLOAD);
	}

	// Transfer the first part of the nonce to the message
	msg.set(_signing_verifying_nonce, min(MAX_PAYLOAD, 32));
	_signing_verification_ongoing = true;
	_signing_timestamp = hwMillis(); // Set timestamp to determine when to purge nonce
	return true;
}

void signerAtsha204PutNonce(MyMessage &msg)
{
	if (!init_ok) {
		return;
	}

	memcpy(_signing_signing_nonce, (uint8_t*)msg.getCustom(), min(MAX_PAYLOAD, 32));
	if (MAX_PAYLOAD < 32) {
		// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
		memset(&_signing_signing_nonce[MAX_PAYLOAD], 0xAA, 32-MAX_PAYLOAD);
	}
}

bool signerAtsha204SignMsg(MyMessage &msg)
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
		// Salt the signature with the senders nodeId and the unique serial of the ATSHA device
		// We can reuse the nonce buffer now since it is no longer needed
		memcpy(_signing_signing_nonce, _signing_hmac, 32);
		_signing_signing_nonce[32] = msg.sender;
		memcpy(&_signing_signing_nonce[33], _signing_node_serial_info, 9);
		// We can 'void' sha256 because the hash is already put in the correct place
		(void)signerSha256(_signing_signing_nonce, 32+1+9);
		SIGN_DEBUG(PSTR("SGN:BND:SIG WHI,ID=%" PRIu8 "\n"), msg.sender);
#ifdef MY_DEBUG_VERBOSE_SIGNING
		buf2str(_signing_node_serial_info, 9);
		SIGN_DEBUG(PSTR("SGN:BND:SIG WHI,SERIAL=%s\n"), printStr);
#endif
	}

	// Put device back to sleep
	atsha204_sleep();

	// Overwrite the first byte in the signature with the signing identifier
	_signing_hmac[0] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], _signing_hmac, min(MAX_PAYLOAD-mGetLength(msg), 32));

	return true;
}

bool signerAtsha204VerifyMsg(MyMessage &msg)
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
				// We can 'void' sha256 because the hash is already put in the correct place
				(void)signerSha256(_signing_verifying_nonce, 32+1+9);
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
			// Put device back to sleep
			atsha204_sleep();
			return false;
		}
#endif

		// Put device back to sleep
		atsha204_sleep();

		// Overwrite the first byte in the signature with the signing identifier
		_signing_hmac[0] = SIGNING_IDENTIFIER;

		// Compare the calculated signature with the provided signature
		if (signerMemcmp(&msg.data[mGetLength(msg)], _signing_hmac,
		                 min(MAX_PAYLOAD-mGetLength(msg), 32))) {
			return false;
		} else {
			return true;
		}
	}
}

// Helper to calculate signature of msg (returned in _signing_rx_buffer[SHA204_BUFFER_POS_DATA])
// (=_signing_hmac)
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
		uint16_t bytes_to_include = min(bytes_left, 32);

		(void)atsha204_wakeup(_signing_temp_message); // Issue wakeup to reset watchdog
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
			atsha204_idle(); // Idle the chip to allow the wakeup call to reset the watchdog
		}
	}
#ifdef MY_DEBUG_VERBOSE_SIGNING
	buf2str(_signing_hmac, 32);
	SIGN_DEBUG(PSTR("SGN:BND:HMAC=%s\n"), printStr);
#endif
}

// Helper to calculate a ATSHA204A specific HMAC-SHA256 using provided 32 byte nonce and data
// (zero padded to 32 bytes)
// The pointer to the HMAC is returned, but the HMAC is also stored in
// _signing_rx_buffer[SHA204_BUFFER_POS_DATA] (=_signing_hmac)
static uint8_t* signerAtsha204AHmac(const uint8_t* nonce, const uint8_t* data)
{
	// Program the data to sign into the ATSHA204
	(void)atsha204_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG, 8 << 3, 32,
	                       (uint8_t*)data,
	                       WRITE_COUNT_LONG, _signing_tx_buffer, WRITE_RSP_SIZE, _signing_rx_buffer);

	// Program the nonce to use for the signature (has to be done just before GENDIG
	// due to chip limitations)
	(void)atsha204_execute(SHA204_NONCE, NONCE_MODE_PASSTHROUGH, 0, 32, (uint8_t*)nonce,
	                       NONCE_COUNT_LONG, _signing_tx_buffer, NONCE_RSP_SIZE_SHORT,
	                       _signing_rx_buffer);

	// Generate digest of data and nonce
	(void)atsha204_execute(SHA204_GENDIG, GENDIG_ZONE_DATA, 8, 0, NULL,
	                       GENDIG_COUNT_DATA, _signing_tx_buffer, GENDIG_RSP_SIZE,
	                       _signing_rx_buffer);

	// Calculate HMAC of message+nonce digest and secret key
	(void)atsha204_execute(SHA204_HMAC, HMAC_MODE_SOURCE_FLAG_MATCH, 0, 0, NULL,
	                       HMAC_COUNT, _signing_tx_buffer, HMAC_RSP_SIZE, _signing_rx_buffer);
	return &_signing_rx_buffer[SHA204_BUFFER_POS_DATA];
}

// Helper to calculate a generic SHA256 digest of provided buffer (only supports one block)
// The pointer to the hash is returned, but the hash is also stored in
// _signing_rx_buffer[SHA204_BUFFER_POS_DATA])
static uint8_t* signerSha256(const uint8_t* data, size_t sz)
{
	// Initiate SHA256 calculator
	(void)atsha204_execute(SHA204_SHA, SHA_INIT, 0, 0, NULL,
	                       SHA_COUNT_SHORT, _signing_tx_buffer, SHA_RSP_SIZE_SHORT,
	                       _signing_rx_buffer);

	// Calculate a hash
	memset(_signing_temp_message, 0x00, SHA_MSG_SIZE);
	memcpy(_signing_temp_message, data, sz);
	_signing_temp_message[sz] = 0x80;
	// Write length data to the last bytes
	_signing_temp_message[SHA_MSG_SIZE-2] = (sz >> 5);
	_signing_temp_message[SHA_MSG_SIZE-1] = (sz << 3);
	(void)atsha204_execute(SHA204_SHA, SHA_CALC, 0, SHA_MSG_SIZE, _signing_temp_message,
	                       SHA_COUNT_LONG, _signing_tx_buffer, SHA_RSP_SIZE_LONG, _signing_rx_buffer);
	return &_signing_rx_buffer[SHA204_BUFFER_POS_DATA];
}
#endif //MY_SIGNING_ATSHA204
