// Simple binary switch example 
// Connect button or door/window reed switch between 
// digitial I/O pin 3 (BUTTON_PIN below) and GND.

#include <MySensor.h>
#include <SPI.h>
#include <Bounce2.h>

#define CHILD_ID 3
#define BUTTON_PIN  3  // Arduino Digital I/O pin for button/reed switch

MySensor gw;
Bounce debouncer = Bounce(); 
int oldValue=-1;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(CHILD_ID,V_TRIPPED);

void setup()  
{  
  gw.begin();

 // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
  
  // Register binary input sensor to gw (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  gw.present(CHILD_ID, S_DOOR);  
}


//  Check if digital input has changed and send in new value
void loop() 
{
  debouncer.update();
  // Get the update value
  int value = debouncer.read();
 
  if (value != oldValue) {
     // Send in the new value
     gw.send(msg.set(value==HIGH ? 1 : 0));
     oldValue = value;
  }
} 

