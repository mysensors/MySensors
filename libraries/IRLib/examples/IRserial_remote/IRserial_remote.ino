/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.3   January 2014
 * Copyright 2014 by Chris Young http://cyborg5.com
 * Use serial port to send codes. For details on this project see
   http://tech.cyborg5.com/2013/05/30/irlib-tutorial-part-3-overview-sending-ir-codes/ 
 */
#include <IRLib.h>

IRsend My_Sender;

int protocol;
long code;
int bits;
void setup() {
  Serial.begin(9600);
  delay(2000);while(!Serial);//delay for Leonardo
}

long parseHex (void) {
  long Value=0; char C;delay(100);
  while (Serial.available()>0) {
     C= tolower(Serial.read());
     if ((C>='0')&&(C<='9'))
       C=C-'0';
     else
       if ((C>='a') && (C<='f'))
         C=C-'a'+10;
       else
         return Value;
     Value=  C+(Value<<4);
  };
  return Value;
}
void parseDelimiter () {
  char C;
  while(Serial.available()>0) {
    C=tolower(Serial.peek());
    if( (C>='0') && (C<='9') )return;
    if( (C>='a') && (C<='f') )return;
    C=Serial.read();//throwaway delimiters
    delay (5);
  }
}

void loop() {
  if (Serial.available ()>0) {
    protocol = Serial.parseInt (); parseDelimiter();
    code     = parseHex ();        parseDelimiter();
    bits     = Serial.parseInt (); parseDelimiter();
    Serial.print("Prot:");  Serial.print(protocol);
    Serial.print(" Code:"); Serial.print(code,HEX);
    Serial.print(" Bits:"); Serial.println(bits);
    My_Sender.send(IRTYPES(protocol), code, bits);   
  }
}
