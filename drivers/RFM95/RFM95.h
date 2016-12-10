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
* This section is WIP!
*
* |E| SYS	 | SUB      | Message							| Comment
* |-|-------|----------|-----------------------------------|---------------------------------------------------------------------
* | | RFM95 | INIT     |									| Initialise RFM95 radio
* | | RFM95 | RCV      | SEND ACK							| ACK request received, sending ACK back
* | | RFM95 | PTC      | LEVEL=%d							| Set TX power level
* | | RFM95 | SAC      | SEND ACK TO=%d,RSSI=%d			| Send ACK to node (TO), RSSI of received message (RSSI)
* | | RFM95 | ATC      | ADJ TXL,cR=%d,tR=%d,TXL=%d		| Adjust TX level, current RSSI (cR), target RSSI (tR), TX level (TXL)
* | | RFM95 | SWR      | SEND TO=%d,RETRY=%d				| Send message to (TO), NACK retry counter (RETRY)
* | | RFM95 | SWR      | ACK FROM=%d,SEQ=%d,RSSI=%d,SNR=%d	| ACK received from node (FROM), seq ID (SEQ), (RSSI), (SNR)
* |!| RFM95 | SWR      | NACK								| No ACK received

*
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

#if defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined(__AVR_ATmega88) || defined(__AVR_ATmega8__) || defined(__AVR_ATmega88__)
#define RFM95_IRQ_PIN		(2)
#define RFM95_RST_PIN		(9)
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284P__)
#define RFM95_IRQ_PIN		(2)
#define RFM95_RST_PIN		(9)
#elif defined(__AVR_ATmega32U4__)
#define RFM95_IRQ_PIN		(3)
#define RFM95_RST_PIN		(9)
#elif defined(__arm__)
#define RFM95_IRQ_PIN		(10)
#define RFM95_RST_PIN		(27)
#else
#define RFM95_IRQ_PIN		(2)			//!< RFM95_IRQ_PIN
#define RFM95_RST_PIN		(9)			//!< RFM95_IRQ_PIN
#endif

// SPI settings
#define RFM95_SPI_CS (SS)					//!< SPI CS/SS pin


#if !defined(MY_RFM95_SPI_MAX_SPEED)
#define MY_RFM95_SPI_MAX_SPEED 4000000	//!< SPI speed
#endif
#define MY_RFM95_SPI_DATA_ORDER MSBFIRST	//!< SPI data order
#define MY_RFM95_SPI_DATA_MODE SPI_MODE0	//!< SPI mode


#if defined (ARDUINO) && !defined (__arm__) && !defined (_SPI)
#include <SPI.h>
#if defined(MY_SOFTSPI)
SoftSPI<MY_SOFT_SPI_MISO_PIN, MY_SOFT_SPI_MOSI_PIN, MY_SOFT_SPI_SCK_PIN, MY_RF24_SPI_DATA_MODE>
_SPI;
#else
#define _SPI SPI
#endif
#else
#if defined(__arm__)
#include <SPI.h>
#else
extern HardwareSPI SPI;		//!< SPI
#endif

#if !defined(_SPI)
#define _SPI SPI			//!< SPI
#endif
#endif

// debug
#if defined(MY_DEBUG_VERBOSE_RFM95)
#define RFM95_DEBUG(x,...) debug(x, ##__VA_ARGS__)	//!< Debug print
#else
#define RFM95_DEBUG(x,...)							//!< Debug null
#endif

#define RFM95_FIFO_SIZE					(0xFFu)						//!< Max number of bytes the LORA Rx/Tx FIFO can hold
#define RFM95_RX_FIFO_ADDR				(0x00u)						//!< RX FIFO addr pointer
#define RFM95_TX_FIFO_ADDR				(0x80u)						//!< TX FIFO addr pointer
#define RFM95_MAX_PACKET_LEN			(0x40u)						//!< This is the maximum number of bytes that can be carried by the LORA
#define RFM95_PREAMBLE_LENGTH			(8u)						//!< Preamble length, default=8


#define RFM95_CAD_TIMEOUT_MS			(2*1000ul)					//!< channel activity detection timeout
#define RFM95_FXOSC						(32000000.0f)				//!< The crystal oscillator frequency of the module
#define RFM95_FSTEP						(RFM95_FXOSC / 524288ul)	//!< The Frequency Synthesizer step = RFM95_FXOSC / 

