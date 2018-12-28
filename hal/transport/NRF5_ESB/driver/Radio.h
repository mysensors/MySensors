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

#ifndef __NRF_RADIO_H__
#define __NRF_RADIO_H__

#if !defined(ARDUINO_ARCH_NRF5)
#error "NRF5 Radio is not supported for this platform."
#endif

#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Timer to use
#define NRF5_RADIO_TIMER NRF_TIMER0
#define NRF5_RADIO_TIMER_IRQ_HANDLER TIMER0_IRQHandler
#define NRF5_RADIO_TIMER_IRQN TIMER0_IRQn

// debug
#if defined(MY_DEBUG_VERBOSE_NRF5_ESB)
#define NRF5_RADIO_DEBUG(x, ...) DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< DEBUG
#else
#define NRF5_RADIO_DEBUG(x, ...)  //!< DEBUG null
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
#ifdef RADIO_TXPOWER_TXPOWER_Pos9dBm
	// nRF52840
	NRF5_PA_MAX = RADIO_TXPOWER_TXPOWER_Pos9dBm,
#else
	// nRF51x22/nRF52822
	NRF5_PA_MAX = RADIO_TXPOWER_TXPOWER_Pos4dBm,
#endif
} nrf5_txpower_e;

// Radio mode (Data rate)
typedef enum {
	NRF5_1MBPS = RADIO_MODE_MODE_Nrf_1Mbit,
	NRF5_2MBPS = RADIO_MODE_MODE_Nrf_2Mbit,
	NRF5_250KBPS = RADIO_MODE_MODE_Nrf_250Kbit, // Deprecated!!!
	NRF5_BLE_1MBPS = RADIO_MODE_MODE_Ble_1Mbit,
} nrf5_mode_e;

int16_t NRF5_getTxPowerPercent(void);
int16_t NRF5_getTxPowerLevel(void);
bool NRF5_setTxPowerPercent(const uint8_t powerPercent);


#endif // __NRF_RADIO_H__
