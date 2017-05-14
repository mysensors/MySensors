/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
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

// Initialize library and handle sketch functions like we want to

extern "C" void __libc_init_array(void);

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() {}

int main(void)
{
	// Suspend UART
	NRF_UART0->TASKS_STOPRX = 1;
	NRF_UART0->TASKS_STOPTX = 1;
	NRF_UART0->TASKS_SUSPEND = 1;

	// Clock is manged by sleep modes. Radio depends on HFCLK.
	// Force to start HFCLK
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
		;

	// Enable low latency sleep mode
	NRF_POWER->TASKS_CONSTLAT = 1;

	// Enable cache on >= NRF52
#ifndef NRF51
	NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Msk;
#endif

	init();
	initVariant(); // arduino-nRF5 specific
	delay(1);

	_begin(); // Startup MySensors library

	for (;;) {
		_process(); // Process incoming data
		if (loop) {
			loop(); // Call sketch loop
		}
		if (serialEventRun) {
			serialEventRun();
		}
	}
	return 0;
}
