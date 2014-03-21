// Example sketch showing how to control ir devices
// An IR LED must be connected to Arduino PWM pin 3.
// An optional ir receiver can be connected to PWM pin 8. 
// All receied ir signals will be sent to gateway device stored in VAR_1.
// When binary light on is clicked - sketch will send volume up ir command
// When binary light off is clicked - sketch will send volume down ir command

#include <Sensor.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <IRremote.h>

int RECV_PIN = 8;

#define CHILD_1  3  // childId

IRsend irsend;
IRrecv irrecv(RECV_PIN);
decode_results results;

Sensor gw;

void setup()  
{  
  irrecv.enableIRIn(); // Start the ir receiver
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("IR Sensor", "1.0");

  // Register a sensors to gw. Use binary light for test purposes.
  gw.sendSensorPresentation(CHILD_1, S_LIGHT);
}


void loop() 
{
  if (gw.messageAvailable()) {
    message_s message = gw.getMessage(); 

     int incomingRelayStatus = atoi(message.data);
     if (incomingRelayStatus == 1) {
      irsend.sendNEC(0x1EE17887, 32); // Vol up yamaha ysp-900
     } else {
      irsend.sendNEC(0x1EE1F807, 32); // Vol down yamaha ysp-900
     }
     // Start receiving ir again...
    irrecv.enableIRIn(); 
  }
  
 
  if (irrecv.decode(&results)) {
    char buffer[10];
    sprintf(buffer, "%08lx", results.value);
    dump(&results);
    // Send ir result to gw
    gw.sendVariable(CHILD_1, V_VAR1, buffer);
    irrecv.resume(); // Receive the next value
  }
}


// Dumps out the decode_results structure.
// Call this after IRrecv::decode()
// void * to work around compiler issue
//void dump(void *v) {
//  decode_results *results = (decode_results *)v
void dump(decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    Serial.print("Unknown encoding: ");
  } 
  else if (results->decode_type == NEC) {
    Serial.print("Decoded NEC: ");
  } 
  else if (results->decode_type == SONY) {
    Serial.print("Decoded SONY: ");
  } 
  else if (results->decode_type == RC5) {
    Serial.print("Decoded RC5: ");
  } 
  else if (results->decode_type == RC6) {
    Serial.print("Decoded RC6: ");
  }
  else if (results->decode_type == PANASONIC) {	
    Serial.print("Decoded PANASONIC - Address: ");
    Serial.print(results->panasonicAddress,HEX);
    Serial.print(" Value: ");
  }
  else if (results->decode_type == JVC) {
     Serial.print("Decoded JVC: ");
  }
  Serial.print(results->value, HEX);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
  Serial.print("Raw (");
  Serial.print(count, DEC);
  Serial.print("): ");

  for (int i = 0; i < count; i++) {
    if ((i % 2) == 1) {
      Serial.print(results->rawbuf[i]*USECPERTICK, DEC);
    } 
    else {
      Serial.print(-(int)results->rawbuf[i]*USECPERTICK, DEC);
    }
    Serial.print(" ");
  }
  Serial.println("");
}

