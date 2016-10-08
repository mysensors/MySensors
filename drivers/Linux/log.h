#ifndef LOG_H
#define LOG_H

#include <stdarg.h>
#include <syslog.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void mys_log_v(int level, const char *fmt, va_list args);
extern void mys_log(int level, const char *fmt, ...) __attribute__((format(printf,2,3)));

#ifdef __cplusplus
}
#endif

#endif
