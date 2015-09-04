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
 * without actually modifying or recompiling the libnonstickrary itself. It implements the 
 * Phillips RC-MM also known as Nokia Protocol. Is used by AT&T U-Verse cable boxes.
 * There are three different varieties that are 12, 24, or 32 bits.
 * This is a modified version of the IRecord example.
 * If you would like to make this a permanent part of your library you could copy the class 
 * prototypes to IRLib.h and the code itself to IRLib.cpp. Search for the word "ADDITIONAL"
 * to see where to add various pieces of code. Also see the Samsung36 example for details.
 */
#include <IRLib.h>
#include <IRLibMatch.h>

class IRdecodeRCMM: public virtual IRdecodeBase
{
public:
  bool decode(void); 
};

class IRsendRCMM: public virtual IRsendBase
{
public:
  void send(unsigned long data, unsigned char nbits);
};

#define RCMM (LAST_PROTOCOL+1)
#define RCMM_HEAD_MARK 417
#define RCMM_DATA_MARK 167
#define RCMM_ZERO      278
#define RCMM_ONE       444
#define RCMM_TWO       611
#define RCMM_THREE     778

/*
 * According to http://www.hifi-remote.com/johnsfine/DecodeIR.html#Nokia
 * The IRP notation for these protocols are:
 * Nokia 12 bit: {36k,msb}<164,-276|164,-445|164,-614|164,-783>(412,-276,D:4,F:8,164,-???)+ 
 * Nokia 24-bit: {36k,msb}<164,-276|164,-445|164,-614|164,-783>(412,-276,D:8,S:8,F:8,164,-???)+ 
 * Nokia 32 bit: {36k,msb}<164,-276|164,-445|164,-614|164,-783>(412,-276,D:8,S:8,X:8,F:8,164,^100m)+ 
 * Slightly different timing values are documented at 
 * http://www.sbprojects.com/knowledge/ir/rcmm.php
 * We will use the timing from the latter reference.
 * Unlike most protocols which defined sequences for a logical "0" and "1", this protocol
 * encodes 2 bits per pulse. Therefore it encodes a logical "2" and "3" as well.
 * The length of the mark is constant but the length of the space denotes the bit values.
 * Note the 32-bit version uses a toggle bit of 0x8000 and as usual it is up to the end-user
 * to implement it outside the library routines.
 */
/*
 * Normally IRLib uses a plus or minus percentage to determine if an interval matches the
 * desired value. However this protocol uses extremely long intervals of similar length.
 * For example using the default 25% tolerance the RCMM_TWO value 611 would be accepted for 
 * anything between 458 and 763. The low end is actually closer to RCMM_ONE value of 444
 * and the upper range is closer to RCM_THREE value of 778. To implement this protocol
 * we created a new match routine ABS_MATCH which allows you to specify an absolute
 * number of microseconds of tolerance for comparison.
 */ 
#define RCMM_TOLERANCE 80
bool IRdecodeRCMM::decode(void) {
  long data; 
  IRLIB_ATTEMPT_MESSAGE(F("RCMM"));
  if ( (rawlen!=(12+2)) && (rawlen!=(24+2)) && (rawlen!=(32+4)) ) return RAW_COUNT_ERROR;
  if (!MATCH(rawbuf[1],RCMM_HEAD_MARK)) return HEADER_MARK_ERROR(RCMM_HEAD_MARK);
  if (!MATCH(rawbuf[2],RCMM_ZERO)) return HEADER_SPACE_ERROR(RCMM_ZERO);
  offset=3; data=0;
  while (offset < (rawlen-1)) {
      if (!ABS_MATCH(rawbuf[offset],RCMM_DATA_MARK, RCMM_TOLERANCE+50)) return DATA_MARK_ERROR(RCMM_DATA_MARK);
      offset++;
      if (ABS_MATCH(rawbuf[offset],RCMM_ZERO, RCMM_TOLERANCE) ) { //Logical "0"
        data <<= 2;
      } 
      else if (ABS_MATCH(rawbuf[offset],RCMM_ONE, RCMM_TOLERANCE) ) { //Logical "1"
        data = (data<<2) + 1;
      } 
      else if (ABS_MATCH(rawbuf[offset],RCMM_TWO, RCMM_TOLERANCE) ) { //Logical "2"
        data = (data<<2) + 2;
      } 
      else if (ABS_MATCH(rawbuf[offset],RCMM_THREE, RCMM_TOLERANCE) ) { //Logical "3"
        data = (data<<2) + 3;
      } 
      else return DATA_SPACE_ERROR(RCMM_ZERO);
      offset++;
  }
  if (!MATCH(rawbuf[offset],RCMM_DATA_MARK))  return DATA_MARK_ERROR(RCMM_DATA_MARK);
  bits = rawlen-4;//set bit length
  value = data;//put remaining bits in value
  decode_type= static_cast<IRTYPES>RCMM;
  return true;
};

