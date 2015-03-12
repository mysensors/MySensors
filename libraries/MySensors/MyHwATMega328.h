#ifndef MyHwATMega328_h
#define MyHwATMega328_h

#include "MyConfig.h"
#include "MyMessage.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include "utility/LowPower.h"

#include "MyHw.h"

#ifdef __cplusplus
#include <Arduino.h>
#include <SPI.h>
#endif


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
