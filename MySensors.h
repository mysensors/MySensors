/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
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
 * @file MySensors.h
 *
 * MySensors main interface (includes all necessary code for the library)
 */
#ifndef MySensors_h
#define MySensors_h

#ifdef __cplusplus
#include <Arduino.h>
#endif

#include "MyConfig.h"
#include "core/MySensorsCore.h"

// Detect node type
/**
 * @def MY_GATEWAY_FEATURE
 * @brief Is set for gateway sketches.
 */
/**
 * @def MY_IS_GATEWAY
 * @brief Is true when @ref MY_GATEWAY_FEATURE is set.
 */
/**
 * @def MY_NODE_TYPE
 * @brief Contain a string describing the class of sketch/node (gateway/repeater/node).
 */

#if defined(MY_GATEWAY_SERIAL) || defined(MY_GATEWAY_W5100) || defined(MY_GATEWAY_ENC28J60) || defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_LINUX) || defined(MY_GATEWAY_MQTT_CLIENT)
#define MY_GATEWAY_FEATURE
#define MY_IS_GATEWAY (true)
#define MY_NODE_TYPE "GW"
#elif defined(MY_REPEATER_FEATURE)
#define MY_IS_GATEWAY (false)
#define MY_NODE_TYPE "REPEATER"
#else
#define MY_IS_GATEWAY (false)
#define MY_NODE_TYPE "NODE"
#endif

// DEBUG
#if defined(MY_DEBUG)
// standard debug output
#define MY_DEBUG_VERBOSE_CORE	//!< MY_DEBUG_VERBOSE_CORE
#define MY_DEBUG_VERBOSE_TRANSPORT	//!< MY_DEBUG_VERBOSE_TRANSPORT
#define MY_DEBUG_VERBOSE_OTA_UPDATE	//!< MY_DEBUG_VERBOSE_OTA_UPDATE
#endif

#if defined(MY_DEBUG) || defined(MY_DEBUG_VERBOSE_CORE) || defined(MY_DEBUG_VERBOSE_TRANSPORT) || defined(MY_DEBUG_VERBOSE_SIGNING) || defined(MY_DEBUG_VERBOSE_OTA_UPDATE) || defined(MY_DEBUG_VERBOSE_RF24) || defined(MY_DEBUG_VERBOSE_NRF5_ESB) || defined(MY_DEBUG_VERBOSE_RFM69) || defined(MY_DEBUG_VERBOSE_RFM95)
#define DEBUG_OUTPUT_ENABLED	//!< DEBUG_OUTPUT_ENABLED
#define DEBUG_OUTPUT(x,...)		hwDebugPrint(x, ##__VA_ARGS__)	//!< debug
#else
#define DEBUG_OUTPUT(x,...)								//!< debug NULL
#endif

// transport layer files
#define debug(x,...)			DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug

// temp. workaround for nRF5 verifier: redirect RF24 to NRF_ESB
#if defined(ARDUINO_ARCH_NRF5) && (defined(MY_RADIO_RF24) || defined(MY_RADIO_NRF24) )
#undef MY_RADIO_RF24
#undef MY_RADIO_NRF24
#define MY_RADIO_NRF5_ESB
#endif

// Enable sensor network "feature" if one of the transport types was enabled
#if defined(MY_RADIO_RF24) || defined(MY_RADIO_NRF5_ESB) || defined(MY_RADIO_RFM69) || defined(MY_RADIO_RFM95) || defined(MY_RS485)
#define MY_SENSOR_NETWORK
#endif

// HARDWARE
#if defined(ARDUINO_ARCH_ESP8266)
#include "hal/architecture/MyHwESP8266.cpp"
#elif defined(ARDUINO_ARCH_AVR)
#include "drivers/AVR/DigitalWriteFast/digitalWriteFast.h"
#include "hal/architecture/MyHwAVR.cpp"
#elif defined(ARDUINO_ARCH_SAMD)
#include "drivers/extEEPROM/extEEPROM.cpp"
#include "hal/architecture/MyHwSAMD.cpp"
#elif defined(ARDUINO_ARCH_STM32F1)
#include "hal/architecture/MyHwSTM32F1.cpp"
#elif defined(ARDUINO_ARCH_NRF5)
#include "drivers/NVM/VirtualPage.cpp"
#include "drivers/NVM/NVRAM.cpp"
#include "hal/architecture/MyHwNRF5.cpp"
#elif defined(__arm__) && defined(TEENSYDUINO)
#include "hal/architecture/MyHwTeensy3.cpp"
#elif defined(__linux__)
#include "hal/architecture/MyHwLinuxGeneric.cpp"
#endif

// LEDS
#if !defined(MY_DEFAULT_ERR_LED_PIN) && defined(MY_HW_ERR_LED_PIN)
#define MY_DEFAULT_ERR_LED_PIN MY_HW_ERR_LED_PIN
#endif

