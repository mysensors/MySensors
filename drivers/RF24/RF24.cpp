/*
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2015 Sensnology AB
* Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
* Based on maniacbug's RF24 library, copyright (C) 2011 J. Coliz <maniacbug@ymail.com>
* RF24 driver refactored and optimized for speed and size, copyright (C) 2016 Olivier Mauti <olivier@mysensors.org>
*/

#include "RF24.h"

LOCAL uint8_t MY_RF24_BASE_ADDR[MY_RF24_ADDR_WIDTH] = { MY_RF24_BASE_RADIO_ID };
LOCAL uint8_t MY_RF24_NODE_ADDRESS = AUTO;

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
LOCAL RF24_receiveCallbackType RF24_receiveCallback = NULL;
#endif

#ifdef LINUX_SPI_BCM
uint8_t spi_rxbuff[32+1] ; //SPI receive buffer (payload max 32 bytes)
uint8_t spi_txbuff[32+1] ; //SPI transmit buffer (payload max 32 bytes + 1 byte for the command)
#endif

LOCAL void RF24_csn(const bool level)
{
	hwDigitalWrite(MY_RF24_CS_PIN, level);
}

LOCAL void RF24_ce(const bool level)
{
	hwDigitalWrite(MY_RF24_CE_PIN, level);
}

LOCAL uint8_t RF24_spiMultiByteTransfer(const uint8_t cmd, uint8_t* buf, uint8_t len,
                                        const bool aReadMode)
{
	uint8_t status;
	uint8_t* current = buf;
#if !defined(MY_SOFTSPI)
	_SPI.beginTransaction(SPISettings(MY_RF24_SPI_MAX_SPEED, MY_RF24_SPI_DATA_ORDER,
	                                  MY_RF24_SPI_DATA_MODE));
#endif
	RF24_csn(LOW);
	// timing
	delayMicroseconds(10);
#ifdef LINUX_SPI_BCM
	uint8_t * prx = spi_rxbuff;
	uint8_t * ptx = spi_txbuff;
	uint8_t size = len + 1; // Add register value to transmit buffer

	*ptx++ = cmd;
	while ( len-- ) {
		if (aReadMode) {
			*ptx++ = RF24_NOP;
		} else {
			*ptx++ = *current++;
		}
	}
	_SPI.transfernb( (char *) spi_txbuff, (char *) spi_rxbuff, size);
	if (aReadMode) {
		if (size == 2) {
			status = *++prx;   // result is 2nd byte of receive buffer
		} else {
			status = *prx++; // status is 1st byte of receive buffer
			// decrement before to skip status byte
			while (--size) {
				*buf++ = *prx++;
			}
		}
	} else {
		status = *prx; // status is 1st byte of receive buffer
	}
#else
	status = _SPI.transfer(cmd);
	while ( len-- ) {
		if (aReadMode) {
			status = _SPI.transfer(RF24_NOP);
			if (buf != NULL) {
				*current++ = status;
			}
		} else {
			status = _SPI.transfer(*current++);
		}
	}
#endif
	RF24_csn(HIGH);
#if !defined(MY_SOFTSPI)
	_SPI.endTransaction();
#endif
	// timing
	delayMicroseconds(10);
	return status;
}

LOCAL uint8_t RF24_spiByteTransfer(const uint8_t cmd)
{
	return RF24_spiMultiByteTransfer(cmd, NULL, 0, false);
}

LOCAL uint8_t RF24_RAW_readByteRegister(const uint8_t cmd)
{
	const uint8_t value = RF24_spiMultiByteTransfer(cmd, NULL, 1, true);
	RF24_DEBUG(PSTR("RF24:read register, reg=%d, value=%d\n"), cmd & RF24_REGISTER_MASK, value);
	return value;
}

LOCAL uint8_t RF24_RAW_writeByteRegister(const uint8_t cmd, uint8_t value)
{
	RF24_DEBUG(PSTR("RF24:write register, reg=%d, value=%d\n"), cmd & RF24_REGISTER_MASK, value);
	return RF24_spiMultiByteTransfer( cmd , &value, 1, false);
}

LOCAL void RF24_flushRX(void)
{
	RF24_DEBUG(PSTR("RF24:flushRX\n"));
	RF24_spiByteTransfer(RF24_FLUSH_RX);
}

LOCAL void RF24_flushTX(void)
{
	RF24_DEBUG(PSTR("RF24:flushTX\n"));
	RF24_spiByteTransfer(RF24_FLUSH_TX);
}

