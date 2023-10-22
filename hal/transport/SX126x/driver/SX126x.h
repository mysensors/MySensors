/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2023 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Based on Olivier Mauti's RFM95 driver and Semtecs reference driver for the SX126x
 *
 * SX126x driver for MySensors, Copyright (C) 2020 Eduard Iten <eduard@iten.pro>
 *
 */

/**
* @file SX126x.h
*
* @defgroup SX126xgrp SX126x
* @ingroup internals
* @{
*
* SX126x driver-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
* - [!] Exclamation mark is prepended in case of error
*
* |E| SYS    | SUB  | Message                                | Comment
* |-|--------|------|----------------------------------------|-----------------------------------------------------------------------------------------
* | | SX126x | INIT |                                        | Initialise SX126x radio
* | | SX126x | INIT | PWRPIN=%%d                             | Power pin number (if configured)
* | | SX126x | INIT | BSYPIN=%%d                             | Busy pin number (if configured)
* | | SX126x | INIT | IRQPIN=%%d                             | DIO1 pin number (if configured)
* | | SX126x | INIT | RSTPIN=%%d                             | Reset pin number (if configured)
* | | SX126x | INIT | ASWPIN=%%d                             | Antenna switch pin number (if configured)
* | | SX126x | INIT | DIO3TCXO,VCONF:%%dX,DELAY:%%dms        | DIO 3 was configured as TCXO control with voltage (VCNONF) and start up delay (DELAY)ms
* | | SX126x | INIT | TCXO,EXT                               | External TCXO was configured
* | | SX126x | INIT | DIO2AntSw                              | DIO 2 was configured as antenna switch control
* |!| SX126x | INIT | SANCHK FAIL                            | Sanity check failed, check wiring or replace module
* |!| SX126x | INIT | ERR:0x%%X%%X                           | An error occured during init. See GetDeviceErrors in datasheet
* | | SX126x | RCV  | SEND ACK                               | ACK request received, sending ACK back
* | | SX126x | PTC  | LEVEL=%%d                              | Set TX power level
* | | SX126x | SAC  | SEND ACK,TO=%%d,RSSI=%%d,SNR=%%d       | Send ACK to node (TO), RSSI of received message (RSSI), SNR of message (SNR)
* | | SX126x | ATC  | ADJ TXL,cR=%%d,tR=%%d..%%d,rTXL=%%d    | Adjust TX level, current RSSI (cR), target RSSI range (tR), requested TX level (rTXL)
* | | SX126x | SWR  | SEND,TO=%%d,RETRY=%%d                  | Send message to (TO), NACK retry counter (RETRY)
* | | SX126x | SWR  | ACK FROM=%%d,SEQ=%%d,RSSI=%%d,SNR=%%d  | ACK received from node (FROM), seq ID (SEQ), (RSSI), (SNR)
* |!| SX126x | SWR  | NACK                                   | No ACK received
* |!| SX126x | CAD  |                                        | Could not send, channel active
* | | SX126x | SPP  | PCT=%%d,TX LEVEL=%%d                   | Set TX level percent (PCT), TX level (LEVEL)
* | | SX126x | PWD  |                                        | Power down radio
* | | SX126x | PWU  |                                        | Power up radio
*
* SX126x modem configuration
*
* BW = Bandwidth in kHz
* CR = Error correction code
* SF = Spreading factor, chips / symbol
*
* | CONFIG           | BW    | CR  | SF   | Comment               | air-time (15 bytes)
* |------------------|-------|-----|------|-----------------------|------------------------
* | BW125CR45SF128   | 125   | 4/5 | 128  | Default, medium range | 50ms
* | BW500CR45SF128   | 500   | 4/5 | 128  | Fast, short range     | 15ms
* | BW31_25CR48SF512 | 31.25 | 4/8 | 512  | Slow, long range      | 900ms
* | BW125CR48SF4096  | 125   | 4/8 | 4096 | Slow, long range      | 1500ms
*
* See here for air-time calculation: https://docs.google.com/spreadsheets/d/1voGAtQAjC1qBmaVuP1ApNKs1ekgUjavHuVQIXyYSvNc
*
* @brief API declaration for SX126x
*
*/

#ifndef _SX126x_h
#define _SX126x_h

#include <stdint.h>

