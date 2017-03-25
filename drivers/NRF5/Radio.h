
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

#ifndef __RADIO_H__
#define __RADIO_H__

#if ! defined (ARDUINO)
#error "NRF5 Radio is not supported for this platform."
#endif

#if defined(NRF5_SOFTDEVICE)
#warning "NRF5 SoftDevice cannot be used with NRF5 Radio."
#endif

#include <nrf.h>

// Timer to use
#define NRF5_RADIO_TIMER NRF_TIMER2
#define NRF5_RADIO_TIMER_IRQ_HANDLER TIMER2_IRQHandler
#define NRF5_RADIO_TIMER_IRQN TIMER2_IRQn

// PPI Channels
#define NRF5_RADIO_PPI_TIMER_START 14
#define NRF5_RADIO_PPI_TIMER_STOP 15
#define NRF5_RADIO_PPI_CLEAR ((1 << NRF5_RADIO_PPI_TIMER_START) | (1 << NRF5_RADIO_PPI_TIMER_STOP))

// debug
#if defined(MY_DEBUG_VERBOSE_NRF5_ESB)
#define NRF5_RADIO_DEBUG(x,...) debug(x, ##__VA_ARGS__)
#else
#define NRF5_RADIO_DEBUG(x,...)
#endif

// tx power
typedef enum {
  #ifdef NRF51
  NRF5_PA_MIN = RADIO_TXPOWER_TXPOWER_Neg30dBm, // Deprecated
  #else
  NRF5_PA_MIN = RADIO_TXPOWER_TXPOWER_Neg40dBm,
  #endif
  NRF5_PA_LOW = RADIO_TXPOWER_TXPOWER_Neg16dBm,
  NRF5_PA_HIGH = RADIO_TXPOWER_TXPOWER_0dBm,
  NRF5_PA_MAX = RADIO_TXPOWER_TXPOWER_Pos4dBm
} nrf5_txpower_e;

// Radio mode (Data rate)
typedef enum {
  NRF5_1MBPS = RADIO_MODE_MODE_Nrf_1Mbit,
  NRF5_2MBPS = RADIO_MODE_MODE_Nrf_2Mbit,
  NRF5_250KBPS = RADIO_MODE_MODE_Nrf_250Kbit, // Deprecated!!!
  NRF5_BLE_1MBPS = RADIO_MODE_MODE_Ble_1Mbit,
} nrf5_mode_e;

#endif // __RADIO_H__