LOCAL uint8_t RF24_getStatus(void)
{
	return RF24_spiByteTransfer(RF24_NOP);
}

LOCAL uint8_t RF24_getFIFOStatus(void)
{
	return RF24_readByteRegister(RF24_FIFO_STATUS);
}

LOCAL void RF24_setChannel(const uint8_t channel)
{
	RF24_writeByteRegister(RF24_RF_CH,channel);
}

LOCAL void RF24_setRetries(const uint8_t retransmitDelay, const uint8_t retransmitCount)
{
	RF24_writeByteRegister(RF24_SETUP_RETR, retransmitDelay << RF24_ARD | retransmitCount << RF24_ARC);
}

LOCAL void RF24_setAddressWidth(const uint8_t width)
{
	RF24_writeByteRegister(RF24_SETUP_AW, width - 2);
}

LOCAL void RF24_setRFSetup(const uint8_t RFsetup)
{
	RF24_writeByteRegister(RF24_RF_SETUP, RFsetup);
}

LOCAL void RF24_setFeature(const uint8_t feature)
{
	RF24_writeByteRegister(RF24_FEATURE, feature);
}

LOCAL void RF24_setPipe(const uint8_t pipe)
{
	RF24_writeByteRegister(RF24_EN_RXADDR, pipe);
}

LOCAL void RF24_setAutoACK(const uint8_t pipe)
{
	RF24_writeByteRegister(RF24_EN_AA, pipe);
}

LOCAL void RF24_setDynamicPayload(const uint8_t pipe)
{
	RF24_writeByteRegister(RF24_DYNPD, pipe);
}

LOCAL void RF24_setRFConfiguration(const uint8_t configuration)
{
	RF24_writeByteRegister(RF24_NRF_CONFIG, configuration);
}

LOCAL void RF24_setPipeAddress(const uint8_t pipe, uint8_t* address, const uint8_t width)
{
	RF24_writeMultiByteRegister(pipe, address, width);
}

LOCAL void RF24_setPipeLSB(const uint8_t pipe, const uint8_t LSB)
{
	RF24_writeByteRegister(pipe, LSB);
}

LOCAL uint8_t RF24_getObserveTX(void)
{
	return RF24_readByteRegister(RF24_OBSERVE_TX);
}

LOCAL void RF24_setStatus(const uint8_t status)
{
	RF24_writeByteRegister(RF24_STATUS, status);
}
LOCAL void RF24_enableFeatures(void)
{
	RF24_RAW_writeByteRegister(RF24_ACTIVATE, 0x73);
}

LOCAL void RF24_openWritingPipe(const uint8_t recipient)
{
	RF24_DEBUG(PSTR("RF24:OPEN WPIPE,RCPT=%d\n"), recipient); // open writing pipe
	// only write LSB of RX0 and TX pipe
	RF24_setPipeLSB(RF24_RX_ADDR_P0, recipient);
	RF24_setPipeLSB(RF24_TX_ADDR, recipient);
}

LOCAL void RF24_startListening(void)
{
	RF24_DEBUG(PSTR("RF24:STRT LIS\n"));	// start listening
	// toggle PRX
	RF24_setRFConfiguration(MY_RF24_CONFIGURATION | _BV(RF24_PWR_UP) | _BV(RF24_PRIM_RX) );
	// all RX pipe addresses must be unique, therefore skip if node ID is AUTO
	if(MY_RF24_NODE_ADDRESS!=AUTO) {
		RF24_setPipeLSB(RF24_RX_ADDR_P0, MY_RF24_NODE_ADDRESS);
	}
	// start listening
	RF24_ce(HIGH);
}

LOCAL void RF24_stopListening(void)
{
	RF24_DEBUG(PSTR("RF24:STP LIS\n"));	// stop listening
	RF24_ce(LOW);
	// timing
	delayMicroseconds(130);
	RF24_setRFConfiguration(MY_RF24_CONFIGURATION | _BV(RF24_PWR_UP) );
	// timing
	delayMicroseconds(100);
}

LOCAL void RF24_powerDown(void)
{
	RF24_ce(LOW);
	RF24_setRFConfiguration(MY_RF24_CONFIGURATION);
	RF24_DEBUG(PSTR("RF24:PD\n")); // power down
}

