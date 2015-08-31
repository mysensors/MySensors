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
#include "MySigningAtsha204.h"

#define SIGNING_IDENTIFIER (1)

// Uncomment this to get some useful serial debug info (Serial.print and Serial.println expected)
//#define DEBUG_SIGNING

#ifdef DEBUG_SIGNING
#define DEBUG_SIGNING_PRINTLN(args) Serial.println(args)
#else
#define DEBUG_SIGNING_PRINTLN(args)
#endif

#ifdef DEBUG_SIGNING
static void DEBUG_SIGNING_PRINTBUF(const __FlashStringHelper* str, uint8_t* buf, uint8_t sz)
{
	Serial.println(str);
	for (int i=0; i<sz; i++)
	{
		if (buf[i] < 0x10)
		{
			Serial.print('0'); // Because Serial.print does not 0-pad HEX
		}
		Serial.print(buf[i], HEX);
	}
	Serial.println();
}
#else
#define DEBUG_SIGNING_PRINTBUF(str, buf, sz)
#endif


MySigningAtsha204::MySigningAtsha204(bool requestSignatures,
#ifdef MY_SECURE_NODE_WHITELISTING
	uint8_t nof_whitelist_entries, const whitelist_entry_t* the_whitelist,
#endif
	uint8_t atshaPin)
	:
	MySigning(requestSignatures),
	atsha204(atshaPin),
#ifdef MY_SECURE_NODE_WHITELISTING
	whitelist(the_whitelist),
	whitlist_sz(nof_whitelist_entries),
#endif
	verification_ongoing(false)
{
}

bool MySigningAtsha204::getNonce(MyMessage &msg) {
	// Generate random number for use as nonce
	// We used a basic whitening technique that takes the first byte of a new random value and builds up a 32-byte random value
	// This 32-byte random value is then hashed (SHA256) to produce the resulting nonce
	for (int i = 0; i < 32; i++) {
		if (atsha204.sha204m_execute(SHA204_RANDOM, RANDOM_NO_SEED_UPDATE, 0, 0, NULL,
											RANDOM_COUNT, tx_buffer, RANDOM_RSP_SIZE, rx_buffer) != SHA204_SUCCESS) {
			DEBUG_SIGNING_PRINTLN(F("FTGN")); // FTGN = Failed to generate nonce
			return false;
		}
		current_nonce[i] = rx_buffer[SHA204_BUFFER_POS_DATA];
	}
	memcpy(current_nonce, sha256(current_nonce, 32), MAX_PAYLOAD);

	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&current_nonce[MAX_PAYLOAD], 0xAA, sizeof(current_nonce)-MAX_PAYLOAD);

	// Replace the first byte in the nonce with our signing identifier
	current_nonce[0] = SIGNING_IDENTIFIER;

	// Transfer the first part of the nonce to the message
	msg.set(current_nonce, MAX_PAYLOAD);
	verification_ongoing = true;
	timestamp = millis(); // Set timestamp to determine when to purge nonce
	// Be a little fancy to handle turnover (prolong the time allowed to timeout after turnover)
	// Note that if message is "too" quick, and arrives before turnover, it will be rejected
	// but this is consider such a rare case that it is accepted and rejects are 'safe'
	if (timestamp + MY_VERIFICATION_TIMEOUT_MS < millis()) timestamp = 0;
	return true;
}

bool MySigningAtsha204::checkTimer() {
	if (verification_ongoing) {
		if (millis() < timestamp || millis() > timestamp + MY_VERIFICATION_TIMEOUT_MS) {
			DEBUG_SIGNING_PRINTLN(F("VT")); // VT = Verification timeout
			// Purge nonce
			memset(current_nonce, 0x00, NONCE_NUMIN_SIZE_PASSTHROUGH);
			verification_ongoing = false;
			return false; 
		}
	}
	return true;
}

bool MySigningAtsha204::putNonce(MyMessage &msg) {
	if (((uint8_t*)msg.getCustom())[0] != SIGNING_IDENTIFIER) {
		DEBUG_SIGNING_PRINTLN(F("ISI")); // ISI = Incorrect signing identifier
		return false; 
	}

	memcpy(current_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(&current_nonce[MAX_PAYLOAD], 0xAA, sizeof(current_nonce)-MAX_PAYLOAD);
	return true;
}

bool MySigningAtsha204::signMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_SIGNING_PRINTLN(F("MTOL")); // Message too large for signature to fit
		return false; 
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	calculateSignature(msg);

#ifdef MY_SECURE_NODE_WHITELISTING
	// Salt the signature with the senders nodeId and the unique serial of the ATSHA device
	memcpy(current_nonce, &rx_buffer[SHA204_BUFFER_POS_DATA], 32); // We can reuse the nonce buffer now since it is no longer needed
	current_nonce[32] = msg.sender;
	atsha204.getSerialNumber(&current_nonce[33]);
	(void)sha256(current_nonce, 32+1+SHA204_SERIAL_SZ); // we can 'void' sha256 because the hash is already put in the correct place
	DEBUG_SIGNING_PRINTLN(F("SWS")); // SWS = Signature whitelist salted
#endif

	// Overwrite the first byte in the signature with the signing identifier
	rx_buffer[SHA204_BUFFER_POS_DATA] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], &rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool MySigningAtsha204::verifyMsg(MyMessage &msg) {
	if (!verification_ongoing) {
		DEBUG_SIGNING_PRINTLN(F("NAVS")); // NAVS = No active verification session
		return false; 
	} else {
		// Make sure we have not expired
		if (!checkTimer()) {
			return false; 
		}

		verification_ongoing = false;

		if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER) {
			DEBUG_SIGNING_PRINTLN(F("ISI")); // ISI = Incorrect signing identifier
			return false; 
		}

		DEBUG_SIGNING_PRINTBUF(F("SIM:"), (uint8_t*)&msg.data[mGetLength(msg)], MAX_PAYLOAD-mGetLength(msg)); // SIM = Signature in message
		calculateSignature(msg); // Get signature of message

#ifdef MY_SECURE_NODE_WHITELISTING
		// Look up the senders nodeId in our whitelist and salt the signature with that data
		for (int j=0; j < whitlist_sz; j++) {
			if (whitelist[j].nodeId == msg.sender) {
				DEBUG_SIGNING_PRINTLN(F("SIW")); // SIW = Sender found in whitelist
				memcpy(current_nonce, &rx_buffer[SHA204_BUFFER_POS_DATA], 32); // We can reuse the nonce buffer now since it is no longer needed
				current_nonce[32] = msg.sender;
				memcpy(&current_nonce[33], whitelist[j].serial, SHA204_SERIAL_SZ);
				(void)sha256(current_nonce, 32+1+SHA204_SERIAL_SZ); // we can 'void' sha256 because the hash is already put in the correct place
				break;
			}
		}
#endif

		// Overwrite the first byte in the signature with the signing identifier
		rx_buffer[SHA204_BUFFER_POS_DATA] = SIGNING_IDENTIFIER;

		// Compare the caluclated signature with the provided signature
		if (memcmp(&msg.data[mGetLength(msg)], &rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg))) {
			DEBUG_SIGNING_PRINTBUF(F("SNOK:"), &rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg)); // SNOK = Signature bad
