

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
#elif defined(ARDUINO_AVR_YUN) || defined(ARDUINO_AVR_LEONARDO)

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



// Sanguino
#elif defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__)
 #define ALTSS_USE_TIMER1
 #define INPUT_CAPTURE_PIN		14 // receive
 #define OUTPUT_COMPARE_A_PIN		13 // transmit
 #define OUTPUT_COMPARE_B_PIN		12 // unusable PWM


// Unknown board
#else
#error "Please define your board timer and pins"
#endif

