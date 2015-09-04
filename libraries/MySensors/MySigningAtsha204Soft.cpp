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

#include "MyConfig.h"
#include "MySigning.h"
#include "MySigningAtsha204Soft.h"
#include "utility/sha256.h"

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
	int i;
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

// Initialize hmacKey from MyConfig.h (codebender didn't like static initialization in constructor)
uint8_t MySigningAtsha204Soft::hmacKey[32] = { MY_HMAC_KEY };

MySigningAtsha204Soft::MySigningAtsha204Soft(bool requestSignatures,
#ifdef MY_SECURE_NODE_WHITELISTING
	uint8_t nof_whitelist_entries, const whitelist_entry_t* the_whitelist,
	const uint8_t* the_serial,
#endif
	uint8_t randomseedPin)
	:
	MySigning(requestSignatures),
#ifdef MY_SECURE_NODE_WHITELISTING
	whitlist_sz(nof_whitelist_entries),
	whitelist(the_whitelist),
	node_serial_info(the_serial),
#endif
	Sha256(),
	verification_ongoing(false),
	rndPin(randomseedPin)
{
}

bool MySigningAtsha204Soft::getNonce(MyMessage &msg) {
	// Set randomseed
	randomSeed(analogRead(rndPin));

	// We used a basic whitening technique that takes the first byte of a new random value and builds up a 32-byte random value
	// This 32-byte random value is then hashed (SHA256) to produce the resulting nonce
	Sha256.init();
	for (int i = 0; i < 32; i++) {
		Sha256.write(random(255));
	}
	memcpy(current_nonce, Sha256.result(), MAX_PAYLOAD);

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

bool MySigningAtsha204Soft::checkTimer() {
	if (verification_ongoing) {
		if (millis() < timestamp || millis() > timestamp + MY_VERIFICATION_TIMEOUT_MS) {
			DEBUG_SIGNING_PRINTLN(F("VT")); // VT = Verification timeout
			// Purge nonce
			memset(current_nonce, 0xAA, 32);
			verification_ongoing = false;
			return false; 
		}
	}
	return true;
}

bool MySigningAtsha204Soft::putNonce(MyMessage &msg) {
	if (((uint8_t*)msg.getCustom())[0] != SIGNING_IDENTIFIER) {
		DEBUG_SIGNING_PRINTLN(F("ISI")); // ISI = Incorrect signing identifier
		return false; 
	}

	memcpy(current_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	return true;
}

bool MySigningAtsha204Soft::signMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_SIGNING_PRINTLN(F("MTOL")); // Message too large for signature to fit
		return false; 
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	calculateSignature(msg);

#ifdef MY_SECURE_NODE_WHITELISTING
	// Salt the signature with the senders nodeId and the (hopefully) unique serial The Creator has provided
	Sha256.init();
	for (int i=0; i<32; i++) Sha256.write(hmac[i]);
	Sha256.write(msg.sender);
	for (int i=0; i<SHA204_SERIAL_SZ; i++) Sha256.write(node_serial_info[i]);
	memcpy(hmac, Sha256.result(), 32);
	DEBUG_SIGNING_PRINTLN(F("SWS")); // SWS = Signature whitelist salted
#endif

	// Overwrite the first byte in the signature with the signing identifier
	hmac[0] = SIGNING_IDENTIFIER;

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], hmac, MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool MySigningAtsha204Soft::verifyMsg(MyMessage &msg) {
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

		// Get signature of message
		DEBUG_SIGNING_PRINTBUF(F("SIM:"), (uint8_t*)&msg.data[mGetLength(msg)], MAX_PAYLOAD-mGetLength(msg)); // SIM = Signature in message
		calculateSignature(msg);

#ifdef MY_SECURE_NODE_WHITELISTING
		// Look up the senders nodeId in our whitelist and salt the signature with that data
		for (int j=0; j < whitlist_sz; j++) {
			if (whitelist[j].nodeId == msg.sender) {
				DEBUG_SIGNING_PRINTLN(F("SIW")); // SIW = Sender found in whitelist
				Sha256.init();
				for (int i=0; i<32; i++) Sha256.write(hmac[i]);
				Sha256.write(msg.sender);
				for (int i=0; i<SHA204_SERIAL_SZ; i++) Sha256.write(whitelist[j].serial[i]);
				memcpy(hmac, Sha256.result(), 32);
				break;
			}
		}
#endif

		// Overwrite the first byte in the signature with the signing identifier
		hmac[0] = SIGNING_IDENTIFIER;

		// Compare the caluclated signature with the provided signature
		if (memcmp(&msg.data[mGetLength(msg)], hmac, MAX_PAYLOAD-mGetLength(msg))) {
			DEBUG_SIGNING_PRINTBUF(F("SNOK:"), hmac, MAX_PAYLOAD-mGetLength(msg)); // SNOK = Signature bad
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

// Helper to calculate signature of msg (returned in hmac)
void MySigningAtsha204Soft::calculateSignature(MyMessage &msg) {
	memset(temp_message, 0, 32);
	memcpy(temp_message, (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
	DEBUG_SIGNING_PRINTBUF(F("MSG:"), (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg))); // MSG = Message to sign
	DEBUG_SIGNING_PRINTBUF(F("CNC:"), current_nonce, 32); // CNC = Current nonce

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
	Sha256.init();
	for (int i=0; i<32; i++) Sha256.write(temp_message[i]);
	Sha256.write(0x15); // OPCODE
	Sha256.write(0x02); // param1
	Sha256.write(0x08); // param2(1)
	Sha256.write(0x00); // param2(2)
	Sha256.write(0xEE); // SN[8]
	Sha256.write(0x01); // SN[0]
	Sha256.write(0x23); // SN[1]
	for (int i=0; i<25; i++) Sha256.write(0x00);
	for (int i=0; i<32; i++) Sha256.write(current_nonce[i]);
	// Purge nonce when used
	memset(current_nonce, 0xAA, 32);
	memcpy(temp_message, Sha256.result(), 32);

	// Feed "message" to HMAC calculator
	Sha256.initHmac(hmacKey,32); // Set the key to use
	for (int i=0; i<32; i++) Sha256.write(0x00); // 32 bytes zeroes
	for (int i=0; i<32; i++) Sha256.write(temp_message[i]); // 32 bytes digest
	Sha256.write(0x11); // OPCODE
	Sha256.write(0x04); // Mode
	Sha256.write(0x00); // SlotID(1)
	Sha256.write(0x00); // SlotID(2)
	for (int i=0; i<11; i++) Sha256.write(0x00); // 11 bytes zeroes
	Sha256.write(0xEE); // SN[8]
	for (int i=0; i<4; i++) Sha256.write(0x00); // 4 bytes zeroes
	Sha256.write(0x01); // SN[0]
	Sha256.write(0x23); // SN[1]
	for (int i=0; i<2; i++) Sha256.write(0x00); // 2 bytes zeroes

	memcpy(hmac, Sha256.resultHmac(), 32);

	DEBUG_SIGNING_PRINTBUF(F("HMAC:"), hmac, 32);
}
