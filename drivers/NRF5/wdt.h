/* Copyright (c) 2002, 2004 Marek Michalkiewicz
   Copyright (c) 2005, 2006, 2007 Eric B. Weddington
   Copyright (c) 2016 Frank Holtz
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in
     the documentation and/or other materials provided with the
     distribution.

   * Neither the name of the copyright holders nor the names of
     contributors may be used to endorse or promote products derived
     from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE. */

/* $Id$ */

/*
   avr/wdt.h - macros for AVR watchdog timer
 */

#ifndef _NRF5_WDT_H_
#define _NRF5_WDT_H_

#include <nrf.h>
#include <stdint.h>

/** \file */
/** \defgroup avr_watchdog <avr/wdt.h>: Watchdog timer handling
    \code #include <avr/wdt.h> \endcode
    @ingroup internals

    This header file declares the interface to some inline macros
    handling the watchdog timer like present in many AVR devices.
    In order to prevent the watchdog timer configuration from being
    accidentally altered by a crashing application, a special timed
    sequence is required in order to change it.  The macros within
    this header file handle the required sequence automatically
    before changing any value.  Interrupts will be disabled during
    the manipulation.
*/

/**
   \ingroup avr_watchdog
   Reset the watchdog timer.  When the watchdog timer is enabled,
   a call to this instruction is required before the timer expires,
   otherwise a watchdog-initiated device reset will occur.
*/

#define wdt_reset() NRF_WDT->RR[0] = WDT_RR_RR_Reload

/**
   \ingroup avr_watchdog
   Enable the watchdog timer, configuring it for expiry after
   \c timeout (ms).

   The WDT is running in sleep mode.

   See also the symbolic constants \c WDTO_15MS et al.
*/
#define wdt_enable(timeout) \
	NRF_WDT->CONFIG = NRF_WDT->CONFIG = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) | ( WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos); \
	NRF_WDT->CRV = (32768*timeout)/1000; \
	NRF_WDT->RREN |= WDT_RREN_RR0_Msk;  \
	NRF_WDT->TASKS_START = 1

/**
   \ingroup avr_watchdog
   Disable the watchdog timer. On nRF5 the timer cannot disabled.
   The period is set to 36h of CPU run time. The WDT is stopped in sleep mode.
*/
#define wdt_disable() \
	NRF_WDT->CONFIG = NRF_WDT->CONFIG = (WDT_CONFIG_HALT_Pause << WDT_CONFIG_HALT_Pos) | ( WDT_CONFIG_SLEEP_Pause << WDT_CONFIG_SLEEP_Pos); \
	NRF_WDT->CRV = 4294967295

/**
   \ingroup avr_watchdog
   Symbolic constants for the watchdog timeout.

   Possible timeout values are: 15 ms, 30 ms, 60 ms, 120 ms, 250 ms,
   500 ms, 1 s, 2 s, 4s, 8s.  (Not all devices allow 4 s or 8 s.)
   Symbolic constants are formed by the prefix
   \c WDTO_, followed by the time.

   Example that would select a watchdog timer expiry of approximately
   500 ms:
   \code
   wdt_enable(WDTO_500MS);
   \endcode
*/
#define WDTO_15MS   15

/** \ingroup avr_watchdog
    See \c WDTO_15MS */
#define WDTO_30MS   30

/** \ingroup avr_watchdog
    See \c WDTO_15MS */
#define WDTO_60MS   60

/** \ingroup avr_watchdog
    See \c WDTO_15MS */
#define WDTO_120MS  120

/** \ingroup avr_watchdog
    See \c WDTO_15MS */
#define WDTO_250MS  250

/** \ingroup avr_watchdog
    See \c WDTO_15MS */
#define WDTO_500MS  500

/** \ingroup avr_watchdog
    See \c WDTO_15MS */
#define WDTO_1S     1000

/** \ingroup avr_watchdog
    See \c WDTO_15MS */
#define WDTO_2S     2000

/** \ingroup avr_watchdog
    See \c WDTO_15MS
    Note: This is only available on the
    ATtiny2313,
    ATtiny24, ATtiny44, ATtiny84, ATtiny84A,
    ATtiny25, ATtiny45, ATtiny85,
    ATtiny261, ATtiny461, ATtiny861,
    ATmega48, ATmega88, ATmega168,
    ATmega48P, ATmega88P, ATmega168P, ATmega328P,
    ATmega164P, ATmega324P, ATmega644P, ATmega644,
    ATmega640, ATmega1280, ATmega1281, ATmega2560, ATmega2561,
    ATmega8HVA, ATmega16HVA, ATmega32HVB,
    ATmega406, ATmega1284P,
    AT90PWM1, AT90PWM2, AT90PWM2B, AT90PWM3, AT90PWM3B, AT90PWM216, AT90PWM316,
    AT90PWM81, AT90PWM161,
    AT90USB82, AT90USB162,
    AT90USB646, AT90USB647, AT90USB1286, AT90USB1287,
    ATtiny48, ATtiny88,
    nRF51822, nRF52832
    */
#define WDTO_4S     4000

/** \ingroup avr_watchdog
    See \c WDTO_15MS
    Note: This is only available on the
    ATtiny2313,
    ATtiny24, ATtiny44, ATtiny84, ATtiny84A,
    ATtiny25, ATtiny45, ATtiny85,
    ATtiny261, ATtiny461, ATtiny861,
    ATmega48, ATmega48A, ATmega48PA, ATmega88, ATmega168,
    ATmega48P, ATmega88P, ATmega168P, ATmega328P,
    ATmega164P, ATmega324P, ATmega644P, ATmega644,
    ATmega640, ATmega1280, ATmega1281, ATmega2560, ATmega2561,
    ATmega8HVA, ATmega16HVA, ATmega32HVB,
    ATmega406, ATmega1284P,
    ATmega2564RFR2, ATmega256RFR2, ATmega1284RFR2, ATmega128RFR2, ATmega644RFR2, ATmega64RFR2
    AT90PWM1, AT90PWM2, AT90PWM2B, AT90PWM3, AT90PWM3B, AT90PWM216, AT90PWM316,
    AT90PWM81, AT90PWM161,
    AT90USB82, AT90USB162,
    AT90USB646, AT90USB647, AT90USB1286, AT90USB1287,
    ATtiny48, ATtiny88,
    ATxmega16a4u, ATxmega32a4u,
    ATxmega16c4, ATxmega32c4,
    ATxmega128c3, ATxmega192c3, ATxmega256c3,
    nRF51822, nRF52832
    */
#define WDTO_8S     8000

#endif /* _NRF5_WDT_H_ */
