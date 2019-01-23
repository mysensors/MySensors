/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#if defined(MY_RFM69_NEW_DRIVER)

#include "hal/transport/RFM69/driver/new/RFM69_new.h"

bool transportInit(void)
{
	const bool result = RFM69_initialise(MY_RFM69_FREQUENCY);
#if defined(MY_GATEWAY_FEATURE) || defined(MY_RFM69_ATC_MODE_DISABLED)
	// ATC mode function not used
	(void)RFM69_ATCmode;
#else
	RFM69_ATCmode(true, MY_RFM69_ATC_TARGET_RSSI_DBM);
#endif

#ifdef MY_RFM69_ENABLE_ENCRYPTION
	uint8_t RFM69_psk[16];
#ifdef MY_ENCRYPTION_SIMPLE_PASSWD
	(void)memset(RFM69_psk, 0, 16);
	(void)memcpy(RFM69_psk, MY_ENCRYPTION_SIMPLE_PASSWD, strnlen(MY_ENCRYPTION_SIMPLE_PASSWD, 16));
#else
	hwReadConfigBlock((void *)RFM69_psk, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
#endif
	RFM69_encrypt((const char *)RFM69_psk);
	(void)memset(RFM69_psk, 0, 16); // Make sure it is purged from memory when set
#else
	(void)RFM69_encrypt;
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

bool transportSend(const uint8_t to, const void *data, uint8_t len, const bool noACK)
{
	if (noACK) {
		(void)RFM69_sendWithRetry(to, data, len, 0, 0);
		return true;
	}
	return RFM69_sendWithRetry(to, data, len);
}

bool transportAvailable(void)
{
	RFM69_handler();
	return RFM69_available();
}

bool transportSanityCheck(void)
{
	return RFM69_sanityCheck();
}

uint8_t transportReceive(void *data)
{
	return RFM69_receive((uint8_t *)data, MAX_MESSAGE_LENGTH);
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

void transportSetTargetRSSI(const int16_t targetSignalStrength)
{
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RFM69_ATC_MODE_DISABLED)
	RFM69_ATCmode(true, targetSignalStrength);
#else
	(void)targetSignalStrength;
#endif
}

int16_t transportGetSendingRSSI(void)
{
	return RFM69_getSendingRSSI();
}

int16_t transportGetReceivingRSSI(void)
{
	return RFM69_getReceivingRSSI();
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
	return RFM69_getTxPowerPercent();
}

int16_t transportGetTxPowerLevel(void)
{
	return RFM69_getTxPowerLevel();
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return RFM69_setTxPowerPercent(powerPercent);
}

#else

#include "hal/transport/RFM69/driver/old/RFM69_old.h"

RFM69 _radio(MY_RFM69_CS_PIN, MY_RFM69_IRQ_PIN, MY_RFM69HW, MY_RFM69_IRQ_NUM);
uint8_t _address;

bool transportInit(void)
{
#if defined(MY_RFM69_POWER_PIN)
	//hwPinMode(MY_RFM69_POWER_PIN, OUTPUT);
#endif
#ifdef MY_RF69_DIO5
	//hwPinMode(MY_RF69_DIO5, INPUT);
#endif
	// Start up the radio library (_address will be set later by the MySensors library)
	if (_radio.initialize(MY_RFM69_FREQUENCY, _address, MY_RFM69_NETWORKID)) {
#ifdef MY_RFM69_ENABLE_ENCRYPTION
		uint8_t RFM69_psk[16];
#ifdef MY_ENCRYPTION_SIMPLE_PASSWD
		(void)memset(RFM69_psk, 0, 16);
		(void)memcpy(RFM69_psk, MY_ENCRYPTION_SIMPLE_PASSWD, strnlen(MY_ENCRYPTION_SIMPLE_PASSWD, 16));
#else
		hwReadConfigBlock((void *)RFM69_psk, (void *)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
#endif
		_radio.encrypt((const char *)RFM69_psk);
		(void)memset(RFM69_psk, 0, 16); // Make sure it is purged from memory when set
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

bool transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	if (noACK) {
		(void)_radio.sendWithRetry(to, data, len, 0, 0);
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

uint8_t transportReceive(void *data)
{
	// save payload length
	const uint8_t dataLen = _radio.DATALEN < MAX_MESSAGE_LENGTH? _radio.DATALEN : MAX_MESSAGE_LENGTH;
	(void)memcpy((void *)data, (void *)_radio.DATA, dataLen);
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

int16_t transportGetSendingRSSI(void)
{
	return INVALID_RSSI;
}

int16_t transportGetReceivingRSSI(void)
{
	return _radio.RSSI;
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
	return INVALID_PERCENT;
}

int16_t transportGetTxPowerLevel(void)
{
	return INVALID_LEVEL;
}

bool transportSetTxPowerLevel(const uint8_t powerLevel)
{
	// not implemented
	(void)powerLevel;
	return false;
}

#endif
