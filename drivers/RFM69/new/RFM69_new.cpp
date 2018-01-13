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
 *
 * RFM69 driver refactored for Mysensors
 *
 * Based on :
 * - LowPowerLab RFM69 Lib Copyright Felix Rusu (2014), felix@lowpowerlab.com
 * - Automatic Transmit Power Control class derived from RFM69 library.
 *	  Discussion and details in this forum post: https://lowpowerlab.com/forum/index.php/topic,688.0.html
 *	  Copyright Thomas Studwell (2014,2015)
 * - Mysensors generic radio driver implementation Copyright (C) 2017 Olivier Mauti <olivier@mysensors.org>
 *
 * Changes by : @tekka, @scalz, @marceloagno
 *
 * Definitions for Semtech SX1231/H radios:
 * http://www.semtech.com/images/datasheet/sx1231.pdf
 * http://www.semtech.com/images/datasheet/sx1231h.pdf
 */

#include "RFM69_new.h"

// debug
#if defined(MY_DEBUG_VERBOSE_RFM69)
#define RFM69_DEBUG(x,...)	DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< Debug print
#else
#define RFM69_DEBUG(x,...)	//!< DEBUG null
#endif

volatile rfm69_internal_t RFM69;	//!< internal variables

#if defined (SREG)	&& !defined(SPI_HAS_TRANSACTION)
uint8_t _SREG;		// Used to save and restore the SREG values in SPI transactions
#endif

#if defined (SPCR) && defined (SPSR) && !defined(SPI_HAS_TRANSACTION)
uint8_t _SPCR; //!< _SPCR
uint8_t _SPSR; //!< _SPSR
#endif

#ifdef LINUX_SPI_BCM
// SPI RX and TX buffers (max packet len + 1 byte for the command)
uint8_t spi_rxbuff[RFM69_MAX_PACKET_LEN + 1];
uint8_t spi_txbuff[RFM69_MAX_PACKET_LEN + 1];
#endif

LOCAL void RFM69_csn(const bool level)
{
	hwDigitalWrite(MY_RFM69_CS_PIN, level);
}

LOCAL void RFM69_prepareSPITransaction(void)
{
#if !defined(MY_SOFTSPI) && defined(SPI_HAS_TRANSACTION)
	RFM69_SPI.beginTransaction(SPISettings(MY_RFM69_SPI_SPEED, RFM69_SPI_DATA_ORDER,
	                                       RFM69_SPI_DATA_MODE));
#else
#if defined(SREG)
	_SREG = SREG;
#endif
	noInterrupts();
#if defined(SPCR) && defined(SPSR)
	// save current SPI settings
	_SPCR = SPCR;
	_SPSR = SPSR;
#endif

	// set RFM69 SPI settings
#if !defined(MY_SOFTSPI)
	RFM69_SPI.setDataMode(RFM69_SPI_DATA_MODE);
	RFM69_SPI.setBitOrder(RFM69_SPI_DATA_ORDER);
	RFM69_SPI.setClockDivider(RFM69_CLOCK_DIV);
#endif

#endif
}

LOCAL void RFM69_concludeSPITransaction(void)
{
#if !defined(MY_SOFTSPI) && defined(SPI_HAS_TRANSACTION)
	RFM69_SPI.endTransaction();
#else
	// restore SPI settings to what they were before talking to RFM69
#if defined(SPCR) && defined(SPSR)
	SPCR = _SPCR;
	SPSR = _SPSR;
#endif
	// restore the prior interrupt state
#if defined(SREG)
	SREG = _SREG;
#endif
	interrupts();
#endif
}

LOCAL uint8_t RFM69_spiMultiByteTransfer(const uint8_t cmd, uint8_t* buf, uint8_t len,
        const bool aReadMode)
{
	uint8_t status;
	uint8_t* current = buf;

	RFM69_prepareSPITransaction();
	RFM69_csn(LOW);

#ifdef LINUX_SPI_BCM
	uint8_t * prx = spi_rxbuff;
	uint8_t * ptx = spi_txbuff;
	uint8_t size = len + 1; // Add register value to transmit buffer

	*ptx++ = cmd;
	while (len--) {
		if (aReadMode) {
			*ptx++ = (uint8_t)RFM69_NOP;
		} else {
			*ptx++ = *current++;
		}
	}
	RFM69_SPI.transfernb((char *)spi_txbuff, (char *)spi_rxbuff, size);
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
	status = RFM69_SPI.transfer(cmd);
	while (len--) {
		if (aReadMode) {
			status = RFM69_SPI.transfer((uint8_t)RFM69_NOP);
			if (buf != NULL) {
				*current++ = status;
			}
		} else {
			status = RFM69_SPI.transfer(*current++);
		}
	}
#endif

	RFM69_csn(HIGH);
	RFM69_concludeSPITransaction();

	return status;
}

// low level register access
LOCAL uint8_t RFM69_RAW_readByteRegister(const uint8_t address)
{
	const uint8_t value =  RFM69_spiMultiByteTransfer(address, NULL, 1, true);
	//RFM69_DEBUG(PSTR("RFM69:read register, reg=0x%02" PRIx8 ", value=%" PRIu8 "\n"), address, value);
	return value;
}

// low level register access
LOCAL uint8_t RFM69_RAW_writeByteRegister(const uint8_t address, uint8_t value)
{
	//RFM69_DEBUG(PSTR("RFM69:write register, reg=0x%02" PRIx8 ", value=%" PRIu8 "\n"), address & 0x7F, value);
	return RFM69_spiMultiByteTransfer(address, &value, 1, false);
}

// macros, saves space
#define RFM69_readReg(__reg) RFM69_RAW_readByteRegister(__reg & RFM69_READ_REGISTER)
#define RFM69_writeReg(__reg, __value) RFM69_RAW_writeByteRegister((__reg | RFM69_WRITE_REGISTER), __value )
#define RFM69_burstReadReg(__reg, __buf, __len) RFM69_spiMultiByteTransfer( __reg & RFM69_READ_REGISTER, (uint8_t*)__buf, __len, true )
#define RFM69_burstWriteReg(__reg, __buf, __len) RFM69_spiMultiByteTransfer( __reg | RFM69_WRITE_REGISTER, (uint8_t*)__buf, __len, false )

LOCAL bool RFM69_initialise(const uint32_t frequencyHz)
{
	RFM69_DEBUG(PSTR("RFM69:INIT\n"));
	// power up radio if power pin defined
#if defined(MY_RFM69_POWER_PIN)
	hwPinMode(MY_RFM69_POWER_PIN, OUTPUT);
#endif
	RFM69_powerUp();
	// reset radio module if rst pin defined
#if defined(MY_RFM69_RST_PIN)
	hwPinMode(MY_RFM69_RST_PIN, OUTPUT);
	hwDigitalWrite(MY_RFM69_RST_PIN, HIGH);
	// 100uS
	delayMicroseconds(100);
	hwDigitalWrite(MY_RFM69_RST_PIN, LOW);
	// wait until chip ready
	delay(5);
	RFM69_DEBUG(PSTR("RFM69:INIT:PIN,CS=%" PRIu8 ",IQP=%" PRIu8 ",IQN=%" PRIu8 ",RST=%" PRIu8 "\n"),
	            MY_RFM69_CS_PIN,MY_RFM69_IRQ_PIN,
	            MY_RFM69_IRQ_NUM,MY_RFM69_RST_PIN);
#else
	RFM69_DEBUG(PSTR("RFM69:INIT:PIN,CS=%" PRIu8 ",IQP=%" PRIu8 ",IQN=%" PRIu8 "\n"),MY_RFM69_CS_PIN,
	            MY_RFM69_IRQ_PIN,
	            MY_RFM69_IRQ_NUM);
#endif

	// set variables
	RFM69.address = RFM69_BROADCAST_ADDRESS;
	RFM69.dataReceived = false;
	RFM69.ackReceived = false;
	RFM69.txSequenceNumber = 0;	// initialise TX sequence counter
	RFM69.powerLevel = MY_RFM69_TX_POWER_DBM + 1;	// will be overwritten when set
	RFM69.radioMode = RFM69_RADIO_MODE_SLEEP;
	RFM69.ATCenabled = false;
	RFM69.listenModeEnabled = false;
	RFM69.ATCtargetRSSI = RFM69_RSSItoInternal(RFM69_TARGET_RSSI_DBM);

	// SPI init
	hwDigitalWrite(MY_RFM69_CS_PIN, HIGH);
	hwPinMode(MY_RFM69_CS_PIN, OUTPUT);
	RFM69_SPI.begin();

	RFM69_setConfiguration();
	RFM69_setFrequency(frequencyHz);
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);

	// Encryption is persistent between resets and can trip you up during debugging.
	// Disable it during initialization so we always start from a known state.
	RFM69_encrypt(0);
	(void)RFM69_setTxPowerLevel(MY_RFM69_TX_POWER_DBM);
	// IRQ
	hwPinMode(MY_RFM69_IRQ_PIN, INPUT);
