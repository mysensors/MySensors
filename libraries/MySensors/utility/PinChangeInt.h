// We use 4-character tabstops, so IN VIM:  <esc>:set ts=4   and  <esc>:set sw=4
// ...that's: ESCAPE key, colon key, then "s-e-t SPACE key t-s-=-4"
//
/*
 * 	This is the PinChangeInt library for the Arduino.

	See google code project for latest, bugs and info http://code.google.com/p/arduino-pinchangeint/
	For more information Refer to avr-gcc header files, arduino source and atmega datasheet.

	This library was inspired by and derived from "johnboiles" (it seems) 
	PCInt Arduino Playground example here: http://www.arduino.cc/playground/Main/PcInt
	If you are the original author, please let us know at the google code page
	
	It provides an extension to the interrupt support for arduino by
	adding pin change interrupts, giving a way for users to have
	interrupts drive off of any pin.

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	(the file gpl.txt is included with the library's zip package)
*/
//-------- define these in your sketch, if applicable ----------------------------------------------------------
//-------- These must go in your sketch ahead of the #include <PinChangeInt.h> statement -----------------------
// You can reduce the memory footprint of this handler by declaring that there will be no pin change interrupts
// on any one or two of the three ports.  If only a single port remains, the handler will be declared inline
// reducing the size and latency of the handler.
// #define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
// #define NO_PORTC_PINCHANGES // to indicate that port c will not be used for pin change interrupts
// #define NO_PORTD_PINCHANGES // to indicate that port d will not be used for pin change interrupts
// --- Mega support ---
// #define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
// #define NO_PORTJ_PINCHANGES // to indicate that port c will not be used for pin change interrupts
// #define NO_PORTK_PINCHANGES // to indicate that port d will not be used for pin change interrupts
// In the Mega, there is no Port C, no Port D.  Instead, you get Port J and Port K.  Port B remains.
// Port J, however, is practically useless because there is only 1 pin available for interrupts.  Most
// of the Port J pins are not even connected to a header connection.  // </end> "Mega Support" notes
// --- Sanguino, Mioduino support ---
// #define NO_PORTA_PINCHANGES // to indicate that port a will not be used for pin change interrupts
// --------------------
//
// Other preprocessor directives...
// You can reduce the code size by 20-50 bytes, and you can speed up the interrupt routine
// slightly by declaring that you don't care if the static variables PCintPort::pinState and/or
// PCintPort::arduinoPin are set and made available to your interrupt routine.
// #define NO_PIN_STATE        // to indicate that you don't need the pinState
// #define NO_PIN_NUMBER       // to indicate that you don't need the arduinoPin
// #define DISABLE_PCINT_MULTI_SERVICE // to limit the handler to servicing a single interrupt per invocation.
// #define GET_PCINT_VERSION   // to enable the uint16_t getPCIintVersion () function.
// The following is intended for testing purposes.  If defined, then a whole host of static variables can be read
// in your interrupt subroutine.  It is not defined by default, and you DO NOT want to define this in
// Production code!:
// #define PINMODE
//-------- define the above in your sketch, if applicable ------------------------------------------------------

/*
	PinChangeInt.h
	---- VERSIONS --- (NOTE TO SELF: Update the PCINT_VERSION define, below) -----------------
Version 2.19 (beta) Tue Nov 20 07:33:37 CST 2012
Version 2.17 (beta) Sat Nov 17 09:46:50 CST 2012
Version 2.11 (beta) Mon Nov 12 09:33:06 CST 2012

	Version 2.01 (beta) Thu Jun 28 12:35:48 CDT 2012

	Version 1.72 Wed Mar 14 18:57:55 CDT 2012

	Version 1.71beta Sat Mar 10 12:57:05 CST 2012

	Version 1.6beta Fri Feb 10 08:48:35 CST 2012

	Version 1.51 Sun Feb  5 23:28:02 CST 2012

	Version 1.5 Thu Feb  2 18:09:49 CST 2012

	Version 1.4 Tue Jan 10 09:41:14 CST 2012

	Version 1.3 Sat Dec  3 22:56:20 CST 2011

	Version 1.2 Sat Dec  3 Sat Dec  3 09:15:52 CST 2011

	Version 1.1 Sat Dec  3 00:06:03 CST 2011
	*/

#ifndef PinChangeInt_h
#define	PinChangeInt_h

#define PCINT_VERSION 2190 // This number MUST agree with the version number, above.

#include "stddef.h"

// Thanks to Maurice Beelen, nms277, Akesson Karlpetter, and Orly Andico for these fixes.
#if defined(ARDUINO) && ARDUINO >= 100
  #include <Arduino.h>
  #include <new.h>
  #include <wiring_private.h> // cby and sbi defined here
#else
  #include <WProgram.h>
  #include <pins_arduino.h>
  #ifndef   LIBCALL_PINCHANGEINT
    #include "../cppfix/cppfix.h"
  #endif
#endif


#undef DEBUG

/*
* Theory: all IO pins on Atmega168 are covered by Pin Change Interrupts.
* The PCINT corresponding to the pin must be enabled and masked, and
* an ISR routine provided.  Since PCINTs are per port, not per pin, the ISR
* must use some logic to actually implement a per-pin interrupt service.
*/

