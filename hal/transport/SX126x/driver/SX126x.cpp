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
 * Based on Olivier Mauti's SX126x driver
 *
 * SX126x driver for MySensors, Copyright (C) 2020 Eduard Iten <eduard@iten.pro>
 *
 */

#include "SX126x.h"

// debug
#if defined(MY_DEBUG_VERBOSE_SX126x)
#define SX126x_DEBUG(x, ...) DEBUG_OUTPUT(x, ##__VA_ARGS__) //!< Debug print
#else
#define SX126x_DEBUG(x, ...) //!< DEBUG null
#endif

//Global status variable
static sx126x_internal_t SX126x;

//helper funcions
#define SX126x_internalToSNR(internalSNR) internalSNR/4

static sx126x_RSSI_t SX126x_RSSItoInternal(const int16_t externalRSSI)
{
	return static_cast<sx126x_RSSI_t>(constrain(externalRSSI + SX126x_RSSI_OFFSET, 0, 255));
}

static int16_t SX126x_internalToRSSI(const sx126x_RSSI_t internalRSSI)
{
	return static_cast<int16_t>(internalRSSI - SX126x_RSSI_OFFSET);
}

static bool SX126x_initialise()
{
	// setting pin modes
	SX126x_DEBUG(PSTR("SX126x:INIT\n"));
#if defined(MY_SX126x_POWER_PIN)
	hwPinMode(MY_SX_126x_POWER_PIN, OUTPUT);
	SX126x_powerUp();
	SX126x_DEBUG(PSTR("SX126x:INIT:PWRPIN=%u\n"), MY_SX126x_POWER_PIN);
#endif
#if defined(MY_SX126x_BUSY_PIN)
	hwPinMode(MY_SX126x_BUSY_PIN, INPUT);
	SX126x_DEBUG(PSTR("SX126x:INIT:BSYPIN=%u\n"), MY_SX126x_BUSY_PIN);
#endif
#if defined(MY_SX126x_IRQ_PIN)
	hwPinMode(MY_SX126x_IRQ_PIN, INPUT);
	SX126x_DEBUG(PSTR("SX126x:INIT:IRQPIN=%u\n"), MY_SX126x_IRQ_PIN);
#endif
#if defined(MY_SX126x_RESET_PIN)
	hwPinMode(MY_SX126x_RESET_PIN, OUTPUT);
	hwDigitalWrite(MY_SX126x_RESET_PIN, LOW);
	delay(SX126x_POWERUP_DELAY_MS);
	hwDigitalWrite(MY_SX126x_RESET_PIN, HIGH);
	delay(SX126x_POWERUP_DELAY_MS);			//wait until SX126x is ready
	SX126x_DEBUG(PSTR("SX126x:INIT:RSTPIN=%u\n"), MY_SX126x_RESET_PIN);
#endif
#ifdef MY_SX126x_ANT_SWITCH_PIN
	hwPinMode(MY_SX126x_ANT_SWITCH_PIN, OUTPUT);
	hwDigitalWrite(MY_SX126x_ANT_SWITCH_PIN, LOW);
	SX126x_DEBUG(PSTR("SX126x:INIT:ASWPIN=%u\n"), MY_SX126x_ANT_SWITCH_PIN);
#endif

#if !defined(__linux__)
	hwDigitalWrite(MY_SX126x_CS_PIN, HIGH);
	hwPinMode(MY_SX126x_CS_PIN, OUTPUT);
#endif
	SX126x_SPI.begin();

	SX126x.address = SX126x_BROADCAST_ADDRESS;
	SX126x.ackReceived = false;
	SX126x.dataReceived = false;
	SX126x.txSequenceNumber = 0;
	SX126x.powerLevel = 0;
	SX126x.targetRSSI = MY_SX126x_ATC_TARGET_DBM;
	SX126x.ATCenabled = false;

	SX126x_sleep();
	SX126x_wakeUp();
	SX126x_standBy();
	//are we using a TCXO and if so, is it controlled by the SX126x
#if defined(MY_SX126x_USE_TCXO)
#if defined(MY_SX126x_TCXO_VOLTAGE)
	uint8_t tcxoParameters[4];
	uint32_t tcxoDelay = MY_SX126c_TCXO_STARTUP_DELAY <<
	                     6; // Delay has to be set as multiples of 16.25uS
	tcxoParameters[0] = MY_SX126x_TCXO_VOLTAGE;
	tcxoParameters[1] = (tcxoDelay >> 16) & 0xFF;
	tcxoParameters[2] = (tcxoDelay >> 8) & 0xFF;
	tcxoParameters[3] = tcxoDelay & 0xFF;
	SX126x_sendCommand(SX126x_SET_TCXOMODE, tcxoParameters, 4);
	SX126x_DEBUG(PSTR("SX126x:INIT:DIO3TCXO,VCONF:%02X,DELAY:%ums\n"), MY_SX126x_TCXO_VOLTAGE,
	             MY_SX126c_TCXO_STARTUP_DELAY);
#else
	SX126x_DEBUG(PSTR("SX126x:INIT:TCXO,EXT\n"));
#endif //MY_SX126x_USE_TCXO
#endif //MY_SX126x_TCXO_VOLTAGE

	// recallibrate all oscillators, needs to be done in STDBY_RC
	SX126x_sendCommand(SX126x_CALIBRATE, 0x7F);

	// Antenna RX/TX switch logic
#if defined(MY_SX126x_USE_DIO2_ANT_SWITCH) && defined(MY_SX126x_ANT_SWITCH_PIN)
#error MY_SX126x_USE_DIO2_ANT_SWITCH and MY_SX126x_ANT_SWITCH_PIN both defined which makes no sense
#endif
#if !defined(MY_SX126x_USE_DIO2_ANT_SWITCH) && !defined(MY_SX126x_ANT_SWITCH_PIN)
#error Eighter MY_SX126x_USE_DIO2_ANT_SWITCH or MY_SX126x_ANT_SWITCH_PIN has to be defined
#endif
#if defined(MY_SX126x_USE_DIO2_ANT_SWITCH)
	SX126x_sendCommand(SX126x_SET_RFSWITCHMODE, true);
	SX126x_DEBUG(PSTR("SX126x:INIT:DIO2AntSw\n"));
#endif

	// set regulator to DC/DC
	SX126x_standBy();
	SX126x_sendCommand(SX126x_SET_REGULATORMODE, 0x01);

	// set buffer base addresses
	uint8_t bufBaseAddresses[2];
	bufBaseAddresses[0] = 0x00; //<! tx base address
	bufBaseAddresses[1] = 0x00; //<! rx base address
	SX126x_sendCommand(SX126x_SET_BUFFERBASEADDRESS, bufBaseAddresses, 2);

	// set power
	SX126x_txPower(MY_SX126x_TX_POWER_DBM);
	sx126x_modulationParams_t modulationParams;

	// set frequency
	SX126x_setFrequency(MY_SX126x_FREQUENCY);

	// configure modem for lora
	SX126x_standBy();
	SX126x_sendCommand(SX126x_SET_PACKETTYPE, (uint8_t)PACKET_TYPE_LORA);
	modulationParams.fields.bandwidth = MY_SX126x_LORA_BW;
	modulationParams.fields.codingRate = MY_SX126x_LORA_CR;
	modulationParams.fields.spreadingFactor = MY_SX126x_LORA_SF;
	modulationParams.fields.lowDatarateOptimize = false;
	SX126x_sendCommand(SX126x_SET_MODULATIONPARAMS, modulationParams.values, 4);
	SX126x_setPacketParameters(0xFF);

	// disable and clear all interrupts
	attachInterrupt(MY_SX126x_IRQ_NUM, SX126x_interruptHandler, RISING);
	SX126x_setIrqMask(SX126x_IRQ_NONE);
	SX126x_clearIrq(SX126x_IRQ_ALL);

	// set LoRa syncword
	SX126x_sendRegister(SX126x_REG_LORASW, 0x12);

	if (SX126x_sanityCheck()) {
		return true;
	}
	SX126x_DEBUG(PSTR("!SX126x:INIT:SANCHK FAIL\n"));
	return false;
}

