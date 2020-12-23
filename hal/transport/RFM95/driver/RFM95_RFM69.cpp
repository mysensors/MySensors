/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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
 * RFM95 driver refactored and optimized for MySensors, Copyright (C) 2017-2019 Olivier Mauti <olivier@mysensors.org>
 *
 * Definitions for HopeRF LoRa radios:
 * https://www.hoperf.com/data/upload/portal/20190611/RFM95W-V1.1.pdf
 *
 */

#include "RFM95_RFM69.h"

// debug
#if defined(MY_DEBUG_VERBOSE_RFM95)
#define RFM95_DEBUG(x,...)	DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< Debug print
#else
#define RFM95_DEBUG(x,...)	//!< DEBUG null
#endif

rfm95_internal_t RFM95;	//!< internal variables
volatile bool RFM95_irq; //<! rfm95 irq flag

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

LOCAL void RFM95_prepareSPITransaction(void)
{
#if !defined(MY_SOFTSPI) && defined(SPI_HAS_TRANSACTION)
	RFM95_SPI.beginTransaction(SPISettings(MY_RFM95_SPI_SPEED, RFM95_SPI_DATA_ORDER,
	                                       RFM95_SPI_DATA_MODE));
#endif
	RFM95_csn(LOW);
}

LOCAL void RFM95_concludeSPITransaction(void)
{
	RFM95_csn(HIGH);
#if !defined(MY_SOFTSPI) && defined(SPI_HAS_TRANSACTION)
	RFM95_SPI.endTransaction();
#endif
}

LOCAL uint8_t RFM95_spiMultiByteTransfer(const uint8_t cmd, uint8_t *buf, uint8_t len,
        const bool aReadMode)
{
	uint8_t status;
	uint8_t *current = buf;
	RFM95_prepareSPITransaction();
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
			(void)RFM95_SPI.transfer(*current++);
		}
	}
#endif
	RFM95_concludeSPITransaction();
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
LOCAL uint8_t RFM95_readReg(const uint8_t reg)
{
	return RFM95_RAW_readByteRegister(reg & RFM95_READ_REGISTER);
}

LOCAL uint8_t RFM95_writeReg(const uint8_t reg, const uint8_t value)
{
	return RFM95_RAW_writeByteRegister(reg | RFM95_WRITE_REGISTER, value);
}

/*
LOCAL uint8_t RFM95_burstReadReg(const uint8_t reg, void *buf, uint8_t len)
{
	return RFM95_spiMultiByteTransfer(reg & RFM95_READ_REGISTER, (uint8_t *)buf, len, true);
}
*/

LOCAL uint8_t RFM95_burstWriteReg(const uint8_t reg, const void *buf, uint8_t len)
{
	return RFM95_spiMultiByteTransfer(reg | RFM95_WRITE_REGISTER, (uint8_t *)buf, len, false);
}

LOCAL rfm95_RSSI_t RFM95_RSSItoInternal(const int16_t externalRSSI)
{
	// conversion for RFM95 FSK/OOK mode
	return (rfm95_RSSI_t)-(externalRSSI * 2);
}

LOCAL int16_t RFM95_internalToRSSI(const rfm95_RSSI_t internalRSSI)
{
	// conversion for RFM95 FSK/OOK mode
	return (int16_t)-(internalRSSI / 2);
}

