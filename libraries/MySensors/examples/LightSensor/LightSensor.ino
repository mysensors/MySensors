#include <Sleep_n0m1.h>
#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>  
#include <Sensor.h>  

#define CHILD_ID_LIGHT 0
#define LIGHT_SENSOR_ANALOG_PIN 0

unsigned long SLEEP_TIME = 30; // Sleep time between reads (in seconds)

Sensor gw;
int lastLightLevel;
Sleep sleep;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Light Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.sendSensorPresentation(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
}

void loop()      
{     
  int lightLevel = (1023-analogRead(LIGHT_SENSOR_ANALOG_PIN))/10.23; 
  Serial.println(lightLevel);
  if (lightLevel != lastLightLevel) {
      gw.sendVariable(CHILD_ID_LIGHT, V_LIGHT_LEVEL, lightLevel);
      lastLightLevel = lightLevel;
  }
  
  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  delay(1000); //delay to allow serial to fully print before sleep
  gw.powerDown();
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(SLEEP_TIME * 1000); //sleep for: sleepTime 
}



