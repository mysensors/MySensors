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
#include "drivers/CircularBuffer/CircularBuffer.h"
#include <stdio.h>

// Parameters
// auto retry delay
#define NRF5_ESB_ARD (1500) // us

// auto retry count
#define NRF5_ESB_ARC (15)

// TX address id
#define NRF5_ESB_TX_ADDR (4)

// internal functions
static uint8_t reverse_byte(uint8_t address);
static inline void wait_for_interrupt();

// RX Buffer
static NRF5_ESB_Packet rx_circular_buffer_buffer[MY_NRF5_RX_BUFFER_SIZE];
// Poiter to rx circular buffer
static NRF5_ESB_Packet *rx_buffer;
// Circular buffer
static CircularBuffer<NRF5_ESB_Packet>
    rx_circular_buffer(rx_circular_buffer_buffer, MY_NRF5_RX_BUFFER_SIZE);
// Dedect duplicate packages for every pipe available
static volatile uint32_t package_ids[8];

// TX Buffer
static NRF5_ESB_Packet tx_buffer;
// ACK packet
static NRF5_ESB_Packet ackPacket;
// Flag for ack received
static volatile bool ack_received;
// Flag for end event
static volatile bool events_end;
// Flag if radio is disabled
static volatile bool radio_disabled;

// Initialize radio unit
static bool NRF5_ESB_initialize() {
  NRF5_RADIO_DEBUG(PSTR("NRF5:Initialize ESB mode\n"));

  // Radio depends on HFCLK. Start HFCLK
  NRF_CLOCK->TASKS_HFCLKSTART = 1;
  while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    ;

  // Power on radio unit
  NRF_RADIO->POWER = 1;

  // Enable radio interrupt
  NVIC_SetPriority(RADIO_IRQn, 1);
  NVIC_EnableIRQ(RADIO_IRQn);

  // Enable timer interrupt
  NVIC_SetPriority(RADIO_IRQn, 2);
  NVIC_EnableIRQ(NRF5_RADIO_TIMER_IRQN);

  // Clear events
  NRF_RADIO->EVENTS_END = 0;

  // Select interrupt events
  NRF_RADIO->INTENSET = RADIO_INTENSET_END_Msk;

  // Configure radio parameters: tx power
  NRF_RADIO->TXPOWER = MY_NRF5_ESB_PA_LEVEL << RADIO_TXPOWER_TXPOWER_Pos;

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
  NRF_RADIO->PREFIX0 = 0xffffff00UL; // node address
  NRF_RADIO->BASE0 = reverse_byte(address[1]) << 24 |
                     reverse_byte(address[2]) << 16 |
                     reverse_byte(address[3]) << 8 | reverse_byte(address[4]);
  NRF_RADIO->BASE1 = reverse_byte(address[1]) << 24 |
                     reverse_byte(address[2]) << 16 |
                     reverse_byte(address[3]) << 8 | reverse_byte(address[4]);
  NRF_RADIO->PREFIX1 = 0xffffffffUL; // Broadcast and send address
  NRF5_RADIO_DEBUG(PSTR("NRF5:BASE0=%08x PREFIX0=%08x\n"), NRF_RADIO->BASE0,
                   NRF_RADIO->PREFIX0);
  NRF_RADIO->RXADDRESSES = 0xffUL; // node, broadcast and TX

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
      ((MY_NRF5_ESB_ADDR_WIDTH - 1)
       << RADIO_PCNF1_BALEN_Pos) | // Set base address length
      (RADIO_PCNF1_ENDIAN_Big << RADIO_PCNF1_ENDIAN_Pos) | // Big endian
      (RADIO_PCNF1_WHITEEN_Disabled
       << RADIO_PCNF1_WHITEEN_Pos); // Disable whitening

  // Prepare timer running at 1 MHz
  NRF5_RADIO_TIMER->PRESCALER = 4;
  // in 16 Bit mode
  NRF5_RADIO_TIMER->BITMODE = TIMER_BITMODE_BITMODE_16Bit;
  // Stop timer when CC0 reached
  NRF5_RADIO_TIMER->SHORTS =
      TIMER_SHORTS_COMPARE0_CLEAR_Msk | TIMER_SHORTS_COMPARE0_STOP_Msk;
  // Set auto retry delay minus t_TXEN
  NRF5_RADIO_TIMER->EVENTS_COMPARE[0] = NRF5_ESB_ARD - 140;

  // Configure PPI (Programmable peripheral interconnect)
  NRF_PPI->CH[NRF5_RADIO_PPI_TIMER_START].EEP =
      (uint32_t)&NRF_RADIO->EVENTS_READY;
  NRF_PPI->CH[NRF5_RADIO_PPI_TIMER_START].TEP =
      (uint32_t)&NRF5_RADIO_TIMER->TASKS_START;
  NRF_PPI->CH[NRF5_RADIO_PPI_TIMER_STOP].EEP = (uint32_t)&NRF_RADIO->EVENTS_END;
  NRF_PPI->CH[NRF5_RADIO_PPI_TIMER_STOP].TEP =
      (uint32_t)&NRF5_RADIO_TIMER->TASKS_STOP;

  // Prepare ACK packet
  ackPacket.len = 0;
  ackPacket.pid = 0;
  ackPacket.noack = 0;

  // Set internal variables
  radio_disabled = true;

  return true;
}