#if !defined(MY_DEFAULT_TX_LED_PIN) && defined(MY_HW_TX_LED_PIN)
#define MY_DEFAULT_TX_LED_PIN MY_HW_TX_LED_PIN
#endif

#if !defined(MY_DEFAULT_RX_LED_PIN) && defined(MY_HW_TX_LED_PIN)
#define MY_DEFAULT_RX_LED_PIN MY_HW_TX_LED_PIN
#endif

#if defined(MY_LEDS_BLINKING_FEATURE)
#error MY_LEDS_BLINKING_FEATURE is now removed from MySensors core,\
define MY_DEFAULT_ERR_LED_PIN, MY_DEFAULT_TX_LED_PIN or\
MY_DEFAULT_RX_LED_PIN in your sketch instead to enable LEDs
#endif

#if defined(MY_DEFAULT_RX_LED_PIN) || defined(MY_DEFAULT_TX_LED_PIN) || defined(MY_DEFAULT_ERR_LED_PIN)
#include "core/MyLeds.cpp"
#else
#include "core/MyLeds.h"
#endif

#include "core/MyIndication.cpp"


// INCLUSION MODE
#if defined(MY_INCLUSION_MODE_FEATURE)
#include "core/MyInclusionMode.cpp"
#endif


// SIGNING
#if defined(MY_SIGNING_ATSHA204) || defined(MY_SIGNING_SOFT)
#define MY_SIGNING_FEATURE	//!< MY_SIGNING_FEATURE
#endif
#include "core/MySigning.cpp"
#include "drivers/ATSHA204/sha256.cpp"
#if defined(MY_SIGNING_FEATURE)
// SIGNING COMMON FUNCTIONS
#if defined(MY_SIGNING_ATSHA204) && defined(MY_SIGNING_SOFT)
#error Only one signing engine can be activated
#endif
#if defined(MY_SIGNING_ATSHA204) && defined(__linux__)
#error No support for ATSHA204 on this platform
#endif

#if defined(MY_SIGNING_ATSHA204)
#include "core/MySigningAtsha204.cpp"
#include "drivers/ATSHA204/ATSHA204.cpp"
#elif defined(MY_SIGNING_SOFT)
#include "core/MySigningAtsha204Soft.cpp"
#endif
#endif


// FLASH
#if defined(MY_OTA_FIRMWARE_FEATURE)
#include "drivers/SPIFlash/SPIFlash.cpp"
#include "core/MyOTAFirmwareUpdate.cpp"
#endif

// GATEWAY - TRANSPORT
#if defined(MY_CONTROLLER_IP_ADDRESS) || defined(MY_CONTROLLER_URL_ADDRESS)
#define MY_GATEWAY_CLIENT_MODE
#endif

#if defined(MY_USE_UDP) && !defined(MY_GATEWAY_CLIENT_MODE)
#error You must specify MY_CONTROLLER_IP_ADDRESS or MY_CONTROLLER_URL_ADDRESS for UDP
#endif

#if defined(MY_GATEWAY_MQTT_CLIENT)
#if defined(MY_SENSOR_NETWORK)
// We assume that a gateway having a radio also should act as repeater
#define MY_REPEATER_FEATURE

#endif
// GATEWAY - COMMON FUNCTIONS
// We support MQTT Client using W5100, ESP8266 and Linux
#if !defined(MY_GATEWAY_CLIENT_MODE)
#error You must specify MY_CONTROLLER_IP_ADDRESS or MY_CONTROLLER_URL_ADDRESS
#endif

#if !defined(MY_MQTT_PUBLISH_TOPIC_PREFIX)
#error You must specify a topic publish prefix MY_MQTT_PUBLISH_TOPIC_PREFIX for this MQTT client
#endif

#if !defined(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX)
#error You must specify a topic subscribe prefix MY_MQTT_SUBSCRIBE_TOPIC_PREFIX for this MQTT client
#endif

#if !defined(MY_MQTT_CLIENT_ID)
#error You must define a unique MY_MQTT_CLIENT_ID for this MQTT client
#endif

#include "core/MyGatewayTransport.cpp"
#include "core/MyProtocolMySensors.cpp"

#if defined(MY_GATEWAY_LINUX)
#include "drivers/Linux/EthernetClient.h"
#include "drivers/Linux/EthernetServer.h"
#include "drivers/Linux/IPAddress.h"
#endif
#include "drivers/PubSubClient/PubSubClient.cpp"
#include "core/MyGatewayTransportMQTTClient.cpp"
#elif defined(MY_GATEWAY_FEATURE)
// GATEWAY - COMMON FUNCTIONS
#include "core/MyGatewayTransport.cpp"

#include "core/MyProtocolMySensors.cpp"