static void SX126x_interruptHandler()
{
	noInterrupts();
	SX126x.irqFired = true;
	SX126x.radioMode = SX126x_MODE_STDBY_RC;
	interrupts();
}

static void SX126x_handle()
{
#ifdef MY_SX126x_IRQ_PIN
	if (SX126x.irqFired) {
#endif
		uint32_t irqStatus = SX126x_IRQ_NONE;
		uint8_t irqBuffer[3];

		//get the interrupt fired
		SX126x_readCommand(SX126x_GET_IRQSTATUS, irqBuffer, 2);
		if (irqBuffer[0] | irqBuffer[1]) {
			irqStatus = irqBuffer[0] << 8 | irqBuffer[1];
			//Transmission done
			if (irqStatus & SX126x_IRQ_TX_DONE) {
				SX126x.txComplete = true;
#ifdef MY_SX126x_ANT_SWITCH_PIN
				hwDigitalWrite(MY_SX126x_ANT_SWITCH_PIN, LOW);
#endif
				SX126x_rx();
			}

			//Reception done
			if (irqStatus & SX126x_IRQ_RX_DONE) {
				sx126x_rxBufferStatus_t bufferStatus;
				sx126x_packetStatus_t packetStatus;
				SX126x_readCommand(SX126x_GET_RXBUFFERSTATUS, bufferStatus.values, 2);
				bufferStatus.fields.payloadLength = min(bufferStatus.fields.payloadLength, SX126x_MAX_PACKET_LEN);
				SX126x.currentPacket.payloadLen = bufferStatus.fields.payloadLength - SX126x_HEADER_LEN;
				SX126x_readBuffer(
				    bufferStatus.fields.startPointer,
				    SX126x.currentPacket.data,
				    bufferStatus.fields.payloadLength);
				SX126x_readCommand(SX126x_GET_PACKETSTATUS, packetStatus.values, 3);
				SX126x.currentPacket.RSSI = SX126x_RSSItoInternal(-packetStatus.fields.rawRssiPkt / 2);
				SX126x.currentPacket.SNR = packetStatus.fields.rawSnrPkt;
				if ((SX126x.currentPacket.header.version >= SX126x_MIN_PACKET_HEADER_VERSION) &&
				        (SX126x_PROMISCUOUS || SX126x.currentPacket.header.recipient == SX126x.address ||
				         SX126x.currentPacket.header.recipient == SX126x_BROADCAST_ADDRESS)) {
					// Message for us
					SX126x.ackReceived = SX126x.currentPacket.header.controlFlags.fields.ackReceived &&
					                     !SX126x.currentPacket.header.controlFlags.fields.ackRequested;
					SX126x.dataReceived = !SX126x.ackReceived;
				}
				//SX126x_rx();
			}

			//CAD done
			if (irqStatus & SX126x_IRQ_CAD_DONE) {
				SX126x.channelFree = true;
				SX126x.radioMode = SX126x_MODE_STDBY_RC;
			}

			//CAD channel active
			if (irqStatus & SX126x_IRQ_CAD_ACTIVITY_DETECTED) {
				SX126x.channelActive = true;
				SX126x.radioMode = SX126x_MODE_RX;
			}

			SX126x_clearIrq(SX126x_IRQ_ALL);
			SX126x.irqFired = false;
		}
#ifdef MY_SX126x_IRQ_PIN
	}
#endif
}

