// Example sketch showing how to control physical relays. 

#include <MySensor.h>
#include <SPI.h>
#include <RF24.h>


#define RELAY_1  3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 1 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay

MySensor gw;

void setup()  
{   
  // Initialize library and add callback for incoming messages
  gw.begin(incomingMessage, false);
  // Send the sketch version information to the gateway and Controller
  gw.sketchInfo("Relay", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  for (int i=0; i<NUMBER_OF_RELAYS;i++) {
    gw.present(RELAY_1+i, S_LIGHT);
  }
  // Fetch relay status
  for (int i=0; i<NUMBER_OF_RELAYS;i++) {
    // Make sure relays are off when starting up
    digitalWrite(RELAY_1+i, RELAY_OFF);
    // Then set relay pins in output mode
    pinMode(RELAY_1+i, OUTPUT);   
      
    // Request and wait for relay status from controller
    gw.request(RELAY_1+i, V_LIGHT);
  }
  
}


void loop() 
{
  // Process incoming messages
  gw.process();
}

void incomingMessage(MyMessage message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LIGHT) {
     // Change relay state
     digitalWrite(message.sensor, message.getBool()?RELAY_ON:RELAY_OFF);

     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}

