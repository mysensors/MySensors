/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */


#ifndef MyConfig_h
#define MyConfig_h
#include <stdint.h>

// Enable debug flag for debug prints. This will add a lot to the size of the final sketch but good
// to see what is actually is happening when developing
//#define MY_DEBUG

// Enable this line, If you are using TX(1), RX(0) as normal I/O pin
// #define MY_DISABLED_SERIAL

// Serial output baud rate (for debug prints and serial gateway)
#ifndef MY_BAUD_RATE
#define MY_BAUD_RATE 115200
#endif

/**********************************
*  Over the air firmware updates
***********************************/

// The following define enables the safe over-the-air firmware update feature
// which requires external flash and the DualOptiBoot bootloader.
// Note: You can still have OTA FW updates without external flash but it
// requires the MYSBootloader and disabled MY_OTA_FIRMWARE_FEATURE
//#define MY_OTA_FIRMWARE_FEATURE
// Slave select pin for external flash
#ifdef MY_OTA_FLASH_SS
#define MY_OTA_FLASH_SS 8
#endif
// Flash jdecid
#ifdef MY_OTA_FLASH_JDECID
#define MY_OTA_FLASH_JDECID 0x1F65
#endif

/**********************************
*  Information LEDs blinking
***********************************/
// This feature enables LEDs blinking on message receive, transmit
// or if some error occurred. This was commonly used only in gateways,
// but now can be used in any sensor node. Also the LEDs can now be
// disabled in the gateway.

//#define MY_WITH_LEDS_BLINKING

// The following setting allows you to inverse the blinking feature WITH_LEDS_BLINKING
// When WITH_LEDS_BLINKING_INVERSE is enabled LEDSs are normally turned on and switches
// off when blinking

//#define WITH_LEDS_BLINKING_INVERSE


// default LEDs blinking period in milliseconds
#ifndef MY_DEFAULT_LED_BLINK_PERIOD
#define MY_DEFAULT_LED_BLINK_PERIOD 300
#endif
// The RX LED default pin
#ifndef MY_DEFAULT_RX_LED_PIN
#define MY_DEFAULT_RX_LED_PIN 6
#endif
// The TX LED default pin
#ifndef MY_DEFAULT_TX_LED_PIN
#define MY_DEFAULT_TX_LED_PIN 5
#endif
// The Error LED default pin
#ifndef MY_DEFAULT_ERR_LED_PIN
#define MY_DEFAULT_ERR_LED_PIN 4
#endif

/**********************************************
*  Gateway inclusion button/mode configuration
**********************************************/
// Enabled inclusion mode feature
//#define MY_INCLUSION_MODE_FEATURE

// Enables inclusion-mode button feature on the gateway device
//#define MY_INCLUSION_BUTTON_FEATURE

// Disable inclusion mode button if inclusion mode feature is not enabled
#ifndef MY_INCLUSION_MODE_FEATURE
#undef MY_INCLUSION_BUTTON_FEATURE
#endif

// The default input pin used for the inclusion mode button
#ifndef MY_INCLUSION_MODE_BUTTON_PIN
#define MY_INCLUSION_MODE_BUTTON_PIN 3
#endif
// Number of seconds (default one minute) inclusion mode should be enabled
#ifndef MY_INCLUSION_MODE_DURATION
#define MY_INCLUSION_MODE_DURATION 60
#endif

/**********************************
*  Message Signing Settings
***********************************/
// Disable to completely disable signing functionality in library
//#define MY_SIGNING_FEATURE

// Define a suitable timeout for a signature verification session
// Consider the turn-around from a nonce being generated to a signed message being received
// which might vary, especially in networks with many hops. 5s ought to be enough for anyone.
#ifndef MY_VERIFICATION_TIMEOUT_MS
#define MY_VERIFICATION_TIMEOUT_MS 5000
#endif

