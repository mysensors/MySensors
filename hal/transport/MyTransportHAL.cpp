/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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

#include "MyTransportHAL.h"

// debug
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_HAL)
#define TRANSPORT_HAL_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug
#else
#define TRANSPORT_HAL_DEBUG(x,...)	//!< debug NULL
#endif

#if (MY_TRANSPORT_COUNT > 1)
#if defined(MY_RADIO_RFM69) && !defined(MY_RFM69_NEW_DRIVER)
#error Multitransport only works with the new RFM69 driver!
#endif

#if (MY_TRANSPORT_COUNT < 4)
// 2 - 3 transports, 2 bits: 0=BC, 1=TSP1, 2=TSP2, 3=TSP3
// 256 / 4 = 64 bytes RAM
#define SIZE_BITS_PER_CHANNEL 2
#elif (MY_TRANSPORT_COUNT >= 4) && (MY_TRANSPORT_COUNT < 16)
// 4 - 7 transports, 4 bits: 0000 - 1111
// 256 / 2 = 128 bytes RAM
#define SIZE_BITS_PER_CHANNEL 4
#else
#error Sorry, 16+ transports not supported :)
#endif

#define SIZE_CHANNELS_PER_BYTE ( 8 / SIZE_BITS_PER_CHANNEL)

static uint8_t channelRoute[SIZE_CHANNEL_ROUTE / SIZE_CHANNELS_PER_BYTE];

transportChannelID_t transportGetChannel(const uint8_t nodeId)
{
	if (nodeId == BROADCAST_ADDRESS) {
		return TRANSPORT_ALL_CHANNEL_ID;
	}
	const uint8_t arrayPos = nodeId / SIZE_CHANNELS_PER_BYTE;
	const uint8_t bitPos = (nodeId % SIZE_CHANNELS_PER_BYTE) * SIZE_BITS_PER_CHANNEL;
	const uint8_t channel = (channelRoute[arrayPos] >> bitPos) & ((1 << SIZE_BITS_PER_CHANNEL) - 1);
	TRANSPORT_HAL_DEBUG(PSTR("THA:GCH:GET N=%" PRIu8 ",CH=%" PRIu8 "\n"), nodeId, channel);
	return static_cast<transportChannelID_t>(channel);
}

void transportUpdateChannel(const uint8_t nodeId, const transportChannelID_t channel)
{
	if (nodeId != BROADCAST_ADDRESS) {
		const uint8_t arrayPos = nodeId / SIZE_CHANNELS_PER_BYTE;
		const uint8_t bitPos = (nodeId % SIZE_CHANNELS_PER_BYTE) * SIZE_BITS_PER_CHANNEL;
		uint8_t val = channelRoute[arrayPos];
		// clear pos
		val &= ~(((1 << SIZE_BITS_PER_CHANNEL) - 1) << bitPos);
		// set pos
		val |= (static_cast<uint8_t>(channel) & ((1 << SIZE_BITS_PER_CHANNEL) - 1)) << bitPos;
		channelRoute[arrayPos] = val;
		TRANSPORT_HAL_DEBUG(PSTR("THA:UCH:SET N=%" PRIu8 ",CH=%" PRIu8 "\n"), nodeId, channel);
	}
}

// reset channel routes
void transportResetChannels(void)
{
	for (uint16_t i = 0; i < sizeof(channelRoute); i++) {
		channelRoute[i] = 0;
	}
}

void transportDebugChannels(void)
{
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_HAL)
	for (uint16_t i = 0; i < sizeof(channelRoute); i++) {
		if (transportGetChannel(i) != TRANSPORT_ALL_CHANNEL_ID) {
			TRANSPORT_HAL_DEBUG(PSTR("THA:DCH:%" PRIu8 ": %" PRIu8 "\n"), i, transportGetChannel(i));
		}
	}
#endif
}

#endif

#if defined(MY_TRANSPORT_RX_QUEUE)
static RXQueuedMessage_t transportRxQueueStorage[RX_QUEUE_BUFFER_SIZE];
static CircularBuffer<RXQueuedMessage_t> transportRxQueue(transportRxQueueStorage,
        RX_QUEUE_BUFFER_SIZE);


RXQueuedMessage_t *transportHALGetQueueBuffer(void)
{
	return transportRxQueue.getFront();
}

bool transportHALPushQueueBuffer(RXQueuedMessage_t *buffer)
{
	return transportRxQueue.pushFront(buffer);
}

#endif

