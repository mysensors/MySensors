// Example sketch showing how to securely control locks. 
// This example will remember lock state even after power failure.

#include <MySigningAtsha204Soft.h>
#include <MyTransportNRF24.h>
#include <MySensor.h>
#include <SPI.h>

#define LOCK_1  3  // Arduino Digital I/O pin number for first lock (second on pin+1 etc)
#define NOF_LOCKS 1 // Total number of attached locks
#define LOCK_LOCK 1  // GPIO value to write to lock attached lock
#define LOCK_UNLOCK 0 // GPIO value to write to unlock attached lock

MyTransportNRF24 radio;  // NRFRF24L01 radio driver
MySigningAtsha204Soft signer;  // Message signing driver
MySensor gw(radio, signer);

void setup()  
{   
  // Initialize library and add callback for incoming messages (signing is required)
  gw.begin(incomingMessage, AUTO, true, AUTO);
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Secure Lock", "1.0");

  // Fetch lock status
  for (int lock=1, pin=LOCK_1; lock<=NOF_LOCKS;lock++, pin++) {
    // Register all locks to gw (they will be created as child devices)
    gw.present(lock, S_LOCK, false);
    // Then set lock pins in output mode
    pinMode(pin, OUTPUT);   
    // Set lock to last known state (using eeprom storage) 
    digitalWrite(pin, gw.loadState(lock)?LOCK_LOCK:LOCK_UNLOCK);
  }
}


void loop() 
{
  // Alway process incoming messages whenever possible
  gw.process();
}

void incomingMessage(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LOCK_STATUS && message.sensor<=NOF_LOCKS) {
     // Change relay state
     digitalWrite(message.sensor-1+LOCK_1, message.getBool()?LOCK_LOCK:LOCK_UNLOCK);
     // Store state in eeprom
     gw.saveState(message.sensor, message.getBool());
     // Write some debug info
     Serial.print("Incoming change for lock:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}

