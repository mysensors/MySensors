#ifndef __GATEWAYUTIL_H__
#define __GATEWAYUTIL_H__

#include <MyTransport.h>


#ifdef ARDUINO


uint8_t inclusionTime = 1; // Number of minutes inclusion mode is enabled
uint8_t pinInclusion =  3; // Input pin that should trigger inclusion mode

#define MAX_RECEIVE_LENGTH 100 // Max buffersize needed for messages coming from controller
#define MAX_SEND_LENGTH 120 // Max buffersize needed for messages destined for controller

volatile boolean buttonTriggeredInclusion;
boolean inclusionMode; // Keeps track on inclusion mode
void (*serial)(const char *fmt, ... );

MyParserSerial parser;

void setInclusionMode(boolean newMode);

char convBuf[MAX_PAYLOAD*2+1];
char serialBuffer[MAX_SEND_LENGTH]; // Buffer for building string when sending data to vera
unsigned long inclusionStartTime;

void setupGateway(uint8_t _inc, uint8_t _incTime, void (* _serial)(const char *, ... )) {
  inclusionMode = 0;
  buttonTriggeredInclusion = false;
  serial = _serial;

  pinInclusion = _inc;
  inclusionTime = _incTime;
  
  // Setup digital in that triggers inclusion mode
  pinMode(pinInclusion, INPUT);
  digitalWrite(pinInclusion, HIGH);
}



void startInclusionInterrupt() {
  buttonTriggeredInclusion = true;
}

void incomingMessage(const MyMessage &message) {
//  if (mGetCommand(message) == C_PRESENTATION && inclusionMode) {
//	gw.rxBlink(3);
//   } else {
//	gw.rxBlink(1);
//   }
   // Pass along the message from sensors to serial line
   serial(PSTR("%d;%d;%d;%d;%d;%s\n"),message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type, message.getString(convBuf));
} 



void checkButtonTriggeredInclusion() {
   if (buttonTriggeredInclusion) {
    // Ok, someone pressed the inclusion button on the gateway
    // start inclusion mode for 1 munute.
#ifdef DEBUG
    serial(PSTR("0;0;%d;0;%d;Inclusion started by button.\n"),  C_INTERNAL, I_LOG_MESSAGE);
#endif
    buttonTriggeredInclusion = false;
    setInclusionMode(true);
  }
}

void checkInclusionFinished() {
  if (inclusionMode && millis()-inclusionStartTime>60000UL*inclusionTime) {
    // inclusionTimeInMinutes minute(s) has passed.. stop inclusion mode
    setInclusionMode(false);
  }
}

void parseAndSend(MySensor &gw, char *commandBuffer) {
  boolean ok;
  MyMessage &msg = gw.getLastMessage();

  if (parser.parse(msg, commandBuffer)) {
    uint8_t command = mGetCommand(msg);

    if (msg.destination==GATEWAY_ADDRESS && command==C_INTERNAL) {
      // Handle messages directed to gateway
      if (msg.type == I_VERSION) {
        // Request for version
        serial(PSTR("0;0;%d;0;%d;%s\n"), C_INTERNAL, I_VERSION, LIBRARY_VERSION);
      } else if (msg.type == I_INCLUSION_MODE) {
        // Request to change inclusion mode
        setInclusionMode(atoi(msg.data) == 1);
      }
    } else {
      #ifdef WITH_LEDS_BLINKING
      gw.txBlink(1);
      #endif
      ok = gw.sendRoute(msg);
      if (!ok) {
        #ifdef WITH_LEDS_BLINKING
        gw.errBlink(1);
        #endif
      }
    }
  }
}

void setInclusionMode(boolean newMode) {
  if (newMode != inclusionMode) {
    inclusionMode = newMode;
    // Send back mode change on serial line to ack command
    serial(PSTR("0;0;%d;0;%d;%d\n"), C_INTERNAL, I_INCLUSION_MODE, inclusionMode?1:0);

    if (inclusionMode) {
      inclusionStartTime = millis();
    }
  }
}


#else
#error This example is only for use on Arduino.
#endif // ARDUINO

#endif 