void SX126x_deviceReady(void)
{
	if (SX126x.radioMode == SX126x_MODE_SLEEP || SX126x.radioMode == SX126x_MODE_RX_DC) {
		SX126x_wakeUp();
	}
	SX126x_busy();
	SX126x.radioMode = SX126x_MODE_STDBY_RC;
}

static void SX126x_wakeUp()
{
	noInterrupts();
	hwDigitalWrite(MY_SX126x_CS_PIN, LOW);
	SX126x_SPI.transfer(SX126x_GET_STATUS);
	SX126x_SPI.transfer(0x00);
	hwDigitalWrite(MY_SX126x_CS_PIN, HIGH);
	interrupts();
}

static void SX126x_standBy()
{
	SX126x_deviceReady();
	SX126x_sendCommand(SX126x_SET_STANDBY, SX126x_STDBY_RC);
	SX126x.radioMode = SX126x_MODE_STDBY_RC;
}

static void SX126x_busy(void)
{
#ifdef MY_SX126x_BUSY_PIN
	while (hwDigitalRead(MY_SX126x_BUSY_PIN) == 1) {
	}
#else
	delay(1);
#endif
}

static void SX126x_sleep(void)
{
	SX126x_deviceReady();
	sx126x_sleepParams_t sleepParams = { 0 };
	sleepParams.fields.warmStart = true;
	SX126x_sendCommand(SX126x_SET_SLEEP, &sleepParams.value, sizeof(sleepParams.value));
	delayMicroseconds(500);
	SX126x.radioMode = SX126x_MODE_SLEEP;
}

