#include <MySensor.h>  
#include <Sleep_n0m1.h>
#include <SPI.h>
#include <RF24.h>

#define DIGITAL_INPUT_SENSOR 3   // The digital input you attached your motion sensor.  (Only 2 and 3 generates interrupt!)
#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 1   // Id of the sensor child

MySensor gw;
Sleep sleep;
// Initialize motion message
MyMessage msg(CHILD_ID, V_TRIPPED);

void setup()  
{  
  //eeprom_write_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, 1);
  //eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, 255);
  //delay(2000);
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Motion Sensor", "1.0");

  pinMode(DIGITAL_INPUT_SENSOR, INPUT);      // sets the motion sensor digital pin as input
  // Register all sensors to gw (they will be created as child devices)
  delay(500);
  gw.present(CHILD_ID, S_MOTION);
  delay(1000);
  
}

void loop()     
{     
  // Read digital motion value
  boolean tripped = digitalRead(DIGITAL_INPUT_SENSOR) == HIGH; 
        
  Serial.println(tripped);
  gw.send(msg.set(tripped?"1":"0"));  // Send tripped value to gw 
 
  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  delay(200); //delay to allow serial to fully print before sleep
  gw.powerDown();
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepInterrupt(INTERRUPT,CHANGE);
}


