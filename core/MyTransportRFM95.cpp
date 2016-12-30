/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2016 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

#include "MyConfig.h"
#include "MyTransport.h"
#include "drivers/RFM95/RFM95.h"

bool transportInit(void)
{
	const bool result = RFM95_initialise(MY_RFM95_FREQUENCY);
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RFM95_ATC_MODE_DISABLED)
	// only enable ATC mode nodes
	RFM95_ATCmode(true, MY_RFM95_ATC_TARGET_RSSI);
#endif
	return result;
}

void transportSetAddress(const uint8_t address)
{
	RFM95_setAddress(address);
}

uint8_t transportGetAddress(void)
{
	return RFM95_getAddress();
}

bool transportSend(const uint8_t to, const void* data, const uint8_t len)
{
	return RFM95_sendWithRetry(to, data, len);
}

bool transportAvailable(void)
{
	return RFM95_available();
}

bool transportSanityCheck(void)
{
	return RFM95_sanityCheck();
}

uint8_t transportReceive(void* data)
{
	return RFM95_recv((uint8_t*)data);
}

void transportPowerDown(void)
{
	(void)RFM95_sleep();
}

// experimental
// **********************************************
int16_t transportGetReceivingSignalStrength(void)
{
	return RFM95_getReceivingRSSI();
}
int16_t transportGetSendingSignalStrength(void)
{
	return RFM95_getSendingRSSI();
}
int8_t transportGetReceivingSNR(void)
{
	return RFM95_getReceivingSNR();
}
int8_t transportGetSendingSNR(void)
{
	return RFM95_getSendingSNR();
}
uint8_t transportGetTxPower(void)
{
	return RFM95_getTxPowerPercent();
}
// **********************************************
