/****
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
