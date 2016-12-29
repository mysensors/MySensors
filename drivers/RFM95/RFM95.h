/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2016 Sensnology AB
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
 * RFM95 driver refactored and optimized for MySensors, Copyright (C) 2016 Olivier Mauti <olivier@mysensors.org>
 *
 * Changelog:
 * - ACK with sequenceNumber
 * - ATC control
 *
 * Definitions for HopeRF LoRa radios:
 * http://www.hoperf.com/upload/rf/RFM95_96_97_98W.pdf
 * http://www.hoperf.cn/upload/rfchip/RF96_97_98.pdf
 *
 */

/**
* @file RFM95.h
*
* @defgroup RFM95grp RFM95
* @ingroup internals
* @{
*
* RFM95 driver-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
* - [!] Exclamation mark is prepended in case of error
*
* |E| SYS	| SUB	| Message							| Comment
* |-|-------|-------|-----------------------------------|---------------------------------------------------------------------
* | | RFM95 | INIT	|									| Initialise RFM95 radio
* |!| RFM95 | INIT	| SANCHK FAIL						| Sanity check failed, check wiring or replace module
* | | RFM95 | RCV	| SEND ACK							| ACK request received, sending ACK back
* | | RFM95 | PTC	| LEVEL=%d							| Set TX power level
* | | RFM95 | SAC	| SEND ACK TO=%d,RSSI=%d			| Send ACK to node (TO), RSSI of received message (RSSI)
* | | RFM95 | ATC	| ADJ TXL,cR=%d,tR=%d,TXL=%d		| Adjust TX level, current RSSI (cR), target RSSI (tR), TX level (TXL)
* | | RFM95 | SWR	| SEND TO=%d,RETRY=%d				| Send message to (TO), NACK retry counter (RETRY)
* | | RFM95 | SWR	| ACK FROM=%d,SEQ=%d,RSSI=%d,SNR=%d	| ACK received from node (FROM), seq ID (SEQ), (RSSI), (SNR)
* |!| RFM95 | SWR	| NACK								| No ACK received
*
* RFM95 modem configuration
*
* BW = Bandwidth in kHz
* CR = Error correction code
* SF = Spreading factor, chips / symbol
*
* | CONFIG           | REG_1D | REG_1E | REG_26 | BW    | CR  | SF   | Comment
* |------------------|--------|--------|--------|-------|-----|------|-----------------------------
* | BW125CR45SF128   | 0x72   | 0x74   | 0x04   | 125   | 4/5 | 128  | Default, medium range
* | BW500CR45SF128   | 0x92   | 0x74   | 0x04   | 500   | 4/5 | 128  | Fast, short range
* | BW31_25CR48SF512 | 0x48   | 0x94   | 0x04   | 31.25 | 4/8 | 512  | Slow, long range
* | BW125CR48SF4096  | 0x78   | 0xC4   | 0x0C   | 125   | 4/8 | 4096 | Slow, long range
*
* @brief API declaration for RFM95
*
*/

#ifndef _RFM95_h
#define _RFM95_h

#include "RFM95registers.h"

// default PIN assignments, can be overridden
#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega88) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
#define RFM95_IRQ_PIN		(2)		//!< RFM95_IRQ_PIN
#define RFM95_RST_PIN		(9)		//!< RFM95_RST_PIN
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
#define RFM95_IRQ_PIN		(2)
#define RFM95_RST_PIN		(9)		//!< RFM95_RST_PIN
#elif defined(__AVR_ATmega32U4__)
#define RFM95_IRQ_PIN		(3)		//!< RFM95_IRQ_PIN
#define RFM95_RST_PIN		(9)		//!< RFM95_RST_PIN
#elif defined(__arm__)
#define RFM95_IRQ_PIN		(10)	//!< RFM95_IRQ_PIN
#define RFM95_RST_PIN		(27)	//!< RFM95_RST_PIN
#else
#define RFM95_IRQ_PIN		(2)		//!< RFM95_IRQ_PIN
#define RFM95_RST_PIN		(9)		//!< RFM95_RST_PIN
#endif
#define RFM95_SPI_CS		(SS)	//!< RFM95_SPI_CS

