#ifndef __GATEWAYUTIL_H__
#define __GATEWAYUTIL_H__

#include <stdarg.h>

#ifdef ARDUINO


uint8_t pinRx = 8; // Rx led pin
uint8_t pinTx = 9; // Tx led pin
uint8_t pinEr = 7; // Err led pin
uint8_t inclusionTime = 1; // Number of minutes inclusion mode is enabled
uint8_t pinInclusion =  3; // Input pin that should trigger inclusion mode

#define MAX_RECEIVE_LENGTH 100 // Max buffersize needed for messages coming from controller
#define MAX_SEND_LENGTH 120 // Max buffersize needed for messages destined for controller

volatile boolean buttonTriggeredInclusion;
volatile uint8_t countRx;
volatile uint8_t countTx;
volatile uint8_t countErr;
boolean inclusionMode; // Keeps track on inclusion mode
void (*serial)(const char *fmt, ... );

MyMessage msg;

void setInclusionMode(boolean newMode);
void txBlink(uint8_t cnt);
void rxBlink(uint8_t cnt);
void errBlink(uint8_t cnt);

char convBuf[MAX_PAYLOAD*2+1];
char serialBuffer[MAX_SEND_LENGTH]; // Buffer for building string when sending data to vera
unsigned long inclusionStartTime;

void setupGateway(uint8_t _rx, uint8_t _tx, uint8_t _er, uint8_t _inc, uint8_t _incTime, void (* _serial)(const char *, ... )) {
  inclusionMode = 0;
  buttonTriggeredInclusion = false;
  serial = _serial;

  pinRx = _rx;
  pinTx = _tx;
  pinEr = _er;
  pinInclusion = _inc;
  inclusionTime = _incTime;
  
  countRx = 0;
  countTx = 0;
  countErr = 0;

  // Setup led pins
  pinMode(pinRx, OUTPUT);
  pinMode(pinTx, OUTPUT);
  pinMode(pinEr, OUTPUT);
  digitalWrite(pinRx, LOW);
  digitalWrite(pinTx, LOW);
  digitalWrite(pinEr, LOW);

  // Setup digital in that triggers inclusion mode
  pinMode(pinInclusion, INPUT);
  digitalWrite(pinInclusion, HIGH);

  // Set initial state of leds
  digitalWrite(pinRx, HIGH);
  digitalWrite(pinTx, HIGH);
  digitalWrite(pinEr, HIGH);

}



void startInclusionInterrupt() {
  buttonTriggeredInclusion = true;
}

void incomingMessage(const MyMessage &message) {
  if (mGetCommand(message) == C_PRESENTATION && inclusionMode) {
	rxBlink(3);
   } else {
	rxBlink(1);
   }
   // Pass along the message from sensors to serial line
   serial(PSTR("%d;%d;%d;%d;%d;%s\n"),message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type, message.getString(convBuf));
} 



void checkButtonTriggeredInclusion() {
   if (buttonTriggeredInclusion) {
    // Ok, someone pressed the inclusion button on the gateway
    // start inclusion mode for 1 munute.
    serial(PSTR("0;0;%d;0;%d;Inclusion started by button.\n"),  C_INTERNAL, I_LOG_MESSAGE);
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

uint8_t h2i(char c) {
  uint8_t i = 0;
  if (c <= '9')
    i += c - '0';
  else if (c >= 'a')
    i += c - 'a' + 10;
  else
    i += c - 'A' + 10;
  return i;
}

void parseAndSend(MySensor gw, char *commandBuffer) {
  boolean ok = false;
  char *str, *p, *value=NULL;
  uint8_t bvalue[MAX_PAYLOAD];
  uint8_t blen = 0;
  int i = 0;
  uint16_t destination = 0;
  uint8_t sensor = 0;
  uint8_t command = 0;
  uint8_t type = 0;
  uint8_t ack = 0;

  // Extract command data coming on serial line
  for (str = strtok_r(commandBuffer, ";", &p);       // split using semicolon
  		str && i < 6;         // loop while str is not null an max 5 times
  		str = strtok_r(NULL, ";", &p)               // get subsequent tokens
				) {
	switch (i) {
	  case 0: // Radioid (destination)
	 	destination = atoi(str);
		break;
	  case 1: // Childid
		sensor = atoi(str);
		break;
	  case 2: // Message type
		command = atoi(str);
		break;
	  case 3: // Should we request ack from destination?
		ack = atoi(str);
		break;
	  case 4: // Data type
		type = atoi(str);
		break;
	  case 5: // Variable value
		if (command == C_STREAM) {
			blen = 0;
			uint8_t val;
			while (*str) {
				val = h2i(*str++) << 4;
				val += h2i(*str++);
				bvalue[blen] = val;
				blen++;
			}
		} else {
			value = str;
			// Remove ending carriage return character (if it exists)
			uint8_t lastCharacter = strlen(value)-1;
			if (value[lastCharacter] == '\r')
				value[lastCharacter] = 0;
		}
		break;
	  }
	  i++;
  }

  if (destination==GATEWAY_ADDRESS && command==C_INTERNAL) {
    // Handle messages directed to gateway
    if (type == I_VERSION) {
      // Request for version
      serial(PSTR("0;0;%d;0;%d;%s\n"), C_INTERNAL, I_VERSION, LIBRARY_VERSION);
    } else if (type == I_INCLUSION_MODE) {
      // Request to change inclusion mode
      setInclusionMode(atoi(value) == 1);
    }
  } else {
    txBlink(1);
    msg.sender = GATEWAY_ADDRESS;
    msg.destination = destination;
    msg.sensor = sensor;
    msg.type = type;
    mSetCommand(msg,command);
    mSetRequestAck(msg,ack?1:0);
    mSetAck(msg,false);
    if (command == C_STREAM)
      msg.set(bvalue, blen);
    else
      msg.set(value);
    ok = gw.sendRoute(msg);
    if (!ok) {
      errBlink(1);
    }
  }
}


void setInclusionMode(boolean newMode) {
  if (newMode != inclusionMode)
    inclusionMode = newMode;
    // Send back mode change on serial line to ack command
    serial(PSTR("0;0;%d;0;%d;%d\n"), C_INTERNAL, I_INCLUSION_MODE, inclusionMode?1:0);

    if (inclusionMode) {
      inclusionStartTime = millis();
    }
}




void ledTimersInterrupt() {
  if(countRx && countRx != 255) {
    // switch led on
    digitalWrite(pinRx, LOW);
  } else if(!countRx) {
     // switching off
     digitalWrite(pinRx, HIGH);
   }
   if(countRx != 255) { countRx--; }

  if(countTx && countTx != 255) {
    // switch led on
    digitalWrite(pinTx, LOW);
  } else if(!countTx) {
     // switching off
     digitalWrite(pinTx, HIGH);
   }
   if(countTx != 255) { countTx--; }
   else if(inclusionMode) { countTx = 8; }

  if(countErr && countErr != 255) {
    // switch led on
    digitalWrite(pinEr, LOW);
  } else if(!countErr) {
     // switching off
     digitalWrite(pinEr, HIGH);
   }
   if(countErr != 255) { countErr--; }

}

void rxBlink(uint8_t cnt) {
  if(countRx == 255) { countRx = cnt; }
}
void txBlink(uint8_t cnt) {
  if(countTx == 255 && !inclusionMode) { countTx = cnt; }
}
void errBlink(uint8_t cnt) {
  if(countErr == 255) { countErr = cnt; }
}



#else
#error This example is only for use on Arduino.
#endif // ARDUINO

#endif 