#if defined (SPI_HAS_TRANSACTION) && !defined (ESP8266) && !defined (MY_SOFTSPI)
	RFM69_SPI.usingInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN));
#endif

#ifdef MY_DEBUG_VERBOSE_RFM69_REGISTERS
	RFM69_readAllRegs();
#endif

	if (!RFM69_sanityCheck()) {
		RFM69_DEBUG(
		    PSTR("!RFM69:INIT:SANCHK FAIL\n")); // sanity check failed, check wiring or replace module
		return false;
	}
	attachInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN), RFM69_interruptHandler, RISING);
	return true;
}

// IRQ handler: PayloadReady (RX) & PacketSent (TX) mapped to DI0
LOCAL void RFM69_interruptHandler(void)
{
#if defined(MY_RFM69_ENABLE_LISTENMODE)
	if (RFM69.listenModeEnabled) {
		RFM69_listenModeReset();
		//noInterrupts();
		union {                      // union to simplify addressing of long and short parts of time offset
			uint32_t l;
			uint8_t b[4];
		} burstRemaining;

		burstRemaining.l = 0;
	}
#endif
	const uint8_t regIrqFlags2 = RFM69_readReg(RFM69_REG_IRQFLAGS2);
	if ( RFM69.radioMode == RFM69_RADIO_MODE_RX && (regIrqFlags2 & RFM69_IRQFLAGS2_PAYLOADREADY) ) {
		RFM69.currentPacket.RSSI = RFM69_readRSSI();
		(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
		// use the fifo level irq as indicator if header bytes received
		if (regIrqFlags2 & RFM69_IRQFLAGS2_FIFOLEVEL) {
			RFM69_prepareSPITransaction();
			RFM69_csn(LOW);
#ifdef LINUX_SPI_BCM
			char data[RFM69_MAX_PACKET_LEN + 1];   // max packet len + 1 byte for the command
			data[0] = RFM69_REG_FIFO & RFM69_READ_REGISTER;
			RFM69_SPI.transfern(data, 3);

			RFM69.currentPacket.header.packetLen = data[1];
			RFM69.currentPacket.header.recipient = data[2];

			if (RFM69.currentPacket.header.packetLen > RFM69_MAX_PACKET_LEN) {
				RFM69.currentPacket.header.packetLen = RFM69_MAX_PACKET_LEN;
			}

			data[0] = RFM69_REG_FIFO & RFM69_READ_REGISTER;
			//SPI.transfern(data, RFM69.currentPacket.header.packetLen - 1); //TODO: Wrong packetLen?
			RFM69_SPI.transfern(data, RFM69.currentPacket.header.packetLen);

			//(void)memcpy((void*)&RFM69.currentPacket.data[2], (void*)&data[1], RFM69.currentPacket.header.packetLen - 2);   //TODO: Wrong packetLen?
			(void)memcpy((void*)&RFM69.currentPacket.data[2], (void*)&data[1],
			             RFM69.currentPacket.header.packetLen - 1);

			if (RFM69.currentPacket.header.version >= RFM69_MIN_PACKET_HEADER_VERSION) {
				RFM69.currentPacket.payloadLen = min(RFM69.currentPacket.header.packetLen - (RFM69_HEADER_LEN - 1),
				                                     RFM69_MAX_PACKET_LEN);
				RFM69.ackReceived = RFM69_getACKReceived(RFM69.currentPacket.header.controlFlags);
				RFM69.dataReceived = !RFM69.ackReceived;
			}
#else
			RFM69_SPI.transfer(RFM69_REG_FIFO & RFM69_READ_REGISTER);
			// set reading pointer
			uint8_t* current = (uint8_t*)&RFM69.currentPacket;
			bool headerRead = false;
			// first read header
			uint8_t readingLength = RFM69_HEADER_LEN;
			while (readingLength--) {
				*current++ = RFM69_SPI.transfer((uint8_t)0x00);
				if (!readingLength && !headerRead) {
					// header read
					headerRead = true;
					if (RFM69.currentPacket.header.version >= RFM69_MIN_PACKET_HEADER_VERSION) {
						// read payload
						readingLength = RFM69.currentPacket.header.packetLen - (RFM69_HEADER_LEN - 1);
						if (readingLength > RFM69_MAX_PACKET_LEN) {
							readingLength = RFM69_MAX_PACKET_LEN;
						}
						RFM69.currentPacket.payloadLen = readingLength;
						RFM69.ackReceived = RFM69_getACKReceived(RFM69.currentPacket.header.controlFlags);
						RFM69.dataReceived = !RFM69.ackReceived;
					}
				}
			}
#endif
			RFM69_csn(HIGH);
			RFM69_concludeSPITransaction();
		}
		// radio remains in stdby until paket read
	} else if (RFM69.radioMode == RFM69_RADIO_MODE_TX && (regIrqFlags2 & RFM69_IRQFLAGS2_PACKETSENT) ) {
		RFM69.dataSent = true;
		// back to RX
		(void)RFM69_setRadioMode(RFM69_RADIO_MODE_RX);
	}
}

LOCAL bool RFM69_available(void)
{
	if (RFM69.dataReceived) {
		// data received - we are still in STDBY from IRQ handler
		//(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
		return true;
	} else if (RFM69.radioMode == RFM69_RADIO_MODE_TX) {
		return false;
	} else if (RFM69.radioMode != RFM69_RADIO_MODE_RX) {
		// we are not in RX, and no data received
		RFM69_setRadioMode(RFM69_RADIO_MODE_RX);
	}
	return false;
}



LOCAL uint8_t RFM69_recv(uint8_t* buf, const uint8_t maxBufSize)
{
	// atomic
	noInterrupts();

	const uint8_t payloadLen = RFM69.currentPacket.payloadLen < maxBufSize?
	                           RFM69.currentPacket.payloadLen : maxBufSize;
	const uint8_t sender = RFM69.currentPacket.header.sender;
	const rfm69_sequenceNumber_t sequenceNumber = RFM69.currentPacket.header.sequenceNumber;
	const uint8_t controlFlags = RFM69.currentPacket.header.controlFlags;
	const rfm69_RSSI_t RSSI = RFM69.currentPacket.RSSI;	// of incoming packet

	if (buf != NULL) {
		(void)memcpy((void*)buf, (void*)RFM69.currentPacket.payload, payloadLen);
		// packet read
		RFM69.dataReceived = false;
	}

	interrupts(); // explicitly re-enable interrupts

	if (RFM69_getACKRequested(controlFlags) && !RFM69_getACKReceived(controlFlags)) {
		RFM69_sendACK(sender, sequenceNumber,RSSI);
	}

	return payloadLen;
}

LOCAL bool RFM69_sendFrame(rfm69_packet_t &packet, const bool increaseSequenceCounter)
{
	/*
	if(!RFM69_waitCAD()) {
		// channel not free
		return false;
	}
	*/


	// set radio to standby to load fifo
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
	if (increaseSequenceCounter) {
		// increase sequence counter, overflow is ok
		RFM69.txSequenceNumber++;
	}
	packet.header.sequenceNumber = RFM69.txSequenceNumber;

	RFM69_writeReg(RFM69_REG_PACKETCONFIG2,
	               (RFM69_readReg(RFM69_REG_PACKETCONFIG2) & 0xFB) | RFM69_PACKET2_RXRESTART); // avoid RX deadlocks

	// write packet
	const uint8_t finalLen = packet.payloadLen + RFM69_HEADER_LEN; // including length byte
	RFM69_burstWriteReg(RFM69_REG_FIFO, packet.data, finalLen);

	// send message
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_TX);
	const uint32_t txStart = hwMillis();
	// irq handler set radiomode to rx once packet sent
	while (!RFM69.dataSent && hwMillis() - txStart < RFM69_TX_LIMIT_MS) { // wait until packet sent
		doYield();
	}
	return RFM69.dataSent;
}