LOCAL bool RFM95_initialise(const uint32_t frequencyHz)
{
	// power pin, if defined
#if defined(MY_RFM95_POWER_PIN)
	hwPinMode(MY_RFM95_POWER_PIN, OUTPUT);
#endif
	RFM95_powerUp();
	// reset radio module if rst pin defined
#if defined(MY_RFM95_RST_PIN)
	hwPinMode(MY_RFM95_RST_PIN, OUTPUT);
	hwDigitalWrite(MY_RFM95_RST_PIN, LOW);
	delayMicroseconds(RFM95_RESET_DELAY_US);
	hwDigitalWrite(MY_RFM95_RST_PIN, HIGH);
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
	RFM95.lastPacket.sender = RFM95_BROADCAST_ADDRESS;
	RFM95.lastPacket.sequenceNumber = 0xFF;
	// SPI init
#if !defined(__linux__)
	hwDigitalWrite(MY_RFM95_CS_PIN, HIGH);
	hwPinMode(MY_RFM95_CS_PIN, OUTPUT);
#endif
	RFM95_SPI.begin();

	RFM95_DEBUG(PSTR("RFM95:INIT:RFM69\n"));
	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);

	(void)RFM95_writeReg(RFM95_REG_02_BITRATE_MSB, RFM95_BITRATEMSB_55555);
	(void)RFM95_writeReg(RFM95_REG_03_BITRATE_LSB, RFM95_BITRATELSB_55555);
	// (Fdev + BitRate / 2 <= 500KHz)
	(void)RFM95_writeReg(RFM95_REG_04_FDEV_MSB, RFM95_FDEVMSB_50000);
	(void)RFM95_writeReg(RFM95_REG_05_FDEV_LSB, RFM95_FDEVLSB_50000);
	(void)RFM95_writeReg(RFM95_REG_0C_LNA, RFM95_LNA_GAIN_G1 | RFM95_LNA_BOOST_ON);
	(void)RFM95_writeReg(RFM95_REG_0D_RX_CONFIG,
	                     RFM95_RX_CONFIG_RESTARTRX_ON_COLLISION_OFF | RFM95_RX_CONFIG_AFCAUTO_OFF |
	                     RFM95_RX_CONFIG_AGCAUTO_ON | RFM95_RX_CONFIG_RXTRIGGER_OFF);
	(void)RFM95_writeReg(RFM95_REG_0E_RSSI_CONFIG,
	                     RFM95_RSSI_CONFIG_OFFSET_P_00_DB | RFM95_RSSI_CONFIG_SMOOTHING_8);
	//(void)RFM95_writeReg(RFM95_REG_0F_RSSI_COLLISION, 0x0A); // collision detection, +10dB during RX => this triggers RX restart
	(void)RFM95_writeReg(RFM95_REG_10_RSSI_THRESHOLD, RFM95_RSSItoInternal(-90));
	// single side RxBw = FxOsc/(RxBwMant*2^(RxBwExp+2)), i.e. for 250kHz total Bw => RxBw should be 125kHz, i.e. FxOSC=32Mhz, RxBwMant=16, RxBwExp=2
	(void)RFM95_writeReg(RFM95_REG_12_RXBW, RFM95_RXBW_MANT_16 | RFM95_RXBW_EXP_2);
	(void)RFM95_writeReg(RFM95_REG_13_AFCBW, RFM95_RXBW_MANT_16 | RFM95_RXBW_EXP_2);
	(void)RFM95_writeReg(RFM95_REG_1A_AFCFEI, RFM95_AFCFEI_AFCAUTOCLEAR_ON);
	(void)RFM95_writeReg(RFM95_REG_24_OSC, RFM95_OSC_CLKOUT_OFF);
	(void)RFM95_writeReg(RFM95_REG_25_PREAMBLE_MSB, RFM95_PREAMBLESIZE_MSB_VALUE);
	(void)RFM95_writeReg(RFM95_REG_26_PREAMBLE_LSB, RFM95_PREAMBLESIZE_LSB_VALUE);
	(void)RFM95_writeReg(RFM95_REG_27_SYNC_CONFIG,
	                     RFM95_SYNC_AUTORXRESTART_NO_PLL | RFM95_SYNC_PREAMBLE_POLARITY_AA | RFM95_SYNC_ON |
	                     RFM95_SYNC_FIFO_FILL_AUTO | RFM95_SYNC_SIZE_2);
	(void)RFM95_writeReg(RFM95_REG_28_SYNC_VALUE1, RFM95_SYNCVALUE1);
	(void)RFM95_writeReg(RFM95_REG_29_SYNC_VALUE2, RFM95_SYNCVALUE2);
	(void)RFM95_writeReg(RFM95_REG_30_PACKET_CONFIG1, RFM95_CONFIG_WHITE);
	(void)RFM95_writeReg(RFM95_REG_31_PACKET_CONFIG2, RFM95_CONFIG_PACKET);
	(void)RFM95_writeReg(RFM95_REG_32_PAYLOAD_LENGTH, RFM95_MAX_PACKET_LEN);
	(void)RFM95_writeReg(RFM95_REG_33_NODE_ADDR, RFM95_BROADCAST_ADDRESS);
	(void)RFM95_writeReg(RFM95_REG_34_BROADCAST_ADDR, RFM95_BROADCAST_ADDRESS);
	(void)RFM95_writeReg(RFM95_REG_35_FIFO_THRESHOLD,
	                     RFM95_TXSTART_CONDITION_FIFO_THRESHOLD | (RFM95_HEADER_LEN - 1));
	(void)RFM95_writeReg(RFM95_REG_36_SEQ_CONFIG1,
	                     RFM95_SEQ_CONFIG1_SEQUENCER_STOP); // disable sequencer
	(void)RFM95_writeReg(RFM95_REG_3B_IMAGECAL, RFM95_IMAGECAL_TEMPTHRESHOLD_10);

	// IRQ on packet sent (TX mode) and payload ready (RX mode)
	(void)RFM95_writeReg(RFM95_REG_40_DIO_MAPPING1,
	                     RFM95_DIOMAPPING1_DIO0_00 | RFM95_DIOMAPPING1_DIO1_11 | RFM95_DIOMAPPING1_DIO2_00);
	RFM95_setFrequency(frequencyHz);

	// calibrate image rejection mixer at used frequency (automatical calibration after POR at 434MHz)
	(void)RFM95_writeReg(RFM95_REG_3B_IMAGECAL, RFM95_IMAGECAL_IMAGECAL_START);
	while (RFM95_readReg(RFM95_REG_3B_IMAGECAL) & RFM95_IMAGECAL_IMAGECAL_RUNNING) {};

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