#define RFM95_PACKET_HEADER_VERSION		(1u)			//!< RFM95 packet header version
#define RFM95_MIN_PACKET_HEADER_VERSION (1u)			//!< Minimal RFM95 packet header version
#define RFM95_RETRIES					(2u)			//!< Retries in case of failed transmission
#define RFM95_RETRY_TIMEOUT_MS			(500ul)			//!< Timeout for ACK, adjustments needed if modem configuration changed (air time different)

#define RFM95_BIT_ACK_REQUESTED			(7u)			//!< RFM95 header, controlFlag, bit 7
#define RFM95_BIT_ACK_RECEIVED			(6u)			//!< RFM95 header, controlFlag, bit 6
#define RFM95_BIT_ACK_RSSI_REPORT		(5u)			//!< RFM95 header, controlFlag, bit 5

#define RFM95_BROADCAST_ADDRESS			(255u)			//!< Broadcasting address 
#define RFM95_RSSI_OFFSET				(137u)			//!< RSSI offset
#define RFM95_TARGET_RSSI				(-60)			//!< RSSI target
#define RFM95_PROMISCUOUS				(false)			//!< RFM95 promiscuous mode

#define RFM95_getACKRequested(__value) ((bool)bitRead(__value, RFM95_BIT_ACK_REQUESTED))					//!< getACKRequested
#define RFM95_setACKRequested(__value, __flag) bitWrite(__value, RFM95_BIT_ACK_REQUESTED,__flag)			//!< setACKRequested
#define RFM95_getACKReceived(__value) ((bool)bitRead(__value, RFM95_BIT_ACK_RECEIVED))						//!< getACKReceived
#define RFM95_setACKReceived(__value, __flag) bitWrite(__value, RFM95_BIT_ACK_RECEIVED,__flag)				//!< setACKReceived
#define RFM95_setACKRSSIReport(__value, __flag) bitWrite(__value, RFM95_BIT_ACK_RSSI_REPORT,__flag)			//!< setACKRSSIReport
#define RFM95_getACKRSSIReport(__value) ((bool)bitRead(__value, RFM95_BIT_ACK_RSSI_REPORT))					//!< getACKRSSIReport

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
* @brief Use the radio's Channel Activity Detect (CAD) function to detect channel activity
* @return true if channel is in use
*/
LOCAL bool RFM95_isChannelActive(void);
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
* @brief RFM95_waitCAD
* @return True if no channel activity detected
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
* @brief RFM95_getRSSI
* @return RSSI Signal strength of last packet
*/
LOCAL int16_t RFM95_getRSSI(void);
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

volatile rfm95_internal_t RFM95;	//!< internal variables

// Register access
#define RFM95_READ_REGISTER		(0x7Fu)	//!< reading register
#define RFM95_WRITE_REGISTER	(0x80u)	//!< writing register

