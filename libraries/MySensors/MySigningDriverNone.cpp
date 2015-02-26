/*
 Disabled signing backend. Does not provide any security measures.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#include "MySigningDriver.h"
#include "MySigningDriverNone.h"

// Uncomment this to get some useful serial debug info (Serial.print and Serial.println expected)
//#define DEBUG_NONE_SIGNING

#ifdef DEBUG_NONE_SIGNING
#define DEBUG_NONE_PRINTLN(args) Serial.println(args)
#else
#define DEBUG_NONE_PRINTLN(args)
#endif

#ifdef DEBUG_NONE_SIGNING
static void DEBUG_NONE_PRINTBUF(char* str, uint8_t* buf, uint8_t sz)
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
#define DEBUG_NONE_PRINTBUF(str, buf, sz)
#endif

MySigningDriverNone::MySigningDriverNone() : MySigningDriver() {
}

bool MySigningDriverNone::getNonce(MyMessage &msg) {
	return true;
}

bool MySigningDriverNone::checkTimer() {
	return true;
}

bool MySigningDriverNone::putNonce(MyMessage &msg) {
	return true;
}

bool MySigningDriverNone::signMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_NONE_PRINTLN("Cannot fit any signature to this message");
		return false; 
	}
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	msg.data[mGetLength(msg)] = SIGNING_IDENTIFIER; // Add signing identifier
	return true;
}

bool MySigningDriverNone::verifyMsg(MyMessage &msg) {
	if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER)
		return false;
	else
		return true;
}