// SX126x hardware defaults
#if !defined(SX126x_SPI)
#define SX126x_SPI hwSPI //!< default SPI
#endif

// default PIN assignments, can be overridden
#if defined(ARDUINO_ARCH_AVR)
#if defined(__AVR_ATmega32U4__)
#define DEFAULT_SX126x_IRQ_PIN			(3)				//!< DEFAULT_SX126x_IRQ_PIN
#else
#define DEFAULT_SX126x_IRQ_PIN			(2)				//!< DEFAULT_SX126x_IRQ_PIN
#endif
#elif defined(ARDUINO_ARCH_ESP8266)
#define DEFAULT_SX126x_IRQ_PIN			(5)				//!< DEFAULT_SX126x_IRQ_PIN
#elif defined(ARDUINO_ARCH_ESP32)
#define DEFAULT_SX126x_IRQ_PIN			(16)												//!< DEFAULT_SX126x_IRQ_PIN
#define DEFAULT_SX126x_IRQ_NUM			digitalPinToInterrupt(DEFAULT_SX126x_IRQ_PIN)		//!< DEFAULT_SX126x_IRQ_NUM
#elif defined(ARDUINO_ARCH_SAMD)
#define DEFAULT_SX126x_IRQ_PIN			(2)				//!< DEFAULT_SX126x_IRQ_PIN
#elif defined(LINUX_ARCH_RASPBERRYPI)
#define DEFAULT_SX126x_IRQ_PIN			(22)			//!< DEFAULT_SX126x_IRQ_PIN
#elif defined(ARDUINO_ARCH_STM32F1)
#define DEFAULT_SX126x_IRQ_PIN			(PA3)			//!< DEFAULT_SX126x_IRQ_PIN
#elif defined(TEENSYDUINO)
#define DEFAULT_SX126x_IRQ_PIN			(8)				//!< DEFAULT_SX126x_IRQ_PIN
#else
#define DEFAULT_SX126x_IRQ_PIN			(2)				//!< DEFAULT_SX126x_IRQ_PIN
#endif

#ifndef DEFAULT_SX126x_CS_PIN
#define DEFAULT_SX126x_CS_PIN			(SS)			//!< DEFAULT_SX126x_CS_PIN
#endif

// Frequency helpers
#define SX126x_XTAL_FREQ ( double )32000000				//!< XTAL frequency
#define SX126x_FREQ_DIV  ( double )pow( 2.0, 25.0 )		//!<  Frequency divider
#define SX126x_FREQ_STEP ( double )( SX126x_XTAL_FREQ / SX126x_FREQ_DIV )	//!< Frequency step 

// SX126x radio configurations: reg_1d, reg_1e, reg_26 (see datasheet)
#define SX126x_BW125CR45SF128 SX126x_BW_125KHZ | SX126x_CODING_RATE_4_5, SX126x_SPREADING_FACTOR_128CPS | SX126x_RX_PAYLOAD_CRC_ON, SX126x_AGC_AUTO_ON									 //!< 0x72,0x74,0x04
#define SX126x_BW500CR45SF128 SX126x_BW_500KHZ | SX126x_CODING_RATE_4_5, SX126x_SPREADING_FACTOR_128CPS | SX126x_RX_PAYLOAD_CRC_ON, SX126x_AGC_AUTO_ON									 //!< 0x92,0x74,0x04
#define SX126x_BW31_25CR48SF512 SX126x_BW_31_25KHZ | SX126x_CODING_RATE_4_8, SX126x_SPREADING_FACTOR_512CPS | SX126x_RX_PAYLOAD_CRC_ON, SX126x_AGC_AUTO_ON								 //!< 0x48,0x94,0x04
#define SX126x_BW125CR48SF4096 SX126x_BW_125KHZ | SX126x_CODING_RATE_4_8, SX126x_SPREADING_FACTOR_4096CPS | SX126x_RX_PAYLOAD_CRC_ON, SX126x_AGC_AUTO_ON | SX126x_LOW_DATA_RATE_OPTIMIZE //!< 0x78,0xc4,0x0C

#define SX126x_BROADCAST_ADDRESS (255u)	 //!< Broadcasting address
#define SX126x_ATC_TARGET_RANGE_DBM (2u) //!< ATC target range +/- dBm
#define SX126x_RSSI_OFFSET (137u)		 //!< RSSI offset
#define SX126x_TARGET_RSSI (-70)		 //!< RSSI target
#define SX126x_PROMISCUOUS (false)		 //!< SX126x promiscuous mode

