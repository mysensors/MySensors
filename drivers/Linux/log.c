#include <stdio.h>
#include <syslog.h>
#include <stdarg.h>
#include "log.h"

void mys_log_v(int level, const char *fmt, va_list args)
{
	vsyslog(level, fmt, args);
}

void
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
mys_log(int level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	mys_log_v(level, fmt, args);
	va_end(args);
}
