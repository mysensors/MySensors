#ifndef MyConfig_h
#define MyConfig_h
#include <stdint.h>
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

// MySigningDriverAtsha204 default setting
#define MY_ATSHA204_PIN 17 // A3 - pin where ATSHA204 is attached

// MySigningDriverAtsha204Soft default settings
#define MY_RANDOMSEED_PIN 7 // A7 - Pin used for random generation (do not connect anything to this)
// Key to use for HMAC calculation in MySigningDriverAtsha204Soft (32 bytes)
#define MY_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00


/**********************************
*  NRF24L01 Driver Defaults
***********************************/
#define RF24_CE_PIN		   9
#define RF24_CS_PIN		   10
#define RF24_PA_LEVEL 	   RF24_PA_MAX
#define RF24_CHANNEL	   76             //RF channel for the sensor net, 0-127
#define RF24_DATARATE 	   RF24_250KBPS   //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
#define RF24_BASE_RADIO_ID ((uint64_t)0xA8A8E1FC00LL) // This is also act as base value for sensor nodeId addresses. Change this (or channel) if you have more than one sensor network.

// Enable SOFTSPI for the W5100 Ethernet module
//#define SOFTSPI
#ifdef SOFTSPI
	// Define the soft SPI pins used for NRF radio
	const uint8_t SOFT_SPI_MISO_PIN = 16;
    const uint8_t SOFT_SPI_MOSI_PIN = 15;
    const uint8_t SOFT_SPI_SCK_PIN = 14;
#endif


#endif
