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
 *
 * Based on Mike McCauley's RFM95 library, Copyright (C) 2014 Mike McCauley <mikem@airspayce.com>
 * Radiohead http://www.airspayce.com/mikem/arduino/RadioHead/index.html
 *
 * RFM95 driver refactored and optimized for MySensors, Copyright (C) 2017-2018 Olivier Mauti <olivier@mysensors.org>
 *
 * Definitions for HopeRF LoRa radios:
 * http://www.hoperf.com/upload/rf/RFM95_96_97_98W.pdf
 *
 */

#include "RFM95.h"

// debug
#if defined(MY_DEBUG_VERBOSE_RFM95)
#define RFM95_DEBUG(x,...)	DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< Debug print
#else
#define RFM95_DEBUG(x,...)	//!< DEBUG null
#endif

rfm95_internal_t RFM95;	//!< internal variables
volatile uint8_t RFM95_irq; //<! rfm95 irq flag

#if defined(__linux__)
// SPI RX and TX buffers (max packet len + 1 byte for the command)
uint8_t RFM95_spi_rxbuff[RFM95_MAX_PACKET_LEN + 1];
uint8_t RFM95_spi_txbuff[RFM95_MAX_PACKET_LEN + 1];
#endif

LOCAL void RFM95_csn(const bool level)
{
#if defined(__linux__)
	(void)level;
#else
	hwDigitalWrite(MY_RFM95_CS_PIN, level);
#endif
}

LOCAL uint8_t RFM95_spiMultiByteTransfer(const uint8_t cmd, uint8_t *buf, uint8_t len,
        const bool aReadMode)
{
	uint8_t status;
	uint8_t *current = buf;
#if !defined(MY_SOFTSPI) && defined(SPI_HAS_TRANSACTION)
	RFM95_SPI.beginTransaction(SPISettings(MY_RFM95_SPI_SPEED, RFM95_SPI_DATA_ORDER,
	                                       RFM95_SPI_DATA_MODE));
#endif

	RFM95_csn(LOW);
#if defined(__linux__)
	uint8_t *prx = RFM95_spi_rxbuff;
	uint8_t *ptx = RFM95_spi_txbuff;
	uint8_t size = len + 1; // Add register value to transmit buffer

	*ptx++ = cmd;
	while (len--) {
		if (aReadMode) {
			*ptx++ = (uint8_t)RFM95_NOP;
		} else {
			*ptx++ = *current++;
		}
	}
	RFM95_SPI.transfernb((char *)RFM95_spi_txbuff, (char *)RFM95_spi_rxbuff, size);
	if (aReadMode) {
		if (size == 2) {
			status = *++prx;   // result is 2nd byte of receive buffer
		} else {
			status = *prx++; // status is 1st byte of receive buffer
			// decrement before to skip status byte
			while (--size && (buf != NULL)) {
				*buf++ = *prx++;
			}
		}
	} else {
		status = *prx; // status is 1st byte of receive buffer
	}
#else
	status = RFM95_SPI.transfer(cmd);
	while (len--) {
		if (aReadMode) {
			status = RFM95_SPI.transfer((uint8_t)RFM95_NOP);
			if (buf != NULL) {
				*current++ = status;
			}
		} else {
			status = RFM95_SPI.transfer(*current++);
		}
	}
#endif
	RFM95_csn(HIGH);

#if !defined(MY_SOFTSPI) && defined(SPI_HAS_TRANSACTION)
	RFM95_SPI.endTransaction();
#endif
	return status;
}

// low level register access
LOCAL uint8_t RFM95_RAW_readByteRegister(const uint8_t address)
{
	return RFM95_spiMultiByteTransfer(address, NULL, 1, true);
}

// low level register access
LOCAL uint8_t RFM95_RAW_writeByteRegister(const uint8_t address, uint8_t value)
{
	return RFM95_spiMultiByteTransfer(address, &value, 1, false);
}

// helper functions
LOCAL inline uint8_t RFM95_readReg(const uint8_t reg)
{
	return RFM95_RAW_readByteRegister(reg & RFM95_READ_REGISTER);
}