static void NRF5_ESB_powerDown() {
  NRF5_RADIO_DEBUG(PSTR("NRF5:STRT PD\n"));

  // Disable inerrupt
  NVIC_DisableIRQ(RADIO_IRQn);
  NVIC_DisableIRQ(NRF5_RADIO_TIMER_IRQN);

  // Clear PPI
  NRF_PPI->CHENCLR = NRF5_RADIO_PPI_CLEAR;

  // Power off readio unit
  NRF_RADIO->POWER = 0;
};

// always true
static bool NRF5_ESB_sanityCheck() { return true; }

static void NRF5_ESB_setNodeAddress(const uint8_t address) {
  NRF5_RADIO_DEBUG(PSTR("NRF5:SET ADDR %d\n"), address);
  NRF_RADIO->PREFIX0 = 0xffffff00UL | reverse_byte(address);
}

static uint8_t NRF5_ESB_getNodeID() { return reverse_byte(NRF_RADIO->PREFIX0); }

static void NRF5_ESB_startListening() {
  // Set buffer
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

  // Enable RX when ready, Enable RX after disabling task
  NRF_RADIO->SHORTS =
      RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_DISABLED_RXEN_Msk;

  // Switch to RX
  if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {
    NRF_RADIO->TASKS_RXEN = 1;
  } else {
    NRF_RADIO->TASKS_DISABLE = 1;
  }

  radio_disabled = false;

  // Log message after enabling RX!
  NRF5_RADIO_DEBUG(PSTR("NRF5:STRT LIS\n"));
}

static bool NRF5_ESB_isDataAvailable() {
  return rx_circular_buffer.available() > 0;
}