bool transportHALInit(void)
{
	TRANSPORT_HAL_DEBUG(PSTR("THA:INIT:TSP CNT=%" PRIu8 "\n"), MY_TRANSPORT_COUNT);

#if (MY_TRANSPORT_COUNT > 1)
	// initialize channel routing table
	transportResetChannels();
#endif

#if defined(MY_TRANSPORT_ENCRYPTION)
	uint8_t transportPSK[16];
	transportEncryptionInit(transportPSK, sizeof(transportPSK));

#if defined(MY_RADIO_RFM69)
	RFM69_transportEncrypt((const uint8_t *)transportPSK);
#else
	//set up AES-key
	AES128CBCInit(transportPSK);
#endif
#endif

	bool result = true;
#if defined(MY_RADIO_RF24)
	result &= RF24_transportInit();
#endif
#if defined(MY_RADIO_RFM69)
	result &= RFM69_transportInit();
#endif
#if defined(MY_RADIO_RFM95)
	result &= RFM95_transportInit();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	result &= NRF5_ESB_transportInit();
#endif
#if defined(MY_RS485)
	result &= RS485_transportInit();
#endif

#if defined(MY_TRANSPORT_ENCRYPTION)
	// Make sure it is purged from memory when set
	(void)memset((void *)transportPSK, 0,
	             sizeof(transportPSK));
#endif
	return result;
}

void transportHALHandler(void)
{
#if defined(MY_RADIO_RF24)
	RF24_transportTask();
#endif
#if defined(MY_RADIO_RFM69)
	RFM69_transportTask();
#endif
#if defined(MY_RADIO_RFM95)
	RFM95_transportTask();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	NRF5_ESB_transportTask();
#endif
#if defined(MY_RS485)
	RS485_transportTask();
#endif
}

void transportHALSetAddress(const uint8_t address)
{
	TRANSPORT_HAL_DEBUG(PSTR("THA:SAD:ADDR=%" PRIu8 "\n"), address);
#if defined(MY_RADIO_RF24)
	RF24_transportSetAddress(address);
#endif
#if defined(MY_RADIO_RFM69)
	RFM69_transportSetAddress(address);
#endif
#if defined(MY_RADIO_RFM95)
	RFM95_transportSetAddress(address);
#endif
#if defined(MY_RADIO_NRF5_ESB)
	NRF5_ESB_transportSetAddress(address);
#endif
#if defined(MY_RS485)
	RS485_transportSetAddress(address);
#endif
	// cppcheck
	(void)address;
}

uint8_t transportHALGetAddress(void)
{
	uint8_t result = AUTO;
	// only first match returned
#if defined(MY_RADIO_RF24)
	result = RF24_transportGetAddress();
#elif defined(MY_RADIO_RFM69)
	result = RFM69_transportGetAddress();
#elif defined(MY_RADIO_RFM95)
	result = RFM95_transportGetAddress();
#elif defined(MY_RADIO_NRF5_ESB)
	result = NRF5_ESB_transportGetAddress();
#elif defined(MY_RS485)
	result = RS485_transportGetAddress();
#endif

	TRANSPORT_HAL_DEBUG(PSTR("THA:GAD:ADDR=%" PRIu8 "\n"), result);
	return result;
}

bool transportHALDataAvailable(void)
{
	bool result = false;
#if defined(MY_TRANSPORT_RX_QUEUE)
	result = !transportRxQueue.empty();
#else
#if defined(MY_RADIO_RF24)
	result = RF24_transportDataAvailable();
#elif defined(MY_RADIO_RFM69)
	result = RFM69_transportDataAvailable();
#elif defined(MY_RADIO_RFM95)
	result = RFM95_transportDataAvailable();
#elif  defined(MY_RADIO_NRF5_ESB)
	result = NRF5_ESB_transportDataAvailable();
#elif  defined(MY_RS485)
	result = RS485_transportDataAvailable();
#endif
#endif

#if defined(MY_DEBUG_VERBOSE_TRANSPORT_HAL)
	// cppcheck-suppress knownConditionTrueFalse
	if (result) {
		TRANSPORT_HAL_DEBUG(PSTR("THA:DATA:AVAIL\n"));
	}
#endif
	return result;
}

bool transportHALSanityCheck(void)
{
	bool result = true;
#if defined(MY_RADIO_RF24)
	result &= RF24_transportSanityCheck();
#endif
#if defined(MY_RADIO_RFM69)
	result &= RFM69_transportSanityCheck();
#endif
#if defined(MY_RADIO_RFM95)
	result &= RFM95_transportSanityCheck();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	result &= NRF5_ESB_transportSanityCheck();
#endif
#if defined(MY_RS485)
	result &= RS485_transportSanityCheck();
#endif
	TRANSPORT_HAL_DEBUG(PSTR("THA:SAN:RES=%" PRIu8 "\n"), result);
	return result;
}

