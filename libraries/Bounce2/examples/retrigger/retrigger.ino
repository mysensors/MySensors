#include <Bounce2.h>

/* 
DESCRIPTION
====================
Simple example of the bounce library that shows how to retrigger an event when a button is held down.
In this case, the debug LED will blink every 500 ms as long as the button is held down.
Open the Serial Monitor (57600 baud) for debug messages.

CIRCUIT
====================
https://raw.github.com/thomasfredericks/Bounce-Arduino-Wiring/master/Bounce/examples/circuit-bounce-change-duration-retrigger.png
*/

#define BUTTON_PIN 2
#define LED_PIN 13

// Instantiate a Bounce object
Bounce debouncer = Bounce(); 

int buttonState;
unsigned long buttonPressTimeStamp;

int ledState;

void setup() {
  
  Serial.begin(57600);
  
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
  
  //Setup the LED
  pinMode(LED_PIN,OUTPUT);
  digitalWrite(LED_PIN,ledState);
  
}

void loop() {
 // Update the debouncer and get the changed state
  boolean changed = debouncer.update();


  
  if ( changed ) {
       // Get the update value
     int value = debouncer.read();
    if ( value == HIGH) {
       ledState = LOW;
       digitalWrite(LED_PIN, ledState );
   
       buttonState = 0;
       Serial.println("Button released (state 0)");
   
   } else {
          ledState = HIGH;
       digitalWrite(LED_PIN, ledState );
       
         buttonState = 1;
         Serial.println("Button pressed (state 1)");
         buttonPressTimeStamp = millis();
     
   }
  }
  
  if  ( buttonState == 1 ) {
    if ( millis() - buttonPressTimeStamp >= 500 ) {
         buttonPressTimeStamp = millis();
         if ( ledState == HIGH ) ledState = LOW;
         else if ( ledState == LOW ) ledState = HIGH;
         digitalWrite(LED_PIN, ledState );
        Serial.println("Retriggering button");
    }
  }
 
 
}


