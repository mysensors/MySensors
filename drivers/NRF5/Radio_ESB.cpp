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

#include "Radio.h"
#include "Radio_ESB.h"
#include "hal/architecture/MyHwNRF5.h"
#include "drivers/CircularBuffer/CircularBuffer.h"
#include <stdio.h>

// internal functions
static uint8_t reverse_byte(uint8_t address);

// RX Buffer
static NRF5_ESB_Packet rx_circular_buffer_buffer[MY_NRF5_ESB_RX_BUFFER_SIZE];
// Poiter to rx circular buffer
static NRF5_ESB_Packet *rx_buffer;
// Circular buffer
static CircularBuffer<NRF5_ESB_Packet>
rx_circular_buffer(rx_circular_buffer_buffer, MY_NRF5_ESB_RX_BUFFER_SIZE);
// Dedect duplicate packages for every pipe available
static volatile uint32_t package_ids[8];

// ACK Buffer
static NRF5_ESB_Packet ack_buffer;

// TX Buffer
static NRF5_ESB_Packet tx_buffer;
// remaining TX retries
static volatile int8_t tx_retries;
// PID number for ACK
static volatile int8_t ack_pid;
// Flag for ack received
static volatile bool ack_received;
// Flag for end TX event
static volatile bool events_end_tx;
// Flag if radio is disabled
static volatile bool radio_disabled;
// Last RSSI sample provided by NRF5_ESB_readMessage
static volatile int16_t rssi_rx;
// Last RSSI sample by last package
static volatile int16_t rssi_tx;
// Buffer node address
static uint8_t node_address = 0;
// TX power level
static int8_t tx_power_level = (MY_NRF5_ESB_PA_LEVEL << RADIO_TXPOWER_TXPOWER_Pos);

// Initialize radio unit
static bool NRF5_ESB_initialize()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:INIT:ESB\n"));

	// Power on radio unit
	NRF_RADIO->POWER = 1;

	// Enable radio interrupt
	NVIC_SetPriority(RADIO_IRQn, 1);
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	// Enable timer interrupt
	NVIC_SetPriority(NRF5_RADIO_TIMER_IRQN, 2);
	NVIC_ClearPendingIRQ(NRF5_RADIO_TIMER_IRQN);
	NVIC_EnableIRQ(NRF5_RADIO_TIMER_IRQN);

	// Clear events
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_BCMATCH = 0;

	// Disable all interrupts
	NRF_RADIO->INTENCLR = (uint32_t)~0;

	// Select interrupt events
	NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk | RADIO_INTENSET_BCMATCH_Msk;

	// Configure radio parameters: tx power
	NRF_RADIO->TXPOWER = tx_power_level;

	// Configure radio parameters: radio channel
	NRF_RADIO->FREQUENCY = MY_NRF5_ESB_CHANNEL;

	// Configure radio parameters: data rate
	NRF_RADIO->MODE = MY_NRF5_ESB_MODE;

	// Configure radio parameters: CRC16
	NRF_RADIO->CRCCNF = (RADIO_CRCCNF_LEN_Two << RADIO_CRCCNF_LEN_Pos);
	NRF_RADIO->CRCINIT = 0xFFFFUL;
	NRF_RADIO->CRCPOLY = 0x11021UL;

	// Radio address config
	uint8_t address[MY_NRF5_ESB_ADDR_WIDTH] = {MY_NRF5_ESB_BASE_RADIO_ID};

	// Configure addresses
	NRF_RADIO->PREFIX0 = (NRF5_ESB_NODE_ADDR_MSK | reverse_byte(node_address) <<
	                      (NRF5_ESB_NODE_ADDR << 5));
	NRF_RADIO->BASE0 = reverse_byte(address[1]) << 24 |
	                   reverse_byte(address[2]) << 16 |
	                   reverse_byte(address[3]) << 8 | reverse_byte(address[4]);
	NRF_RADIO->BASE1 = reverse_byte(address[1]) << 24 |
	                   reverse_byte(address[2]) << 16 |
	                   reverse_byte(address[3]) << 8 | reverse_byte(address[4]);
	NRF_RADIO->PREFIX1 = NRF5_ESB_TX_ADDR_MSK; // Broadcast and send address

	// Enable listening on Node and BC address
	NRF_RADIO->RXADDRESSES = (1 << NRF5_ESB_NODE_ADDR) | (1 << NRF5_ESB_BC_ADDR);

	// Packet configuration for NRF24 compatibility
	NRF_RADIO->PCNF0 = (6 << RADIO_PCNF0_LFLEN_Pos) | // 6 Bits length field
	                   (0 << RADIO_PCNF0_S0LEN_Pos) | // No S0