static bool SX126x_txPower(sx126x_powerLevel_t power)
{
	sx126x_paSettings_t paSettings = { 0, 0, 0, 0x01 };
	paSettings.fields.paLut = 0x01;
#if (SX126x_VARIANT == 1)
	paSettings.fields.hpMax = 0x00;
	paSettings.fields.deviceSel = 0x01;
	if (power >= 15) {
		paSettings.fields.paDutyCycle = 0x06;
	} else {
		paSettings.fields.paDutyCylce = 0x04;
	}
	if (power >=14) {
		power = 14;
	} else if (power <= -3) {
		power = -3;
	}
	SX126x_sendRegister(SX126x_REG_OCP, 0x18); // 80mA over current protection
#else if (SX126xVARIANT == 2)
	paSettings.fields.deviceSel = 0x00;
	power = constrain(power, -9, 22);
	power = constrain(power, MY_SX126x_MIN_POWER_LEVEL_DBM, MY_SX126x_MAX_POWER_LEVEL_DBM);
	SX126x.powerLevel = power;
	paSettings.fields.paDutyCycle = 0x04;
	paSettings.fields.hpMax = 0x07;
	SX126x_sendRegister(SX126x_REG_OCP, 0x38); // 160mA over current protection
#endif

	SX126x_sendCommand(SX126x_SET_PACONFIG, paSettings.values, 4);
	sx126x_txSettings_t txSettings;
	txSettings.fields.power = power;
	txSettings.fields.rampTime = RADIO_RAMP_200_US;
	SX126x_sendCommand(SX126x_SET_TXPARAMS, txSettings.values, 2);
	SX126x.powerLevel = power;
	SX126x_DEBUG(PSTR("SX126x:PTC:LEVEL=%d"), SX126x.powerLevel);
	return true;
}

static void SX126x_sendCommand(sx126x_commands_t command, uint8_t *buffer, uint16_t size)
{
	SX126x_busy();
	hwDigitalWrite(MY_SX126x_CS_PIN, LOW);
	SX126x_SPI.transfer(command);
	for (int i = 0; i < size; i++) {
		SX126x_SPI.transfer(buffer[i]);
	}
	hwDigitalWrite(MY_SX126x_CS_PIN, HIGH);
	if (command != SX126x_SET_SLEEP) {
		SX126x_busy();
	}
}

static void SX126x_sendCommand(sx126x_commands_t command, uint8_t parameter)
{
	uint8_t *buffer;
	buffer = &parameter;
	SX126x_sendCommand(command, buffer, 1);
}

static void SX126x_readCommand(sx126x_commands_t command, uint8_t *buffer, uint16_t size)
{
	SX126x_busy();
	hwDigitalWrite(MY_SX126x_CS_PIN, LOW);
	SX126x_SPI.transfer(command);
	SX126x_SPI.transfer(0x00);
	for (int i = 0; i < size; i++) {
		buffer[i] = SX126x_SPI.transfer(0x00);
	}
	hwDigitalWrite(MY_SX126x_CS_PIN, HIGH);
	SX126x_busy();
}

void SX126x_sendRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
	SX126x_busy();
	hwDigitalWrite(MY_SX126x_CS_PIN, LOW);
	SX126x_SPI.transferBytes(NULL, buffer, size);
	hwDigitalWrite(MY_SX126x_CS_PIN, HIGH);
	SX126x_busy();
}

void SX126x_sendRegister(uint16_t address, uint8_t value)
{
	SX126x_sendRegisters(address, &value, 1);
}

static void SX126x_setFrequency(uint32_t frequency)
{
	uint8_t buf[4];
	uint32_t freq;

	// image rejection calibration
	if (frequency > 900000000) {
		buf[0] = 0xE1;
		buf[1] = 0xE9;
	} else if (frequency > 850000000) {
		buf[0] = 0xD7;
		buf[1] = 0xD8;
	} else if (frequency > 770000000) {
		buf[0] = 0xC1;
		buf[1] = 0xC5;
	} else if (frequency > 460000000) {
		buf[0] = 0x75;
		buf[1] = 0x81;
	} else if (frequency > 425000000) {
		buf[0] = 0x6B;
		buf[1] = 0x6F;
	}

	SX126x_setIrqMask(SX126x_IRQ_NONE);
	SX126x_sendCommand(SX126x_CALIBRATEIMAGE, buf, 2);

	// calculate and send PLL parameters
	freq = (uint32_t)((double)frequency / (double)SX126x_FREQ_STEP);
	buf[0] = (uint8_t)((freq >> 24) & 0xFF);
	buf[1] = (uint8_t)((freq >> 16) & 0xFF);
	buf[2] = (uint8_t)((freq >> 8) & 0xFF);
	buf[3] = (uint8_t)(freq & 0xFF);
	SX126x_sendCommand(SX126x_SET_RFFREQUENCY, buf, 4);
}

