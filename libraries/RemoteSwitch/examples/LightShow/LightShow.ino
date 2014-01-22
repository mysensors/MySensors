/*
* Demo for RF remote switch transmitter.
* For details, see RemoteTransmitter.h!
*
* This sketch switches some devices on and off in a loop.
*
* Setup:
* - Connect a 433MHz transmitter to digital pin 11.
*/

#include <RemoteTransmitter.h>

// Intantiate a new ActionTransmitter remote, use pin 11
ActionTransmitter actionTransmitter(11);

// Intantiate a new KaKuTransmitter remote, also use pin 11 (same transmitter!)
KaKuTransmitter kaKuTransmitter(11);

// Intantiate a new Blokker remote, also use pin 11 (same transmitter!)
BlokkerTransmitter blokkerTransmitter(11);

// Intantiate a new Elro remote, also use pin 11 (same transmitter!)
ElroTransmitter elroTransmitter(11);


void setup() {
}

void loop() {  
  // Switch off KaKu-device 10 on address M
  kaKuTransmitter.sendSignal('M',10,false);
  
  // Switch on Action-device B on system code 1.
  actionTransmitter.sendSignal(1,'B',true);
  
  // Switch on Blokker-device 7.
  blokkerTransmitter.sendSignal(7,true);
  
  // Switch on Elro-device C on system code 1.
  elroTransmitter.sendSignal(1,'C',true);
  
  
  // Wait 2 seconds
  delay(2000);
  
  
  
  // Switch on KaKu-device 2 of group 3 on address M (which is the same as device 10 on address M!)
  kaKuTransmitter.sendSignal('M',3,2,true);
  
  // Switch off Action-device B on system code 1.
  actionTransmitter.sendSignal(1,'B',false);
  
  // Switch off Blokker-device 7.
  blokkerTransmitter.sendSignal(7,false);
  
  // Switch off Elro-device C on system code 1.
  elroTransmitter.sendSignal(1,'C',false);  
  
  
  // Wait 4 seconds
  delay(4000);
}
