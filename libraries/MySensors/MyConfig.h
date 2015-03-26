#ifndef MyConfig_h
#define MyConfig_h
#include <stdint.h>

// Enable debug flag for debug prints. This will add a lot to the size of the final sketch but good
// to see what is actually is happening when developing
#define DEBUG

// Serial output baud rate (for debug prints and serial gateway)
#define BAUD_RATE 115200


/**********************************
*  Over the air firmware updates
***********************************/

// The following define enables the safe over-the-air firmware update feature
// which requires external flash and the DualOptiBoot bootloader.
// Note: You can still have OTA FW updates without external flash but it
// requires the MYSBootloader and disabled MY_OTA_FIRMWARE_FEATURE
#define MY_OTA_FIRMWARE_FEATURE
// Slave select pin for external flash
#define MY_OTA_FLASH_SS 8
// Number of times to request a fw block before giving up
#define MY_OTA_RETRY 5
// Number of millisecons before re-request a fw block
#define MY_OTA_RETRY_DELAY 500
// Bootloader version
#define MY_OTA_BOOTLOADER_MAJOR_VERSION 2
#define MY_OTA_BOOTLOADER_MINOR_VERSION 0
#define MY_OTA_BOOTLOADER_VERSION (MY_OTA_BOOTLOADER_MINOR_VERSION * 256 + MY_OTA_BOOTLOADER_MAJOR_VERSION)


/**********************************
*  Message Signing Settings
***********************************/
// Disable to completly disable signing functionality in library
//#define MY_SIGNING_FEATURE

// Define a suitable timeout for a signature verification session
// Consider the turnaround from a nonce being generated to a signed message being received
// which might vary, especially in networks with many hops. 5s ought to be enough for anyone.
#define MY_VERIFICATION_TIMEOUT_MS 5000

// Enable to turn on whitelisting
// When enabled, a signing node will salt the signature with it's unique signature and nodeId.
// The verifying node will look up the sender in a local table of trusted nodes and
// do the corresponding salting in order to verify the signature.
// For this reason, if whitelisting is enabled on one of the nodes in a sign-verify pair, both
// nodes have to implement whitelisting for this to work.
// Note that a node can still transmit a non-salted message (i.e. have whitelisting disabled)
// to a node that has whitelisting enabled (assuming the receiver does not have a matching entry
// for the sender in it's whitelist)
//#define MY_SECURE_NODE_WHITELISTING

// MySigningAtsha204 default setting
#define MY_ATSHA204_PIN 17 // A3 - pin where ATSHA204 is attached

// MySigningAtsha204Soft default settings
#define MY_RANDOMSEED_PIN 7 // A7 - Pin used for random generation (do not connect anything to this)

// Key to use for HMAC calculation in MySigningAtsha204Soft (32 bytes)
#define MY_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
                    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00


/**********************************
*  NRF24L01 Driver Defaults
***********************************/
#define RF24_CE_PIN		   9
#define RF24_CS_PIN		   10
#define RF24_PA_LEVEL 	   RF24_PA_MAX
#define RF24_PA_LEVEL_GW   RF24_PA_LOW
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