static void SX126x_setIrqMask(uint16_t mask)
{
	uint8_t buf[8];
	buf[0] = mask >> 8 & 0xFF;
	buf[1] = mask & 0xFF;
	buf[2] = buf[0];
	buf[3] = buf[1];
	buf[4] = SX126x_IRQ_NONE >> 8 & 0xFF; //we do not use DIO2 and DIO3 as IRQ
	buf[5] = SX126x_IRQ_NONE & 0xFF;
	buf[6] = buf[4];
	buf[7] = buf[5];
	SX126x_sendCommand(SX126x_CFG_DIOIRQ, buf, 8);
}

static void SX126x_clearIrq(uint16_t mask)
{
	uint8_t buf[2];
	buf[0] = mask >> 8 & 0xFF;
	buf[1] = mask & 0xFF;
	SX126x_sendCommand(SX126x_CLR_IRQSTATUS, buf, 2);
}

static void SX126x_setPacketParameters(uint8_t length)
{
	uint8_t buf[9] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	buf[0] = SX126x_PREAMBLE_LENGTH >> 8 & 0xFF;
	buf[1] = SX126x_PREAMBLE_LENGTH & 0xFF;
	buf[2] = 0x00;                  //!< Variable length packet
	buf[3] = length;                //!< length of payload
	buf[4] = 0x01;                  //!< CRC on
	buf[5] = 0x00;					//!< standard IRQ setup
	SX126x_sendCommand(SX126x_SET_PACKETPARAMS, buf, 6);
}

static bool SX126x_sanityCheck()
{
	uint8_t buf[2];
	SX126x_readCommand(SX126x_GET_PACKETTYPE, buf, 1);
	if (buf[0]!=0x01) {
		return false;
	}
	SX126x_readCommand(SX126x_GET_ERROR, buf, 2);
	if (buf[0] & 0x7F || buf[1] & 0x01) {
		SX126x_DEBUG(PSTR("!SX126x:INIT:ERR:0x%02X%02X"), buf[1], buf[2]);
		return false;
	}

	return true;
}

static void SX126x_setAddress(uint8_t address)
{
	SX126x.address = address;
}

static uint8_t SX126x_getAddress(void)
{
	return SX126x.address;
}

static bool SX126x_sendWithRetry(const uint8_t recipient, const void *buffer,
                                 const uint8_t bufferSize, const bool noACK)
{
	sx126x_controlFlags_t flags{ 0 };
	flags.fields.ackRequested = !noACK;
	SX126x.txSequenceNumber++;
	for (uint8_t retry = 0; retry < SX126x_RETRIES; retry++) {
		SX126x_DEBUG(PSTR("SX126x:SWR:SEND,TO=%u,SEQ=%u,RETRY=%u\n"),
		             recipient,
		             SX126x.txSequenceNumber,
		             retry);
		if (!SX126x_send(recipient, (uint8_t *)buffer, bufferSize, flags)) {
			return false;
		}
		SX126x_rx();
		if (noACK) {
			return true;
		}
		uint32_t start = hwMillis();
		while (hwMillis() < start + SX126x_RETRY_TIMEOUT_MS) {
			SX126x_handle();
			if (SX126x.ackReceived) {
				SX126x.ackReceived = false;
				//is it the ACK for our current packet?
				if (SX126x.currentPacket.header.sender == recipient &&
				        SX126x.currentPacket.ACK.sequenceNumber == SX126x.txSequenceNumber) {
					SX126x_DEBUG(PSTR("SX126x:SWR:ACK FROM=%u,SEQ=%u,RSSI=%d,SNR:%d\n"),
					             SX126x.currentPacket.header.sender,
					             SX126x.currentPacket.ACK.sequenceNumber,
					             SX126x_internalToRSSI(SX126x.currentPacket.ACK.RSSI),
					             SX126x_internalToSNR(SX126x.currentPacket.ACK.SNR));
					if (SX126x.ATCenabled) {
						SX126x_ATC();
					}
					return true;
				}
			}
			doYield();
		}
		SX126x_DEBUG(PSTR("!SX126x:SWR:NACK\n"));
		const uint32_t enterCSMAMS = hwMillis();
		const uint16_t randDelayCSMA = start % 100;
		while (hwMillis() - enterCSMAMS < randDelayCSMA) {
			doYield();
		}
		if (SX126x.ATCenabled) {
			SX126x_txPower(SX126x.powerLevel + 2); //increase power, maybe we are far away from gateway
		}
		return false;
	}

}

