#ifndef MyHwATMega328_h
#define MyHwATMega328_h

#include "MyHw.h"
#include "MyConfig.h"
#include "MyMessage.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/crc16.h>



#ifdef __cplusplus
#include <Arduino.h>
#include <SPI.h>
#endif

// Define these as macros to save valuable space
#define hw_init() Serial.begin(BAUD_RATE)
#define hw_watchdogReset() wdt_reset()
#define hw_reboot() wdt_enable(WDTO_15MS); while (1)
#define hw_millis() millis()
#define hw_readConfig(__pos) (eeprom_read_byte((uint8_t*)(__pos)))
#define hw_writeConfig(__pos, __value) (eeprom_update_byte((uint8_t*)(__pos), (__value)))
#define hw_readConfigBlock(__buf, __pos, __length) (eeprom_read_block((__buf), (void*)(__pos), (__length)))
#define hw_writeConfigBlock(__pos, __buf, __length) (eeprom_write_block((void*)(__pos), (void*)__buf, (__length)))



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

/*	void init();
	void watchdogReset();
	void reboot();
	unsigned long millis();
	uint8_t readConfig(uint8_t pos);
	void writeConfig(uint8_t pos, uint8_t value);
	void readConfigBlock(void* buf, void * pos, size_t length);
	void writeConfigBlock(void* pos, void* buf, size_t length); */

	void sleep(unsigned long ms);
	bool sleep(uint8_t interrupt, uint8_t mode, unsigned long ms);
	uint8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms);
#ifdef DEBUG
	void debugPrint(bool isGW, const char *fmt, ... );
#endif
private:
};
#endif
