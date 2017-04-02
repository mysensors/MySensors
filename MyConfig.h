/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in RAM or EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * @file MyConfig.h
 *
 * MySensors specific configurations
 */
#ifndef MyConfig_h
#define MyConfig_h
#include <stdint.h>


/**********************************
*  Serial and debug options
***********************************/

/**
 * @def MY_DEBUG
 * @brief Enable MY_DEBUG in sketch to show debug prints. This option will add a lot to the size of the
 * final sketch but is helpful to see what is actually is happening during development
 */
//#define MY_DEBUG

/**
 * @def MY_SPECIAL_DEBUG
 * @brief Enable MY_SPECIAL_DEBUG to enable I_DEBUG messages.
 * I_DEBUG requests are:
 * R: routing info (only repeaters): received msg XXYY (as stream), where XX is the node and YY the routing node
 * V: CPU voltage
 * F: CPU frequency
 * M: free memory
 * E: clear MySensors EEPROM area and reboot (i.e. "factory" reset)
 */
//#define MY_SPECIAL_DEBUG

/**
 * @def MY_DISABLED_SERIAL
 * @brief Enable MY_DISABLED_SERIAL in sketch if you want to use TX(1), RX(0) as normal I/O pins
 */
//#define MY_DISABLED_SERIAL

/**
 * @def MY_CORE_ONLY
 * @brief Enable MY_CORE_ONLY flag if you want to use core functions without loading the framework
 */
//#define MY_CORE_ONLY

// Turn off debug if serial pins is used for other stuff
#ifdef MY_DISABLED_SERIAL
#undef MY_DEBUG
#endif

/**
 * @def MY_BAUD_RATE
 * @brief Serial output baud rate (debug prints and serial gateway speed).
 */
#ifndef MY_BAUD_RATE
#define MY_BAUD_RATE					(115200ul)
#endif
/**
 * @def MY_SERIAL_OUTPUT_SIZE
 * @brief Max. characters for serial output.
 */
#ifndef MY_SERIAL_OUTPUT_SIZE
#define MY_SERIAL_OUTPUT_SIZE			(120u)
#endif

/**
 * @def MY_DISABLE_REMOTE_RESET
 * @brief Disables over-the-air reset of node
 */
//#define MY_DISABLE_REMOTE_RESET

/**********************************
*  Radio selection and node config
***********************************/

// Selecting uplink transport layer is optional (for a gateway node).
//#define MY_RADIO_RF24
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95
//#define MY_RS485

// legacy
#ifdef MY_RADIO_NRF24
//MY_RADIO_NRF24 is deprecated
#define MY_RADIO_RF24
#endif

/**
 * @def MY_RAM_ROUTING_TABLE_FEATURE
 * @brief If enabled, the routing table is kept in RAM (if memory allows) and saved in regular intervals.
 * note: AVR has limited memory, use with care
 */
#define MY_RAM_ROUTING_TABLE_FEATURE

/**
 * @def MY_ROUTING_TABLE_SAVE_INTERVAL_MS
 * @brief Interval to dump content of routing table to eeprom
 */
#ifndef MY_ROUTING_TABLE_SAVE_INTERVAL_MS
#define MY_ROUTING_TABLE_SAVE_INTERVAL_MS	(30*60*1000ul)
#endif
/**
 * @def MY_TRANSPORT_SANITY_CHECK
 * @brief If enabled, node will check transport in regular intervals to detect HW issues and re-initialize in case of failure.
 * This feature is enabled for all repeater nodes (incl. GW)
 */
//#define MY_TRANSPORT_SANITY_CHECK

/**
 * @def MY_TRANSPORT_SANITY_CHECK_INTERVAL_MS
 * @brief Interval (in ms) for transport sanity checks
 */
#ifndef MY_TRANSPORT_SANITY_CHECK_INTERVAL_MS
#define MY_TRANSPORT_SANITY_CHECK_INTERVAL_MS (15*60*1000ul)
#endif
/**
 * @def MY_TRANSPORT_DISCOVERY_INTERVAL_MS
 * @brief This is a gateway-only feature: Interval (in ms) to issue network discovery checks
 */
#ifndef MY_TRANSPORT_DISCOVERY_INTERVAL_MS
#define MY_TRANSPORT_DISCOVERY_INTERVAL_MS (20*60*1000ul)
#endif

/**
 *@def MY_TRANSPORT_UPLINK_CHECK_DISABLED
 *@brief If set, uplink check to GW is disabled during transport initialisation
 */
//#define MY_TRANSPORT_UPLINK_CHECK_DISABLED

/**
 *@def MY_TRANSPORT_MAX_TX_FAILURES
 *@brief Set to override max. consecutive TX failures until SNP is initiated
 */
//#define MY_TRANSPORT_MAX_TX_FAILURES (10u)

/**
 * @def MY_REGISTRATION_FEATURE
 * @brief If enabled, node has to register to gateway/controller before allowed to send sensor data.
 */
#define MY_REGISTRATION_FEATURE

/**
 * @def MY_REGISTRATION_RETRIES
 * @brief Number of registration retries if no reply received from GW/controller
 */

#ifndef MY_REGISTRATION_RETRIES
#define MY_REGISTRATION_RETRIES			(3u)
#endif

/**
* @def MY_REGISTRATION_DEFAULT
* @brief Node registration default - this applies if no registration response is received from controller
*/
#define MY_REGISTRATION_DEFAULT			true

/**
 * @def MY_REGISTRATION_CONTROLLER
 * @brief If enabled, node registration request has to be handled by controller
 */
// #define MY_REGISTRATION_CONTROLLER

