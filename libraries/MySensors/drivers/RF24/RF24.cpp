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

uint8_t MY_RF24_BASE_ADDR[MY_RF24_ADDR_WIDTH] = { MY_RF24_BASE_RADIO_ID };
uint8_t MY_RF24_NODE_ADDRESS = AUTO;

void csn(bool level) {
	digitalWrite(MY_RF24_CS_PIN, level);		
}

void ce(bool level) {
	digitalWrite(MY_RF24_CE_PIN, level);
}

uint8_t spiMultiByteTransfer(uint8_t cmd, uint8_t* buf, uint8_t len, bool aReadMode) {
	uint8_t* current = buf;
	#if !defined(MY_SOFTSPI)
		_SPI.beginTransaction(SPISettings(MY_RF24_SPI_MAX_SPEED, MY_RF24_SPI_DATA_ORDER, MY_RF24_SPI_DATA_MODE));
	#endif
	csn(LOW);
	// timing
	delayMicroseconds(10);
	uint8_t status = _SPI.transfer( cmd );
	while ( len-- ) {
		if (aReadMode) {		
			status = _SPI.transfer( NOP );
			if(buf != NULL) *current++ = status;
		} else status = _SPI.transfer(*current++);
	}
	csn(HIGH);
	#if !defined(MY_SOFTSPI)
		_SPI.endTransaction();
	#endif
	// timing
	delayMicroseconds(10);
	return status;
} 

uint8_t spiByteTransfer(uint8_t cmd) {
	return spiMultiByteTransfer( cmd, NULL, 0, false);
}

uint8_t _readByteRegister(uint8_t cmd) {
	uint8_t value = spiMultiByteTransfer( cmd, NULL, 1, true);
	RF24_DEBUG(PSTR("read register, reg=%d, value=%d\n"), cmd & (R_REGISTER ^ 0xFF), value);
	return value;
}

uint8_t _writeByteRegister(uint8_t cmd, uint8_t value) {
	RF24_DEBUG(PSTR("write register, reg=%d, value=%d\n"), cmd & (W_REGISTER ^ 0xFF), value);
	return spiMultiByteTransfer( cmd , &value, 1, false);
}

#define readByteRegister(reg) _readByteRegister( R_REGISTER | ( REGISTER_MASK & (reg) ) )
#define writeByteRegister(reg, value) _writeByteRegister( W_REGISTER | ( REGISTER_MASK & (reg) ), value )
#define writeMultiByteRegister(reg, buf, len) spiMultiByteTransfer( W_REGISTER | ( REGISTER_MASK & (reg) ), (uint8_t*)buf, len, false )

void flushRX(void) {
	RF24_DEBUG(PSTR("flushRX\n"));
	spiByteTransfer( FLUSH_RX );
}

void flushTX(void) {
	RF24_DEBUG(PSTR("flushTX\n"));
	spiByteTransfer( FLUSH_TX );
}

uint8_t getStatus(void) {
	return spiByteTransfer( NOP );
}

void openWritingPipe(uint8_t recipient) {	
	RF24_DEBUG(PSTR("open writing pipe, recipient=%d\n"), recipient);	
	// only write LSB of RX0 and TX pipe
	writeByteRegister(RX_ADDR_P0, recipient);
	writeByteRegister(TX_ADDR, recipient);		
}

void startListening(void) {
	RF24_DEBUG(PSTR("start listening\n"));
	// toggle PRX		
	writeByteRegister(NRF_CONFIG, MY_RF24_CONFIGURATION | _BV(PWR_UP) | _BV(PRIM_RX) );	
	// all RX pipe addresses must be unique, therefore skip if node ID is 0xFF
	if(MY_RF24_NODE_ADDRESS!=AUTO) writeByteRegister(RX_ADDR_P0, MY_RF24_NODE_ADDRESS);
	// start listening
	ce(HIGH);
}

void stopListening(void) {
	RF24_DEBUG(PSTR("stop listening\n"));
	ce(LOW);
	// timing
	delayMicroseconds(130);
	writeByteRegister(NRF_CONFIG, MY_RF24_CONFIGURATION | _BV(PWR_UP) );
	// timing
	delayMicroseconds(100);
}

void powerDown(void) {
	ce(LOW);
	writeByteRegister(NRF_CONFIG, 0x00);
	RF24_DEBUG(PSTR("power down\n"));
}

bool sendMessage( uint8_t recipient, const void* buf, uint8_t len ) {
	uint8_t status;

	stopListening();
	openWritingPipe( recipient );		
	RF24_DEBUG(PSTR("send message to %d, len=%d\n"),recipient,len);
	// flush TX FIFO
	flushTX();
	// this command is affected in clones (e.g. Si24R1):  flipped NoACK bit when using W_TX_PAYLOAD_NO_ACK / W_TX_PAYLOAD
	// spiMultiByteTransfer( recipient==BROADCAST_ADDRESS ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD, (uint8_t*)buf, len, false );
	// we are safe by disabling AutoACK on the broadcasting pipe
	spiMultiByteTransfer( W_TX_PAYLOAD, (uint8_t*)buf, len, false );
	// go
	ce(HIGH);
	// TX starts after ~10us
	do {
		status = getStatus();
	} while  (!(status & ( _BV(MAX_RT) | _BV(TX_DS) )));
		
	ce(LOW);
	// reset interrupts
	writeByteRegister(RF24_STATUS, _BV(TX_DS) | _BV(MAX_RT) );
	// Max retries exceeded
	if( status & _BV(MAX_RT)){
		// flush packet
		RF24_DEBUG(PSTR("MAX_RT\n"));
		flushTX();
	}
		
	startListening();
	
	return (status & _BV(TX_DS));
}

