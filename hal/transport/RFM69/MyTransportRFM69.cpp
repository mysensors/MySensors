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
 *
 * MultiTransport implementation created by Olivier Mauti 2020 <olivier@mysensors.org>
 */

#if defined(MY_RFM69_NEW_DRIVER)

#include "hal/transport/RFM69/driver/new/RFM69_new.h"

bool RFM69_transportInit(void)
{
	const bool result = RFM69_initialise(MY_RFM69_FREQUENCY);
#if defined(MY_GATEWAY_FEATURE) || defined(MY_RFM69_ATC_MODE_DISABLED)
	// ATC mode function not used
	(void)RFM69_ATCmode;
#else
	RFM69_ATCmode(true, MY_RFM69_ATC_TARGET_RSSI_DBM);
#endif
	return result;
}

void RFM69_transportSetAddress(const uint8_t address)
{
	RFM69_setAddress(address);
}

uint8_t RFM69_transportGetAddress(void)
{
	return RFM69_getAddress();
}

bool RFM69_transportSend(const uint8_t to, const void *data, uint8_t len, const bool noACK)
{
	return RFM69_sendWithRetry(to, data, len, noACK);
}

bool RFM69_transportDataAvailable(void)
{
	return RFM69_available();
}

void RFM69_transportTask(void)
{
	RFM69_handling();
#if defined(MY_TRANSPORT_RX_QUEUE)
	if (RFM69_available()) {
		RXQueuedMessage_t *msgIn = transportHALGetQueueBuffer();
		if (msgIn != NULL) {
			msgIn->channel = TRANSPORT_RFM69_CHANNEL_ID;
			msgIn->length = RFM69_receive((uint8_t *)&msgIn->data,
			                              sizeof(msgIn->data));
			(void)transportHALPushQueueBuffer(msgIn);
		}
	}
#endif
}

void RFM69_transportEncrypt(const uint8_t *key)
{
	RFM69_HWencryption((const char *)key);
}

bool RFM69_transportSanityCheck(void)
{
	return RFM69_sanityCheck();
}

uint8_t RFM69_transportReceive(void *data, const uint8_t maxBufSize)
{
	return RFM69_receive((uint8_t *)data, maxBufSize);
}

void RFM69_transportSleep(void)
{
	(void)RFM69_sleep();
}

void RFM69_transportStandBy(void)
{
	(void)RFM69_standBy();
}

void RFM69_transportPowerDown(void)
{
	(void)RFM69_powerDown();
}

void RFM69_transportPowerUp(void)
{
	(void)RFM69_powerUp();
}

bool RFM69_transportSetTxPowerLevel(const uint8_t powerLevel)
{
	// range 0..23
	return RFM69_setTxPowerLevel(powerLevel);
}

void RFM69_transportSetTargetRSSI(const int16_t targetSignalStrength)
{
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RFM69_ATC_MODE_DISABLED)
	RFM69_ATCmode(true, targetSignalStrength);
#else
	(void)targetSignalStrength;
#endif
}

int16_t RFM69_transportGetSendingRSSI(void)
{
	return RFM69_getSendingRSSI();
}

int16_t RFM69_transportGetReceivingRSSI(void)
{
	return RFM69_getReceivingRSSI();
}

int16_t RFM69_transportGetSendingSNR(void)
{
	return INVALID_SNR;
}

int16_t RFM69_transportGetReceivingSNR(void)
{
	return INVALID_SNR;
}

int16_t RFM69_transportGetTxPowerPercent(void)
{
	return RFM69_getTxPowerPercent();
}

int16_t RFM69_transportGetTxPowerLevel(void)
{
	return RFM69_getTxPowerLevel();
}

bool RFM69_transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return RFM69_setTxPowerPercent(powerPercent);
}

#else

#include "hal/transport/RFM69/driver/old/RFM69_old.h"

RFM69 _radio(MY_RFM69_CS_PIN, MY_RFM69_IRQ_PIN, MY_RFM69HW, MY_RFM69_IRQ_NUM);
uint8_t _address;

bool RFM69_transportInit(void)
{
#if defined(MY_RFM69_POWER_PIN)
	//hwPinMode(MY_RFM69_POWER_PIN, OUTPUT);
#endif
#ifdef MY_RF69_DIO5
	//hwPinMode(MY_RF69_DIO5, INPUT);
#endif
	// Start up the radio library (_address will be set later by the MySensors library)
	if (_radio.initialize(MY_RFM69_FREQUENCY, _address, MY_RFM69_NETWORKID)) {
		return true;
	}
	return false;
}

void RFM69_transportTask(void)
{
	// not implemented
}

void RFM69_transportEncrypt(const uint8_t *key)
{
	_radio.encrypt((const char *)key);
}

void RFM69_transportSetAddress(const uint8_t address)
{
	_address = address;
	_radio.setAddress(address);
}

uint8_t RFM69_transportGetAddress(void)
{
	return _address;
}

bool RFM69_transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	if (noACK) {
		(void)_radio.sendWithRetry(to, data, len, 0, 0);
		return true;
	}
	return _radio.sendWithRetry(to, data, len);
}

bool RFM69_transportDataAvailable(void)
{
	return _radio.receiveDone();
}

bool RFM69_transportSanityCheck(void)
{
	return _radio.sanityCheck();
}

uint8_t RFM69_transportReceive(void *data, const uint8_t maxBufSize)
{
	// save payload length
	const uint8_t dataLen = _radio.DATALEN < maxBufSize ? _radio.DATALEN : maxBufSize;
	(void)memcpy((void *)data, (void *)_radio.DATA, dataLen);
	// Send ack back if this message wasn't a broadcast
	if (_radio.ACKRequested()) {
		_radio.sendACK();
	}
	return dataLen;
}

void RFM69_transportSleep(void)
{
	_radio.sleep();
}

void RFM69_transportStandBy(void)
{
	_radio.standBy();
}

void RFM69_transportPowerDown(void)
{
	_radio.powerDown();
}

void RFM69_transportPowerUp(void)
{
	_radio.powerUp();
}

int16_t RFM69_transportGetSendingRSSI(void)
{
	return INVALID_RSSI;
}

int16_t RFM69_transportGetReceivingRSSI(void)
{
	return _radio.RSSI;
}

int16_t RFM69_transportGetSendingSNR(void)
{
	return INVALID_SNR;
}

int16_t RFM69_transportGetReceivingSNR(void)
{
	return INVALID_SNR;
}

int16_t RFM69_transportGetTxPowerPercent(void)
{
	return INVALID_PERCENT;
}

bool RFM69_transportSetTxPowerPercent(const uint8_t powerPercent)
{
	// not implemented
	(void)powerPercent;
	return false;
}

int16_t RFM69_transportGetTxPowerLevel(void)
{
	return INVALID_LEVEL;
}

bool RFM69_transportSetTxPowerLevel(const uint8_t powerLevel)
{
	// not implemented
	(void)powerLevel;
	return false;
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	// not implemented
	(void)powerPercent;
	return false;
}


#endif
