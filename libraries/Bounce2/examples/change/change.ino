#include <Bounce2.h>

// Detect the falling edge

// Circuit :
// https://raw.github.com/thomasfredericks/Bounce-Arduino-Wiring/master/Bounce/examples/circuit-bounce-change-duration-retrigger.png

#define BUTTON_PIN 2
#define LED_PIN 13

int ledState = LOW;


// Instantiate a Bounce object
Bounce debouncer = Bounce(); 

void setup() {
   // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup Bounce object
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(500);
  
  //Setup the LED
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,ledState);
  
}

void loop() {

  
  boolean stateChanged = debouncer.update();
  int state = debouncer.read();
  
  // Detect the falling edge
   if ( stateChanged && state == LOW ) {
    
       if ( ledState == LOW ) {
         ledState = HIGH;
       } else {
         ledState = LOW;
       }
       digitalWrite(LED_PIN,ledState);
     
   }
}