static bool SX126x_send(const uint8_t recipient, uint8_t *data, const uint8_t len,
                        const sx126x_controlFlags_t flags)
{
	sx126x_packet_t packet;
	packet.header.version = SX126x_PACKET_HEADER_VERSION;
	packet.header.sender = SX126x.address;
	packet.header.recipient = recipient;
	packet.payloadLen = min(len, (uint8_t)SX126x_MAX_PAYLOAD_LEN);
	packet.header.controlFlags = flags;
	memcpy((void *)&packet.payload, (void *)data, packet.payloadLen);
	return SX126x_sendPacket(&packet);
}

static bool SX126x_sendPacket(sx126x_packet_t *packet)
{
	if (!SX126x_cad()) {
		return false;
	}
	packet->header.sequenceNumber = SX126x.txSequenceNumber;
	uint8_t finalLength = packet->payloadLen + SX126x_HEADER_LEN;
	SX126x_setPacketParameters(finalLength);
	SX126x_sendBuffer(0x00, packet->data, finalLength);
	SX126x_tx();
	uint32_t txStart = hwMillis();
	while (!SX126x.txComplete) {
		SX126x_handle();
		if (millis() > txStart + MY_SX126x_TX_TIMEOUT_MS) {
			return false;
		}
	}
	SX126x.txComplete = false;
	return true;
}

static void SX126x_sendBuffer(const uint8_t offset, const uint8_t *buffer, const uint8_t size)
{
	SX126x_busy();
	hwDigitalWrite(MY_SX126x_CS_PIN, LOW);
	SX126x_SPI.transfer(SX126x_WRITE_BUFFER);
	SX126x_SPI.transfer(offset);
	for (int i = 0; i < size; i++) {
		SX126x_SPI.transfer(buffer[i]);
	}
	hwDigitalWrite(MY_SX126x_CS_PIN, HIGH);
	SX126x_busy();
}

static void SX126x_readBuffer(const uint8_t offset, uint8_t *buffer, const uint8_t size)
{
	SX126x_busy();
	hwDigitalWrite(MY_SX126x_CS_PIN, LOW);
	SX126x_SPI.transfer(SX126x_READ_BUFFER);
	SX126x_SPI.transfer(offset);
	SX126x_SPI.transfer(0x00); //discard status byte
	for (int i = 0; i < size; i++) {
		buffer[i] = SX126x_SPI.transfer(0x00);
	}
	hwDigitalWrite(MY_SX126x_CS_PIN, HIGH);
	SX126x_busy();
}

static void SX126x_tx()
{
	uint8_t timeout[3] = { 0x00, 0x00, 0x00 }; //no timeout
	SX126x_deviceReady();
	SX126x_setIrqMask(SX126x_IRQ_TX_DONE);
	SX126x_sendCommand(SX126x_SET_TX, timeout, 3);
#ifdef MY_SX126x_ANT_SWITCH_PIN
	hwDigitalWrite(MY_SX126x_ANT_SWITCH_PIN, HIGH);
#endif
	SX126x.radioMode = SX126x_MODE_TX;
}

static void SX126x_rx()
{
	uint8_t timeout[3] = { 0x00, 0x00, 0x00 }; //no timeout, go into standby after reception
	SX126x_deviceReady();
	SX126x_setIrqMask(SX126x_IRQ_RX_DONE | SX126x_IRQ_CRC_ERROR | SX126x_IRQ_RX_TX_TIMEOUT);
	SX126x_sendCommand(SX126x_SET_LORASYMBTIMEOUT, 0);
	SX126x_sendCommand(SX126x_SET_RX, timeout, 3);
	SX126x.radioMode = SX126x_MODE_RX;
}

