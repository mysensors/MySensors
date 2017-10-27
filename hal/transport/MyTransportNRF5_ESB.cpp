/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list:
 * https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyTransportHAL.h"
#include "drivers/NRF5/Radio.h"
#include "drivers/NRF5/Radio_ESB.h"

#include "drivers/CircularBuffer/CircularBuffer.h"

#if defined(MY_NRF5_ESB_ENABLE_ENCRYPTION)
#include "drivers/AES/AES.h"
#endif

#if defined(MY_NRF5_ESB_ENABLE_ENCRYPTION)
AES _aes;
uint8_t _dataenc[32] = {0};
uint8_t _psk[16];
#endif

bool transportInit(void)
{
#if defined(MY_NRF5_ESB_ENABLE_ENCRYPTION)
#ifdef MY_SIGNING_SIMPLE_PASSWD
	memset(_psk, 0, 16);
	memcpy(_psk, MY_SIGNING_SIMPLE_PASSWD, strnlen(MY_SIGNING_SIMPLE_PASSWD, 16));
#else
	hwReadConfigBlock((void*)_psk, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
#endif
	// set up AES-key
	_aes.set_key(_psk, 16);
	// Make sure it is purged from memory when set
	memset(_psk, 0, 16);
#endif

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
#if defined(MY_NRF5_ESB_ENABLE_ENCRYPTION)
	// copy input data because it is read-only
	(void)memcpy(_dataenc, data, len);
	// has to be adjusted, WIP!
	_aes.set_IV(0);
	const uint8_t finalLength = len > 16 ? 32 : 16;
	// encrypt data
	_aes.cbc_encrypt(_dataenc, _dataenc, finalLength / 16);
	return NRF5_ESB_sendMessage(to, _dataenc, finalLength, noACK);
#else
	return NRF5_ESB_sendMessage(to, data, len, noACK);
#endif
}

bool transportAvailable(void)
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
#if defined(MY_NRF5_ESB_ENABLE_ENCRYPTION)
	// has to be adjusted, WIP!
	_aes.set_IV(0);
	// decrypt data
	if (_aes.cbc_decrypt((uint8_t *)(data), (uint8_t *)(data),
	                     len > 16 ? 2 : 1) != AES_SUCCESS) {
		len = 0;
	}
#endif
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
