/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.4   March 2014
 * Copyright 2014 by Chris Young http://cyborg5.com
 * Based on original example sketch for IRremote library 
 * Version 0.11 September, 2009I know prescription and no
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
/*
 * This example demonstrates how to extend this library to add a new protocol
 * without actually modifying or recompiling the library itself. It implements the protocol 
 * known as GIcable which is used by some Motorola cable boxes.
 * This is a modified version of the IRecord example.
 * If you would like to make this a permanent part of your library you could copy the class 
 * prototypes to IRLib.h and the code itself to IRLib.cpp. Search for the word "ADDITIONAL"
 * to see where to add various pieces of code. Also see the Samsung36 example for details.
 */
#include <IRLib.h>
#include <IRLibMatch.h>

class IRdecodeGIcable: public virtual IRdecodeBase
{
public:
  bool decode(void); 
};
class IRsendGIcable: public virtual IRsendBase
{
public:
  void send(unsigned long data, bool Repeat);
};

#define GICABLE (LAST_PROTOCOL+1)

/* The IRP notation for this protocol according to
 * http://www.hifi-remote.com/johnsfine/DecodeIR.html#G.I. Cable
 * is "{38.7k,490}<1,-4.5|1,-9>(18,-9,F:8,D:4,C:4,1,-84,(18,-4.5,1,-178)*) {C = -(D + F:4 + F:4:4)}"
 */
bool IRdecodeGIcable::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("GIcable"));
  // Check for repeat
  if (rawlen == 4 && MATCH(rawbuf[1], 490*18) && MATCH(rawbuf[2],2205)) {
    bits = 0;
    value = REPEAT;
    decode_type= static_cast<IRTYPES>GICABLE;
    return true;
  }
  if(!decodeGeneric(36, 18*490, 9*490, 0, 490, 9*490, 2205/*(4.5*490)*/)) return false;
  decode_type= static_cast<IRTYPES>GICABLE;
  return true;
};

void IRsendGIcable::send(unsigned long data, bool Repeat) {
  if(Repeat) {
    enableIROut(39);
    mark (490*18); space (2205); mark (490); space(220);delay (87);//actually "space(87200);"
  } else {
    sendGeneric(data,16, 490*18, 490*9, 490, 490, 490*9, 2205, 39, true);
  }
};

//Create a custom class that combines this new protocol with all the others
class MyCustomSend: 
public virtual IRsend,
public virtual IRsendGIcable
{
public:
  void send(IRTYPES Type, unsigned long data, int nbits);
};
void MyCustomSend::send(IRTYPES Type, unsigned long data, int nbits) {
  if (Type==GICABLE){
    IRsendGIcable::send(data,false);
    //un-comment the line below to text repeats
    //delay(3000); IRsendGIcable::send(data,true);
  }
  else
    IRsend::send(Type, data, nbits);
}
class MyCustomDecode: 
public virtual IRdecode,
public virtual IRdecodeGIcable
{
public:
  virtual bool decode(void);    // Calls each decode routine individually
  void DumpResults(void);
};
bool MyCustomDecode::decode(void) {
  if (IRdecodeGIcable::decode()) return true;
  return IRdecode::decode ();
}
void MyCustomDecode::DumpResults(void){
  if(decode_type==GICABLE) {
    Serial.print(F("Decoded GIcable: Value:")); Serial.print(value, HEX);
  };
  IRdecode::DumpResults();
};

MyCustomDecode My_Decoder;

MyCustomSend My_Sender;

int RECV_PIN =11;

IRrecv My_Receiver(RECV_PIN);
IRTYPES codeType; // The type of code
unsigned long codeValue;   // The data bits
int codeBits; // The length of the code in bits
bool GotOne; 

void setup()
{
  GotOne=false;
  codeType=UNKNOWN; 
  codeValue=0; 
  codeBits=0;
  Serial.begin(9600);
  Serial.println(F("Send a code from your remote and we will record it."));
  Serial.println(F("Type any character and press enter. We will send the recorded code."));
  My_Receiver.enableIRIn(); // Start the receiver
}
void loop() {
  if (Serial.available()>0) {
    unsigned char c=Serial.read();
    if (c=='p') {//Send a test pattern
      GotOne= true;  codeType=GICABLE; codeValue=0x1234; codeBits=16;
    }
    if(GotOne) {
      My_Sender.send(codeType,codeValue,codeBits);
      Serial.print(F("Sent "));
      if (codeType== GICABLE) Serial.print(F("GIcable")); else Serial.print(Pnames(codeType));
      Serial.print(F(" Value:0x"));
      Serial.print(codeValue, HEX);
      Serial.print(F(" Bits:"));
      Serial.println(codeBits, DEC);
      My_Receiver.enableIRIn(); // Re-enable receiver
    }
  } 
  else if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();
    if(My_Decoder.decode_type == UNKNOWN) {
      Serial.println(F("Unknown type received. Ignoring."));
    } else {
      codeType= My_Decoder.decode_type;
      codeValue= My_Decoder.value;
      codeBits= My_Decoder.bits;
      GotOne=true;
    }
    My_Decoder.DumpResults();
    delay(1000);
    My_Receiver.resume(); 
  }
}