#if (MY_SX126x_MAX_POWER_LEVEL_DBM) <= (MY_SX126x_MIN_POWER_LEVEL_DBM)
#error SX126x_MAX_POWER_DBM has to be bigger than SX126x_MIN_POWER_DBM
#endif

#define SX126x_PACKET_HEADER_VERSION (1u)     //!< SX126x packet header version
#define SX126x_MIN_PACKET_HEADER_VERSION (1u) //!< Minimal SX126x packet header version

#if !defined(SX126x_RETRY_TIMEOUT_MS)
// air-time approximation for timeout, 1 hop ~15 bytes payload - adjust if needed
// BW125/SF128: 50ms
// BW500/SF128: 15ms
// BW31.25/SF512: 900ms
// BW125/SF4096: 1500ms
#define SX126x_RETRY_TIMEOUT_MS (500ul) //!< Timeout for ACK, adjustments needed if modem configuration changed (air time different)
#endif

#if !defined(MY_SX126x_TX_TIMEOUT_MS)
#define MY_SX126x_TX_TIMEOUT_MS                 (5*1000ul)		//!< TX timeout
#endif

/**
 * @def MY_SX126x_TX_POWER_DBM
 * @brief Set TX power level, default 13dBm (overridden if ATC mode enabled)
 *
 * See here https://en.wikipedia.org/wiki/Short_Range_Devices
 */
#ifndef MY_SX126x_TX_POWER_DBM
#define MY_SX126x_TX_POWER_DBM (13u)	// 20mW
#endif

// Frequency definitions
#define SX126x_169MHZ (169000000ul) //!< 169 Mhz
#define SX126x_315MHZ (315000000ul) //!< 315 Mhz
#define SX126x_434MHZ (433920000ul) //!< 433.92 Mhz
#define SX126x_868MHZ (868100000ul) //!< 868.1 Mhz
#define SX126x_915MHZ (915000000ul) //!< 915 Mhz

#define SX126x_RETRIES (5u)				   //!< Retries in case of failed transmission
#define SX126x_MAX_PACKET_LEN (0x40u)	   //!< This is the maximum number of bytes that can be carried by the LORA
#define SX126x_PREAMBLE_LENGTH (8u)		   //!< Preamble length, default=8
#define SX126x_CAD_TIMEOUT_MS (2 * 1000ul) //!< channel activity detection timeout

#define SX126x_POWERUP_DELAY_MS 3	 //!< Power up time of the SX126x
#define SX126x_SLEEP_WARM_START 0x04 //!< Retain configuration during sleep

/**
 * @brief SX126x register adresses
 */
#define SX126x_REG_OCP           0x08E7 //!< Over current protection value
#define SX126x_REG_LORASW		 0x0740 //!< LoRa SyncWord

/**
 * @brief Commands for the SX126x chip
 */