#ifdef RADIO_PCNF0_S1INCL_Pos
	                   (1 << RADIO_PCNF0_S1INCL_Pos) | // Force include S1 in RAM
#endif
	                   (3 << RADIO_PCNF0_S1LEN_Pos); // 3 Bits S1 (NOACK and PID)

	// Packet configuration
	NRF_RADIO->PCNF1 =
	    (MAX_MESSAGE_LENGTH << RADIO_PCNF1_MAXLEN_Pos) | // maximum length
	    (0 << RADIO_PCNF1_STATLEN_Pos) |                 // minimum message length
	    ((MY_NRF5_ESB_ADDR_WIDTH - 1) << RADIO_PCNF1_BALEN_Pos) | // Set base address length
	    (RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos) | // Big endian
	    (RADIO_PCNF1_WHITEEN_Disabled << RADIO_PCNF1_WHITEEN_Pos); // Disable whitening

	// HINT: Fast ramp up can enabled here. Needs more code on other lines
	// Fast ramp up isn't supported by NRF24 and NRF51 series.

	// Set bitcounter to trigger interrupt after ACK bit
	NRF_RADIO->BCC = NRF5_ESB_BITCOUNTER;

#ifdef NRF51
	// Enable timer
	NRF5_RADIO_TIMER->POWER = 1;
#endif
	// Stop timer, if running
	NRF5_RADIO_TIMER->TASKS_STOP = 1;
	// Prepare timer running at 1 MHz/1us
	NRF5_RADIO_TIMER->PRESCALER = 4;
	// Timer mode
	NRF5_RADIO_TIMER->MODE = TIMER_MODE_MODE_Timer;
	// in 16 Bit mode
	NRF5_RADIO_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit << TIMER_BITMODE_BITMODE_Pos;
	// Stop timer when CC0 reached
	NRF5_RADIO_TIMER->SHORTS =
	    TIMER_SHORTS_COMPARE1_CLEAR_Msk | TIMER_SHORTS_COMPARE1_STOP_Msk;
	// Reset timer
	NRF5_RADIO_TIMER->TASKS_CLEAR = 1;
	// Reset compare events
	NRF5_RADIO_TIMER->EVENTS_COMPARE[0] = 0;
	NRF5_RADIO_TIMER->EVENTS_COMPARE[1] = 0;
	// Enable interrupt
	NRF5_RADIO_TIMER->INTENSET = TIMER_INTENSET_COMPARE0_Enabled << TIMER_INTENSET_COMPARE0_Pos;
	NRF5_RADIO_TIMER->INTENSET = TIMER_INTENSET_COMPARE1_Enabled << TIMER_INTENSET_COMPARE1_Pos;

	/** Configure PPI (Programmable peripheral interconnect) */
	// Start timer on END event
	NRF_PPI->CH[NRF5_ESB_PPI_TIMER_START].EEP = (uint32_t)&NRF_RADIO->EVENTS_END;
	NRF_PPI->CH[NRF5_ESB_PPI_TIMER_START].TEP = (uint32_t)&NRF5_RADIO_TIMER->TASKS_START;
	// Disable Radio after CC[0]
	NRF_PPI->CH[NRF5_ESB_PPI_TIMER_RADIO_DISABLE].EEP = (uint32_t)&NRF5_RADIO_TIMER->EVENTS_COMPARE[0];
	NRF_PPI->CH[NRF5_ESB_PPI_TIMER_RADIO_DISABLE].TEP = (uint32_t)&NRF_RADIO->TASKS_DISABLE;

	// Set internal variables
	radio_disabled = true;

	return true;
}

static void NRF5_ESB_powerDown()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:PD\n"));

	// Disable inerrupt
	NVIC_DisableIRQ(RADIO_IRQn);
	NVIC_DisableIRQ(NRF5_RADIO_TIMER_IRQN);

	// Clear PPI
	NRF_PPI->CHENCLR = NRF5_ESB_PPI_BITS;

	// Save power level
	tx_power_level = NRF_RADIO->TXPOWER;

	// Power off readio unit
	NRF_RADIO->POWER = 0;

	// Shutdown timer
	NRF5_RADIO_TIMER->TASKS_SHUTDOWN = 1;
