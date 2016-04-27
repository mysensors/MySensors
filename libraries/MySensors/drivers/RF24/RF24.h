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

#include <Arduino.h>
#include <stddef.h>
#if defined SOFTSPI
	#include <DigitalIO.h>
#endif

// SPI settings
#if !defined(MY_RF24_SPI_MAX_SPEED)
	// default 2Mhz - safe for nRF24L01+ clones
	#define MY_RF24_SPI_MAX_SPEED 2000000	
#endif
#define MY_RF24_SPI_DATA_ORDER MSBFIRST
#define MY_RF24_SPI_DATA_MODE SPI_MODE0

// settings
#define MY_RF24_CONFIGURATION (uint8_t) (RF24_CRC_16 << 1)
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

#if defined (ARDUINO) && !defined (__arm__) && !defined (_SPI)
	#if defined(SPI_UART)
		#include <SPI_UART.h>
		#define _SPI uspi
	#elif defined(MY_SOFTSPI)
		#include "drivers/AVR/DigitalIO/DigitalIO.h"
		// change these pins to your liking
		const uint8_t SOFT_SPI_MISO_PIN = MY_SOFT_SPI_MOSI_PIN;
		const uint8_t SOFT_SPI_MOSI_PIN = MY_SOFT_SPI_MISO_PIN;
		const uint8_t SOFT_SPI_SCK_PIN = MY_SOFT_SPI_SCK_PIN;
		const uint8_t SPI_MODE = 0;
		#define _SPI spi
	#elif defined(ARDUINO_ARCH_SAMD)
		#include <SPI.h>
		#define _SPI SPI1
	#else	    
		#include <SPI.h>
		#define _SPI SPI
	#endif
#else
	#include <stdint.h>
	#include <stdio.h>
	#include <string.h>
	#if defined(__arm__) || defined (CORE_TEENSY)
		#include <SPI.h>
	#endif

	#if !defined(CORE_TEENSY)
		#define _BV(x) (1<<(x))
		#if !defined(__arm__)
			extern HardwareSPI SPI;
		#endif
	#else
		#define printf MY_SERIALDEVICE.printf
	#endif
	#if !defined(_SPI)
		#define _SPI SPI
	#endif
#endif

#if defined(MY_SOFTSPI)
	SoftSPI<MY_SOFT_SPI_MISO_PIN, MY_SOFT_SPI_MOSI_PIN, MY_SOFT_SPI_SCK_PIN, SPI_MODE> spi;
#elif defined (SPI_UART)
	SPIUARTClass uspi;
#endif

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

#endif // __RF24_H__