typedef enum sx126xCommands_e {
	SX126x_RESET_STATS = 0x00,
	SX126x_CLR_IRQSTATUS = 0x02,
	SX126x_CLR_ERROR = 0x07,
	SX126x_CFG_DIOIRQ = 0x08,
	SX126x_SET_LORASYMBTIMEOUT = 0xA0,
	SX126x_WRITE_REGISTER = 0x0D,
	SX126x_WRITE_BUFFER = 0x0E,
	SX126x_GET_STATS = 0x10,
	SX126x_GET_PACKETTYPE = 0x11,
	SX126x_GET_IRQSTATUS = 0x12,
	SX126x_GET_RXBUFFERSTATUS = 0x13,
	SX126x_GET_PACKETSTATUS = 0x14,
	SX126x_GET_RSSIINST = 0x15,
	SX126x_GET_ERROR = 0x17,
	SX126x_READ_REGISTER = 0x1D,
	SX126x_READ_BUFFER = 0x1E,
	SX126x_SET_STANDBY = 0x80,
	SX126x_SET_RX = 0x82,
	SX126x_SET_TX = 0x83,
	SX126x_SET_SLEEP = 0x84,
	SX126x_SET_RFFREQUENCY = 0x86,
	SX126x_SET_CADPARAMS = 0x88,
	SX126x_CALIBRATE = 0x89,
	SX126x_SET_PACKETTYPE = 0x8A,
	SX126x_SET_MODULATIONPARAMS = 0x8B,
	SX126x_SET_PACKETPARAMS = 0x8C,
	SX126x_SET_TXPARAMS = 0x8E,
	SX126x_SET_BUFFERBASEADDRESS = 0x8F,
	SX126x_SET_TXFALLBACKMODE = 0x93,
	SX126x_SET_RXDUTYCYCLE = 0x94,
	SX126x_SET_PACONFIG = 0x95,
	SX126x_SET_REGULATORMODE = 0x96,
	SX126x_SET_TCXOMODE = 0x97,
	SX126x_CALIBRATEIMAGE = 0x98,
	SX126x_SET_RFSWITCHMODE = 0x9D,
	SX126x_SET_STOPRXTIMERONPREAMBLE = 0x9F,
	SX126x_GET_STATUS = 0xC0,
	SX126x_SET_FS = 0xC1,
	SX126x_SET_CAD = 0xC5,
	SX126x_SET_TXCONTINUOUSWAVE = 0xD1,
	SX126x_SET_TXCONTINUOUSPREAMBLE = 0xD2,
} sx126x_commands_t;

/**
 * @brief Declares the oscillator in use while in standby mode
 *
 * Using the STDBY_RC standby mode allow to reduce the energy consumption
 * STDBY_XOSC should be used for time critical applications
 */
typedef enum {
	SX126x_STDBY_RC = 0x00,
	SX126x_STDBY_XOSC = 0x01,
} sx126x_radioStandbyModes_t;

/**
 * @brief Declares the power regulation used to power the device
 *
 * This command allows the user to specify if DC-DC or LDO is used for power regulation.
 * Using only LDO implies that the Rx or Tx current is doubled
 */
typedef enum {
	SX126x_USE_LDO = 0x00, // default
	SX126x_USE_DCDC = 0x01,
} sx126x_radioRegulatorMode_t;

/**
 * @brief Declares the possible packet type (i.e. modem) used
 */
typedef enum {
	SX126x_PACKET_TYPE_GFSK = 0x00,
	SX126x_PACKET_TYPE_LORA = 0x01,
	SX126x_PACKET_TYPE_NONE = 0x0F,
} sx126x_radioPacketTypes_t;

/**
 * @brief data type to receive RX buffer status
 */
typedef union {
	struct {
		uint8_t payloadLength;	//!< payloadLength
		uint8_t startPointer;	//!< startPointer
	} fields;					//!< fields
	uint8_t values[2];			//!< values
} sx126x_rxBufferStatus_t;

/**
 * @brief data type to receive packet status
 */
typedef union {
	struct {
		uint8_t rawRssiPkt;		//!< rawRssiPkt
		int8_t rawSnrPkt;		//!< rawSnrPkt
		uint8_t rawSignalRssiPkt; //!< rawSignalRssiPkt
	} fields;					//!< fields
	uint8_t values[3];			//!< values
} sx126x_packetStatus_t;

/**
 * @brief sx126x sleep parameters
 */
typedef union {
	struct {
		bool wakeUpRTC : 1; //!< wake up on RTC timeout
		uint8_t : 1;		//!< reserved for future use
		bool warmStart : 1; //!< false: cold start. true: warm start with config retention
		uint8_t : 5;		//!< reserved
	} fields;				//!< fields
	uint8_t value;			//!< value
} sx126x_sleepParams_t;

/**
 * @brief SX126x operation Modes
 */
typedef enum {
	SX126x_MODE_SLEEP = 0x00, //!< SX126x is in sleep mode
	SX126x_MODE_STDBY_RC,	  //!< SX126x is in standby mode with RC oscillator
	SX126x_MODE_STDBY_XOSC,	  //!< SX126x is in standby mode with XOSC oscillator
	SX126x_MODE_FS,			  //!< SX126x is in frequency synthesis mode
	SX126x_MODE_TX,			  //!< SX126x is in transmit mode
	SX126x_MODE_RX,			  //!< SX126x is in receive mode
	SX126x_MODE_RX_DC,		  //!< SX126x is in receive duty cycle mode
	SX126x_MODE_CAD			  //!< SX126x is in channel activity detection mode
} sx126x_radioModes_t;