// GATEWAY - CONFIGURATION
#if defined(MY_SENSOR_NETWORK)
// We assume that a gateway having a radio also should act as repeater
#define MY_REPEATER_FEATURE
#endif
#if !defined(MY_PORT)
#error You must define MY_PORT (controller or gatway port to open)
#endif
#if defined(MY_GATEWAY_ESP8266)
// GATEWAY - ESP8266
#include "core/MyGatewayTransportEthernet.cpp"
#elif defined(MY_GATEWAY_LINUX)
// GATEWAY - Generic Linux
#if defined(MY_USE_UDP)
#error UDP mode is not available for Linux
#endif
#include "drivers/Linux/EthernetClient.h"
#include "drivers/Linux/EthernetServer.h"
#include "drivers/Linux/IPAddress.h"
#include "core/MyGatewayTransportEthernet.cpp"
#elif defined(MY_GATEWAY_W5100)
// GATEWAY - W5100
#include "core/MyGatewayTransportEthernet.cpp"
#elif defined(MY_GATEWAY_ENC28J60)
// GATEWAY - ENC28J60
#if defined(MY_USE_UDP)
#error UDP mode is not available for ENC28J60
#endif
#include "core/MyGatewayTransportEthernet.cpp"
#elif defined(MY_GATEWAY_SERIAL)
// GATEWAY - SERIAL
#include "core/MyGatewayTransportSerial.cpp"
#endif
#endif

// TRANSPORT
// count enabled transports
#if defined(MY_RADIO_RF24)
#define __RF24CNT 1		//!< __RF24CNT
#else
#define __RF24CNT 0		//!< __RF24CNT
#endif
#if defined(MY_RADIO_NRF5_ESB)
#define __NRF5ESBCNT 1 //!< __NRF5ESBCNT
#else
#define __NRF5ESBCNT 0 //!< __NRF5ESBCNT
#endif
#if defined(MY_RADIO_RFM69)
#define __RFM69CNT 1	//!< __RFM69CNT
#else
#define __RFM69CNT 0	//!< __RFM69CNT
#endif
#if defined(MY_RADIO_RFM95)
#define __RFM95CNT 1	//!< __RFM95CNT
#else
#define __RFM95CNT 0	//!< __RFM95CNT
#endif
#if defined(MY_RS485)
#define __RS485CNT 1	//!< __RS485CNT
#else
#define __RS485CNT 0	//!< __RS485CNT
#endif

#if (__RF24CNT + __NRF5ESBCNT + __RFM69CNT + __RFM95CNT + __RS485CNT > 1)
#error Only one forward link driver can be activated
#endif

// TRANSPORT INCLUDES
#if defined(MY_RADIO_RF24) || defined(MY_RADIO_NRF5_ESB) || defined(MY_RADIO_RFM69) || defined(MY_RADIO_RFM95) || defined(MY_RS485)
#include "hal/transport/MyTransportHAL.h"
#include "core/MyTransport.h"


// SANITY CHECK FEATURE
#if defined(MY_REPEATER_FEATURE)
#define MY_TRANSPORT_SANITY_CHECK
#endif

// PARENT CHECK
#if defined(MY_PARENT_NODE_IS_STATIC) && (MY_PARENT_NODE_ID == AUTO)
#error Parent is static but no parent ID defined, set MY_PARENT_NODE_ID.
#endif

#if defined(MY_TRANSPORT_DONT_CARE_MODE)
#error This directive is deprecated, set MY_TRANSPORT_WAIT_READY_MS instead!
#endif

// RAM ROUTING TABLE
#if defined(MY_RAM_ROUTING_TABLE_FEATURE) && defined(MY_REPEATER_FEATURE)
// activate feature based on architecture
#if defined(ARDUINO_ARCH_ESP8266) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_NRF5) || defined(__linux__)
#define MY_RAM_ROUTING_TABLE_ENABLED
#elif defined(ARDUINO_ARCH_AVR)
#if defined(__avr_atmega1280__) || defined(__avr_atmega1284__) || defined(__avr_atmega2560__)
// >4kb, enable it
#define MY_RAM_ROUTING_TABLE_ENABLED
#else
// memory limited, enable with care
// #define MY_RAM_ROUTING_TABLE_ENABLED
#endif // __avr_atmega1280__, __avr_atmega1284__, __avr_atmega2560__
#endif // ARDUINO_ARCH_AVR
#endif

// SOFTSPI
#ifdef MY_SOFTSPI
#if defined(ARDUINO_ARCH_ESP8266)
#error Soft SPI is not available on ESP8266
#endif
#include "drivers/AVR/DigitalIO/DigitalIO.h"
#endif

