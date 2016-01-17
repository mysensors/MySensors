/*************************************************** 
  This is an example for our Adafruit FONA Cellular Module
  since the FONA 3G does not do auto-baud very well, this demo 
  fixes the baud rate to 4800 from the default 115200
  
  Designed specifically to work with the Adafruit FONA 3G
  ----> http://www.adafruit.com/products/2691
  ----> http://www.adafruit.com/products/2687

  These cellular modules use TTL Serial to communicate, 2 pins are 
  required to interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/

#include "Adafruit_FONA.h"

#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4

// this is a large buffer for replies
char replybuffer[255];

// We default to using software serial. If you want to use hardware serial
// (because softserial isnt supported) comment out the following three lines 
// and uncomment the HardwareSerial line
#include <SoftwareSerial.h>
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;

// Hardware serial is also possible!
//  HardwareSerial *fonaSerial = &Serial1;

Adafruit_FONA fona = Adafruit_FONA(FONA_RST);

uint8_t readline(char *buff, uint8_t maxbuff, uint16_t timeout = 0);

void setup() {
  while (!Serial);

  Serial.begin(115200);
  Serial.println(F("FONA set baudrate"));

  Serial.println(F("First trying 115200 baud"));
  // start at 115200 baud
  fonaSerial->begin(115200);
  fona.begin(*fonaSerial);
  
  // send the command to reset the baud rate to 4800
  fona.setBaudrate(4800); 
  
  // restart with 4800 baud
  fonaSerial->begin(4800);
  Serial.println(F("Initializing @ 4800 baud..."));
  
  if (! fona.begin(*fonaSerial)) {
    Serial.println(F("Couldn't find FONA"));
    while(1);
  }
  Serial.println(F("FONA is OK"));

  // Print module IMEI number.
  char imei[15] = {0}; // MUST use a 16 character buffer for IMEI!
  uint8_t imeiLen = fona.getIMEI(imei);
  if (imeiLen > 0) {
    Serial.print("Module IMEI: "); Serial.println(imei);
  }

}

void loop() {
}