/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyright (C) 2016 Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 */

#ifndef __RF24_LINUX_H__
#define __RF24_LINUX_H__

// SPI settings
#if !defined(MY_RF24_SPI_MAX_SPEED)
	#define MY_RF24_SPI_MAX_SPEED BCM2835_SPI_SPEED_8MHZ	
#endif

// verify RF24 IRQ defs
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	#if !defined(MY_RF24_IRQ_PIN)
		#error Message buffering feature requires MY_RF24_IRQ_PIN to be defined!
	#endif
	#ifndef SPI_HAS_TRANSACTION
		#error RF24 IRQ usage requires transactional SPI support
	#endif
#else
	#ifdef MY_RX_MESSAGE_BUFFER_SIZE
		#error Receive message buffering requires RF24 IRQ usage
	#endif
#endif

// pipes
#define BROADCAST_PIPE 1
#define NODE_PIPE 2

// debug 
#if defined(MY_DEBUG_VERBOSE_RF24)
	#define RF24_DEBUG(x,...) debug(x, ##__VA_ARGS__)
#else
	#define RF24_DEBUG(x,...)
#endif

#ifdef LINUX_ARCH_RASPBERRYPI
	// CE pin
	#ifdef MY_RF24_CE_PIN
		#ifdef __RPI_BPLUS
			#if MY_RF24_CE_PIN == 3
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_03
			#elif MY_RF24_CE_PIN == 5
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_05
			#elif MY_RF24_CE_PIN == 7
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_07
			#elif MY_RF24_CE_PIN == 8
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_08
			#elif MY_RF24_CE_PIN == 10
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_10
			#elif MY_RF24_CE_PIN == 11
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_11
			#elif MY_RF24_CE_PIN == 12
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_12
			#elif MY_RF24_CE_PIN == 13
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_13
			#elif MY_RF24_CE_PIN == 15
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_15
			#elif MY_RF24_CE_PIN == 16
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_16
			#elif MY_RF24_CE_PIN == 18
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_18
			#elif MY_RF24_CE_PIN == 19
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_19
			#elif MY_RF24_CE_PIN == 21
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_21
			#elif MY_RF24_CE_PIN == 22
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_22
			#elif MY_RF24_CE_PIN == 23
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_23
			#elif MY_RF24_CE_PIN == 24
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_24
			#elif MY_RF24_CE_PIN == 26
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_26
			#elif MY_RF24_CE_PIN == 29
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_29
			#elif MY_RF24_CE_PIN == 31
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_31
			#elif MY_RF24_CE_PIN == 32
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_32
			#elif MY_RF24_CE_PIN == 33
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_33
			#elif MY_RF24_CE_PIN == 35
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_35
			#elif MY_RF24_CE_PIN == 36
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_36
			#elif MY_RF24_CE_PIN == 37
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_37
			#elif MY_RF24_CE_PIN == 38
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_38
			#elif MY_RF24_CE_PIN == 40
				#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_40
			#else
				#error invalid value for MY_RF24_CE_PIN
			#endif
		#else
			#if MY_RF24_CE_PIN == 3
				#define RF24_CE_PIN RPI_V2_GPIO_P1_03
			#elif MY_RF24_CE_PIN == 5
				#define RF24_CE_PIN RPI_V2_GPIO_P1_05
			#elif MY_RF24_CE_PIN == 7
				#define RF24_CE_PIN RPI_V2_GPIO_P1_07
			#elif MY_RF24_CE_PIN == 8
				#define RF24_CE_PIN RPI_V2_GPIO_P1_08
			#elif MY_RF24_CE_PIN == 10
				#define RF24_CE_PIN RPI_V2_GPIO_P1_10
			#elif MY_RF24_CE_PIN == 11
				#define RF24_CE_PIN RPI_V2_GPIO_P1_11
			#elif MY_RF24_CE_PIN == 12
				#define RF24_CE_PIN RPI_V2_GPIO_P1_12
			#elif MY_RF24_CE_PIN == 13
				#define RF24_CE_PIN RPI_V2_GPIO_P1_13
			#elif MY_RF24_CE_PIN == 15
				#define RF24_CE_PIN RPI_V2_GPIO_P1_15
			#elif MY_RF24_CE_PIN == 16
				#define RF24_CE_PIN RPI_V2_GPIO_P1_16
			#elif MY_RF24_CE_PIN == 18
				#define RF24_CE_PIN RPI_V2_GPIO_P1_18
			#elif MY_RF24_CE_PIN == 19
				#define RF24_CE_PIN RPI_V2_GPIO_P1_19
			#elif MY_RF24_CE_PIN == 21
				#define RF24_CE_PIN RPI_V2_GPIO_P1_21
			#elif MY_RF24_CE_PIN == 22
				#define RF24_CE_PIN RPI_V2_GPIO_P1_22
			#elif MY_RF24_CE_PIN == 23
				#define RF24_CE_PIN RPI_V2_GPIO_P1_23
			#elif MY_RF24_CE_PIN == 24
				#define RF24_CE_PIN RPI_V2_GPIO_P1_24
			#elif MY_RF24_CE_PIN == 26
				#define RF24_CE_PIN RPI_V2_GPIO_P1_26
			#elif MY_RF24_CE_PIN == 29
				#define RF24_CE_PIN RPI_V2_GPIO_P1_29
			#elif MY_RF24_CE_PIN == 31
				#define RF24_CE_PIN RPI_V2_GPIO_P1_31
			#elif MY_RF24_CE_PIN == 32
				#define RF24_CE_PIN RPI_V2_GPIO_P1_32
			#elif MY_RF24_CE_PIN == 33
				#define RF24_CE_PIN RPI_V2_GPIO_P1_33
			#elif MY_RF24_CE_PIN == 35
				#define RF24_CE_PIN RPI_V2_GPIO_P1_35
			#elif MY_RF24_CE_PIN == 36
				#define RF24_CE_PIN RPI_V2_GPIO_P1_36
			#elif MY_RF24_CE_PIN == 37
				#define RF24_CE_PIN RPI_V2_GPIO_P1_37
			#elif MY_RF24_CE_PIN == 38
				#define RF24_CE_PIN RPI_V2_GPIO_P1_38
			#elif MY_RF24_CE_PIN == 40
				#define RF24_CE_PIN RPI_V2_GPIO_P1_40
			#else
				#error invalid value for MY_RF24_CE_PIN
			#endif
		#endif
	#else
		#ifdef __RPI_BPLUS
			#define RF24_CE_PIN RPI_BPLUS_GPIO_J8_22
		#else
			#define RF24_CE_PIN RPI_V2_GPIO_P1_22
		#endif
	#endif

	// CS pin
	#ifdef MY_RF24_CS_PIN
		#ifdef __RPI_BPLUS
			#if MY_RF24_CS_PIN == 3
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_03
			#elif MY_RF24_CS_PIN == 5
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_05
			#elif MY_RF24_CS_PIN == 7
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_07
			#elif MY_RF24_CS_PIN == 8
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_08
			#elif MY_RF24_CS_PIN == 10
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_10
			#elif MY_RF24_CS_PIN == 11
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_11
			#elif MY_RF24_CS_PIN == 12
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_12
			#elif MY_RF24_CS_PIN == 13
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_13
			#elif MY_RF24_CS_PIN == 15
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_15
			#elif MY_RF24_CS_PIN == 16
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_16
			#elif MY_RF24_CS_PIN == 18
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_18
			#elif MY_RF24_CS_PIN == 19
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_19
			#elif MY_RF24_CS_PIN == 21
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_21
			#elif MY_RF24_CS_PIN == 22
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_22
			#elif MY_RF24_CS_PIN == 23
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_23
			#elif MY_RF24_CS_PIN == 24
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_24
			#elif MY_RF24_CS_PIN == 26
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_26
			#elif MY_RF24_CS_PIN == 29
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_29
			#elif MY_RF24_CS_PIN == 31
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_31
			#elif MY_RF24_CS_PIN == 32
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_32
			#elif MY_RF24_CS_PIN == 33
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_33
			#elif MY_RF24_CS_PIN == 35
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_35
			#elif MY_RF24_CS_PIN == 36
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_36
			#elif MY_RF24_CS_PIN == 37
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_37
			#elif MY_RF24_CS_PIN == 38
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_38
			#elif MY_RF24_CS_PIN == 40
				#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_40
			#else
				#error invalid value for MY_RF24_CS_PIN
			#endif
		#else
			#if MY_RF24_CS_PIN == 3
				#define RF24_CS_PIN RPI_V2_GPIO_P1_03
			#elif MY_RF24_CS_PIN == 5
				#define RF24_CS_PIN RPI_V2_GPIO_P1_05
			#elif MY_RF24_CS_PIN == 7
				#define RF24_CS_PIN RPI_V2_GPIO_P1_07
			#elif MY_RF24_CS_PIN == 8
				#define RF24_CS_PIN RPI_V2_GPIO_P1_08
			#elif MY_RF24_CS_PIN == 10
				#define RF24_CS_PIN RPI_V2_GPIO_P1_10
			#elif MY_RF24_CS_PIN == 11
				#define RF24_CS_PIN RPI_V2_GPIO_P1_11
			#elif MY_RF24_CS_PIN == 12
				#define RF24_CS_PIN RPI_V2_GPIO_P1_12
			#elif MY_RF24_CS_PIN == 13
				#define RF24_CS_PIN RPI_V2_GPIO_P1_13
			#elif MY_RF24_CS_PIN == 15
				#define RF24_CS_PIN RPI_V2_GPIO_P1_15
			#elif MY_RF24_CS_PIN == 16
				#define RF24_CS_PIN RPI_V2_GPIO_P1_16
			#elif MY_RF24_CS_PIN == 18
				#define RF24_CS_PIN RPI_V2_GPIO_P1_18
			#elif MY_RF24_CS_PIN == 19
				#define RF24_CS_PIN RPI_V2_GPIO_P1_19
			#elif MY_RF24_CS_PIN == 21
				#define RF24_CS_PIN RPI_V2_GPIO_P1_21
			#elif MY_RF24_CS_PIN == 22
				#define RF24_CS_PIN RPI_V2_GPIO_P1_22
			#elif MY_RF24_CS_PIN == 23
				#define RF24_CS_PIN RPI_V2_GPIO_P1_23
			#elif MY_RF24_CS_PIN == 24
				#define RF24_CS_PIN BCM2835_SPI_CS0
			#elif MY_RF24_CS_PIN == 26
				#define RF24_CS_PIN BCM2835_SPI_CS1
			#elif MY_RF24_CS_PIN == 29
				#define RF24_CS_PIN RPI_V2_GPIO_P1_29
			#elif MY_RF24_CS_PIN == 31
				#define RF24_CS_PIN RPI_V2_GPIO_P1_31
			#elif MY_RF24_CS_PIN == 32
				#define RF24_CS_PIN RPI_V2_GPIO_P1_32
			#elif MY_RF24_CS_PIN == 33
				#define RF24_CS_PIN RPI_V2_GPIO_P1_33
			#elif MY_RF24_CS_PIN == 35
				#define RF24_CS_PIN RPI_V2_GPIO_P1_35
			#elif MY_RF24_CS_PIN == 36
				#define RF24_CS_PIN RPI_V2_GPIO_P1_36
			#elif MY_RF24_CS_PIN == 37
				#define RF24_CS_PIN RPI_V2_GPIO_P1_37
			#elif MY_RF24_CS_PIN == 38
				#define RF24_CS_PIN RPI_V2_GPIO_P1_38
			#elif MY_RF24_CS_PIN == 40
				#define RF24_CS_PIN RPI_V2_GPIO_P1_40
			#else
				#error invalid value for MY_RF24_CS_PIN
			#endif
		#endif
	#else
		#ifdef __RPI_BPLUS
			#define RF24_CS_PIN RPI_BPLUS_GPIO_J8_24
		#else
			#define RF24_CS_PIN BCM2835_SPI_CS0
		#endif
	#endif
#endif

#endif