LOCAL inline uint8_t RFM95_writeReg(const uint8_t reg, const uint8_t value)
{
	return RFM95_RAW_writeByteRegister(reg | RFM95_WRITE_REGISTER, value);
}

LOCAL inline uint8_t RFM95_burstReadReg(const uint8_t reg, void *buf, uint8_t len)
{
	return RFM95_spiMultiByteTransfer(reg & RFM95_READ_REGISTER, (uint8_t *)buf, len, true);
}

LOCAL inline uint8_t RFM95_burstWriteReg(const uint8_t reg, const void *buf, uint8_t len)
{
	return RFM95_spiMultiByteTransfer(reg | RFM95_WRITE_REGISTER, (uint8_t *)buf, len, false);
}

LOCAL inline rfm95_RSSI_t RFM95_RSSItoInternal(const int16_t externalRSSI)
{
	return static_cast<rfm95_RSSI_t>(externalRSSI + RFM95_RSSI_OFFSET);
}

LOCAL inline int16_t RFM95_internalToRSSI(const rfm95_RSSI_t internalRSSI)
{
	return static_cast<int16_t>(internalRSSI - RFM95_RSSI_OFFSET);
}

LOCAL bool RFM95_initialise(const uint32_t frequencyHz)
{
	RFM95_DEBUG(PSTR("RFM95:INIT\n"));
	// power pin, if defined
#if defined(MY_RFM95_POWER_PIN)
	hwPinMode(MY_RFM95_POWER_PIN, OUTPUT);
#endif
	RFM95_powerUp();
	// reset radio module if rst pin defined
#if defined(MY_RFM95_RST_PIN)
	hwPinMode(MY_RFM95_RST_PIN, OUTPUT);
	hwDigitalWrite(MY_RFM95_RST_PIN, LOW);
	// 100uS
	delayMicroseconds(RFM95_POWERUP_DELAY_MS);
	hwDigitalWrite(MY_RFM95_RST_PIN, HIGH);
	// wait until chip ready
	delay(5);
	RFM95_DEBUG(PSTR("RFM95:INIT:PIN,CS=%" PRIu8 ",IQP=%" PRIu8 ",IQN=%" PRIu8 ",RST=%" PRIu8 "\n"),
	            MY_RFM95_CS_PIN, MY_RFM95_IRQ_PIN,
	            MY_RFM95_IRQ_NUM,MY_RFM95_RST_PIN);
#else
	RFM95_DEBUG(PSTR("RFM95:INIT:PIN,CS=%" PRIu8 ",IQP=%" PRIu8 ",IQN=%" PRIu8 "\n"), MY_RFM95_CS_PIN,
	            MY_RFM95_IRQ_PIN,
	            MY_RFM95_IRQ_NUM);
#endif

	// set variables
	RFM95.address = RFM95_BROADCAST_ADDRESS;
	RFM95.ackReceived = false;
	RFM95.dataReceived = false;
	RFM95.txSequenceNumber = 0;	// initialise TX sequence counter
	RFM95.powerLevel = 0;
	RFM95.ATCenabled = false;
	RFM95.ATCtargetRSSI = RFM95_RSSItoInternal(RFM95_TARGET_RSSI);

	// SPI init
#if !defined(__linux__)
	hwDigitalWrite(MY_RFM95_CS_PIN, HIGH);
	hwPinMode(MY_RFM95_CS_PIN, OUTPUT);
#endif
	RFM95_SPI.begin();

	// Set LoRa mode (during sleep mode)
	(void)RFM95_writeReg(RFM95_REG_01_OP_MODE, RFM95_MODE_SLEEP | RFM95_LONG_RANGE_MODE);
	delay(10); // Wait for sleep mode to take over

	// TCXO init, if present
#if defined(MY_RFM95_TCXO)
	RFM95_enableTCXO();
#else
	(void)RFM95_enableTCXO;
#endif

	// Set up FIFO, 256 bytes: LoRa max message 64 bytes, set half RX half TX (default)
	(void)RFM95_writeReg(RFM95_REG_0F_FIFO_RX_BASE_ADDR, RFM95_RX_FIFO_ADDR);
	(void)RFM95_writeReg(RFM95_REG_0E_FIFO_TX_BASE_ADDR, RFM95_TX_FIFO_ADDR);
	(void)RFM95_writeReg(RFM95_REG_23_MAX_PAYLOAD_LENGTH, RFM95_MAX_PACKET_LEN);

	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);
	const rfm95_modemConfig_t configuration = { MY_RFM95_MODEM_CONFIGRUATION };
	RFM95_setModemRegisters(&configuration);
	RFM95_setPreambleLength(RFM95_PREAMBLE_LENGTH);
	RFM95_setFrequency(frequencyHz);
	(void)RFM95_setTxPowerLevel(MY_RFM95_TX_POWER_DBM);

	if (!RFM95_sanityCheck()) {
		// sanity check failed, check wiring or replace module
		RFM95_DEBUG(PSTR("!RFM95:INIT:SANCHK FAIL\n"));
		return false;
	}

	// IRQ
	RFM95_irq = false;
	hwPinMode(MY_RFM95_IRQ_PIN, INPUT);
	attachInterrupt(MY_RFM95_IRQ_NUM, RFM95_interruptHandler, RISING);
	return true;
}

