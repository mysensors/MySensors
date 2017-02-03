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

#include "MyTransportHAL.h"

#if defined(MY_RFM69_NEW_DRIVER)

#include "drivers/RFM69/new/RFM69_new.h"

bool transportInit(void)
{
	const bool result = RFM69_initialise(MY_RFM69_FREQUENCY);
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RFM69_ATC_MODE_DISABLED)
	// only enable ATC mode nodes
	RFM69_ATCmode(true, MY_RFM69_ATC_TARGET_RSSI_DBM);
#endif

#ifdef MY_RFM69_ENABLE_ENCRYPTION
	uint8_t _psk[16];
	hwReadConfigBlock((void*)_psk, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
	RFM69_encrypt((const char*)_psk);
	memset(_psk, 0, 16); // Make sure it is purged from memory when set
#endif

	return result;
}

void transportSetAddress(const uint8_t address)
{
	RFM69_setAddress(address);
}

uint8_t transportGetAddress(void)
{
	return RFM69_getAddress();
}

bool transportSend(const uint8_t to, const void* data, uint8_t len, const bool sendAndForget)
{
	if (sendAndForget) {
		(void)RFM69_sendWithRetry(to, data, len, 0);
		return true;
	}
	return RFM69_sendWithRetry(to, data, len);
}

bool transportAvailable(void)
{
	return RFM69_available();
}

bool transportSanityCheck(void)
{
	return RFM69_sanityCheck();
}

uint8_t transportReceive(void* data)
{
	return RFM69_recv((uint8_t*)data);
}

void transportSleep(void)
{
	(void)RFM69_sleep();
}

void transportStandBy(void)
{
	(void)RFM69_standBy();
}

void transportPowerDown(void)
{
	(void)RFM69_powerDown();
}

void transportPowerUp(void)
{
	(void)RFM69_powerUp();
}

bool transportSetTxPowerLevel(const uint8_t powerLevel)
{
	// range 0..23
	return RFM69_setTxPowerLevel(powerLevel);
}

void transportSetTargetRSSI(int16_t targetSignalStrength)
{
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RFM69_ATC_MODE_DISABLED)
	RFM69_ATCmode(true, targetSignalStrength);
#endif
}

// experimental
// **********************************************

int16_t transportGetSendingRSSI(void)
{
	return static_cast<int16_t>(RFM69_getSendingRSSI());
}

int16_t transportGetReceivingRSSI(void)
{
	return static_cast<int16_t>(RFM69_getReceivingRSSI());
}

int16_t transportGetSendingSNR(void)
{
	return static_cast<int16_t>(INVALID_SNR);
}

int16_t transportGetReceivingSNR(void)
{
	return static_cast<int16_t>(INVALID_SNR);
}

int16_t transportGetTxPowerPercent(void)
{
	return static_cast<int16_t>(RFM69_getTxPowerPercent());
}

int16_t transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(RFM69_getTxPowerLevel());
}
bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return RFM69_setTxPowerPercent(powerPercent);
}

#else

#include "drivers/RFM69/old/RFM69_old.h"

RFM69 _radio(MY_RFM69_CS_PIN, MY_RFM69_IRQ_PIN, MY_RFM69HW,
             digitalPinToInterrupt(MY_RFM69_IRQ_PIN));
uint8_t _address;

bool transportInit(void)
{
	// Start up the radio library (_address will be set later by the MySensors library)
	if (_radio.initialize(MY_RFM69_FREQUENCY, _address, MY_RFM69_NETWORKID)) {
#ifdef MY_RFM69_ENABLE_ENCRYPTION
		uint8_t _psk[16];
		hwReadConfigBlock((void*)_psk, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
		_radio.encrypt((const char*)_psk);
		memset(_psk, 0, 16); // Make sure it is purged from memory when set
#endif
		return true;
	}
	return false;
}

void transportSetAddress(const uint8_t address)
{
	_address = address;
	_radio.setAddress(address);
}

uint8_t transportGetAddress(void)
{
	return _address;
}

bool transportSend(const uint8_t to, const void* data, const uint8_t len, const bool sendAndForget)
{
	if (sendAndForget) {
		(void)_radio.sendWithRetry(to, data, len, 0);
		return true;
	}
	return _radio.sendWithRetry(to, data, len);
}

bool transportAvailable(void)
{
	return _radio.receiveDone();
}

bool transportSanityCheck(void)
{
	return _radio.sanityCheck();
}

uint8_t transportReceive(void* data)
{
	(void)memcpy(&data, (const void *)_radio.DATA, _radio.DATALEN);
	// save payload length
	const uint8_t dataLen = _radio.DATALEN;
	// Send ack back if this message wasn't a broadcast
	if (_radio.ACKRequested()) {
		_radio.sendACK();
	}
	return dataLen;
}

void transportSleep(void)
{
	_radio.sleep();
}

void transportStandBy(void)
{
	_radio.standBy();
}
void transportPowerDown(void)
{
	_radio.powerDown();
}
void transportPowerUp(void)
{
	_radio.powerUp();
}

// experimental
// **********************************************
int16_t transportGetSendingRSSI(void)
{
	return static_cast<int16_t>(INVALID_RSSI);
}

int16_t transportGetReceivingRSSI(void)
{
	return static_cast<int16_t>(_radio.RSSI);
}

int16_t transportGetSendingSNR(void)
{
	return static_cast<int16_t>(INVALID_SNR);
}

int16_t transportGetReceivingSNR(void)
{
	return static_cast<int16_t>(INVALID_SNR);
}

int16_t transportGetTxPowerPercent(void)
{
	return static_cast<int16_t>(0);
}

int16_t transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(0);
}

// **********************************************

#endif