/*
*
* This sketch clears radioId, relayId and routing info in EEPROM 
*
*/

#include <SPI.h>
#include <EEPROM.h>  

void setup()  
{ 
  for (int i=0;i<512;i++) {
    EEPROM.write(i, 0xff);
  }
}

void loop()      
{ 
  // Nothing to do here...
} 