/**
 * @brief LoRa spreading factors
 */
typedef enum {
	LORA_SF5                                = 0x05,
	LORA_SF6                                = 0x06,
	LORA_SF7                                = 0x07,
	LORA_SF8                                = 0x08,
	LORA_SF9                                = 0x09,
	LORA_SF10                               = 0x0A,
	LORA_SF11                               = 0x0B,
	LORA_SF12                               = 0x0C,
} sx126x_loRaSpreadingFactors_t;

/**
 * @brief LoRa Bandwiths
 */
typedef enum {
	LORA_BW_500                             = 6,
	LORA_BW_250                             = 5,
	LORA_BW_125                             = 4,
	LORA_BW_062                             = 3,
	LORA_BW_041                             = 10,
	LORA_BW_031                             = 2,
	LORA_BW_020                             = 9,
	LORA_BW_015                             = 1,
	LORA_BW_010                             = 8,
	LORA_BW_007                             = 0,
} sx126x_loRaBandwidths_t;

/**
 * @brief LoRa coding rates
 */
typedef enum {
	LORA_CR_4_5                             = 0x01,
	LORA_CR_4_6                             = 0x02,
	LORA_CR_4_7                             = 0x03,
	LORA_CR_4_8                             = 0x04,
} sx126x_loRaCodingRates_t;

/**
 * @brief LoRa CAD number of symbols
 */
typedef enum {
	LORA_CAD_ON_1_SYMB  = 0x00,
	LORA_CAD_ON_2_SYMB  = 0x01,
	LORA_CAD_ON_4_SYMB  = 0x02,
	LORA_CAD_ON_8_SYMB  = 0x03,
	LORA_CAD_ON_16_SYMB = 0x04,
} sx126x_loRaCADSymbols_t;

/**
 * @brief CAD exit type
 */
typedef enum {
	SX126x_CAD_ONLY = 0x00,
	SX126x_CAD_RX   = 0x01,
} sx126x_cadExitModes_t;

/**
 * @brief CAD parameters type
 */
typedef union {
	struct {
		sx126x_loRaCADSymbols_t cadSymbolsNum;		//!< cadSymbolsNum
		uint8_t cadDetPeak;							//!< cadDetPeak
		uint8_t cadDetMin;							//!< cadDetMin
		sx126x_cadExitModes_t cadExitMode;			//!< cadExitMode
		uint8_t cadTimeout1;						//!< cadTimeout1
		uint8_t cadTimeout2;						//!< cadTimeout2
		uint8_t cadTimeout3;						//!< cadTimeout3
	} fields;										//!< fields
	uint8_t values[7];								//!< values
} sx126x_cadParameters_t;

// Define CAD parameters

/*!
 * @brief Typedef for the SX126x modulation settings
 */
typedef struct {
	union {
		struct {
			sx126x_loRaSpreadingFactors_t spreadingFactor;  //!< Spreading Factor for the LoRa modulation
			sx126x_loRaBandwidths_t bandwidth;         		//!< Bandwidth for the LoRa modulation
			sx126x_loRaCodingRates_t codingRate;        	//!< Coding rate for the LoRa modulation
			bool lowDatarateOptimize; 						//!< Indicates if the modem uses the low datarate optimization
		} fields;											//!< fields
		uint8_t values[8];									//!< values
	};
} sx126x_modulationParams_t;

/**
 * @brief SX126x packet types
 */
typedef enum {
	PACKET_TYPE_GFSK                        = 0x00,
	PACKET_TYPE_LORA                        = 0x01,
	PACKET_TYPE_NONE                        = 0x0F,
} SX126x_packetTypes_t;

/**
 * @brief PA settings parameters type
 */
typedef struct {
	union {
		struct {
			uint8_t paDutyCycle;		//!< paDutyCycle
			uint8_t hpMax;				//!< hpMax
			uint8_t deviceSel;			//!< deviceSel
			uint8_t paLut = 0x01;		//!< paLut
		} fields;						//!< fields
		uint8_t values[4];				//!< values
	};
} sx126x_paSettings_t;

/**
 * @brief Ramping time for power amplifier
 */