bool transportHALReceive(MyMessage *inMsg, uint8_t *msgLength)
{
	uint8_t payloadLength = 0;
#if defined(MY_TRANSPORT_RX_QUEUE)
	TRANSPORT_HAL_DEBUG(PSTR("THA:MSG:RXQ,LEN=%" PRIu8 "\n"), transportRxQueue.available());
	RXQueuedMessage_t *newMessage = transportRxQueue.getBack();
	if (newMessage != NULL) {
		payloadLength = newMessage->length; // check if buffer loading is OF protected!
		(void)memcpy((void *)&inMsg->last, (void *)&newMessage->data, payloadLength);
		(void)transportRxQueue.popBack();
	}
#else
#if defined(MY_RADIO_RF24)
	payloadLength = RF24_transportReceive((void *)&inMsg->last, MAX_MESSAGE_SIZE);
#elif defined(MY_RADIO_RFM69)
	payloadLength = RFM69_transportReceive((void *)&inMsg->last, MAX_MESSAGE_SIZE);
#elif defined(MY_RADIO_RFM95)
	payloadLength = RFM95_transportReceive((void *)&inMsg->last, MAX_MESSAGE_SIZE);
#elif defined(MY_RADIO_NRF5_ESB)
	payloadLength = NRF5_ESB_transportReceive((void *)&inMsg->last, MAX_MESSAGE_SIZE);
#elif defined(MY_RS485)
	payloadLength = RS485_transportReceive((void *)&inMsg->last, MAX_MESSAGE_SIZE);
#endif
#endif

#if defined(MY_DEBUG_VERBOSE_TRANSPORT_HAL)
	hwDebugBuf2Str((const uint8_t *)&inMsg->last, payloadLength);
	TRANSPORT_HAL_DEBUG(PSTR("THA:RCV:MSG=%s\n"), hwDebugPrintStr);
#endif
	if (payloadLength < HEADER_SIZE) {
		TRANSPORT_HAL_DEBUG(PSTR("!THA:RCV:HEADER\n"));
		return false;
	}
	// Reject messages with incorrect protocol version
	if (!inMsg->isProtocolVersionValid()) {
		setIndication(INDICATION_ERR_VERSION);
		TRANSPORT_HAL_DEBUG(PSTR("!THA:RCV:PVER=%" PRIu8 "\n"),
		                    inMsg->getVersion());	// protocol version mismatch
		return false;
	}
	*msgLength = inMsg->getLength();

#if (defined(MY_TRANSPORT_ENCRYPTION) && !defined(MY_RADIO_RFM69)) || defined(MY_RFM69_ENABLE_SW_ENCRYPTION)
	// payload length = a multiple of blocksize length for decrypted messages, i.e. cannot be used for payload length check
#else
	// Reject payloads with incorrect length
	const uint8_t expectedMessageLength = inMsg->getExpectedMessageSize();
	if (payloadLength != expectedMessageLength) {
		setIndication(INDICATION_ERR_LENGTH);
		TRANSPORT_HAL_DEBUG(PSTR("!THA:MSG:LEN=%" PRIu8 ",EXP=%" PRIu8 "\n"), payloadLength,
		                    expectedMessageLength); // invalid payload length
		return false;
	}
	TRANSPORT_HAL_DEBUG(PSTR("THA:MSG:RCV,LEN=%" PRIu8 "\n"), payloadLength);
#endif

#if (MY_TRANSPORT_COUNT > 1)
	transportUpdateChannel(inMsg->last, newMessage->channel);
#endif

	return true;
}


bool transportHALSend(const uint8_t nextRecipient, const MyMessage *outMsg, uint8_t len,
                      const bool noACK)
{
	if (outMsg == NULL) {
		// nothing to send
		TRANSPORT_HAL_DEBUG(PSTR("!THA:SND:MSG NULL\n"));
		return false;
	}
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_HAL)
	hwDebugBuf2Str((const uint8_t *)&outMsg->last, len);
	TRANSPORT_HAL_DEBUG(PSTR("THA:SND:MSG=%s\n"), hwDebugPrintStr);
#endif

#if (MY_TRANSPORT_COUNT > 1)
	const transportChannelID_t channel = transportGetChannel(nextRecipient);
#else
	// cppcheck-suppress unreadVariable
	const transportChannelID_t channel = TRANSPORT_ALL_CHANNEL_ID;
