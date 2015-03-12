#ifndef MyHwATMega328_h
#define MyHwATMega328_h

#include "MyConfig.h"
#include "MyMessage.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include "MyHw.h"

#ifdef __cplusplus
#include <Arduino.h>
#include <SPI.h>
#endif

enum period_t
{
	SLEEP_15Ms,
	SLEEP_30MS,
	SLEEP_60MS,
	SLEEP_120MS,
	SLEEP_250MS,
	SLEEP_500MS,
	SLEEP_1S,
	SLEEP_2S,
	SLEEP_4S,
	SLEEP_8S,
	SLEEP_FOREVER
};

class MyHwATMega328 : public MyHw
{ 
public:
	MyHwATMega328();

	void init();
	void watchdogReset();
	void reboot();
	unsigned long millisec();
	void sleep(unsigned long ms);
	bool sleep(uint8_t interrupt, uint8_t mode, unsigned long ms);
	uint8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms);
	uint8_t readConfigByte(uint8_t pos);
	void writeConfigByte(uint8_t pos, uint8_t value);
	void readConfigBlock(void* buf, void * pos, size_t length);
	void writeConfigBlock(void* pos, void* buf, size_t length);
#ifdef DEBUG
	void debugPrint(bool isGW, const char *fmt, ... );
#endif
private:
};
#endif
