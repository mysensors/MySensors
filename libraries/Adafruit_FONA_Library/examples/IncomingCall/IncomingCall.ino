// FONA Incoming Call Number Example
// Listens for a call and displays the phone number of the caller (if available).
// Use this example to add phone call detection to your own FONA sketch.
#include "Adafruit_FONA.h"

// Pins which are connected to the FONA.
// Note that this is different from FONAtest!
#define FONA_RX            3
#define FONA_TX            4
#define FONA_RST           5

// Note you need to map interrupt number to pin number
// for your board.  On an Uno & Mega interrupt 0 is
// digital pin 2, and on a Leonardo interrupt 0 is
// digital pin 3.  See this page for a complete table:
//   http://arduino.cc/en/Reference/attachInterrupt
// Make sure this interrupt pin is connected to FONA RI!
#define FONA_RI_INTERRUPT  0

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Hardware serial is also possible!
//  HardwareSerial *fonaSerial = &Serial1;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

void setup() {
  Serial.begin(115200);
  Serial.println(F("FONA incoming call example"));
  Serial.println(F("Initializing....(May take 3 seconds)"));

  fonaSerial->begin(4800);
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while(1);
  }
  Serial.println(F("FONA is OK"));

  // Enable incoming call notification.
  if(fona.callerIdNotification(true, FONA_RI_INTERRUPT)) {
    Serial.println(F("Caller id notification enabled."));
  }
  else {
    Serial.println(F("Caller id notification disabled"));
  }
}

void loop(){
  // Create a small string buffer to hold incoming call number.
  char phone[32] = {0};
  // Check for an incoming call.  Will return true if a call is incoming.
  if(fona.incomingCallNumber(phone)){
    Serial.println(F("RING!"));
    Serial.print(F("Phone Number: "));
    Serial.println(phone);
  }
}