#endif

	bool result = true;

#if defined(MY_RADIO_RF24)
	// cppcheck-suppress knownConditionTrueFalse
	if (channel == TRANSPORT_RF24_CHANNEL_ID || channel == TRANSPORT_ALL_CHANNEL_ID) {
		result &= RF24_transportSend(nextRecipient, (void *)&outMsg->last, len,
		                             noACK);
	}
#endif
#if defined(MY_RADIO_RFM69)
	// cppcheck-suppress knownConditionTrueFalse
	if (channel == TRANSPORT_RFM69_CHANNEL_ID || channel == TRANSPORT_ALL_CHANNEL_ID) {
		result &= RFM69_transportSend(nextRecipient, (void *)&outMsg->last, len, noACK);
	}
#endif
#if defined(MY_RADIO_RFM95)
	// cppcheck-suppress knownConditionTrueFalse
	if (channel == TRANSPORT_RFM95_CHANNEL_ID || channel == TRANSPORT_ALL_CHANNEL_ID) {
		result &= RFM95_transportSend(nextRecipient, (void *)&outMsg->last, len,
		                              noACK);
	}
#endif
#if defined(MY_RADIO_NRF5_ESB)
	// cppcheck-suppress knownConditionTrueFalse
	if (channel == TRANSPORT_NRF5_ESB_CHANNEL_ID || channel == TRANSPORT_ALL_CHANNEL_ID) {
		result &= NRF5_ESB_transportSend(nextRecipient, (void *)&outMsg->last, len,
		                                 noACK);
	}
#endif
#if defined(MY_RS485)
	// cppcheck-suppress knownConditionTrueFalse
	if (channel == TRANSPORT_RS485_CHANNEL_ID || channel == TRANSPORT_ALL_CHANNEL_ID) {
		result &= RS485_transportSend(nextRecipient, (void *)&outMsg->last, len,
		                              noACK);
	}
#endif
#if (MY_TRANSPORT_COUNT > 1)
	// if we receive a hwACK (result==true && !noACK) and message is not BC (checked in transportUpdateChannel() ), update channel table accordingly
	if (result && !noACK) {
		transportUpdateChannel(nextRecipient, channel);
	}
#endif

	TRANSPORT_HAL_DEBUG(PSTR("THA:SND:MSG LEN=%" PRIu8 ",RES=%" PRIu8 "\n"), len, result);
	return result; // or channel == TRANSPORT_ALL_CHANNEL_ID (broadcast / unknown route)
}

int16_t transportHALGetSendingRSSI(void)
{
#if (MY_TRANSPORT_COUNT == 1)
#if defined(MY_RADIO_RF24)
	return RF24_transportGetSendingRSSI();
#endif
#if defined(MY_RADIO_RFM69)
	return RFM69_transportGetSendingRSSI();
#endif
#if defined(MY_RADIO_RFM95)
	return RFM95_transportGetSendingRSSI();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	return NRF5_ESB_transportGetSendingRSSI();
#endif
#if defined(MY_RS485)
	return RS485_transportGetSendingRSSI();
#endif
#endif
	return FUNCTION_NOT_SUPPORTED;
}

int16_t transportHALGetReceivingRSSI(void)
{
#if (MY_TRANSPORT_COUNT == 1)
#if defined(MY_RADIO_RF24)
	return RF24_transportGetReceivingRSSI();
#endif
#if defined(MY_RADIO_RFM69)
	return RFM69_transportGetReceivingRSSI();
#endif
#if defined(MY_RADIO_RFM95)
	return RFM95_transportGetReceivingRSSI();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	return NRF5_ESB_transportGetReceivingRSSI();
#endif
#if defined(MY_RS485)
	return RS485_transportGetReceivingRSSI();
#endif
#endif
	return FUNCTION_NOT_SUPPORTED;
}

int16_t transportHALGetSendingSNR(void)
{
#if (MY_TRANSPORT_COUNT == 1)
#if defined(MY_RADIO_RF24)
	return RF24_transportGetSendingSNR();
#endif
#if defined(MY_RADIO_RFM69)
	return RFM69_transportGetSendingSNR();
#endif
#if defined(MY_RADIO_RFM95)
	return RFM95_transportGetSendingSNR();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	return NRF5_ESB_transportGetSendingSNR();
#endif
#if defined(MY_RS485)
	return RS485_transportGetSendingSNR();
#endif
#endif
	return FUNCTION_NOT_SUPPORTED;
}