/* Pin to interrupt map:
* D0-D7 = PCINT 16-23 = PCIR2 = PD = PCIE2 = pcmsk2
* D8-D13 = PCINT 0-5 = PCIR0 = PB = PCIE0 = pcmsk0
* A0-A5 (D14-D19) = PCINT 8-13 = PCIR1 = PC = PCIE1 = pcmsk1
*/

#undef	INLINE_PCINT
#define INLINE_PCINT
// Thanks to cserveny...@gmail.com for MEGA support!
#if defined __AVR_ATmega2560__ || defined __AVR_ATmega1280__ || defined __AVR_ATmega1281__ || defined __AVR_ATmega2561__ || defined __AVR_ATmega640__
	#define __USE_PORT_JK
	// Mega does not have PORTA, C or D
	#define NO_PORTA_PINCHANGES
	#define NO_PORTC_PINCHANGES
	#define NO_PORTD_PINCHANGES
	#if ((defined(NO_PORTB_PINCHANGES) && defined(NO_PORTJ_PINCHANGES)) || \
			(defined(NO_PORTJ_PINCHANGES) && defined(NO_PORTK_PINCHANGES)) || \
			(defined(NO_PORTK_PINCHANGES) && defined(NO_PORTB_PINCHANGES)))
		#define	INLINE_PCINT inline
	#endif
#else
	#if defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)
		#ifndef NO_PORTA_PINCHANGES
			#define __USE_PORT_A
		#endif
	#else
		#define NO_PORTA_PINCHANGES
	#endif
	// if defined only D .OR. only C .OR. only B .OR. only A, then inline it
	#if (   (defined(NO_PORTA_PINCHANGES) && defined(NO_PORTB_PINCHANGES) && defined(NO_PORTC_PINCHANGES)) || \
			(defined(NO_PORTA_PINCHANGES) && defined(NO_PORTB_PINCHANGES) && defined(NO_PORTD_PINCHANGES)) || \
			(defined(NO_PORTA_PINCHANGES) && defined(NO_PORTC_PINCHANGES) && defined(NO_PORTD_PINCHANGES)) || \
			(defined(NO_PORTB_PINCHANGES) && defined(NO_PORTC_PINCHANGES) && defined(NO_PORTD_PINCHANGES)) )
		#define	INLINE_PCINT inline
	#endif
#endif

// Provide drop in compatibility with johnboiles PCInt project at
// http://www.arduino.cc/playground/Main/PcInt
#define	PCdetachInterrupt(pin)	PCintPort::detachInterrupt(pin)
#define	PCattachInterrupt(pin,userFunc,mode) PCintPort::attachInterrupt(pin, userFunc,mode)
#define PCgetArduinoPin() PCintPort::getArduinoPin()


typedef void (*PCIntvoidFuncPtr)(void);

class PCintPort {
public:
	PCintPort(int index,int pcindex, volatile uint8_t& maskReg) :
	portInputReg(*portInputRegister(index)),
	portPCMask(maskReg),
	PCICRbit(1 << pcindex),
	portRisingPins(0),
	portFallingPins(0),
	firstPin(NULL)
#ifdef PINMODE
	,intrCount(0)
#endif
	{
		#ifdef FLASH
		ledsetup();
		#endif
	}
	volatile	uint8_t&		portInputReg;
	static		int8_t attachInterrupt(uint8_t pin, PCIntvoidFuncPtr userFunc, int mode);
	static		void detachInterrupt(uint8_t pin);
	INLINE_PCINT void PCint();
	static volatile uint8_t curr;
	#ifndef NO_PIN_NUMBER
	static	volatile uint8_t	arduinoPin;
	#endif
	#ifndef NO_PIN_STATE
	static volatile	uint8_t	pinState;
	#endif
	#ifdef PINMODE
	static volatile uint8_t pinmode;
	static volatile uint8_t s_portRisingPins;
	static volatile uint8_t s_portFallingPins;
	static volatile uint8_t s_lastPinView;
	static volatile uint8_t s_pmask;
	static volatile char s_PORT;
	static volatile uint8_t s_changedPins;
	static volatile uint8_t s_portRisingPins_nCurr;
	static volatile uint8_t s_portFallingPins_nNCurr;
	static volatile uint8_t s_currXORlastPinView;
	volatile uint8_t intrCount;
	static volatile uint8_t s_count;
	static volatile uint8_t pcint_multi;
	static volatile uint8_t PCIFRbug;
	#endif
	#ifdef FLASH
	static void ledsetup(void);
	#endif

protected:
	class PCintPin {
	public:
		PCintPin() :
		PCintFunc((PCIntvoidFuncPtr)NULL),
		mode(0) {}
		PCIntvoidFuncPtr PCintFunc;
		uint8_t 	mode;
		uint8_t		mask;
		uint8_t arduinoPin;
		PCintPin* next;
	};
	void 		enable(PCintPin* pin, PCIntvoidFuncPtr userFunc, uint8_t mode);
	int8_t		addPin(uint8_t arduinoPin,PCIntvoidFuncPtr userFunc, uint8_t mode);
	volatile	uint8_t&		portPCMask;
	const		uint8_t			PCICRbit;
	volatile	uint8_t			portRisingPins;
	volatile	uint8_t			portFallingPins;
	volatile uint8_t		lastPinView;
	PCintPin*	firstPin;
};

#endif // #ifndef PinChangeInt_h *******************************************************************
