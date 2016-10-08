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

#ifndef __RF24_H__
#define __RF24_H__

#define LOCAL static

// SPI settings
#if !defined(MY_RF24_SPI_MAX_SPEED)
	// default 2Mhz - safe for nRF24L01+ clones
	#define MY_RF24_SPI_MAX_SPEED 2000000	
#endif
#define MY_RF24_SPI_DATA_ORDER MSBFIRST
#define MY_RF24_SPI_DATA_MODE SPI_MODE0

#if defined (ARDUINO) && !defined (__arm__) && !defined (_SPI)
	#include <SPI.h>
	#if defined(MY_SOFTSPI)
		SoftSPI<MY_SOFT_SPI_MISO_PIN, MY_SOFT_SPI_MOSI_PIN, MY_SOFT_SPI_SCK_PIN, MY_RF24_SPI_DATA_MODE> _SPI;
	#else	    
		#define _SPI SPI
	#endif
#else
	#include <stdint.h>
	#include <stdio.h>
	#include <string.h>
	
	#define _BV(x) (1<<(x))
	
	#if defined(__arm__)
		#include <SPI.h>
	#else
		extern HardwareSPI SPI;
	#endif
	
	#if !defined(_SPI)
		#define _SPI SPI
	#endif
#endif

// verify RF24 IRQ defs
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	#if !defined(MY_RF24_IRQ_PIN)
		#error Message buffering feature requires MY_RF24_IRQ_PIN to be defined!
	#endif
	// SoftSPI does not support usingInterrupt()
	#ifdef MY_SOFTSPI
		#error RF24 IRQ usage cannot be used with Soft SPI
	#endif
	// ESP8266 does not support usingInterrupt()
	#ifdef ARDUINO_ARCH_ESP8266
		#error RF24 IRQ usage cannot be used with ESP8266
	#endif
	#ifndef SPI_HAS_TRANSACTION
		#error RF24 IRQ usage requires transactional SPI support
	#endif
#else
	#ifdef MY_RX_MESSAGE_BUFFER_SIZE
		#error Receive message buffering requires RF24 IRQ usage
	#endif
#endif


// RF24 settings
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	#define MY_RF24_CONFIGURATION (uint8_t) ((RF24_CRC_16 << 2) | (1 << MASK_TX_DS) | (1 << MASK_MAX_RT))
#else
	#define MY_RF24_CONFIGURATION (uint8_t) (RF24_CRC_16 << 2)
#endif
#define MY_RF24_FEATURE (uint8_t)( _BV(EN_DPL) | _BV(EN_ACK_PAY) )
#define MY_RF24_RF_SETUP (uint8_t)( ((MY_RF24_DATARATE & 0b10 ) << 4) | ((MY_RF24_DATARATE & 0b01 ) << 3) | (MY_RF24_PA_LEVEL << 1) ) + 1 // +1 for Si24R1

// pipes
#define BROADCAST_PIPE 1
#define NODE_PIPE 0

// debug 
#if defined(MY_DEBUG_VERBOSE_RF24)
	#define RF24_DEBUG(x,...) debug(x, ##__VA_ARGS__)
#else
	#define RF24_DEBUG(x,...)
#endif

// PA levels
#define RF24_PA_MIN 	0 
#define RF24_PA_LOW 	1 
#define RF24_PA_HIGH 	2 
#define RF24_PA_MAX 	3

// data rate
#define RF24_1MBPS 		0
#define RF24_2MBPS 		1
#define RF24_250KBPS 	2

// CRC
#define RF24_CRC_DISABLED 	0 
#define RF24_CRC_8 			2 
#define RF24_CRC_16 		3

// ARD, auto retry delay
#define RF24_ARD 5 //=1500us

// ARD, auto retry count
#define RF24_ARC 15

// nRF24L01(+) register definitions
#define NRF_CONFIG  0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define RF24_STATUS 0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD	    0x1C
#define FEATURE	    0x1D

// instructions
#define R_REGISTER    		0x00
#define RPD         		0x09
#define W_REGISTER    		0x20
#define REGISTER_MASK 		0x1F
#define ACTIVATE      		0x50
#define R_RX_PL_WID   		0x60
#define R_RX_PAYLOAD  		0x61
#define W_TX_PAYLOAD  		0xA0
#define W_ACK_PAYLOAD 		0xA8
#define W_TX_PAYLOAD_NO_ACK 0xB0
#define FLUSH_TX      		0xE1
#define FLUSH_RX      		0xE2
#define REUSE_TX_PL   		0xE3
#define NOP           		0xFF

