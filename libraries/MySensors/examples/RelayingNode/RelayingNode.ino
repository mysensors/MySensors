// Example sketch showing how to create a node thay relays messages
// from nodes far from gateway back to gateway. 
// Important here is that node uses Relay-class and calls 
// gw.messageAvailable() frequently. This sketch should not 
// sleep.

#include <Relay.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>

// Set RADIO_ID to something unique in your sensor network (1-254)
// or set to AUTO if you want gw to assign a RADIO_ID for you.
#define RADIO_ID AUTO

Relay gw(9,10);

void setup()  
{  
  Serial.begin(BAUD_RATE);  // Used to write debug info
  gw.begin(RADIO_ID);
  
}

void loop() 
{
  if (gw.messageAvailable()) {
    // Incoming message for this node... 
  }
}

