/* Flood Sensor

 This sketch will light up the LED on Pin 13, when water (anything conductive) bridges the gap in the sensor.
  
   created 05-18-2012 by PvE
   Contributor: epierre
      
 */

#include <MySensor.h>  
#include <SPI.h>

#define CHILD_ID_FLOOD 0
#define FLOOD_SENSOR_ANALOG_PIN 4 // the number of the Flood Sensor pin
#define ledPin =  13;        // the number of the LED pin

MySensor gw;
MyMessage msg(CHILD_ID_FLOOD, V_TRIPPED);

// Variables will change:
int ledState = HIGH;         // the current state of the output pin
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int floodSensorState = 0;         // variable for reading the floodSensors status

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

 void setup() {
  
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Flood Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(FLOOD_SENSOR_ANALOG_PIN, S_WATER_LEAK);  
  
  // initialize the flood Sensor pin as an input:
   pinMode(FLOOD_SENSOR_ANALOG_PIN, INPUT);    
   pinMode(ledPin, OUTPUT);    
 }

 void loop(){
   // read the state of the flood Sensor value:
   floodSensorState = digitalRead(floodSensors);

 // check to see if you just pressed the button 
  // (i.e. the input went from LOW to HIGH),  and you've waited 
  // long enough since the last press to ignore any noise:  

  // If the switch changed, due to noise or pressing:
  if (floodSensorState != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  } 
  
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:
    buttonState = floodSensorState;
  }

   // check if the flood Sensor is wet.
   // if it is, the floodSensorState is HIGH:
   if (buttonState == HIGH) {    
     // turn LED on:    
     digitalWrite(ledPin, LOW);  
	 gw.sendVariable(CHILD_ID, V_TRIPPED, "1");  // Send tripped value to gw 

   }
   else {
     // turn LED off:
     digitalWrite(ledPin, HIGH);
	 gw.sendVariable(CHILD_ID, V_TRIPPED, "0");  // Send un-tripped value to gw 
   }
  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = floodSensorState;
   
 }
