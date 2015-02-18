/*
 Dummy signing backend. Does not provide any security measures.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningDriverDummy_h
#define MySigningDriverDummy_h

#include "MyConfig.h"
#include "MySigningDriver.h"
#include <stdint.h>

// The dummy signing driver implements the most rudimentary form of signing
// No pre-shared key is used, nonce is simply incremeted each turn and signature
// is only calculated on the fist byte using data XOR nonce.
// This driver is intended for library debugging and sensor network integrity
// verification (signed messages will force the radio to transmit all 32 bytes of data).
// It does check SIGNING_IDENTIFIER byte to avoid illegal mixing of signing back-ends in
// the network (as seen by this node) and it does verify proper execution order on the API.
// The dummy driver rejects all other back-ends.
class MySigningDriverDummy : public MySigningDriver
{ 
public:
	MySigningDriverDummy();
	bool getNonce(MyMessage &msg);
	bool checkTimer(void);
	bool putNonce(MyMessage &msg);
	bool signMsg(MyMessage &msg);
	bool verifyMsg(MyMessage &msg);
private:
	unsigned long timestamp;
	bool verification_ongoing;
	uint8_t current_nonce;
};

#endif
