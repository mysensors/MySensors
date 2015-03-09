/*
 ATSHA204 emulated signing backend. The emulated ATSHA204 implementation offers pseudo random
 number generation and HMAC-SHA256 authentication compatible with a "physical" ATSHA204.
 NOTE: Key is stored in clear text in the Arduino firmware. Therefore, the use of this back-end
 could compromise the key used in the signed message infrastructure if device is lost and its memory
 dumped.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#include "MyConfig.h"
#include "MySigning.h"
#include "MySigningAtsha204Soft.h"
#include "utility/sha256.h"

// Uncomment this to get some useful serial debug info (Serial.print and Serial.println expected)
//#define DEBUG_ATSHASOFT_SIGNING

#ifdef DEBUG_ATSHASOFT_SIGNING
#define DEBUG_ATSHASOFT_PRINTLN(args) Serial.println(args)
#else
#define DEBUG_ATSHASOFT_PRINTLN(args)
#endif

#ifdef DEBUG_ATSHASOFT_SIGNING
static void DEBUG_ATSHASOFT_PRINTBUF(char* str, uint8_t* buf, uint8_t sz)
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
#define DEBUG_ATSHASOFT_PRINTBUF(str, buf, sz)
#endif

MySigningAtsha204Soft::MySigningAtsha204Soft(bool requestSignatures, uint8_t randomseedPin)
	:
	MySigning(requestSignatures),
	rndPin(randomseedPin),
	hmacKey({MY_HMAC_KEY}),
	verification_ongoing(false),
	Sha256()

{
	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(current_nonce, 0xAA, sizeof(current_nonce));
}

bool MySigningAtsha204Soft::getNonce(MyMessage &msg) {
	// Set randomseed
	randomSeed(analogRead(rndPin));
	for (int i = 0; i < MAX_PAYLOAD; i++)
	{
		current_nonce[i] = random(255);
	}
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
			DEBUG_ATSHASOFT_PRINTLN("Verification timeout");
			// Purge nonce
			memset(current_nonce, 0xAA, 32);
			verification_ongoing = false;
			return false; 
		}
	}
	return true;
}

bool MySigningAtsha204Soft::putNonce(MyMessage &msg) {
	if (mGetLength(msg) != MAX_PAYLOAD) {
		DEBUG_ATSHASOFT_PRINTLN("Incoming nonce with incorrect size");
		return false; // We require as big nonce as possible
	}

	if (((uint8_t*)msg.getCustom())[0] != SIGNING_IDENTIFIER) {
		DEBUG_ATSHASOFT_PRINTLN("Incorrect signing identifier");
		return false; 
	}

	memcpy(current_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	return true;
}

bool MySigningAtsha204Soft::signMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_ATSHASOFT_PRINTLN("Cannot fit any signature to this message");
		return false; 
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	DEBUG_ATSHASOFT_PRINTBUF("Message to sign:", (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
	if (!calculateSignature(msg)) {
		DEBUG_ATSHASOFT_PRINTLN("Failed to calculate signature");
		mSetSigned(msg, 0); // make sure signing flag is truthful
		return false;
	}

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], hmac, MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool MySigningAtsha204Soft::verifyMsg(MyMessage &msg) {
	if (!verification_ongoing) {
		DEBUG_ATSHASOFT_PRINTLN("No active verification session");
		return false; 
	} else {
		// Make sure we have not expired
		if (!checkTimer()) {
			return false; 
		}

		verification_ongoing = false;

		if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER) {
			DEBUG_ATSHASOFT_PRINTLN("Incorrect signing identifier");
			return false; 
		}

		DEBUG_ATSHASOFT_PRINTBUF("Message to verify:", (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
		DEBUG_ATSHASOFT_PRINTBUF("Signature in message:", (uint8_t*)&msg.data[mGetLength(msg)], MAX_PAYLOAD-mGetLength(msg));

		// Get signature of message
		if (!calculateSignature(msg)) {
			DEBUG_ATSHASOFT_PRINTLN("Failed to calculate signature");
			return false; 
		}

		// Compare the caluclated signature with the provided signature
		if (memcmp(&msg.data[mGetLength(msg)], hmac, MAX_PAYLOAD-mGetLength(msg))) {
			DEBUG_ATSHASOFT_PRINTBUF("Signature bad. Calculated signature:", hmac, MAX_PAYLOAD-mGetLength(msg));
			return false; 
		} else {
			DEBUG_ATSHASOFT_PRINTLN("Signature ok");
			return true;
		}
	}
}

// Helper to calculate signature of msg (returned in rx_buffer[SHA204_BUFFER_POS_DATA])
bool MySigningAtsha204Soft::calculateSignature(MyMessage &msg) {
	memset(temp_message, 0, 32);
	memcpy(temp_message, (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));

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

	hmac = Sha256.resultHmac();

	// Overwrite the first byte in the signature with the signing identifier
	hmac[0] = SIGNING_IDENTIFIER;

	DEBUG_ATSHASOFT_PRINTBUF("HMAC:", hmac, 32);
	return true;
}
