#include <Bounce2.h>

/* 
DESCRIPTION
====================
Simple example of the bounce library that switches the debug LED when a button is pressed.

CIRCUIT
====================
https://raw.github.com/thomasfredericks/Bounce-Arduino-Wiring/master/Bounce/examples/circuit-bounce-change-duration-retrigger.png
*/


#define BUTTON_PIN 2
#define LED_PIN 13



// Instantiate a Bounce object
Bounce debouncer = Bounce(); 

void setup() {
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
  
  //Setup the LED
  pinMode(LED_PIN,OUTPUT);
  
}

void loop() {
 // Update the debouncer
  debouncer.update();
 
 // Get the update value
 int value = debouncer.read();
 
 // Turn on or off the LED
 if ( value == HIGH) {
   digitalWrite(LED_PIN, HIGH );
 } else {
    digitalWrite(LED_PIN, LOW );
 }
 
}

