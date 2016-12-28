/*
 * Optimized digital functions for AVR microcontrollers
 * based on http://code.google.com/p/digitalwritefast
 */

#ifndef __digitalWriteFast_h_
#define __digitalWriteFast_h_

#if defined(ARDUINO_AVR_MEGA) || defined(ARDUINO_AVR_MEGA1280) || defined(ARDUINO_AVR_MEGA2560) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega1281__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega2561__)
#define __digitalPinToPortReg(__pin) \
	(((__pin) >= 22 && (__pin) <= 29) ? &PORTA : \
	 ((((__pin) >= 10 && (__pin) <= 13) || ((__pin) >= 50 && (__pin) <= 53)) ? &PORTB : \
	  (((__pin) >= 30 && (__pin) <= 37) ? &PORTC : \
	   ((((__pin) >= 18 && (__pin) <= 21) || (__pin) == 38) ? &PORTD : \
	    ((((__pin) <= 3) || (__pin) == 5) ? &PORTE : \
	     (((__pin) >= 54 && (__pin) <= 61) ? &PORTF : \
	      ((((__pin) >= 39 && (__pin) <= 41) || (__pin) == 4) ? &PORTG : \
	       ((((__pin) >= 6 && (__pin) <= 9) || (__pin) == 16 || (__pin) == 17) ? &PORTH : \
	        (((__pin) == 14 || (__pin) == 15) ? &PORTJ : \
	         (((__pin) >= 62 && (__pin) <= 69) ? &PORTK : &PORTL))))))))))
#define __digitalPinToDDRReg(__pin) \
	(((__pin) >= 22 && (__pin) <= 29) ? &DDRA : \
	 ((((__pin) >= 10 && (__pin) <= 13) || ((__pin) >= 50 && (__pin) <= 53)) ? &DDRB : \
	  (((__pin) >= 30 && (__pin) <= 37) ? &DDRC : \
	   ((((__pin) >= 18 && (__pin) <= 21) || (__pin) == 38) ? &DDRD : \
	    ((((__pin) <= 3) || (__pin) == 5) ? &DDRE : \
	     (((__pin) >= 54 && (__pin) <= 61) ? &DDRF : \
	      ((((__pin) >= 39 && (__pin) <= 41) || (__pin) == 4) ? &DDRG : \
	       ((((__pin) >= 6 && (__pin) <= 9) || (__pin) == 16 || (__pin) == 17) ? &DDRH : \
	        (((__pin) == 14 || (__pin) == 15) ? &DDRJ : \
	         (((__pin) >= 62 && (__pin) <= 69) ? &DDRK : &DDRL))))))))))
#define __digitalPinToPINReg(__pin) \
	(((__pin) >= 22 && (__pin) <= 29) ? &PINA : \
	 ((((__pin) >= 10 && (__pin) <= 13) || ((__pin) >= 50 && (__pin) <= 53)) ? &PINB : \
	  (((__pin) >= 30 && (__pin) <= 37) ? &PINC : \
	   ((((__pin) >= 18 && (__pin) <= 21) || (__pin) == 38) ? &PIND : \
	    ((((__pin) <= 3) || (__pin) == 5) ? &PINE : \
	     (((__pin) >= 54 && (__pin) <= 61) ? &PINF : \
	      ((((__pin) >= 39 && (__pin) <= 41) || (__pin) == 4) ? &PING : \
	       ((((__pin) >= 6 && (__pin) <= 9) || (__pin) == 16 || (__pin) == 17) ? &PINH : \
	        (((__pin) == 14 || (__pin) == 15) ? &PINJ : \
	         (((__pin) >= 62 && (__pin) <= 69) ? &PINK : &PINL))))))))))
