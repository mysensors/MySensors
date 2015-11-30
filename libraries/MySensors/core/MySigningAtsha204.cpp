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
 * ATSHA204 signing backend. The Atmel ATSHA204 offers true random number generation and
 * HMAC-SHA256 authentication with a readout-protected key.
 *
 */
#if defined(ARDUINO_ARCH_AVR)

#include "MySigning.h"

#define SIGNING_IDENTIFIER (1)

// Define MY_DEBUG in your sketch to enable signing backend debugprints

ATSHA204Class atsha204(MY_SIGNING_ATSHA204_PIN);
unsigned long _signing_timestamp;
bool _signing_verification_ongoing = false;
uint8_t _signing_current_nonce[NONCE_NUMIN_SIZE_PASSTHROUGH+SHA204_SERIAL_SZ+1];
uint8_t _signing_temp_message[SHA_MSG_SIZE];
uint8_t _singning_rx_buffer[SHA204_RSP_SIZE_MAX];
uint8_t _singning_tx_buffer[SHA204_CMD_SIZE_MAX];

#ifdef MY_SIGNING_NODE_WHITELISTING
	const whitelist_entry_t _signing_whitelist[] = MY_SIGNING_NODE_WHITELISTING;
#endif

void signerCalculateSignature(MyMessage &msg);
uint8_t* signerSha256(const uint8_t* data, size_t sz);


#ifdef MY_DEBUG
static char i2h(uint8_t i)
{
	uint8_t k = i & 0x0F;
	if (k <= 9)
		return '0' + k;
	else
		return 'A' + k - 10;
}