/**
 * @def MY_CORE_COMPATIBILITY_CHECK
 * @brief If enabled, library compatibility is checked during node registration. Incompatible libraries are unable to send sensor data.
 */
#define MY_CORE_COMPATIBILITY_CHECK

/**
 * @def MY_TRANSPORT_WAIT_READY_MS
 * @brief Timeout in MS until transport is ready during startup, set to 0 for no timeout
 */
#ifndef MY_TRANSPORT_WAIT_READY_MS
#define MY_TRANSPORT_WAIT_READY_MS		(0ul)
#endif

/**
 * @def MY_PASSIVE_NODE
 * @brief If enabled, the node operates fully autonomously, i.e. messages are sent without ACKing.
 * note: All transport-related checks and safety-mechanisms are disabled.
 * Requirement: @ref MY_NODE_ID needs to be set, @ref MY_PARENT_NODE_ID and @ref MY_PARENT_NODE_IS_STATIC are optional.
 * Singing, registration, and OTA FW update are disabled.
 */
//#define MY_PASSIVE_NODE


/**
 * @def MY_NODE_ID
 * @brief Node id defaults to AUTO (tries to fetch id from controller).
 */
#ifndef MY_NODE_ID
#define MY_NODE_ID						AUTO
#endif

/**
 * @def MY_PARENT_NODE_ID
 * @brief Node parent defaults to AUTO (tries to find a parent automatically).
 */
#ifndef MY_PARENT_NODE_ID
#define MY_PARENT_NODE_ID				AUTO
#endif

/**
 * @def MY_PARENT_NODE_IS_STATIC
 * @brief Enable MY_PARENT_NODE_IS_STATIC to disable fall back if parent node fails
 */
//#define MY_PARENT_NODE_IS_STATIC

/**
 * @def MY_REPEATER_FEATURE
 * @brief Enables repeater functionality (relays messages from other nodes)
 */
//#define MY_REPEATER_FEATURE

/**
 * @def MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS
 * @brief Timeout (in ms) to re-establish link if node is send to sleep and transport is not ready.
 */
#ifndef MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS
#define MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS (10*1000ul)
#endif

/**
 * @def MY_SMART_SLEEP_WAIT_DURATION_MS
 * @brief The wait period (in ms) before going to sleep when using smartSleep-functions.
 *
 * This period has to be long enough for controller to be able to send out
 * potential buffered messages.
 */
#ifndef MY_SMART_SLEEP_WAIT_DURATION_MS
#define MY_SMART_SLEEP_WAIT_DURATION_MS (500ul)
#endif

/**********************************
*  Over the air firmware updates
***********************************/

// Enable MY_OTA_FIRMWARE_FEATURE in sketch to allow safe over-the-air firmware updates.
// This feature requires external flash and the DualOptiBoot boot-loader.
// Note: You can still have OTA FW updates without external flash but it
// requires the MYSBootloader and disabled MY_OTA_FIRMWARE_FEATURE
//#define MY_OTA_FIRMWARE_FEATURE

/**
 * @def MY_OTA_FLASH_SS
 * @brief Slave select pin for external flash.
 */
#ifndef MY_OTA_FLASH_SS
#define MY_OTA_FLASH_SS					(8)
#endif

/**
 * @def MY_OTA_FLASH_JDECID
 * @brief Flash JDECID. Use (0x00) if unknown.
 */
#ifndef MY_OTA_FLASH_JDECID
#define MY_OTA_FLASH_JDECID				(0x1F65)
#endif


/**********************************
*  Gateway config
***********************************/

/**
 * @def MY_GATEWAY_MAX_RECEIVE_LENGTH
 * @brief Max buffersize needed for messages coming from controller.
 */
#ifndef MY_GATEWAY_MAX_RECEIVE_LENGTH
#define MY_GATEWAY_MAX_RECEIVE_LENGTH	(100u)
#endif

/**
 * @def MY_GATEWAY_MAX_SEND_LENGTH
 * @brief Max buffer size when sending messages.
 */
#ifndef MY_GATEWAY_MAX_SEND_LENGTH
#define MY_GATEWAY_MAX_SEND_LENGTH		(120u)
#endif

/**
 * @def MY_GATEWAY_MAX_CLIENTS
 * @brief Max number of parallel clients (sever mode).
 */
#ifndef MY_GATEWAY_MAX_CLIENTS
#define MY_GATEWAY_MAX_CLIENTS			(1u)
#endif



/**********************************
*  Information LEDs blinking
***********************************/
// If one of the following is defined here, or in the sketch, the pin will be used for the
// corresponding led function.
// They have to be enabled here (or in your sketch). Replace x with the pin number you have the LED on.
//
// NOTE!! that on some platforms (for example sensebender GW) the hardware variant can enable LEDs by default,
// These defaults can be overridden by defining one of these.
//#define MY_DEFAULT_ERR_LED_PIN x
//#define MY_DEFAULT_TX_LED_PIN x
//#define MY_DEFAULT_RX_LED_PIN x

/**
 * @def MY_WITH_LEDS_BLINKING_INVERSE
 * @brief The following setting allows you to inverse the LED blinking.
 * When MY_WITH_LEDS_BLINKING_INVERSE is enabled LEDSs are normally turned on and switches off when blinking.
 */
//#define MY_WITH_LEDS_BLINKING_INVERSE

/**
 * @def MY_INDICATION_HANDLER
 * @brief Enable to use own indication handler.
 */
//#define MY_INDICATION_HANDLER

/**
* @def MY_DEFAULT_LED_BLINK_PERIOD
* @brief Default LEDs blinking period in milliseconds.
*/
#ifndef MY_DEFAULT_LED_BLINK_PERIOD
#define MY_DEFAULT_LED_BLINK_PERIOD 300
#endif



