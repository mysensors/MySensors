#ifndef MyConfig_h
#define MyConfig_h

/**********************************
*  Enable/Disable debug logging
***********************************/
#define DEBUG


/**********************************
*  Message Signing Settings
***********************************/
// Define a suitable timeout for a signature verification session
// Consider the turnaround from a nonce being generated to a signed message being received
// which might vary, especially in networks with many hops. 5s ought to be enough for anyone.
#define MY_VERIFICATION_TIMEOUT_MS 5000

// MySigningDriverAtsha204 settinga
#define MY_ATSHA204_PIN 17 // A3 - pin where ATSHA204 is attached

// MySigningDriverAtsha204Soft settings
#define MY_RANDOMSEED_PIN 7 // A7 - Pin used for random generation (do not connect anything to this)
// Key to use for HMAC calculation in MySigningDriverAtsha204Soft (32 bytes)
#define MY_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00


/**********************************
*  NRF24L01 Driver Defaults
***********************************/
#define RF24_CE_PIN		   9
#define RF24_CS_PIN		   10
#define RF24_PA_LEVEL 	   RF24_PA_MAX



#endif