// SPI settings
#if !defined(MY_RFM95_SPI_MAX_SPEED)
#define MY_RFM95_SPI_MAX_SPEED		(4*1000000ul)	//!< Max SPI speed, 4Mhz
#endif
#define MY_RFM95_SPI_DATA_ORDER		MSBFIRST		//!< SPI data order
#define MY_RFM95_SPI_DATA_MODE		SPI_MODE0		//!< SPI mode


#if defined (ARDUINO) && !defined (__arm__) && !defined (_SPI)
#include <SPI.h>
#if defined(MY_SOFTSPI)
SoftSPI<MY_SOFT_SPI_MISO_PIN, MY_SOFT_SPI_MOSI_PIN, MY_SOFT_SPI_SCK_PIN, MY_RF24_SPI_DATA_MODE>
_SPI;
#else
#define _SPI SPI					//!< SPI
#endif
#else
#if defined(__arm__)
#include <SPI.h>
#else
extern HardwareSPI SPI;				//!< SPI
#endif

#if !defined(_SPI)
#define _SPI SPI					//!< SPI
#endif
#endif

// debug
#if defined(MY_DEBUG_VERBOSE_RFM95)
#define RFM95_DEBUG(x,...) debug(x, ##__VA_ARGS__)	//!< Debug print
#else
#define RFM95_DEBUG(x,...)							//!< Debug null
#endif

#define RFM95_FIFO_SIZE					(0xFFu)			//!< Max number of bytes the LORA Rx/Tx FIFO can hold
#define RFM95_RX_FIFO_ADDR				(0x00u)			//!< RX FIFO addr pointer
#define RFM95_TX_FIFO_ADDR				(0x80u)			//!< TX FIFO addr pointer
#define RFM95_MAX_PACKET_LEN			(0x40u)			//!< This is the maximum number of bytes that can be carried by the LORA
#define RFM95_PREAMBLE_LENGTH			(8u)			//!< Preamble length, default=8
#define RFM95_RETRIES					(2u)			//!< Retries in case of failed transmission
#define RFM95_RETRY_TIMEOUT_MS			(500ul)			//!< Timeout for ACK, adjustments needed if modem configuration changed (air time different)
#define RFM95_CAD_TIMEOUT_MS			(2*1000ul)		//!< channel activity detection timeout

#define RFM95_PACKET_HEADER_VERSION		(1u)			//!< RFM95 packet header version
#define RFM95_MIN_PACKET_HEADER_VERSION (1u)			//!< Minimal RFM95 packet header version
#define RFM95_BIT_ACK_REQUESTED			(7u)			//!< RFM95 header, controlFlag, bit 7
#define RFM95_BIT_ACK_RECEIVED			(6u)			//!< RFM95 header, controlFlag, bit 6
#define RFM95_BIT_ACK_RSSI_REPORT		(5u)			//!< RFM95 header, controlFlag, bit 5

#define RFM95_BROADCAST_ADDRESS			(255u)			//!< Broadcasting address 
#define RFM95_ATC_TARGET_RANGE_PERCENT	(5u)			//!< ATC target range +/-%
#define RFM95_RSSI_OFFSET				(137u)			//!< RSSI offset
#define RFM95_TARGET_RSSI				(-60)			//!< RSSI target
#define RFM95_RSSI_INVALID				(127)			//!< RSSI invalid
#define RFM95_SNR_INVALID				(127)			//!< SNR invalid
#define RFM95_PROMISCUOUS				(false)			//!< RFM95 promiscuous mode

#define RFM95_FXOSC						(32*1000000ul)				//!< The crystal oscillator frequency of the module
#define RFM95_FSTEP						(RFM95_FXOSC / 524288ul)	//!< The Frequency Synthesizer step = RFM95_FXOSC / 