#ifdef NRF51
	// Power off timer
	NRF5_RADIO_TIMER->POWER = 0;
#endif
}

static void NRF5_ESB_powerUp()
{
	// Not needed
}

static void NRF5_ESB_sleep()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:SLP\n"));
	NRF5_ESB_powerDown();
}

static void NRF5_ESB_standBy()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:SBY\n"));
	NRF5_ESB_initialize();
}

static bool NRF5_ESB_sanityCheck()
{
	// always true
	return true;
}

static void NRF5_ESB_setNodeAddress(const uint8_t address)
{
	node_address = address;
	NRF_RADIO->PREFIX0 = (NRF_RADIO->PREFIX0 & NRF5_ESB_NODE_ADDR_MSK) |
	                     reverse_byte(node_address) << (NRF5_ESB_NODE_ADDR << 5);
}

static uint8_t NRF5_ESB_getNodeID()
{
	return reverse_byte((NRF_RADIO->PREFIX0 & NRF5_ESB_NODE_ADDR_MSK) >> (NRF5_ESB_NODE_ADDR << 5));
}

static void NRF5_ESB_startListening()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:STL\n"));

	// Check if radio is initialized
	if (NRF_RADIO->POWER == 0) {
		NRF5_ESB_initialize();
	}

	// Enable Ready interrupt
	NRF_RADIO->INTENSET = RADIO_INTENSET_READY_Msk;

	// Enable RX when ready, Enable RX after disabling task
	NRF_RADIO->SHORTS = NRF5_ESB_SHORTS_RX;

	// Switch to RX
	if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {
		NRF_RADIO->TASKS_RXEN = 1;
	} else {
		NRF_RADIO->TASKS_DISABLE = 1;
	}

	radio_disabled = false;
}

static bool NRF5_ESB_isDataAvailable()
{
	return rx_circular_buffer.available() > 0;
}

static uint8_t NRF5_ESB_readMessage(void *data)
{
	// get content from rx buffer
	NRF5_ESB_Packet *buffer = rx_circular_buffer.getBack();
	// Nothing to read?
	if (buffer == NULL) {
		return 0;
	}

	// copy content
	memcpy(data, buffer->data, buffer->len);
	uint8_t ret = buffer->len;
	rssi_rx = 0-buffer->rssi;

	// Debug message
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
	NRF5_RADIO_DEBUG(PSTR("NRF5:RX:LEN=%d,NOACK=%d,PID=%d,RSSI=%d,RX=%d\n"),
	                 buffer->len, buffer->noack, buffer->pid, rssi_rx, buffer->rxmatch);
#endif

	// release buffer
	rx_circular_buffer.popBack();

	// Check if radio was disabled by buffer end
	if (radio_disabled == true) {
		// Wait until disabling radio is finised
		while (NRF_RADIO->EVENTS_DISABLED == 0)
			;
		// Enable radio
		NRF5_ESB_startListening();
	}

	return ret;
}

void NRF5_ESB_endtx();
void NRF5_ESB_starttx()
{
	if (tx_retries > 0) {
		// Prevent radio to write into TX memory while receiving
		if (NRF_RADIO->PACKETPTR != (uint32_t)&tx_buffer) {
			// Disable shorts
			NRF_RADIO->SHORTS = 0;
			// Disable radio
			NRF_RADIO->TASKS_DISABLE = 1;
		}

		// Mark TX as unfinised
		events_end_tx = false;

		// Configure TX address to address at index NRF5_ESB_TX_ADDR
		NRF_RADIO->TXADDRESS = NRF5_ESB_TX_ADDR;

		// Enable TX when ready, Enable TX after disabling task
		NRF_RADIO->SHORTS = NRF5_ESB_SHORTS_TX;

		// reset timer
		NRF_RESET_EVENT(NRF5_RADIO_TIMER->EVENTS_COMPARE[0]);
		NRF_RESET_EVENT(NRF5_RADIO_TIMER->EVENTS_COMPARE[1]);
		NRF5_RADIO_TIMER->TASKS_STOP = 1;
		NRF5_RADIO_TIMER->TASKS_CLEAR = 1;
		// Set retransmit time
		NRF5_RADIO_TIMER->CC[1] = NRF5_ESB_ARD - NRF5_ESB_RAMP_UP_TIME;
		// Set radio disable time to ACK_WAIT time
		NRF5_RADIO_TIMER->CC[0] = NRF5_ESB_ACK_WAIT;

		// Set PPI
		NRF_PPI->CHENSET = NRF5_ESB_PPI_BITS;

		// Disable Ready interrupt
		NRF_RADIO->INTENCLR = RADIO_INTENSET_READY_Msk;

		// Set buffer
		NRF_RADIO->PACKETPTR = (uint32_t)&tx_buffer;

		// Switch to TX
		if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {
			NRF_RADIO->TASKS_TXEN = 1;
		} else {
			NRF_RADIO->TASKS_DISABLE = 1;
		}
	} else {
		// finised TX
		NRF5_ESB_endtx();
	}
	tx_retries--;
}