#if defined(ARDUINO_ARCH_ESP32)
#define IRQ_HANDLER_ATTR IRAM_ATTR
#elif defined(ARDUINO_ARCH_ESP8266)
#define IRQ_HANDLER_ATTR ICACHE_RAM_ATTR
#else
#define IRQ_HANDLER_ATTR
#endif
LOCAL void IRQ_HANDLER_ATTR RFM95_interruptHandler(void)
{
	// set flag
	RFM95_irq = true;
}

LOCAL void RFM95_handling(void)
{
	if (RFM95_irq) {
		RFM95.currentPacket.RSSI = static_cast<rfm95_RSSI_t>(RFM95_readReg(RFM95_REG_11_RSSI_VALUE));
		// clear flag, 8bit - no need for critical section
		//RFM69_irq = false; ==> flag cleared when transition to RX or TX
		RFM95_DEBUG(PSTR("RFM95:IRQ\n"));
		// radio: RX
		const uint8_t regIrqFlags2 = RFM95_readReg(RFM95_REG_3F_IRQ_FLAGS2);
		if (regIrqFlags2 & RFM95_IRQ2_PAYLOAD_READY) {
			(void)RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);
			RFM95_RXFIFOHandling();
		} else {
			RFM95_DEBUG(PSTR("!RFM95:IRQ NH, IRQ2=% " PRIu8 "\n"), regIrqFlags2); // not handled IRQ
		}
	}
}

// RxDone, TxDone
LOCAL void RFM95_RXFIFOHandling(void)
{
	// reset flags
	RFM95.ackReceived = false;
	RFM95.dataReceived = false;
	RFM95_prepareSPITransaction();
	(void)RFM95_SPI.transfer(RFM95_REG_00_FIFO & RFM95_READ_REGISTER);
	// set reading pointer
	uint8_t *current = (uint8_t *)&RFM95.currentPacket;
	bool headerRead = false;
	// first read header
	uint8_t readingLength = RFM95_HEADER_LEN;
	while (readingLength--) {
		*current++ = RFM95_SPI.transfer((uint8_t)RFM95_NOP);
		if (!readingLength && !headerRead) {
			// header read
			headerRead = true;
			if (RFM95.currentPacket.header.version >= RFM95_MIN_PACKET_HEADER_VERSION) {
				// read payload
				readingLength = min(RFM95.currentPacket.header.packetLen - (RFM95_HEADER_LEN - 1),
				                    RFM95_MAX_PACKET_LEN);
				// save payload length
				RFM95.currentPacket.payloadLen = readingLength;
				RFM95.ackReceived = RFM95_getACKReceived(RFM95.currentPacket.header.controlFlags);
				RFM95.dataReceived = !RFM95.ackReceived;
			}
		}
	}
	RFM95_concludeSPITransaction();

	// ACK handling
	if (RFM95_getACKRequested(RFM95.currentPacket.header.controlFlags)) {
#if (F_CPU > 16*1000000ul)
		// delay for fast nodes
		delay(5);
#endif
		RFM95_sendACK(RFM95.currentPacket.header.sender, RFM95.currentPacket.header.sequenceNumber,
		              RFM95.currentPacket.RSSI);
		// radio in RX
	}

	// primitive deduplication
	if (RFM95.dataReceived) {
		if (RFM95.currentPacket.header.sender == RFM95.lastPacket.sender &&
		        RFM95.currentPacket.header.sequenceNumber == RFM95.lastPacket.sequenceNumber) {
			// same packet received
			RFM95.dataReceived = false;
			RFM95_DEBUG(PSTR("!RFM95:PKT DD\n")); // packet de-duplication
		} else {
			// new packet received
			RFM95_DEBUG(PSTR("RFM95:NEW PKT\n"));
			RFM95.lastPacket.sender = RFM95.currentPacket.header.sender;
			RFM95.lastPacket.sequenceNumber = RFM95.currentPacket.header.sequenceNumber;
		}
	}

}