LOCAL bool RFM69_send(const uint8_t recipient, uint8_t* data, const uint8_t len,
                      const rfm69_controlFlags_t flags, const bool increaseSequenceCounter)
{
	// assemble packet
	rfm69_packet_t packet;
	packet.header.version = RFM69_PACKET_HEADER_VERSION;
	packet.header.sender = RFM69.address;
	packet.header.recipient = recipient;
	packet.header.controlFlags = 0x00;	// reset
	packet.payloadLen = min(len, (uint8_t)RFM69_MAX_PAYLOAD_LEN);
	packet.header.controlFlags = flags;
	(void)memcpy(&packet.payload, data, packet.payloadLen); // copy payload
	packet.header.packetLen = packet.payloadLen + (RFM69_HEADER_LEN - 1); // -1 length byte
	return RFM69_sendFrame(packet, increaseSequenceCounter);
}

LOCAL void RFM69_setFrequency(const uint32_t frequencyHz)
{
	const uint32_t freqHz = (uint32_t)(frequencyHz / RFM69_FSTEP);
	RFM69_writeReg(RFM69_REG_FRFMSB, freqHz >> 16);
	RFM69_writeReg(RFM69_REG_FRFMID, freqHz >> 8);
	RFM69_writeReg(RFM69_REG_FRFLSB, freqHz);
}

LOCAL void RFM69_setHighPowerRegs(const bool onOff)
{
#if defined(RFM69_VERSION_HW)
	RFM69_writeReg(RFM69_REG_OCP, (onOff ? RFM69_OCP_OFF : RFM69_OCP_ON ) | RFM69_OCP_TRIM_95);
	RFM69_writeReg(RFM69_REG_TESTPA1, onOff ? 0x5D : 0x55);
	RFM69_writeReg(RFM69_REG_TESTPA2, onOff ? 0x7C : 0x70);
#else
	(void)onOff;
#endif
}

LOCAL bool RFM69_setTxPowerLevel(rfm69_powerlevel_t newPowerLevel)
{
	// limit power levels
	newPowerLevel = max((rfm69_powerlevel_t)RFM69_MIN_POWER_LEVEL_DBM, newPowerLevel);
	newPowerLevel = min((rfm69_powerlevel_t)RFM69_MAX_POWER_LEVEL_DBM, newPowerLevel);

	if (RFM69.powerLevel == newPowerLevel) {
		RFM69_DEBUG(PSTR("RFM69:PTX:NO ADJ\n"));
		return false;
	}

	RFM69.powerLevel = newPowerLevel;
	uint8_t palevel;
#if !defined(RFM69_VERSION_HW)
	// -18dBm to +13dBm, PA0, offset 0, i.e. -18dBm = level 0
	palevel = RFM69_PALEVEL_PA0_ON | ((uint8_t)(newPowerLevel + 18));

#else
	if (newPowerLevel <= (rfm69_powerlevel_t)13) {
		// -2dBm to +13dBm, PA1, offset 16, i.e. -2dBm = level 16
		palevel = RFM69_PALEVEL_PA1_ON | ((uint8_t)(newPowerLevel + 18));
	} else if (newPowerLevel >= (rfm69_powerlevel_t)RFM69_HIGH_POWER_DBM) {
		// +18dBm to +20dBm, PA1 and PA2, Boost settings, 18dBm = level 29
		palevel = RFM69_PALEVEL_PA1_ON | RFM69_PALEVEL_PA2_ON | ((uint8_t)(newPowerLevel + 11));
	} else {
		// +14dBm to +17dBm, PA1 and PA2
		palevel = RFM69_PALEVEL_PA1_ON | RFM69_PALEVEL_PA2_ON | ((uint8_t)(newPowerLevel + 14));
	}
#endif
	RFM69_writeReg(RFM69_REG_PALEVEL, palevel);
	RFM69_DEBUG(PSTR("RFM69:PTX:LEVEL=%" PRIi8 " dBm\n"),newPowerLevel);
	return true;
}

LOCAL void RFM69_setAddress(const uint8_t addr)
{
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
	RFM69.address = addr;
	RFM69_writeReg(RFM69_REG_NODEADRS, addr);
}

LOCAL uint8_t RFM69_getAddress(void)
{
	return RFM69.address;
}

LOCAL bool RFM69_setRadioMode(const rfm69_radio_mode_t newRadioMode)
{
	if (RFM69.radioMode == newRadioMode) {
		// no change
		return false;
	}

	uint8_t regMode;

	if (newRadioMode == RFM69_RADIO_MODE_STDBY) {
		regMode = RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_STANDBY;
	} else if (newRadioMode == RFM69_RADIO_MODE_SLEEP) {
		regMode = RFM69_OPMODE_SEQUENCER_OFF | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_SLEEP;
	} else if (newRadioMode == RFM69_RADIO_MODE_RX) {
		RFM69.dataReceived = false;
		RFM69.ackReceived = false;
		regMode = RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_RECEIVER;
		RFM69_writeReg(RFM69_REG_DIOMAPPING1, RFM69_DIOMAPPING1_DIO0_01); // Interrupt on PayloadReady, DIO0
		// disable high power settings
		RFM69_setHighPowerRegs(false);
		RFM69_writeReg(RFM69_REG_PACKETCONFIG2,
		               (RFM69_readReg(RFM69_REG_PACKETCONFIG2) & 0xFB) | RFM69_PACKET2_RXRESTART); // avoid RX deadlocks
	} else if (newRadioMode == RFM69_RADIO_MODE_TX) {
		RFM69.dataSent = false;
		regMode = RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_TRANSMITTER;
		RFM69_writeReg(RFM69_REG_DIOMAPPING1, RFM69_DIOMAPPING1_DIO0_00); // Interrupt on PacketSent, DIO0
		RFM69_setHighPowerRegs(RFM69.powerLevel >= (rfm69_powerlevel_t)RFM69_HIGH_POWER_DBM);
	} else if (newRadioMode == RFM69_RADIO_MODE_SYNTH) {
		regMode = RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_SYNTHESIZER;
	}
#if defined(MY_RFM69_ENABLE_LISTENMODE)
	else if (newRadioMode == RFM69_RADIO_MODE_LISTEN) {
		// Start LISTENMODE
		regMode = RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_ON;

		RFM69.listenModeEnabled = true;

		RFM69_DEBUG(PSTR("RFM69:LSM:Start..\n"));
		while (RFM69_readReg(RFM69_REG_IRQFLAGS2) & RFM69_IRQFLAGS2_PACKETSENT ==
		        0x00); // wait for ModeReady

		RFM69_listenModeReset();
		detachInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN));
		attachInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN), RFM69_interruptHandler, RISING);

		RFM69_writeReg(RFM69_REG_OPMODE,
		               RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_STANDBY); //standby mode

		//TODO _isHighSpeed = true;
		//TODO _haveEncryptKey = false;
		uint32_t rxDuration = RFM69_LISTEN_RX_US;
		uint32_t idleDuration = RFM69_LISTEN_IDLE_US;
		RFM69_listenModeSetDurations(rxDuration, idleDuration);

		RFM69_writeReg(RFM69_REG_DIOMAPPING1, RFM69_DIOMAPPING1_DIO0_01);
		RFM69_writeReg(RFM69_REG_FRFMSB, RFM69_readReg(RFM69_REG_FRFMSB) + 1);
		RFM69_writeReg(RFM69_REG_FRFLSB,
		               RFM69_readReg(RFM69_REG_FRFLSB));      // MUST write to LSB to affect change!
		RFM69_listenModeApplyHighSpeedSettings();
		RFM69_writeReg(RFM69_REG_PACKETCONFIG1,
		               RFM69_PACKET1_FORMAT_VARIABLE | RFM69_PACKET1_DCFREE_WHITENING | RFM69_PACKET1_CRC_ON |
		               RFM69_PACKET1_CRCAUTOCLEAR_ON);
		RFM69_writeReg(RFM69_REG_PACKETCONFIG2,
		               RFM69_PACKET2_RXRESTARTDELAY_NONE | RFM69_PACKET2_AUTORXRESTART_ON | RFM69_PACKET2_AES_OFF);
		RFM69_writeReg(RFM69_REG_SYNCVALUE1, 0x5A);
		RFM69_writeReg(RFM69_REG_SYNCVALUE2, 0x5A);
		RFM69_setListenConfig(idleListenResolution, rxListenResolution, RFM69_LISTEN1_CRITERIA_RSSI,
		                      RFM69_LISTEN1_END_10);
		RFM69_setListenCoefIdle(idleListenCoef);
		RFM69_setListenCoefRx(rxListenCoef);
		RFM69_writeReg(RFM69_REG_RSSITHRESH, 180);
		RFM69_writeReg(RFM69_REG_RXTIMEOUT2, 75);
		RFM69_writeReg(RFM69_REG_OPMODE, RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_STANDBY);
		RFM69_writeReg(RFM69_REG_OPMODE,
		               RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_ON | RFM69_OPMODE_STANDBY);
		//WIP
	}