// helper macros
#define RFM95_getACKRequested(__value) ((bool)bitRead(__value, RFM95_BIT_ACK_REQUESTED))				//!< getACKRequested
#define RFM95_setACKRequested(__value, __flag) bitWrite(__value, RFM95_BIT_ACK_REQUESTED,__flag)		//!< setACKRequested
#define RFM95_getACKReceived(__value) ((bool)bitRead(__value, RFM95_BIT_ACK_RECEIVED))					//!< getACKReceived
#define RFM95_setACKReceived(__value, __flag) bitWrite(__value, RFM95_BIT_ACK_RECEIVED,__flag)			//!< setACKReceived
#define RFM95_setACKRSSIReport(__value, __flag) bitWrite(__value, RFM95_BIT_ACK_RSSI_REPORT,__flag)		//!< setACKRSSIReport
#define RFM95_getACKRSSIReport(__value) ((bool)bitRead(__value, RFM95_BIT_ACK_RSSI_REPORT))				//!< getACKRSSIReport
#define RFM95_internalToRSSI(__value)	((int16_t)(__value - RFM95_RSSI_OFFSET))	//!< Convert internal RSSI to RSSI
#define RFM95_RSSItoInternal(__value)	((uint8_t)(__value + RFM95_RSSI_OFFSET))	//!< Convert RSSI to internal RSSI
#define RFM95_internalToSNR(__value)	((int8_t)(__value / 4))						//!< Convert internal SNR to SNR


// RFM95 radio configurations: reg_1d, reg_1e, reg_26 (see datasheet)
#define RFM95_BW125CR45SF128 RFM95_BW_125KHZ | RFM95_CODING_RATE_4_5, RFM95_SPREADING_FACTOR_128CPS | RFM95_RX_PAYLOAD_CRC_ON, RFM95_AGC_AUTO_ON //!< 0x72,0x74,0x04
#define RFM95_BW500CR45SF128 RFM95_BW_500KHZ | RFM95_CODING_RATE_4_5, RFM95_SPREADING_FACTOR_128CPS | RFM95_RX_PAYLOAD_CRC_ON, RFM95_AGC_AUTO_ON //!< 0x92,0x74,0x04
#define RFM95_BW31_25CR48SF512 RFM95_BW_31_25KHZ | RFM95_CODING_RATE_4_8, RFM95_SPREADING_FACTOR_512CPS | RFM95_RX_PAYLOAD_CRC_ON, RFM95_AGC_AUTO_ON //!< 0x48,0x94,0x04
#define RFM95_BW125CR48SF4096	RFM95_BW_125KHZ | RFM95_CODING_RATE_4_8, RFM95_SPREADING_FACTOR_4096CPS | RFM95_RX_PAYLOAD_CRC_ON, RFM95_AGC_AUTO_ON | RFM95_LOW_DATA_RATE_OPTIMIZE	//!< 0x78,0xc4,0x0C

#define RFM95_MIN_POWER_LEVEL_DBM		(5u)	//!< min. power level
#define RFM95_MAX_POWER_LEVEL_DBM		(23u)	//!< max. power level

/**
* @brief Radio modes
*/
typedef enum {
	RFM95_RADIO_MODE_RX = 0,					//!< RX mode
	RFM95_RADIO_MODE_TX = 1,					//!< TX mode
	RFM95_RADIO_MODE_CAD = 2,					//!< CAD mode
	RFM95_RADIO_MODE_SLEEP = 3,					//!< SLEEP mode
	RFM95_RADIO_MODE_STDBY = 4					//!< STDBY mode
} rfm95_radioMode_t;

/**
* @brief RFM95 modem config registers
*/
typedef struct {
	uint8_t	reg_1d; //!< Value for register REG_1D_MODEM_CONFIG1
	uint8_t	reg_1e;	//!< Value for register REG_1E_MODEM_CONFIG2
	uint8_t	reg_26;	//!< Value for register REG_26_MODEM_CONFIG3
} rfm95_modemConfig_t;