// Registers, available LoRa mode
#define RFM95_REG_00_FIFO						0x00 //!< REG_00_FIFO
#define RFM95_REG_01_OP_MODE					0x01 //!< REG_01_OP_MODE
#define RFM95_REG_02_RESERVED					0x02 //!< REG_02_RESERVED
#define RFM95_REG_03_RESERVED					0x03 //!< REG_03_RESERVED
#define RFM95_REG_04_RESERVED					0x04 //!< REG_04_RESERVED
#define RFM95_REG_05_RESERVED					0x05 //!< REG_05_RESERVED
#define RFM95_REG_06_FRF_MSB					0x06 //!< REG_06_FRF_MSB
#define RFM95_REG_07_FRF_MID					0x07 //!< REG_07_FRF_MID
#define RFM95_REG_08_FRF_LSB					0x08 //!< REG_08_FRF_LSB
#define RFM95_REG_09_PA_CONFIG					0x09 //!< REG_09_PA_CONFIG
#define RFM95_REG_0A_PA_RAMP					0x0a //!< REG_0A_PA_RAMP
#define RFM95_REG_0B_OCP						0x0b //!< REG_0B_OCP
#define RFM95_REG_0C_LNA						0x0c //!< REG_0C_LNA
#define RFM95_REG_0D_FIFO_ADDR_PTR				0x0d //!< REG_0D_FIFO_ADDR_PTR
#define RFM95_REG_0E_FIFO_TX_BASE_ADDR			0x0e //!< REG_0E_FIFO_TX_BASE_ADDR
#define RFM95_REG_0F_FIFO_RX_BASE_ADDR			0x0f //!< REG_0F_FIFO_RX_BASE_ADDR
#define RFM95_REG_10_FIFO_RX_CURRENT_ADDR		0x10 //!< REG_10_FIFO_RX_CURRENT_ADDR
#define RFM95_REG_11_IRQ_FLAGS_MASK				0x11 //!< REG_11_IRQ_FLAGS_MASK
#define RFM95_REG_12_IRQ_FLAGS					0x12 //!< REG_12_IRQ_FLAGS
#define RFM95_REG_13_RX_NB_BYTES				0x13 //!< REG_13_RX_NB_BYTES
#define RFM95_REG_14_RX_HEADER_CNT_VALUE_MSB	0x14 //!< REG_14_RX_HEADER_CNT_VALUE_MSB
#define RFM95_REG_15_RX_HEADER_CNT_VALUE_LSB	0x15 //!< REG_15_RX_HEADER_CNT_VALUE_LSB
#define RFM95_REG_16_RX_PACKET_CNT_VALUE_MSB	0x16 //!< REG_16_RX_PACKET_CNT_VALUE_MSB
#define RFM95_REG_17_RX_PACKET_CNT_VALUE_LSB	0x17 //!< REG_17_RX_PACKET_CNT_VALUE_LSB
#define RFM95_REG_18_MODEM_STAT					0x18 //!< REG_18_MODEM_STAT
#define RFM95_REG_19_PKT_SNR_VALUE				0x19 //!< REG_19_PKT_SNR_VALUE
#define RFM95_REG_1A_PKT_RSSI_VALUE				0x1a //!< REG_1A_PKT_RSSI_VALUE
#define RFM95_REG_1B_RSSI_VALUE					0x1b //!< REG_1B_RSSI_VALUE
#define RFM95_REG_1C_HOP_CHANNEL				0x1c //!< REG_1C_HOP_CHANNEL
#define RFM95_REG_1D_MODEM_CONFIG1				0x1d //!< REG_1D_MODEM_CONFIG1
#define RFM95_REG_1E_MODEM_CONFIG2				0x1e //!< REG_1E_MODEM_CONFIG2
#define RFM95_REG_1F_SYMB_TIMEOUT_LSB			0x1f //!< REG_1F_SYMB_TIMEOUT_LSB
#define RFM95_REG_20_PREAMBLE_MSB				0x20 //!< REG_20_PREAMBLE_MSB
#define RFM95_REG_21_PREAMBLE_LSB				0x21 //!< REG_21_PREAMBLE_LSB
#define RFM95_REG_22_PAYLOAD_LENGTH				0x22 //!< REG_22_PAYLOAD_LENGTH
#define RFM95_REG_23_MAX_PAYLOAD_LENGTH			0x23 //!< REG_23_MAX_PAYLOAD_LENGTH
#define RFM95_REG_24_HOP_PERIOD					0x24 //!< REG_24_HOP_PERIOD
#define RFM95_REG_25_FIFO_RX_BYTE_ADDR			0x25 //!< REG_25_FIFO_RX_BYTE_ADDR
#define RFM95_REG_26_MODEM_CONFIG3				0x26 //!< REG_26_MODEM_CONFIG3

// Reserved when in LoRa mode
#define RFM95_REG_40_DIO_MAPPING1				0x40 //!< REG_40_DIO_MAPPING1
#define RFM95_REG_41_DIO_MAPPING2				0x41 //!< REG_41_DIO_MAPPING2
#define RFM95_REG_42_VERSION					0x42 //!< REG_42_VERSION
#define RFM95_REG_4B_TCXO						0x4b //!< REG_4B_TCXO
#define RFM95_REG_4D_PA_DAC						0x4d //!< REG_4D_PA_DAC
#define RFM95_REG_5B_FORMER_TEMP				0x5b //!< REG_5B_FORMER_TEMP
#define RFM95_REG_61_AGC_REF					0x61 //!< REG_61_AGC_REF
#define RFM95_REG_62_AGC_THRESH1				0x62 //!< REG_62_AGC_THRESH1
#define RFM95_REG_63_AGC_THRESH2				0x63 //!< REG_63_AGC_THRESH2
#define RFM95_REG_64_AGC_THRESH3				0x64 //!< REG_64_AGC_THRESH3

// RFM95_REG_01_OP_MODE 0x01
#define RFM95_LONG_RANGE_MODE					0x80 //!< LONG_RANGE_MODE
#define RFM95_ACCESS_SHARED_REG					0x40 //!< ACCESS_SHARED_REG

