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

#ifndef MyLeds_h
#define MyLeds_h


#ifdef MY_WITH_LEDS_BLINKING_INVERSE
#define LED_ON 0x1
#define LED_OFF 0x0
#else
#define LED_ON 0x0
#define LED_OFF 0x1
#endif

#if defined(MY_DEFAULT_TX_LED_PIN) || defined(MY_DEFAULT_RX_LED_PIN) || defined(MY_DEFAULT_ERR_LED_PIN)
#define ledBlinkTx(x,...) ledsBlinkTx(x)
#define ledBlinkRx(x,...) ledsBlinkRx(x)
#define ledBlinkErr(x,...) ledsBlinkErr(x)

/**
 * Blink with LEDs
 * @param cnt how many blink cycles to keep the LED on. Default cycle is 300ms
 */
void ledsInit();
void ledsBlinkRx(uint8_t cnt);
void ledsBlinkTx(uint8_t cnt);
void ledsBlinkErr(uint8_t cnt);
void ledsProcess(); // do the actual blinking
/**
 * Test if any LED is currently blinking.
 * @return true when one or more LEDs are blinking, false otherwise.
 */
bool ledsBlinking();

#else
// Remove led functions if feature is disabled
#define ledBlinkTx(x,...)
#define ledBlinkRx(x,...)
#define ledBlinkErr(x,...)
#endif

#endif
