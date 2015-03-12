/*
*
* This sketch clears radioId, relayId and routing info in EEPROM 
*
*/

#include <SPI.h>
#include <EEPROM.h>  
#include <MySensor.h>  

void setup()  
{ 
  Serial.begin(BAUD_RATE);
  Serial.println("Started clearing. Please wait...");
  for (int i=0;i<512;i++) {
    EEPROM.write(i, 0xff);
  }
  Serial.println("Clering done. You're ready to go!");
}

void loop()      
{ 
  // Nothing to do here...
} 
