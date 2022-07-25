/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * CAN bus transport added by Adam Słowik <slowik.adam@gmail.com>
 * Copyright (C) 2022 Adam Słowik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
#include "hal/transport/CAN/driver/mcp_can.h"
#include "hal/transport/CAN/driver/mcp_can.cpp"
#include "MyTransportCAN.h"
#if defined(MY_DEBUG_VERBOSE_CAN)
#define CAN_DEBUG(x,...)	DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< Debug print
#else
#define CAN_DEBUG(x,...)	//!< DEBUG null
#endif
MCP_CAN CAN0(MY_CAN_CS);
bool canInitialized = false;

//input buffer for raw data (from library).
long unsigned int rxId;
unsigned char len = 0;
unsigned char rxBuf[8];
unsigned char _nodeId;

//message id updated for every outgoing mesage
uint8_t message_id = 0;

//buffer element
typedef struct {
	uint8_t len;
	uint8_t data[MAX_MESSAGE_SIZE];
	uint8_t address;
	uint8_t lastReceivedPart;
	bool locked;
	uint8_t age;
	uint8_t packetId;
	bool ready;
} CAN_Packet;

//buffer
CAN_Packet packets[MY_CAN_BUF_SIZE];

//filter incoming messages (MCP2515 feature).
bool _initFilters()
{
	if (!canInitialized) {
		return false;
	}
	uint8_t err = 0;
	err += CAN0.setMode(MODE_CONFIG);

	err += CAN0.init_Mask(0, 1,
	                      0x0000FF00);                // Init first mask. Only destination address will be used to filter messages
	err += CAN0.init_Filt(0, 1, BROADCAST_ADDRESS <<
	                      8);      // Init first filter. Accept broadcast messages.
	err += CAN0.init_Filt(1, 1,
	                      _nodeId << 8);                // Init second filter. Accept messages send to this node.
	//second mask and filters need to be set. Otherwise all messages would be accepted.
	err += CAN0.init_Mask(1, 1, 0xFFFFFFFF);                // Init second mask.
	err += CAN0.init_Filt(2, 1, 0xFFFFFFFF);                // Init third filter.
	err += CAN0.init_Filt(3, 1, 0xFFFFFFFF);                // Init fourth filter.
	err += CAN0.init_Filt(4, 1, 0xFFFFFFFF);                // Init fifth filter.
	err += CAN0.init_Filt(5, 1, 0xFFFFFFFF);                // Init sixth filter.
	err += CAN0.setMode(MCP_NORMAL);
	hwPinMode(MY_CAN_INT, INPUT);
	return err == 0;
}

bool transportInit(void)
{
	CAN_DEBUG(PSTR("CAN:INIT:CS=%" PRIu8 ",INT=%" PRIu8 ",SPE=%" PRIu8 ",CLK=%" PRIu8 "\n"), MY_CAN_CS,
	          MY_CAN_INT, MY_CAN_SPEED, MY_CAN_CLOCK);

	if (CAN0.begin(MCP_STDEXT, MY_CAN_SPEED, MY_CAN_CLOCK) != CAN_OK) {
		canInitialized = false;
		return false;
	}
	canInitialized = true;
	for (uint8_t i = 0; i < MY_CAN_BUF_SIZE; i++) {
		_cleanSlot(i);
	}
	return _initFilters();
}

//clear single slot in buffer.
void _cleanSlot(uint8_t slot)
{
	packets[slot].locked = false;
	packets[slot].len = 0;
	packets[slot].address = 0;
	packets[slot].lastReceivedPart = 0;
	packets[slot].age = 0;
	packets[slot].packetId = 0;
	packets[slot].ready = false;
}

//find empty slot in buffer
uint8_t _findCanPacketSlot()
{
	uint8_t slot = MY_CAN_BUF_SIZE;
	uint8_t i;
	for (i = 0; i < MY_CAN_BUF_SIZE; i++) {
		if (packets[i].locked) {
			packets[i].age++;
		} else {
			slot = i;
		}
	}
	if (slot < MY_CAN_BUF_SIZE) {
		return slot;
	}
	//if empty slot not found. Clear oldest message.
	slot = 0;
	for (i = 1; i < MY_CAN_BUF_SIZE; i++) {
		if (packets[i].age > packets[slot].age) {
			slot = i;
		}
	}
	_cleanSlot(slot);
	CAN_DEBUG(PSTR("!CAN:RCV:SLOT=%" PRIu8 " message dropped\n"), slot);

	return slot;
}

