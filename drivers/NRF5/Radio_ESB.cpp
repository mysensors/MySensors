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


/*
 * This is the implementation of the Enhanced Shockburst (ESB) protocol.
 * Most activity is driven by Shortcuts and PPI.
 * This code is not compatible with SoftDevices because of sharing resources
 * like Timer0 and PPI.
 *
 * TX Mode
 * =======
 *
 * TX (noACK)flow:
 * 1   2    TX    3                                        4
 * |---|----------|----------------------------------------|
 *
 * TX (ACK)flow:
 * 1   2    TX    3   5 RX 6  (RX) 7      8                4
 * |---|----------|---|----|-------|------|----------------|
 *
 * 1 = Start timer
 * 2 = TXEN via CC[3] and PPI[+0] (8µS NRF51 / 100µS NRF52 with fast ramp up)
 * 3 = TX DISABLED Event -> Disable Radio (Shortcurt)
 *     - ACK NRF51: Start RX via radio shortcut  |5|
 *     - ACK NRF52: Calculate CC[0]=CC[2]+80µS via PPI[27] and *radioDisabled |5|
 * 		- ACK+noACK: Caluclate CC[2]+ARD via PPI[27] |4|
 *     - ACK: Calculate CC[1] to disable RX after NRF5_ESB_ACK_WAIT of listening (*radioReady) |7|
 * 4 = CC[2] Event at Auto Retry Delay (ARD)-> Clear Timer at CC[2] (Shortcut)
 * 5 = RXEN (NRF52) via CC[0] and PPI[21]; NRF51: DISABLED->RXEN Shortcut (NRF51)
 * 6 = ADDRESS Event -> Caluclate CC[1] via PPI[27] |8|
 * 7 = CC[1] without ADDRESS event -> disable radio
 * 8 = CC[1] with ADDRESS event -> disable radio
 *
 * Timer Events
 * - CC[0]: Enable RX (ACK + noACK) |5|
 * - CC[1]: Disable radio (noACK) |7,8|
 * - CC[2]: Clear timer. Restart TX process. (ACK + noACK)
 * - CC[3]: Enable TX (ACK + noACK)
 * - CC[4]: NRF52 Disable radio (reset, when tx is hanging)
 *
 * RX Mode
 * =======
 *
 * RX (noACK)flow:
 * 1 2  3  RX 4    5
 * |-|--|-----|----|
 *
 * RX (ACK)flow:
 * 1 2  3  RX 4 6 7 TX 8
 * |-|--|-----|-|-|----|
 *
 * 1 = Start RX
 * 2 = Ready event
 *     - Configure DMA Memory (rx/ack buffer)
 *     - Configure Shortcuts
 * 3 = ADDRESS EVENT (RX)
 *     - NRF52: Calculate CC[1]
 * 		 - NRF52: Start Timer via PPI (PAN#102)
 * 4 = END Event
 *     - Prepare shortcut for next radio mode (RX/TX)
 *     - ACK: prepare ACK Payload
 *     - ACK NRF52: Calculate CC[0]=CC[2]+80µS via PPI[27] to start TX |6|
 *     - NoACK NRF52: Stop Timer
 * 5 = Restart radio via CC[1], PPI[21]; CC[1]->Timer Clear+Stop; (PAN#102)
 * 6 = CC[0] -> TX -> Start TX; CC[0]->Timer Clear+Stop;
 * 7 = TX Ready:
 *     - Configure Shortcut END -> DISABLE -> RX
 * 8 = END event; Switch back to RX via Shortcut
 *
 * Timer Events
 * - CC[0]: Enable TX (ACK) |6|
 * - CC[1]: Disable radio (ACK, PAN#102) |5|
 */

#include "Radio.h"
#include "Radio_ESB.h"
#include "hal/architecture/NRF5/MyHwNRF5.h"
#include "drivers/CircularBuffer/CircularBuffer.h"
#include <stdio.h>

/*
 *  State Machine
 */

// PRIVATE state machine functions
void stEnter(esbState_t* state);
void stNull(void);

void stTxPPI(void);
void stTxEnd(void);
void stTxDisabled(void);

void stTXAckPPI(void);
void stTxAckReady(void);
void stTxAckAddress(void);
void stTxAckEnd(void);
void stTxAckDisabled(void);

void stRxPPI(void);

void stRxReady(void);
void stRxAddress(void);
void stRxEnd(void);
void stRxDisabled(void);

void stNoPPI(void);

// States:

// Radio disabled
static esbState_t stDisabled = {0, 0, (1 << NRF5_ESB_NODE_ADDR) | (1 << NRF5_ESB_BC_ADDR), 0, stNull, stNull, stNull, stNull, 0,  stNoPPI};

