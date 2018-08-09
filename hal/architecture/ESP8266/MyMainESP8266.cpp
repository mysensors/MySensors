/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2018 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

//This may be used to change user task stack size:
//#define CONT_STACKSIZE 4096
#include <Arduino.h>
#include "Schedule.h"
extern "C" {
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "mem.h"
#include "user_interface.h"
#include "cont.h"
}
#include <core_version.h>
#include "gdb_hooks.h"

#define LOOP_TASK_PRIORITY 1
#define LOOP_QUEUE_SIZE    1
#define OPTIMISTIC_YIELD_TIME_US 16000

extern "C" void call_user_start();
extern void loop();
extern void setup();
extern void(*__init_array_start)(void);
extern void(*__init_array_end)(void);

/* Not static, used in Esp.cpp */
struct rst_info resetInfo;

/* Not static, used in core_esp8266_postmortem.c.
* Placed into noinit section because we assign value to this variable
* before .bss is zero-filled, and need to preserve the value.
*/
cont_t* g_pcont __attribute__((section(".noinit")));

/* Event queue used by the main (arduino) task */
static os_event_t s_loop_queue[LOOP_QUEUE_SIZE];

/* Used to implement optimistic_yield */
static uint32_t s_micros_at_task_start;


extern "C" {
	extern const uint32_t __attribute__((section(".ver_number"))) core_version =
	    ARDUINO_ESP8266_GIT_VER;
	const char* core_release =
#ifdef ARDUINO_ESP8266_RELEASE
	    ARDUINO_ESP8266_RELEASE;
#else
	    NULL;
#endif
} // extern "C"

void initVariant() __attribute__((weak));
void initVariant()
{
}

void preloop_update_frequency() __attribute__((weak));
void preloop_update_frequency()
{
#if defined(F_CPU) && (F_CPU == 160000000L)
	REG_SET_BIT(0x3ff00014, BIT(0));
	ets_update_cpu_frequency(160);
#endif
}


extern "C" void esp_yield()
{
	if (cont_can_yield(g_pcont)) {
		cont_yield(g_pcont);
	}
}

extern "C" void esp_schedule()
{
	ets_post(LOOP_TASK_PRIORITY, 0, 0);
}

extern "C" void __yield()
{
	if (cont_can_yield(g_pcont)) {
		esp_schedule();
		esp_yield();
	} else {
		panic();
	}
}

extern "C" void yield(void) __attribute__((weak, alias("__yield")));

extern "C" void optimistic_yield(uint32_t interval_us)
{
	if (cont_can_yield(g_pcont) &&
	        (system_get_time() - s_micros_at_task_start) > interval_us) {
		yield();
	}
}

static void loop_wrapper()
{
	static bool setup_done = false;
	preloop_update_frequency();
	if (!setup_done) {
		_begin();		// Startup MySensors library
		setup_done = true;
	}
	_process();			// Process incoming data
	loop();
	run_scheduled_functions();
	esp_schedule();
}

static void loop_task(os_event_t *events)
{
	(void)events;
	s_micros_at_task_start = system_get_time();
	cont_run(g_pcont, &loop_wrapper);
	if (cont_check(g_pcont) != 0) {
		panic();
	}
}

static void do_global_ctors(void)
{
	void(**p)(void) = &__init_array_end;
	while (p != &__init_array_start) {
		(*--p)();
	}
}

void init_done()
{
	system_set_os_print(1);
	gdb_init();
	do_global_ctors();
	esp_schedule();
}

/* This is the entry point of the application.
* It gets called on the default stack, which grows down from the top
* of DRAM area.
* .bss has not been zeroed out yet, but .data and .rodata are in place.
* Cache is not enabled, so only ROM and IRAM functions can be called.
* Peripherals (except for SPI0 and UART0) are not initialized.
* This function does not return.
*/
/*
A bit of explanation for this entry point:

SYS is the SDK task/context used by the upperlying system to run its
administrative tasks (at least WLAN and lwip's receive callbacks and
Ticker).  NONOS-SDK is designed to run user's non-threaded code in
another specific task/context with its own stack in BSS.

Some clever fellows found that the SYS stack was a large and quite unused
piece of ram that we could use for the user's stack instead of using user's
main memory, thus saving around 4KB on ram/heap.

A problem arose later, which is that this stack can heavily be used by
the SDK for some features.  One of these features is WPS.  We still don't
know if other features are using this, or if this memory is going to be
used in future SDK releases.

WPS beeing flawed by its poor security, or not beeing used by lots of
users, it has been decided that we are still going to use that memory for
user's stack and disable the use of WPS, with an option to revert that
back at the user's discretion.  This selection can be done with the
global define NO_EXTRA_4K_HEAP.  An option has been added to the board
generator script.

References:
https://github.com/esp8266/Arduino/pull/4553
https://github.com/esp8266/Arduino/pull/4622
https://github.com/esp8266/Arduino/issues/4779
https://github.com/esp8266/Arduino/pull/4889

*/

#ifdef NO_EXTRA_4K_HEAP
/* this is the default NONOS-SDK user's heap location */
cont_t g_cont __attribute__((aligned(16)));
#endif

extern "C" void ICACHE_RAM_ATTR app_entry(void)
{
#ifdef NO_EXTRA_4K_HEAP

	/* this is the default NONOS-SDK user's heap location */
	g_pcont = &g_cont;

#else

	/* Allocate continuation context on this SYS stack,
	and save pointer to it. */
	cont_t s_cont __attribute__((aligned(16)));
	g_pcont = &s_cont;

#endif

	/* Call the entry point of the SDK code. */
	call_user_start();
}

extern "C" void user_init(void)
{
	struct rst_info *rtc_info_ptr = system_get_rst_info();
	memcpy((void *)&resetInfo, (void *)rtc_info_ptr, sizeof(resetInfo));

	uart_div_modify(0, UART_CLK_FREQ / (115200));

	init();

	initVariant();

	cont_init(g_pcont);

	ets_task(loop_task,
	         LOOP_TASK_PRIORITY, s_loop_queue,
	         LOOP_QUEUE_SIZE);

	system_init_done_cb(&init_done);
}