// POWER PIN
#if defined(MY_RF24_POWER_PIN) || defined(MY_RFM69_POWER_PIN) || defined(MY_RFM95_POWER_PIN)
#define RADIO_CAN_POWER_OFF (true)
#else
#define RADIO_CAN_POWER_OFF (false)
#endif

// Transport drivers
#if defined(MY_RADIO_RF24)
#if defined(MY_RF24_ENABLE_ENCRYPTION)
#include "drivers/AES/AES.cpp"
#endif
#include "drivers/RF24/RF24.cpp"
#include "hal/transport/MyTransportRF24.cpp"
#elif defined(MY_RADIO_NRF5_ESB)
#if  !defined(ARDUINO_ARCH_NRF5)
#error No support for nRF5 radio on this platform
#endif
#if defined(MY_NRF5_ESB_ENABLE_ENCRYPTION)
#include "drivers/AES/AES.cpp"
#endif
#include "drivers/NRF5/Radio.cpp"
#include "drivers/NRF5/Radio_ESB.cpp"
#include "hal/transport/MyTransportNRF5_ESB.cpp"
#elif defined(MY_RS485)
#if !defined(MY_RS485_HWSERIAL)
#if defined(__linux__)
#error You must specify MY_RS485_HWSERIAL for RS485 transport
#endif
#include "drivers/AltSoftSerial/AltSoftSerial.cpp"
#endif
#include "hal/transport/MyTransportRS485.cpp"
#elif defined(MY_RADIO_RFM69)
#if defined(MY_RFM69_NEW_DRIVER)
#include "drivers/RFM69/new/RFM69_new.cpp"
#else
#include "drivers/RFM69/old/RFM69_old.cpp"
#endif
#include "hal/transport/MyTransportRFM69.cpp"
#elif defined(MY_RADIO_RFM95)
#if defined(MY_RFM95_ENABLE_ENCRYPTION)
#include "drivers/AES/AES.cpp"
#endif
#include "drivers/RFM95/RFM95.cpp"
#include "hal/transport/MyTransportRFM95.cpp"
#endif

// PASSIVE MODE
#if defined(MY_PASSIVE_NODE)
#define MY_TRANSPORT_UPLINK_CHECK_DISABLED
#define MY_PARENT_NODE_IS_STATIC // prevents searching new parent
#undef MY_REGISTRATION_FEATURE
#undef MY_SIGNING_FEATURE
#undef MY_OTA_FIRMWARE_FEATURE
#if (defined(MY_GATEWAY_FEATURE) || defined(MY_REPEATER_FEATURE))
#error This node is configured as GW/repeater, MY_PASSIVE_NODE cannot be set simultaneously
#endif
#if (MY_NODE_ID == AUTO)
#error MY_PASSIVE_NODE configuration requires setting MY_NODE_ID
#endif
#endif

#include "core/MyTransport.cpp"
#endif

// Make sure to disable child features when parent feature is disabled
#if !defined(MY_SENSOR_NETWORK)
#undef MY_OTA_FIRMWARE_FEATURE
#undef MY_REPEATER_FEATURE
#undef MY_SIGNING_NODE_WHITELISTING
#undef MY_SIGNING_FEATURE
#endif

#if !defined(MY_GATEWAY_FEATURE)
#undef MY_INCLUSION_MODE_FEATURE
#undef MY_INCLUSION_BUTTON_FEATURE
#endif

#if !defined(MY_CORE_ONLY)
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_SENSOR_NETWORK)
#error No forward link or gateway feature activated. This means nowhere to send messages! Pretty pointless.
#endif
#endif

#include "core/MyCapabilities.h"
#include "core/MyMessage.cpp"
#include "core/MySensorsCore.cpp"

// HW mains
#if !defined(MY_CORE_ONLY)
#if defined(ARDUINO_ARCH_ESP8266)
#include "hal/architecture/MyMainESP8266.cpp"
#elif defined(ARDUINO_ARCH_NRF5)
#include "hal/architecture/MyMainNRF5.cpp"
#elif defined(__linux__)
#include "hal/architecture/MyMainLinux.cpp"
#elif defined(ARDUINO_ARCH_STM32F1)
#include "hal/architecture/MyMainSTM32F1.cpp"
#elif defined(TEENSYDUINO)
#include "hal/architecture/MyMainTeensy3.cpp"
#else
#include "hal/architecture/MyMainDefault.cpp"
#endif
#endif

#endif

// Doxygen specific constructs, not included when built normally
// This is used to enable disabled macros/definitions to be included in the documentation as well.
#if DOXYGEN
#define MY_GATEWAY_FEATURE
#define MY_LEDS_BLINKING_FEATURE //!< \deprecated use MY_DEFAULT_RX_LED_PIN, MY_DEFAULT_TX_LED_PIN and/or MY_DEFAULT_ERR_LED_PIN instead **** DEPRECATED, DO NOT USE ****
#endif
