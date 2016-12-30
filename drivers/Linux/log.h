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
