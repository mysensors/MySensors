#include <SPI.h>
#include <MySensor.h>  

#define DIGITAL_INPUT_SOIL_SENSOR 3   // Digital input did you attach your soil sensor.  
#define INTERRUPT DIGITAL_INPUT_SOIL_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 0   // Id of the sensor child

MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);
int lastSoilValue = -1;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Soil Moisture Sensor", "1.0");
  // sets the soil sensor digital pin as input
  pinMode(DIGITAL_INPUT_SOIL_SENSOR, INPUT);      
  // Register all sensors to gw (they will be created as child devices)  
  gw.present(CHILD_ID, S_MOTION);
}
 
void loop()     
{     
  // Read digital soil value
  int soilValue = digitalRead(DIGITAL_INPUT_SOIL_SENSOR); // 1 = Not triggered, 0 = In soil with water 
  if (soilValue != lastSoilValue) {
    Serial.println(soilValue);
    gw.send(msg.set(soilValue==0?1:0));  // Send the inverse to gw as tripped should be when no water in soil
    lastSoilValue = soilValue;
  }
  // Power down the radio and arduino until digital input changes.
  gw.sleep(INTERRUPT,CHANGE);
}


