/*
 ATSHA204 emulated signing backend. The emulated ATSHA204 implementation offers pseudo random
 number generation and HMAC-SHA256 authentication compatible with a "physical" ATSHA204.
 NOTE: Key is stored in clear text in the Arduino firmware. Therefore, the use of this back-end
 could compromise the key used in the signed message infrastructure if device is lost and its memory
 dumped.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningHmac256Soft_h
#define MySigningHmac256Soft_h

#include "MyConfig.h"
#include "MySigning.h"
#include "utility/ATSHA204.h"
#include "utility/sha256.h"
#include <stdint.h>

#define SIGNING_IDENTIFIER (1)

#ifdef MY_SECURE_NODE_WHITELISTING
typedef struct {
	uint8_t nodeId;
	uint8_t serial[SHA204_SERIAL_SZ];
} whitelist_entry_t;
#endif

// This implementation is the pure software variant of the ATSHA204.
// It is designed to work fully compliant with nodes using ATSHA204 in the network
// and therefore uses the same signing identifier as ATSHA204.
// Because it is completly software based, the quality of the generated random numbers
// is weaker though. Random numbers are generated using the Arduino library and seed
// is sampled from an analog pin. This pin should unconnected in the hardware.
// The pin is selected using MY_RANDOMSEED_PIN in MyConfig.h.
class MySigningAtsha204Soft : public MySigning
{ 
public:
	MySigningAtsha204Soft(bool requestSignatures=true,
#ifdef MY_SECURE_NODE_WHITELISTING
		uint8_t nof_whitelist_entries=0, const whitelist_entry_t* the_whitelist=NULL,
		const uint8_t* the_serial=NULL, //SHA204_SERIAL_SZ sized buffer expected if provided
#endif
		uint8_t randomseedPin=MY_RANDOMSEED_PIN);
	bool getNonce(MyMessage &msg);
	bool checkTimer(void);
	bool putNonce(MyMessage &msg);
	bool signMsg(MyMessage &msg);
	bool verifyMsg(MyMessage &msg);
private:
	Sha256Class Sha256;
	unsigned long timestamp;
	bool verification_ongoing;
	uint8_t current_nonce[NONCE_NUMIN_SIZE_PASSTHROUGH];
	uint8_t temp_message[32];
	uint8_t hmacKey[32];
	uint8_t rndPin;
	uint8_t hmac[32];
	bool calculateSignature(MyMessage &msg);
#ifdef MY_SECURE_NODE_WHITELISTING
	uint8_t whitlist_sz;
	const whitelist_entry_t* whitelist;
	const uint8_t* node_serial_info;
#endif
};

#endif