//find slot with previous data parts.
uint8_t _findCanPacketSlot(long unsigned int from, long unsigned int currentPart,
                           long unsigned int messageId)
{
	uint8_t slot = MY_CAN_BUF_SIZE;
	uint8_t i;
	for (i = 0; i < MY_CAN_BUF_SIZE; i++) {
#if defined(MY_DEBUG_VERBOSE_CAN_INTERNAL)
		CAN_DEBUG(PSTR("CAN:RCV:LCK=%" PRIu8 ",ADDR=%" PRIu8
		               ",PACK_ID=%" PRIu8 ",LAST_PART=%" PRIu8 "\n"), packets[i].locked, packets[i].address,
		          packets[i].packetId,
		          packets[i].lastReceivedPart);
#endif
		if (packets[i].locked && packets[i].address == from && packets[i].packetId == messageId &&
		        packets[i].lastReceivedPart == currentPart) {
			slot = i;
			break;
		}
	}
	if (slot == MY_CAN_BUF_SIZE) {
		CAN_DEBUG(PSTR("!CAN:RCV:proper slot not found\n"));
	}
	return slot;
}

// from address 8bits (A)
// to address 8 bits (B)
// current part number 4 bits (C)
// total part count 4 bits (D)
// 3 bits message_id (E)
// 1 bit require ack (F)
// 1 bit is ack (G)
// 1 bit is extended frame (H). (FIXED)
// 1 bit RTR (Remote Transmission Request) (I) (FIXED)
// 1 bit SRR (Substitute Remote Request)  (J) (FIXED)
// header model (32 bits)
// HIJG FEEE DDDD CCCC BBBB BBBB AAAA AAAA
long unsigned int _buildHeader(uint8_t messageId, uint8_t totalPartCount, uint8_t currentPartNumber,
                               uint8_t toAddress, uint8_t fromAddress)
{
	long unsigned int header =
	    0x80; // set H=1 (FIXED), I=0 (FIXED), J=0 (FIXED), G=0 (To be implemented), F=0 (To be implemented)
	header += (messageId & 0x07); //set messageId
	header = header << 4;
	header += (totalPartCount & 0x0F);//set total part count
	header = header << 4;
	header += (currentPartNumber & 0x0F);//set current part number
	header = header << 8;
	header += toAddress;//set destination address
	header = header << 8;
	header += fromAddress;//set source address
	CAN_DEBUG(PSTR("CAN:SND:CANH=%" PRIu32 ",ID=%" PRIu8 ",TOTAL=%" PRIu8 ",CURR=%" PRIu8 ",TO=%" PRIu8
	               ",FROM=%" PRIu8 "\n"),
	          header, messageId, totalPartCount, currentPartNumber, toAddress, fromAddress);
	return header;
}

bool transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	(void) noACK;    // some ack is provided by CAN itself. TODO implement application layer ack.
	const char *datap = static_cast<char const *>(data);
	//calculate number of frames
	uint8_t noOfFrames = len / 8;
	if (len % 8 != 0) {
		noOfFrames++;
	}
	//update message_id
	message_id++;
	//make sure message_id isn't longer than 3 bits.
	message_id = message_id & 0x07;

	CAN_DEBUG(PSTR("CAN:SND:LN=%" PRIu8 ",NOF=%" PRIu8 "\n"), len, noOfFrames);
	uint8_t currentFrame;
	for (currentFrame = 0; currentFrame < noOfFrames; currentFrame++) {
		uint8_t partLen;
		if (len <= 8) {
			partLen = len;
		} else if (currentFrame * 8 + 8 <= len) {
			partLen = 8;
		} else {
			partLen = len % 8;
		}
		uint8_t buff[8];
		uint8_t j = 0;
		for (j = 0; j < partLen; j++) {
			buff[j] = datap[currentFrame * 8 + j];
		}

		CAN_DEBUG(PSTR("CAN:SND:LN=%" PRIu8 ",DTA0=%" PRIu8 ",DTA1=%" PRIu8 ",DTA2=%" PRIu8 ",DTA3=%" PRIu8
		               ",DTA4=%" PRIu8 ",DTA5=%" PRIu8 ",DTA6=%" PRIu8 ",DTA7=%" PRIu8 "\n"),
		          partLen, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5], buff[6], buff[7]);

		byte sndStat = CAN0.sendMsgBuf(_buildHeader(message_id, noOfFrames, currentFrame, to, _nodeId),
		                               partLen, buff);
		if (sndStat == CAN_OK) {
			CAN_DEBUG(PSTR("CAN:SND:OK\n"));
		} else {
			CAN_DEBUG(PSTR("!CAN:SND:FAIL\n"));
			return false;
		}
	}
	return true;
}

