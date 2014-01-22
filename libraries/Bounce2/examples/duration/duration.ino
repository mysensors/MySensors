#include <Bounce2.h>

/* 
DESCRIPTION
====================
Simple example of the bounce library that sets three states for a button through the duration of the press.
Open the Serial Monitor (57600 baud) for debug messages.

CIRCUIT
====================
https://raw.github.com/thomasfredericks/Bounce-Arduino-Wiring/master/Bounce/examples/circuit-bounce-change-duration-retrigger.png
*/

#define BUTTON_PIN 2
#define LED_PIN 13

// Instantiate a Bounce object
Bounce debouncer = Bounce(); 

// The current buttonState
// 0 : released
// 1 : pressed less than 2 seconds
// 2 : pressed longer than 2 seconds
int buttonState; 
unsigned long buttonPressTimeStamp;

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
  
}

void loop() {
 // Update the debouncer and get the changed state
  boolean changed = debouncer.update();


  
  if ( changed ) {
       // Get the update value
     int value = debouncer.read();
    if ( value == HIGH) {
       digitalWrite(LED_PIN, HIGH );
   
       buttonState = 0;
       Serial.println("Button released (state 0)");
   
   } else {
         digitalWrite(LED_PIN, LOW );
         buttonState = 1;
         Serial.println("Button pressed (state 1)");
         buttonPressTimeStamp = millis();
     
   }
  }
  
  if  ( buttonState == 1 ) {
    if ( millis() - buttonPressTimeStamp >= 2000 ) {
        buttonState = 2;
       Serial.println("Button held for two seconds (state 2)");
    }
  }
 
 
}