#ifdef MY_SECURE_NODE_WHITELISTING
			DEBUG_SIGNING_PRINTLN(F("W?")); // W? = Is the sender whitelisted?
#endif
			return false; 
		} else {
			DEBUG_SIGNING_PRINTLN(F("SOK")); // SOK = Signature OK
			return true;
		}
	}
}

// Helper to calculate signature of msg (returned in rx_buffer[SHA204_BUFFER_POS_DATA])
void MySigningAtsha204::calculateSignature(MyMessage &msg) {
	memset(temp_message, 0, 32);
	memcpy(temp_message, (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));

	// Program the data to sign into the ATSHA204
	DEBUG_SIGNING_PRINTBUF(F("MSG:"), (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg))); // MSG = Message to sign
	DEBUG_SIGNING_PRINTBUF(F("CNC:"), current_nonce, 32); // CNC = Current nonce
	(void)atsha204.sha204m_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG, 8 << 3, 32, temp_message,
									WRITE_COUNT_LONG, tx_buffer, WRITE_RSP_SIZE, rx_buffer);

	// Program the nonce to use for the signature (has to be done just before GENDIG due to chip limitations)
	(void)atsha204.sha204m_execute(SHA204_NONCE, NONCE_MODE_PASSTHROUGH, 0, NONCE_NUMIN_SIZE_PASSTHROUGH, current_nonce,
									NONCE_COUNT_LONG, tx_buffer, NONCE_RSP_SIZE_SHORT, rx_buffer);

	// Purge nonce when used
	memset(current_nonce, 0x00, NONCE_NUMIN_SIZE_PASSTHROUGH);

	// Generate digest of data and nonce
	(void)atsha204.sha204m_execute(SHA204_GENDIG, GENDIG_ZONE_DATA, 8, 0, NULL,
									GENDIG_COUNT_DATA, tx_buffer, GENDIG_RSP_SIZE, rx_buffer);

	// Calculate HMAC of message+nonce digest and secret key
	(void)atsha204.sha204m_execute(SHA204_HMAC, HMAC_MODE_SOURCE_FLAG_MATCH, 0, 0, NULL,
									HMAC_COUNT, tx_buffer, HMAC_RSP_SIZE, rx_buffer);

	// Put device back to sleep
	atsha204.sha204c_sleep();

	DEBUG_SIGNING_PRINTBUF(F("HMAC:"), &rx_buffer[SHA204_BUFFER_POS_DATA], 32);
}

// Helper to calculate a generic SHA256 digest of provided buffer (only supports one block)
// The pointer to the hash is returned, but the hash is also stored in rx_buffer[SHA204_BUFFER_POS_DATA]) 
uint8_t* MySigningAtsha204::sha256(const uint8_t* data, size_t sz) {
	// Initiate SHA256 calculator
	(void)atsha204.sha204m_execute(SHA204_SHA, SHA_INIT, 0, 0, NULL,
									SHA_COUNT_SHORT, tx_buffer, SHA_RSP_SIZE_SHORT, rx_buffer);

	// Calculate a hash
	memset(temp_message, 0x00, SHA_MSG_SIZE);
	memcpy(temp_message, data, sz);
	temp_message[sz] = 0x80;
	// Write length data to the last bytes
	temp_message[SHA_MSG_SIZE-2] = (sz >> 5);
	temp_message[SHA_MSG_SIZE-1] = (sz << 3);
	DEBUG_SIGNING_PRINTBUF(F("DTH:"), temp_message, SHA_MSG_SIZE); // DTH = Data to hash
	(void)atsha204.sha204m_execute(SHA204_SHA, SHA_CALC, 0, SHA_MSG_SIZE, temp_message,
									SHA_COUNT_LONG, tx_buffer, SHA_RSP_SIZE_LONG, rx_buffer);

	// Put device back to sleep
	atsha204.sha204c_sleep();

	DEBUG_SIGNING_PRINTBUF(F("SHA:"), &rx_buffer[SHA204_BUFFER_POS_DATA], 32);
	return &rx_buffer[SHA204_BUFFER_POS_DATA];
}
#endif // #if defined(ARDUINO_ARCH_AVR)
