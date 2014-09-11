char dummyvar; // to get Arduinoi IDE to include core headers properly

/*
  MsTimer2 is a small and very easy to use library to interface Timer2 with
  humans. It's called MsTimer2 because it "hardcodes" a resolution of 1
  millisecond on timer2
  For Details see: http://www.arduino.cc/playground/Main/MsTimer2
*/
#include <MsTimer2.h>

// Switch on LED on and off each half second

#if defined(ARDUINO) && ARDUINO >= 100
const int led_pin = LED_BUILTIN;	// 1.0 built in LED pin var
#else
#if defined(CORE_LED0_PIN)
const int led_pin = CORE_LED0_PIN;	// 3rd party LED pin define
#else
const int led_pin = 13;			// default to pin 13
#endif
#endif


void flash()
{
  static boolean output = HIGH;
  
  digitalWrite(led_pin, output);
  output = !output;
}

void setup()
{
  pinMode(led_pin, OUTPUT);

  MsTimer2::set(500, flash); // 500ms period
  MsTimer2::start();
}

void loop()
{
}