LOCAL void RFM95_interruptHandler(void)
{
	// set flag
	RFM95_irq = true;
}

// RxDone, TxDone, CADDone is mapped to DI0
LOCAL void RFM95_interruptHandling(void)
{
	// read interrupt register
	const uint8_t irqFlags = RFM95_readReg(RFM95_REG_12_IRQ_FLAGS);
	if (RFM95.radioMode == RFM95_RADIO_MODE_RX && (irqFlags & RFM95_RX_DONE)) {
		// RXSingle mode: Radio goes automatically to STDBY after packet received
		(void)RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);
		// Check CRC flag
		if (!(irqFlags & RFM95_PAYLOAD_CRC_ERROR)) {
			const uint8_t bufLen = min(RFM95_readReg(RFM95_REG_13_RX_NB_BYTES), (uint8_t)RFM95_MAX_PACKET_LEN);
			if (bufLen >= RFM95_HEADER_LEN) {
				// Reset the fifo read ptr to the beginning of the packet
				(void)RFM95_writeReg(RFM95_REG_0D_FIFO_ADDR_PTR, RFM95_readReg(RFM95_REG_10_FIFO_RX_CURRENT_ADDR));
				(void)RFM95_burstReadReg(RFM95_REG_00_FIFO, RFM95.currentPacket.data, bufLen);
				RFM95.currentPacket.RSSI = static_cast<rfm95_RSSI_t>(RFM95_readReg(
				                               RFM95_REG_1A_PKT_RSSI_VALUE)); // RSSI of latest packet received
				RFM95.currentPacket.SNR = static_cast<rfm95_SNR_t>(RFM95_readReg(RFM95_REG_19_PKT_SNR_VALUE));
				RFM95.currentPacket.payloadLen = bufLen - RFM95_HEADER_LEN;
				if ((RFM95.currentPacket.header.version >= RFM95_MIN_PACKET_HEADER_VERSION) &&
				        (RFM95_PROMISCUOUS || RFM95.currentPacket.header.recipient == RFM95.address ||
				         RFM95.currentPacket.header.recipient == RFM95_BROADCAST_ADDRESS)) {
					// Message for us
					RFM95.ackReceived = RFM95_getACKReceived(RFM95.currentPacket.header.controlFlags) &&
					                    !RFM95_getACKRequested(RFM95.currentPacket.header.controlFlags);
					RFM95.dataReceived = !RFM95.ackReceived;
				}
			}
		} else {
			// CRC error
			RFM95_DEBUG(PSTR("!RFM95:IRH:CRC ERROR\n"));
			// FIFO is cleared when switch from STDBY to RX or TX
			(void)RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
		}
	} else if (RFM95.radioMode == RFM95_RADIO_MODE_TX && (irqFlags & RFM95_TX_DONE) ) {
		(void)RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
	} else if (RFM95.radioMode == RFM95_RADIO_MODE_CAD && (irqFlags & RFM95_CAD_DONE) ) {
		RFM95.channelActive = irqFlags & RFM95_CAD_DETECTED;
		(void)RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);
	}
	// Clear IRQ flags
	RFM95_writeReg(RFM95_REG_12_IRQ_FLAGS, RFM95_CLEAR_IRQ);
}

