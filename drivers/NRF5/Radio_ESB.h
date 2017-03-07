/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Frank Holtz
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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
#if MAX_MESSAGE_LENGTH>(32)
#error "Unsupported message length. (MAX_MESSAGE_LENGTH)"
#endif

// check rx buffer size
#if MY_NRF5_RX_BUFFER_SIZE<(4)
#error "MY_NRF5_RX_BUFFER_SIZE must be greater than 3."
#endif

static bool NRF5_ESB_initialize();
static void NRF5_ESB_powerDown();
static bool NRF5_ESB_sanityCheck();

static void NRF5_ESB_setNodeAddress(const uint8_t address);
static uint8_t NRF5_ESB_getNodeID();

static void NRF5_ESB_startListening();
static bool NRF5_ESB_isDataAvailable();
static uint8_t NRF5_ESB_readMessage(void* data);

static bool NRF5_ESB_sendMessage(uint8_t recipient, const void* buf, uint8_t len);

// Structure of radio rackets
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
  } __attribute__((packed));
  // attributes to handle packages
  uint8_t state;
} NRF5_ESB_Packet;


#endif // __NRF5_H__
