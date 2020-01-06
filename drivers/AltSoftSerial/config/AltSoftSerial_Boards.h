/* An Alternative Software Serial Library
 * http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
 * Copyright (c) 2014 PJRC.COM, LLC, Paul Stoffregen, paul@pjrc.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


// Teensy 2.0
//
#if defined(__AVR_ATmega32U4__) && defined(CORE_TEENSY)

//#define ALTSS_USE_TIMER1
//#define INPUT_CAPTURE_PIN		22 // receive
//#define OUTPUT_COMPARE_A_PIN		14 // transmit
//#define OUTPUT_COMPARE_B_PIN		15 // unusable PWM
//#define OUTPUT_COMPARE_C_PIN		 4 // unusable PWM

#define ALTSS_USE_TIMER3
#define INPUT_CAPTURE_PIN		10 // receive
#define OUTPUT_COMPARE_A_PIN		 9 // transmit



// Teensy++ 2.0
//
#elif defined(__AVR_AT90USB1286__) && defined(CORE_TEENSY)

#define ALTSS_USE_TIMER1
#define INPUT_CAPTURE_PIN		 4 // receive
#define OUTPUT_COMPARE_A_PIN		25 // transmit
#define OUTPUT_COMPARE_B_PIN		26 // unusable PWM
#define OUTPUT_COMPARE_C_PIN		27 // unusable PWM

//#define ALTSS_USE_TIMER3
//#define INPUT_CAPTURE_PIN		17 // receive
//#define OUTPUT_COMPARE_A_PIN		16 // transmit
//#define OUTPUT_COMPARE_B_PIN		15 // unusable PWM
//#define OUTPUT_COMPARE_C_PIN		14 // unusable PWM


// Teensy 3.x
//
#elif defined(__MK20DX128__) || defined(__MK20DX256__) || defined(__MK64FX512__) || defined(__MK66FX1M0__)
#define ALTSS_USE_FTM0
#define INPUT_CAPTURE_PIN		20 // receive       (FTM0_CH5)
#define OUTPUT_COMPARE_A_PIN		21 // transmit      (FTM0_CH6)
#define OUTPUT_COMPARE_B_PIN		22 // unusable PWM  (FTM0_CH0)
#define OUTPUT_COMPARE_C_PIN		23 // PWM usable fixed freq
#define OUTPUT_COMPARE_D_PIN		 5 // PWM usable fixed freq
#define OUTPUT_COMPARE_E_PIN		 6 // PWM usable fixed freq
#define OUTPUT_COMPARE_F_PIN		 9 // PWM usable fixed freq
#define OUTPUT_COMPARE_G_PIN		10 // PWM usable fixed freq


// Wiring-S
//
#elif defined(__AVR_ATmega644P__) && defined(WIRING)

#define ALTSS_USE_TIMER1
#define INPUT_CAPTURE_PIN		 6 // receive
#define OUTPUT_COMPARE_A_PIN		 5 // transmit
#define OUTPUT_COMPARE_B_PIN		 4 // unusable PWM



// Arduino Uno, Duemilanove, LilyPad, etc
//
#elif defined(__AVR_ATmega168__) || defined(__AVR_ATmega328P__)

#define ALTSS_USE_TIMER1
#define INPUT_CAPTURE_PIN		 8 // receive
#define OUTPUT_COMPARE_A_PIN		 9 // transmit
#define OUTPUT_COMPARE_B_PIN		10 // unusable PWM


// Arduino Leonardo & Yun (from Cristian Maglie)
//
#elif defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_LEONARDO) || defined(__AVR_ATmega32U4__)

//#define ALTSS_USE_TIMER1
//#define INPUT_CAPTURE_PIN		4  // receive
//#define OUTPUT_COMPARE_A_PIN	9 // transmit
//#define OUTPUT_COMPARE_B_PIN	10 // unusable PWM
//#define OUTPUT_COMPARE_C_PIN	11 // unusable PWM

#define ALTSS_USE_TIMER3
#define INPUT_CAPTURE_PIN		13 // receive
#define OUTPUT_COMPARE_A_PIN		5 // transmit


// Arduino Mega
//
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

//#define ALTSS_USE_TIMER4
//#define INPUT_CAPTURE_PIN		49 // receive
//#define OUTPUT_COMPARE_A_PIN		 6 // transmit
//#define OUTPUT_COMPARE_B_PIN		 7 // unusable PWM
//#define OUTPUT_COMPARE_C_PIN		 8 // unusable PWM

#define ALTSS_USE_TIMER5
#define INPUT_CAPTURE_PIN		48 // receive
#define OUTPUT_COMPARE_A_PIN		46 // transmit
#define OUTPUT_COMPARE_B_PIN		45 // unusable PWM
#define OUTPUT_COMPARE_C_PIN		44 // unusable PWM



// EnviroDIY Mayfly, Sodaq Mbili
#elif defined ARDUINO_AVR_ENVIRODIY_MAYFLY || defined ARDUINO_AVR_SODAQ_MBILI
#define ALTSS_USE_TIMER1
#define INPUT_CAPTURE_PIN		6 // receive
#define OUTPUT_COMPARE_A_PIN	5 // transmit
#define OUTPUT_COMPARE_B_PIN	4 // unusable PWM



// Sanguino, Mighty 1284
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) || defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega1284__)
#define ALTSS_USE_TIMER1
#define INPUT_CAPTURE_PIN		14 // receive
#define OUTPUT_COMPARE_A_PIN		13 // transmit
#define OUTPUT_COMPARE_B_PIN		12 // unusable PWM



// Unknown board
#else
#error "Please define your board timer and pins"
#endif