/**********************************************
*  Gateway inclusion button/mode configuration
**********************************************/

/**
* @def MY_INCLUSION_MODE_FEATURE
* @brief Enables inclusion mode feature.
*/
//#define MY_INCLUSION_MODE_FEATURE

/**
* @def MY_INCLUSION_BUTTON_FEATURE
* @brief Enables inclusion-mode button feature on the gateway device
*/
//#define MY_INCLUSION_BUTTON_FEATURE

// Disable inclusion mode button if inclusion mode feature is not enabled
#ifndef MY_INCLUSION_MODE_FEATURE
#undef MY_INCLUSION_BUTTON_FEATURE
#endif

/**
 * @def MY_INCLUSION_MODE_BUTTON_PIN
 * @brief The default input pin used for the inclusion mode button.
 */
#ifndef MY_INCLUSION_MODE_BUTTON_PIN
#if defined(ARDUINO_ARCH_ESP8266)
#define MY_INCLUSION_MODE_BUTTON_PIN	(5)
#else
#define MY_INCLUSION_MODE_BUTTON_PIN	(3)
#endif
#endif

/**
 * @def MY_INCLUSION_MODE_DURATION
 * @brief Number of seconds (default one minute) inclusion mode should be enabled.
 */

#ifndef MY_INCLUSION_MODE_DURATION
#define MY_INCLUSION_MODE_DURATION		(60)
#endif

/**
 * @def MY_INCLUSION_BUTTON_PRESSED
 * @brief The logical level indicating a pressed inclusion mode button.
 */
#if defined(MY_INCLUSION_BUTTON_EXTERNAL_PULLUP)
#define MY_INCLUSION_BUTTON_PRESSED		HIGH
#else
#define MY_INCLUSION_BUTTON_PRESSED		LOW
#endif

/**********************************
*  Message Signing Settings
***********************************/
/**
 * @def MY_DEBUG_VERBOSE_SIGNING
 * @brief Flag for verbose debug prints related to signing. Requires DEBUG to be enabled.
 * This will add even more to the size of the final sketch!
 */
//#define MY_DEBUG_VERBOSE_SIGNING

/**
 * @def MY_SIGNING_ATSHA204
 * @brief Enables HW backed signing functionality in library.
 *
 * For any signing related functionality to be included, this define or @ref MY_SIGNING_SOFT has to be enabled.
 */
//#define MY_SIGNING_ATSHA204

/**
 * @def MY_SIGNING_SOFT
 * @brief Enables SW backed signing functionality in library.
 *
 * For any signing related functionality to be included, this define or @ref MY_SIGNING_ATSHA204 has to be enabled.
 */
//#define MY_SIGNING_SOFT

/**
 * @def MY_SIGNING_REQUEST_SIGNATURES
 * @brief Enable this to inform gateway to sign all messages sent to this node.
 *
 * If used for a gateway, gateway will only request signatures from nodes that in turn
 * request signatures from gateway.
 */
//#define MY_SIGNING_REQUEST_SIGNATURES

/**
 * @def MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL
 * @brief Enable this to have gateway require all nodes in the network to sign messages sent to it.
 * @ref MY_SIGNING_REQUEST_SIGNATURES must also be set.
 *
 * Use this for maximum security, but be aware that every single node will have to be personalized before they can be used.
 * Note that if this is enabled, and whitelisting is also enabled, whitelisting will also be in effect for all nodes.
 */
//#define MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL

/**
 * @def MY_VERIFICATION_TIMEOUT_MS
 * @brief Define a suitable timeout for a signature verification session
 *
 * Consider the turnaround from a nonce being generated to a signed message being received
 * which might vary, especially in networks with many hops. 5s ought to be enough for anyone.
 */
#ifndef MY_VERIFICATION_TIMEOUT_MS
#define MY_VERIFICATION_TIMEOUT_MS		(5*1000ul)
#endif

/**
 * @def MY_SIGNING_NODE_WHITELISTING
 * @brief Enable to turn on whitelisting
 *
 * When enabled, a signing node will salt the signature with it's unique signature and nodeId.<br>
 * The verifying node will look up the sender in a local table of trusted nodes and
 * do the corresponding salting in order to verify the signature.<br>
 * For this reason, if whitelisting is enabled on one of the nodes in a sign-verify pair, both
 * nodes have to implement whitelisting for this to work.<br>
 * Note that a node can still transmit a non-salted message (i.e. have whitelisting disabled)
 * to a node that has whitelisting enabled (assuming the receiver does not have a matching entry
 * for the sender in it's whitelist). The whitelist to use is defined as the value of the flag.
 */
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}}}

/**
 * @def MY_SIGNING_ATSHA204_PIN
 * @brief Atsha204 default pin setting
 *
 * Pin where ATSHA204 is attached
 */
#ifndef MY_SIGNING_ATSHA204_PIN
#define MY_SIGNING_ATSHA204_PIN			(17)
#endif

/**
 * @def MY_SIGNING_SOFT_RANDOMSEED_PIN
 * @brief Pin used for random generation in soft signing
 *
 * Do not connect anything to this when soft signing is enabled
 */
#ifndef MY_SIGNING_SOFT_RANDOMSEED_PIN
#define MY_SIGNING_SOFT_RANDOMSEED_PIN	(7)
#endif

/**********************************
*  RS485 Driver Defaults
***********************************/

/**
 * @def MY_RS485_BAUD_RATE
 * @brief The RS485 BAUD rate.
 */
#ifndef MY_RS485_BAUD_RATE
#define MY_RS485_BAUD_RATE				(9600)
#endif

/**
 * @def MY_RS485_MAX_MESSAGE_LENGTH
 * @brief The maximum message length used for RS485.
 */