// Radio in RX mode listening on node and BC address
static esbState_t stRx       = {
#if defined(NRF52) or defined(NRF5_ESB_ENABLE_DEBUG_LEDS)
	// Address is required for timer control (PAN#102)
	(RADIO_INTENSET_READY_Msk | RADIO_INTENSET_ADDRESS_Msk | RADIO_INTENSET_END_Msk), // Radio events
#else
	(RADIO_INTENSET_READY_Msk | RADIO_INTENSET_END_Msk), // Radio events
#endif
	(RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_ADDRESS_RSSISTART_Msk | RADIO_SHORTS_DISABLED_RSSISTOP_Msk), // Common radio shortcuts for RX and RX+ACK
	((1 << NRF5_ESB_NODE_ADDR) | (1 << NRF5_ESB_BC_ADDR)), // Listen on node address + broadcast address
	NRF5_ESB_NODE_ADDR, // TX address
	stRxReady,
	stRxAddress,
	stRxEnd,
#ifdef NRF52
	stRxDisabled,
#else
	stNull,
#endif
	((TIMER_SHORTS_COMPARE0_CLEAR_Enabled << TIMER_SHORTS_COMPARE0_CLEAR_Pos) | (TIMER_SHORTS_COMPARE0_STOP_Enabled << TIMER_SHORTS_COMPARE0_STOP_Pos) | (TIMER_SHORTS_COMPARE1_CLEAR_Enabled << TIMER_SHORTS_COMPARE1_CLEAR_Pos) | (TIMER_SHORTS_COMPARE1_STOP_Enabled << TIMER_SHORTS_COMPARE1_STOP_Pos)),
	stRxPPI 		// setup PPI
};

// Radio in TX mode
static esbState_t stTx = {
#ifdef NRF5_ESB_DO_CAPTURE
	(RADIO_INTENSET_END_Msk | RADIO_INTENSET_DISABLED_Msk), // Radio events
#else
	(RADIO_INTENSET_DISABLED_Msk), // Radio events
#endif
	(RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos | RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos), // Radio shortcuts
	0, // No listening
	NRF5_ESB_TX_ADDR, // TX address
	stNull,
	stNull,
	stTxEnd,
	stTxDisabled,
	((TIMER_SHORTS_COMPARE2_CLEAR_Enabled << TIMER_SHORTS_COMPARE2_CLEAR_Pos)),
	stTxPPI 		// setup PPI
};
// Radio in TX mode switching to receive ACK messages after TX
static esbState_t stTxAck = {
	(RADIO_INTENSET_READY_Msk | RADIO_INTENSET_ADDRESS_Msk | RADIO_INTENSET_END_Msk | RADIO_INTENSET_DISABLED_Msk), // Radio events
	(RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos | RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos), // Radio shortcuts
	(1 << NRF5_ESB_TX_ADDR),	// Only listen on TX Address for ACK receive
	NRF5_ESB_TX_ADDR, // TX address
	stTxAckReady, // READY event
	stTxAckAddress, // ADDRESS event
	stTxAckEnd, // END event
	stTxAckDisabled, // DISABLED event
	((TIMER_SHORTS_COMPARE2_CLEAR_Enabled << TIMER_SHORTS_COMPARE2_CLEAR_Pos)),
	stTXAckPPI 		// setup PPI
};
//static esbState_t stTxAck    = {0, 0, (1 << NRF5_ESB_TX_ADDR), stNull, stNull, stNull, stNull, stNull, 0, 0, stNull, stTxTimer, stNoPPI};
// Predefine a state
static esbState_t* currentState = &stDisabled;

// internal functions
void _disableRadio();
inline void _stopTimer();
void _microDelay(uint32_t us);
static uint8_t reverse_byte(uint8_t address);

// RX Buffer
static NRF5_ESB_Packet rx_circular_buffer_buffer[MY_NRF5_ESB_RX_BUFFER_SIZE];
// Circular buffer
static CircularBuffer<NRF5_ESB_Packet> rx_circular_buffer(rx_circular_buffer_buffer,
        MY_NRF5_ESB_RX_BUFFER_SIZE);
// Dedect duplicate packages for every pipe available
static volatile uint32_t last_package_id;
// Buffer for radio unit
static NRF5_ESB_Packet radio_buffer;
// remaining TX retries
static volatile int8_t tx_retries;
// Flag for ack received
static volatile bool ack_received;
// Flag for end TX event
static volatile bool events_end_tx;
// Last RSSI sample provided by NRF5_ESB_readMessage
static volatile int16_t rssi_rx;
// Last RSSI sample by last package
static volatile int16_t rssi_tx;
// Buffer node address
static uint8_t node_address = 0;
// TX power level
static int8_t tx_power_level = (MY_NRF5_ESB_PA_LEVEL << RADIO_TXPOWER_TXPOWER_Pos);
// PID for outgoing packages
static uint8_t tx_pid;
// Buffer state
static uint8_t radio_buffer_state;

