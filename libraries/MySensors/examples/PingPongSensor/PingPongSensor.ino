/***
 *  This is a simple sketch used to demenstrate and test node-to-node MySensor's communication.
 *  To use this sketch, assemble MySensors nodes - they need nothing more than a radio
 *  1.  Flash each node with the same sketch, open the console and type either 0 or 1 to the respective nodes to set thei ID
 *  2.  You only need to set the node id once, and restart the nodes
 *  3.  To being a ping-pong test, simply type T in the console for one of the nodes.
 *
 *  2015-05-25 Bruce Lacey v1.0
 */
 
#include <MySensor.h>
#include <SPI.h>
#include "MYSLog.h"

#define VSN "v1.0"

// Define two generic nodes with a single child
#define YING 200
#define YANG 201
#define CHILD 1

// Sensor objects
MySensor gw;

MyMessage mPing(CHILD, V_VAR1);   //Ping message
MyMessage mPong(CHILD, V_VAR2);   //Pong message

void setup() {

  Serial.begin(115200);
    
  gw.begin(incomingMessage);    // Node ids are written to EEPROM during sensor node configuration by entering a 0 or 1
  gw.present(CHILD, S_CUSTOM);  //
  
  gw.sendSketchInfo( nodeTypeAsCharRepresentation( gw.getNodeId() ), VSN );
  LOG(F("\n%sReady.\n"), nodeTypeAsCharRepresentation(gw.getNodeId()));
}

void loop() {
  
  // Interactive command and control
  // Entering a number from 0 or 1 will write the node 200 (YING) or 201 (YANG) to EEPROM
  // Entering T on either node will initiatve a ping-pong test.
  if (Serial.available()) {
    byte inChar = Serial.read();
    uint8_t node = gw.getNodeId();
    
    // Manual Test Mode
    if (inChar == 'T' || inChar == 't') {
      LOG(F("T received - starting test...\n"));
      MyMessage msg = mPong;
      msg.sender = (node == YING ? YANG : YING);
      sendPingOrPongResponse( msg );     
    }
    else if (inChar == '0' or inChar == '1') {
      byte nodeID = 200 + (inChar - '0');
      setNodeId(nodeID);
    }
    else {
      LOG("Invalid input\n");
    }
  }

  // Process sensor messages
  gw.process();
}

void incomingMessage(const MyMessage &message) {
  
  LOG(F("Received %s from %s\n"), msgTypeAsCharRepresentation((mysensor_data)message.type), nodeTypeAsCharRepresentation(message.sender));

  delay(250);
  sendPingOrPongResponse( message );  
}

void sendPingOrPongResponse( MyMessage msg ) {
  
  MyMessage response = (msg.type == V_VAR1 ? mPong : mPing);
 
  LOG(F("Sending %s to %s\n"), msgTypeAsCharRepresentation( (mysensor_data)response.type ), nodeTypeAsCharRepresentation(msg.sender));
  
  // Set payload to current time in millis to ensure each message is unique
  response.set( millis() );
  response.setDestination(msg.sender);
  gw.send(response);
}

void setNodeId(byte nodeID) {
  LOG(F("Setting node id to: %i.\n***Please restart the node for changes to take effect.\n"), nodeID);
  eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, (byte)nodeID);
}

const char * msgTypeAsCharRepresentation( mysensor_data mType ) {
  return mType == V_VAR1 ? "Ping" : "Pong";
}

const char * nodeTypeAsCharRepresentation( uint8_t node ) {
  return node == YING ? "Ying Node" : "Yang Node";
}
