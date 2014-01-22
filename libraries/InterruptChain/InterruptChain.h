/*
 * IntereruptChain library v1.3.0 (20130601)
 *
 * Copyright 2011-2013 by Randy Simons http://randysimons.nl/
 *
 * License: GPLv3. See license.txt
 */

#ifndef InterruptChain_h
#define InterruptChain_h

#include <Arduino.h>

// Arduino Mega has 6 interrupts. For smaller Arduinos and / or to save a few bytes memory you can lower it to 2 or even 1. Don't go higher than 6 tho.
#define MAX_INTERRUPTS 6

typedef void (*InterruptCallback)();

/**
 * For internal use
 */
class InterruptChainLink {
	public:
		InterruptChainLink *next;
		InterruptCallback callback;

		void init(InterruptCallback callbackIn, InterruptChainLink *nextIn);

		void processInterrupt();    
};

class InterruptChain {  
	public:
		/**
		 * Add an interrupt handler on interrupt pin interruptNr. The callback is of the same type as
		 * Arduino's standard attachInterrupt().
		 *
		 * So, instead of attachInterrupt(0, callback, CHANGE); you can use
		 * InterruptChain::addInterruptCallback(0, callback);
		 * 
		 * You can add more than one callback to a single interrupt! The callbacks are called in 
		 * the reversed order in which they were added.
		 * addInterruptCallback will also call this.enable(interruptNr).
		 *
		 */
		static void addInterruptCallback(byte interruptNr, InterruptCallback callbackIn);
		
		/**
		 * Enables interrupt handling by InterruptChain for given interrupt pin. Note that this
		 * is different from Arduino's interrupts(), which will enable _all_ interrupts on the CPU.
		 *
		 * @param interruptNr The interrupt pin number for which to enable handling.
		 */
		static void enable(byte interruptNr);
		
		/**
		 * Disables interrupt handling by InterruptChain for given interrupt pin. Note that this 
		 * is different from Arduino's noInterrupts(), which will disable _all_ interrupts on the CPU.
		 *
		 * @param interruptNr The interrupt pin number for which to disable handling.
		 */
		static void disable(byte interruptNr);
		
		/**
		 * Set the interrupt mode for given interrupt pin. By default the interrupt mode is CHANGE. 
		 * If you need this changed, best to call setMode before adding interrupt handlers.
		 * 
		 * @param interruptNr Interrupt to set
		 * @param modeIn LOW, CHANGE, RISING or FALLING
		 */
		static void setMode(byte interruptNr, byte modeIn);
	
	private:
		static InterruptChainLink *chain[MAX_INTERRUPTS];
		static byte mode[MAX_INTERRUPTS];

		static void processInterrupt0();

		static void processInterrupt1();

		static void processInterrupt2();

		static void processInterrupt3();

		static void processInterrupt4();

		static void processInterrupt5();
};

#endif