#define RFM95_MODE_SLEEP						0x00 //!< MODE_SLEEP
#define RFM95_MODE_STDBY						0x01 //!< MODE_STDBY
#define RFM95_MODE_FSTX							0x02 //!< MODE_FSTX
#define RFM95_MODE_TX							0x03 //!< MODE_TX
#define RFM95_MODE_FSRX							0x04 //!< MODE_FSRX
#define RFM95_MODE_RXCONTINUOUS					0x05 //!< MODE_RXCONTINUOUS
#define RFM95_MODE_RXSINGLE						0x06 //!< MODE_RXSINGLE
#define RFM95_MODE_CAD							0x07 //!< MODE_CAD

// RFM95_REG_09_PA_CONFIG 0x09
#define RFM95_OUTPUT_POWER						0x0F //!< OUTPUT_POWER
#define RFM95_MAX_POWER							0x70 //!< MAX_POWER
#define RFM95_PA_SELECT							0x80 //!< PA_SELECT

// RFM95_REG_0A_PA_RAMP 0x0a
#define RFM95_PA_RAMP_3_4MS						0x00 //!< PA_RAMP_3_4MS
#define RFM95_PA_RAMP_2MS						0x01 //!< PA_RAMP_2MS
#define RFM95_PA_RAMP_1MS						0x02 //!< PA_RAMP_1MS
#define RFM95_PA_RAMP_500US						0x03 //!< PA_RAMP_500US
#define RFM95_PA_RAMP_250US						0x04 //!< PA_RAMP_250US
#define RFM95_PA_RAMP_125US						0x05 //!< PA_RAMP_125US
#define RFM95_PA_RAMP_100US						0x06 //!< PA_RAMP_100US
#define RFM95_PA_RAMP_62US						0x07 //!< PA_RAMP_62US
#define RFM95_PA_RAMP_50US						0x08 //!< PA_RAMP_50US
#define RFM95_PA_RAMP_40US						0x09 //!< PA_RAMP_40US
#define RFM95_PA_RAMP_31US						0x0A //!< PA_RAMP_31US
#define RFM95_PA_RAMP_25US						0x0B //!< PA_RAMP_25US
#define RFM95_PA_RAMP_20US						0x0C //!< PA_RAMP_20US
#define RFM95_PA_RAMP_15US						0x0D //!< PA_RAMP_15US
#define RFM95_PA_RAMP_12US						0x0E //!< PA_RAMP_12US
#define RFM95_PA_RAMP_10US						0x0F //!< PA_RAMP_10US
#define RFM95_LOW_PN_TX_PLL_OFF					0x10 //!<LOW_PN_TX_PLL_OFF

// RFM95_REG_0B_OCP 0x0b
#define RFM95_OCP_TRIM							0x1f //!< OCP_TRIM
#define RFM95_OCP_ON							0x20 //!< OCP_ON

// RFM95_REG_0C_LNA 0x0c
#define RFM95_LNA_BOOST_DEFAULT					0x20 //!< LNA_BOOST_DEFAULT
#define RFM95_LNA_BOOST							0x03 //!< LNA_BOOST

// RFM95_REG_11_IRQ_FLAGS_MASK 0x11
#define RFM95_CAD_DETECTED_MASK					0x01 //!< CAD_DETECTED_MASK
#define RFM95_FHSS_CHANGE_CHANNEL_MASK			0x02 //!< FHSS_CHANGE_CHANNEL_MASK
#define RFM95_CAD_DONE_MASK						0x04 //!< CAD_DONE_MASK
#define RFM95_TX_DONE_MASK						0x08 //!< TX_DONE_MASK
#define RFM95_VALID_HEADER_MASK					0x10 //!< VALID_HEADER_MASK
#define RFM95_PAYLOAD_CRC_ERROR_MASK			0x20 //!< PAYLOAD_CRC_ERROR_MASK
#define RFM95_RX_DONE_MASK						0x40 //!< RX_DONE_MASK
#define RFM95_RX_TIMEOUT_MASK					0x80 //!< RX_TIMEOUT_MASK

// RFM95_REG_12_IRQ_FLAGS 0x12
#define RFM95_CAD_DETECTED						0x01 //!< CAD_DETECTED
#define RFM95_FHSS_CHANGE_CHANNEL				0x02 //!< FHSS_CHANGE_CHANNEL
#define RFM95_CAD_DONE							0x04 //!< CAD_DONE
#define RFM95_TX_DONE							0x08 //!< TX_DONE
#define RFM95_VALID_HEADER						0x10 //!< VALID_HEADER
#define RFM95_PAYLOAD_CRC_ERROR					0x20 //!< PAYLOAD_CRC_ERROR
#define RFM95_RX_DONE							0x40 //!< RX_DONE
#define RFM95_RX_TIMEOUT						0x80 //!< RX_TIMEOUT