#define __digitalPinToBit(__pin) \
	(((__pin) >=  7 && (__pin) <=  9) ? (__pin) - 3 : \
	 (((__pin) >= 10 && (__pin) <= 13) ? (__pin) - 6 : \
	  (((__pin) >= 22 && (__pin) <= 29) ? (__pin) - 22 : \
	   (((__pin) >= 30 && (__pin) <= 37) ? 37 - (__pin) : \
	    (((__pin) >= 39 && (__pin) <= 41) ? 41 - (__pin) : \
	     (((__pin) >= 42 && (__pin) <= 49) ? 49 - (__pin) : \
	      (((__pin) >= 50 && (__pin) <= 53) ? 53 - (__pin) : \
	       (((__pin) >= 54 && (__pin) <= 61) ? (__pin) - 54 : \
	        (((__pin) >= 62 && (__pin) <= 69) ? (__pin) - 62 : \
	         (((__pin) == 0 || (__pin) == 15 || (__pin) == 17 || (__pin) == 21) ? 0 : \
	          (((__pin) == 1 || (__pin) == 14 || (__pin) == 16 || (__pin) == 20) ? 1 : \
	           (((__pin) == 19) ? 2 : \
	            (((__pin) == 5 || (__pin) == 6 || (__pin) == 18) ? 3 : \
	             (((__pin) == 2) ? 4 : \
	              (((__pin) == 3 || (__pin) == 4) ? 5 : 7)))))))))))))))
#elif defined(__AVR_ATmega644__) || defined(__AVR_ATmega644P__) || defined(__AVR_ATmega1284__) || defined(__AVR_ATmega1284P__)
#define __digitalPinToPortReg(__pin)	(((__pin) <= 7) ? &PORTB : (((__pin) >= 8 && (__pin) <= 15) ? &PORTD : (((__pin) >= 16 && (__pin) <= 23) ? &PORTC : &PORTA)))
#define __digitalPinToDDRReg(__pin)		(((__pin) <= 7) ? &DDRB : (((__pin) >= 8 && (__pin) <= 15) ? &DDRD : (((__pin) >= 8 && (__pin) <= 15) ? &DDRC : &DDRA)))
#define __digitalPinToPINReg(__pin)		(((__pin) <= 7) ? &PINB : (((__pin) >= 8 && (__pin) <= 15) ? &PIND : (((__pin) >= 8 && (__pin) <= 15) ? &PINC : &PINA)))
#define __digitalPinToBit(__pin)		(((__pin) <= 7) ? (__pin) : (((__pin) >= 8 && (__pin) <= 15) ? (__pin) - 8 : (((__pin) >= 16 && (__pin) <= 23) ? (__pin) - 16 : (__pin) - 24)))
#elif defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega16U4__) || defined(__AVR_ATmega32U4__)
#define __digitalPinToPortReg(__pin)	((((__pin) <= 4) || (__pin) == 6 || (__pin) == 12 || (__pin) == 24 || (__pin) == 25 || (__pin) == 29) ? &PORTD : (((__pin) == 5 || (__pin) == 13) ? &PORTC : (((__pin) >= 18 && (__pin) <= 23)) ? &PORTF : (((__pin) == 7) ? &PORTE : &PORTB)))
#define __digitalPinToDDRReg(__pin)		((((__pin) <= 4) || (__pin) == 6 || (__pin) == 12 || (__pin) == 24 || (__pin) == 25 || (__pin) == 29) ? &DDRD : (((__pin) == 5 || (__pin) == 13) ? &DDRC : (((__pin) >= 18 && (__pin) <= 23)) ? &DDRF : (((__pin) == 7) ? &DDRE : &DDRB)))
#define __digitalPinToPINReg(__pin)		((((__pin) <= 4) || (__pin) == 6 || (__pin) == 12 || (__pin) == 24 || (__pin) == 25 || (__pin) == 29) ? &PIND : (((__pin) == 5 || (__pin) == 13) ? &PINC : (((__pin) >= 18 && (__pin) <= 23)) ? &PINF : (((__pin) == 7) ? &PINE : &PINB)))
#define __digitalPinToBit(__pin)		(((__pin) >= 8 && (__pin) <= 11) ? (__pin) - 4 : (((__pin) >= 18 && (__pin) <= 21) ? 25 - (__pin) : (((__pin) == 0) ? 2 : (((__pin) == 1) ? 3 : (((__pin) == 2) ? 1 : (((__pin) == 3) ? 0 : (((__pin) == 4) ? 4 : (((__pin) == 6) ? 7 : (((__pin) == 13) ? 7 : (((__pin) == 14) ? 3 : (((__pin) == 15) ? 1 : (((__pin) == 16) ? 2 : (((__pin) == 17) ? 0 : (((__pin) == 22) ? 1 : (((__pin) == 23) ? 0 : (((__pin) == 24) ? 4 : (((__pin) == 25) ? 7 : (((__pin) == 26) ? 4 : (((__pin) == 27) ? 5 : 6 )))))))))))))))))))
#elif defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_DUEMILANOVE) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
#if defined(__AVR_ATmega328PB__)
#define __digitalPinToPortReg(__pin)	(((__pin) <= 7) ? &PORTD : (((__pin) >= 8 && (__pin) <= 13) ? &PORTB : (((__pin) >= 14 && (__pin) <= 19) ? &PORTC : &PORTE)))
#define __digitalPinToDDRReg(__pin)		(((__pin) <= 7) ? &DDRD : (((__pin) >= 8 && (__pin) <= 13) ? &DDRB : (((__pin) >= 14 && (__pin) <= 19) ? &DDRC : &DDRE)))
#define __digitalPinToPINReg(__pin)		(((__pin) <= 7) ? &PIND : (((__pin) >= 8 && (__pin) <= 13) ? &PINB : (((__pin) >= 14 && (__pin) <= 19) ? &PINC : &PINE)))
#define __digitalPinToBit(__pin)		(((__pin) <= 7) ? (__pin) : (((__pin) >= 8 && (__pin) <= 13) ? (__pin) - 8 : (((__pin) >= 14 && (__pin) <= 19) ? (__pin) - 14 : (((__pin) >= 20 && (__pin) <= 21) ? (__pin) - 18 : (__pin) - 22))))
#else
#define __digitalPinToPortReg(__pin)	(((__pin) <= 7) ? &PORTD : (((__pin) >= 8 && (__pin) <= 13) ? &PORTB : &PORTC))
#define __digitalPinToDDRReg(__pin)		(((__pin) <= 7) ? &DDRD : (((__pin) >= 8 && (__pin) <= 13) ? &DDRB : &DDRC))
#define __digitalPinToPINReg(__pin)		(((__pin) <= 7) ? &PIND : (((__pin) >= 8 && (__pin) <= 13) ? &PINB : &PINC))
#define __digitalPinToBit(__pin)		(((__pin) <= 7) ? (__pin) : (((__pin) >= 8 && (__pin) <= 13) ? (__pin) - 8 : (__pin) - 14))
#endif
#endif

