#include <Sleep_n0m1.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <Sensor.h>  

#define DIGITAL_INPUT_SOIL_SENSOR 3   // Digital input did you attach your soil sensor.  
#define INTERRUPT DIGITAL_INPUT_SOIL_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 0   // Id of the sensor child

Sensor gw;
Sleep sleep;
int lastSoilValue = -1;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Soil Moisture Sensor", "1.0");

  pinMode(DIGITAL_INPUT_SOIL_SENSOR, INPUT);      // sets the soil sensor digital pin as input
  // Register all sensors to gw (they will be created as child devices)  
  gw.sendSensorPresentation(CHILD_ID, S_MOTION);
}
 
void loop()     
{     
  // Read digital soil value
  int soilValue = digitalRead(DIGITAL_INPUT_SOIL_SENSOR); // 1 = Not triggered, 0 = In soil with water 
  if (soilValue != lastSoilValue) {
    Serial.println(soilValue);
    gw.sendVariable(CHILD_ID, V_TRIPPED, soilValue==0?"1":"0");  // Send the inverse to gw as tripped should be when no water in soil
    lastSoilValue = soilValue;
  }
  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  gw.powerDown();
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepInterrupt(INTERRUPT,CHANGE);
}


