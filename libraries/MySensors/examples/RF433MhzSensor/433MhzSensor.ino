/*
 * This sketch demonstrates how to use InterruptChain to receive and
 * decode remote switches (old and new) and remote sensors.
 *
 * Basically, this sketch combines the features of the ShowReceivedCode
 * and ShowReceivedCodeNewKaku examples of RemoteSwitch and the
 * ThermoHygroReceiver of RemoteSensor all at the same time!
 *
 * After uploading, enable the serial monitor at 115200 baud.
 * When you press buttons on a 433MHz remote control, as supported by 
 * RemoteSwitch or NewRemoteSwitch, the code will be echoed.
 * At the same time, if data of a remote thermo/hygro-sensor is
 * received, as supported by RemoteSensor, it will be echoed as well.
 *
 * Setup:
 * - connect a 433MHz receiver on digital pin 2.
 *
 *
 * MySensor note: This example has not yet been adopted but can be used as an example
 * of receiving 433Mhz stuff. 
 * 
 * One idea could be to echo received 433 codes back to gateway to be able to pick up and
 * handle data over there.
 *
 */

#include <RemoteReceiver.h>
#include <NewRemoteReceiver.h>
#include <SensorReceiver.h>
#include <InterruptChain.h>

void setup() {
  Serial.begin(115200);

  // Interrupt -1 to indicate you will call the interrupt handler with InterruptChain
  RemoteReceiver::init(-1, 2, showOldCode);
  
  // Again, interrupt -1 to indicate you will call the interrupt handler with InterruptChain
  NewRemoteReceiver::init(-1, 2, showNewCode);

  // And once more, interrupt -1 to indicate you will call the interrupt handler with InterruptChain
  SensorReceiver::init(-1, showTempHumi);

  // On interrupt, call the interrupt handlers of remote and sensor receivers
  InterruptChain::addInterruptCallback(0, RemoteReceiver::interruptHandler);
  InterruptChain::addInterruptCallback(0, NewRemoteReceiver::interruptHandler);
  InterruptChain::addInterruptCallback(0, SensorReceiver::interruptHandler);
}

void loop() {
   // You can do other stuff here!
}

// shows the received code sent from an old-style remote switch
void showOldCode(unsigned long receivedCode, unsigned int period) {
  // Print the received code.
  Serial.print("Code: ");
  Serial.print(receivedCode);
  Serial.print(", period: ");
  Serial.print(period);
  Serial.println("us.");
}

// Shows the received code sent from an new-style remote switch
void showNewCode(NewRemoteCode receivedCode) {
  // Print the received code.
  Serial.print("Addr ");
  Serial.print(receivedCode.address);
  
  if (receivedCode.groupBit) {
    Serial.print(" group");
  } else {
    Serial.print(" unit ");
    Serial.print(receivedCode.unit);
  }
  
  switch (receivedCode.switchType) {
    case NewRemoteCode::off:
      Serial.print(" off");
      break;
    case NewRemoteCode::on:
      Serial.print(" on");
      break;
    case NewRemoteCode::dim:
      Serial.print(" dim level ");
      Serial.print(receivedCode.dimLevel);
      break;
    case NewRemoteCode::on_with_dim:
      Serial.print(" on with dim level ");
      Serial.print(receivedCode.dimLevel);
      break;
  }
  
  Serial.print(", period: ");
  Serial.print(receivedCode.period);
  Serial.println("us.");
}

// Shows the received sensor data
void showTempHumi(byte *data) {
  // is data a ThermoHygro-device?
  if ((data[3] & 0x1f) == 0x1e) {
    // Yes!
    byte channel, randomId;
    int temp;
    byte humidity;

    // Decode the data
    SensorReceiver::decodeThermoHygro(data, channel, randomId, temp, humidity);
  
    // Print temperature. Note: temp is 10x the actual temperature!
    Serial.print("Temperature: ");
    Serial.print(temp / 10); // units
    Serial.print('.');
    Serial.println(temp % 10); // decimal
  }
}