void NRF5_ESB_endtx()
{
	// Clear PPI
	NRF_PPI->CHENCLR = NRF5_ESB_PPI_BITS;
	// Enable Ready interrupt
	NRF_RADIO->INTENSET = RADIO_INTENSET_READY_Msk;
	// Stop Timer
	NRF5_RADIO_TIMER->TASKS_STOP = 1;
	// Shutdown timer
	NRF5_RADIO_TIMER->TASKS_SHUTDOWN = 1;
	// Mark TX as end
	events_end_tx = true;
	// Debug output
#ifdef NRF5_ESB_DEBUG_INT_TX_END
	NRF5_RADIO_DEBUG(PSTR("NRF5:INT:ENDTX\n"));
#endif
}

static bool NRF5_ESB_sendMessage(uint8_t recipient, const void *buf, uint8_t len, const bool noACK)
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:SND:TO=%d,LEN=%d,PID=%d,NOACK=%d\n"), recipient, len, tx_buffer.pid,
	                 tx_buffer.noack); // send message
	// Check if radio is initialized
	if (NRF_RADIO->POWER == 0) {
		NRF5_ESB_initialize();
	}

	// check length and truncate data
	if (len > MAX_MESSAGE_LENGTH) {
		len = MAX_MESSAGE_LENGTH;
	}

	// copy data to tx_buffer
	memcpy(&tx_buffer.data[0], buf, len);

	// build metadata
	tx_buffer.len = len;
#ifndef MY_NRF5_ESB_REVERSE_ACK
	tx_buffer.noack = noACK || recipient==BROADCAST_ADDRESS;
#else
	// reverse the noack bit
	tx_buffer.noack = !(noACK || recipient==BROADCAST_ADDRESS);
#endif
	tx_buffer.pid++;

	// Calculate number of retries
	tx_retries = ((recipient == BROADCAST_ADDRESS)?(NRF5_ESB_BC):(NRF5_ESB_ARC));
	int8_t tx_retries_start = tx_retries;

	// This line disables sending a broadcast packet multiple times
	// ack_received = (recipient == BROADCAST_ADDRESS);
	// This line force to send an boradcast packet multiple times like NRF24
	ack_received = tx_buffer.noack;

	// configure TX address
	NRF_RADIO->PREFIX1 = (NRF_RADIO->PREFIX1 & NRF5_ESB_TX_ADDR_MSK) |
	                     (reverse_byte(recipient) << (NRF5_ESB_TX_ADDR - 4));

	// Enable listening on Node, BC and TX address
	NRF_RADIO->RXADDRESSES = (1 << NRF5_ESB_NODE_ADDR) | (1 << NRF5_ESB_BC_ADDR) |
	                         (1 << NRF5_ESB_TX_ADDR);

	// Set RSSI to invalid
	rssi_tx = INVALID_RSSI;

	NRF5_ESB_starttx();

	// Wait for end of transmission
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
	uint32_t wakeups = 0;
#endif
	while (events_end_tx == false) {
		// Power off CPU until next interrupt
		hwSleep();
		// hwWaitForInterrupt();
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
		wakeups++;
#endif
	}

	// Enable listening on Node and BC address
	NRF_RADIO->RXADDRESSES = (1 << NRF5_ESB_NODE_ADDR) | (1 << NRF5_ESB_BC_ADDR);

	// Calculate RSSI
	if (rssi_tx == INVALID_RSSI) {
		// calculate pseudo-RSSI based on retransmission counter (ARC)
		// min -104dBm at 250kBps
		// Arbitrary definition: ARC 0 == -29, ARC 15 = -104
		rssi_tx = (-29 - (8 * (tx_retries_start - tx_retries)));
	}