// Enable to turn on white-listing
// When enabled, a signing node will salt the signature with it's unique signature and nodeId.
// The verifying node will look up the sender in a local table of trusted nodes and
// do the corresponding salting in order to verify the signature.
// For this reason, if white listing is enabled on one of the nodes in a sign-verify pair, both
// nodes have to implement white listing for this to work.
// Note that a node can still transmit a non-salted message (i.e. have white listing disabled)
// to a node that has white listing enabled (assuming the receiver does not have a matching entry
// for the sender in it's white list)
//#define MY_SECURE_NODE_WHITELISTING

// MySigningAtsha204 default setting
#ifndef MY_ATSHA204_PIN
#define MY_ATSHA204_PIN 17 // A3 - pin where ATSHA204 is attached
#endif

// MySigningAtsha204Soft default settings
#ifndef MY_RANDOMSEED_PIN
#define MY_RANDOMSEED_PIN 7 // A7 - Pin used for random generation (do not connect anything to this)
#endif

// Key to use for HMAC calculation in MySigningAtsha204Soft (32 bytes)
#ifndef MY_HMAC_KEY
#define MY_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
#endif

/**********************************
*  NRF24L01 Driver Defaults
***********************************/
#define RF24_CE_PIN		   9
#define RF24_CS_PIN		   10
#define RF24_PA_LEVEL 	   RF24_PA_MAX
#define RF24_PA_LEVEL_GW   RF24_PA_LOW
// RF channel for the sensor net, 0-127
#define RF24_CHANNEL	   76
//RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
#define RF24_DATARATE 	   RF24_250KBPS
// This is also act as base value for sensor nodeId addresses. Change this (or channel) if you have more than one sensor network.
#define RF24_BASE_RADIO_ID ((uint64_t)0xA8A8E1FC00LL)

// Enable SOFTSPI for NRF24L01 when using the W5100 Ethernet module
//#define MY_SOFTSPI

#ifndef MY_SOFT_SPI_SCK_PIN
#define MY_SOFT_SPI_SCK_PIN 14
#endif
#ifndef MY_SOFT_SPI_MISO_PIN
#define MY_SOFT_SPI_MISO_PIN 16
#endif
#ifndef MY_SOFT_SPI_MOSI_PIN
#define MY_SOFT_SPI_MOSI_PIN 15
#endif

/**********************************
*  RFM69 Driver Defaults
***********************************/

// Default frequency to use. This must match the hardware version of the RFM69 radio (uncomment one):
#ifndef MY_RFM69_FREQUENCY
// #define MY_RFM69_FREQUENCY   RF69_433MHZ
#define MY_RFM69_FREQUENCY   RF69_868MHZ
//#define MY_RFM69_FREQUENCY     RF69_915MHZ
#endif

// Default network id. Use the same for all nodes that will talk to each other
#ifndef MY_RFM69_NETWORKID
#define MY_RFM69_NETWORKID     100
#endif
#ifndef MY_RF69_IRQ_PIN
#define MY_RF69_IRQ_PIN RF69_IRQ_PIN
#endif
#ifndef MY_RF69_SPI_CS
#define MY_RF69_SPI_CS RF69_SPI_CS
#endif
#ifndef MY_RF69_IRQ_NUM
#define MY_RF69_IRQ_NUM RF69_IRQ_NUM
#endif

// Enable this for encryption of packets
//#define MY_RFM69_ENABLE_ENCRYPTION
#ifndef MY_RFM69_ENCRYPTKEY
#define MY_RFM69_ENCRYPTKEY    "sampleEncryptKey" //exactly the same 16 characters/bytes on all nodes!
#endif

/**************************************
*  Gateway Transport Ethernet Defaults
***************************************/
// Enable to use DHCP for getting IP address. If not defined, static IP is expected
//#define IP_ADDRESS_DHCP

// Enable to use UDP instead of plain Ethernet
#define MY_USE_UDP

#endif