#endif
	else {
		regMode = RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_STANDBY;
	}

	// set new mode
	RFM69_writeReg(RFM69_REG_OPMODE, regMode);

	// Waking from sleep mode may take longer
	if (RFM69.radioMode == RFM69_RADIO_MODE_SLEEP) {
		// wait for ModeReady
		if (!RFM69_isModeReady()) {
			return false;
		}
	}
	RFM69.radioMode = newRadioMode;
	return true;
}

LOCAL void RFM69_powerUp(void)
{
#if defined(MY_RFM69_POWER_PIN)
	hwDigitalWrite(MY_RFM69_POWER_PIN, HIGH);
	delay(RFM69_POWERUP_DELAY_MS);
#endif
}
LOCAL void RFM69_powerDown(void)
{
#if defined(MY_RFM69_POWER_PIN)
	hwDigitalWrite(MY_RFM69_POWER_PIN, LOW);
#endif
}

LOCAL bool RFM69_sleep(void)
{
	RFM69_DEBUG(PSTR("RFM69:RSL\n"));	// put radio to sleep
	return RFM69_setRadioMode(RFM69_RADIO_MODE_SLEEP);
}

LOCAL bool RFM69_standBy(void)
{
	RFM69_DEBUG(PSTR("RFM69:RSB\n"));	// put radio to standby
	return RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
}


// should be called immediately after reception in case sender wants ACK
LOCAL void RFM69_sendACK(const uint8_t recipient, const rfm69_sequenceNumber_t sequenceNumber,
                         const rfm69_RSSI_t RSSI)
{
	RFM69_DEBUG(PSTR("RFM69:SAC:SEND ACK,TO=%" PRIu8 ",RSSI=%" PRIi16 "\n"),recipient,
	            RFM69_internalToRSSI(RSSI));
	rfm69_ack_t ACK;
	ACK.sequenceNumber = sequenceNumber;
	ACK.RSSI = RSSI;
	rfm69_controlFlags_t flags = 0x00;	// reset flags
	RFM69_setACKReceived(flags, true);
	RFM69_setACKRSSIReport(flags, true);
	(void)RFM69_send(recipient, (uint8_t*)&ACK, sizeof(rfm69_ack_t), flags);
}

LOCAL bool RFM69_executeATC(const rfm69_RSSI_t currentRSSI, const rfm69_RSSI_t targetRSSI)
{
	rfm69_powerlevel_t newPowerLevel = RFM69.powerLevel;
	if (RFM69_internalToRSSI(currentRSSI) < RFM69_internalToRSSI(targetRSSI *
	        (1 + RFM69_ATC_TARGET_RANGE_PERCENT/100)) &&
	        newPowerLevel < RFM69_MAX_POWER_LEVEL_DBM) {
		// increase transmitter power
		newPowerLevel++;
	} else if (RFM69_internalToRSSI(currentRSSI) > RFM69_internalToRSSI(targetRSSI *
	           (1 - RFM69_ATC_TARGET_RANGE_PERCENT/100)) &&
	           newPowerLevel > RFM69_MIN_POWER_LEVEL_DBM) {
		// decrease transmitter power
		newPowerLevel--;
	} else {
		// nothing to adjust
		return false;
	}
	RFM69_DEBUG(PSTR("RFM69:ATC:ADJ TXL,cR=%" PRIi16 ",tR=%" PRIi16 ",TXL=%" PRIi16 "\n"),
	            RFM69_internalToRSSI(currentRSSI),
	            RFM69_internalToRSSI(targetRSSI), newPowerLevel);

	return RFM69_setTxPowerLevel(newPowerLevel);
}

LOCAL void RFM69_ATCmode(const bool onOff, const int16_t targetRSSI)
{
	RFM69.ATCenabled = onOff;
	RFM69.ATCtargetRSSI = RFM69_RSSItoInternal(targetRSSI);
}


LOCAL bool RFM69_sendWithRetry(const uint8_t recipient, const void* buffer,
                               const uint8_t bufferSize, const uint8_t retries, const uint32_t retryWaitTimeMS)
{
	for (uint8_t retry = 0; retry <= retries; retry++) {
		RFM69_DEBUG(PSTR("RFM69:SWR:SEND,TO=%" PRIu8 ",RETRY=%" PRIu8 "\n"), recipient, retry);
		rfm69_controlFlags_t flags = 0x00; // reset all flags
		RFM69_setACKRequested(flags, (recipient != RFM69_BROADCAST_ADDRESS));
		RFM69_setACKRSSIReport(flags, RFM69.ATCenabled);
		(void)RFM69_send(recipient, (uint8_t*)buffer, bufferSize, flags, !retry);
		if (recipient == RFM69_BROADCAST_ADDRESS) {
			// no ACK requested
			return true;
		}
		// radio is in RX
		const uint32_t enterMS = hwMillis();
		while (hwMillis() - enterMS < retryWaitTimeMS) {
			if (RFM69.ackReceived) {
				// radio is in stdby
				const uint8_t sender = RFM69.currentPacket.header.sender;
				const rfm69_sequenceNumber_t ACKsequenceNumber = RFM69.currentPacket.ACK.sequenceNumber;
				const rfm69_controlFlags_t flags = RFM69.currentPacket.header.controlFlags;
				const rfm69_RSSI_t RSSI = RFM69.currentPacket.ACK.RSSI;
				RFM69.ackReceived = false;
				// packet read, back to RX
				RFM69_setRadioMode(RFM69_RADIO_MODE_RX);
				if (sender == recipient && ACKsequenceNumber == RFM69.txSequenceNumber) {
					RFM69_DEBUG(PSTR("RFM69:SWR:ACK,FROM=%" PRIu8 ",SEQ=%" PRIu8 ",RSSI=%" PRIi16 "\n"),sender,
					            ACKsequenceNumber,
					            RFM69_internalToRSSI(RSSI));

					// ATC
					if (RFM69.ATCenabled && RFM69_getACKRSSIReport(flags)) {
						(void)RFM69_executeATC(RSSI,RFM69.ATCtargetRSSI);
					} // ATC
					// return to RX
					return true;
				} // seq check
			}
			doYield();
		}
		RFM69_DEBUG(PSTR("!RFM69:SWR:NACK\n"));
		if (RFM69.ATCenabled) {
			// No ACK received, maybe out of reach: increase power level
			RFM69_setTxPowerLevel(RFM69.powerLevel + 1);
		}
		// random 100ms
		const uint32_t enterCSMAMS = hwMillis();
		const uint16_t randDelayCSMA = enterMS & 40; // 61 bytes + ACK take ~40ms
		while (hwMillis() - enterCSMAMS < randDelayCSMA) {
			doYield();
		}
	}
	return false;
}

LOCAL int16_t RFM69_getSendingRSSI(void)
{
	// own RSSI, as measured by the recipient - ACK part
	if (RFM69_getACKRSSIReport(RFM69.currentPacket.header.controlFlags)) {
		return RFM69_internalToRSSI(RFM69.currentPacket.ACK.RSSI);
	} else {
		// not valid
		return 127;
	}
}

LOCAL int16_t RFM69_getReceivingRSSI(void)
{
	// RSSI from sender
	return RFM69_internalToRSSI(RFM69.currentPacket.RSSI);
}

LOCAL bool RFM69_setTxPowerPercent(uint8_t newPowerPercent)
{
	newPowerPercent = min(newPowerPercent, (uint8_t)100);	// limit
	const rfm69_powerlevel_t newPowerLevel = static_cast<rfm69_powerlevel_t>
	        (RFM69_MIN_POWER_LEVEL_DBM + (RFM69_MAX_POWER_LEVEL_DBM
	                                      - RFM69_MIN_POWER_LEVEL_DBM) * (newPowerPercent / 100.0f));
	RFM69_DEBUG(PSTR("RFM69:SPP:PCT=%" PRIu8 ",TX LEVEL=%" PRIi8 "\n"), newPowerPercent,newPowerLevel);
	return RFM69_setTxPowerLevel(newPowerLevel);
}



