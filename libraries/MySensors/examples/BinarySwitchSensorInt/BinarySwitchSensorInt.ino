// Interrupt driven binary switch example with dual interrupts
// Author: Patrick 'Anticimex' Fallberg
// Connect one button or door/window reed switch between 
// digitial I/O pin 3 (BUTTON_PIN below) and GND and the other
// one in similar fashion on digital I/O pin 2.
// This example is designed to fit Arduino Nano/Pro Mini

#include <MySensor.h>
#include <SPI.h>

#define SKETCH_NAME "Binary Sensor"
#define SKETCH_MAJOR_VER "1"
#define SKETCH_MINOR_VER "0"

#define PRIMARY_CHILD_ID 3
#define SECONDARY_CHILD_ID 4

#define PRIMARY_BUTTON_PIN 2   // Arduino Digital I/O pin for button/reed switch
#define SECONDARY_BUTTON_PIN 3 // Arduino Digital I/O pin for button/reed switch

#if (PRIMARY_BUTTON_PIN < 2 || PRIMARY_BUTTON_PIN > 3)
#error PRIMARY_BUTTON_PIN must be either 2 or 3 for interrupts to work
#endif
#if (SECONDARY_BUTTON_PIN < 2 || SECONDARY_BUTTON_PIN > 3)
#error SECONDARY_BUTTON_PIN must be either 2 or 3 for interrupts to work
#endif
#if (PRIMARY_BUTTON_PIN == SECONDARY_BUTTON_PIN)
#error PRIMARY_BUTTON_PIN and BUTTON_PIN2 cannot be the same
#endif
#if (PRIMARY_CHILD_ID == SECONDARY_CHILD_ID)
#error PRIMARY_CHILD_ID and SECONDARY_CHILD_ID cannot be the same
#endif
 
MySensor sensor_node;

// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(PRIMARY_CHILD_ID, V_TRIPPED);
MyMessage msg2(SECONDARY_CHILD_ID, V_TRIPPED);

void setup()  
{  
  sensor_node.begin();

  // Setup the buttons
  pinMode(PRIMARY_BUTTON_PIN, INPUT);
  pinMode(SECONDARY_BUTTON_PIN, INPUT);

  // Activate internal pull-ups
  digitalWrite(PRIMARY_BUTTON_PIN, HIGH);
  digitalWrite(SECONDARY_BUTTON_PIN, HIGH);
  
  // Send the sketch version information to the gateway and Controller
  sensor_node.sendSketchInfo(SKETCH_NAME, SKETCH_MAJOR_VER"."SKETCH_MINOR_VER);

  // Register binary input sensor to sensor_node (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  sensor_node.present(PRIMARY_CHILD_ID, S_DOOR);  
  sensor_node.present(SECONDARY_CHILD_ID, S_DOOR);  
}

// Loop will iterate on changes on the BUTTON_PINs
void loop() 
{
  uint8_t value;
  static uint8_t sentValue=2;
  static uint8_t sentValue2=2;

  // Short delay to allow buttons to properly settle
  sensor_node.sleep(5);
  
  value = digitalRead(PRIMARY_BUTTON_PIN);
  
  if (value != sentValue) {
     // Value has changed from last transmission, send the updated value
     sensor_node.send(msg.set(value==HIGH ? 1 : 0));
     sentValue = value;
  }

  value = digitalRead(SECONDARY_BUTTON_PIN);
  
  if (value != sentValue2) {
     // Value has changed from last transmission, send the updated value
     sensor_node.send(msg2.set(value==HIGH ? 1 : 0));
     sentValue2 = value;
  }

  // Sleep until something happens with the sensor
  sensor_node.sleep(PRIMARY_BUTTON_PIN-2, CHANGE, SECONDARY_BUTTON_PIN-2, CHANGE, 0);
} 
