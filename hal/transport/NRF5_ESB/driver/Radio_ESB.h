/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Frank Holtz
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list:
 * https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef __NRF5_ESB_H__
#define __NRF5_ESB_H__

#include "Radio.h"
#include <Arduino.h>

// Check maximum messae length
#if MAX_MESSAGE_LENGTH > (32)
#error "Unsupported message length. (MAX_MESSAGE_LENGTH)"
#endif

// check rx buffer size
#if MY_NRF5_ESB_RX_BUFFER_SIZE < (4)
#error "MY_NRF5_ESB_RX_BUFFER_SIZE must be greater than 3."
#endif

/** Wait for start of an ACK packet in us
 * Calculation: ramp up time + packet header (57 Bit): round to 9 Byte
 * If you don't receive ACK packages, you have to increase this value.
 * My measured (Arduino Uno + nRF24L01P) minimal timeouts:
 *   250kbit 411us -> 182 us to ACK start
 *   1MBit   205us -> 147 us
 *   2MBit   173us -> 143 us
 */
#define NRF5_ESB_ACK_WAIT                                                      \
	((NRF5_ESB_RAMP_UP_TIME << 1) + (9 << NRF5_ESB_byte_time()))

// auto retry delay in us, don't set this value < 1500us@250kbit
#define NRF5_ESB_ARD (1500)

// auto retry count with noACK is false
#define NRF5_ESB_ARC_ACK (15)

// auto retry count with noACK is true
#define NRF5_ESB_ARC_NOACK (3)

// How often broadcast messages are send
#define NRF5_ESB_BC_ARC (3)

// Node address index
#define NRF5_ESB_NODE_ADDR (0)
#define NRF5_ESB_NODE_ADDR_MSK (0xffffff00UL)

// TX address index
#define NRF5_ESB_TX_ADDR (4)
#define NRF5_ESB_TX_ADDR_MSK (0xffffff00UL)

// BC address index
#define NRF5_ESB_BC_ADDR (7)
#define NRF5_ESB_BC_ADDR_MSK (0xffffffffUL)

// Shorts for RX mode
#define NRF5_ESB_SHORTS_RX                                                     \
	(RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_START_Msk |                 \
	 RADIO_SHORTS_DISABLED_RXEN_Msk | RADIO_SHORTS_ADDRESS_BCSTART_Msk |         \
	 RADIO_SHORTS_ADDRESS_RSSISTART_Msk | RADIO_SHORTS_DISABLED_RSSISTOP_Msk)

// Shorts for TX mode
#define NRF5_ESB_SHORTS_TX                                                     \
	(RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_END_START_Msk |                 \
	 RADIO_SHORTS_DISABLED_TXEN_Msk | RADIO_SHORTS_ADDRESS_BCSTART_Msk)

// Shorts to switch from RX to TX
#define NRF5_ESB_SHORTS_RX_TX                                                  \
	(RADIO_SHORTS_END_DISABLE_Msk | RADIO_SHORTS_DISABLED_TXEN_Msk |             \
	 RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_BCSTART_Msk)

// Shorts to switch from TX to RX
#define NRF5_ESB_SHORTS_TX_RX                                                  \
	(RADIO_SHORTS_END_DISABLE_Msk | RADIO_SHORTS_DISABLED_RXEN_Msk |             \
	 RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_BCSTART_Msk |           \
	 RADIO_SHORTS_ADDRESS_RSSISTART_Msk | RADIO_SHORTS_DISABLED_RSSISTOP_Msk)

// PPI Channels for TX
#if (NRF5_RADIO_TIMER_IRQN != TIMER0_IRQn)
// Use two regular PPI channels
#define NRF5_ESB_PPI_TIMER_START 14
#define NRF5_ESB_PPI_TIMER_RADIO_DISABLE 15
#else
// Use one regular PPI channel and one predefined PPI channel
#define NRF5_ESB_USE_PREDEFINED_PPI
#define NRF5_ESB_PPI_TIMER_START 15
#define NRF5_ESB_PPI_TIMER_RADIO_DISABLE 22
#endif
#define NRF5_ESB_PPI_BITS                                                    \
	((1 << NRF5_ESB_PPI_TIMER_START) |                                         \
	 (1 << NRF5_ESB_PPI_TIMER_RADIO_DISABLE))

/** Bitcounter for Packet Control Field length
 * 6 Bits address length + 3 Bits S1 (NOACK + PID)
 */
#define NRF5_ESB_BITCOUNTER (9)

/** ramp up time
 * Time to activate radio TX or RX mode
 */
#define NRF5_ESB_RAMP_UP_TIME (140)


static bool NRF5_ESB_initialize();
static void NRF5_ESB_powerDown();
static void NRF5_ESB_powerUp();
static void NRF5_ESB_sleep();
static void NRF5_ESB_standBy();
static bool NRF5_ESB_sanityCheck();

static void NRF5_ESB_setNodeAddress(const uint8_t address);
static uint8_t NRF5_ESB_getNodeID();

static void NRF5_ESB_startListening();
static bool NRF5_ESB_isDataAvailable();
static uint8_t NRF5_ESB_readMessage(void *data);

static bool NRF5_ESB_sendMessage(uint8_t recipient, const void *buf, uint8_t len, const bool noACK);

static int16_t NRF5_ESB_getSendingRSSI();
static int16_t NRF5_ESB_getReceivingRSSI();

// Calculate time to transmit an byte in us as bit shift -> 2^X
static inline uint8_t NRF5_ESB_byte_time();

/** Structure of radio rackets
 */
typedef struct nrf5_radio_packet_s {
	// structure written by radio unit
	struct {
		uint8_t len;
		union {
			uint8_t s1;
			struct {
				uint8_t noack : 1;
				uint8_t pid : 2;
			};
		};
		uint8_t data[MAX_MESSAGE_LENGTH];
		int8_t rssi;
		/** Debug data structure */
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
		uint32_t rxmatch;
#endif
	}
#ifndef DOXYGEN
	__attribute__((packed));
#endif
} NRF5_ESB_Packet;

#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
static uint32_t intcntr_bcmatch;
static uint32_t intcntr_ready;
static uint32_t intcntr_end;
#endif

#endif // __NRF5_H__