LOCAL rfm69_powerlevel_t RFM69_getTxPowerLevel(void)
{
	// report TX level in dBm
	return RFM69.powerLevel;
}

LOCAL uint8_t RFM69_getTxPowerPercent(void)
{
	// report TX level in %
	const uint8_t result = static_cast<uint8_t>(100.0f * (RFM69.powerLevel -
	                       RFM69_MIN_POWER_LEVEL_DBM) /
	                       (RFM69_MAX_POWER_LEVEL_DBM
	                        - RFM69_MIN_POWER_LEVEL_DBM));
	return result;
}

LOCAL bool RFM69_sanityCheck(void)
{
	bool result = true; // default
	result &= RFM69_readReg(RFM69_REG_DATAMODUL) == (RFM69_DATAMODUL_DATAMODE_PACKET |
	          RFM69_DATAMODUL_MODULATIONTYPE_FSK | RFM69_DATAMODUL_MODULATIONSHAPING_00);
	// check Bitrate
	result &= RFM69_readReg(RFM69_REG_BITRATEMSB) == MY_RFM69_BITRATE_MSB;
	result &= RFM69_readReg(RFM69_REG_BITRATELSB) == MY_RFM69_BITRATE_LSB;
	// default: 5KHz, (FDEV + BitRate / 2 <= 500KHz)
	result &= RFM69_readReg(RFM69_REG_FDEVMSB) == RFM69_FDEVMSB_50000;
	result &= RFM69_readReg(RFM69_REG_FDEVLSB) == RFM69_FDEVLSB_50000;
	// check network
	result &= RFM69_readReg(RFM69_REG_SYNCVALUE2) == MY_RFM69_NETWORKID;
	return result;
}

LOCAL void RFM69_setConfiguration(void)
{
	const uint8_t CONFIG[][2] = {
		{ RFM69_REG_OPMODE, RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_OFF | RFM69_OPMODE_STANDBY },
		{ RFM69_REG_DATAMODUL, RFM69_DATAMODUL_DATAMODE_PACKET | RFM69_DATAMODUL_MODULATIONTYPE_FSK | RFM69_DATAMODUL_MODULATIONSHAPING_00 },
		{ RFM69_REG_BITRATEMSB, MY_RFM69_BITRATE_MSB },
		{ RFM69_REG_BITRATELSB, MY_RFM69_BITRATE_LSB },
		{ RFM69_REG_FDEVMSB, RFM69_FDEVMSB_50000 },
		{ RFM69_REG_FDEVLSB, RFM69_FDEVLSB_50000 },
		//{ RFM69_REG_FRFMSB, RFM69_FREQ_MSB },
		//{ RFM69_REG_FRFMID, RFM69_FREQ_MID },
		//{ RFM69_REG_FRFLSB, RFM69_FREQ_LSB },
		{ RFM69_REG_LNA, RFM69_LNA_ZIN_200 | RFM69_LNA_CURRENTGAIN },
		{ RFM69_REG_RXBW, RFM69_RXBW_DCCFREQ_010 | RFM69_RXBW_MANT_16 | RFM69_RXBW_EXP_2 },
		//{ RFM69_REG_DIOMAPPING1, RFM69_DIOMAPPING1_DIO0_01 },
		{ RFM69_REG_DIOMAPPING2, RFM69_DIOMAPPING2_CLKOUT_OFF },
		{ RFM69_REG_IRQFLAGS2, RFM69_IRQFLAGS2_FIFOOVERRUN },
		{ RFM69_REG_RSSITHRESH, 220 /*RFM69_RSSITHRESH_VALUE*/ },
		{ RFM69_REG_PREAMBLEMSB, 0 },	// default
		{ RFM69_REG_PREAMBLELSB, 3 },	// default
		{ RFM69_REG_SYNCCONFIG, RFM69_SYNC_ON | RFM69_SYNC_FIFOFILL_AUTO | RFM69_SYNC_SIZE_2 | RFM69_SYNC_TOL_0 },
		{ RFM69_REG_SYNCVALUE1, RFM69_SYNCVALUE1 },
		{ RFM69_REG_SYNCVALUE2, MY_RFM69_NETWORKID },
		// changed from dcfree_none to dcfree_whitening
		{ RFM69_REG_PACKETCONFIG1, RFM69_PACKET1_FORMAT_VARIABLE | RFM69_PACKET1_DCFREE_WHITENING | RFM69_PACKET1_CRC_ON | RFM69_PACKET1_CRCAUTOCLEAR_ON | RFM69_PACKET1_ADRSFILTERING_NODEBROADCAST },
		{ RFM69_REG_PAYLOADLENGTH, 66 }, // in variable length mode: the max frame size, not used in TX
		{ RFM69_REG_NODEADRS, RFM69_BROADCAST_ADDRESS },	// init
		{ RFM69_REG_BROADCASTADRS, RFM69_BROADCAST_ADDRESS },
		{ RFM69_REG_FIFOTHRESH, RFM69_FIFOTHRESH_TXSTART_FIFOTHRESH | (RFM69_HEADER_LEN - 1) },	// start transmitting when rfm69 header loaded, fifo level irq when header bytes received (irq asserted when n bytes exceeded)
		{ RFM69_REG_PACKETCONFIG2, RFM69_PACKET2_RXRESTARTDELAY_2BITS | RFM69_PACKET2_AUTORXRESTART_OFF | RFM69_PACKET2_AES_OFF },
		{ RFM69_REG_TESTDAGC, RFM69_DAGC_IMPROVED_LOWBETA0 },
		{ 255, 0}
	};

	for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
		RFM69_writeReg(CONFIG[i][0], CONFIG[i][1]);
	}

}

LOCAL bool RFM69_isModeReady(void)
{
	const uint32_t enterMS = hwMillis();
	while (hwMillis() - enterMS < RFM69_MODE_READY_TIMEOUT_MS) {
		if (RFM69_readReg(RFM69_REG_IRQFLAGS1) & RFM69_IRQFLAGS1_MODEREADY) {
			return true;
		}
	}
	return false;
}

LOCAL void RFM69_encrypt(const char* key)
{
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
	if (key != NULL) {
		RFM69_burstWriteReg(RFM69_REG_AESKEY1, key, 16);
	}
	RFM69_writeReg(RFM69_REG_PACKETCONFIG2,
	               (RFM69_readReg(RFM69_REG_PACKETCONFIG2) & 0xFE) | (key ? 1 : 0));
}

LOCAL rfm69_RSSI_t RFM69_readRSSI(bool forceTrigger)
{
	// RSSI trigger not needed if DAGC is in continuous mode (we are)
	if (forceTrigger) {
		RFM69_writeReg(RFM69_REG_RSSICONFIG, RFM69_RSSI_START);
		uint16_t timeout = 0xFFFF;
		while (!(RFM69_readReg(RFM69_REG_RSSICONFIG) & RFM69_RSSI_DONE) && timeout--) {
		};
	}
	return RFM69_readReg(RFM69_REG_RSSIVALUE);
}

/* UNUSED

LOCAL uint8_t RFM69_getVersion(void)
{
	return RFM69_readReg(RFM69_REG_VERSION);
}

LOCAL uint8_t RFM69_readTemperature(const uint8_t calFactor)
{
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
	RFM69_writeReg(RFM69_REG_TEMP1, RFM69_TEMP1_MEAS_START);
	const uint32_t enterMS = hwMillis();
	while ((RFM69_readReg(RFM69_REG_TEMP1) & RFM69_TEMP1_MEAS_RUNNING) && hwMillis() - enterMS < 500);
	return ~RFM69_readReg(RFM69_REG_TEMP2) + RFM69_COURSE_TEMP_COEF +
	       calFactor; // 'complement' corrects the slope, rising temp = rising val
}

LOCAL void RFM69_rcCalibration(void)
{
	RFM69_writeReg(RFM69_REG_OSC1, RFM69_OSC1_RCCAL_START);
	const uint32_t enterMS = hwMillis();
	while (!(RFM69_readReg(RFM69_REG_OSC1) & RFM69_OSC1_RCCAL_DONE) && hwMillis() - enterMS < 500) {
	};
}

LOCAL uint8_t RFM69_setLNA(const uint8_t newReg)
{
	const uint8_t oldReg = RFM69_readReg(RFM69_REG_LNA);
	RFM69_writeReg(RFM69_REG_LNA, ((newReg & 7) | (oldReg &
	                               ~7)));   // just control the LNA Gain bits for now
	return oldReg;  // return the original value in case we need to restore it
}

LOCAL void RFM69_ATCmode(const bool onOff, const int16_t targetRSSI)
{
	RFM69.ATCenabled = onOff;
	RFM69.ATCtargetRSSI = RFM69_RSSItoInternal(targetRSSI);
}

LOCAL bool RFM69_waitCAD(void)
{
	const uint32_t enterMS = hwMillis();
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_RX);
	while (RFM69.radioMode == RFM69_RADIO_MODE_RX && !RFM69.dataReceived &&
		RFM69_RSSItoInternal(RFM69_readRSSI()) > RFM69_RSSItoInternal(RFM69_CSMA_LIMIT_DBM)) {
		if (hwMillis() - enterMS > RFM69_CSMA_TIMEOUT_MS) {
			return false;
		}
	}
	return true;
}

*/


