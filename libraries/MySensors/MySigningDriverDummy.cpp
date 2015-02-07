/*
 Dummy signing backend. Does not provide any security measures.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#include "MySigningDriver.h"
#include "MySigningDriverDummy.h"

#ifdef MYSENSORS_SIGNING_DUMMY
// Uncomment this to get some useful serial debug info (Serial.print and Serial.println expected)
//#define DEBUG_DUMMY_SIGNING

#ifdef DEBUG_DUMMY_SIGNING
#define DEBUG_DUMMY_PRINTLN(args) Serial.println(args)
#else
#define DEBUG_DUMMY_PRINTLN(args)
#endif

#ifdef DEBUG_DUMMY_SIGNING
static void DEBUG_DUMMY_PRINTBUF(char* str, uint8_t* buf, uint8_t sz)
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
#define DEBUG_DUMMY_PRINTBUF(str, buf, sz)
#endif

MySigningDriverDummy::MySigningDriverDummy() : MySigningDriver() {
	verification_ongoing = false;
	current_nonce = 0;
}

bool MySigningDriverDummy::getNonce(MyMessage &msg) {
	uint8_t nonce_buf[2];
	nonce_buf[0] = SIGNING_IDENTIFIER;
	// This is normally a buffer but for our minimal driver we use a single byte
	// We at least use a new nonce for every session
	nonce_buf[1] = ++current_nonce;
	msg.set(nonce_buf, 2);
	verification_ongoing = true;
	timestamp = millis(); // Set timestamp to determine when to purge nonce
	// Be a little fancy to handle turnover (prolong the time allowed to timeout after turnover)
	// Note that if message is "too" quick, and arrives before turnover, it will be rejected
	// but this is consider such a rare case that it is accepted and rejects are 'safe'
	if (timestamp + VERIFICATION_TIMEOUT_MS < millis()) timestamp = 0;
	return true;
}

bool MySigningDriverDummy::checkTimer() {
	if (verification_ongoing) {
		if (millis() < timestamp || millis() > timestamp + VERIFICATION_TIMEOUT_MS) {
			DEBUG_DUMMY_PRINTLN("Verification timeout");
			// Purge nonce
			current_nonce = 0; // purge nonce when used
			verification_ongoing = false;
			return false; 
		}
	}
	return true;
}

bool MySigningDriverDummy::putNonce(MyMessage &msg) {
	if (mGetLength(msg) != 2) {
		DEBUG_DUMMY_PRINTLN("Incoming nonce with incorrect size");
		return false; // We require a signing identifier and a 1byte nonce
	}

	if (((uint8_t*)msg.getCustom())[0] != SIGNING_IDENTIFIER) {
		DEBUG_DUMMY_PRINTLN("Incorrect signing identifier");
		return false; 
	}

	current_nonce = ((uint8_t*)msg.getCustom())[1];
	return true;
}

bool MySigningDriverDummy::signMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_DUMMY_PRINTLN("Cannot fit any signature to this message");
		return false; 
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	DEBUG_DUMMY_PRINTBUF("Message to sign:", (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
	uint8_t signature = msg.data[0] ^ current_nonce;
	msg.data[mGetLength(msg)] = SIGNING_IDENTIFIER; // Add signing identifier
	msg.data[mGetLength(msg)+1] = signature;
	current_nonce = 0; // purge nonce when used
	return true;
}

bool MySigningDriverDummy::verifyMsg(MyMessage &msg) {
	if (!verification_ongoing) {
		DEBUG_DUMMY_PRINTLN("No active verification session");
		return false; 
	} else {
		// Make sure we have not expired
		if (!checkTimer()) {
			return false; 
		}

		verification_ongoing = false;

		if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER) {
			DEBUG_DUMMY_PRINTLN("Incorrect signing identifier");
			return false; 
		}

		DEBUG_DUMMY_PRINTBUF("Message to verify:", (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
		DEBUG_DUMMY_PRINTBUF("Signature in message:", (uint8_t*)&msg.data[mGetLength(msg)], 2);

		// The dummy signing is the nonce XOR the first payload byte,
		// so we "verify" by redoing that calculation with the nonce we transmitted
		// Message without a payload this implementation always accepts as signed and messages that cannot
		// fit a signature are also accepted as signed
		if (mGetLength(msg) && mGetLength(msg) < MAX_PAYLOAD-2) {
			uint8_t signature = msg.data[0] ^ current_nonce;
			if (msg.data[mGetLength(msg)+1] != signature) {
				DEBUG_DUMMY_PRINTBUF("Signature bad. Calculated signature:", &signature, 1);
				return false;
			} else {
				DEBUG_DUMMY_PRINTLN("Signature ok");
				return true;
			}
		} else {
			DEBUG_DUMMY_PRINTLN("No signature in message");
			return false; 
		}
	}
}
#endif
