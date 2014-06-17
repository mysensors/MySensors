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

#include <SPI.h>
#include <MySensor.h>  
#include <BH1750.h>
#include <Wire.h> 

#define CHILD_ID_LIGHT 0
#define LIGHT_SENSOR_ANALOG_PIN 0
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

BH1750 lightSensor;
MySensor gw;
MyMessage msg(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
uint16_t lastlux;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Light Lux Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  
  lightSensor.begin();
}

void loop()      
{     
  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
  Serial.println(lux);
  if (lux != lastlux) {
      gw.send(msg.set(lux));
      lastlux = lux;
  }
  
  gw.sleep(SLEEP_TIME);
}
