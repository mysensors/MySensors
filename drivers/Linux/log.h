/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef LOG_H
#define LOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void logOpen(int options, int facility);

extern void vlogInfo(const char *fmt, va_list args);
extern void logInfo(const char *fmt, ...) __attribute__((format(printf,1,2)));

extern void vlogError(const char *fmt, va_list args);
extern void logError(const char *fmt, ...) __attribute__((format(printf,1,2)));

extern void vlogNotice(const char *fmt, va_list args);
extern void logNotice(const char *fmt, ...) __attribute__((format(printf,1,2)));

extern void vlogDebug(const char *fmt, va_list args);
extern void logDebug(const char *fmt, ...) __attribute__((format(printf,1,2)));

extern void vlogWarning(const char *fmt, va_list args);
extern void logWarning(const char *fmt, ...) __attribute__((format(printf,1,2)));

#ifdef __cplusplus
}
#endif

#endif
