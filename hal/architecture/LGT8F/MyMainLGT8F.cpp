/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
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

// Initialize library and handle sketch functions like we want to

// #include <Arduino.h>
// #include <avr/wdt.h>
// #include <stdint.h>

#if defined(__LGT8FX8E__) || defined(__LGT8FX8P__)
void __patch_wdt(void) __attribute__((naked))
__attribute__((section(".init3")));
void __patch_wdt(void) {
    MCUSR = 0;
    wdt_disable();
}
#endif

// Declared weak in Arduino.h to allow user redefinitions.
int atexit(void (*/*func*/)()) { return 0; }

// Weak empty variant initialization function.
// May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() {}

void setupUSB() __attribute__((weak));
void setupUSB() {}

void sysClock(uint8_t mode) {
    if (mode == EXT_OSC) {
        // set to right prescale
        CLKPR = 0x80;
        CLKPR = 0x01;

        asm volatile("nop");
        asm volatile("nop");

        // enable external crystal
        PMCR = 0x80;
        PMCR = 0x97;

        // waiting for crystal stable
        for (GPIOR0 = 0xff; GPIOR0 > 0; --GPIOR0)
            ;
        for (GPIOR0 = 0xff; GPIOR0 > 0; --GPIOR0)
            ;

        // switch to external crystal
        PMCR = 0x80;
        PMCR = 0xb7;

        // waiting for crystal stable
        for (GPIOR0 = 0xff; GPIOR0 > 0; --GPIOR0)
            ;
        for (GPIOR0 = 0xff; GPIOR0 > 0; --GPIOR0)
            ;
        // set to right prescale
        CLKPR = 0x80;
        CLKPR = 0x00;
    } else if (mode == INT_OSC) {
        // prescaler settings
        CLKPR = 0x80;
        CLKPR = 0x01;

        // switch to internal crystal
        GPIOR0 = PMCR & 0x9f;
        PMCR = 0x80;
        PMCR = GPIOR0;

        // disable external crystal
        GPIOR0 = PMCR & 0xfb;
        PMCR = 0x80;
        PMCR = GPIOR0;
    }
}

void lgt8fx8x_init() {
#if defined(__LGT8F_SSOP20__)
    GPIOR0 = PMXCR | 0x07;
    PMXCR = 0x80;
    PMXCR = GPIOR0;
#endif

#if defined(__LGT8FX8E__)
    // store ivref calibration
    GPIOR1 = VCAL1;
    GPIOR2 = VCAL2;

    // enable 1KB E2PROM
    ECCR = 0x80;
    ECCR = 0x40;

    // clock source settings
    if ((VDTCR & 0x0C) == 0x0C) {
        // switch to external crystal
        sysClock(EXT_OSC);
    } else {
        CLKPR = 0x80;
        CLKPR = 0x01;
    }
#else
    // enable 32KRC for WDT
    GPIOR0 = PMCR | 0x10;
    PMCR = 0x80;
    PMCR = GPIOR0;

    // clock scalar to 16MHz
    CLKPR = 0x80;
    CLKPR = 0x01;
#endif
}

// START CHANGE BY DBUEZAS & SEISFELD
void lgt8fx8x_clk_src() {
// select clock source
#if defined(CLOCK_SOURCE)
#if CLOCK_SOURCE == 1
    // internal clock is default, do nothing
    // sysClock(INT_OSC);
#elif CLOCK_SOURCE == 2
    sysClock(EXT_OSC);
#endif
#endif

// select clock prescaler
#if defined(F_CPU)
    CLKPR = 0x80;
#if F_CPU == 32000000L
    CLKPR = 0x00;
#elif F_CPU == 16000000L
    CLKPR = 0x01;
#elif F_CPU == 8000000L
    CLKPR = 0x02;
#elif F_CPU == 4000000L
    CLKPR = 0x03;
#elif F_CPU == 2000000L
    CLKPR = 0x04;
#elif F_CPU == 1000000L
    CLKPR = 0x05;
#endif
#endif
}
// END CHANGE BY DBUEZAS & SEISFELD

int main(void) {

#if defined(__LGT8F__)
    lgt8fx8x_init();
#if defined(CLOCK_SOURCE)
    lgt8fx8x_clk_src();
#endif
#endif

    init();
    // initVariant();

#if defined(USBCON)
    USBDevice.attach();
#endif

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
