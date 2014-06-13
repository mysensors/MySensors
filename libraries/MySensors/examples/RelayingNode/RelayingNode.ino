// Example sketch showing how to create a node thay relays messages
// from nodes far from gateway back to gateway. 
// Important here is that node uses Relay-class and calls 
// gw.messageAvailable() frequently. This sketch should not 
// sleep.

#include <MySensor.h>
#include <SPI.h>
#include <RF24.h>

MySensor gw;

void setup()  
{  
  // The third argument enables relaying mode.
  gw.begin(NULL, AUTO, true);

  //Send the sensor node sketch version information to the gateway
  gw.sendSketchInfo("Relaying Node", "1.0");
}

void loop() 
{
  // By calling process() you route messages in the background
  gw.process();
}

