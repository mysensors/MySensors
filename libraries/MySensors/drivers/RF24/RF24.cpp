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

LOCAL void RF24_csn(bool level) {
	digitalWrite(MY_RF24_CS_PIN, level);		
}

LOCAL void RF24_ce(bool level) {
	digitalWrite(MY_RF24_CE_PIN, level);
}

LOCAL uint8_t RF24_spiMultiByteTransfer(uint8_t cmd, uint8_t* buf, uint8_t len, bool aReadMode) {
	uint8_t* current = buf;
	#if !defined(MY_SOFTSPI)
		_SPI.beginTransaction(SPISettings(MY_RF24_SPI_MAX_SPEED, MY_RF24_SPI_DATA_ORDER, MY_RF24_SPI_DATA_MODE));
	#endif
	RF24_csn(LOW);
	// timing
	delayMicroseconds(10);
	uint8_t status = _SPI.transfer( cmd );
	while ( len-- ) {
		if (aReadMode) {		
			status = _SPI.transfer( NOP );
			if(buf != NULL) *current++ = status;
		} else status = _SPI.transfer(*current++);
	}
	RF24_csn(HIGH);
	#if !defined(MY_SOFTSPI)
		_SPI.endTransaction();
	#endif
	// timing
	delayMicroseconds(10);
	return status;
} 

LOCAL uint8_t RF24_spiByteTransfer(uint8_t cmd) {
	return RF24_spiMultiByteTransfer( cmd, NULL, 0, false);
}

LOCAL uint8_t RF24_RAW_readByteRegister(uint8_t cmd) {
	uint8_t value = RF24_spiMultiByteTransfer( cmd, NULL, 1, true);
	RF24_DEBUG(PSTR("read register, reg=%d, value=%d\n"), cmd & (R_REGISTER ^ 0xFF), value);
	return value;
}

LOCAL uint8_t RF24_RAW_writeByteRegister(uint8_t cmd, uint8_t value) {
	RF24_DEBUG(PSTR("write register, reg=%d, value=%d\n"), cmd & (W_REGISTER ^ 0xFF), value);
	return RF24_spiMultiByteTransfer( cmd , &value, 1, false);
}

LOCAL void RF24_flushRX(void) {
	RF24_DEBUG(PSTR("RF24_flushRX\n"));
	RF24_spiByteTransfer( FLUSH_RX );
}

LOCAL void RF24_flushTX(void) {
	RF24_DEBUG(PSTR("RF24_flushTX\n"));
	RF24_spiByteTransfer( FLUSH_TX );
}

LOCAL uint8_t RF24_getStatus(void) {
	return RF24_spiByteTransfer( NOP );
}

LOCAL void RF24_openWritingPipe(uint8_t recipient) {	
	RF24_DEBUG(PSTR("open writing pipe, recipient=%d\n"), recipient);	
	// only write LSB of RX0 and TX pipe
	RF24_writeByteRegister(RX_ADDR_P0, recipient);
	RF24_writeByteRegister(TX_ADDR, recipient);		
}

LOCAL void RF24_startListening(void) {
	RF24_DEBUG(PSTR("start listening\n"));
	// toggle PRX		
	RF24_writeByteRegister(NRF_CONFIG, MY_RF24_CONFIGURATION | _BV(PWR_UP) | _BV(PRIM_RX) );	
	// all RX pipe addresses must be unique, therefore skip if node ID is 0xFF
	if(MY_RF24_NODE_ADDRESS!=AUTO) RF24_writeByteRegister(RX_ADDR_P0, MY_RF24_NODE_ADDRESS);
	// start listening
	RF24_ce(HIGH);
}

LOCAL void RF24_stopListening(void) {
	RF24_DEBUG(PSTR("stop listening\n"));
	RF24_ce(LOW);
	// timing
	delayMicroseconds(130);
	RF24_writeByteRegister(NRF_CONFIG, MY_RF24_CONFIGURATION | _BV(PWR_UP) );
	// timing
	delayMicroseconds(100);
}

LOCAL void RF24_powerDown(void) {
	RF24_ce(LOW);
	RF24_writeByteRegister(NRF_CONFIG, 0x00);
	RF24_DEBUG(PSTR("power down\n"));
}

LOCAL bool RF24_sendMessage( uint8_t recipient, const void* buf, uint8_t len ) {
	uint8_t status;

	RF24_stopListening();
	RF24_openWritingPipe( recipient );		
	RF24_DEBUG(PSTR("send message to %d, len=%d\n"),recipient,len);
	// flush TX FIFO
	RF24_flushTX();
	// this command is affected in clones (e.g. Si24R1):  flipped NoACK bit when using W_TX_PAYLOAD_NO_ACK / W_TX_PAYLOAD
	// RF24_spiMultiByteTransfer( recipient==BROADCAST_ADDRESS ? W_TX_PAYLOAD_NO_ACK : W_TX_PAYLOAD, (uint8_t*)buf, len, false );
	// we are safe by disabling AutoACK on the broadcasting pipe
	RF24_spiMultiByteTransfer( W_TX_PAYLOAD, (uint8_t*)buf, len, false );
	// go
	RF24_ce(HIGH);
	// TX starts after ~10us
	do {
		status = RF24_getStatus();
	} while  (!(status & ( _BV(MAX_RT) | _BV(TX_DS) )));
		
	RF24_ce(LOW);
	// reset interrupts
	RF24_writeByteRegister(RF24_STATUS, _BV(TX_DS) | _BV(MAX_RT) );
	// Max retries exceeded
	if( status & _BV(MAX_RT)){
		// flush packet
		RF24_DEBUG(PSTR("MAX_RT\n"));
		RF24_flushTX();
	}
		
	RF24_startListening();
	
	return (status & _BV(TX_DS));
}

