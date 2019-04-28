/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list:
 * https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "hal/transport/NRF5_ESB/driver/Radio.h"
#include "hal/transport/NRF5_ESB/driver/Radio_ESB.h"

#include "drivers/CircularBuffer/CircularBuffer.h"

bool transportInit(void)
{
	return NRF5_ESB_initialize();
}

void transportSetAddress(const uint8_t address)
{
	NRF5_ESB_setNodeAddress(address);
	NRF5_ESB_startListening();
}

uint8_t transportGetAddress(void)
{
	return NRF5_ESB_getNodeID();
}

bool transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	return NRF5_ESB_sendMessage(to, data, len, noACK);
}

bool transportDataAvailable(void)
{
	return NRF5_ESB_isDataAvailable();
}

bool transportSanityCheck(void)
{
	return NRF5_ESB_sanityCheck();
}

uint8_t transportReceive(void *data)
{
	uint8_t len = 0;
	len = NRF5_ESB_readMessage(data);
	return len;
}

void transportPowerDown(void)
{
	NRF5_ESB_powerDown();
}

void transportPowerUp(void)
{
	NRF5_ESB_powerUp();
}

void transportSleep(void)
{
	NRF5_ESB_sleep();
}

void transportStandBy(void)
{
	NRF5_ESB_standBy();
}

int16_t transportGetSendingRSSI(void)
{
	return NRF5_ESB_getSendingRSSI();
}

int16_t transportGetReceivingRSSI(void)
{
	return NRF5_ESB_getReceivingRSSI();
}

int16_t transportGetSendingSNR(void)
{
	return INVALID_SNR;
}

int16_t transportGetReceivingSNR(void)
{
	return INVALID_SNR;
}

int16_t transportGetTxPowerPercent(void)
{
	return NRF5_getTxPowerPercent();
}

int16_t transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(NRF5_getTxPowerLevel());
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return NRF5_setTxPowerPercent(powerPercent);
}