static bool SX126x_cad()
{
	sx126x_cadParameters_t cadParameters;
	SX126x_deviceReady();
	//this are recommandations extrapolated from AN1200.48
	if (MY_SX126x_LORA_BW < LORA_BW_250) {
		switch (MY_SX126x_LORA_SF) {
		case LORA_SF5:
		case LORA_SF6:
		case LORA_SF7:
		case LORA_SF8:
			cadParameters.fields.cadDetPeak = 22;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_2_SYMB;
			break;
		case LORA_SF9:
			cadParameters.fields.cadDetPeak = 23;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF10:
			cadParameters.fields.cadDetPeak = 24;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF11:
			cadParameters.fields.cadDetPeak = 25;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF12:
		default:
			cadParameters.fields.cadDetPeak = 28;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		}
	} else {
		switch (MY_SX126x_LORA_SF) {
		case LORA_SF5:
		case LORA_SF6:
		case LORA_SF7:
			cadParameters.fields.cadDetPeak = 21;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF8:
			cadParameters.fields.cadDetPeak = 22;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF9:
			cadParameters.fields.cadDetPeak = 22;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF10:
			cadParameters.fields.cadDetPeak = 23;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF11:
			cadParameters.fields.cadDetPeak = 25;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_4_SYMB;
			break;
		case LORA_SF12:
			cadParameters.fields.cadDetPeak = 29;
			cadParameters.fields.cadSymbolsNum = LORA_CAD_ON_8_SYMB;
			break;
		}
	}
	cadParameters.fields.cadExitMode = SX126x_CAD_RX;
	cadParameters.fields.cadTimeout1 = 0x00;
	cadParameters.fields.cadTimeout2 = 0x00;
	cadParameters.fields.cadTimeout3 = 0x00;

	SX126x_standBy();
	SX126x_sendCommand(SX126x_SET_CADPARAMS, cadParameters.values, 7);
	SX126x_setIrqMask(SX126x_IRQ_CAD_ACTIVITY_DETECTED | SX126x_IRQ_CAD_DONE);
	SX126x.channelActive = false;
	SX126x.channelFree = false;
	SX126x_sendCommand(SX126x_SET_CAD, NULL, 0);
	SX126x.radioMode = SX126x_MODE_CAD;
	while (!SX126x.channelActive && !SX126x.channelFree) {
		SX126x_handle();
	}
	if (SX126x.channelFree) {
		return true;
	}
	SX126x_DEBUG(PSTR("!SX126x:CAD\n"));
	return false;
}

static bool SX126x_packetAvailable()
{
	if (SX126x.radioMode != SX126x_MODE_RX && SX126x.radioMode != SX126x_MODE_TX) {
		//if we are not sending or already in receive, go into receive;
		SX126x_rx();
	}
	return SX126x.dataReceived;
}

static uint8_t SX126x_getData(uint8_t *buffer, const uint8_t bufferSize)
{
	const uint8_t payloadSize = min(SX126x.currentPacket.payloadLen, bufferSize);
	if (buffer != NULL) {
		(void)memcpy((void *)buffer, (void *)&SX126x.currentPacket.payload, payloadSize);
	}
	// clear data flag
	SX126x.dataReceived = false;
	// ACK handling
	if (SX126x.currentPacket.header.controlFlags.fields.ackRequested &&
	        !SX126x.currentPacket.header.controlFlags.fields.ackReceived) {
#if defined(MY_GATEWAY_FEATURE) && (F_CPU>16*1000000ul)
		// delay for fast GW and slow nodes
		delay(50);
#endif
		SX126x_sendAck(
		    SX126x.currentPacket.header.sender,
		    SX126x.currentPacket.header.sequenceNumber,
		    SX126x.currentPacket.RSSI,
		    SX126x.currentPacket.SNR);
	}
	return payloadSize;
}

static void SX126x_sendAck(const uint8_t recipient, const sx126x_sequenceNumber_t sequenceNumber,
                           const sx126x_RSSI_t RSSI, const sx126x_SNR_t SNR)
{
	SX126x_DEBUG(PSTR("SX126x:SAC:SEND ACK,TO=%u,SEQ=%u,RSSI=%d,SNR=%u\n"),
	             recipient,
	             sequenceNumber,
	             SX126x_internalToRSSI(RSSI),
	             SX126x_internalToSNR(SNR));
	sx126x_ack_t ACK;
	ACK.sequenceNumber = sequenceNumber;
	ACK.RSSI = RSSI;
	ACK.SNR = SNR;
	sx126x_controlFlags_t flags = { 0 };
	flags.fields.ackReceived = true;
	flags.fields.ackRssiReport = true;
	(void)SX126x_send(recipient, (uint8_t *)&ACK, sizeof(sx126x_ack_t), flags);
}