typedef enum {
	RADIO_RAMP_10_US                        = 0x00,
	RADIO_RAMP_20_US                        = 0x01,
	RADIO_RAMP_40_US                        = 0x02,
	RADIO_RAMP_80_US                        = 0x03,
	RADIO_RAMP_200_US                       = 0x04,
	RADIO_RAMP_800_US                       = 0x05,
	RADIO_RAMP_1700_US                      = 0x06,
	RADIO_RAMP_3400_US                      = 0x07,
} sx126x_RampTimes_t;

/**
 * @brief Transmission settings parameters type
 */
typedef struct {
	union {
		struct {
			uint8_t power;					//!< power
			sx126x_RampTimes_t rampTime;	//!< rampTime
		} fields;							//!< fields
		uint8_t values[2];					//!< values
	};
} sx126x_txSettings_t;

/**
* @brief Sequence number data type
*/
typedef uint16_t sx126x_sequenceNumber_t; // will eventually change to uint8_t in 3.0
/**
* @brief RSSI data type
*/
typedef uint8_t sx126x_RSSI_t;
/**
* @brief SNR data type
*/
typedef int8_t sx126x_SNR_t;
/**
* @brief Control flag data type
*/
typedef union {
	struct {
		uint8_t :5;				//!< reserved
		bool ackRssiReport :1;	//!< ackRssiReport
		bool ackReceived :1;	//!< ackReceived
		bool ackRequested :1;	//!< ackRequested
	} fields;					//!< fields
	uint8_t values;				//!< values
} sx126x_controlFlags_t;

/**
* @brief Power level in dBm
*/
typedef int8_t sx126x_powerLevel_t;
/**
* @brief SX126x LoRa header
*/
typedef struct {
	uint8_t version;						//!< Header version
	uint8_t recipient;						//!< Payload recipient
	uint8_t sender;							//!< Payload sender
	sx126x_controlFlags_t controlFlags;		//!< Control flags, used for ACK
	sx126x_sequenceNumber_t sequenceNumber; //!< Packet sequence number, used for ACK
} __attribute__((packed)) sx126x_header_t;

/**
* @brief SX126x LoRa ACK packet structure
*/
typedef struct {
	sx126x_sequenceNumber_t sequenceNumber; //!< sequence number
	sx126x_RSSI_t RSSI;						//!< RSSI
	sx126x_SNR_t SNR;						//!< SNR
} __attribute__((packed)) sx126x_ack_t;

#define SX126x_HEADER_LEN sizeof(sx126x_header_t)						   //!< Size header inside LoRa payload
#define SX126x_MAX_PAYLOAD_LEN (SX126x_MAX_PACKET_LEN - SX126x_HEADER_LEN) //!< Max payload length

/**
* @brief LoRa packet structure
*/
typedef struct {
	union {
		struct {
			sx126x_header_t header; //!< LoRa header
			union {
				uint8_t payload[SX126x_MAX_PAYLOAD_LEN]; //!< Payload, i.e. MySensors message
				sx126x_ack_t ACK;						 //!< Union: ACK
			};
		};
		uint8_t data[SX126x_MAX_PACKET_LEN]; //!< RAW
	};
	uint8_t payloadLen; //!< Length of payload (excluding header)
	sx126x_RSSI_t RSSI; //!< RSSI of current packet, RSSI = value - 137
	sx126x_SNR_t SNR;	//!< SNR of current packet
} __attribute__((packed)) sx126x_packet_t;

/**
 * @brief SX126x internal variables
 */
typedef struct {
	uint8_t address;						  //!< Node address
	sx126x_packet_t currentPacket;			  //!< Buffer for current packet
	sx126x_sequenceNumber_t txSequenceNumber; //!< SX126x_txSequenceNumber
	sx126x_powerLevel_t powerLevel;			  //!< TX power level dBm
	sx126x_radioModes_t radioMode;			  //!< Current radio mode
	sx126x_powerLevel_t targetRSSI;           //!< ATC target power level
	bool ATCenabled;                          //!< ATC enabled
	bool channelActive : 1;					  //!< SX126x_cadDetected
	bool channelFree : 1;                     //!< SX126x_cadDone
	volatile bool ackReceived : 1;			  //!< ACK received
	volatile bool dataReceived : 1;			  //!< Data received
	volatile bool txComplete : 1;			  //!< Transmission complete
	volatile bool irqFired : 1;               //!< irq occured
} sx126x_internal_t;

