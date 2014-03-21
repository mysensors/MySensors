// Example sketch showing how to control physical relays. 

#include <Relay.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>

#define RELAY_1  3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 1 
#define RELAY_ON 0
#define RELAY_OFF 1

Sensor gw;

void setup()  
{  
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Relay", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  for (int i=0; i<NUMBER_OF_RELAYS;i++) {
    gw.sendSensorPresentation(RELAY_1+i, S_LIGHT);
  }
  // Fetch relay status
  for (int i=0; i<NUMBER_OF_RELAYS;i++) {
    // Make sure relays are off when starting up
    digitalWrite(RELAY_1+i, RELAY_OFF);
    // Then set relay pins in output mode
    pinMode(RELAY_1+i, OUTPUT);   
      
    // Request/wait for relay status
    gw.getStatus(RELAY_1+i, V_LIGHT);
    setRelayStatus(gw.getMessage()); // Wait here until status message arrive from gw
  }
  
}


/*
*  Example on how to asynchronously check for new messages from gw
*/
void loop() 
{
  if (gw.messageAvailable()) {
    message_s message = gw.getMessage(); 
    setRelayStatus(message);
  }
}

void setRelayStatus(message_s message) {
  if (message.header.messageType=M_SET_VARIABLE &&
      message.header.type==V_LIGHT) {
     int incomingRelayStatus = atoi(message.data);
     // Change relay state
     digitalWrite(message.header.childId, incomingRelayStatus==1?RELAY_ON:RELAY_OFF);
     // Write some debug info
     Serial.print("Incoming change for relay on pin:");
     Serial.print(message.header.childId);
     Serial.print(", New status: ");
     Serial.println(incomingRelayStatus);
   }
}