LOCAL bool RF24_sendMessage(const uint8_t recipient, const void* buf, const uint8_t len)
{
	uint8_t RF24_status;
	RF24_stopListening();
	RF24_openWritingPipe( recipient );
	RF24_DEBUG(PSTR("RF24:SND:TO=%d,LEN=%d\n"),recipient,len); // send message
	// flush TX FIFO
	RF24_flushTX();
	// this command is affected in clones (e.g. Si24R1):  flipped NoACK bit when using W_TX_PAYLOAD_NO_ACK / W_TX_PAYLOAD
	// AutoACK is disabled on the broadcasting pipe - NO_ACK prevents resending
	RF24_spiMultiByteTransfer(recipient == BROADCAST_ADDRESS ? RF24_WRITE_TX_PAYLOAD_NO_ACK :
	                          RF24_WRITE_TX_PAYLOAD, (uint8_t*)buf, len, false );
	// go, TX starts after ~10us
	RF24_ce(HIGH);
	// timeout counter to detect HW issues
	uint16_t timeout = 0xFFFF;
	do {
		RF24_status = RF24_getStatus();
	} while  (!(RF24_status & ( _BV(RF24_MAX_RT) | _BV(RF24_TX_DS) )) && timeout--);
	// timeout value after successful TX on 16Mhz AVR ~ 65500, i.e. msg is transmitted after ~36 loop cycles
	RF24_ce(LOW);
	// reset interrupts
	RF24_setStatus(_BV(RF24_TX_DS) | _BV(RF24_MAX_RT) );
	// Max retries exceeded
	if(RF24_status & _BV(RF24_MAX_RT)) {
		// flush packet
		RF24_DEBUG(PSTR("!RF24:SND:MAX_RT\n"));	// max retries, no ACK
		RF24_flushTX();
	}
	RF24_startListening();
	// true if message sent
	return (RF24_status & _BV(RF24_TX_DS));
}

LOCAL uint8_t RF24_getDynamicPayloadSize(void)
{
	uint8_t result = RF24_spiMultiByteTransfer(RF24_READ_RX_PL_WID, NULL, 1, true);
	// check if payload size invalid
	if(result > 32) {
		RF24_DEBUG(PSTR("!RF24:GDP:PAYL LEN INVALID=%d\n"),result); // payload len invalid
		RF24_flushRX();
		result = 0;
	}
	return result;
}

LOCAL bool RF24_isDataAvailable(void)
{
	return (!(RF24_getFIFOStatus() & _BV(0)) );
}


LOCAL uint8_t RF24_readMessage(void* buf)
{
	const uint8_t len = RF24_getDynamicPayloadSize();
	RF24_DEBUG(PSTR("RF24:RDM:MSG LEN=%d\n"), len);	// read message
	RF24_spiMultiByteTransfer(RF24_READ_RX_PAYLOAD,(uint8_t*)buf,len,true);
	// clear RX interrupt
	RF24_setStatus(_BV(RF24_RX_DR));
	return len;
}

LOCAL void RF24_setNodeAddress(const uint8_t address)
{
	if(address!=AUTO) {
		MY_RF24_NODE_ADDRESS = address;
		// enable node pipe
		RF24_setPipe(_BV(RF24_ERX_P0 + RF24_BROADCAST_PIPE) | _BV(RF24_ERX_P0));
		// enable autoACK on pipe 0
		RF24_setAutoACK(_BV(RF24_ENAA_P0));
	}
}

LOCAL uint8_t RF24_getNodeID(void)
{
	return MY_RF24_NODE_ADDRESS;
}

LOCAL bool RF24_sanityCheck(void)
{
	// detect HW defect, configuration errors or interrupted SPI line, CE disconnect cannot be detected
	return (RF24_readByteRegister(RF24_RF_SETUP) == MY_RF24_RF_SETUP) & (RF24_readByteRegister(
	            RF24_RF_CH) == MY_RF24_CHANNEL);
}

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
LOCAL void RF24_irqHandler(void)
{
	if (RF24_receiveCallback) {
		// Will stay for a while (several 100us) in this interrupt handler. Any interrupts from serial
		// rx coming in during our stay will not be handled and will cause characters to be lost.
		// As a workaround we re-enable interrupts to allow nested processing of other interrupts.
		// Our own handler is disconnected to prevent recursive calling of this handler.
#if defined(MY_GATEWAY_SERIAL) && !defined(__linux__)
		detachInterrupt(digitalPinToInterrupt(MY_RF24_IRQ_PIN));
		interrupts();
#endif
		// Read FIFO until empty.
		// Procedure acc. to datasheet (pg. 63):
		// 1.Read payload, 2.Clear RX_DR IRQ, 3.Read FIFO_status, 4.Repeat when more data available.
		// Datasheet (ch. 8.5) states, that the nRF de-asserts IRQ after reading STATUS.

		// Start checking if RX-FIFO is not empty, as we might end up here from an interrupt
		// for a message we've already read.
		while (RF24_isDataAvailable()) {
			RF24_receiveCallback();		// Must call RF24_readMessage(), which will clear RX_DR IRQ !
		}
		// Restore our interrupt handler.
#if defined(MY_GATEWAY_SERIAL) && !defined(__linux__)
		noInterrupts();
		attachInterrupt(digitalPinToInterrupt(MY_RF24_IRQ_PIN), RF24_irqHandler, FALLING);
#endif
	} else {
		// clear RX interrupt
		RF24_setStatus(_BV(RF24_RX_DR));
	}
}