// Initialize radio unit
static bool NRF5_ESB_initialize()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:INIT:ESB\n"));

	// Power on radio unit
	NRF_RADIO->POWER = 1;

	// Disable shorts
	NRF_RADIO->SHORTS = 0;

	// Disable radio
	NRF_RADIO->TASKS_DISABLE = 1;

	// Enable radio interrupt
	NVIC_SetPriority(RADIO_IRQn, 1);
	NVIC_ClearPendingIRQ(RADIO_IRQn);
	NVIC_EnableIRQ(RADIO_IRQn);

	// Enable timer interrupt
	NVIC_SetPriority(NRF5_RADIO_TIMER_IRQN, 1);
	NVIC_ClearPendingIRQ(NRF5_RADIO_TIMER_IRQN);
	NVIC_DisableIRQ(NRF5_RADIO_TIMER_IRQN);

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
	NRF_RADIO->PREFIX1 = NRF5_ESB_TX_ADDR_MSK; // Broadcast and send address
	NRF_RADIO->BASE0 = reverse_byte(address[1]) << 24 |
	                   reverse_byte(address[2]) << 16 |
	                   reverse_byte(address[3]) << 8 | reverse_byte(address[4]);
	NRF_RADIO->BASE1 = NRF_RADIO->BASE0;

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

	// Enable fast ramp up for controllers supporting this feature >NRF51
#ifdef NRF5_ESB_FAST_RU
	NRF_RADIO->MODECNF0 = ( RADIO_MODECNF0_RU_Fast << RADIO_MODECNF0_RU_Pos);
#endif
#ifdef RADIO_MODECNF0_DTX_B1
	NRF_RADIO->MODECNF0 |= (RADIO_MODECNF0_DTX_B1 << RADIO_MODECNF0_DTX_Pos);
#endif

	// Radio buffer storing packages
	NRF_RADIO->PACKETPTR = (uint32_t)&radio_buffer;

#ifdef NRF51
	// Enable timer
	NRF5_RADIO_TIMER->POWER = 1;
#endif
	// Stop timer, if running
	_stopTimer();
	// Prepare timer running at 1 MHz/1us
	NRF5_RADIO_TIMER->PRESCALER = 4;
	// Timer mode
	NRF5_RADIO_TIMER->MODE = TIMER_MODE_MODE_Timer;
	// in 16 Bit mode
	NRF5_RADIO_TIMER->BITMODE = TIMER_BITMODE_BITMODE_32Bit << TIMER_BITMODE_BITMODE_Pos;
	// Disable shorts
	NRF5_RADIO_TIMER->SHORTS = 0;

	// Enter disabled state
	stEnter(&stDisabled);

	// Initialize PID for TX packages to a random value
	hwGetentropy(&tx_pid, 1);

#ifdef	NRF5_ESB_ENABLE_DEBUG_LEDS
	hwPinMode(PIN_LED2, OUTPUT_H0H1);
	hwPinMode(PIN_LED3, OUTPUT_H0H1);
	hwPinMode(PIN_LED4, OUTPUT_H0H1);
	hwPinMode(PIN_LED5, OUTPUT_H0H1);
	hwPinMode(PIN_LED6, OUTPUT_H0H1);
	hwPinMode(PIN_LED7, OUTPUT_H0H1);
	hwPinMode(PIN_LED8, OUTPUT_H0H1);
#endif

	return true;
}

static void NRF5_ESB_powerDown()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:PD\n"));

	// Enter disabled state
	stEnter(&stDisabled);

	// Disable inerrupt
	NVIC_DisableIRQ(RADIO_IRQn);
	NVIC_DisableIRQ(NRF5_RADIO_TIMER_IRQN);

	// Save power level
	tx_power_level = NRF_RADIO->TXPOWER;

	// Power off readio unit
	NRF_RADIO->POWER = 0;

	// Stop timer
	_stopTimer();
#ifdef NRF51
	// Power off timer
	NRF5_RADIO_TIMER->POWER = 0;
#endif
}

static void NRF5_ESB_powerUp()
{
	NRF5_ESB_initialize();
}

static void NRF5_ESB_sleep()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:SLP\n"));

	// Enter disabled state
	stEnter(&stDisabled);

	// Disable shorts
	NRF_RADIO->SHORTS = 0;

	// Disable radio
	NRF_RADIO->TASKS_DISABLE = 1;
}

static void NRF5_ESB_standBy()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:SBY\n"));
	NRF5_ESB_startListening();
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
	return node_address;
}

static void NRF5_ESB_startListening()
{
	NRF5_RADIO_DEBUG(PSTR("NRF5:STL\n"));

	// Check if radio is initialized
	if (NRF_RADIO->POWER == 0) {
		NRF5_ESB_initialize();
	}

	// Mark buffer
	radio_buffer_state = NRF5_ESB_BUFFER_STATE_DISABLED;

	// Enter RX state
	stEnter(&stRx);

	// Reset last received package id
	last_package_id = 0;

	// Switch to RX
	if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {
		NRF_RADIO->TASKS_RXEN = 1;
	} else {
		NRF_RADIO->TASKS_DISABLE = 1;
	}
}

static bool NRF5_ESB_isDataAvailable()
{
	return rx_circular_buffer.available() > 0;
}

