/*
  Vera Arduino BH1750FVI Light sensor
  communicate using I2C Protocol
  this library enable 2 slave device addresses
  Main address  0x23
  secondary address 0x5C
  connect the sensor as follows :

  VCC  >>> 5V
  Gnd  >>> Gnd
  ADDR >>> NC or GND  
  SCL  >>> A5
  SDA  >>> A4
  
  Contribution: idefix
 
*/


#include <Sleep_n0m1.h>
#include <SPI.h>
#include <RF24.h>
#include <EEPROM.h>  
#include <Sensor.h>  
#include <BH1750.h>
#include <Wire.h> 

#define CHILD_ID_LIGHT 0
#define LIGHT_SENSOR_ANALOG_PIN 0
unsigned long SLEEP_TIME = 30; // Sleep time between reads (in seconds)

BH1750  lightSensor;
Sensor gw;
Sleep sleep;
int lastlux;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Light Lux Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.sendSensorPresentation(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  
  lightSensor.begin();
}

void loop()      
{     
  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
  Serial.println(lux);
  if (lux != lastlux) {
      gw.sendVariable(CHILD_ID_LIGHT, V_LIGHT_LEVEL, lux);
      lastlux = lux;
  }
  
  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  delay(1000); //delay to allow serial to fully print before sleep
  gw.powerDown();
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(SLEEP_TIME * 1000); //sleep for: sleepTime 
}