LOCAL void RF24_registerReceiveCallback(RF24_receiveCallbackType cb)
{
	MY_CRITICAL_SECTION {
		RF24_receiveCallback = cb;
	}
}
#endif

LOCAL bool RF24_initialize(void)
{
	// prevent warning
	(void)RF24_getObserveTX;

	// Initialize pins
	hwPinMode(MY_RF24_CE_PIN,OUTPUT);
	hwPinMode(MY_RF24_CS_PIN,OUTPUT);
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	hwPinMode(MY_RF24_IRQ_PIN,INPUT);
#endif
	// Initialize SPI
	_SPI.begin();
	RF24_ce(LOW);
	RF24_csn(HIGH);
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	// assure SPI can be used from interrupt context
	// Note: ESP8266 & SoftSPI currently do not support interrupt usage for SPI,
	// therefore it is unsafe to use MY_RF24_IRQ_PIN with ESP8266/SoftSPI!
	_SPI.usingInterrupt(digitalPinToInterrupt(MY_RF24_IRQ_PIN));
	// attach interrupt
	attachInterrupt(digitalPinToInterrupt(MY_RF24_IRQ_PIN), RF24_irqHandler, FALLING);
#endif
	// CRC and power up
	RF24_setRFConfiguration(MY_RF24_CONFIGURATION | _BV(RF24_PWR_UP)) ;
	// settle >2ms
	delay(5);
	// set address width
	RF24_setAddressWidth(MY_RF24_ADDR_WIDTH);
	// auto retransmit delay 1500us, auto retransmit count 15
	RF24_setRetries(RF24_SET_ARD, RF24_SET_ARC);
	// set channel
	RF24_setChannel(MY_RF24_CHANNEL);
	// set data rate and pa level
	RF24_setRFSetup(MY_RF24_RF_SETUP);
	// toggle features (necessary on some clones and non-P versions)
	RF24_enableFeatures();
	// enable ACK payload and dynamic payload
	RF24_setFeature(MY_RF24_FEATURE);
	// sanity check (this function is P/non-P independent)
	if (!RF24_sanityCheck()) {
		RF24_DEBUG(PSTR("!RF24:INIT:SANCHK FAIL\n")); // sanity check failed, check wiring or replace module
		return false;
	}
	// enable broadcasting pipe
	RF24_setPipe(_BV(RF24_ERX_P0 + RF24_BROADCAST_PIPE));
	// disable AA on all pipes, activate when node pipe set
	RF24_setAutoACK(0x00);
	// enable dynamic payloads on used pipes
	RF24_setDynamicPayload(_BV(RF24_DPL_P0 + RF24_BROADCAST_PIPE) | _BV(RF24_DPL_P0));
	// listen to broadcast pipe
	MY_RF24_BASE_ADDR[0] = BROADCAST_ADDRESS;
	RF24_setPipeAddress(RF24_RX_ADDR_P0 + RF24_BROADCAST_PIPE, (uint8_t*)&MY_RF24_BASE_ADDR,
	                    RF24_BROADCAST_PIPE > 1 ? 1 : MY_RF24_ADDR_WIDTH);
	// pipe 0, set full address, later only LSB is updated
	RF24_setPipeAddress(RF24_RX_ADDR_P0, (uint8_t*)&MY_RF24_BASE_ADDR, MY_RF24_ADDR_WIDTH);
	RF24_setPipeAddress(RF24_TX_ADDR, (uint8_t*)&MY_RF24_BASE_ADDR, MY_RF24_ADDR_WIDTH);
	// reset FIFO
	RF24_flushRX();
	RF24_flushTX();
	// reset interrupts
	RF24_setStatus(_BV(RF24_TX_DS) | _BV(RF24_MAX_RT) | _BV(RF24_RX_DR));
	return true;
}