uint8_t getDynamicPayloadSize(void) {
	uint8_t result = spiMultiByteTransfer(R_RX_PL_WID,NULL,1,true);
	// check if payload size invalid
	if(result > 32) { 
		RF24_DEBUG(PSTR("invalid payload length = %d\n"),result);
		flushRX(); 
		result = 0; 
	}
	return result;
}


bool IsDataAvailable(uint8_t* to) {
	uint8_t pipe_num = ( getStatus() >> RX_P_NO ) & 0b0111;
	#if defined(MY_DEBUG_VERBOSE_RF24)
		if(pipe_num <= 5)
			RF24_DEBUG(PSTR("Data available on pipe %d\n"),pipe_num);	
	#endif	
	if (pipe_num == NODE_PIPE)
		*to = MY_RF24_NODE_ADDRESS;
	else if (pipe_num == BROADCAST_PIPE)
		*to = BROADCAST_ADDRESS;
	return (pipe_num <= 5);
}


uint8_t readMessage( void* buf) {
	uint8_t len = getDynamicPayloadSize();
	RF24_DEBUG(PSTR("read message, len=%d\n"), len);
	spiMultiByteTransfer( R_RX_PAYLOAD , (uint8_t*)buf, len, true ); 
	// clear RX interrupt
	writeByteRegister(RF24_STATUS, _BV(RX_DR) );
	return len;
}

void setNodeAddress(uint8_t address) {
	MY_RF24_NODE_ADDRESS = address;
	// enable node pipe
	writeByteRegister(EN_RXADDR, _BV(ERX_P0 + BROADCAST_PIPE) | _BV(ERX_P0) );
	// enable autoACK on pipe 0
	writeByteRegister(EN_AA, _BV(ENAA_P0) );
}

uint8_t getNodeID(void) {
	return MY_RF24_NODE_ADDRESS;
}

bool initializeRF24(void) {
	// Initialize pins
	pinMode(MY_RF24_CE_PIN,OUTPUT);
	pinMode(MY_RF24_CS_PIN,OUTPUT);
	// Initialize SPI
	_SPI.begin();
	ce(LOW);
	csn(HIGH);
	// CRC and power up
	writeByteRegister(NRF_CONFIG, MY_RF24_CONFIGURATION | _BV(PWR_UP) ) ;
	// settle >2ms
	delay(5);
	// set address width
	writeByteRegister(SETUP_AW, MY_RF24_ADDR_WIDTH - 2 );
	// auto retransmit delay 1500us, auto retransmit count 15
	writeByteRegister(SETUP_RETR, RF24_ARD << ARD | RF24_ARC << ARC);
	// set channel
	writeByteRegister(RF_CH, MY_RF24_CHANNEL);
	// set data rate and pa level
	writeByteRegister(RF_SETUP, MY_RF24_RF_SETUP);
	// sanity check
	#if defined(MY_RF24_SANITY_CHECK)
		if(readByteRegister(RF_SETUP)!=MY_RF24_RF_SETUP) {
			RF24_DEBUG(PSTR("Sanity check failed: RF_SETUP register=%d instead of %d, check wiring, replace module or non-P version\n"),readByteRegister(RF_SETUP), MY_RF24_RF_SETUP);
			return false;
		}
	#endif
	// toggle features (necessary on some clones)
	writeByteRegister(ACTIVATE,0x73);
	// enable ACK payload and dynamic payload
	writeByteRegister(FEATURE, MY_RF24_FEATURE );
    // enable broadcasting pipe
	writeByteRegister(EN_RXADDR, _BV(ERX_P0 + BROADCAST_PIPE) );
	// disable AA on all pipes, activate when node pipe set
	writeByteRegister(EN_AA, 0x00 );
	// enable dynamic payloads on used pipes
	writeByteRegister(DYNPD, _BV(DPL_P0 + BROADCAST_PIPE) | _BV(DPL_P0));
	// listen to broadcast pipe
	MY_RF24_BASE_ADDR[0] = BROADCAST_ADDRESS;
	writeMultiByteRegister(RX_ADDR_P0 + BROADCAST_PIPE, (uint8_t*)&MY_RF24_BASE_ADDR, BROADCAST_PIPE > 1 ? 1 : MY_RF24_ADDR_WIDTH);
	// pipe 0, set full address, later only LSB is updated
	writeMultiByteRegister(RX_ADDR_P0, (uint8_t*)&MY_RF24_BASE_ADDR, MY_RF24_ADDR_WIDTH);
	writeMultiByteRegister(TX_ADDR, (uint8_t*)&MY_RF24_BASE_ADDR, MY_RF24_ADDR_WIDTH);
	// reset FIFO
	flushRX();
	flushTX();
	// reset interrupts
	writeByteRegister(RF24_STATUS, _BV(TX_DS) | _BV(MAX_RT) | _BV(RX_DR));
	return true;
}

