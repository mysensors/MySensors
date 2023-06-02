/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "hal/transport/SX126x/driver/SX126x.h"

bool transportInit(void)
{
	const bool result = SX126x_initialise();
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_SX126x_ATC_MODE_DISABLED)
	SX126x_setATC(true, SX126x_TARGET_RSSI);
#endif
	return result;
}

void transportSetAddress(const uint8_t address)
{
	SX126x_setAddress(address);
}

uint8_t transportGetAddress(void)
{
	return SX126x_getAddress();
}

bool transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	return SX126x_sendWithRetry(to, data, len, noACK);
}

bool transportDataAvailable(void)
{
	SX126x_handle();
	return SX126x_packetAvailable();
}

bool transportSanityCheck(void)
{
	return SX126x_sanityCheck();
}

uint8_t transportReceive(void *data)
{
	return SX126x_getData(static_cast<uint8_t *>(data), MAX_MESSAGE_SIZE);
}

void transportSleep(void)
{
	SX126x_sleep();
}

void transportStandBy(void)
{
	SX126x_standBy();
}

void transportPowerDown(void)
{
	SX126x_powerDown();
}

void transportPowerUp(void)
{
	SX126x_powerUp();
}

void transportToggleATCmode(const bool OnOff, const int16_t targetRSSI)
{
	SX126x_setATC(OnOff, targetRSSI);
}

int16_t transportGetSendingRSSI(void)
{
	return SX126x_getSendingRSSI();
}

int16_t transportGetReceivingRSSI(void)
{
	return SX126x_getReceivingRSSI();
}

int16_t transportGetSendingSNR(void)
{
	return SX126x_getSendingSNR();
}

int16_t transportGetReceivingSNR(void)
{
	return SX126x_getReceivingSNR();
}

int16_t transportGetTxPowerPercent(void)
{
	return SX126x_getTxPowerPercent();
}

int16_t transportGetTxPowerLevel(void)
{
	return SX126x_getTxPowerLevel();
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return SX126x_setTxPowerPercent(powerPercent);
}