LOCAL bool RFM95_available(void)
{
	if (RFM95.dataReceived) {
		// data received - we are still in STDBY from IRQ handler
		return true;
	} else if (RFM95.radioMode != RFM95_RADIO_MODE_RX) {
		// we are not in RX, not CAD, and no data received
		(void)RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
	}
	return false;
}

LOCAL uint8_t RFM95_receive(uint8_t *buf, const uint8_t maxBufSize)
{
	if (buf == NULL) {
		return 0;
	}
	// clear data flag
	RFM95.dataReceived = false;
	const uint8_t payloadLen = min(RFM95.currentPacket.payloadLen, maxBufSize);
	(void)memcpy((void *)buf, (void *)&RFM95.currentPacket.payload, payloadLen);
	return payloadLen;
}

LOCAL bool RFM95_send(const rfm95_packet_t *packet)
{
	RFM95_DEBUG(PSTR("RFM95:SND:LEN=%" PRIu8 "\n"), packet->header.packetLen + 1);
#if defined(MY_DEBUG_VERBOSE_RFM95)
	hwDebugBuf2Str((const uint8_t *)&packet->data, packet->header.packetLen + 1);
	RFM95_DEBUG(PSTR("RFM95:SND:RAW=%s\n"), hwDebugPrintStr);
#endif

	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
	delay(3); // timing for RF startup until RSSI sampling
	const uint32_t CSMA_START_MS = hwMillis();
	bool channelFree;
	do {
		channelFree = RFM95_channelFree();
		doYield();
	} while (!channelFree && (hwMillis() - CSMA_START_MS < MY_RFM95_CSMA_TIMEOUT_MS));
	if (!channelFree) {
		return false;
	}
	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_TX);
	(void)RFM95_burstWriteReg(RFM95_REG_00_FIFO, packet->data,
	                          packet->header.packetLen + 1); // + 1 for length byte
	const uint32_t startTX_MS = hwMillis();
	// send message, if sent, irq fires and radio returns to standby
	while (!RFM95_irq && (hwMillis() - startTX_MS < MY_RFM95_TX_TIMEOUT_MS)) {
		doYield();
	}
	// back to RX
	//RFM95_irq = false; ==> IRQ flag cleared in setRadioMode()
	//const bool result = RFM95_readReg(RFM95_REG_3F_IRQ_FLAGS2) & RFM95_IRQ2_PACKET_SENT;
	(void)RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
	//return result;
	return true;
}

LOCAL bool RFM95_encrypt(const char *key)
{
	return false;
}

