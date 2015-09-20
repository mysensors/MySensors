/*
* Demo for RF remote switch receiver.
* For details, see RemoteReceiver.h!
*
* This sketch demonstrates how to use the static version of
* RemoteReceiver::sendCode, which can be used in low-memory
* situations.
*
* Connect the transmitter to digital pin 11, and the receiver to digital pin 2.
*
* When run, this sketch waits for a valid code from the receiver, decodes it,
* and retransmits it after 5 seconds.
*/

#include <RemoteReceiver.h>
#include <RemoteTransmitter.h>

void setup() {
  // See example Show_received_code for info on this
  RemoteReceiver::init(0, 3, retransmitter);
}

void loop() {
}

void retransmitter(unsigned long receivedCode, unsigned int period) {  
  // Disable the receiver; otherwise it might pick up the retransmit as well.
  RemoteReceiver::disable();
  
  // Need interrupts for delay()
  interrupts();
    
  // Wait 5 seconds before sending.
  delay(5000);

  // Retransmit the signal 8 times ( == 2^3) on pin 11. Note: no object was created!
  RemoteTransmitter::sendCode(11, receivedCode, period, 3);
  
  RemoteReceiver::enable();
}