LOCAL void RFM95_handler(void)
{
	if (RFM95_irq) {
		RFM95_irq = false;
		RFM95_interruptHandling();
	}
}

LOCAL bool RFM95_available(void)
{
	if (RFM95.dataReceived) {
		// data received - we are still in STDBY from IRQ handler
		return true;
	} else if (RFM95.radioMode == RFM95_RADIO_MODE_TX) {
		return false;
	} else if (RFM95.radioMode != RFM95_RADIO_MODE_RX) {
		// we are not in RX, not CAD, and no data received
		(void)RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
	}
	return false;
}

LOCAL uint8_t RFM95_receive(uint8_t *buf, const uint8_t maxBufSize)
{
	const uint8_t payloadLen = min(RFM95.currentPacket.payloadLen, maxBufSize);
	const uint8_t sender = RFM95.currentPacket.header.sender;
	const rfm95_sequenceNumber_t sequenceNumber = RFM95.currentPacket.header.sequenceNumber;
	const rfm95_controlFlags_t controlFlags = RFM95.currentPacket.header.controlFlags;
	const rfm95_RSSI_t RSSI = RFM95.currentPacket.RSSI;
	const rfm95_SNR_t SNR = RFM95.currentPacket.SNR;
	if (buf != NULL) {
		(void)memcpy((void *)buf, (void *)&RFM95.currentPacket.payload, payloadLen);
	}
	// clear data flag
	RFM95.dataReceived = false;
	// ACK handling
	if (RFM95_getACKRequested(controlFlags) && !RFM95_getACKReceived(controlFlags)) {
#if defined(MY_GATEWAY_FEATURE) && (F_CPU>16*1000000ul)
		// delay for fast GW and slow nodes
		delay(50);
#endif
		RFM95_sendACK(sender, sequenceNumber, RSSI, SNR);
	}
	return payloadLen;
}

LOCAL bool RFM95_sendFrame(rfm95_packet_t *packet, const bool increaseSequenceCounter)
{
	// Check channel activity
	if (!RFM95_waitCAD()) {
		return false;
	}
	// radio is in STDBY
	if (increaseSequenceCounter) {
		// increase sequence counter, overflow is ok
		RFM95.txSequenceNumber++;
	}
	packet->header.sequenceNumber = RFM95.txSequenceNumber;
	// Position at the beginning of the TX FIFO
	(void)RFM95_writeReg(RFM95_REG_0D_FIFO_ADDR_PTR, RFM95_TX_FIFO_ADDR);
	// write packet
	const uint8_t finalLen = packet->payloadLen + RFM95_HEADER_LEN;
	(void)RFM95_burstWriteReg(RFM95_REG_00_FIFO, packet->data, finalLen);
	// total payload length
	(void)RFM95_writeReg(RFM95_REG_22_PAYLOAD_LENGTH, finalLen);
	// send message, if sent, irq fires and radio returns to standby
	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_TX);
	// wait until IRQ fires or timeout
	const uint32_t startTX_MS = hwMillis();
	// todo: make this payload length + bit rate dependend
	while (!RFM95_irq && (hwMillis() - startTX_MS < MY_RFM95_TX_TIMEOUT_MS) ) {
		doYield();
	}
	return RFM95_irq;
}