/**
* @brief Sequence number data type
*/
typedef uint16_t rfm95_sequenceNumber_t;
/**
* @brief RSSI data type
*/
typedef uint8_t rfm95_RSSI_t;
/**
* @brief SNR data type
*/
typedef int8_t rfm95_SNR_t;
/**
* @brief Control flag data type
*/
typedef uint8_t rfm95_flag_t;
/**
* @brief RFM95 LoRa header
*/
typedef struct {
	uint8_t version;								//!< Header version
	uint8_t recipient;								//!< Payload recipient
	uint8_t sender;									//!< Payload sender
	rfm95_flag_t controlFlags;						//!< Control flags, used for ACK
	rfm95_sequenceNumber_t sequenceNumber;			//!< Packet sequence number, used for ACK
} __attribute__((packed)) rfm95_header_t;

/**
* @brief RFM95 LoRa ACK packet structure
*/
typedef struct {
	rfm95_sequenceNumber_t sequenceNumber;				//!< sequence number
	rfm95_RSSI_t RSSI;									//!< RSSI
	rfm95_SNR_t SNR;									//!< SNR
} __attribute__((packed)) rfm95_ack_t;


#define RFM95_HEADER_LEN sizeof(rfm95_header_t)		//!< Size header inside LoRa payload
#define RFM95_MAX_PAYLOAD_LEN (RFM95_MAX_PACKET_LEN - RFM95_HEADER_LEN)	//!< Max payload length

/**
* @brief LoRa packet structure
*/
typedef struct {
	union {
		struct {
			rfm95_header_t header;						//!< LoRa header
			union {
				uint8_t payload[RFM95_MAX_PAYLOAD_LEN];	//!< Payload, i.e. MySensors message
				rfm95_ack_t ACK;						//!< Union: ACK
			};
		};
		uint8_t data[RFM95_MAX_PACKET_LEN];				//!< RAW
	};
	uint8_t payloadLen;									//!< Length of payload (excluding header)
	rfm95_RSSI_t RSSI;									//!< RSSI of current packet, RSSI = value - 137
	rfm95_SNR_t SNR;									//!< SNR of current packet
} __attribute__((packed)) rfm95_packet_t;


/**
* @brief RFM95 internal variables
*/
typedef struct {
	uint8_t address;							//!< Node address
	rfm95_packet_t currentPacket;				//!< Buffer for current packet
	rfm95_sequenceNumber_t txSequenceNumber;	//!< RFM95_txSequenceNumber
	uint8_t powerLevel;							//!< TX power level dBm
	uint8_t ATCtargetRSSI;						//!< ATC: target RSSI
	// 8 bit
	rfm95_radioMode_t radioMode : 3;			//!< current transceiver state
	bool cad : 1;								//!< RFM95_cad
	bool rxBufferValid : 1;						//!< RX buffer valid
	bool ATCenabled : 1;						//!< ATC enabled
	uint8_t reserved : 2;						//!< reserved
} rfm95_internal_t;

#define LOCAL static		//!< static

/**
* @brief Initialise the driver transport hardware and software
* @param frequency
* @return True if initialisation succeeded
*/
LOCAL bool RFM95_initialise(const float frequency);
/**
* @brief Set the driver/node address
* @param addr
*/
LOCAL void RFM95_setAddress(const uint8_t addr);
/**
* @brief Get driver/node address
* @return Node address
*/
LOCAL uint8_t RFM95_getAddress(void);
/**
* @brief Sets all the registered required to configure the data modem in the RF95/96/97/98, including the
* bandwidth, spreading factor etc.
* @param config See modemConfig_t and references therein
*/
LOCAL void RFM95_setModemRegisters(const rfm95_modemConfig_t* config);
/**
* @brief Tests whether a new message is available
* @return True if a new, complete, error-free uncollected message is available to be retreived by @ref RFM95_recv()
*/
LOCAL bool RFM95_available(void);
/**
* @brief If a valid message is received, copy it to buf and return length. 0 byte messages are permitted.
* @param buf Location to copy the received message
* @return Number of bytes
*/
LOCAL uint8_t RFM95_recv(uint8_t* buf);
/**
* @brief RFM95_send
* @param packet
* @return True if packet sent
*/
LOCAL bool RFM95_send(rfm95_packet_t &packet);
/**
* @brief RFM95_sendFrame
* @param recipient
* @param data
* @param len
* @param flags
* @return True if frame sent
*/
LOCAL bool RFM95_sendFrame(const uint8_t recipient, uint8_t* data, const uint8_t len,
                           const rfm95_flag_t flags);