#if defined(ARDUINO_ARCH_AVR)
#if !defined(__atomicWrite)
#define __atomicWrite(A,P,V) do { if ((A) < 0x40) {bitWrite((A), (P), (V) );} else {uint8_t register saveSreg = SREG;cli();bitWrite((A), (P), (V));SREG = saveSreg;}} while(0)
#endif
#if !defined(digitalWriteFast)
#define digitalWriteFast(__pin, __value) do { if (__builtin_constant_p(__pin) && __builtin_constant_p(__value)) { bitWrite(*__digitalPinToPortReg(__pin), (uint8_t)__digitalPinToBit(__pin), (__value)); } else { digitalWrite((__pin), (__value)); } } while (0)
#endif
#if !defined(pinModeFast)
#define pinModeFast(__pin, __mode) do { if (__builtin_constant_p(__pin) && __builtin_constant_p(__mode) && (__mode!=INPUT_PULLUP)) { bitWrite(*__digitalPinToDDRReg(__pin), (uint8_t)__digitalPinToBit(__pin), (__mode)); } else { pinMode((__pin), (__mode)); } } while (0)
#endif
#if !defined(digitalReadFast)
#define digitalReadFast(__pin) ( (bool) (__builtin_constant_p(__pin) ) ? (( bitRead(*__digitalPinToPINReg(__pin), (uint8_t)__digitalPinToBit(__pin))) ) : digitalRead((__pin)) )
#endif
#endif

#endif