static uint8_t NRF5_ESB_readMessage(void *data) {
  // get content from rx buffer
  NRF5_ESB_Packet *buffer = rx_circular_buffer.getBack();
  // Nothing to read?
  if (buffer == NULL) {
    return 0;
  }
  // copy content
  memcpy(data, buffer->data, buffer->len);
  uint8_t ret = buffer->len;
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

static bool NRF5_ESB_sendMessage(uint8_t recipient, const void *buf,
                                 uint8_t len) {
  NRF5_RADIO_DEBUG(PSTR("NRF5:SND:TO=%d,LEN=%d,PID=%d,NOACK=%d\n"), recipient,
                   len, tx_buffer.pid, tx_buffer.noack); // send message
  // check length
  if (len > MAX_MESSAGE_LENGTH)
    len = MAX_MESSAGE_LENGTH;

  // copy data to tx_buffer
  memcpy(&tx_buffer.data[0], buf, len);

  // build metadata
  tx_buffer.len = len;
  tx_buffer.noack = false;
  tx_buffer.pid++;

  // This line disables sending a broadcast packet multiple times
  // ack_received = (recipient == BROADCAST_ADDRESS);
  // This line force to send an boradcast packet multiple times like NRF24
  ack_received = false;

  for (int retries = 0; retries < NRF5_ESB_ARC; retries++) {
    // Set buffer
    NRF_RADIO->PACKETPTR = (uint32_t)&tx_buffer;

    // set TX address
    NRF_RADIO->PREFIX1 = 0xffffff00UL | reverse_byte(recipient);
    NRF_RADIO->TXADDRESS = NRF5_ESB_TX_ADDR; // Address to send data

    // Enable TX when ready, Enable TX after disabling task
    NRF_RADIO->SHORTS =
        RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_DISABLED_TXEN_Msk;

    // reset timer
    NRF5_RADIO_TIMER->EVENTS_COMPARE[0] = 0;
    NRF5_RADIO_TIMER->TASKS_CLEAR = 1;

    // Set PPI
    NRF_PPI->CHENSET =
        (1 << NRF5_RADIO_PPI_TIMER_START) | (1 << NRF5_RADIO_PPI_TIMER_STOP);

    // Switch to TX
    if (NRF_RADIO->STATE == RADIO_STATE_STATE_Disabled) {
      NRF_RADIO->TASKS_TXEN = 1;
    } else {
      NRF_RADIO->TASKS_DISABLE = 1;
    }

    // Wait for end of transmission
    events_end = false;
    while ((events_end == false) && (NRF5_RADIO_TIMER->EVENTS_COMPARE[0] == 0))
      wait_for_interrupt();

    // end loop, when ack_received is true
    if (ack_received)
      break;
  }

  // Clear PPI
  NRF_PPI->CHENCLR = NRF5_RADIO_PPI_CLEAR;

  // reset TX address
  NRF_RADIO->PREFIX1 = 0xffffffffUL;
  return ack_received;
};

/*
 * Internal helper functions
 */

// Reverse a byte for address
static uint8_t reverse_byte(uint8_t address) {
#if __CORTEX_M >= (0x01U)
  return __REV(__RBIT(address));
#else
  address =
      ((address * 0x0802LU & 0x22110LU) | (address * 0x8020LU & 0x88440LU)) *
          0x10101LU >>
      16;
#endif
  return address;
}

static inline void wait_for_interrupt() {
  __DSB();
  __WFI();
}

// Send ACK
bool send_ack() {
  // Send ACK when packet is received at first address
  if (NRF_RADIO->RXMATCH == 0) {
    ackPacket.pid++;
    // set pointer
    NRF_RADIO->PACKETPTR = (uint32_t)&ackPacket;
    NRF_RADIO->TXADDRESS = 0x00UL; // Address to send data

    // Enable TX when ready, Enable TX after disabling task
    NRF_RADIO->SHORTS =
        RADIO_SHORTS_READY_START_Msk | RADIO_SHORTS_DISABLED_TXEN_Msk;

    // Switch to TX
    NRF_RADIO->TASKS_DISABLE = 1;

    // Ack send
    return true;
  }
  // No ACK send
  return false;
}

// Interrupt handler
extern "C" {
void RADIO_IRQHandler() {
  if (NRF_RADIO->EVENTS_END == 1) {
    events_end = true;
    NRF_RADIO->EVENTS_END = 0;

    if (NRF_RADIO->STATE == RADIO_STATE_STATE_RxIdle) {
      bool ack_send = false;
#ifdef MY_DEBUG_VERBOSE_NRF5_ESB
      NRF5_ESB_Packet *old_rx_buffer = rx_buffer;
#endif

      // calculate a package id
      uint32_t pkgid = rx_buffer->pid << 16 | NRF_RADIO->RXCRC;
      if (NRF_RADIO->CRCSTATUS) {
        if (pkgid != package_ids[NRF_RADIO->RXMATCH]) {
          // correct package -> store id to dedect duplicates
          package_ids[NRF_RADIO->RXMATCH] = pkgid;

          if (NRF_RADIO->RXMATCH == NRF5_ESB_TX_ADDR) { // Send address -> ACK
            // stop listening on TX address
            NRF_RADIO->PREFIX1 = 0xffffffffUL;
            // mark ack
            ack_received = true;
          } else {
            // Use next buffer for RX
            if ((rx_circular_buffer.pushFront(rx_buffer)) &&
                (rx_buffer->noack == false)) {
              // send ack
              ack_send = send_ack();
            }
          }
        } else {
          if (rx_buffer->noack == false) {
            ack_send = send_ack();
          }
        }
      }
      // start listening
      if (!ack_send) {
        rx_buffer = rx_circular_buffer.getFront();
        // Check if a buffer is available
        if (rx_buffer != NULL) {
          // set pointer
          NRF_RADIO->PACKETPTR = (uint32_t)rx_buffer;
          // Start listening
          NRF_RADIO->TASKS_START = 1UL;
        } else {
          // No buffer -> disable listening
          radio_disabled = true;
          NRF_RADIO->SHORTS = 0;
          NRF_RADIO->EVENTS_DISABLED = 0;
          NRF_RADIO->TASKS_DISABLE = 1;
        }
      }
      NRF5_RADIO_DEBUG(PSTR("NRF5:INT LEN=%d,CRC=%d,NOACK=%d,PID=%d,"
                            "RX=%d\n"),
                       old_rx_buffer->len, NRF_RADIO->CRCSTATUS,
                       old_rx_buffer->noack, old_rx_buffer->pid,
                       NRF_RADIO->RXMATCH);
    } else if (NRF_RADIO->STATE == RADIO_STATE_STATE_TxIdle) {
      // Was in TX state -> Switch to RX
      NRF5_ESB_startListening();
    }
    return;
  }
}
void NRF5_RADIO_TIMER_IRQ_HANDLER() {}
} // extern "C"
