/*
 ATSHA204 signing backend. The Atmel ATSHA204 offers true random number generation and
 HMAC-SHA256 authentication with a readout-protected key.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningDriverAtsha204_h
#define MySigningDriverAtsha204_h

#include "MyConfig.h"
#include "MySigningDriver.h"
#include "utility/ATSHA204.h"
#include <stdint.h>

#define SIGNING_IDENTIFIER (1)

// The ATSHA204 is capable of generating proper random numbers for nonce
// and can calculate HMAC-SHA256 signatures. This is enterprise-
// level of security and ought to implement the signing needs for anybody.
class MySigningDriverAtsha204 : public MySigningDriver
{ 
public:
	MySigningDriverAtsha204();
	bool getNonce(MyMessage &msg);
	bool checkTimer(void);
	bool putNonce(MyMessage &msg);
	bool signMsg(MyMessage &msg);
	bool verifyMsg(MyMessage &msg);
private:
	atsha204Class atsha204;
	unsigned long timestamp;
	bool verification_ongoing;
	uint8_t current_nonce[NONCE_NUMIN_SIZE_PASSTHROUGH];
	uint8_t temp_message[32];
	uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
	uint8_t tx_buffer[SHA204_CMD_SIZE_MAX];
	bool calculateSignature(MyMessage &msg);
};

#endif