LOCAL bool RFM95_send(const uint8_t recipient, uint8_t *data, const uint8_t len,
                      const rfm95_controlFlags_t flags, const bool increaseSequenceCounter)
{
	rfm95_packet_t packet;
	packet.header.version = RFM95_PACKET_HEADER_VERSION;
	packet.header.sender = RFM95.address;
	packet.header.recipient = recipient;
	packet.payloadLen = min(len, (uint8_t)RFM95_MAX_PAYLOAD_LEN);
	packet.header.controlFlags = flags;
	(void)memcpy((void *)&packet.payload, (void *)data, packet.payloadLen);
	return RFM95_sendFrame(&packet, increaseSequenceCounter);
}

LOCAL void RFM95_setFrequency(const uint32_t frequencyHz)
{
	const uint32_t freqReg = (uint32_t)(frequencyHz / RFM95_FSTEP);
	(void)RFM95_writeReg(RFM95_REG_06_FRF_MSB, (uint8_t)((freqReg >> 16) & 0xff));
	(void)RFM95_writeReg(RFM95_REG_07_FRF_MID, (uint8_t)((freqReg >> 8) & 0xff));
	(void)RFM95_writeReg(RFM95_REG_08_FRF_LSB, (uint8_t)(freqReg & 0xff));
}

LOCAL bool RFM95_setTxPowerLevel(rfm95_powerLevel_t newPowerLevel)
{
	// RFM95/96/97/98 does not have RFO pins connected to anything. Only PA_BOOST
	newPowerLevel = max((int8_t)RFM95_MIN_POWER_LEVEL_DBM, newPowerLevel);
	newPowerLevel = min((int8_t)RFM95_MAX_POWER_LEVEL_DBM, newPowerLevel);
	if (newPowerLevel != RFM95.powerLevel) {
		RFM95.powerLevel = newPowerLevel;
		uint8_t val;
		if (newPowerLevel > 20) {
			// enable DAC, adds 3dBm
			// The documentation is pretty confusing on this topic: PaSelect says the max power is 20dBm,
			// but OutputPower claims it would be 17dBm. Measurements show 20dBm is correct
			(void)RFM95_writeReg(RFM95_REG_4D_PA_DAC, RFM95_PA_DAC_ENABLE);
			val = newPowerLevel - 8;
		} else {
			(void)RFM95_writeReg(RFM95_REG_4D_PA_DAC, RFM95_PA_DAC_DISABLE);
			val = newPowerLevel - 5;
		}
		(void)RFM95_writeReg(RFM95_REG_09_PA_CONFIG, RFM95_PA_SELECT | val);
		RFM95_DEBUG(PSTR("RFM95:PTX:LEVEL=%" PRIi8 "\n"), newPowerLevel);
		return true;
	}
	return false;

}

LOCAL void RFM95_enableTCXO(void)
{
	while ((RFM95_readReg(RFM95_REG_4B_TCXO) & RFM95_TCXO_TCXO_INPUT_ON) != RFM95_TCXO_TCXO_INPUT_ON) {
		(void)RFM95_writeReg(RFM95_REG_4B_TCXO,
		                     (RFM95_readReg(RFM95_REG_4B_TCXO) | RFM95_TCXO_TCXO_INPUT_ON));
	}
}

// Sets registers from a canned modem configuration structure
LOCAL void RFM95_setModemRegisters(const rfm95_modemConfig_t *config)
{
	(void)RFM95_writeReg(RFM95_REG_1D_MODEM_CONFIG1, config->reg_1d);
	(void)RFM95_writeReg(RFM95_REG_1E_MODEM_CONFIG2, config->reg_1e);
	(void)RFM95_writeReg(RFM95_REG_26_MODEM_CONFIG3, config->reg_26);
}

LOCAL void RFM95_setPreambleLength(const uint16_t preambleLength)
{
	(void)RFM95_writeReg(RFM95_REG_20_PREAMBLE_MSB, (uint8_t)((preambleLength >> 8) & 0xff));
	(void)RFM95_writeReg(RFM95_REG_21_PREAMBLE_LSB, (uint8_t)(preambleLength & 0xff));
}

LOCAL void RFM95_setAddress(const uint8_t addr)
{
	RFM95.address = addr;
}