#ifndef MY_RS485_MAX_MESSAGE_LENGTH
#define MY_RS485_MAX_MESSAGE_LENGTH		(40)
#endif

/**
 * @def MY_RS485_DE_PIN
 * @brief RS485 driver enable pin.
 */
//#define MY_RS485_DE_PIN				(2)

/**
 * @def MY_RS485_HWSERIAL
 * @brief Enable this if RS485 is connected to a hardware serial port.
 */
//#define MY_RS485_HWSERIAL				Serial1

/**********************************
*  NRF24L01P Driver Defaults
***********************************/

/**
 * @def MY_RF24_ENABLE_ENCRYPTION
 * @brief Enables RF24 encryption (all nodes and gateway must have this enabled, and all must be personalized with the same AES key)
 */
//#define MY_RF24_ENABLE_ENCRYPTION

/**
 * @def MY_DEBUG_VERBOSE_RF24
 * @brief Enable MY_DEBUG_VERBOSE_RF24 flag for verbose debug prints related to the RF24 driver.
 */
//#define MY_DEBUG_VERBOSE_RF24

/**
 * @def MY_RF24_SPI_SPEED
 * @brief MY_RF24_SPI_SPEED to overrule default nRF24L01+ SPI speed, 2MHz - safe for nRF24L01+ clones
 */
#ifndef MY_RF24_SPI_SPEED
#define MY_RF24_SPI_SPEED				(2*1000000ul)
#endif

/**
 * @def MY_RF24_CE_PIN
 * @brief Default RF24 chip enable pin setting. Override in sketch if needed.
 */
#ifndef MY_RF24_CE_PIN
#define MY_RF24_CE_PIN					DEFAULT_RF24_CE_PIN
#endif

/**
 * @def MY_RF24_CS_PIN
 * @brief Default RF24 chip select pin setting. Override in sketch if needed.
 */
#ifndef MY_RF24_CS_PIN
#define MY_RF24_CS_PIN					DEFAULT_RF24_CS_PIN
#endif

/**
 * @def MY_RF24_POWER_PIN
 * @brief NRF24 power pin, uncomment if used
 */
//#define MY_RF24_POWER_PIN				(3)

/**
 * @def MY_RX_MESSAGE_BUFFER_FEATURE
 * @brief This enabled the receiving buffer feature.
 *
 * This feature is currently not supported for RFM69 and RS485, for RF24 MY_RF24_IRQ_PIN has to be defined.
 */
//#define MY_RX_MESSAGE_BUFFER_FEATURE

/**
 * @def MY_RX_MESSAGE_BUFFER_SIZE
 * @brief Declare the amount of incoming messages that can be buffered.
 */
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
#ifndef MY_RX_MESSAGE_BUFFER_SIZE
#define MY_RX_MESSAGE_BUFFER_SIZE		(20)
#endif
#endif

/**
 * @def MY_RF24_PA_LEVEL
 * @brief Default RF24 PA level. Override in sketch if needed.
 *
 * RF24_PA_LOW = -18dBm
 * RF24_PA_MID = -12dBm
 * RF24_PA_HIGH = -6dBm
 * RF24_PA_MAX = 0dBm
 */
#ifndef MY_RF24_PA_LEVEL
#define MY_RF24_PA_LEVEL				RF24_PA_HIGH
#endif

/**
 * @def MY_RF24_CHANNEL
 * @brief RF channel for the sensor net, 0-125.
 * Frequence: 2400 Mhz - 2525 Mhz Channels: 126
 * http://www.mysensors.org/radio/nRF24L01Plus.pdf
 * 0 => 2400 Mhz (RF24 channel 1)
 * 1 => 2401 Mhz (RF24 channel 2)
 * 76 => 2476 Mhz (RF24 channel 77)
 * 83 => 2483 Mhz (RF24 channel 84)
 * 124 => 2524 Mhz (RF24 channel 125)
 * 125 => 2525 Mhz (RF24 channel 126)
 * In some countries there might be limitations, in Germany for example only the range 2400,0 - 2483,5 Mhz is allowed
 * http://www.bundesnetzagentur.de/SharedDocs/Downloads/DE/Sachgebiete/Telekommunikation/Unternehmen_Institutionen/Frequenzen/Allgemeinzuteilungen/2013_10_WLAN_2,4GHz_pdf.pdf
 */
#ifndef MY_RF24_CHANNEL
#define MY_RF24_CHANNEL					(76)
#endif

/**
 * @def MY_RF24_DATARATE
 * @brief RF24 datarate (RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps or RF24_2MBPS for 2Mbps).
 */
#ifndef MY_RF24_DATARATE
#define MY_RF24_DATARATE				RF24_250KBPS
#endif

/**
 * @def MY_RF24_BASE_RADIO_ID
 * @brief RF24 radio network identifier.
 *
 * This acts as base value for sensor nodeId addresses. Change this (or channel) if you have more than one sensor network.
 */
#ifndef MY_RF24_BASE_RADIO_ID
#define MY_RF24_BASE_RADIO_ID			0x00,0xFC,0xE1,0xA8,0xA8
#endif

/**
 * @def MY_RF24_ADDR_WIDTH
 * @brief RF24 address width.
 *
 * This defines the width of the base address.
 */
#ifndef MY_RF24_ADDR_WIDTH
#define MY_RF24_ADDR_WIDTH				(5)
#endif

// Enable SOFTSPI for NRF24L01, useful for the W5100 Ethernet module
//#define MY_SOFTSPI

/**
 * @def MY_SOFT_SPI_SCK_PIN
 * @brief Soft SPI SCK pin.
 */
