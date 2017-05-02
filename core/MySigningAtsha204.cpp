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
 * ATSHA204 signing backend. The Atmel ATSHA204 offers true random number generation and
 * HMAC-SHA256 authentication with a readout-protected key.
 *
 */

#include "MySigning.h"

#define SIGNING_IDENTIFIER (1) //HMAC-SHA256

#if defined(MY_DEBUG_VERBOSE_SIGNING)
#define SIGN_DEBUG(x,...) hwDebugPrint(x, ##__VA_ARGS__)
#else
#define SIGN_DEBUG(x,...)
#endif

// Define MY_DEBUG_VERBOSE_SIGNING in your sketch to enable signing backend debugprints

unsigned long _signing_timestamp;
bool _signing_verification_ongoing = false;
uint8_t _signing_verifying_nonce[NONCE_NUMIN_SIZE_PASSTHROUGH+SHA204_SERIAL_SZ+1];
uint8_t _signing_signing_nonce[NONCE_NUMIN_SIZE_PASSTHROUGH+SHA204_SERIAL_SZ+1];
uint8_t _signing_temp_message[SHA_MSG_SIZE];
uint8_t _signing_rx_buffer[SHA204_RSP_SIZE_MAX];
uint8_t _signing_tx_buffer[SHA204_CMD_SIZE_MAX];
extern uint8_t _doWhitelist[32];

#ifdef MY_SIGNING_NODE_WHITELISTING
const whitelist_entry_t _signing_whitelist[] = MY_SIGNING_NODE_WHITELISTING;
#endif

static void signerCalculateSignature(MyMessage &msg, bool signing);
static uint8_t* signerSha256(const uint8_t* data, size_t sz);

static bool init_ok = false;

bool signerAtsha204Init(void)
{
	init_ok = true;
	atsha204_init(MY_SIGNING_ATSHA204_PIN);

	(void)atsha204_wakeup(_signing_temp_message);
	// Read the configuration lock flag to determine if device is personalized or not
	if (atsha204_read(_signing_tx_buffer, _signing_rx_buffer,
	                  SHA204_ZONE_CONFIG, 0x15<<2) != SHA204_SUCCESS) {
		SIGN_DEBUG(PSTR("Could not read ATSHA204A lock config, refusing to use backend\n"));
		init_ok = false;
	} else if (_signing_rx_buffer[SHA204_BUFFER_POS_DATA+3] != 0x00) {
		SIGN_DEBUG(PSTR("ATSHA204A not personalized, refusing to use backend\n"));
		init_ok = false;
	}
	return init_ok;
}

bool signerAtsha204CheckTimer(void)
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
			memset(_signing_signing_nonce, 0x00, NONCE_NUMIN_SIZE_PASSTHROUGH);
			memset(_signing_verifying_nonce, 0x00, NONCE_NUMIN_SIZE_PASSTHROUGH);
			_signing_verification_ongoing = false;
			return false;
		}
	}
	return true;
}

