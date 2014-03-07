/*
 * This sketch simply repeats data received from remote weather sensors made by Cresta.
 * 
 * Setup:
 *  - connect digital output of a 433MHz receiver to digital pin 2 of Arduino.
 *  - connect transmitter input of a 433MHz transmitter to digital pin 11
 *  - An LED on pin 13 will tell you if and when a signal has been received and transmitted.
 */

#include <SensorReceiver.h>
#include <SensorTransmitter.h>

#define LED_PIN 13
#define TRANSMITTER_PIN 11

void setup() {
  pinMode(LED_PIN, OUTPUT);
  
  // Since we're not instantiating SensorTransmitter, but only use the static methods of SensorTransmitter,
  // the pin mode must be set manually.
  pinMode(TRANSMITTER_PIN, OUTPUT); 

  // When no signal has been received, the LED is lit.
  digitalWrite(LED_PIN, HIGH);   

  // Init the receiver on interrupt pin 0 (digital pin 2).
  // Set the callback to function "retransmit", which is called
  // whenever valid sensor data has been received.
  SensorReceiver::init(0, retransmit);    
}

void loop() {
}

void retransmit(byte *data) {  
  // Data received

  // Wait a second after a receiving. There's little point for decoding and sending the same signal multiple times.	
  SensorReceiver::disable();
  interrupts(); // delay() requires that interrupts are enabled
  delay(1000);    

  // Flash LED when transmitting. 
  digitalWrite(LED_PIN, HIGH);

  // Transmit signal. Note: this is a static method, no object required!
  SensorTransmitter::sendPackage(TRANSMITTER_PIN, data);

  digitalWrite(LED_PIN, LOW);

  noInterrupts();
  SensorReceiver::enable();     
}