#ifndef MY_SOFT_SPI_SCK_PIN
#define MY_SOFT_SPI_SCK_PIN				(14)
#endif

/**
 * @def MY_SOFT_SPI_MISO_PIN
 * @brief Soft SPI MISO pin.
 */
#ifndef MY_SOFT_SPI_MISO_PIN
#define MY_SOFT_SPI_MISO_PIN			(16)
#endif

/**
 * @def MY_SOFT_SPI_MOSI_PIN
 * @brief Soft SPI MOSI pin.
 */
#ifndef MY_SOFT_SPI_MOSI_PIN
#define MY_SOFT_SPI_MOSI_PIN			(15)
#endif

/**********************************
*  RFM69 Driver Defaults
***********************************/

/**
 * @def MY_DEBUG_VERBOSE_RFM69
 * @brief Enable MY_DEBUG_VERBOSE_RFM69 flag for verbose debug prints related to the RFM69 driver.
 */
//#define MY_DEBUG_VERBOSE_RFM69

/**
 * @def MY_DEBUG_VERBOSE_RFM69_REGISTERS
 * @brief Print content of RFM69 radio registers.
 */
//#define MY_DEBUG_VERBOSE_RFM69_REGISTERS

/**
 * @def MY_RFM69_NEW_DRIVER
 * @brief Enable MY_RFM69_NEW_DRIVER to enable the refactored RFM69 driver.
 * IMPORTANT: This driver is not compatible with the old (=default) RFM69 driver.
 */
//#define MY_RFM69_NEW_DRIVER

/**
 * @def MY_RFM69_FREQUENCY
 * @brief RFM69 frequency to use: RFM69_433MHZ, RFM69_868MHZ, RFM69_915MHZ, or custom frequency in Hz (new RFM69 driver only).
 *
 * This must match the hardware version of the RFM69 radio.
 * Additional information: https://en.wikipedia.org/wiki/Short_Range_Devices
 */
#ifndef MY_RFM69_FREQUENCY
#define MY_RFM69_FREQUENCY				RFM69_868MHZ
#endif

/**
 * @def MY_IS_RFM69HW
 * @brief Enable this if you're running the RFM69HW model.
 */
//#define MY_IS_RFM69HW

/**
 * @def MY_RFM69HW
 * @brief Set to true if @ref MY_IS_RFM69HW is set.
 */
#ifdef MY_IS_RFM69HW
#define MY_RFM69HW true
#else
#define MY_RFM69HW false
#endif

/**
 * @def MY_RFM69_TX_POWER_DBM
 * @brief Set TX power level, default 5dBm (overriden if ATC mode enabled).
 */
#ifndef MY_RFM69_TX_POWER_DBM
#define MY_RFM69_TX_POWER_DBM			(5)
#endif

/**
 * @def MY_RFM69_ATC_TARGET_RSSI_DBM
 * @brief Target RSSI level (in dBm) for RFM69 ATC mode.
 */
#ifndef MY_RFM69_ATC_TARGET_RSSI_DBM
#define MY_RFM69_ATC_TARGET_RSSI_DBM	(-80)
#endif

/**
 * @def MY_RFM69_ATC_MODE_DISABLED
 * @brief Switch to disable ATC mode of RFM69 driver.
 */
//#define MY_RFM69_ATC_MODE_DISABLED

/**
 * @def MY_RFM69_MAX_POWER_LEVEL_DBM
 * @brief Set max TX power in dBm if local legislation requires
 * 1mW = 0dBm, 10mW = 10dBm, 25mW = 14dBm, 100mW = 20dBm
 * see here:  https://en.wikipedia.org/wiki/Short_Range_Devices
 */
//#define MY_RFM69_MAX_POWER_LEVEL_DBM		(10u)

/**
 * @def MY_RFM69_NETWORKID
 * @brief RFM69 Network ID. Use the same for all nodes that will talk to each other.
 */
#ifndef MY_RFM69_NETWORKID
#define MY_RFM69_NETWORKID				(100)
#endif

/**
 * @def MY_RFM69_RST_PIN
 * @brief RFM69 reset pin, uncomment if used.
 */
//#define MY_RFM69_RST_PIN				(9)

#ifdef MY_RF69_RESET
// legacy, older board files
#define MY_RFM69_RST_PIN				MY_RF69_RESET
#endif

/**
 * @def MY_RFM69_POWER_PIN
 * @brief RFM69 power pin
 */
//#define MY_RFM69_POWER_PIN			(3)

/**
 * @def MY_RFM69_IRQ_PIN
 * @brief RFM69 IRQ pin
 */
#ifndef MY_RFM69_IRQ_PIN
#ifdef MY_RF69_IRQ_PIN
// legacy, older board files
#define MY_RFM69_IRQ_PIN				MY_RF69_IRQ_PIN
#else
#define MY_RFM69_IRQ_PIN				DEFAULT_RFM69_IRQ_PIN
#endif
#endif


/**
 * @def MY_RFM69_IRQ_NUM
 * @brief RFM69 IRQ number.
 */
#ifndef MY_RFM69_IRQ_NUM
#ifdef MY_RF69_IRQ_NUM
// legacy, older board files
#define MY_RFM69_IRQ_NUM				MY_RF69_IRQ_NUM
#else
#define MY_RFM69_IRQ_NUM				DEFAULT_RFM69_IRQ_NUM
#endif
#endif


/**
 * @def MY_RFM69_CS_PIN
 * @brief RFM69 SPI chip select pin.
 */
#ifndef MY_RFM69_CS_PIN
#ifdef MY_RF69_SPI_CS
// legacy, older board files
#define MY_RFM69_CS_PIN					MY_RF69_SPI_CS
#else
#define MY_RFM69_CS_PIN					DEFAULT_RFM69_CS_PIN
#endif
#endif