LOCAL uint8_t RFM95_getAddress(void)
{
	return RFM95.address;
}

LOCAL bool RFM95_setRadioMode(const rfm95_radioMode_t newRadioMode)
{
	if (RFM95.radioMode == newRadioMode) {
		return false;
	}
	uint8_t regMode;

	if (newRadioMode == RFM95_RADIO_MODE_STDBY) {
		regMode = RFM95_MODE_STDBY;
	} else if (newRadioMode == RFM95_RADIO_MODE_SLEEP) {
		regMode = RFM95_MODE_SLEEP;
	} else if (newRadioMode == RFM95_RADIO_MODE_CAD) {
		regMode = RFM95_MODE_CAD;
		(void)RFM95_writeReg(RFM95_REG_40_DIO_MAPPING1, 0x80); // Interrupt on CadDone, DIO0
	} else if (newRadioMode == RFM95_RADIO_MODE_RX) {
		RFM95.dataReceived = false;
		RFM95.ackReceived = false;
		regMode = RFM95_MODE_RXCONTINUOUS;
		(void)RFM95_writeReg(RFM95_REG_40_DIO_MAPPING1, 0x00); // Interrupt on RxDone, DIO0
		(void)RFM95_writeReg(RFM95_REG_0D_FIFO_ADDR_PTR,
		                     RFM95_RX_FIFO_ADDR); // set FIFO ptr to beginning of RX FIFO address
	} else if (newRadioMode == RFM95_RADIO_MODE_TX) {
		regMode = RFM95_MODE_TX;
		(void)RFM95_writeReg(RFM95_REG_40_DIO_MAPPING1, 0x40); // Interrupt on TxDone, DIO0
	} else {
		return false;
	}
	(void)RFM95_writeReg(RFM95_REG_01_OP_MODE, regMode);

	RFM95.radioMode = newRadioMode;
	return true;
}

LOCAL void RFM95_powerUp(void)
{
#if defined(MY_RFM95_POWER_PIN)
	RFM95_DEBUG(PSTR("RFM95:PWU\n"));	// power up radio
	hwDigitalWrite(MY_RFM95_POWER_PIN, HIGH);
	delay(RFM95_POWERUP_DELAY_MS);
#endif
}
LOCAL void RFM95_powerDown(void)
{
#if defined(MY_RFM95_POWER_PIN)
	RFM95_DEBUG(PSTR("RFM95:PWD\n"));	// power down radio
	hwDigitalWrite(MY_RFM95_POWER_PIN, LOW);
#endif
}

LOCAL bool RFM95_sleep(void)
{
	RFM95_DEBUG(PSTR("RFM95:RSL\n"));	// put radio to sleep
	return RFM95_setRadioMode(RFM95_RADIO_MODE_SLEEP);
}

LOCAL bool RFM95_standBy(void)
{
	RFM95_DEBUG(PSTR("RFM95:RSB\n"));	// put radio to standby
	return RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);
}


// should be called immediately after reception in case sender wants ACK
LOCAL void RFM95_sendACK(const uint8_t recipient, const rfm95_sequenceNumber_t sequenceNumber,
                         const rfm95_RSSI_t RSSI, const rfm95_SNR_t SNR)
{
	RFM95_DEBUG(PSTR("RFM95:SAC:SEND ACK,TO=%" PRIu8 ",SEQ=%" PRIu16 ",RSSI=%" PRIi16 ",SNR=%" PRIi8
	                 "\n"),recipient,sequenceNumber,
	            RFM95_internalToRSSI(RSSI),RFM95_internalToSNR(SNR));
	rfm95_ack_t ACK;
	ACK.sequenceNumber = sequenceNumber;
	ACK.RSSI = RSSI;
	ACK.SNR = SNR;
	rfm95_controlFlags_t flags = 0u;
	RFM95_setACKReceived(flags, true);
	RFM95_setACKRSSIReport(flags, true);
	(void)RFM95_send(recipient, (uint8_t *)&ACK, sizeof(rfm95_ack_t), flags);
}

