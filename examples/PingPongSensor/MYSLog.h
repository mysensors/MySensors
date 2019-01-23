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
 *
 *******************************
 *
 * Formatted logging to the Serial console.
 * Compiled in by setting LOGDEBUG
 *
 * 2015-05-25  Bruce Lacey V1.0
 *
 * Based upon Arduino Playground prior art and should be moved to
 * the MySensors library at some point as a common debug logging facility
 */
#ifndef MYSLog_h
#define MYSLog_h

#define LOGDEBUG 1

#if defined ( LOGDEBUG )
#define LOG(fmt, args... ) log( fmt, ## args );
#else
#define log(fmt, args... )
#endif

void log(const char *fmt, ... )
{
	char buff[128];
	va_list args;
	va_start (args, fmt);
	vsnprintf(buff, sizeof(buff), fmt, args);
	va_end (args);
	buff[sizeof(buff)/sizeof(buff[0])-1]='\0';
	Serial.print(buff);
}

void log(const __FlashStringHelper *fmt, ... )
{
	char buf[128]; // resulting string limited to 128 chars
	va_list args;
	va_start (args, fmt);
#ifdef __AVR__
	vsnprintf_P(buf, sizeof(buf), (const char *)fmt, args); // progmem for AVR
#else
	vsnprintf(buf, sizeof(buf), (const char *)fmt, args); // for the rest of the world
#endif
	va_end(args);
	Serial.print(buf);
}

#endif
