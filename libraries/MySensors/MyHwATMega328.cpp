#include "MyHw.h"
#include "MyHwATMega328.h"

int8_t pinIntTrigger = 0;
void wakeUp()	 //place to send the interrupts
{
	pinIntTrigger = 1;
}
void wakeUp2()	 //place to send the second interrupts
{
	pinIntTrigger = 2;
}

MyHwATMega328::MyHwATMega328() : MyHw()
{
}


void MyHwATMega328::init() {
	Serial.begin(BAUD_RATE);
}

void MyHwATMega328::watchdogReset() {
	wdt_reset();
}

void MyHwATMega328::reboot() {
	wdt_enable(WDTO_15MS);
	for (;;);
}

unsigned long MyHwATMega328::millisec() {
	return millis();
}



#define	bodOn(mode)	\
do { 						\
      set_sleep_mode(mode); \
      cli();				\
      sleep_enable();		\
      sei();				\
      sleep_cpu();			\
      sleep_disable();		\
      sei();				\
} while (0);

#if defined __AVR_ATmega328P__
#define	bodOff(mode)\
do { 						\
      set_sleep_mode(mode); \
      cli();				\
      sleep_enable();		\
			sleep_bod_disable(); \
      sei();				\
      sleep_cpu();			\
      sleep_disable();		\
      sei();				\
} while (0);
#endif

void powerDown(period_t period) {

	ADCSRA &= ~(1 << ADEN);

	if (period != SLEEP_FOREVER)
	{
		wdt_enable(period);
		WDTCSR |= (1 << WDIE);
	}
	#if defined __AVR_ATmega328P__
		bodOff(SLEEP_MODE_PWR_DOWN);
	#else
		bodOn(SLEEP_MODE_PWR_DOWN);
	#endif

	ADCSRA |= (1 << ADEN);
}

void MyHwATMega328::sleep(unsigned long ms) {
	// Let serial prints finish (debug, log etc)
	Serial.flush();
	pinIntTrigger = 0;
	while (!pinIntTrigger && ms >= 8000) { powerDown(SLEEP_8S); ms -= 8000; }
	if (!pinIntTrigger && ms >= 4000)    { powerDown(SLEEP_4S); ms -= 4000; }
	if (!pinIntTrigger && ms >= 2000)    { powerDown(SLEEP_2S); ms -= 2000; }
	if (!pinIntTrigger && ms >= 1000)    { powerDown(SLEEP_1S); ms -= 1000; }
	if (!pinIntTrigger && ms >= 500)     { powerDown(SLEEP_500MS); ms -= 500; }
	if (!pinIntTrigger && ms >= 250)     { powerDown(SLEEP_250MS); ms -= 250; }
	if (!pinIntTrigger && ms >= 125)     { powerDown(SLEEP_120MS); ms -= 120; }
	if (!pinIntTrigger && ms >= 64)      { powerDown(SLEEP_60MS); ms -= 60; }
	if (!pinIntTrigger && ms >= 32)      { powerDown(SLEEP_30MS); ms -= 30; }
	if (!pinIntTrigger && ms >= 16)      { powerDown(SLEEP_15Ms); ms -= 15; }
}

bool MyHwATMega328::sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	// Let serial prints finish (debug, log etc)
	bool pinTriggeredWakeup = true;
	attachInterrupt(interrupt, wakeUp, mode);
	if (ms>0) {
		pinIntTrigger = 0;
		sleep(ms);
		if (0 == pinIntTrigger) {
			pinTriggeredWakeup = false;
		}
	} else {
		Serial.flush();
		powerDown(SLEEP_FOREVER);
	}
	detachInterrupt(interrupt);
	return pinTriggeredWakeup;
}

uint8_t MyHwATMega328::sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	int8_t retVal = 1;
	attachInterrupt(interrupt1, wakeUp, mode1);
	attachInterrupt(interrupt2, wakeUp2, mode2);
	if (ms>0) {
		pinIntTrigger = 0;
		sleep(ms);
		if (0 == pinIntTrigger) {
			retVal = -1;
		}
	} else {
		Serial.flush();
		powerDown(SLEEP_FOREVER);
	}
	detachInterrupt(interrupt1);
	detachInterrupt(interrupt2);

	if (1 == pinIntTrigger) {
		retVal = (int8_t)interrupt1;
	} else if (2 == pinIntTrigger) {
		retVal = (int8_t)interrupt2;
	}
	return retVal;
}

uint8_t MyHwATMega328::readConfigByte(uint8_t pos) {
	return eeprom_read_byte((uint8_t*)pos);
}

void MyHwATMega328::writeConfigByte(uint8_t pos, uint8_t value) {
	eeprom_update_byte((uint8_t*)pos, value);
}

void MyHwATMega328::readConfigBlock(void* buf, void * pos, size_t length) {
	eeprom_read_block(buf, (void*)pos, length);
}

void MyHwATMega328::writeConfigBlock(void* pos, void* buf, size_t length) {
	eeprom_write_block((void*)pos, (void*)buf, length);
}


#ifdef DEBUG
void MyHwATMega328::debugPrint(bool isGW, const char *fmt, ... ) {
	char fmtBuffer[300];
	if (isGW) {
		// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
		snprintf_P(fmtBuffer, 299, PSTR("0;0;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
		Serial.print(fmtBuffer);
	}
	va_list args;
	va_start (args, fmt );
	va_end (args);
	if (isGW) {
		// Truncate message if this is gateway node
		vsnprintf_P(fmtBuffer, 60, fmt, args);
		fmtBuffer[59] = '\n';
		fmtBuffer[60] = '\0';
	} else {
		vsnprintf_P(fmtBuffer, 299, fmt, args);
	}
	va_end (args);
	Serial.print(fmtBuffer);
	Serial.flush();

	//Serial.write(freeRam());
}
#endif

