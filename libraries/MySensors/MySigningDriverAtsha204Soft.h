/*
 ATSHA204 emulated signing backend. The emulated ATSHA204 implementation offers pseudo random
 number generation and HMAC-SHA256 authentication compatible with a "physical" ATSHA204.
 NOTE: Key is stored in clear text in the Arduino firmware. Therefore, the use of this back-end
 could compromise the key used in the signed message infrastructure if device is lost and its memory
 dumped.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningDriverHmac256Soft_h
#define MySigningDriverHmac256Soft_h

#include "MyConfig.h"
#include "MySigningDriver.h"
#include "utility/ATSHA204.h"
#include <stdint.h>

// This implementation is the pure software variant of the ATSHA204.
// It is designed to work fully compliant with nodes using ATSHA204 in the network
// and therefore uses the same signing identifier as ATSHA204.
// Because it is completly software based, the quality of the generated random numbers
// is weaker though. Random numbers are generated using the Arduino library and seed
// is sampled from an analog pin. This pin should unconnected in the hardware.
// The pin is selected using RANDOMSEED_PIN in MyConfig.h.
class MySigningDriverAtsha204Soft : public MySigningDriver
{ 
public:
	MySigningDriverAtsha204Soft();
	bool getNonce(MyMessage &msg);
	bool checkTimer(void);
	bool putNonce(MyMessage &msg);
	bool signMsg(MyMessage &msg);
	bool verifyMsg(MyMessage &msg);
private:
	unsigned long timestamp;
	bool verification_ongoing;
	uint8_t current_nonce[NONCE_NUMIN_SIZE_PASSTHROUGH];
	uint8_t temp_message[32];
	uint8_t* hmac;
	bool calculateSignature(MyMessage &msg);
};

#endif
