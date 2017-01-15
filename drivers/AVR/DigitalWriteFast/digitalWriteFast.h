/*
 * Optimized digital functions for AVR microcontrollers
 * based on http://code.google.com/p/digitalwritefast
 */

#ifndef __digitalWriteFast_h_
#define __digitalWriteFast_h_

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_DUEMILANOVE) || defined(__AVR_ATmega328__) || defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__) || defined (__AVR_ATmega168__)
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
#define digitalWriteFast(__pin, __value) do { if (__builtin_constant_p(__pin) && __builtin_constant_p(__value)) { bitWrite(*__digitalPinToPortReg(__pin), (uint8_t)__digitalPinToBit(__pin), (__value)); } else { digitalWrite((__pin), (__value)); } } while (0)
#define pinModeFast(__pin, __mode) do { if (__builtin_constant_p(__pin) && __builtin_constant_p(__mode) && (__mode!=INPUT_PULLUP)) { bitWrite(*__digitalPinToDDRReg(__pin), (uint8_t)__digitalPinToBit(__pin), (__mode)); } else { pinMode((__pin), (__mode)); } } while (0)
#define digitalReadFast(__pin) ( (bool) (__builtin_constant_p(__pin) ) ? (( bitRead(*__digitalPinToPINReg(__pin), (uint8_t)__digitalPinToBit(__pin))) ) : digitalRead((__pin)) )
#else
// for all other archs use built-in pin access functions
#define digitalWriteFast(__pin, __value) digitalWrite(__pin, __value)
#define pinModeFast(__pin, __value) pinMode(__pin, __value)
#define digitalReadFast(__pin) digitalRead(__pin)
#endif

#endif