bool signerAtsha204GetNonce(MyMessage &msg)
{
	if (!init_ok) {
		SIGN_DEBUG(PSTR("Backend not initialized\n"));
		return false;
	}
	SIGN_DEBUG(PSTR("Signing backend: ATSHA204\n"));
	// Generate random number for use as nonce
	// We used a basic whitening technique that XORs each byte in a 32byte random value with current hwMillis() counter
	// This 32-byte random value is then hashed (SHA256) to produce the resulting nonce
	(void)atsha204_wakeup(_signing_temp_message);
	if (atsha204_execute(SHA204_RANDOM, RANDOM_SEED_UPDATE, 0, 0, NULL,
	                     RANDOM_COUNT, _signing_tx_buffer, RANDOM_RSP_SIZE, _signing_rx_buffer) != SHA204_SUCCESS) {
		SIGN_DEBUG(PSTR("Failed to generate nonce\n"));
		return false;
	}
	for (int i = 0; i < 32; i++) {
		_signing_verifying_nonce[i] = _signing_rx_buffer[SHA204_BUFFER_POS_DATA+i] ^ (hwMillis()&0xFF);
	}
	memcpy(_signing_verifying_nonce, signerSha256(_signing_verifying_nonce, 32), MAX_PAYLOAD);

	atsha204_idle(); // We just idle the chip now since we expect to use it soon when the signed message arrives

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

void signerAtsha204PutNonce(MyMessage &msg)
{
	if (!init_ok) {
		SIGN_DEBUG(PSTR("Backend not initialized\n"));
		return;
	}
	SIGN_DEBUG(PSTR("Signing backend: ATSHA204\n"));

	memcpy(_signing_signing_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&_signing_signing_nonce[MAX_PAYLOAD], 0xAA, sizeof(_signing_signing_nonce)-MAX_PAYLOAD);
}

bool signerAtsha204SignMsg(MyMessage &msg)
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
		// Salt the signature with the senders nodeId and the unique serial of the ATSHA device
		memcpy(_signing_signing_nonce, &_signing_rx_buffer[SHA204_BUFFER_POS_DATA],
		       32); // We can reuse the nonce buffer now since it is no longer needed
		_signing_signing_nonce[32] = msg.sender;
		atsha204_getSerialNumber(&_signing_signing_nonce[33]);
		(void)signerSha256(_signing_signing_nonce,
		                   32+1+SHA204_SERIAL_SZ); // we can 'void' sha256 because the hash is already put in the correct place
		SIGN_DEBUG(PSTR("Signature salted with serial: %02X%02X%02X%02X%02X%02X%02X%02X%02X\n"),
		           _signing_signing_nonce[33], _signing_signing_nonce[34], _signing_signing_nonce[35],
		           _signing_signing_nonce[36], _signing_signing_nonce[37], _signing_signing_nonce[38],
		           _signing_signing_nonce[39], _signing_signing_nonce[40], _signing_signing_nonce[41]);
	}

	// Put device back to sleep
	atsha204_sleep();

	// Overwrite the first byte in the signature with the signing identifier
	_signing_rx_buffer[SHA204_BUFFER_POS_DATA] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], &_signing_rx_buffer[SHA204_BUFFER_POS_DATA],
	       MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool signerAtsha204VerifyMsg(MyMessage &msg)
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
				memcpy(_signing_verifying_nonce, &_signing_rx_buffer[SHA204_BUFFER_POS_DATA],
				       32); // We can reuse the nonce buffer now since it is no longer needed
				_signing_verifying_nonce[32] = msg.sender;
				memcpy(&_signing_verifying_nonce[33], _signing_whitelist[j].serial, SHA204_SERIAL_SZ);
				(void)signerSha256(_signing_verifying_nonce,
				                   32+1+SHA204_SERIAL_SZ); // we can 'void' sha256 because the hash is already put in the correct place
				break;
			}
		}
		if (j == NUM_OF(_signing_whitelist)) {
			SIGN_DEBUG(PSTR("Sender not found in whitelist, message rejected!\n"));
			// Put device back to sleep
			atsha204_sleep();
			return false;
		}