// ************************  LISTENMODE SECTION   ************************

#if defined(MY_RFM69_ENABLE_LISTENMODE)
LOCAL void RFM69_setListenConfig(const uint8_t listenResolIdle, const uint8_t listenResolRx,
                                 const uint8_t listenCriteria, const uint8_t listenEnd)
{
	/*
	ListenResolIdle one of:
	RFM69_LISTEN_RESOL_IDLE_64US
	RFM69_LISTEN_RESOL_IDLE_4_1MS
	RFM69_LISTEN_RESOL_IDLE_262MS
	Sets the listen mode idle time to 64 usec, 4.1ms or 262 ms
	ListenResolRx one of:
	RFM69_LISTEN_RESOL_RX_64US
	RFM69_LISTEN_RESOL_RX_4_1MS
	RFM69_LISTEN_RESOL_RX_262MS
	Sets the listen mode Rx time to 64 usec, 4.1ms or 262 ms
	ListenCriteria
	Criteria for packet acceptance in listen mode, either:
	RFM69_LISTEN_CRITERIA_RSSI
	signal strength above RssiThreshold
	RFM69_LISTEN_CRITERIA_RSSI_SYNC
	Signal strength above RssiThresshold and SyncAddress matched.
	ListenEnd:
	Action taken after acceptance of a packet in Listen mode:
	RFM69_LISTEN_END_STAY_RX_LISTEN_STOP
	Chip stays in Rx mode, listen mode stops and must be disabled.
	RFM69_LISTEN_END_RX_UNTIL_LISTEN_STOP
	Chip stays in Rx mode until PayloadReady or Timeout interrupt
	occurs. Listen mode stops and must be disabled.
	RFM69_LISTEN_END_RX_UNTIL_LISTEN_RESUME
	Chip stays in Rx mode until PayloadReady or Timeout interrupt
	occurs. Listen mode then resumes in Idle State.
	FIFO lost at next RX wakeup.
	Default: (RFM69_LISTEN_RESOL_IDLE_4_1MS, RFM69_LISTEN_RESOL_RX_64US,
	RFM69_LISTEN_CRITERIA_RSSI, RFM69_LISTEN_END_RX_UNTIL_LISTEN_STOP)
	*/

	RFM69_writeReg(RFM69_REG_LISTEN1, listenResolIdle | listenResolRx | listenCriteria | listenEnd);
}

LOCAL void RFM69_setListenCoefIdle(const uint8_t coeffIdle)
{
	/*
	Duration of the Idle phase in Listen mode.
	t ListenIdle = ListenCoefIdle * ListenResolIdle
	Default 0xf5; 245
	*/

	RFM69_writeReg(RFM69_REG_LISTEN2, coeffIdle);
}

LOCAL void RFM69_setListenCoefRx(const uint8_t coeffRX)
{
	/*
	Duration of the Rx phase in Listen mode.
	t ListenRx = ListenCoefRx * ListenResolRx
	Default 0x20; 32
	*/

	RFM69_writeReg(RFM69_REG_LISTEN3, coeffRX);
}

LOCAL void RFM69_listenModeApplyHighSpeedSettings(void)
{
	if (!_isHighSpeed) {
		return;
	}
	RFM69_writeReg(RFM69_REG_BITRATEMSB, RFM69_BITRATEMSB_200000);
	RFM69_writeReg(RFM69_REG_BITRATELSB, RFM69_BITRATELSB_200000);
	RFM69_writeReg(RFM69_REG_FDEVMSB, RFM69_FDEVMSB_100000);
	RFM69_writeReg(RFM69_REG_FDEVLSB, RFM69_FDEVLSB_100000);
	RFM69_writeReg(RFM69_REG_RXBW, RFM69_RXBW_DCCFREQ_000 | RFM69_RXBW_MANT_20 | RFM69_RXBW_EXP_0);

	// Force LNA to the highest gain
	//RFM69_writeReg(RFM69_REG_LNA, (RFM69_readReg(REG_LNA) << 2) | RF_LNA_GAINSELECT_MAX);
}

LOCAL void RFM69_listenModeReset(void)
{
	// reset vars

	//DATALEN = 0;
	//SENDERID = 0;
	//TARGETID = 0;
	//PAYLOADLEN = 0;
	//ACK_REQUESTED = 0;
	//ACK_RECEIVED = 0;
	//LISTEN_BURST_REMAINING_MS = 0;
}

LOCAL void RFM69_ListenModeStart(void)
{
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_LISTEN);
	/*
	RFM69_DEBUG(PSTR("RFM69:LSM:Start..\n"));
	while (RFM69_readReg(RFM69_REG_IRQFLAGS2) & RFM69_IRQFLAGS2_PACKETSENT == 0x00); // wait for ModeReady
	RFM69_listenModeReset();

	detachInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN));
	attachInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN), RFM69_interruptHandler, RISING);

	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);

	RFM69_writeReg(RFM69_REG_DIOMAPPING1, RFM69_DIOMAPPING1_DIO0_01);
	RFM69_writeReg(RFM69_REG_FRFMSB, RFM69_readReg(RFM69_REG_FRFMSB) + 1);
	RFM69_writeReg(RFM69_REG_FRFLSB, RFM69_readReg(RFM69_REG_FRFLSB));      // MUST write to LSB to affect change!

	RFM69_listenModeApplyHighSpeedSettings();

	RFM69_writeReg(RFM69_REG_PACKETCONFIG1, RFM69_PACKET1_FORMAT_VARIABLE | RFM69_PACKET1_DCFREE_WHITENING | RFM69_PACKET1_CRC_ON | RFM69_PACKET1_CRCAUTOCLEAR_ON);
	RFM69_writeReg(RFM69_REG_PACKETCONFIG2, RFM69_PACKET2_RXRESTARTDELAY_NONE | RFM69_PACKET2_AUTORXRESTART_ON | RFM69_PACKET2_AES_OFF);
	RFM69_writeReg(RFM69_REG_SYNCVALUE1, 0x5A);
	RFM69_writeReg(RFM69_REG_SYNCVALUE2, 0x5A);

	RFM69_setListenConfig(idleListenResolution, rxListenResolution, RFM69_LISTEN1_CRITERIA_RSSI, RFM69_LISTEN1_END_10);
	RFM69_setListenCoefIdle(idleListenCoef);
	RFM69_setListenCoefRx(rxListenCoef);

	RFM69_writeReg(RFM69_REG_RSSITHRESH, 180);
	RFM69_writeReg(RFM69_REG_RXTIMEOUT2, 75);
	RFM69_writeReg(RFM69_REG_OPMODE, RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_STANDBY);
	RFM69_writeReg(RFM69_REG_OPMODE, RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTEN_ON | RFM69_OPMODE_STANDBY);
	*/
}

LOCAL bool RFM69_listenModeEnd(void)
{
	RFM69_DEBUG(PSTR("RFM69:LSM:End..\n"));
	detachInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN));

	RFM69_writeReg(RFM69_REG_OPMODE,
	               RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_LISTENABORT | RFM69_OPMODE_STANDBY);
	RFM69_writeReg(RFM69_REG_OPMODE, RFM69_OPMODE_SEQUENCER_ON | RFM69_OPMODE_STANDBY);
	RFM69_writeReg(RFM69_REG_RXTIMEOUT2, 0);

	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);

	// wait for ModeReady
	//if (!rfm69_ismodeready()) {
	//	return false;
	//}
	RFM69_isModeReady();
	RFM69_listenModeReset();
	RFM69_reinitRadio();
}