static void DEBUG_SIGNING_PRINTBUF(const __FlashStringHelper* str, uint8_t* buf, uint8_t sz)
{
	static char printBuffer[300];
#ifdef MY_GATEWAY_FEATURE
	// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
	snprintf_P(printBuffer, 299, PSTR("0;0;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
	Serial.print(printBuffer);
#endif
	for (int i=0; i<sz; i++)
	{
		printBuffer[i * 2] = i2h(buf[i] >> 4);
		printBuffer[(i * 2) + 1] = i2h(buf[i]);
	}
	printBuffer[sz * 2] = '\0';
#ifdef MY_GATEWAY_FEATURE
	// Truncate message if this is gateway node
	printBuffer[MY_GATEWAY_MAX_SEND_LENGTH-1-strlen_P((const char*)str)] = '\0';
#endif
	Serial.print(str);
	if (sz > 0)
	{
		Serial.print(printBuffer);
	}
	Serial.println("");
}
#else
#define DEBUG_SIGNING_PRINTBUF(str, buf, sz)
#endif

bool signerGetNonce(MyMessage &msg) {
	DEBUG_SIGNING_PRINTBUF(F("Signing backend: ATSHA204"), NULL, 0);
	// Generate random number for use as nonce
	// We used a basic whitening technique that takes the first byte of a new random value and builds up a 32-byte random value
	// This 32-byte random value is then hashed (SHA256) to produce the resulting nonce
	for (int i = 0; i < 32; i++) {
		if (atsha204.sha204m_execute(SHA204_RANDOM, RANDOM_NO_SEED_UPDATE, 0, 0, NULL,
											RANDOM_COUNT, _singning_tx_buffer, RANDOM_RSP_SIZE, _singning_rx_buffer) != SHA204_SUCCESS) {
			DEBUG_SIGNING_PRINTBUF(F("Failed to generate nonce"), NULL, 0);
			return false;
		}
		_signing_current_nonce[i] = _singning_rx_buffer[SHA204_BUFFER_POS_DATA];
	}
	memcpy(_signing_current_nonce, signerSha256(_signing_current_nonce, 32), MAX_PAYLOAD);

	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&_signing_current_nonce[MAX_PAYLOAD], 0xAA, sizeof(_signing_current_nonce)-MAX_PAYLOAD);

	// Replace the first byte in the nonce with our signing identifier
	_signing_current_nonce[0] = SIGNING_IDENTIFIER;

	// Transfer the first part of the nonce to the message
	msg.set(_signing_current_nonce, MAX_PAYLOAD);
	_signing_verification_ongoing = true;
	_signing_timestamp = millis(); // Set timestamp to determine when to purge nonce
	// Be a little fancy to handle turnover (prolong the time allowed to timeout after turnover)
	// Note that if message is "too" quick, and arrives before turnover, it will be rejected
	// but this is consider such a rare case that it is accepted and rejects are 'safe'
	if (_signing_timestamp + MY_VERIFICATION_TIMEOUT_MS < millis()) _signing_timestamp = 0;
	return true;
}

bool signerCheckTimer() {
	if (_signing_verification_ongoing) {
		if (millis() < _signing_timestamp || millis() > _signing_timestamp + MY_VERIFICATION_TIMEOUT_MS) {
			DEBUG_SIGNING_PRINTBUF(F("Verification timeout"), NULL, 0);
			// Purge nonce
			memset(_signing_current_nonce, 0x00, NONCE_NUMIN_SIZE_PASSTHROUGH);
			_signing_verification_ongoing = false;
			return false; 
		}
	}
	return true;
}

bool signerPutNonce(MyMessage &msg) {
	DEBUG_SIGNING_PRINTBUF(F("Signing backend: ATSHA204"), NULL, 0);
	if (((uint8_t*)msg.getCustom())[0] != SIGNING_IDENTIFIER) {
		DEBUG_SIGNING_PRINTBUF(F("Incorrect signing identifier"), NULL, 0);
		return false; 
	}

	memcpy(_signing_current_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&_signing_current_nonce[MAX_PAYLOAD], 0xAA, sizeof(_signing_current_nonce)-MAX_PAYLOAD);
	return true;
}

bool signerSignMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_SIGNING_PRINTBUF(F("Message too large"), NULL, 0);
		return false; 
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	signerCalculateSignature(msg);

#ifdef MY_SIGNING_NODE_WHITELISTING
	// Salt the signature with the senders nodeId and the unique serial of the ATSHA device
	memcpy(_signing_current_nonce, &_singning_rx_buffer[SHA204_BUFFER_POS_DATA], 32); // We can reuse the nonce buffer now since it is no longer needed
	_signing_current_nonce[32] = msg.sender;
	atsha204.getSerialNumber(&_signing_current_nonce[33]);
	(void)signerSha256(_signing_current_nonce, 32+1+SHA204_SERIAL_SZ); // we can 'void' sha256 because the hash is already put in the correct place
	DEBUG_SIGNING_PRINTBUF(F("Signature salted with serial"), NULL, 0);
#endif

	// Overwrite the first byte in the signature with the signing identifier
	_singning_rx_buffer[SHA204_BUFFER_POS_DATA] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], &_singning_rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg));
	DEBUG_SIGNING_PRINTBUF(F("Signature in message: "), (uint8_t*)&msg.data[mGetLength(msg)], MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool signerVerifyMsg(MyMessage &msg) {
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

		DEBUG_SIGNING_PRINTBUF(F("Signature in message: "), (uint8_t*)&msg.data[mGetLength(msg)], MAX_PAYLOAD-mGetLength(msg));
		signerCalculateSignature(msg); // Get signature of message

#ifdef MY_SIGNING_NODE_WHITELISTING
		// Look up the senders nodeId in our whitelist and salt the signature with that data
		for (int j=0; j < NUM_OF(_signing_whitelist); j++) {
			if (_signing_whitelist[j].nodeId == msg.sender) {
				DEBUG_SIGNING_PRINTBUF(F("Sender found in whitelist"), NULL, 0);
				memcpy(_signing_current_nonce, &_singning_rx_buffer[SHA204_BUFFER_POS_DATA], 32); // We can reuse the nonce buffer now since it is no longer needed
				_signing_current_nonce[32] = msg.sender;
				memcpy(&_signing_current_nonce[33], _signing_whitelist[j].serial, SHA204_SERIAL_SZ);
				(void)signerSha256(_signing_current_nonce, 32+1+SHA204_SERIAL_SZ); // we can 'void' sha256 because the hash is already put in the correct place
				break;
			}
		}
#endif

		// Overwrite the first byte in the signature with the signing identifier
		_singning_rx_buffer[SHA204_BUFFER_POS_DATA] = SIGNING_IDENTIFIER;

		// Compare the caluclated signature with the provided signature
		if (memcmp(&msg.data[mGetLength(msg)], &_singning_rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg))) {
			DEBUG_SIGNING_PRINTBUF(F("Signature bad: "), &_singning_rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg));
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

// Helper to calculate signature of msg (returned in _singning_rx_buffer[SHA204_BUFFER_POS_DATA])
void signerCalculateSignature(MyMessage &msg) {
	memset(_signing_temp_message, 0, 32);
	memcpy(_signing_temp_message, (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));

	// Program the data to sign into the ATSHA204
	DEBUG_SIGNING_PRINTBUF(F("Message to process: "), (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
	DEBUG_SIGNING_PRINTBUF(F("Current nonce: "), _signing_current_nonce, 32);
	(void)atsha204.sha204m_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG, 8 << 3, 32, _signing_temp_message,
									WRITE_COUNT_LONG, _singning_tx_buffer, WRITE_RSP_SIZE, _singning_rx_buffer);

	// Program the nonce to use for the signature (has to be done just before GENDIG due to chip limitations)
	(void)atsha204.sha204m_execute(SHA204_NONCE, NONCE_MODE_PASSTHROUGH, 0, NONCE_NUMIN_SIZE_PASSTHROUGH, _signing_current_nonce,
									NONCE_COUNT_LONG, _singning_tx_buffer, NONCE_RSP_SIZE_SHORT, _singning_rx_buffer);

	// Purge nonce when used
	memset(_signing_current_nonce, 0x00, NONCE_NUMIN_SIZE_PASSTHROUGH);

	// Generate digest of data and nonce
	(void)atsha204.sha204m_execute(SHA204_GENDIG, GENDIG_ZONE_DATA, 8, 0, NULL,
									GENDIG_COUNT_DATA, _singning_tx_buffer, GENDIG_RSP_SIZE, _singning_rx_buffer);

	// Calculate HMAC of message+nonce digest and secret key
	(void)atsha204.sha204m_execute(SHA204_HMAC, HMAC_MODE_SOURCE_FLAG_MATCH, 0, 0, NULL,
									HMAC_COUNT, _singning_tx_buffer, HMAC_RSP_SIZE, _singning_rx_buffer);

	// Put device back to sleep
	atsha204.sha204c_sleep();

	DEBUG_SIGNING_PRINTBUF(F("HMAC: "), &_singning_rx_buffer[SHA204_BUFFER_POS_DATA], 32);
}

// Helper to calculate a generic SHA256 digest of provided buffer (only supports one block)
// The pointer to the hash is returned, but the hash is also stored in _singning_rx_buffer[SHA204_BUFFER_POS_DATA])
uint8_t* signerSha256(const uint8_t* data, size_t sz) {
	// Initiate SHA256 calculator
	(void)atsha204.sha204m_execute(SHA204_SHA, SHA_INIT, 0, 0, NULL,
									SHA_COUNT_SHORT, _singning_tx_buffer, SHA_RSP_SIZE_SHORT, _singning_rx_buffer);

	// Calculate a hash
	memset(_signing_temp_message, 0x00, SHA_MSG_SIZE);
	memcpy(_signing_temp_message, data, sz);
	_signing_temp_message[sz] = 0x80;
	// Write length data to the last bytes
	_signing_temp_message[SHA_MSG_SIZE-2] = (sz >> 5);
	_signing_temp_message[SHA_MSG_SIZE-1] = (sz << 3);
	(void)atsha204.sha204m_execute(SHA204_SHA, SHA_CALC, 0, SHA_MSG_SIZE, _signing_temp_message,
									SHA_COUNT_LONG, _singning_tx_buffer, SHA_RSP_SIZE_LONG, _singning_rx_buffer);

	// Put device back to sleep
	atsha204.sha204c_sleep();

	DEBUG_SIGNING_PRINTBUF(F("SHA256: "), &_singning_rx_buffer[SHA204_BUFFER_POS_DATA], 32);
	return &_singning_rx_buffer[SHA204_BUFFER_POS_DATA];
}
#endif // #if defined(ARDUINO_ARCH_AVR)
