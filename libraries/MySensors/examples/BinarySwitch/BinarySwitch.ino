// Simple binary switch example 
// Connect button or door/window reed switch between 
// digitial I/O pin 3 (BUTTON_PIN below) and GND.

#include <Sensor.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <Bounce2.h>

#define BUTTON_PIN  3  // Arduino Digital I/O pin for button/reed switch

Sensor gw;
Bounce debouncer = Bounce(); 
int oldValue=-1;

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
  // If S_LIGHT is used, remember to update variable type you send in below.
  gw.sendSensorPresentation(BUTTON_PIN, S_DOOR);  
}


//  Check if digital input has changed and send in new value
void loop() 
{
  debouncer.update();
  // Get the update value
  int value = debouncer.read();
 
  if (value != oldValue) {
     // Send in the new value
     gw.sendVariable(BUTTON_PIN, V_TRIPPED, value==HIGH ? "1" : "0");  // Change to V_LIGHT if you use S_LIGHT in presentation above
     oldValue = value;
  }
} 