LOCAL uint8_t RF24_getDynamicPayloadSize(void) {
	uint8_t result = RF24_spiMultiByteTransfer(R_RX_PL_WID,NULL,1,true);
	// check if payload size invalid
	if(result > 32) { 
		RF24_DEBUG(PSTR("invalid payload length = %d\n"),result);
		RF24_flushRX(); 
		result = 0; 
	}
	return result;
}


LOCAL bool RF24_isDataAvailable(uint8_t* to) {
	uint8_t pipe_num = ( RF24_getStatus() >> RX_P_NO ) & 0b0111;
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


LOCAL uint8_t RF24_readMessage( void* buf) {
	uint8_t len = RF24_getDynamicPayloadSize();
	RF24_DEBUG(PSTR("read message, len=%d\n"), len);
	RF24_spiMultiByteTransfer( R_RX_PAYLOAD , (uint8_t*)buf, len, true ); 
	// clear RX interrupt
	RF24_writeByteRegister(RF24_STATUS, _BV(RX_DR) );
	return len;
}

LOCAL void RF24_setNodeAddress(uint8_t address) {
	if(address!=AUTO){
		MY_RF24_NODE_ADDRESS = address;
		// enable node pipe
		RF24_writeByteRegister(EN_RXADDR, _BV(ERX_P0 + BROADCAST_PIPE) | _BV(ERX_P0) );
		// enable autoACK on pipe 0
		RF24_writeByteRegister(EN_AA, _BV(ENAA_P0) );
	}
}

LOCAL uint8_t RF24_getNodeID(void) {
	return MY_RF24_NODE_ADDRESS;
}

LOCAL bool RF24_initialize(void) {
	// Initialize pins
	pinMode(MY_RF24_CE_PIN,OUTPUT);
	pinMode(MY_RF24_CS_PIN,OUTPUT);
	// Initialize SPI
	_SPI.begin();
	RF24_ce(LOW);
	RF24_csn(HIGH);
	// CRC and power up
	RF24_writeByteRegister(NRF_CONFIG, MY_RF24_CONFIGURATION | _BV(PWR_UP) ) ;
	// settle >2ms
	delay(5);
	// set address width
	RF24_writeByteRegister(SETUP_AW, MY_RF24_ADDR_WIDTH - 2 );
	// auto retransmit delay 1500us, auto retransmit count 15
	RF24_writeByteRegister(SETUP_RETR, RF24_ARD << ARD | RF24_ARC << ARC);
	// set channel
	RF24_writeByteRegister(RF_CH, MY_RF24_CHANNEL);
	// set data rate and pa level
	RF24_writeByteRegister(RF_SETUP, MY_RF24_RF_SETUP);
	// sanity check
	#if defined(MY_RF24_SANITY_CHECK)
		if(RF24_readByteRegister(RF_SETUP)!=MY_RF24_RF_SETUP) {
			RF24_DEBUG(PSTR("Sanity check failed: RF_SETUP register=%d instead of %d, check wiring, replace module or non-P version\n"),RF24_readByteRegister(RF_SETUP), MY_RF24_RF_SETUP);
			return false;
		}
	#endif
	// toggle features (necessary on some clones)
	RF24_writeByteRegister(ACTIVATE,0x73);
	// enable ACK payload and dynamic payload
	RF24_writeByteRegister(FEATURE, MY_RF24_FEATURE );
    // enable broadcasting pipe
	RF24_writeByteRegister(EN_RXADDR, _BV(ERX_P0 + BROADCAST_PIPE) );
	// disable AA on all pipes, activate when node pipe set
	RF24_writeByteRegister(EN_AA, 0x00 );
	// enable dynamic payloads on used pipes
	RF24_writeByteRegister(DYNPD, _BV(DPL_P0 + BROADCAST_PIPE) | _BV(DPL_P0));
	// listen to broadcast pipe
	MY_RF24_BASE_ADDR[0] = BROADCAST_ADDRESS;
	RF24_writeMultiByteRegister(RX_ADDR_P0 + BROADCAST_PIPE, (uint8_t*)&MY_RF24_BASE_ADDR, BROADCAST_PIPE > 1 ? 1 : MY_RF24_ADDR_WIDTH);
	// pipe 0, set full address, later only LSB is updated
	RF24_writeMultiByteRegister(RX_ADDR_P0, (uint8_t*)&MY_RF24_BASE_ADDR, MY_RF24_ADDR_WIDTH);
	RF24_writeMultiByteRegister(TX_ADDR, (uint8_t*)&MY_RF24_BASE_ADDR, MY_RF24_ADDR_WIDTH);
	// reset FIFO
	RF24_flushRX();
	RF24_flushTX();
	// reset interrupts
	RF24_writeByteRegister(RF24_STATUS, _BV(TX_DS) | _BV(MAX_RT) | _BV(RX_DR));
	return true;
}