/**
 * @def MY_RFM69_SPI_SPEED
 * @brief Set to overrule default RFM69 SPI speed.
 */
#ifndef MY_RFM69_SPI_SPEED
#define MY_RFM69_SPI_SPEED				(4*1000000ul)	// datasheet says 10Mhz max.
#endif

/**
 * @def MY_RFM69_ENABLE_ENCRYPTION
 * Enables RFM69 encryption (all nodes and gateway must have this enabled, and all must be personalized with the same AES key)
 */
//#define MY_RFM69_ENABLE_ENCRYPTION

/**
 * Uncomment if you need listenmode, else comment it and save memory
 */
//#define MY_RFM69_ENABLE_LISTENMODE

#if defined(MY_RFM69_ENABLE_LISTENMODE) && !defined(MY_RFM69_DEFAULT_LISTEN_RX_US)
// By default, receive for 256uS in listen mode and idle for ~1s
#define MY_RFM69_DEFAULT_LISTEN_RX_US	(256)
#endif

#if defined(MY_RFM69_ENABLE_LISTENMODE) && !defined(MY_RFM69_DEFAULT_LISTEN_IDLE_US)
// By default, receive for 256uS in listen mode and idle for ~1s
#define  MY_RFM69_DEFAULT_LISTEN_IDLE_US (1*1000000ul)
#endif


/**
 * @def MY_RFM69_BITRATE_MSB
 * The bit rate. Most significant bits. Bitrate between the transmitter and the receiver must be better than 6.5. Refer to RFM69registers.h (L.153) for settings or http://www.semtech.com/apps/filedown/down.php?file=sx1231.pdf
 * @brief Datarate value. Note : RFM69_FOSC(Hz)/MSB_LSBVALUE = Bitrate in kbits
 *
 * @def MY_RFM69_BITRATE_LSB
 * The bit rate. Less significant bits. Bitrate between the transmitter and the receiver must be better than 6.5. Refer to RFM69registers.h (L.153) for settings or http://www.semtech.com/apps/filedown/down.php?file=sx1231.pdf
 * @brief Datarate value. Note : RFM69_FOSC(Hz)/MSB_LSBVALUE = Bitrate in kbits
 */
#if !defined(MY_RFM69_BITRATE_MSB) && !defined(MY_RFM69_BITRATE_LSB)
#define MY_RFM69_BITRATE_MSB			RFM69_BITRATEMSB_55555
#define MY_RFM69_BITRATE_LSB			RFM69_BITRATELSB_55555
#endif

/**********************************
*  RFM95 driver defaults
***********************************/

/**
 * @def MY_DEBUG_VERBOSE_RFM95
 * @brief Enable MY_DEBUG_VERBOSE_RFM95 flag for verbose debug prints related to the RFM95 driver.
 */
//#define MY_DEBUG_VERBOSE_RFM95

/**
 * @def MY_RFM95_FREQUENCY
 * @brief RFM95 frequency to use: RFM95_169MHZ, RFM95_315MHZ, RFM95_434MHZ, RFM95_868MHZ, RFM95_915MHZ, or custom frequency in Hz.
 *
 * This must match the hardware version of the RFM95 radio.
 * Additional information: https://en.wikipedia.org/wiki/Short_Range_Devices

 */
#ifndef MY_RFM95_FREQUENCY
#define MY_RFM95_FREQUENCY				RFM95_868MHZ
#endif

/**
 * @def MY_RFM95_MODEM_CONFIGRUATION
 * @brief RFM95 modem configuration, see table. Default RFM95 modem configuration: @ref RFM95_BW125CR45SF128
 *
 * BW = Bandwidth in kHz
 * CR = Error correction code
 * SF = Spreading factor, chips / symbol
 *
 * | CONFIG					| BW    | CR  | SF   | Comment
 * |------------------------|-------|-----|------|-----------------------------
 * | RFM95_BW125CR45SF128	| 125   | 4/5 | 128  | Default, medium range
 * | RFM95_BW500CR45SF128	| 500   | 4/5 | 128  | Fast, short range
 * | RFM95_BW31_25CR48SF512	| 31.25 | 4/8 | 512  | Slow, long range
 * | RFM95_BW125CR48SF4096	| 125   | 4/8 | 4096 | Slow, long range
 *
 */
#ifndef MY_RFM95_MODEM_CONFIGRUATION
#define MY_RFM95_MODEM_CONFIGRUATION	RFM95_BW125CR45SF128
#endif

/**
 * @def MY_RFM95_RST_PIN
 * @brief RFM95 reset pin, uncomment if used.
 */
//#define MY_RFM95_RST_PIN				(9)

/**
 * @def MY_RFM95_POWER_PIN
 * @brief RFM95 power pin, uncomment if used
 */
//#define MY_RFM95_POWER_PIN			(3)

/**
 * @def MY_RFM95_IRQ_PIN
 * @brief RFM95 IRQ pin.
 */
#ifndef MY_RFM95_IRQ_PIN
#define MY_RFM95_IRQ_PIN				DEFAULT_RFM95_IRQ_PIN
#endif

/**
 * @def MY_RFM95_IRQ_NUM
 * @brief RFM95 IRQ number.
 */
#ifndef MY_RFM95_IRQ_NUM
#define MY_RFM95_IRQ_NUM				DEFAULT_RFM95_IRQ_NUM
#endif

/**
 * @def MY_RFM95_CS_PIN
 * @brief RFM95 SPI chip select pin.
 */
#ifndef MY_RFM95_CS_PIN
#define MY_RFM95_CS_PIN					DEFAULT_RFM95_CS_PIN
#endif

