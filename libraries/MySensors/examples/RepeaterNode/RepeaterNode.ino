// Example sketch showing how to create a node thay repeates messages
// from nodes far from gateway back to gateway. 
// It is important that nodes that has enabled repeater mode calls  
// gw.preocess() frequently. This node should never sleep. 

#include <MySensor.h>
#include <SPI.h>

MySensor gw;

void setup()  
{  
  // The third argument enables repeater mode.
  gw.begin(NULL, AUTO, true);

  //Send the sensor node sketch version information to the gateway
  gw.sendSketchInfo("Repeater Node", "1.0");
}

void loop() 
{
  // By calling process() you route messages in the background
  gw.process();
}