static uint8_t NRF5_ESB_readMessage(void *data)
{
	uint8_t ret = 0;

	// get content from rx buffer
	NRF5_ESB_Packet *buffer = rx_circular_buffer.getBack();
	// Nothing to read?
	if (buffer != NULL) {
		// copy content
		memcpy(data, buffer->data, buffer->len);
		ret = buffer->len;
		rssi_rx = 0-buffer->rssi;

		// Debug message
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
		NRF5_RADIO_DEBUG(PSTR("NRF5:RX:LEN=%" PRIu8 ",NOACK=%" PRIu8 ",PID=%" PRIu8 ",RSSI=%" PRIi16 "\n"),
		                 buffer->len, buffer->noack, buffer->pid, rssi_rx);
#endif

		// release buffer
		rx_circular_buffer.popBack();
	}

	// Check if radio was disabled by buffer end
	if (currentState != &stRx) {
		// Enable radio
		NRF5_ESB_startListening();
	}

	return ret;
}

static bool NRF5_ESB_sendMessage(uint8_t recipient, const void *buf, uint8_t len, const bool noACK)
{
	// Check if radio is initialized
	if (NRF_RADIO->POWER == 0) {
		NRF5_ESB_initialize();
	}

	// check length and truncate data
	if (len > MAX_MESSAGE_LENGTH) {
		len = MAX_MESSAGE_LENGTH;
	}

	// copy data to radio_buffer
	memcpy(&radio_buffer.data[0], buf, len);

	// build metadata
	radio_buffer.len = len;
	radio_buffer.noack = noACK || recipient==BROADCAST_ADDRESS;
	tx_pid++;
	radio_buffer.pid=tx_pid;

	NRF5_RADIO_DEBUG(PSTR("NRF5:SND:TO=%" PRIu8 ",LEN=%" PRIu8 ",PID=%" PRIu8 ",NOACK=%" PRIu8 "\n"),
	                 recipient, len, radio_buffer.pid,
	                 radio_buffer.noack); // send message

	// Calculate number of retries
	if (recipient == BROADCAST_ADDRESS) {
		tx_retries = NRF5_ESB_BC_ARC;
	} else {
		tx_retries = ((noACK == false)?(NRF5_ESB_ARC_ACK):(NRF5_ESB_ARC_NOACK));
	}
	int8_t tx_retries_start = tx_retries;

	// Mark ACK as missing
	ack_received = false;

	// Mark TX as unfinised
	events_end_tx = false;

	// configure TX address
	NRF_RADIO->PREFIX1 = (NRF_RADIO->PREFIX1 & NRF5_ESB_TX_ADDR_MSK) |
	                     (reverse_byte(recipient) << (NRF5_ESB_TX_ADDR - 4));
	// Set RSSI to invalid
	rssi_tx = INVALID_RSSI;

	// Confiure TX state
	if (noACK) {
		stEnter(&stTx);
	} else {
		stEnter(&stTxAck);
	}

	// Configure buffer
	radio_buffer_state = NRF5_ESB_BUFFER_STATE_TX;

	// Configure timer
#ifdef NRF5_ESB_FAST_RU
	// Enable TX on CC[3] event
	NRF5_RADIO_TIMER->CC[3] = NRF5_ESB_PRE_TX_TIME_FAST_RAMPUP;
#else
	// Enable TX on CC[3] event
	NRF5_RADIO_TIMER->CC[3] = NRF5_ESB_PRE_TX_TIME_SLOW_RAMPUP;
#endif
	// Reset timer after ARD. Time is calculated at Disabled event.
	// This isn't requred but if the DISABLED the timer is cleared.
	NRF5_RADIO_TIMER->CC[2]=NRF5_ESB_ARD<<1;

#ifdef NRF5_ESB_TX_BUG_WORKAROUND
	// Reset RTC trigger flag
	nrf5_rtc_event_triggered = false;

	// Configure RTC
#ifdef NRF51
	MY_HW_RTC->POWER = 1;
#endif
	// Reset RTC
	MY_HW_RTC->TASKS_CLEAR = 1;

	// prescaler 0, 30.517 μs resolution -> max 512 s sleep
	MY_HW_RTC->PRESCALER =  0;
	// Set CC[0] ~ tx_retries*2*ARD
	MY_HW_RTC->CC[0] = (tx_retries * NRF5_ESB_ARD *2)/30;

	// Enable timer interrupt
	MY_HW_RTC->INTENSET = RTC_INTENSET_COMPARE0_Msk;
	MY_HW_RTC->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
	MY_HW_RTC->EVENTS_COMPARE[0] = 0;
	MY_HW_RTC->TASKS_START = 1;
	NVIC_SetPriority(MY_HW_RTC_IRQN, 15);
	NVIC_ClearPendingIRQ(MY_HW_RTC_IRQN);
	NVIC_EnableIRQ(MY_HW_RTC_IRQN);
#endif

	// Start timer
	NRF5_RADIO_TIMER->TASKS_CLEAR = 1;
	NRF5_RADIO_TIMER->TASKS_START = 1;

#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
	uint32_t wakeups = 0;
#endif
	// Wait for end of transmission
	while (events_end_tx == false) {
#ifdef NRF5_ESB_TX_BUG_WORKAROUND
		// Power off CPU until next interrupt
		hwSleep();

		// Check if TX/RX is hanging
		if (nrf5_rtc_event_triggered) {
			// No ready event after timeout
			events_end_tx = true;
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
			NRF5_RADIO_DEBUG(PSTR("NRF5:SND:TIMEOUT:\n"));
#endif
		}
#else
		// Power off CPU until next interrupt
		hwSleep();
#endif
		// hwWaitForInterrupt();
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
		wakeups++;
#endif
	}
	NRF5_RADIO_DEBUG(PSTR("NRF5:SND:T=%d\n"),nrf5_rtc_event_triggered);

#ifdef NRF5_ESB_TX_BUG_WORKAROUND
	// Stop RTC
	MY_HW_RTC->INTENCLR = RTC_INTENSET_COMPARE0_Msk;
	MY_HW_RTC->EVTENCLR = RTC_EVTENSET_COMPARE0_Msk;
	MY_HW_RTC->TASKS_STOP = 1;
	NVIC_DisableIRQ(MY_HW_RTC_IRQN);
#ifdef NRF51
	MY_HW_RTC->POWER = 0;
#endif
#endif

	// Calculate RSSI
	if (rssi_tx == INVALID_RSSI) {
		// calculate pseudo-RSSI based on retransmission counter (ARC)
		// min -104dBm at 250kBps
		// Arbitrary definition: ARC 0 == -29, ARC 15 = -104
		rssi_tx = (-29 - (8 * (tx_retries_start - tx_retries)));
	}

#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
	NRF5_RADIO_DEBUG(PSTR("NRF5:SND:END=%" PRIu8 ",ACK=%" PRIu8 ",ACKLEN=%" PRIu8 ",PID=%" PRIu8
	                      ",RTRY=%" PRIi8 ",RSSI=%" PRIi16
	                      ",WAKE=%" PRIu32 "\n"),
	                 events_end_tx, ack_received, radio_buffer.len, radio_buffer.pid, tx_retries_start - tx_retries,
	                 rssi_tx, wakeups);
#endif

#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
	NRF5_RADIO_DEBUG(PSTR("NRF5:SND:TIMER:CC[0]=%" PRIu32 ",CC[1]=%" PRIu32 ",CC[2]=%" PRIu32 ",CC[3]=%"
	                      PRIu32 "\n"),
	                 NRF5_RADIO_TIMER->CC[0],NRF5_RADIO_TIMER->CC[1],NRF5_RADIO_TIMER->CC[2],NRF5_RADIO_TIMER->CC[3]);
#endif

	// Force to disable the radio
	NRF_RADIO->TASKS_DISABLE = 1;

	// Switch into listening mode
	NRF5_ESB_startListening();

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
 * Internal State Machine functions
 */
void stEnter(esbState_t* state)
{
	currentState = state;

	// Reset radio unit
	_disableRadio();

	// Configure radio interrupts
	NRF_RADIO->INTENCLR = (uint32_t)~0;
	NRF_RADIO->INTENSET = state->radioEvents;

	// Configure Shorts
	NRF_RADIO->SHORTS = state->radioShorts;

	// Configure listen address(es)
	NRF_RADIO->RXADDRESSES = state->rxAddresses;

	// Configure TX address to address at index NRF5_ESB_TX_ADDR
	NRF_RADIO->TXADDRESS = state->txAddress;

	// Clear all radio events
	NRF_RADIO->EVENTS_ADDRESS = 0;
	NRF_RADIO->EVENTS_DEVMATCH = 0;
	NRF_RADIO->EVENTS_DEVMISS = 0;
	NRF_RADIO->EVENTS_DISABLED = 0;
	NRF_RADIO->EVENTS_END = 0;
	NRF_RADIO->EVENTS_PAYLOAD = 0;
	NRF_RADIO->EVENTS_READY = 0;
	NRF_RADIO->EVENTS_RSSIEND = 0;

	// Configure Shorts
	NRF5_RADIO_TIMER->SHORTS = state->timerShorts;

	// Reset compare registers
#ifdef NRF51
	for (uint8_t i=0; i<4; i++) {
#else
	for (uint8_t i=0; i<6; i++) {
#endif
		NRF5_RADIO_TIMER->CC[i] = 0;
	}

	// Initialize PPI
	state->ppiInit();
}

void stNull(void)
{
}

void stTxEnd(void)
{
#ifdef NRF5_ESB_DO_CAPTURE
	// Capture without PPI
	NRF5_RADIO_TIMER->TASKS_CAPTURE[2]=1;
#endif
}

void stTxDisabled(void)
{
	tx_retries--;

	if (tx_retries<=0) {
		// Stop timer
		_stopTimer();

		// Mark TX as finished
		events_end_tx = true;
	} else {
		// Reset timer after ARD. CC[2] is filled with the END time via PPI27
		NRF5_RADIO_TIMER->CC[2]+=NRF5_ESB_ARD;
	}
}

void stTxAckReady(void)
{
	if ((NRF_RADIO->STATE == RADIO_STATE_STATE_Rx)||(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle)) {
		// Ongoing RX
		// Configure buffer
		radio_buffer_state = NRF5_ESB_BUFFER_STATE_TX_ACK;
#ifndef NRF5_ESB_FAST_RU
		// Restore shorts: Disable radio after RX
		NRF_RADIO->SHORTS = stTxAck.radioShorts;
#endif
		NRF5_ESB_DEBUG_LED(PIN_LED7, HIGH);
	} else {
		// Ongoing TX
		// Configure buffer
		radio_buffer_state = NRF5_ESB_BUFFER_STATE_TX;
#ifndef NRF5_ESB_FAST_RU
		// Switch to RX after TX by radio event
		NRF_RADIO->SHORTS = (stTxAck.radioShorts | RADIO_SHORTS_DISABLED_RXEN_Enabled <<
		                     RADIO_SHORTS_DISABLED_RXEN_Pos);
#endif
		NRF5_ESB_DEBUG_LED(PIN_LED8, HIGH);
	}
	NRF5_ESB_DEBUG_LED(PIN_LED6, HIGH);
}

void stTxAckAddress(void)
{
	// Configure radio to stop 20µs before ARD event occures (Fix for NRF52 PAN#102)
	NRF5_RADIO_TIMER->CC[1] = (NRF5_RADIO_TIMER->CC[2]-20);
}

void stTxAckEnd(void)
{
	// Configure ARD after TX END
	if (radio_buffer_state == NRF5_ESB_BUFFER_STATE_TX) {
		// Capture without PPI
		NRF5_RADIO_TIMER->TASKS_CAPTURE[2]=1;
#ifdef NRF5_ESB_FAST_RU
		// Enable RX via PPI21 for fast ramp up mode
		NRF5_RADIO_TIMER->CC[0]=NRF5_RADIO_TIMER->CC[2]+NRF5_ESB_PRE_RX_TIME_FAST_RAMPUP;
#endif
		// Disable radio when no ADDRESS event is generated
		NRF5_RADIO_TIMER->CC[1]=NRF5_RADIO_TIMER->CC[2]+NRF5_ESB_ACK_WAIT;
		// Reset timer after ARD. CC[2] is filled with the END time via PPI27
		NRF5_RADIO_TIMER->CC[2]+=NRF5_ESB_ARD;
	} else {
		// End event of RX and valid ACK packet?
		// Accept only ACK packages with 0 or 1 bytes payload.
		if ((radio_buffer_state == NRF5_ESB_BUFFER_STATE_TX_ACK) && (NRF_RADIO->CRCSTATUS != 0) &&
		        (radio_buffer.len<2)) {
			// Stop timer
			_stopTimer();
			// Mark TX as finished
			events_end_tx = true;
			// Mark ACK as received
			ack_received = true;
			// rssi value in ACK included?
			if (radio_buffer.len == 1) {
				rssi_tx = 0-radio_buffer.data[0];
			}
		}
	}
}

void stTxAckDisabled(void)
{
	// Handle retry counter after ACK RX
	if (radio_buffer_state == NRF5_ESB_BUFFER_STATE_TX_ACK) {
		// After RX
		tx_retries--;

		if (tx_retries<=0) {
			// Stop timer
			_stopTimer();

			// Mark TX as finished
			events_end_tx = true;
		}
	} else {
		// After TX
	}
	NRF5_ESB_DEBUG_LED(PIN_LED6, LOW);
	NRF5_ESB_DEBUG_LED(PIN_LED7, LOW);
	NRF5_ESB_DEBUG_LED(PIN_LED8, LOW);
}

// Configure PPI (Programmable peripheral interconnect) for TX mode
void stTxPPI()
{
	// Shared PPI with Ack mode
	stTXAckPPI();

	// Predefined PPI 27: RADIO->EVENTS_END ->	TIMER0->TASKS_CAPTURE[2]
	NRF_PPI->CHENSET = (PPI_CHENSET_CH27_Enabled << PPI_CHENSET_CH27_Pos);
}

void stTXAckPPI()
{
	// Rest PPI
	stNoPPI();

	// Start TX on CC[3] event
	NRF_PPI->CH[NRF5_ESB_PPI_CHANNEL_START].EEP = (uint32_t)&NRF5_RADIO_TIMER->EVENTS_COMPARE[3];
	NRF_PPI->CH[NRF5_ESB_PPI_CHANNEL_START].TEP = (uint32_t)&NRF_RADIO->TASKS_TXEN;
#if __CORTEX_M >= (0x01U)
	NRF_PPI->FORK[NRF5_ESB_PPI_CHANNEL_START].TEP = 0;
#endif
	NRF_PPI->CHENSET = (1 << NRF5_ESB_PPI_CHANNEL_START);

#ifdef NRF5_ESB_FAST_RU
	// Only required for fast ramp up mode
	// Predefined PPI 21: TIMER0->EVENTS_COMPARE[0] -> RADIO->TASKS_RXEN
	NRF_PPI->CHENSET = (PPI_CHENSET_CH21_Enabled << PPI_CHENSET_CH21_Pos);
#endif

	// Predefined PPI 22: TIMER0->EVENTS_COMPARE[1] -> RADIO->TASKS_DISABLE
	NRF_PPI->CHENSET = (PPI_CHENSET_CH22_Enabled << PPI_CHENSET_CH22_Pos);
}

// Configure PPI (Programmable peripheral interconnect) for RX mode
void stRxPPI()
{
	// Rest PPI
	stNoPPI();

#ifdef NRF52
	// Start Timer on ADDRESS event (PAN#102)
	NRF_PPI->CH[NRF5_ESB_PPI_CHANNEL_START].EEP = (uint32_t)&NRF_RADIO->EVENTS_ADDRESS;
#else
	// Start Timer on END event
	NRF_PPI->CH[NRF5_ESB_PPI_CHANNEL_START].EEP = (uint32_t)&NRF_RADIO->EVENTS_END;
#endif
	NRF_PPI->CH[NRF5_ESB_PPI_CHANNEL_START].TEP = (uint32_t)&NRF5_RADIO_TIMER->TASKS_START;
#if __CORTEX_M >= (0x01U)
	NRF_PPI->FORK[NRF5_ESB_PPI_CHANNEL_START].TEP = 0;
#endif
	NRF_PPI->CHENSET = (1 << NRF5_ESB_PPI_CHANNEL_START);

#ifdef NRF5_ESB_FAST_RU
	// Only required for fast ramp up mode
	// Predefined PPI 20: TIMER0->EVENTS_COMPARE[0] -> RADIO->TASKS_TXEN
	NRF_PPI->CHENSET = (PPI_CHENSET_CH20_Enabled << PPI_CHENSET_CH20_Pos);
#endif

#ifdef NRF52
	// PAN#102
	// Predefined PPI 22: TIMER0->EVENTS_COMPARE[1] -> RADIO->TASKS_DISABLE
	NRF_PPI->CHENSET = (PPI_CHENSET_CH22_Enabled << PPI_CHENSET_CH22_Pos);
#endif

	// Predefined PPI 27: RADIO->EVENTS_END ->	TIMER0->TASKS_CAPTURE[2]
	NRF_PPI->CHENSET = (PPI_CHENSET_CH27_Enabled << PPI_CHENSET_CH27_Pos);
}

void stRxReady(void)
{
	if ((NRF_RADIO->STATE == RADIO_STATE_STATE_Rx)||(NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle)) {
		// Ongoing RX
		// Configure buffer
		radio_buffer_state = NRF5_ESB_BUFFER_STATE_RX;
		// Configure default RX shorts without ACK; back to RX after END event
		NRF_RADIO->SHORTS = (stRx.radioShorts | RADIO_SHORTS_END_START_Msk |
		                     RADIO_SHORTS_ADDRESS_RSSISTART_Msk);
	} else {
		// Ongoing ACK
		// Configure TX shortcut. Enable RX after TX
		NRF_RADIO->SHORTS = ( stRx.radioShorts | RADIO_SHORTS_END_DISABLE_Msk |
		                      RADIO_SHORTS_DISABLED_RXEN_Msk);
	}
	// Reset timer
	_stopTimer();
	NRF5_RADIO_TIMER->TASKS_CLEAR = 1;
}

void stRxAddress(void)
{
#ifdef NRF52
	if (NRF_RADIO->STATE == RADIO_STATE_STATE_Rx) {
		// Calculate timeout for PAN#102
		NRF5_RADIO_TIMER->CC[1] = NRF5_ESB_RX_TIMEOUT;
	}
#endif
	NRF5_ESB_DEBUG_LED(PIN_LED6, HIGH);
}

void stRxEnd(void)
{
#ifdef NRF5_ESB_DO_CAPTURE
	// Capture timer for fast ramp up
	NRF5_RADIO_TIMER->TASKS_CAPTURE[2]=1;
#endif
	if (radio_buffer_state == NRF5_ESB_BUFFER_STATE_RX) {
		// RX End
		// Ignore packages without valid CRC or ACK packages or when rx_buffer is full
		if ((NRF_RADIO->CRCSTATUS != 0) && (radio_buffer.len>1) && (!rx_circular_buffer.full())) {
			// Send ACK, when addressed to node id (No ACK for BC for reversed NRF24)
#ifdef MY_NRF5_ESB_STRICT_ACK
			if ((!radio_buffer.noack) && (NRF_RADIO->RXMATCH==NRF5_ESB_NODE_ADDR))
#else
			if (NRF_RADIO->RXMATCH==NRF5_ESB_NODE_ADDR)
#endif
			{
				// Configure buffer, avoid _disableRadio() to stop in this state
				radio_buffer_state = NRF5_ESB_BUFFER_STATE_RX_ACK;
				// Prepare TX
#ifdef NRF5_ESB_FAST_RU
				// Caluclate timer
				NRF5_RADIO_TIMER->CC[0]=NRF5_RADIO_TIMER->CC[2]+NRF5_ESB_POST_RX_TIME_FAST_RAMPUP;
#else
				// Switch to TX after DISABLED event
				NRF_RADIO->SHORTS = (stRx.radioShorts | RADIO_SHORTS_DISABLED_TXEN_Msk );
#endif
				// Disable radio
				NRF_RADIO->TASKS_DISABLE = 1;
			}

			// Calculate package id
			uint32_t new_package_id = (radio_buffer.pid << 16 | NRF_RADIO->RXCRC);

			// New package?
			if (last_package_id!=new_package_id) {
				radio_buffer.rssi = NRF_RADIO->RSSISAMPLE;
				rx_circular_buffer.pushFront(&radio_buffer);
				last_package_id = new_package_id;
			}

			// Prepare ACK packet for that case it's required
			radio_buffer.data[0] = NRF_RADIO->RSSISAMPLE; // Send back RSSI
			radio_buffer.len=1; // data[0] is set some lines before
#ifndef MY_NRF5_ESB_REVERSE_ACK_TX
			radio_buffer.noack = 1; // Set ACK attribute
#else
			radio_buffer.noack = 0; // Set ACK attribute
#endif

		}
	} else {
		// TX END
		// Reset buffer state
		radio_buffer_state = NRF5_ESB_BUFFER_STATE_RX;
	}
#ifdef NRF52
	// Reset RX timeout for PAN#102
	NRF5_RADIO_TIMER->CC[1] = ~0;
#endif
	NRF5_ESB_DEBUG_LED(PIN_LED6, LOW);
}

void stRxDisabled(void)
{
	// Restart Radio (PAN#102)
	if (radio_buffer_state == NRF5_ESB_BUFFER_STATE_RX) {
		NRF_RADIO->SHORTS = stRx.radioShorts;
		radio_buffer_state = NRF5_ESB_BUFFER_STATE_RX;
		NRF_RADIO->TASKS_RXEN = 1;
	}
}


void stNoPPI()
{
	// Clear PPI
	NRF_PPI->CHENCLR = NRF5_ESB_PPI_BITMASK;
}

/*
 * Internal helper functions
 */
// Bring radio in disabled state
void _disableRadio()
{
	// Wait, when RX ACK packages are handled
	for (uint32_t i=0; (radio_buffer_state == NRF5_ESB_BUFFER_STATE_RX_ACK), i<NRF5_ESB_RX_TIMEOUT>>4;
	        i++) {
		_microDelay(16);
	}
	// Disable timer interrupts
	NRF5_RADIO_TIMER->INTENCLR = (uint32_t)~0;
	// Stop shorts working
	NRF_RADIO->SHORTS = 0;
	// Stop timer shorts
	NRF5_RADIO_TIMER->SHORTS=0;
	// Stop timer
	_stopTimer();
	// Clear timer
	NRF5_RADIO_TIMER->TASKS_CLEAR = 1;
	// Disable radio, if not in disbaled state
	if (NRF_RADIO->STATE != RADIO_STATE_STATE_Disabled) {
		// Disable radio to stop Tx
		NRF_RADIO->TASKS_DISABLE = 1;
		// Wait until disabling radio is finised
		while (NRF_RADIO->STATE != RADIO_STATE_STATE_Disabled)
			;
	}
	// Configure buffer
	radio_buffer_state = NRF5_ESB_BUFFER_STATE_DISABLED;
}

inline void _stopTimer()
{
	// Stop timer
	NRF5_RADIO_TIMER->TASKS_STOP = 1;
	// NRF52 PAN#78
	NRF5_RADIO_TIMER->TASKS_SHUTDOWN = 1;
}

void _microDelay(uint32_t us)
{
	uint32_t start = micros() ;

	do {
		yield() ;
	} while ( micros() - start < us ) ;
}

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

extern "C" {
	/** Radio Interrupt handler */
	void RADIO_IRQHandler()
	{
		if (NRF_RADIO->EVENTS_READY == 1) {
			NRF5_ESB_DEBUG_LED(PIN_LED2, HIGH);
			currentState->radioReady();
			NRF_RESET_EVENT(NRF_RADIO->EVENTS_READY);
			NRF5_ESB_DEBUG_LED(PIN_LED2, LOW);
		}
		if (NRF_RADIO->EVENTS_ADDRESS == 1) {
			NRF5_ESB_DEBUG_LED(PIN_LED3, HIGH);
			currentState->radioAddress();
			NRF_RESET_EVENT(NRF_RADIO->EVENTS_ADDRESS);
			NRF5_ESB_DEBUG_LED(PIN_LED3, LOW);
		}
		if (NRF_RADIO->EVENTS_END == 1) {
			NRF5_ESB_DEBUG_LED(PIN_LED4, HIGH);
			currentState->radioEnd();
			NRF_RESET_EVENT(NRF_RADIO->EVENTS_END);
			NRF5_ESB_DEBUG_LED(PIN_LED4, LOW);
		}
		if (NRF_RADIO->EVENTS_DISABLED == 1) {
			NRF5_ESB_DEBUG_LED(PIN_LED5, HIGH);
			currentState->radioDisabled();
			NRF_RESET_EVENT(NRF_RADIO->EVENTS_DISABLED);
			NRF5_ESB_DEBUG_LED(PIN_LED5, LOW);
		}
	}
} // extern "C"
