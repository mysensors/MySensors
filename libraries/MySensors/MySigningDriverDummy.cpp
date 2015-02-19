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
}

bool MySigningDriverDummy::getNonce(MyMessage &msg) {
	return true;
}

bool MySigningDriverDummy::checkTimer() {
	return true;
}

bool MySigningDriverDummy::putNonce(MyMessage &msg) {
	return true;
}

bool MySigningDriverDummy::signMsg(MyMessage &msg) {
	// If we cannot fit any signature in the message, refuse to sign it
	if (mGetLength(msg) > MAX_PAYLOAD-2) {
		DEBUG_DUMMY_PRINTLN("Cannot fit any signature to this message");
		return false; 
	}
	mSetSigned(msg, 1); // make sure signing flag is set before signature is calculated
	msg.data[mGetLength(msg)] = SIGNING_IDENTIFIER; // Add signing identifier
	return true;
}

bool MySigningDriverDummy::verifyMsg(MyMessage &msg) {
	if (msg.data[mGetLength(msg)] != SIGNING_IDENTIFIER)
		return false;
	else
		return true;
}
#endif
