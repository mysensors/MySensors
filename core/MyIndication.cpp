/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyIndication.h"
#if defined(MY_DEFAULT_TX_LED_PIN)|| defined(MY_DEFAULT_RX_LED_PIN) || defined(MY_DEFAULT_ERR_LED_PIN)
#include "MyLeds.h"
#endif

void setIndication( const indication_t ind )
{
#if defined(MY_DEFAULT_TX_LED_PIN)
	if ((INDICATION_TX == ind) || (INDICATION_GW_TX == ind)) {
		ledsBlinkTx(1);
	} else
#endif
#if defined(MY_DEFAULT_RX_LED_PIN)
		if ((INDICATION_RX == ind) || (INDICATION_GW_RX == ind)) {
			ledsBlinkRx(1);
		} else
#endif
#if defined(MY_DEFAULT_ERR_LED_PIN)
			if (ind > INDICATION_ERR_START) {
				// Number of blinks indicates which error occurred.
				ledsBlinkErr(ind-INDICATION_ERR_START);
			}
#endif
	indication(ind);
}

#if !defined(MY_INDICATION_HANDLER)
void indication(indication_t)
{
	// empty function, resolves AVR-specific GCC optimization bug (<5.5) if handler not used
	// see here: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=77326
}
#endif
