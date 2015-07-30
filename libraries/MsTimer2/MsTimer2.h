#ifndef MsTimer2_h
#define MsTimer2_h

#ifdef __AVR__
#include <avr/interrupt.h>
#else
#error MsTimer2 library only works on AVR architecture
#endif

namespace MsTimer2 {
	extern unsigned long msecs;
	extern void (*func)();
	extern volatile unsigned long count;
	extern volatile char overflowing;
	extern volatile unsigned int tcnt2;
	
	void set(unsigned long ms, void (*f)());
	void start();
	void stop();
	void _overflow();
}

#endif
