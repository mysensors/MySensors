/**
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
 * https://github.com/mysensors/Arduino/graphs/contributors
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

// Use timer 0 for predefined PPI for timer0
#define NRF5_RADIO_TIMER NRF_TIMER0
#define NRF5_RADIO_TIMER_IRQ_HANDLER TIMER0_IRQHandler
#define NRF5_RADIO_TIMER_IRQN TIMER0_IRQn

// To support other timers. Implement predefined PPI.
// Option to force timer capture instead of PPI to save PPI channels
//#define NRF5_ESB_DO_CAPTURE

#ifdef RADIO_MODECNF0_RU_Fast
// Enable fast ramp up
#define NRF5_ESB_FAST_RU
#endif

/* Radio timing parameters */
// Time (µS) before start TX(NRF51)
#define NRF5_ESB_PRE_TX_TIME_SLOW_RAMPUP (8)
// Time (µS) before start TX with fast rampup enabled (NRF52)
#define NRF5_ESB_PRE_TX_TIME_FAST_RAMPUP (88)
// Time (µS) before start RX after TX (NRF52)
#define NRF5_ESB_PRE_RX_TIME_FAST_RAMPUP (85)
// Time (µS) before start TX after RX aith ACK (NRF52)
#define NRF5_ESB_POST_RX_TIME_FAST_RAMPUP (NRF5_ESB_PRE_TX_TIME_FAST_RAMPUP)
// auto retry delay in us, don't set this value < 1500us@250kbit
#define NRF5_ESB_ARD (1500)
// Time to wait for ACK messages
#define NRF5_ESB_ACK_WAIT (260)
// auto retry count with noACK is false
#define NRF5_ESB_ARC_ACK (15)
// auto retry count with noACK is true
#define NRF5_ESB_ARC_NOACK (3)
// How often broadcast messages are send
#define NRF5_ESB_BC_ARC (3)
// RX timeout for NRF52 PAN#102 and for _disableRadio
#define NRF5_ESB_RX_TIMEOUT (1500)
// Timeout to stop TX (Bughandler, optional)
//#define NRF5_ESB_TX_BUG_WORKAROUND

// Node address index
#define NRF5_ESB_NODE_ADDR (0)
#define NRF5_ESB_NODE_ADDR_MSK (0xffffff00UL)
// TX address index
#define NRF5_ESB_TX_ADDR (4)
#define NRF5_ESB_TX_ADDR_MSK (0xffffff00UL)
// BC address index
#define NRF5_ESB_BC_ADDR (7)
#define NRF5_ESB_BC_ADDR_MSK (0xffffffffUL)

// PPI Channels
#ifdef NRF51
#define NRF5_ESB_PPI_CHANNEL_START (14)
#define NRF5_ESB_PPI_BITMASK (0xFFF00000) | (1 << NRF5_ESB_PPI_CHANNEL_START) | (1 << (NRF5_ESB_PPI_CHANNEL_START+1))
#else
#define NRF5_ESB_PPI_CHANNEL_START (18)
#define NRF5_ESB_PPI_BITMASK (0xFFF00000) | (1 << NRF5_ESB_PPI_CHANNEL_START) | (1 << (NRF5_ESB_PPI_CHANNEL_START+1))
#endif
#define NRF5_ESB_PPI_BITS (0)

/** @brief Use LEDs for debug output
 *
 * Debug LEDs
 *	PIN_LED2 = Radio READY Event
 *  PIN_LED3 = Radio ADDRESS Event
 *  PIN_LED4 = Radio END Event
 *  PIN_LED5 = Radio DISABLED Event
 *  PIN_LED6 = In RX/TX_ACK (READY/ADDRESS..END/DISABLED)
 *  PIN_LED7 = TX_ACK: TX
 *  PIN_LED8 = TX_ACK: RX
 *
 */
//#define NRF5_ESB_ENABLE_DEBUG_LEDS

// Define routine to enable/disable LED's
#ifdef NRF5_ESB_ENABLE_DEBUG_LEDS
#define NRF5_ESB_DEBUG_LED(x,y) digitalWrite(x,y)
#else
#define NRF5_ESB_DEBUG_LED(x,y)
#endif

/**
 * @brief SM state
 *
 * This structure stores SM state definitions
 */
typedef struct {
	size_t radioEvents;         //!< radio events to enable
	size_t radioShorts;         //!< radio short cuts to enable
	size_t rxAddresses;         //!< radio addresses to listen
	size_t txAddress;           //!< radio address as TX destination
	void(*radioReady)(void);	  //!< radio READY event function
	void(*radioAddress)(void);	//!< radio ADDRESS event function
	void(*radioEnd)(void);    	//!< radio END event function
	void(*radioDisabled)(void);	//!< radio ADDRESS event function
	size_t timerShorts;         //!< timer short cuts to enable
	void(*ppiInit)(void);				//!< initialize PPI
} esbState_t;


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

/**
 * @brief Structure of radio rackets
 *
 * This structure stores radio packets
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
	}
#ifndef DOXYGEN
	__attribute__((packed));
#endif
} NRF5_ESB_Packet;

/**
 * @brief States of common radio buffer
 *
 * For what is the buffer used.
 */
typedef enum {
	NRF5_ESB_BUFFER_STATE_DISABLED,
	NRF5_ESB_BUFFER_STATE_TX,
	NRF5_ESB_BUFFER_STATE_RX,
	NRF5_ESB_BUFFER_STATE_TX_ACK,
	NRF5_ESB_BUFFER_STATE_RX_ACK,
} nrf_esb_buffer_states;

#endif // __NRF5_H__