int16_t transportHALGetReceivingSNR(void)
{
#if (MY_TRANSPORT_COUNT == 1)
#if defined(MY_RADIO_RF24)
	return RF24_transportGetReceivingSNR();
#endif
#if defined(MY_RADIO_RFM69)
	return RFM69_transportGetReceivingSNR();
#endif
#if defined(MY_RADIO_RFM95)
	return RFM95_transportGetReceivingSNR();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	return NRF5_ESB_transportGetReceivingSNR();
#endif
#if defined(MY_RS485)
	return RS485_transportGetReceivingSNR();
#endif
#endif
	return FUNCTION_NOT_SUPPORTED;
}

int16_t transportHALGetTxPowerPercent(void)
{
#if (MY_TRANSPORT_COUNT == 1)
#if defined(MY_RADIO_RF24)
	return RF24_transportGetTxPowerPercent();
#endif
#if defined(MY_RADIO_RFM69)
	return RFM69_transportGetTxPowerPercent();
#endif
#if defined(MY_RADIO_RFM95)
	return RFM95_transportGetTxPowerPercent();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	return NRF5_ESB_transportGetTxPowerPercent();
#endif
#if defined(MY_RS485)
	return RS485_transportGetTxPowerPercent();
#endif
#endif
	return FUNCTION_NOT_SUPPORTED;
}

bool transportHALSetTxPowerPercent(const uint8_t powerPercent)
{
#if (MY_TRANSPORT_COUNT == 1)
#if defined(MY_RADIO_RF24)
	return RF24_transportSetTxPowerPercent(powerPercent);
#endif
#if defined(MY_RADIO_RFM69)
	return RFM69_transportSetTxPowerPercent(powerPercent);
#endif
#if defined(MY_RADIO_RFM95)
	return RFM95_transportSetTxPowerPercent(powerPercent);
#endif
#if defined(MY_RADIO_NRF5_ESB)
	return NRF5_ESB_transportSetTxPowerPercent(powerPercent);
#endif
#if defined(MY_RS485)
	return RS485_transportSetTxPowerPercent(powerPercent);
#endif
#endif
	(void)powerPercent;
	return FUNCTION_NOT_SUPPORTED;
}

int16_t transportHALGetTxPowerLevel(void)
{
#if (MY_TRANSPORT_COUNT == 1)
#if defined(MY_RADIO_RF24)
	return RF24_transportGetTxPowerLevel();
#endif
#if defined(MY_RADIO_RFM69)
	return RFM69_transportGetTxPowerLevel();
#endif
#if defined(MY_RADIO_RFM95)
	return RFM95_transportGetTxPowerLevel();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	return NRF5_ESB_transportGetTxPowerLevel();
#endif
#if defined(MY_RS485)
	return RS485_transportGetTxPowerLevel();
#endif
#endif
	return FUNCTION_NOT_SUPPORTED;
}

void transportHALPowerDown(void)
{
#if defined(MY_RADIO_RF24)
	RF24_transportPowerDown();
#endif
#if defined(MY_RADIO_RFM69)
	RFM69_transportPowerDown();
#endif
#if defined(MY_RADIO_RFM95)
	RFM95_transportPowerDown();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	NRF5_ESB_transportPowerDown();
#endif
#if defined(MY_RS485)
	RS485_transportPowerDown();
#endif
}


void transportHALPowerUp(void)
{
#if defined(MY_RADIO_RF24)
	RF24_transportPowerUp();
#endif
#if defined(MY_RADIO_RFM69)
	RFM69_transportPowerUp();
#endif
#if defined(MY_RADIO_RFM95)
	RFM95_transportPowerUp();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	NRF5_ESB_transportPowerUp();
#endif
#if defined(MY_RS485)
	RS485_transportPowerUp();
#endif
}

void transportHALSleep(void)
{
#if defined(MY_RADIO_RF24)
	RF24_transportSleep();
#endif
#if defined(MY_RADIO_RFM69)
	RFM69_transportSleep();
#endif
#if defined(MY_RADIO_RFM95)
	RFM95_transportSleep();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	NRF5_ESB_transportSleep();
#endif
#if defined(MY_RS485)
	RS485_transportSleep();
#endif
}

void transportHALStandBy(void)
{
#if defined(MY_RADIO_RF24)
	RF24_transportStandBy();
#endif
#if defined(MY_RADIO_RFM69)
	RFM69_transportStandBy();
#endif
#if defined(MY_RADIO_RFM95)
	RFM95_transportStandBy();
#endif
#if defined(MY_RADIO_NRF5_ESB)
	NRF5_ESB_transportStandBy();
#endif
#if defined(MY_RS485)
	RS485_transportStandBy();
#endif
}