/**
 * @def MY_RFM95_SPI_SPEED
 * @brief Set to overrule default RFM95 SPI speed, default 4Mhz.
 */
#ifndef MY_RFM95_SPI_SPEED
#define MY_RFM95_SPI_SPEED				(4*1000000ul)
#endif

/**
 * @def MY_RFM95_TX_POWER_DBM
 * @brief Set TX power level, default 13dBm (overriden if ATC mode enabled)
 * see here: https://en.wikipedia.org/wiki/Short_Range_Devices
 */
#ifndef MY_RFM95_TX_POWER_DBM
#define MY_RFM95_TX_POWER_DBM			(13u)	// 20mW
#endif

/**
 * @def MY_RFM95_ATC_MODE_DISABLED
 * @brief Switch to disable RFM95 ATC mode
 */
//#define MY_RFM95_ATC_MODE_DISABLED

/**
 * @def MY_RFM95_ATC_TARGET_RSSI
 * @brief Target RSSI level (in dBm) for RFM95 ATC mode
 */
#ifndef MY_RFM95_ATC_TARGET_RSSI
#define MY_RFM95_ATC_TARGET_RSSI		(-60)
#endif

/**
 * @def MY_RFM95_MAX_POWER_LEVEL_DBM
 * @brief Set max TX power in dBm if local legislation requires
 * 1mW = 0dBm, 10mW = 10dBm, 25mW = 14dBm, 100mW = 20dBm
 * see here: https://en.wikipedia.org/wiki/Short_Range_Devices
 */
//#define MY_RFM95_MAX_POWER_LEVEL_DBM		(10u)

/**
 * @def MY_RFM95_TCXO
 * @brief Enable to force your radio to use an external frequency source (e.g. TCXO, if present).
 * This allows for better stability using SF 9 to 12.
 */
//#define MY_RFM95_TCXO

/**************************************
* Signal report
***************************************/

/**
 * @def MY_SIGNAL_REPORT_ENABLED
 * @brief Enables signal report functionality, see @ref transportSignalReport().
 * NOTE: This feature adds ~1kB code to the sketch.
 */
#define MY_SIGNAL_REPORT_ENABLED


/**************************************
* Ethernet Gateway Transport  Defaults
***************************************/

// The gateway options available
//#define MY_GATEWAY_W5100
//#define MY_GATEWAY_ENC28J60
//#define MY_GATEWAY_ESP8266
//#define MY_GATEWAY_LINUX

/**
 * @def MY_PORT
 * @brief The Ethernet TCP/UDP port to open on controller or gateway.
 */
#ifndef MY_PORT
#ifdef MY_GATEWAY_MQTT_CLIENT
#define MY_PORT 1883
#else
#define MY_PORT 5003
#endif
#endif

/**
 * @def MY_MQTT_CLIENT_PUBLISH_RETAIN
 * @brief Enables MQTT client to set the retain flag when publishing specific messages.
 */
//#define MY_MQTT_CLIENT_PUBLISH_RETAIN

// Static ip address of gateway (if this is disabled, DHCP will be used)
//#define MY_IP_ADDRESS 192,168,178,66

// Enables UDP mode for Ethernet gateway (W5100)
//#define MY_USE_UDP

/**
 * @def MY_IP_RENEWAL_INTERVAL_MS
 * @brief DHCP, default renewal setting in milliseconds.
 */
#ifndef MY_IP_RENEWAL_INTERVAL_MS
#define MY_IP_RENEWAL_INTERVAL_MS		(60*1000ul)
#endif

/**
 * @def MY_MAC_ADDRESS
 * @brief Ethernet MAC address.
 *
 * This needs to be unique on the network.
 */
#ifndef MY_MAC_ADDRESS
#define MY_MAC_ADDRESS					0xDE,0xAD,0xBE,0xEF,0xFE,0xED
#endif

// Controller ip-address, if this is defined, gateway will act as a client trying to contact controller on MY_PORT.
// If MY_CONTROLLER_IP_ADDRESS is left un-defined, gateway acts as server allowing incoming connections.
//#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 254

/**************************************
* Node Locking
***************************************/

/**
 * @defgroup MyLockgrp MyNodeLock
 * @ingroup internals
 * @{
 * @brief The node lock feature is a security related feature. It locks a node that suspect itself for being
 * under some form of attack.
 *
 * This is achieved by having a counter stored in EEPROM which decrements when suspicious activity is detected.
 * If the counter reaches 0, node will not work anymore and will transmit a @ref I_LOCKED message to the
 * gateway/controller with 30m intervals. Payload is a string with a reason for the locking.
 * The string is abbreviated to accomodate a signature. The following abbreviations exist at the moment:
 * - LDB (Locked During Boot)
 * - TMNR (Too Many Nonce Requests)
 * - TMFV (Too Many Failed Verifications)
 *
 * Typically, the counter only decrements when suspicious activity happens in a row.
 * It is reset if legit traffic is present.

 * Examples of malicious activity are:
 * - Repeatedly incorrectly checksummed OTA firmware
 * - Repeated requests for signing nonces without properly signed messages arriving
 * - Repeatedly failed signature verifications
 *
 * If counter reaches zero, node locks down and EEPROM has to be erased/reset to reactivate node.
 * Node can also be unlocked by grounding a pin (see @ref MY_NODE_UNLOCK_PIN).
 *
 * The size of the counter can be adjusted using @ref MY_NODE_LOCK_COUNTER_MAX.
 *
 * @def MY_NODE_LOCK_FEATURE
 * @brief Enable this to activate intrusion prevention mechanisms on the node.
 */
//#define MY_NODE_LOCK_FEATURE