/**
* @brief RFM95_setPreambleLength
* @param preambleLength
*/
LOCAL void RFM95_setPreambleLength(const uint16_t preambleLength);
/**
* @brief Sets the transmitter and receiver centre frequency
* @param centre Frequency in MHz (137.0 to 1020.0)
*/
LOCAL void RFM95_setFrequency(const float centre);
/**
* @brief Sets the transmitter power output level, and configures the transmitter pin
* @param powerLevel Transmitter power level in dBm (+5 to +23)
* @return True power level adjusted
*/
LOCAL bool RFM95_setTxPower(uint8_t powerLevel);
/**
* @brief Sets the radio into low-power sleep mode
* @return true if sleep mode was successfully entered
*/
LOCAL bool RFM95_sleep(void);
/**
* @brief RFM95_sendACK
* @param recipient
* @param sequenceNumber
* @param RSSI (rfm95_RSSI_t)
* @param SNR (rfm95_RSSI_t)
*/
LOCAL void RFM95_sendACK(const uint8_t recipient, const rfm95_sequenceNumber_t sequenceNumber,
                         const rfm95_RSSI_t RSSI, const rfm95_RSSI_t SNR);
/**
* @brief RFM95_sendWithRetry
* @param recipient
* @param buffer
* @param bufferSize
* @param retries
* @param retryWaitTime
* @return True if packet successfully sent
*/
LOCAL bool RFM95_sendWithRetry(const uint8_t recipient, const void* buffer,
                               const uint8_t bufferSize, const uint8_t retries = RFM95_RETRIES,
                               const uint32_t retryWaitTime = RFM95_RETRY_TIMEOUT_MS);
/**
* @brief Wait until no channel activity detected
* @return True if no channel activity detected, False if timeout occured
*/
LOCAL bool RFM95_waitCAD(void);
/**
* @brief RFM95_waitPacketSent
* @return True if packet sent
*/
LOCAL bool RFM95_waitPacketSent(void);
/**
* @brief RFM95_setRadioMode
* @param newRadioMode
* @return True if mode changed
*/
LOCAL bool RFM95_setRadioMode(const rfm95_radioMode_t newRadioMode);
/**
* @brief Low level interrupt handler
*/
LOCAL void RFM95_interruptHandler(void);
/**
* @brief RFM95_clearRxBuffer
*/
LOCAL void RFM95_clearRxBuffer(void);
/**
* @brief RFM95_getSendingRSSI
* @return RSSI Signal strength of last packet received
*/
LOCAL int16_t RFM95_getReceivingRSSI(void);

/**
* @brief RFM95_getSendingRSSI
* @return RSSI Signal strength of last packet sent (if ACK and ATC enabled)
*/
LOCAL int16_t RFM95_getSendingRSSI(void);
/**
* @brief RFM_executeATC
* @param currentRSSI
* @param targetRSSI
* @return True if power level adjusted
*/
LOCAL bool RFM95_executeATC(const rfm95_RSSI_t currentRSSI, const rfm95_RSSI_t targetRSSI);
/**
* @brief RFM95_ATCmode
* @param targetRSSI Target RSSI for transmitter (default -60)
* @param OnOff True to enable ATC
*/
LOCAL void RFM95_ATCmode(const bool OnOff, const int16_t targetRSSI = RFM95_TARGET_RSSI);
/**
* @brief RFM95_sanityCheck
* @return True if sanity check passed
*/
LOCAL bool RFM95_sanityCheck(void);

#endif

/** @}*/
