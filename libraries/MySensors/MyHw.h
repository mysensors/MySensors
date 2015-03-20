#ifndef MyHw_h
#define MyHw_h

#include <stdint.h>
#include <stddef.h>
#include "MyConfig.h"

/**
 * The Hw contains all hardware specific details of the platform
 */

class MyHw
{ 
public:
	// MyHw constructor
	MyHw();
/*	// The following is defined as macros to save space
  	virtual void init() = 0;
	virtual void watchdogReset() = 0;
	virtual void reboot() = 0;
	virtual unsigned long millisec() = 0;
	virtual uint8_t readConfig(uint8_t pos) = 0;
	virtual void writeConfig(uint8_t pos, uint8_t value) = 0;
	virtual void readConfigBlock(void* buf, void * pos, size_t length) = 0;
	virtual void writeConfigBlock(void* pos, void* buf, uint16_t length) = 0; */
	virtual void sleep(unsigned long ms) = 0;
	virtual bool sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) = 0;
	virtual uint8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) = 0;
	#ifdef DEBUG
	virtual void debugPrint(bool isGW, const char *fmt, ... ) = 0;
	#endif

};

#endif