LOCAL void RFM95_setFrequency(const uint32_t frequencyHz)
{
	RFM95_DEBUG(PSTR("RFM95:INIT:FREQ=%" PRIu32 "\n"), frequencyHz);
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

LOCAL void RFM95_setAddress(const uint8_t addr)
{
	RFM95.address = addr;
	(void)RFM95_writeReg(RFM95_REG_33_NODE_ADDR, addr);
}

LOCAL uint8_t RFM95_getAddress(void)
{
	return RFM95.address;
}

LOCAL bool RFM95_setRadioMode(const rfm95_radioMode_t newRadioMode)
{
	uint8_t regMode = RFM95_RADIO_MODE_STDBY;
	RFM95.radioMode = newRadioMode;
	RFM95_irq = false;

	if (newRadioMode == RFM95_RADIO_MODE_STDBY) {
		regMode = RFM95_MODE_STDBY;
	} else if (newRadioMode == RFM95_RADIO_MODE_SLEEP) {
		regMode = RFM95_MODE_SLEEP;
	} else if (newRadioMode == RFM95_RADIO_MODE_RX) {
		regMode = RFM95_MODE_RX;
	} else if (newRadioMode == RFM95_RADIO_MODE_TX) {
		regMode = RFM95_MODE_TX;
	}
	(void)RFM95_writeReg(RFM95_REG_01_OP_MODE,
	                     RFM95_FSK_OOK_MODE | RFM95_MODULATION_FSK | RFM95_LOW_FREQUENCY_REG | regMode);
	// wait until mode ready
	// while (!(RFM95_readReg(RFM95_REG_3E_IRQ_FLAGS1) & RFM95_IRQ1_MODE_READY)) {};
	RFM95.radioMode = newRadioMode;
	RFM95_DEBUG(PSTR("RFM95:SRM:MODE=%" PRIu8 "\n"), newRadioMode);
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
	RFM95_setRadioMode(RFM95_RADIO_MODE_SLEEP);
	return true;
}

LOCAL bool RFM95_standBy(void)
{
	RFM95_DEBUG(PSTR("RFM95:RSB\n"));	// put radio to standby
	RFM95_setRadioMode(RFM95_RADIO_MODE_STDBY);
	return true;
}

LOCAL void RFM95_sendACK(const uint8_t recipient, const rfm95_rfm69_sequenceNumber_t sequenceNumber,
                         const rfm95_RSSI_t RSSI)
{
	RFM95_DEBUG(PSTR("RFM95:SAC:SEND ACK,TO=%" PRIu8 ",SEQ=%" PRIu16 ",RSSI=%" PRIi16 "\n"),recipient,
	            sequenceNumber,
	            RFM95_internalToRSSI(RSSI));
	rfm95_packet_t packet;
	packet.ACK.RSSI = RSSI;
	packet.ACK.sequenceNumber = sequenceNumber;
	packet.header.version = RFM95_PACKET_HEADER_VERSION;
	packet.header.sender = RFM95.address;
	packet.header.recipient = recipient;
	packet.header.packetLen = sizeof(rfm95_ack_t) + (sizeof(rfm95_header_t) - 1);
	packet.header.sequenceNumber = ++RFM95.txSequenceNumber;
	packet.header.controlFlags = 0;
	RFM95_setACKReceived(packet.header.controlFlags, true);
	RFM95_setACKRSSIReport(packet.header.controlFlags, true);
	(void)RFM95_send(&packet);
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
                               const uint8_t bufferSize, const bool noACK)
{
	rfm95_packet_t packet;
	packet.header.version = RFM95_PACKET_HEADER_VERSION;
	packet.header.sender = RFM95.address;
	packet.header.recipient = recipient;
	packet.header.sequenceNumber =
	    ++RFM95.txSequenceNumber; 	// increase sequence counter, overflow is ok
	packet.header.controlFlags = 0; // reset all flags
	RFM95_setACKRequested(packet.header.controlFlags, !noACK);
	RFM95_setACKRSSIReport(packet.header.controlFlags, RFM95.ATCenabled);
	(void)memcpy((void *)packet.payload, buffer, bufferSize);
	packet.payloadLen = bufferSize;

	packet.header.packetLen = packet.payloadLen + (sizeof(rfm95_header_t) - 1); // -1 length byte

	for (uint8_t TXAttempt = 0; TXAttempt < RFM95_TX_ATTEMPTS; TXAttempt++) {
		RFM95_DEBUG(PSTR("RFM95:SWR:SEND,TO=%" PRIu8 ",SEQ=%" PRIu8 ",TX=%" PRIu8 "\n"), recipient,
		            RFM95.txSequenceNumber, TXAttempt + 1);

		if (!RFM95_send(&packet)) {
			// CSMA check failed
			continue;
		}
		// radio is in RX
		if (noACK) {
			// no ACK requested
			return true;
		}
		const uint32_t enterMS = hwMillis();
		// progressive wait time with random component
		const uint32_t effectiveWaitingTimeMS = RFM95_RETRY_TIMEOUT_MS + (hwMillis() & 0x3Fu) +
		                                        (TXAttempt * 50u);
		RFM95_DEBUG(PSTR("RFM95:SWR:ACK WAIT=%" PRIu32 "\n"), effectiveWaitingTimeMS);
		while (!RFM95.dataReceived && (hwMillis() - enterMS < effectiveWaitingTimeMS)) {
			RFM95_handling();
			if (RFM95.ackReceived) {
				const uint8_t ACKsender = RFM95.currentPacket.header.sender;
				const rfm95_rfm69_sequenceNumber_t ACKsequenceNumber = RFM95.currentPacket.ACK.sequenceNumber;
				const rfm95_controlFlags_t ACKflags = RFM95.currentPacket.header.controlFlags;
				const rfm95_RSSI_t ACKRSSI = RFM95.currentPacket.ACK.RSSI;
				RFM95.ackReceived = false;
				// packet read, back to RX
				RFM95_setRadioMode(RFM95_RADIO_MODE_RX);
				if (ACKsender == recipient &&
				        (ACKsequenceNumber == RFM95.txSequenceNumber)) {
					RFM95_DEBUG(PSTR("RFM95:SWR:ACK FROM=%" PRIu8 ",SEQ=%" PRIu8 ",RSSI=%" PRIi16 "\n"), ACKsender,
					            ACKsequenceNumber,
					            RFM95_internalToRSSI(ACKRSSI));
					// ATC
					if (RFM95.ATCenabled && RFM95_getACKRSSIReport(ACKflags)) {
						(void)RFM95_executeATC(ACKRSSI, RFM95.ATCtargetRSSI);
					}
					return true;
				} // seq check
			}
			doYield();
		}
		if (RFM95.dataReceived) {
			return false;
		}
		RFM95_DEBUG(PSTR("!RFM95:SWR:NACK,SEQ=%" PRIu8 "\n"), RFM95.txSequenceNumber);
		const uint32_t enterCSMAMS = hwMillis();
		const uint16_t randDelayCSMA = enterMS % 100;
		while (hwMillis() - enterCSMAMS < randDelayCSMA) {
			doYield();
		}
	}
	return false;
}

LOCAL bool RFM95_channelFree(void)
{
	// returns true if channel activity under RFM95_CSMA_LIMIT_DBM
	const rfm95_RSSI_t RSSI = RFM95_readReg(RFM95_REG_11_RSSI_VALUE);
	const uint8_t RSSIflag = RFM95_readReg(RFM95_REG_3E_IRQ_FLAGS1) & 0x08;
	RFM95_DEBUG(PSTR("RFM95:CSMA:RSSI=%" PRIi16 ",REG1=%" PRIu8 "\n"), RFM95_internalToRSSI(RSSI),
	            RSSIflag);
	(void)RSSIflag;
	//RFM95_writeReg(RFM95_REG_3E_IRQ_FLAGS1, 0x08); // clear RSSI flag

	return (RSSI > RFM95_RSSItoInternal(-90/*MY_RFM95_CSMA_LIMIT_DBM*/));
	//return !RSSIflag;
}

LOCAL void RFM95_ATCmode(const bool OnOff, const int16_t targetRSSI)
{
	RFM95.ATCenabled = OnOff;
	RFM95.ATCtargetRSSI = RFM95_RSSItoInternal(targetRSSI);
}

LOCAL bool RFM95_sanityCheck(void)
{
	bool result = true;
	result &= RFM95_readReg(RFM95_REG_28_SYNC_VALUE1) == RFM95_SYNCVALUE1;
	result &= RFM95_readReg(RFM95_REG_29_SYNC_VALUE2) == RFM95_SYNCVALUE2;
	result &= RFM95_readReg(RFM95_REG_30_PACKET_CONFIG1) == RFM95_CONFIG_WHITE;
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
	return INVALID_SNR;
}

LOCAL int16_t RFM95_getReceivingRSSI(void)
{
	// RSSI from last received packet
	return static_cast<int16_t>(RFM95_internalToRSSI(RFM95.currentPacket.RSSI));
}

LOCAL int16_t RFM95_getReceivingSNR(void)
{
	return INVALID_SNR;
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

