// Example sketch showing how to create a node thay relays messages
// from nodes far from gateway back to gateway. 
// Important here is that node uses Relay-class and calls 
// gw.messageAvailable() frequently. This sketch should not 
// sleep.

#include <Relay.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>

Relay gw;

void setup()  
{  
  gw.begin();

  //Send the sensor node sketch version information to the gateway
  gw.sendSketchInfo("Relaying Node", "1.0");
}

void loop() 
{
  // By calling this regularely you route messages in the background
  if (gw.messageAvailable()) {  

    // Handleincoming message for this node here... 
  }
}