#endif

		// Put device back to sleep
		atsha204_sleep();

		// Overwrite the first byte in the signature with the signing identifier
		_signing_rx_buffer[SHA204_BUFFER_POS_DATA] = SIGNING_IDENTIFIER;

		// Compare the caluclated signature with the provided signature
		if (signerMemcmp(&msg.data[mGetLength(msg)], &_signing_rx_buffer[SHA204_BUFFER_POS_DATA],
		                 MAX_PAYLOAD-mGetLength(msg))) {
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

// Helper to calculate signature of msg (returned in _signing_rx_buffer[SHA204_BUFFER_POS_DATA])
static void signerCalculateSignature(MyMessage &msg, bool signing)
{
	(void)atsha204_wakeup(_signing_temp_message);
	memset(_signing_temp_message, 0, 32);
	memcpy(_signing_temp_message, (uint8_t*)&msg.data[1-HEADER_SIZE],
	       MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));

	// Program the data to sign into the ATSHA204
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
	(void)atsha204_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG, 8 << 3, 32,
	                       _signing_temp_message,
	                       WRITE_COUNT_LONG, _signing_tx_buffer, WRITE_RSP_SIZE, _signing_rx_buffer);

	// Program the nonce to use for the signature (has to be done just before GENDIG due to chip limitations)
	(void)atsha204_execute(SHA204_NONCE, NONCE_MODE_PASSTHROUGH, 0, NONCE_NUMIN_SIZE_PASSTHROUGH,
	                       signing ? _signing_signing_nonce : _signing_verifying_nonce,
	                       NONCE_COUNT_LONG, _signing_tx_buffer, NONCE_RSP_SIZE_SHORT, _signing_rx_buffer);

	// Purge nonce when used
	memset(signing ? _signing_signing_nonce : _signing_verifying_nonce, 0x00,
	       NONCE_NUMIN_SIZE_PASSTHROUGH);

	// Generate digest of data and nonce
	(void)atsha204_execute(SHA204_GENDIG, GENDIG_ZONE_DATA, 8, 0, NULL,
	                       GENDIG_COUNT_DATA, _signing_tx_buffer, GENDIG_RSP_SIZE, _signing_rx_buffer);

	// Calculate HMAC of message+nonce digest and secret key
	(void)atsha204_execute(SHA204_HMAC, HMAC_MODE_SOURCE_FLAG_MATCH, 0, 0, NULL,
	                       HMAC_COUNT, _signing_tx_buffer, HMAC_RSP_SIZE, _signing_rx_buffer);

	SIGN_DEBUG(PSTR("HMAC: "
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n"),
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+0],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+1],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+2],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+3],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+4],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+5],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+6],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+7],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+8],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+9],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+10],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+11],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+12],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+13],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+14],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+15],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+16],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+17],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+18],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+19],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+20],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+21],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+22],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+23],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+24],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+25],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+26],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+27],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+28],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+29],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+30],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+31]
	          );
}

// Helper to calculate a generic SHA256 digest of provided buffer (only supports one block)
// The pointer to the hash is returned, but the hash is also stored in _signing_rx_buffer[SHA204_BUFFER_POS_DATA])
static uint8_t* signerSha256(const uint8_t* data, size_t sz)
{
	// Initiate SHA256 calculator
	(void)atsha204_execute(SHA204_SHA, SHA_INIT, 0, 0, NULL,
	                       SHA_COUNT_SHORT, _signing_tx_buffer, SHA_RSP_SIZE_SHORT, _signing_rx_buffer);

	// Calculate a hash
	memset(_signing_temp_message, 0x00, SHA_MSG_SIZE);
	memcpy(_signing_temp_message, data, sz);
	_signing_temp_message[sz] = 0x80;
	// Write length data to the last bytes
	_signing_temp_message[SHA_MSG_SIZE-2] = (sz >> 5);
	_signing_temp_message[SHA_MSG_SIZE-1] = (sz << 3);
	(void)atsha204_execute(SHA204_SHA, SHA_CALC, 0, SHA_MSG_SIZE, _signing_temp_message,
	                       SHA_COUNT_LONG, _signing_tx_buffer, SHA_RSP_SIZE_LONG, _signing_rx_buffer);

	SIGN_DEBUG(PSTR("SHA256: "
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
	                "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n"),
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+0],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+1],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+2],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+3],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+4],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+5],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+6],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+7],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+8],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+9],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+10],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+11],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+12],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+13],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+14],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+15],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+16],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+17],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+18],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+19],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+20],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+21],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+22],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+23],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+24],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+25],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+26],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+27],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+28],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+29],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+30],
	           _signing_rx_buffer[SHA204_BUFFER_POS_DATA+31]
	          );
	return &_signing_rx_buffer[SHA204_BUFFER_POS_DATA];
}
