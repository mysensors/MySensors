/*
 ATSHA204 signing backend. The Atmel ATSHA204 offers true random number generation and
 HMAC-SHA256 authentication with a readout-protected key.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#include "MySigning.h"
#include "MySigningAtsha204.h"

// Uncomment this to get some useful serial debug info (Serial.print and Serial.println expected)
//#define DEBUG_ATSHA_SIGNING

#ifdef DEBUG_ATSHA_SIGNING
#define DEBUG_ATSHA_PRINTLN(args) Serial.println(args)
#else
#define DEBUG_ATSHA_PRINTLN(args)
#endif

#ifdef DEBUG_ATSHA_SIGNING
static void DEBUG_ATSHA_PRINTBUF(char* str, uint8_t* buf, uint8_t sz)
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
#define DEBUG_ATSHA_PRINTBUF(str, buf, sz)
#endif


MySigningAtsha204::MySigningAtsha204(bool requestSignatures, uint8_t atshaPin)
	:
	MySigning(requestSignatures),
	atsha204(atshaPin),
	verification_ongoing(false)
{
	// We set the part of the 32-byte nonce that does not fit into a message to 0xAA
	memset(current_nonce, 0xAA, sizeof(current_nonce));
}

bool MySigningAtsha204::getNonce(MyMessage &msg) {
	// Wake device before starting operations
	if (atsha204.sha204c_wakeup(rx_buffer) != SHA204_SUCCESS) {
		DEBUG_ATSHA_PRINTLN("Failed to wake device");
		return false; 
	}

	// Generate random number for use as nonce
	if (atsha204.sha204m_execute(SHA204_RANDOM, RANDOM_NO_SEED_UPDATE, 0, 0, NULL,
									RANDOM_COUNT, tx_buffer, RANDOM_RSP_SIZE, rx_buffer) != SHA204_SUCCESS) {
		DEBUG_ATSHA_PRINTLN("Failed to generate nonce");
		return false; 
	}

	// We can put the device back to sleep as we will program the nonce once we do the verification
	atsha204.sha204c_sleep();

	// Store nonce and replace the first byte in the nonce with our signing identifier
	memcpy(current_nonce, &rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD);
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
			DEBUG_ATSHA_PRINTLN("Verification timeout");
			// Purge nonce
			memset(current_nonce, 0xAA, NONCE_NUMIN_SIZE_PASSTHROUGH);
			verification_ongoing = false;
			return false; 
		}
	}
	return true;
}

bool MySigningAtsha204::putNonce(MyMessage &msg) {
	if (mGetLength(msg) != MAX_PAYLOAD) {
		DEBUG_ATSHA_PRINTLN("Incoming nonce with incorrect size");
		return false; // We require as big nonce as possible
	}

	if (((uint8_t*)msg.getCustom())[0] != SIGNING_IDENTIFIER) {
		DEBUG_ATSHA_PRINTLN("Incorrect signing identifier");
		return false; 
	}

	memcpy(current_nonce, (uint8_t*)msg.getCustom(), MAX_PAYLOAD);
	return true;
}

bool MySigningAtsha204::signMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_ATSHA_PRINTLN("Cannot fit any signature to this message");
		return false; 
	}

	// Calculate signature of message
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	DEBUG_ATSHA_PRINTBUF("Message to sign:", (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
	if (!calculateSignature(msg)) {
		DEBUG_ATSHA_PRINTLN("Failed to calculate signature");
		mSetSigned(msg, 0); // make sure signing flag is truthful
		return false;
	}

	// Transfer as much signature data as the remaining space in the message permits
	memcpy(&msg.data[mGetLength(msg)], &rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg));

	return true;
}

bool MySigningAtsha204::verifyMsg(MyMessage &msg) {
	if (!verification_ongoing) {
		DEBUG_ATSHA_PRINTLN("No active verification session");
		return false; 
	} else {
		// Make sure we have not expired
		if (!checkTimer()) {
			return false; 
		}

		verification_ongoing = false;

		if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER) {
			DEBUG_ATSHA_PRINTLN("Incorrect signing identifier");
			return false; 
		}

		DEBUG_ATSHA_PRINTBUF("Message to verify:", (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));
		DEBUG_ATSHA_PRINTBUF("Signature in message:", (uint8_t*)&msg.data[mGetLength(msg)], MAX_PAYLOAD-mGetLength(msg));

		// Get signature of message
		if (!calculateSignature(msg)) {
			DEBUG_ATSHA_PRINTLN("Failed to calculate signature");
			return false; 
		}

		// Compare the caluclated signature with the provided signature
		if (memcmp(&msg.data[mGetLength(msg)], &rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg))) {
			DEBUG_ATSHA_PRINTBUF("Signature bad. Calculated signature:", &rx_buffer[SHA204_BUFFER_POS_DATA], MAX_PAYLOAD-mGetLength(msg));
			return false; 
		} else {
			DEBUG_ATSHA_PRINTLN("Signature ok");
			return true;
		}
	}
}

// Helper to calculate signature of msg (returned in rx_buffer[SHA204_BUFFER_POS_DATA])
bool MySigningAtsha204::calculateSignature(MyMessage &msg) {
	memset(temp_message, 0, 32);
	memcpy(temp_message, (uint8_t*)&msg.data[1-HEADER_SIZE], MAX_MESSAGE_LENGTH-1-(MAX_PAYLOAD-mGetLength(msg)));

	// Wake device before starting operations
	if (atsha204.sha204c_wakeup(rx_buffer) != SHA204_SUCCESS) return false;

	// Program the data to sign into the ATSHA204
	if (atsha204.sha204m_execute(SHA204_WRITE, SHA204_ZONE_DATA | SHA204_ZONE_COUNT_FLAG, 8 << 3, 32, temp_message,
									WRITE_COUNT_LONG, tx_buffer, WRITE_RSP_SIZE, rx_buffer) != SHA204_SUCCESS) {
		DEBUG_ATSHA_PRINTLN("Failed to program data");
		return false; 
	}

	// Program the nonce to use for the signature (has to be done just before GENDIG due to chip limitations)
	if (atsha204.sha204m_execute(SHA204_NONCE, NONCE_MODE_PASSTHROUGH, 0, NONCE_NUMIN_SIZE_PASSTHROUGH, current_nonce,
									NONCE_COUNT_LONG, tx_buffer, NONCE_RSP_SIZE_SHORT, rx_buffer) != SHA204_SUCCESS) {
		DEBUG_ATSHA_PRINTLN("Failed to program nonce");
		// Purge nonce when used
		memset(current_nonce, 0xAA, NONCE_NUMIN_SIZE_PASSTHROUGH);
		return false; 
	}

	// Purge nonce when used
	memset(current_nonce, 0xAA, NONCE_NUMIN_SIZE_PASSTHROUGH);

	// Generate digest of data and nonce
	if (atsha204.sha204m_execute(SHA204_GENDIG, GENDIG_ZONE_DATA, 8, 0, NULL,
									GENDIG_COUNT_DATA, tx_buffer, GENDIG_RSP_SIZE, rx_buffer) != SHA204_SUCCESS) {
		DEBUG_ATSHA_PRINTLN("Failed to generate digest");
		return false; 
	}

	// Calculate HMAC of message+nonce digest and secret key
	if (atsha204.sha204m_execute(SHA204_HMAC, HMAC_MODE_SOURCE_FLAG_MATCH, 0, 0, NULL,
									HMAC_COUNT, tx_buffer, HMAC_RSP_SIZE, rx_buffer) != SHA204_SUCCESS) {
		DEBUG_ATSHA_PRINTLN("Failed to generate HMAC");
		return false; 
	}

	// Put device back to sleep
	atsha204.sha204c_sleep();

	// Overwrite the first byte in the signature with the signing identifier
	rx_buffer[SHA204_BUFFER_POS_DATA] = SIGNING_IDENTIFIER;

	DEBUG_ATSHA_PRINTBUF("HMAC:", &rx_buffer[SHA204_BUFFER_POS_DATA], 32);
	return true; // We return with the signature in rx_buffer[SHA204_BUFFER_POS_DATA]
}
