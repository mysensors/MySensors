// Interrupt driven binary switch example 
// Author: Patrick 'Anticimex' Fallberg
// Connect button or door/window reed switch between 
// digitial I/O pin 3 (BUTTON_PIN below) and GND.
// This example is designed to fit Arduino Nano/Pro Mini

#include <MySensor.h>
#include <SPI.h>

#define CHILD_ID 3
#define BUTTON_PIN 3  // Arduino Digital I/O pin for button/reed switch

#if (BUTTON_PIN < 2 || BUTTON_PIN > 3)
#error BUTTON_PIN must be either 2 or 3 for interrupts to work
#endif

MySensor gw;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(CHILD_ID,V_TRIPPED);

void setup()  
{  
  gw.begin();

  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Binary Sensor", "1.0");

  // Register binary input sensor to gw (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  gw.present(CHILD_ID, S_DOOR);  
}

// Loop will iterate on changes on the BUTTON_PIN
void loop() 
{
  uint8_t value;
  static uint8_t sentValue=2;

  // Short delay to allow button to properly settle
  gw.sleep(5);
  
  value = digitalRead(BUTTON_PIN);
  
  if (value != sentValue) {
     // Value has changed from last transmission, send the updated value
     gw.send(msg.set(value==HIGH ? 1 : 0));
     sentValue = value;
  }

  // Sleep until something happens with the sensor
  gw.sleep(BUTTON_PIN-2, CHANGE, 0);
} 