LOCAL bool RFM69_reinitRadio(void)
{
	if (!RFM69_initialise(MY_RFM69_FREQUENCY)) {
		return false;
	}
	//TODO
	//if (_haveEncryptKey) RFM69_encrypt(_encryptKey); // Restore the encryption key if necessary
	//if (_isHighSpeed) RFM69_writeReg(RFM69_REG_LNA, (RFM69_readReg(RFM69_REG_LNA) & ~0x3) | RFM69_LNA_GAINSELECT_AUTO);
	return true;
}

LOCAL void RFM69_listenModeSendBurst(const uint8_t recipient, uint8_t* data, const uint8_t len)
{

	detachInterrupt(digitalPinToInterrupt(MY_RFM69_IRQ_PIN));
	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_STDBY);
	RFM69_writeReg(RFM69_REG_PACKETCONFIG1,
	               RFM69_PACKET1_FORMAT_VARIABLE | RFM69_PACKET1_DCFREE_WHITENING | RFM69_PACKET1_CRC_ON |
	               RFM69_PACKET1_CRCAUTOCLEAR_ON);
	RFM69_writeReg(RFM69_REG_PACKETCONFIG2,
	               RFM69_PACKET2_RXRESTARTDELAY_NONE | RFM69_PACKET2_AUTORXRESTART_ON | RFM69_PACKET2_AES_OFF);
	RFM69_writeReg(RFM69_REG_SYNCVALUE1, 0x5A);
	RFM69_writeReg(RFM69_REG_SYNCVALUE2, 0x5A);
	RFM69_listenModeApplyHighSpeedSettings();
	RFM69_writeReg(RFM69_REG_FRFMSB, RFM69_readReg(RFM69_REG_FRFMSB) + 1);
	RFM69_writeReg(RFM69_REG_FRFLSB,
	               RFM69_readReg(RFM69_REG_FRFLSB));      // MUST write to LSB to affect change!

	// TODO set high power level

	union { // union to simplify addressing of long and short parts of time offset
		int32_t l;
		uint8_t b[4];
	} timeRemaining;

	uint16_t cycleDurationMs = listenCycleDurationUs / 1000;

	timeRemaining.l = cycleDurationMs;

	RFM69_DEBUG(PSTR("RFM69:LSM:Send burst for %d ms\n"), cycleDurationMs);

	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_TX);
	uint32_t startTime = hwMillis();

	while (timeRemaining.l > 0) {
		// send burst
		// write to FIFO  TODO refactorize with func, check if send/sendframe could be used, and prepare packet struct
		RFM69_prepareSPITransaction();
		RFM69_csn(LOW);
		RFM69_SPI.transfer(RFM69_REG_FIFO | 0x80);
		RFM69_SPI(len +
		          4);      // two bytes for target and sender node, two bytes for the burst time remaining
		RFM69_SPI.transfer(recipient);
		RFM69_SPI.transfer(_address);

		// We send the burst time remaining with the packet so the receiver knows how long to wait before trying to reply
		RFM69_SPI.transfer(timeRemaining.b[0]);
		RFM69_SPI.transfer(timeRemaining.b[1]);

		for (uint8_t i = 0; i < len; i++) {
			RFM69_SPI.transfer(((uint8_t*)data)[i]);
		}

		RFM69_csn(HIGH);
		RFM69_concludeSPITransaction();

		while ((RFM69_readReg(RFM69_REG_IRQFLAGS2) & RFM69_IRQFLAGS2_FIFONOTEMPTY) !=
		        0x00);  // make sure packet is sent before putting more into the FIFO
		timeRemaining.l = cycleDurationMs - (hwMillis() - startTime);
	}

	(void)RFM69_setRadioMode(RFM69_RADIO_MODE_TX);
	RFM69_reinitRadio();
}

LOCAL uint32_t RFM69_getUsForResolution(uint8_t resolution)
{
	switch (resolution) {
	case RFM69_LISTEN1_RESOL_RX_64:
	case RFM69_LISTEN1_RESOL_IDLE_64:
		return 64;
	case RFM69_LISTEN1_RESOL_RX_4100:
	case RFM69_LISTEN1_RESOL_IDLE_4100:
		return 4100;
	case RFM69_LISTEN1_RESOL_RX_262000:
	case RFM69_LISTEN1_RESOL_IDLE_262000:
		return 262000;
	default:
		// Whoops
		return 0;
	}
}

LOCAL uint32_t RFM69_getCoefForResolution(uint8_t resolution, uint32_t duration)
{
	uint32_t resolDuration = RFM69_getUsForResolution(resolution);
	uint32_t result = duration / resolDuration;

	// If the next-higher coefficient is closer, use that
	if (abs(duration - ((result + 1) * resolDuration)) < abs(duration - (result * resolDuration))) {
		return result + 1;
	}

	return result;
}

LOCAL bool RFM69_chooseResolutionAndCoef(uint8_t *resolutions, uint32_t duration, uint8_t& resolOut,
        uint8_t& coefOut)
{
	for (int i = 0; resolutions[i]; i++) {
		uint32_t coef = RFM69_getCoefForResolution(resolutions[i], duration);
		if (coef <= 255) {
			coefOut = coef;
			resolOut = resolutions[i];
			return true;
		}
	}

	// out of range
	return false;
}

LOCAL bool RFM69_listenModeSetDurations(uint32_t& rxDuration, uint32_t& idleDuration)
{
	uint8_t rxResolutions[] = { RFM69_LISTEN1_RESOL_RX_64, RFM69_LISTEN1_RESOL_RX_4100, RFM69_LISTEN1_RESOL_RX_262000, 0 };
	uint8_t idleResolutions[] = { RFM69_LISTEN1_RESOL_IDLE_64, RFM69_LISTEN1_RESOL_IDLE_4100, RFM69_LISTEN1_RESOL_IDLE_262000, 0 };

	if (!RFM69_chooseResolutionAndCoef(rxResolutions, rxDuration, rxListenResolution, rxListenCoef)) {
		return false;
	}

	if (!RFM69_chooseResolutionAndCoef(idleResolutions, idleDuration, idleListenResolution,
	                                   idleListenCoef)) {
		return false;
	}

	rxDuration = RFM69_getUsForResolution(rxListenResolution) * rxListenCoef;
	idleDuration = RFM69_getUsForResolution(idleListenResolution) * idleListenCoef;
	listenCycleDurationUs = rxDuration + idleDuration;

	return true;
}

LOCAL void RFM69_listenModeGetDurations(uint32_t &rxDuration, uint32_t &idleDuration)
{
	rxDuration = RFM69_getUsForResolution(rxListenResolution) * rxListenCoef;
	idleDuration = RFM69_getUsForResolution(idleListenResolution) * idleListenCoef;
}

#endif

#ifdef MY_DEBUG_VERBOSE_RFM69_REGISTERS