/**
 * @brief SX126x TCXO output voltages
 */
typedef enum {
	SX126x_TCXO_1V6 = 0x00,
	SX126x_TCXO_1V7,
	SX126x_TCXO_1V8,
	SX126x_TCXO_2V2,
	SX126x_TCXO_2V4,
	SX126x_TCXO_2V7,
	SX126x_TCXO_3V0,
	SX126x_TCXO_3V3,
} sx126x_tcxoVoltage_t;

/**
 * @brief IRQ masks
 */
typedef enum {
	SX126x_IRQ_NONE                          = 0x0000,
	SX126x_IRQ_TX_DONE                       = 0x0001,
	SX126x_IRQ_RX_DONE                       = 0x0002,
	SX126x_IRQ_PREAMBLE_DETECTED             = 0x0004,
	SX126x_IRQ_SYNCWORD_VALID                = 0x0008,
	SX126x_IRQ_HEADER_VALID                  = 0x0010,
	SX126x_IRQ_HEADER_ERROR                  = 0x0020,
	SX126x_IRQ_CRC_ERROR                     = 0x0040,
	SX126x_IRQ_CAD_DONE                      = 0x0080,
	SX126x_IRQ_CAD_ACTIVITY_DETECTED         = 0x0100,
	SX126x_IRQ_RX_TX_TIMEOUT                 = 0x0200,
	SX126x_IRQ_ALL                           = 0xFFFF,
} sx126x_irqMasks_t;

/**
 * @brief Sends a command to the SX126x
 * @param command SX126x command
 * @param buffer parameters
 * @param size length of parameters
 **/
static void
SX126x_sendCommand(sx126x_commands_t command, uint8_t *buffer, uint16_t size);

/**
 * @brief Sends a command with a single uint8_t parameter to the SX126x
 * @param command SX126x command
 * @param parameter parameter
 **/
static void SX126x_sendCommand(sx126x_commands_t command, uint8_t parameter);

/**
 * @brief Reads from the SX126x as a reply to a command
 * @param command SX126x command
 * @param buffer parameters
 * @param size length of parameters
 **/
static void SX126x_readCommand(sx126x_commands_t command, uint8_t *buffer, uint16_t size);

/**
 * @brief Writes to a SX126x register
 * @param address register address
 * @param buffer values to write
 * @param size size of buffer
 **/
static void SX126x_sendRegisters(uint16_t address, uint8_t *buffer, uint16_t size);

/**
 * @brief Writes a single byte to a SX126x register
 * @param address register address
 * @param value value to write
 */
static void SX126x_sendRegister(uint16_t address, uint8_t value);

/**
* @brief Initialise the driver transport hardware and software
* @return True if initialisation succeeded
*/
static bool SX126x_initialise();

/**
 * @brief Wake up the SX126x
 **/
static void SX126x_wakeUp();

/**
 * @brief Send the SX126x to standby
 */
static void SX126x_standBy();

/**
 * @brief Wait until SX126x is not busy anymore
 */
static void SX126x_busy();

/**
 * @brief Sends the SX126x to sleep mode
 */
static void SX126x_sleep();

/**
 * @brief Handles SX126x fired irq
 */
static void SX126x_interruptHandler();

/**
 * @brief Handles SX126x events
 */
static void SX126x_handle();

/**
 * @brief Sets TX power of the module
 * @param power the output power in dDm, -3..15 for sx1261, -3..22 for sx1262
 */
static bool SX126x_txPower(sx126x_powerLevel_t power);

/**
 * @brief Sets the operating frequency
 * @param frequency the frequency in Hz
 */
static void SX126x_setFrequency(uint32_t frequency);

/**
 * @brief Sets the IRQ mask of DIO1
 * @param irqMask IRQ mask
 */
static void SX126x_setIrqMask(uint16_t mask);

/**
 * @brief Clears IRQs
 * @param irqMask IRQ mask
 */
static void SX126x_clearIrq(uint16_t mask);

/**
 * @brief Sets the parameters for the LoRa packets
 * @param length length of the LoRa
 */
static void SX126x_setPacketParameters(uint8_t length);

/**
 * @brief Checks if communication to the SX126x works
 * @return True if communication is OK
 */
static bool SX126x_sanityCheck();

