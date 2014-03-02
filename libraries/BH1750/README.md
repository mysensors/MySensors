BH1750
======

An Arduino library for digital light sensor breakout boards containing the BH1750FVI IC.

The board uses I2C for communication. 

2 pins are required to interface to the device.

Datasheet for the light sensor:
http://rohmfs.rohm.com/en/products/databook/datasheet/ic/sensor/light/bh1750fvi-e.pdf


To download, click the ZIP button above. Extract the zip file and  rename the uncompressed folder to BH1750. Check that it contains  BH1750.cpp and BH1750.h, keywords.txt.

Place the BH1750 library folder your &lt;arduinosketchfolder&gt;/libraries/ folder. Restart the IDE.


Example usage:
```c
/*

Example of BH1750 library usage.

This example initalises the BH1750 object using the default
high resolution mode and then makes a light level reading every second.

Connection:
 VCC-5v
 GND-GND
 SCL-SCL(analog pin 5)
 SDA-SDA(analog pin 4)
 ADD-NC or GND

*/

#include <Wire.h>
#include <BH1750.h>


BH1750 lightMeter;


void setup(){
  Serial.begin(9600);
  lightMeter.begin();
  Serial.println("Running...");
}


void loop() {
  uint16_t lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000);
}
```
