#include "log.h"
#include <stdio.h>
#include <stdint.h>
#include <syslog.h>
#include <stdarg.h>

// Default values
static const int log_opts = LOG_CONS | LOG_PERROR;	// print syslog to stderror
static const int log_facility = LOG_USER;

static uint8_t log_open = 0;

void logOpen(int options, int facility)
{
	openlog(NULL, options, facility);
	log_open = 1;
}

void vlogInfo(const char *fmt, va_list args)
{
	if (!log_open) {
		logOpen(log_opts, log_facility);
	}
	vsyslog(LOG_INFO, fmt, args);
}

void
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
logInfo(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogInfo(fmt, args);
	va_end(args);
}

void vlogError(const char *fmt, va_list args)
{
	if (!log_open) {
		logOpen(log_opts, log_facility);
	}
	vsyslog(LOG_ERR, fmt, args);
}

void
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
logError(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogError(fmt, args);
	va_end(args);
}

void vlogNotice(const char *fmt, va_list args)
{
	if (!log_open) {
		logOpen(log_opts, log_facility);
	}
	vsyslog(LOG_NOTICE, fmt, args);
}

void
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
logNotice(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogNotice(fmt, args);
	va_end(args);
}

void vlogDebug(const char *fmt, va_list args)
{
	if (!log_open) {
		logOpen(log_opts, log_facility);
	}
	vsyslog(LOG_DEBUG, fmt, args);
}

void
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
logDebug(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogDebug(fmt, args);
	va_end(args);
}

void vlogWarning(const char *fmt, va_list args)
{
	if (!log_open) {
		logOpen(log_opts, log_facility);
	}
	vsyslog(LOG_WARNING, fmt, args);
}

void
#ifdef __GNUC__
__attribute__((format(printf, 1, 2)))
#endif
logWarning(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vlogWarning(fmt, args);
	va_end(args);
}
