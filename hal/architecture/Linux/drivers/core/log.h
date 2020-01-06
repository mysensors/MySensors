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

#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define vlogError(...) vlog(LOG_ERR, __VA_ARGS__)
#define vlogWarning(...) vlog(LOG_WARNING, __VA_ARGS__)
#define vlogNotice(...) vlog(LOG_NOTICE, __VA_ARGS__)
#define vlogInfo(...) vlog(LOG_INFO, __VA_ARGS__)
#define vlogDebug(...) vlog(LOG_DEBUG, __VA_ARGS__)

void logSetQuiet(uint8_t enable);
void logSetLevel(int level);
void logSetSyslog(int options, int facility);
int logSetPipe(char *pipe_file);
int logSetFile(char *file);
void logClose(void);

void vlog(int level, const char *fmt, va_list args);
void logEmergency(const char *fmt, ...) __attribute__((format(printf,1,2)));
void logAlert(const char *fmt, ...) __attribute__((format(printf,1,2)));
void logCritical(const char *fmt, ...) __attribute__((format(printf,1,2)));
void logError(const char *fmt, ...) __attribute__((format(printf,1,2)));
void logWarning(const char *fmt, ...) __attribute__((format(printf,1,2)));
void logNotice(const char *fmt, ...) __attribute__((format(printf,1,2)));
void logInfo(const char *fmt, ...) __attribute__((format(printf,1,2)));
void logDebug(const char *fmt, ...) __attribute__((format(printf,1,2)));

#ifdef __cplusplus
}
#endif

#endif
