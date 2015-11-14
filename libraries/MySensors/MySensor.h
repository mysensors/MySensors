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
#ifndef MySensor_h
#define MySensor_h

#include "MyConfig.h"
#include "core/MySensorCore.h"

// Detect node type
#if defined(MY_GATEWAY_SERIAL) || defined(MY_GATEWAY_W5100) || defined(MY_GATEWAY_ENC28J60) || defined(ARDUINO_ARCH_ESP8266) || defined(MY_GATEWAY_MQTT_CLIENT)
	#define MY_GATEWAY_FEATURE
	#define MY_IS_GATEWAY (true)
	#define MY_NODE_TYPE "gateway"
#elif defined(MY_REPEATER_FEATURE)
	#define MY_IS_GATEWAY (false)
	#define MY_NODE_TYPE "repeater"
#else
	#define MY_IS_GATEWAY (false)
	#define MY_NODE_TYPE "sensor"
#endif

// Enable radio "feature" if one of the radio types was enabled
#if defined(MY_RADIO_NRF24) || defined(MY_RADIO_RFM69) || defined(MY_RS485)
	#define MY_RADIO_FEATURE
#endif

// HARDWARE
#if defined(ARDUINO_ARCH_ESP8266)
	// Remove PSTR macros from debug prints
	#undef PSTR
	#define PSTR(x) (x)
	//#undef F
	//#define F(x) (x)
	#include "core/MyHwESP8266.cpp"
	// For ESP8266, we always enable gateway feature
	#define MY_GATEWAY_ESP8266
#elif defined(ARDUINO_ARCH_AVR)
	#include "core/MyHwATMega328.cpp"
#endif

// LEDS
#if defined(MY_LEDS_BLINKING_FEATURE)
	#include "core/MyLeds.cpp"
#else
	#include "core/MyLeds.h"
#endif

// INCLUSION MODE
#if defined(MY_INCLUSION_MODE_FEATURE)
	#include "core/MyInclusionMode.cpp"
#endif


// SIGNING
#if defined(MY_SIGNING_ATSHA204) || defined(MY_SIGNING_SOFT)
	// SIGNING COMMON FUNCTIONS
	#include "core/MySigning.cpp"
	#define MY_SIGNING_FEATURE
	#if defined(MY_SIGNING_ATSHA204) && defined(MY_SIGNING_SOFT)
		#error Only one signing engine can be activated
	#endif

	#if defined(MY_SIGNING_ATSHA204)
		#include "core/MySigningAtsha204.cpp"
		#include "drivers/ATSHA204/ATSHA204.cpp"
	#elif defined(MY_SIGNING_SOFT)
		#include "core/MySigningAtsha204Soft.cpp"
		#include "drivers/ATSHA204/sha256.cpp"
	#endif
#endif



// GATEWAY - TRANSPORT
#if defined(MY_GATEWAY_MQTT_CLIENT)
	#if defined(MY_RADIO_FEATURE)
		// We assume that a gateway having a radio also should act as repeater
		#define MY_REPEATER_FEATURE
	#endif
	// GATEWAY - COMMON FUNCTIONS
	// We only support MQTT Client using W5100 and ESP8266 at the moment
	#if !defined(MY_CONTROLLER_IP_ADDRESS)
		#error You must specify MY_CONTROLLER_IP_ADDRESS (MQTT broker address)
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

	#include "drivers/pubsubclient/src/PubSubClient.cpp"
	#include "core/MyGatewayTransport.cpp"
	#include "core/MyGatewayTransportMQTTClient.cpp"
#elif defined(MY_GATEWAY_FEATURE)
	// GATEWAY - COMMON FUNCTIONS
	#include "core/MyGatewayTransport.cpp"

	// We currently only support one protocol at the moment, enable it.
	#include "core/MyProtocolMySensors.cpp"

	// GATEWAY - CONFIGURATION
	#if defined(MY_RADIO_FEATURE)
		// We assume that a gateway having a radio also should act as repeater
		#define MY_REPEATER_FEATURE
	#endif
	#if defined(MY_CONTROLLER_IP_ADDRESS)
		#define MY_GATEWAY_CLIENT_MODE
	#endif
	#if !defined(MY_PORT)
		#error You must define MY_PORT (controller or gatway port to open)
	#endif
	#if defined(MY_GATEWAY_ESP8266)
		// GATEWAY - ESP8266
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


// RADIO
#if defined(MY_RADIO_NRF24) || defined(MY_RADIO_RFM69) || defined(MY_RS485)
	// SOFTSPI
	#ifdef MY_SOFTSPI
		#if defined(ARDUINO_ARCH_ESP8266)
			#error Soft SPI is not available on ESP8266
		#endif
		#include "drivers/AVR/DigitalIO/SoftI2cMaster.cpp"
		#include "drivers/AVR/DigitalIO/PinIO.cpp"
	#endif

	// FLASH
	#ifdef MY_OTA_FIRMWARE_FEATURE
		#include "drivers/SPIFlash/SPIFlash.cpp"
	#endif
	#include "core/MyTransport.cpp"
	#if (defined(MY_RADIO_NRF24) && defined(MY_RADIO_RFM69)) || (defined(MY_RADIO_NRF24) && defined(MY_RS485)) || (defined(MY_RADIO_RFM69) && defined(MY_RS485))
		#error Only one forward link driver can be activated
	#endif
	#if defined(MY_RADIO_NRF24)
		#if defined(MY_RF24_ENABLE_ENCRYPTION)
			#include "drivers/AES/AES.cpp"
		#endif
		#include "drivers/RF24/RF24.cpp"
		#include "core/MyTransportNRF24.cpp"
	#elif defined(MY_RS485)
		#include "drivers/AltSoftSerial/AltSoftSerial.cpp"
		#include "core/MyTransportRS485.cpp"
	#elif defined(MY_RADIO_RFM69)
		#include "drivers/RFM69/RFM69.cpp"
		#include "core/MyTransportRFM69.cpp"
	#endif
#endif

// Make sure to disable child features when parent feature is disabled
#if !defined(MY_RADIO_FEATURE)
	#undef MY_OTA_FIRMWARE_FEATURE
	#undef MY_REPEATER_FEATURE
	#undef MY_SIGNING_NODE_WHITELISTING
	#undef MY_SIGNING_FEATURE
#endif

#if !defined(MY_GATEWAY_FEATURE)
	#undef MY_INCLUSION_MODE_FEATURE
	#undef MY_INCLUSION_BUTTON_FEATURE
#endif

#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RADIO_FEATURE)
	#error No forward link or gateway feature activated. This means nowhere to send messages! Pretty pointless.
#endif


#include "core/MyMessage.cpp"
#include "core/MySensorCore.cpp"

extern void setup();
// Optional sketch functions called by MySensors library
void receive(const MyMessage &message)  __attribute__((weak));
void receiveTime(unsigned long)  __attribute__((weak));
void presentation()  __attribute__((weak));
extern "C" void setup()  __attribute__((weak));
extern "C" void loop()  __attribute__((weak));
extern "C" void loop2()  __attribute__((weak));


#include <Arduino.h>

#if defined(MY_GATEWAY_ESP8266)
	#include "core/MyMainESP8266.cpp"
#else
	#include "core/MyMainDefault.cpp"
#endif


#endif
