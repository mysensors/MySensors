/*
* Demo for RF remote switch receiver.
* For details, see RemoteReceiver.h!
*
* This sketch "translates" an Action-remote to a Blokker-remote.
* When the A-On-button of the Action-remote is pressed, the Blokker-devices
* 5, 6 and 7 are switched on. The A-Off-button switches the devices off again.
*
* Connect the transmitter to digital pin 11, and the receiver to digital pin 2.
*/

#include <RemoteReceiver.h>
#include <RemoteTransmitter.h>

ActionTransmitter actionTransmitter(11);
BlokkerTransmitter blokkerTransmitter(11);

// Prepare the code for switch A (system code 1) on and off, for easy comparision later.
unsigned long actionAOn = actionTransmitter.getTelegram(1,'A',true);
unsigned long actionAOff = actionTransmitter.getTelegram(1,'A',false);

void setup() {
  // See example Show_received_code for info on this
  RemoteReceiver::init(0, 3, translateCode);
}

void loop() {
}

// Callback function is called only when a valid code is received.
void translateCode(unsigned long receivedCode, unsigned int period) {
  // Enabled interrupts, so RemoteReceiver::isReceiving() can be used.
  interrupts();
  
  // Compare the signals
  if (RemoteTransmitter::isSameCode(actionAOn, receivedCode)) {
	// A-On-button pressed!
	
	// Wait for a free ether
    while(RemoteReceiver::isReceiving());
	
	// Switch devices on
    blokkerTransmitter.sendSignal(5,true);
    blokkerTransmitter.sendSignal(6,true);
    blokkerTransmitter.sendSignal(7,true);
	
  } else if (RemoteTransmitter::isSameCode(actionAOff, receivedCode)) {  
	// A-Off-button pressed!
	
	// Wait for a free ether
    while(RemoteReceiver::isReceiving());
	
	// Switch devices off
    blokkerTransmitter.sendSignal(5,false);
    blokkerTransmitter.sendSignal(6,false);
    blokkerTransmitter.sendSignal(7,false);
  }  
}