/**
 * @brief set the node address
 * @param address the device address
 */
static void SX126x_setAddress(uint8_t address);

/**
 * @brief get the node address
 * @return returns the node address
 */
static uint8_t SX126x_getAddress(void);

/**
* @brief SX126x_sendWithRetry
* @param recipient recipient of the message
* @param buffer buffer to send
* @param bufferSize size of the buffer
* @param noACK if set to true, no ack will be requested
* @return True if packet successfully sent
*/
static bool SX126x_sendWithRetry(const uint8_t recipient, const void *buffer,
                                 const uint8_t bufferSize, const bool noACK);

/**
* @brief Prepare packet to send
* @param recipient recipient of the packet
* @param data data of the packet
* @param len length of the data
* @param flags flags of the packet
* @return True if packet sent
*/
static bool SX126x_send(const uint8_t recipient, uint8_t *data, const uint8_t len,
                        const sx126x_controlFlags_t flags);
/**
* @brief SX126x_sendPacket
* @param packet the packet to send
* @return True if frame sent
*/
static bool SX126x_sendPacket(sx126x_packet_t *packet);

/**
 * @brief Sends buffer to the sx126x tx buffer
 * @param offset offset in the tx buffer
 * @param buffer buffer to send
 * @param size size of the buffer
 */
static void SX126x_sendBuffer(const uint8_t offset, const uint8_t *buffer, const uint8_t size);

/**
 * @brief Reads into buffer from the sx126x rx buffer
 * @param offset offset in the rx buffer
 * @param buffer buffer to read into
 * @param size size of the buffer
 */
static void SX126x_readBuffer(const uint8_t offset, uint8_t *buffer, const uint8_t size);

/**
 * @brief Puts SX126x into transmitting mode
 */
static void SX126x_tx();


/**
 * @brief Puts SX126x into receiving mode
 */
static void SX126x_rx();

/**
 * @brief Scans channel for activity
 * @return true, if channel is free, false if there is activity on channel
 */
static bool SX126x_cad();

/**
 * @brief Checks if an unprocessed packet is available
 * @return true if an unprocessed packet is available
 */
static bool SX126x_packetAvailable();

/**
 * @brief Writes available data to the given buffer
 * @param buffer Pointer to the buffer where to write the data
 * @param bufrerSize size of the receiving buffer
 */
static uint8_t SX126x_getData(uint8_t *buffer, const uint8_t bufferSize);

/**
 * @brief sends an acknowledge to the sender
 * @param recipient Recipient of the ack
 * @param sequenceNumber sequence number of the packet we acknowledge
 * @param RSSI RSSI of the package we acknowledge
 * @param SNR SNR of the package we acknowledge
 */
static void SX126x_sendAck(const uint8_t recipient, const sx126x_sequenceNumber_t sequenceNumber,
                           const sx126x_RSSI_t RSSI, const sx126x_SNR_t SNR);

/**
 * @brief sets power level according to target RSSI, based on RSSI reported by ACK
 */
static void SX126x_ATC();

/**
 * @brief sets the ATC mode and target RSSI
 * @param onOff true to switch on ATC
 * @param targetRSSI the desired RSSI
 */
static void SX126x_setATC(bool onOff, int8_t targetRSSI);

/**
 * @brief get our own RSSI as reported by ACK
 * @return own RSSI in dBm
 */
static int16_t SX126x_getSendingRSSI();

/**
 * @brief get our own SNR as reported by ACK
 * @return own SNR in dBm
 */
static int16_t SX126x_getSendingSNR();

/**
 * @brief get remote RSSI
 * @return remote RSSI in dBm
 */
static int16_t SX126x_getReceivingRSSI();

/**
 * @brief get remote SNR
 * @return remote SNR in dBm
 */
static int16_t SX126x_getReceivingSNR();

/**
 * @brief get TX power level in dBm
 * @return TX power level in dBm
 **/
static int8_t SX126x_getTxPowerLevel();

/**
 * @brief get TX power level in percent
 * @return TX power level in percent
 */
static uint8_t SX126x_getTxPowerPercent();

/**
 * @brief set TX power level in percent
 * @param newPowerPercent TX power level in percent
 */
static bool SX126x_setTxPowerPercent(const uint8_t newPowerPercent);

/** @}*/
#endif