#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
	NRF5_RADIO_DEBUG(PSTR("NRF5:SND:END=%d,ACK=%d,RTRY=%d,RSSI=%d,WAKE=%d\n"),
	                 events_end_tx, ack_received, tx_retries_start - tx_retries, rssi_tx, wakeups);
#endif

	return ack_received;
};

static int16_t NRF5_ESB_getSendingRSSI()
{
	return rssi_tx;
}

static int16_t NRF5_ESB_getReceivingRSSI()
{
	return rssi_rx;
}

/*
 * Internal helper functions
 */

// Reverse a byte for address
static uint8_t reverse_byte(uint8_t address)
{
#if __CORTEX_M >= (0x01U)
	return __REV(__RBIT(address));
#else
	address = ((address * 0x0802LU & 0x22110LU) | (address * 0x8020LU & 0x88440LU)) * 0x10101LU >> 16;
#endif
	return address;
}

// Calculate time to transmit an byte in us as bit shift -> 2^X
static inline uint8_t NRF5_ESB_byte_time()
{
	if ((MY_NRF5_ESB_MODE == NRF5_1MBPS) or
	        (MY_NRF5_ESB_MODE == NRF5_BLE_1MBPS)) {
		return (3);
	} else if (MY_NRF5_ESB_MODE == NRF5_2MBPS) {
		return (2);
	} else if (MY_NRF5_ESB_MODE == NRF5_250KBPS) {
		return (5);
	}
}