LOCAL void RFM69_readAllRegs(void)
{
#ifdef RFM69_REGISTER_DETAIL
	int16_t capVal;

	//... State Variables for intelligent decoding
	uint8_t modeFSK = 0;
	int16_t bitRate = 0;
	int16_t freqDev = 0;
	int32_t freqCenter = 0;
#endif

	RFM69_DEBUG(PSTR("RFM69:DUMP:Registers Address | HEX value \n"));

	for (uint8_t regAddr = 1; regAddr <= 0x4F; regAddr++) {
		uint8_t regVal = RFM69_readReg(regAddr);
		RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x Value=0x%02x\n"), regAddr, regVal);
#ifdef RFM69_REGISTER_DETAIL
		switch (regAddr) {
		case 0x1: {
			RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x Controls the automatic Sequencer(see section 4.2)\n"),
			            regAddr);

			if (0x80 & regVal) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:SequencerOff : 1 -> Mode is forced by the user\n"));
			} else {
				RFM69_DEBUG(
				    PSTR("RFM69:DUMP:SequencerOff : 0 -> Operating mode as selected with Mode bits in RegOpMode is automatically reached with the Sequencer\n"));
			}

			RFM69_DEBUG(PSTR("RFM69:DUMP:Enables Listen mode, should be enabled whilst in Standby mode\n"));
			if (0x40 & regVal) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenOn : 1 -> On\n"));
			} else {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenOn : 0->Off(see section 4.3)\n"));
			}

			RFM69_DEBUG(
			    PSTR("RFM69:DUMP:Aborts Listen mode when set together with ListenOn=0 See section 4.3.4 for details (Always reads 0.)\n"));
			if (0x20 & regVal) {
				RFM69_DEBUG(
				    PSTR("RFM69:DUMP:ERROR - ListenAbort should NEVER return 1 this is a write only register\n"));
			}

			RFM69_DEBUG(PSTR("RFM69:DUMP:Transceiver's operating modes\n"));
			capVal = (regVal >> 2) & 0x7;
			if (capVal == 0b000) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Mode : 000 -> Sleep mode (SLEEP)\n"));
			} else if (capVal == 0b001) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Mode : 001 -> Standby mode (STDBY)\n"));
			} else if (capVal == 0b010) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Mode : 010 -> Frequency Synthesizer mode (FS)\n"));
			} else if (capVal == 0b011) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Mode : 011 -> Transmitter mode (TX)\n"));
			} else if (capVal == 0b100) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Mode : 100 -> Receiver Mode (RX)\n"));
			} else {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Mode : %d capVal \n"), capVal);

			}
			break;
		}

		case 0x2: {
			RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x Data Processing mode \n"), regAddr);

			capVal = (regVal >> 5) & 0x3;
			if (capVal == 0b00) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:DataMode : 00 -> Packet mode\n"));
			} else if (capVal == 0b01) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:DataMode : 01 -> reserved\n"));
			} else if (capVal == 0b10) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:DataMode : 10 -> Continuous mode with bit synchronizer\n"));
			} else if (capVal == 0b11) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:DataMode : 11 -> Continuous mode without bit synchronizer\n"));
			}

			RFM69_DEBUG(PSTR("RFM69:DUMP:Modulation scheme\n"));
			capVal = (regVal >> 3) & 0x3;
			if (capVal == 0b00) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Modulation Type : 00 -> FSK\n"));
				modeFSK = 1;
			} else if (capVal == 0b01) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Modulation Type : 01 -> OOK\n"));
			} else if (capVal == 0b10) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Modulation Type : 10 -> reserved\n"));
			} else if (capVal == 0b11) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Modulation Type : 11 -> reserved\n"));
			}

			if (modeFSK) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Data shaping : in FSK\n"));
			} else {
				RFM69_DEBUG(PSTR("RFM69:DUMP:Data shaping : in OOK\n"));
			}

			capVal = regVal & 0x3;
			if (modeFSK) {
				if (capVal == 0b00) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : 00 -> no shaping\n"));
				} else if (capVal == 0b01) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : 01 -> Gaussian filter, BT = 1.0\n"));
				} else if (capVal == 0b10) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : 10 -> Gaussian filter, BT = 0.5\n"));
				} else if (capVal == 0b11) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : 11 -> Gaussian filter, BT = 0.3\n"));
				}
			} else {
				if (capVal == 0b00) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : 00 -> no shaping\n"));
				} else if (capVal == 0b01) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : 01 -> filtering with f(cutoff) = BR\n"));
				} else if (capVal == 0b10) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : 10 -> filtering with f(cutoff) = 2*BR\n"));
				} else if (capVal == 0b11) {
					RFM69_DEBUG(PSTR("RFM69:DUMP:ModulationShaping : ERROR - 11 is reserved\n"));
				}
			}

			break;
		}

		case 0x3: {
			bitRate = (regVal << 8);
			break;
		}

		case 0x4: {
			bitRate |= regVal;
			RFM69_DEBUG(
			    PSTR("RFM69:DUMP:REG=0x%02x Bit Rate (Chip Rate when Manchester encoding is enabled)\n"), regAddr);

			uint32_t val = 32UL * 1000UL * 1000UL / bitRate;
			RFM69_DEBUG(PSTR("RFM69:DUMP:BitRate : %lu\n"), val);
			break;
		}

		case 0x5: {
			freqDev = ((regVal & 0x3f) << 8);
			break;
		}

		case 0x6: {
			freqDev |= regVal;

			uint32_t val = 61UL * freqDev;
			RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x Frequency deviation\n"), regAddr);
			RFM69_DEBUG(PSTR("RFM69:DUMP:Fdev : %lu\n"), val);
			break;
		}

		case 0x7: {
			uint32_t tempVal = regVal;
			freqCenter = (tempVal << 16);
			break;
		}

		case 0x8: {
			uint32_t tempVal = regVal;
			freqCenter = freqCenter | (tempVal << 8);
			break;
		}

		case 0x9: {
			freqCenter = freqCenter | regVal;
			uint32_t val = 61UL * freqCenter;
			RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x RF Carrier frequency \n"), regAddr);
			RFM69_DEBUG(PSTR("RFM69:DUMP:FRF : %lu\n"), val);
			break;
		}

		case 0xa: {
			RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x RC calibration control & status\n"), regAddr);
			if (0x40 & regVal) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:RcCalDone : 1 -> RC calibration is over\n"));
			} else {
				RFM69_DEBUG(PSTR("RFM69:DUMP:RcCalDone : 0 -> RC calibration is in progress\n"));
			}

			break;
		}

		case 0xb: {
			RFM69_DEBUG(
			    PSTR("RFM69:DUMP:REG=0x%02x Improved AFC routine for signals with modulation index lower than 2.  Refer to section 3.4.16 for details\n"),
			    regAddr);
			if (0x20 & regVal) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:AfcLowBetaOn : 1 -> Improved AFC routine\n"));
			} else {
				RFM69_DEBUG(PSTR("RFM69:DUMP:AfcLowBetaOn : 0 -> Standard AFC routine\n"));
			}
			break;
		}

		case 0xc: {
			RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x Reserved\n"), regAddr);
			break;
		}

		case 0xd: {
			byte val;
			RFM69_DEBUG(PSTR("RFM69:DUMP:REG=0x%02x Resolution of Listen mode Idle time (calibrated RC osc)\n"),
			            regAddr);
			val = regVal >> 6;
			if (val == 0b00) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolIdle : 00 -> reserved\n"));
			} else if (val == 0b01) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolIdle : 01 -> 64 us\n"));
			} else if (val == 0b10) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolIdle : 10 -> 4.1 ms\n"));
			} else if (val == 0b11) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolIdle : 11 -> 262 ms\n"));
			}

			RFM69_DEBUG(PSTR("RFM69:DUMP:Resolution of Listen mode Rx time (calibrated RC osc)\n"));
			val = (regVal >> 4) & 0x3;
			if (val == 0b00) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolRx : 00 -> reserved\n"));
			} else if (val == 0b01) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolRx : 01 -> 64 us\n"));
			} else if (val == 0b10) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolRx : 10 -> 4.1 ms\n"));
			} else if (val == 0b11) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenResolRx : 11 -> 262 ms\n"));
			}

			RFM69_DEBUG(PSTR("RFM69:DUMP:Criteria for packet acceptance in Listen mode\n"));
			if (0x8 & regVal) {
				RFM69_DEBUG(
				    PSTR("RFM69:DUMP:ListenCriteria : 1 -> signal strength is above RssiThreshold and SyncAddress matched\n"));
			} else {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenCriteria : 0 -> signal strength is above RssiThreshold\n"));
			}


			RFM69_DEBUG(PSTR("RFM69:DUMP:Action taken after acceptance of a packet in Listen mode\n"));
			val = (regVal >> 1) & 0x3;
			if (val == 0b00) {
				RFM69_DEBUG(
				    PSTR("RFM69:DUMP:ListenEnd : 00 -> chip stays in Rx mode. Listen mode stops and must be disabled (see section 4.3)\n"));
			} else if (val == 0b01) {
				RFM69_DEBUG(
				    PSTR("RFM69:DUMP:ListenEnd : 01 -> chip stays in Rx mode until PayloadReady or Timeout interrupt occurs.  It then goes to the mode defined by Mode. Listen mode stops and must be disabled (see section 4.3)\n"));
			} else if (val == 0b10) {
				RFM69_DEBUG(
				    PSTR("RFM69:DUMP:ListenEnd : 10 -> chip stays in Rx mode until PayloadReady or Timeout occurs.  Listen mode then resumes in Idle state.  FIFO content is lost at next Rx wakeup.\n"));
			} else if (val == 0b11) {
				RFM69_DEBUG(PSTR("RFM69:DUMP:ListenEnd : 11 -> Reserved\n"));
			}
			break;
		}

		default: {
		}
		}
#endif
		(void)regVal;
	}
}

#endif
