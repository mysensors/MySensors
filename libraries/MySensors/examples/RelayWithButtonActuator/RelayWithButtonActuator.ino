// Example sketch för a "light switch" where you can control light or something 
// else from both vera and a local physical button (connected between digital
// pin 3 and GND).
// This node also works as a repeader for other nodes

#include <MySensor.h>
#include <SPI.h>
#include <Bounce2.h>

#define RELAY_PIN  4  // Arduino Digital I/O pin number for relay 
#define BUTTON_PIN  3  // Arduino Digital I/O pin number for button 
#define CHILD_ID 1   // Id of the sensor child
#define RELAY_ON 1
#define RELAY_OFF 0

Bounce debouncer = Bounce(); 
int oldValue=0;
bool state;
MySensor gw;
MyMessage msg(CHILD_ID,V_LIGHT);

void setup()  
{  
  gw.begin(incomingMessage, AUTO, true);

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Relay & Button", "1.0");

 // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID, S_LIGHT);

  // Make sure relays are off when starting up
  digitalWrite(RELAY_PIN, RELAY_OFF);
  // Then set relay pins in output mode
  pinMode(RELAY_PIN, OUTPUT);   
      
  // Set relay to last known state (using eeprom storage) 
  state = gw.loadState(CHILD_ID);
  digitalWrite(RELAY_PIN, state?RELAY_ON:RELAY_OFF);
}


/*
*  Example on how to asynchronously check for new messages from gw
*/
void loop() 
{
  gw.process();
  debouncer.update();
  // Get the update value
  int value = debouncer.read();
  if (value != oldValue && value==0) {
      gw.send(msg.set(state?false:true), true); // Send new state and request ack back
  }
  oldValue = value;
} 
 
void incomingMessage(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.isAck()) {
     Serial.println("This is an ack from gateway");
  }

  if (message.type == V_LIGHT) {
     // Change relay state
     state = message.getBool();
     digitalWrite(RELAY_PIN, state?RELAY_ON:RELAY_OFF);
     // Store state in eeprom
     gw.saveState(CHILD_ID, state);
    
     // Write some debug info
     Serial.print("Incoming change for sensor:");
     Serial.print(message.sensor);
     Serial.print(", New status: ");
     Serial.println(message.getBool());
   } 
}