// bit mnemonics
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0

// auto ACK
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0

// rx pipe
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0

// dynamic payload
#define DPL_P5	    5
#define DPL_P4	    4
#define DPL_P3	    3
#define DPL_P2	    2
#define DPL_P1	    1
#define DPL_P0	    0

#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define RF_DR       3
#define RF_PWR      6
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4
#define RX_FULL     1
#define RX_EMPTY    0

// features
#define EN_DPL	    2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0

#define LNA_HCURR   0
#define RF_DR_LOW   5
#define RF_DR_HIGH  3
#define RF_PWR_LOW  1
#define RF_PWR_HIGH 2

// functions

LOCAL void RF24_csn(bool level); 
LOCAL void RF24_ce(bool level); 
LOCAL uint8_t RF24_spiMultiByteTransfer(uint8_t cmd, uint8_t* buf, uint8_t len, bool aReadMode);
LOCAL uint8_t RF24_spiByteTransfer(uint8_t cmd);
LOCAL uint8_t RF24_RAW_readByteRegister(uint8_t cmd);
LOCAL uint8_t RF24_RAW_writeByteRegister(uint8_t cmd, uint8_t value);

#define RF24_readByteRegister(reg) RF24_RAW_readByteRegister( R_REGISTER | ( REGISTER_MASK & (reg) ) )
#define RF24_writeByteRegister(reg, value) RF24_RAW_writeByteRegister( W_REGISTER | ( REGISTER_MASK & (reg) ), value )
#define RF24_writeMultiByteRegister(reg, buf, len) RF24_spiMultiByteTransfer( W_REGISTER | ( REGISTER_MASK & (reg) ), (uint8_t*)buf, len, false )

LOCAL void RF24_flushRX(void);
LOCAL void RF24_flushTX(void);
LOCAL uint8_t RF24_getStatus(void);
LOCAL uint8_t RF24_getFIFOStatus(void);
LOCAL void RF24_openWritingPipe(uint8_t recipient);
LOCAL void RF24_startListening(void);
LOCAL void RF24_stopListening(void);
LOCAL void RF24_powerDown(void); 
LOCAL bool RF24_sendMessage(uint8_t recipient, const void* buf, uint8_t len);
LOCAL uint8_t RF24_getDynamicPayloadSize(void);
LOCAL bool RF24_isDataAvailable();
LOCAL uint8_t RF24_readMessage(void* buf); 
LOCAL void RF24_setNodeAddress(uint8_t address);
LOCAL uint8_t RF24_getNodeID(void);
LOCAL bool RF24_sanityCheck(void);
LOCAL bool RF24_initialize(void);
LOCAL void RF24_setChannel(uint8_t channel);
LOCAL void RF24_setRetries(uint8_t retransmitDelay, uint8_t retransmitCount);
LOCAL void RF24_setAddressWidth(uint8_t width);
LOCAL void RF24_setRFSetup(uint8_t RFsetup);
LOCAL void RF24_setFeature(uint8_t feature);
LOCAL void RF24_setPipe(uint8_t pipe);
LOCAL void RF24_setAutoACK(uint8_t pipe);
LOCAL void RF24_setDynamicPayload(uint8_t pipe);
LOCAL void RF24_setRFConfiguration(uint8_t configuration);
LOCAL void RF24_setPipeAddress(uint8_t pipe, uint8_t* address, uint8_t width);
LOCAL void RF24_setPipeLSB(uint8_t pipe, uint8_t LSB);
LOCAL uint8_t RF24_getObserveTX(void);
LOCAL void RF24_setStatus(uint8_t status);
LOCAL void RF24_enableFeatures(void);

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	typedef void (*RF24_receiveCallbackType)(void);
  /**
	 * Register a callback, which will be called (from interrupt context) for every message received.
	 * @note When a callback is registered, it _must_ retrieve the message from the nRF24
	 * by calling RF24_readMessage(). Otherwise the interrupt will not get deasserted
	 * and message reception will stop.
	 */
	LOCAL void RF24_registerReceiveCallback( RF24_receiveCallbackType cb );
#endif

#endif // __RF24_H__