static void SX126x_ATC()
{
	int8_t delta;
	sx126x_powerLevel_t newPowerLevel;
	delta = SX126x.targetRSSI - SX126x_internalToRSSI(SX126x.currentPacket.ACK.RSSI);
	sx126x_powerLevel_t oldPowerLevel = SX126x.powerLevel;
	newPowerLevel = SX126x.powerLevel + delta / 2;
	newPowerLevel = constrain(newPowerLevel, MY_SX126x_MIN_POWER_LEVEL_DBM,
	                          MY_SX126x_MAX_POWER_LEVEL_DBM);
	SX126x_DEBUG(PSTR("SX126x:ATC:cR=%d, tR=%d, rTXL=%d\n"),
	             SX126x_internalToRSSI(SX126x.currentPacket.ACK.RSSI),
	             SX126x.targetRSSI,
	             newPowerLevel
	            );
	if (newPowerLevel != SX126x.powerLevel) {
		SX126x_txPower(newPowerLevel);
	}
}

static void SX126x_setATC(bool onOff, int8_t targetRSSI)
{
	SX126x.ATCenabled = onOff;
	SX126x.targetRSSI = targetRSSI;
}

void SX126x_powerUp()
{
#ifdef MY_SX126x_POWER_PIN
	hwDigitalWrite(SX126x_POWER_PIN, HIGH);
	SX126x_DEBUG(PSTR("SX126x:PWU\n");
#endif
}

void SX126x_powerDown()
{
#ifdef MY_SX126x_POWER_PIN
	hwDigitalWrite(SX126x_POWER_PIN, LOW);
	SX126x_DEBUG(PSTR("SX126x:PWD\n");
#endif
}

static int16_t SX126x_getSendingRSSI(void)
{
	// own RSSI, as measured by the recipient - ACK part
	if (SX126x.currentPacket.header.controlFlags.fields.ackRssiReport) {
		return SX126x_internalToRSSI(SX126x.currentPacket.ACK.RSSI);
	} else {
		// not possible
		return INVALID_RSSI;
	}
}

static int16_t SX126x_getSendingSNR(void)
{
	// own SNR, as measured by the recipient - ACK part
	if (SX126x.currentPacket.header.controlFlags.fields.ackRssiReport) {
		return static_cast<int16_t>(SX126x_internalToSNR(SX126x.currentPacket.ACK.SNR));
	} else {
		// not possible
		return INVALID_SNR;
	}
}

static int16_t SX126x_getReceivingRSSI(void)
{
	// RSSI from last received packet
	return static_cast<int16_t>(SX126x_internalToRSSI(SX126x.currentPacket.RSSI));
}

static int16_t SX126x_getReceivingSNR(void)
{
	// SNR from last received packet
	return static_cast<int16_t>(SX126x_internalToSNR(SX126x.currentPacket.SNR));
}

static int8_t SX126x_getTxPowerLevel(void)
{
	return SX126x.powerLevel;
}

static uint8_t SX126x_getTxPowerPercent(void)
{
	// report TX level in %
	const uint8_t result = static_cast<uint8_t>(100.0f * (SX126x.powerLevel -
	                       MY_SX126x_MIN_POWER_LEVEL_DBM) /
	                       (MY_SX126x_MAX_POWER_LEVEL_DBM
	                        - MY_SX126x_MIN_POWER_LEVEL_DBM));
	return result;
}
static bool SX126x_setTxPowerPercent(const uint8_t newPowerPercent)
{
	const sx126x_powerLevel_t newPowerLevel = static_cast<sx126x_powerLevel_t>
	        (MY_SX126x_MIN_POWER_LEVEL_DBM + (MY_SX126x_MAX_POWER_LEVEL_DBM
	                - MY_SX126x_MIN_POWER_LEVEL_DBM) * (newPowerPercent / 100.0f));
	SX126x_DEBUG(PSTR("SX126x:SPP:PCT=%u,TX LEVEL=%n\n"), newPowerPercent, newPowerLevel);
	return SX126x_txPower(newPowerLevel);
}