/**
 * @def MY_NODE_UNLOCK_PIN
 * @brief By grounding this pin durig reset of a locked node, the node will unlock.
 *
 * If using a secure bootloader, grounding the pin is the only option to reactivate the node.
 * If using stock Android bootloader or a DualOptiBoot it is also possible to download a sketch
 * using serial protocol to erase EEPROM to unlock the node.
 */
#ifndef MY_NODE_UNLOCK_PIN
#define MY_NODE_UNLOCK_PIN				(14)
#endif

/**
 * @def MY_NODE_LOCK_COUNTER_MAX
 * @brief Maximum accepted occurances of suspected malicious activity in a node.
 *
 * Counter decrements on reoccuring incidents but resets if legitimate behaviour is identified.
 */
#ifndef MY_NODE_LOCK_COUNTER_MAX
#define MY_NODE_LOCK_COUNTER_MAX		(5)
#endif
/** @}*/ // Node lock group

/**********************************
*  ESP8266 Defaults
***********************************/

/**
 * @def MY_ESP8266_SERIAL_MODE
 * @brief Serial modes: SERIAL_FULL, SERIAL_RX_ONLY, SERIAL_TX_ONLY
 *
 * SERIAL_FULL: Default mode.
 * SERIAL_TX_ONLY: allows to use RX (GPIO3) as a general purpose input/output.
 * SERIAL_RX_ONLY: allows to use TX (GPIO1) as a general purpose input/output.
 */
#ifndef MY_ESP8266_SERIAL_MODE
#define MY_ESP8266_SERIAL_MODE			SERIAL_FULL
#endif

/**************************************
* Linux Settings
***************************************/

/**
 * @def MY_LINUX_SERIAL_PORT
 * @brief Serial device port
 */
#ifndef MY_LINUX_SERIAL_PORT
#define MY_LINUX_SERIAL_PORT			"/dev/ttyACM0"
#endif

/**
 * @def MY_LINUX_IS_SERIAL_PTY
 * @brief Set serial as a pseudo terminal.
 *
 * Enable this if you need to connect to a controller running on the same device.
 */
//#define MY_LINUX_IS_SERIAL_PTY

/**
 * @def MY_LINUX_SERIAL_PTY
 * @brief Symlink name for the PTY device.
 */
#ifndef MY_LINUX_SERIAL_PTY
#define MY_LINUX_SERIAL_PTY				"/dev/ttyMySensorsGateway"
#endif

/**
 * @def MY_LINUX_SERIAL_GROUPNAME
 * @brief Grant access to the specified system group for the serial device.
 */
//#define MY_LINUX_SERIAL_GROUPNAME		"tty"

/**
 * @def MY_LINUX_CONFIG_FILE
 * @brief Set the filepath for the gateway config file
 *
 * For now the configuration file is only used to store the emulated eeprom state
 */
#ifndef MY_LINUX_CONFIG_FILE
#define MY_LINUX_CONFIG_FILE			"/etc/mysensors.dat"
#endif

#endif	// MyConfig_h

// Doxygen specific constructs, not included when built normally
// This is used to enable disabled macros/definitions to be included in the documentation as well.
#if DOXYGEN
// debug
#define MY_DEBUG
#define MY_SPECIAL_DEBUG
#define MY_DISABLED_SERIAL
// linux
#define MY_LINUX_SERIAL_GROUPNAME
#define MY_LINUX_IS_SERIAL_PTY
// inclusion mode
#define MY_INCLUSION_MODE_FEATURE
#define MY_INCLUSION_BUTTON_FEATURE
// transport
#define MY_PARENT_NODE_IS_STATIC
#define MY_REGISTRATION_CONTROLLER
#define MY_TRANSPORT_UPLINK_CHECK_DISABLED
#define MY_TRANSPORT_SANITY_CHECK
#define MY_NODE_LOCK_FEATURE
#define MY_REPEATER_FEATURE
#define MY_PASSIVE_NODE
#define MY_MQTT_CLIENT_PUBLISH_RETAIN
// general
#define MY_WITH_LEDS_BLINKING_INVERSE
#define MY_INDICATION_HANDLER
#define MY_DISABLE_REMOTE_RESET
// signing
#define MY_SIGNING_ATSHA204
#define MY_SIGNING_SOFT
#define MY_SIGNING_REQUEST_SIGNATURES
#define MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL
#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}}}
#define MY_DEBUG_VERBOSE_SIGNING
// RS485
#define MY_RS485_HWSERIAL
// RF24
#define MY_DEBUG_VERBOSE_RF24
#define MY_RF24_POWER_PIN
#define MY_RF24_ENABLE_ENCRYPTION
#define MY_RX_MESSAGE_BUFFER_FEATURE
#define MY_RX_MESSAGE_BUFFER_SIZE
// RFM69
#define MY_IS_RFM69HW
#define MY_RFM69_NEW_DRIVER
#define MY_RFM69_POWER_PIN
#define MY_RFM69_ENABLE_ENCRYPTION
#define MY_RFM69_ATC_MODE_DISABLED
#define MY_RFM69_MAX_POWER_LEVEL_DBM
#define MY_RFM69_RST_PIN
#define MY_DEBUG_VERBOSE_RFM69
#define MY_DEBUG_VERBOSE_RFM69_REGISTERS
// RFM95
#define MY_DEBUG_VERBOSE_RFM95
#define MY_RFM95_ATC_MODE_DISABLED
#define MY_RFM95_RST_PIN
#define MY_RFM95_MODEM_CONFIGRUATION
#define MY_RFM95_POWER_PIN
#define MY_RFM95_TCXO
#define MY_RFM95_MAX_POWER_LEVEL_DBM

#endif