bool transportDataAvailable(void)
{
	if (!hwDigitalRead(MY_CAN_INT)) {             // If CAN_INT pin is low, read receive buffer
		CAN0.readMsgBuf(&rxId, &len, rxBuf);      // Read data: len = data length, buf = data byte(s)
		long unsigned int from = (rxId & 0x000000FF);
		// cppcheck-suppress unreadVariable
		long unsigned int currentPart = (rxId & 0x000F0000) >> 16;
		long unsigned int totalPartCount = (rxId & 0x00F00000) >> 20;
		long unsigned int messageId = (rxId & 0x07000000) >> 24;
#if defined(MY_DEBUG_VERBOSE_CAN)
		long unsigned int to = (rxId & 0x0000FF00) >> 8;
		CAN_DEBUG(PSTR("CAN:RCV:CANH=%" PRIu32 ",ID=%" PRIu32 ",TOTAL=%" PRIu32 ",CURR=%" PRIu32 ",TO=%"
		               PRIu32 ",FROM=%" PRIu32 "\n"),
		          rxId, messageId, totalPartCount, currentPart, to, from);
		CAN_DEBUG(PSTR("CAN:RCV:LN=%" PRIu8 ",DTA0=%" PRIu8 ",DTA1=%" PRIu8 ",DTA2=%" PRIu8 ",DTA3=%" PRIu8
		               ",DTA4=%" PRIu8 ",DTA5=%" PRIu8 ",DTA6=%" PRIu8 ",DTA7=%" PRIu8 "\n"),
		          len, rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4], rxBuf[5], rxBuf[6], rxBuf[7]);
#endif
		uint8_t slot;
		if (currentPart == 0) {
			slot = _findCanPacketSlot();
			packets[slot].locked = true;
			packets[slot].address = from;
			packets[slot].packetId = messageId;
		} else {
			slot = _findCanPacketSlot(from, currentPart, messageId);
		}
		if (slot != MY_CAN_BUF_SIZE) {
			memcpy(packets[slot].data + packets[slot].len, rxBuf, len);
			packets[slot].lastReceivedPart++;
			packets[slot].len += len;
			CAN_DEBUG(PSTR("CAN:RCV:SLOT=%" PRIu8 ",PART=%" PRIu8 "\n"), slot, packets[slot].lastReceivedPart);
			if (packets[slot].lastReceivedPart == totalPartCount) {
				packets[slot].ready = true;
				CAN_DEBUG(PSTR("CAN:RCV:SLOT=%" PRIu8 " complete\n"), slot);
				return true;
			}

		}
	}
	return false;
}

uint8_t transportReceive(void *data)
{
	uint8_t slot = MY_CAN_BUF_SIZE;
	uint8_t i;
	for (i = 0; i < MY_CAN_BUF_SIZE; i++) {
		if (packets[i].ready) {
			slot = i;
		}
	}
	if (slot < MY_CAN_BUF_SIZE) {
		memcpy(data, packets[slot].data, packets[slot].len);
		i = packets[slot].len;
		_cleanSlot(slot);
		return i;
	} else {
		return (0);
	}
}

void transportSetAddress(const uint8_t address)
{
	_nodeId = address;
	_initFilters();
}

uint8_t transportGetAddress(void)
{
	return _nodeId;
}

bool transportSanityCheck(void)
{
	// not implemented yet
	return true;
}

void transportPowerDown(void)
{
	// Nothing to shut down here
}

void transportPowerUp(void)
{
	// not implemented
}

void transportSleep(void)
{
	// not implemented
}

void transportStandBy(void)
{
	// not implemented
}

int16_t transportGetSendingRSSI(void)
{
	// not implemented
	return INVALID_RSSI;
}

int16_t transportGetReceivingRSSI(void)
{
	// not implemented
	return INVALID_RSSI;
}

int16_t transportGetSendingSNR(void)
{
	// not implemented
	return INVALID_SNR;
}

int16_t transportGetReceivingSNR(void)
{
	// not implemented
	return INVALID_SNR;
}

int16_t transportGetTxPowerPercent(void)
{
	// not implemented
	return static_cast<int16_t>(100);
}

int16_t transportGetTxPowerLevel(void)
{
	// not implemented
	return static_cast<int16_t>(100);
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	// not possible
	(void) powerPercent;
	return false;
}