extern "C" {
	/** Radio Interrupt handler */
	void RADIO_IRQHandler()
	{
		/** Bitcounter event is used to switch between RX/TX
		 * In RX mode, when an ACK required packet is received, switch to TX,
		 * elsewhere start RX again.
		 * In TX mode switch always to RX.
		 */
		if (NRF_RADIO->EVENTS_BCMATCH == 1) {
			NRF_RESET_EVENT(NRF_RADIO->EVENTS_BCMATCH);

			// Disable bitcounter
			NRF_RADIO->TASKS_BCSTOP = 1;

			// In RX mode -> prepare ACK or RX
			if (NRF_RADIO->STATE == RADIO_STATE_STATE_Rx) {
				// ACK only for node address and unset noack bit
#ifndef MY_NRF5_ESB_REVERSE_ACK
				if ((NRF_RADIO->RXMATCH != NRF5_ESB_NODE_ADDR) || (rx_buffer->noack)) {
#else
				if ((NRF_RADIO->RXMATCH != NRF5_ESB_NODE_ADDR) || (!rx_buffer->noack)) {
#endif
					// Handle incoming ACK packet
					if (NRF_RADIO->RXMATCH == NRF5_ESB_TX_ADDR) {
						/** Calculate time to switch radio off
						 * This is an ACK packet, the radio is disabled by Timer
						 * event after CC[0], calculate the time switching of the
						 * radio.
						 */
						// Read current timer value
						NRF5_RADIO_TIMER->TASKS_CAPTURE[0] = 1;

						// Set Timer compare register 0 to end of packet (len+CRC)
						NRF5_RADIO_TIMER->CC[0] += ((rx_buffer->len + 3) << NRF5_ESB_byte_time());
#if __CORTEX_M == 0x04
						// read back to clear cache
						(void)NRF5_RADIO_TIMER->CC[0];
#endif
					}
					// No ACK -> Start RX after END
					NRF_RADIO->SHORTS = NRF5_ESB_SHORTS_RX;
				} else {
					// Send ACK after END, an empty packet is provided in READY event
					NRF_RADIO->SHORTS = NRF5_ESB_SHORTS_RX_TX;
					// HINT: Fast ramp must be disabled for every device with unknown
					// support (payload). Needs more code on other lines
				}
			} else {
				// Current mode is TX:
				// After TX the Radio has to be always in RX mode to
				// receive ACK or start implicit listen mode after send.
				NRF_RADIO->SHORTS = NRF5_ESB_SHORTS_TX_RX;
				// HINT: Fast ramp up can enabled here. Needs more code on other lines
			}
		}

		/** Ready event is generated before RX starts
		 * An free rx buffer is allocated or radio is disabled on failures
		 */
		if (NRF_RADIO->EVENTS_READY == 1) {
			NRF_RESET_EVENT(NRF_RADIO->EVENTS_READY);
			// Fetch a new buffer
			rx_buffer = rx_circular_buffer.getFront();

			// Is buffer valid?
			if (rx_buffer == NULL) {
				// No buffer -> disable listening
				radio_disabled = true;
				NRF_RADIO->SHORTS = 0;
				NRF_RADIO->EVENTS_DISABLED = 0;
				NRF_RADIO->TASKS_DISABLE = 1;
				return;
			}

			// Configure DMA target address
			NRF_RADIO->PACKETPTR = (uint32_t)rx_buffer;

			/* Don't care about if next packet RX or ACK,
			 * prepare current rx_buffer to send an ACK */

			// Set outgoing address to node address for ACK packages
			NRF_RADIO->TXADDRESS = NRF5_ESB_NODE_ADDR;

			// HINT: Fast ramp up: Add a fast ramp up signaling payload here. Needs more
			// code on other lines
		}

		/** This event is generated after TX or RX finised
		 */
		if (NRF_RADIO->EVENTS_END == 1) {
			NRF_RESET_EVENT(NRF_RADIO->EVENTS_END);

			// Enable ACK bitcounter for next packet
			NRF_RADIO->BCC = NRF5_ESB_BITCOUNTER;

			// End of RX packet
			if ((NRF_RADIO->STATE == RADIO_STATE_STATE_Rx) or
			        (NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle) or
			        (NRF_RADIO->STATE == RADIO_STATE_STATE_RxDisable) or
			        (NRF_RADIO->STATE == RADIO_STATE_STATE_TxRu)) {
				if (NRF_RADIO->CRCSTATUS) {
					// Ensure no ACK package is recieved
					if (NRF_RADIO->RXMATCH != NRF5_ESB_TX_ADDR) {
						// calculate a package id
						uint32_t pkgid = rx_buffer->pid << 16 | NRF_RADIO->RXCRC;
						if (pkgid != package_ids[NRF_RADIO->RXMATCH]) {
							// correct package -> store id to dedect duplicates
							package_ids[NRF_RADIO->RXMATCH] = pkgid;
							rx_buffer->rssi = ack_buffer.data[0] = NRF_RADIO->RSSISAMPLE;
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
							// Store debug data
							rx_buffer->rxmatch = NRF_RADIO->RXMATCH;
#endif
							// Push data to buffer
							rx_circular_buffer.pushFront(rx_buffer);

							// Prepare ACK package
							ack_buffer.pid++;
							ack_buffer.len=1; // data[0] is set some lines before
#ifndef MY_NRF5_ESB_REVERSE_ACK
							ack_buffer.noack = 1;
#else
							ack_buffer.noack = 0;
#endif

							// Set pointer to ACK packet
							NRF_RADIO->PACKETPTR = (uint32_t)&ack_buffer;
						}
					} else {
						// ACK package received, ducplicates are accepted

						// rssi value in ACK included?
						if (rx_buffer->len == 1) {
							rssi_tx = 0-rx_buffer->data[0];
						}
						// notify TX process
						ack_received = true;
						// End TX
						NRF5_ESB_endtx();
					}
				} else {
					/** Invalid CRC -> Switch back to RX, Stop sending ACK */

					// Enable RX when ready, Enable RX after disabling task
					NRF_RADIO->SHORTS = NRF5_ESB_SHORTS_RX;

					// Start disabling radio -> switch to rx by shorts
					NRF_RADIO->TASKS_DISABLE = 1;
				}
			} else {
				// TX end
			}
		}
	}

	/** Timer Interrupt Handler
	 *  This timer is used to handle TX retransmit timing
	 *
	 */
	void NRF5_RADIO_TIMER_IRQ_HANDLER()
	{
		if (NRF5_RADIO_TIMER->EVENTS_COMPARE[0] == 1) {
			NRF_RESET_EVENT(NRF5_RADIO_TIMER->EVENTS_COMPARE[0]);
		}

		if (NRF5_RADIO_TIMER->EVENTS_COMPARE[1] == 1) {
			NRF5_RADIO_TIMER->TASKS_STOP = 1;
			NRF_RESET_EVENT(NRF5_RADIO_TIMER->EVENTS_COMPARE[1]);
			if (ack_received == false) {
				// missing ACK, start TX again
				NRF5_ESB_starttx();
			} else {
				// finised TX
				NRF5_ESB_endtx();
			}
		}
	}
} // extern "C"
