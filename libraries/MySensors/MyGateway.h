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

#ifndef MyGateway_h
#define MyGateway_h


#include "MyConfig.h"
#include "MySensor.h"
#include "MyHw.h"
#include "MyTransport.h"
#include "MyTransportNRF24.h"
#ifdef MY_SIGNING_FEATURE
#include "MySigning.h"
#include "MySigningNone.h"
#endif
#include "MyMessage.h"
#ifdef MY_OTA_FIRMWARE_FEATURE
#include "utility/SPIFlash.h"
#endif

#include "MyProtocol.h"
#include "MyGatewayTransport.h"
#include "MyGatewayTransportSerial.h"

#include <stddef.h>
#include <stdarg.h>


class MyGateway : public MySensor
{
public:
	// MyProtocolSerial constructor
	MyGateway(MyGatewayTransport &transport=*new MyGatewayTransportSerial(), MyTransport &radio =*new MyTransportNRF24(), MyHw &hw=*new MyHwDriver()
#ifdef MY_SIGNING_FEATURE
		, MySigning &signer=*new MySigningNone()
#endif
#ifdef WITH_LEDS_BLINKING
		, uint8_t _rx=MY_DEFAULT_RX_LED_PIN
		, uint8_t _tx=MY_DEFAULT_TX_LED_PIN
		, uint8_t _er=MY_DEFAULT_ERR_LED_PIN
		, unsigned long _blink_period=MY_DEFAULT_LED_BLINK_PERIOD
#endif
		);


	void enableInclusionModeFeature(uint8_t inclusionModeButtonPin, uint8_t inclusionModeDuration);

	/**
	 * See MySensor.h
	 *
	 * Overloaded to handle local sensor data sent to controller.
	 */
	boolean sendRoute(MyMessage &message);

	/**
	 * Processes incoming messages to this node.
	 * Returns true if there is a message addressed for this node was received.
	 * Preferred way of handling incoming messages is to use the callback defined in begin().
	 */
	boolean process();

private:
	MyGatewayTransport &transport;
};

#endif