void IRsendRCMM::send(unsigned long data, unsigned char nbits) {
  Extent=0;
  data = data << (32 - nbits);
  nbits=nbits/2;
  enableIROut(36);
  mark(RCMM_HEAD_MARK); space(RCMM_ZERO);//Send header
  for (int i = 0; i < nbits; i++) {
    mark(RCMM_DATA_MARK);
    switch (data & 0xC0000000UL) {//use the leftmost two bits
      case 0x00000000UL: space(RCMM_ZERO); break;
      case 0x40000000UL: space(RCMM_ONE); break;
      case 0x80000000UL: space(RCMM_TWO); break;
      case 0xC0000000UL: space(RCMM_THREE); break;
      default: Serial.println("sending error");
    }
    data <<= 2;
  };
  mark(RCMM_DATA_MARK);  
  space(27778-Extent);
};

//Create a custom class that combines this new protocol with all the others
class MyCustomSend: 
public virtual IRsend,
public virtual IRsendRCMM
{
public:
  void send(IRTYPES Type, unsigned long data, int nbits);
};
void MyCustomSend::send(IRTYPES Type, unsigned long data, int nbits) {
  if (Type==RCMM)
    IRsendRCMM::send(data,nbits);
  else
    IRsend::send(Type, data, nbits);
}
class MyCustomDecode: 
public virtual IRdecode,
public virtual IRdecodeRCMM
{
public:
  virtual bool decode(void);    // Calls each decode routine individually
  void DumpResults(void);
};
bool MyCustomDecode::decode(void) {
  if (IRdecodeRCMM::decode()) return true;
  return IRdecode::decode ();
}
void MyCustomDecode::DumpResults(void){
  if(decode_type==RCMM) {
    Serial.print(F("Decoded RCMM: Value:")); Serial.print(value, HEX);
  };
  IRdecode::DumpResults();
};

MyCustomDecode My_Decoder;

MyCustomSend My_Sender;

int RECV_PIN = 11;

IRrecv My_Receiver(RECV_PIN);
IRTYPES codeType;         // The type of code
unsigned long codeValue;  // The data bits
int codeBits;             // The length of the code in bits or for Samsung is storage for data2
bool GotOne; 
char c;

void setup()
{
  GotOne=false;
  codeType=UNKNOWN; 
  codeValue=0; 
  codeBits=0;
  Serial.begin(9600);while (! Serial) {};
  Serial.println(F("Send a code from your remote and we will record it."));
  Serial.println(F("Type any character and press enter. We will send the recorded code."));
  My_Receiver.enableIRIn(); // Start the receiver
  My_Receiver.Mark_Excess=40;
}
void loop() {
  if (Serial.available()>0) {
    c=Serial.read();
    if (c=='p') {//Send a test pattern
      GotOne= true;  codeType=RCMM; codeValue=0x12345678; codeBits= 32;
    }
    if(GotOne) {
      My_Sender.send(codeType,codeValue,codeBits);
      Serial.print(F("Sent "));
      if (codeType== RCMM) Serial.print(F("RCMM")); else Serial.print(Pnames(codeType));
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