LOCAL bool RFM95_executeATC(const rfm95_RSSI_t currentRSSI, const rfm95_RSSI_t targetRSSI)
{
	rfm95_powerLevel_t newPowerLevel = RFM95.powerLevel;
	const int16_t ownRSSI = RFM95_internalToRSSI(currentRSSI);
	const int16_t uRange = RFM95_internalToRSSI(targetRSSI) + RFM95_ATC_TARGET_RANGE_DBM;
	const int16_t lRange = RFM95_internalToRSSI(targetRSSI) - RFM95_ATC_TARGET_RANGE_DBM;
	if (ownRSSI < lRange && RFM95.powerLevel < RFM95_MAX_POWER_LEVEL_DBM) {
		// increase transmitter power
		newPowerLevel++;
	} else if (ownRSSI > uRange && RFM95.powerLevel > RFM95_MIN_POWER_LEVEL_DBM) {
		// decrease transmitter power
		newPowerLevel--;
	} else {
		// nothing to adjust
		return false;
	}
	RFM95_DEBUG(PSTR("RFM95:ATC:ADJ TXL,cR=%" PRIi16 ",tR=%" PRIi16 "..%" PRIi16 ",TXL=%" PRIi8 "\n"),
	            ownRSSI, lRange, uRange, RFM95.powerLevel);
	return RFM95_setTxPowerLevel(newPowerLevel);
}

LOCAL bool RFM95_sendWithRetry(const uint8_t recipient, const void *buffer,
                               const uint8_t bufferSize, const uint8_t retries, const uint32_t retryWaitTime)
{
	for (uint8_t retry = 0; retry <= retries; retry++) {
		RFM95_DEBUG(PSTR("RFM95:SWR:SEND,TO=%" PRIu8 ",SEQ=%" PRIu16 ",RETRY=%" PRIu8 "\n"), recipient,
		            RFM95.txSequenceNumber,
		            retry);
		rfm95_controlFlags_t flags = 0u;
		RFM95_setACKRequested(flags, (recipient != RFM95_BROADCAST_ADDRESS));
		// send packet
		if (!RFM95_send(recipient, (uint8_t *)buffer, bufferSize, flags, !retry)) {
			return false;
		}
		(void)RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
		if (recipient == RFM95_BROADCAST_ADDRESS) {
			return true;
		}
		const uint32_t enterMS = hwMillis();
		while (hwMillis() - enterMS < retryWaitTime && !RFM95.dataReceived) {
			RFM95_handler();
			if (RFM95.ackReceived) {
				const uint8_t sender = RFM95.currentPacket.header.sender;
				const rfm95_sequenceNumber_t ACKsequenceNumber = RFM95.currentPacket.ACK.sequenceNumber;
				const rfm95_controlFlags_t flag = RFM95.currentPacket.header.controlFlags;
				const rfm95_RSSI_t RSSI = RFM95.currentPacket.ACK.RSSI;
				//const rfm95_SNR_t SNR = RFM95.currentPacket.ACK.SNR;
				RFM95.ackReceived = false;
				// packet read, back to RX
				RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
				if (sender == recipient &&
				        (ACKsequenceNumber == RFM95.txSequenceNumber)) {
					RFM95_DEBUG(PSTR("RFM95:SWR:ACK FROM=%" PRIu8 ",SEQ=%" PRIu16 ",RSSI=%" PRIi16 "\n"),sender,
					            ACKsequenceNumber,
					            RFM95_internalToRSSI(RSSI));
					//RFM95_clearRxBuffer();
					// ATC
					if (RFM95.ATCenabled && RFM95_getACKRSSIReport(flag)) {
						(void)RFM95_executeATC(RSSI, RFM95.ATCtargetRSSI);
					}
					return true;
				} // seq check
			}
			doYield();
		}
		RFM95_DEBUG(PSTR("!RFM95:SWR:NACK\n"));
		const uint32_t enterCSMAMS = hwMillis();
		const uint16_t randDelayCSMA = enterMS % 100;
		while (hwMillis() - enterCSMAMS < randDelayCSMA) {
			doYield();
		}
	}
	if (RFM95.ATCenabled) {
		// No ACK received, maybe out of reach: increase power level
		(void)RFM95_setTxPowerLevel(RFM95.powerLevel + 1);
	}
	return false;
}

