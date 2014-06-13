#include <SPI.h>
#include <RF24.h>
#include <MySensor.h>  

#define CHILD_ID_LIGHT 0
#define LIGHT_SENSOR_ANALOG_PIN 0

unsigned long SLEEP_TIME = 30; // Sleep time between reads (in seconds)

MySensor gw;
MyMessage msg(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
int lastLightLevel;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Light Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
}

void loop()      
{     
  int lightLevel = (1023-analogRead(LIGHT_SENSOR_ANALOG_PIN))/10.23; 
  Serial.println(lightLevel);
  if (lightLevel != lastLightLevel) {
      gw.send(msg.set(lightLevel));
      lastLightLevel = lightLevel;
  }
  
  delay(300); //delay to allow serial to fully print before sleep
  gw.sleep(SLEEP_TIME * 1000);
}