// RFM95_REG_18_MODEM_STAT 0x18
#define RFM95_MODEM_STATUS_SIGNAL_DETECTED		0x01 //!< MODEM_STATUS_SIGNAL_DETECTED
#define RFM95_MODEM_STATUS_SIGNAL_SYNCHRONIZED	0x02 //!< MODEM_STATUS_SIGNAL_SYNCHRONIZED
#define RFM95_MODEM_STATUS_RX_ONGOING			0x04 //!< MODEM_STATUS_RX_ONGOING
#define RFM95_MODEM_STATUS_HEADER_INFO_VALID	0x08 //!< MODEM_STATUS_HEADER_INFO_VALID
#define RFM95_MODEM_STATUS_CLEAR				0x10 //!< MODEM_STATUS_CLEAR

// RFM95_REG_1C_HOP_CHANNEL 0x1c
#define RFM95_RX_PAYLOAD_CRC_IS_ON				0x40 //!< RX_PAYLOAD_CRC_IS_ON
#define RFM95_PLL_TIMEOUT						0x80 //!< PLL_TIMEOUT

// RFM95_REG_1D_MODEM_CONFIG1 0x1d

#define RFM95_BW_7_8KHZ							0x00 //!< BW_7_8KHZ	
#define RFM95_BW_10_4KHZ						0x10 //!< BW_10_4KHZ
#define RFM95_BW_15_6KHZ						0x20 //!< BW_15_6KHZ
#define RFM95_BW_20_8KHZ						0x30 //!< BW_20_8KHZ
#define RFM95_BW_31_25KHZ						0x40 //!< BW_31_25KHZ
#define RFM95_BW_41_7KHZ						0x50 //!< BW_41_7KHZ
#define RFM95_BW_62_5KHZ						0x60 //!< BW_62_5KHZ
#define RFM95_BW_125KHZ							0x70 //!< BW_125KHZ
#define RFM95_BW_250KHZ							0x80 //!< BW_250KHZ
#define RFM95_BW_500KHZ							0x90 //!< BW_500KHZ

#define RFM95_IMPLICIT_HEADER_MODE_ON			0x01 //!< IMPLICIT_HEADER_MODE_ON
#define RFM95_CODING_RATE_4_5					0x02 //!< CODING_RATE_4_5
#define RFM95_CODING_RATE_4_6					0x04 //!< CODING_RATE_4_6
#define RFM95_CODING_RATE_4_7					0x06 //!< CODING_RATE_4_7
#define RFM95_CODING_RATE_4_8					0x08 //!< CODING_RATE_4_8

// RFM95_REG_1E_MODEM_CONFIG2 0x1e
#define RFM95_SPREADING_FACTOR_64CPS			0x60 //!< SPREADING_FACTOR_64CPS, SF6
#define RFM95_SPREADING_FACTOR_128CPS			0x70 //!< SPREADING_FACTOR_128CPS, SF7
#define RFM95_SPREADING_FACTOR_256CPS			0x80 //!< SPREADING_FACTOR_256CPS, SF8
#define RFM95_SPREADING_FACTOR_512CPS			0x90 //!< SPREADING_FACTOR_512CPS, SF9
#define RFM95_SPREADING_FACTOR_1024CPS			0xA0 //!< SPREADING_FACTOR_1024CPS, SF10
#define RFM95_SPREADING_FACTOR_2048CPS			0xB0 //!< SPREADING_FACTOR_2048CPS, SF11
#define RFM95_SPREADING_FACTOR_4096CPS			0xC0 //!< SPREADING_FACTOR_4096CPS, SF12

#define RFM95_SYM_TIMEOUT_MSB					0x03 //!< SYM_TIMEOUT_MSB
#define RFM95_RX_PAYLOAD_CRC_ON					0x04 //!< RX_PAYLOAD_CRC_ON
#define RFM95_TX_CONTINUOUS_MOdE				0x08 //!< TX_CONTINUOUS_MODE

// RFM95_REG_26_MODEM_CONFIG3 0x26
#define RFM95_LOW_DATA_RATE_OPTIMIZE			0x08 //!< LOW_DATA_RATE_OPTIMIZE
#define RFM95_AGC_AUTO_ON						0x04 //!< AGC_AUTO_ON

// RFM95_REG_4D_PA_DAC 0x4d
#define RFM95_PA_DAC_DISABLE					0x04 //!< PA_DAC_DISABLE
#define RFM95_PA_DAC_ENABLE						0x07 //!< PA_DAC_ENABLE


#endif

/** @}*/