// Wait until no channel activity detected or timeout
LOCAL bool RFM95_waitCAD(void)
{
	// receiver needs to be in STDBY before entering CAD mode
	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);
	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_CAD);
	const uint32_t enterMS = hwMillis();
	while (RFM95.radioMode == RFM95_RADIO_MODE_CAD && (hwMillis() - enterMS < RFM95_CAD_TIMEOUT_MS) ) {
		doYield();
		RFM95_handler();
	}
	return !RFM95.channelActive;
}

LOCAL void RFM95_ATCmode(const bool OnOff, const int16_t targetRSSI)
{
	RFM95.ATCenabled = OnOff;
	RFM95.ATCtargetRSSI = RFM95_RSSItoInternal(targetRSSI);
}

LOCAL bool RFM95_sanityCheck(void)
{
	bool result = true;
	result &= RFM95_readReg(RFM95_REG_0F_FIFO_RX_BASE_ADDR) == RFM95_RX_FIFO_ADDR;
	result &= RFM95_readReg(RFM95_REG_0E_FIFO_TX_BASE_ADDR) == RFM95_TX_FIFO_ADDR;
	result &= RFM95_readReg(RFM95_REG_23_MAX_PAYLOAD_LENGTH) == RFM95_MAX_PACKET_LEN;
	return result;
}


LOCAL int16_t RFM95_getSendingRSSI(void)
{
	// own RSSI, as measured by the recipient - ACK part
	if (RFM95_getACKRSSIReport(RFM95.currentPacket.header.controlFlags)) {
		return RFM95_internalToRSSI(RFM95.currentPacket.ACK.RSSI);
	} else {
		// not possible
		return INVALID_RSSI;
	}
}

LOCAL int16_t RFM95_getSendingSNR(void)
{
	// own SNR, as measured by the recipient - ACK part
	if (RFM95_getACKRSSIReport(RFM95.currentPacket.header.controlFlags)) {
		return static_cast<int16_t>(RFM95_internalToSNR(RFM95.currentPacket.ACK.SNR));
	} else {
		// not possible
		return INVALID_SNR;
	}
}

LOCAL int16_t RFM95_getReceivingRSSI(void)
{
	// RSSI from last received packet
	return static_cast<int16_t>(RFM95_internalToRSSI(RFM95.currentPacket.RSSI));
}

LOCAL int16_t RFM95_getReceivingSNR(void)
{
	// SNR from last received packet
	return static_cast<int16_t>(RFM95_internalToSNR(RFM95.currentPacket.SNR));
}

LOCAL uint8_t RFM95_getTxPowerLevel(void)
{
	return RFM95.powerLevel;
}

LOCAL uint8_t RFM95_getTxPowerPercent(void)
{
	// report TX level in %
	const uint8_t result = static_cast<uint8_t>(100.0f * (RFM95.powerLevel -
	                       RFM95_MIN_POWER_LEVEL_DBM) /
	                       (RFM95_MAX_POWER_LEVEL_DBM
	                        - RFM95_MIN_POWER_LEVEL_DBM));
	return result;
}
LOCAL bool RFM95_setTxPowerPercent(const uint8_t newPowerPercent)
{
	const rfm95_powerLevel_t newPowerLevel = static_cast<rfm95_powerLevel_t>
	        (RFM95_MIN_POWER_LEVEL_DBM + (RFM95_MAX_POWER_LEVEL_DBM
	                                      - RFM95_MIN_POWER_LEVEL_DBM) * (newPowerPercent / 100.0f));
	RFM95_DEBUG(PSTR("RFM95:SPP:PCT=%" PRIu8 ",TX LEVEL=%" PRIi8 "\n"), newPowerPercent,newPowerLevel);
	return RFM95_setTxPowerLevel(newPowerLevel);
}

