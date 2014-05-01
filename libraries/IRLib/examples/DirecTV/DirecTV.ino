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
 * used by DirecTV. This protocol actually comes in six different varieties.
 * It uses three different frequencies 38, 40, or 57 kHz. It also uses two different varieties
 * lead out times either 9000us or 30000us. The default is 38 kHz and 30000us.
 * You can use a different constructor for the other varieties.
 * This is a modified version of the IRecord example.
 * If you would like to make this a permanent part of your library you could copy the class 
 * prototypes to IRLib.h and the code itself to IRLib.cpp. Search for the word "ADDITIONAL"
 * to see where to add various pieces of code. Also see the Samsung36 example for details.
 */
#include <IRLib.h>
#include <IRLibMatch.h>

class IRdecodeDirecTV: public virtual IRdecodeBase
{
public:
  bool decode(void); 
  bool Repeat;
};

class IRsendDirecTV: public virtual IRsendBase
{
public:
  IRsendDirecTV (int khz= 38,bool LongLeadOut=true);
  void send(unsigned long data, bool Repeat);
  int Freq;
  unsigned int LeadOut;
};

#define DIRECTV (LAST_PROTOCOL+1)

/*
 * According to http://www.hifi-remote.com/johnsfine/DecodeIR.html#DirecTV
 * The IRP notation for this protocol is:
 *{38k,600,msb}<1,-1|1,-2|2,-1|2,-2>(5,(5,-2,D:4,F:8,C:4,1,-50)+) {C=7*(F:2:6)+5*(F:2:4)+3*(F:2:2)+(F:2)} 
 * Unlike most protocols which use a fixed length mark and a variable length or a variable length
 * mark a fixed length space, this protocol varies both the mark and the space.
 * The stream is still a series of marks and spaces but the length of either of those
 * denotes a one or zero. A length of 1200us=logical 1 and length 600us=logical 0
 * It also makes changes to the length of the header mark to devote repeat codes.
 * The first header mark should be 6000us but repeat codes should only be 3000us.
 */
bool IRdecodeDirecTV::decode(void) {
  long data;  int offset; 
  IRLIB_ATTEMPT_MESSAGE(F("DirecTV"));
  if (rawlen != 20) return RAW_COUNT_ERROR;
  if (MATCH(rawbuf[1],3000))
    Repeat=true;
  else 
    if (!MATCH(rawbuf[1],6000)){
      return HEADER_MARK_ERROR(6000);
    } else {
      Repeat=false;
    }
  if (!MATCH(rawbuf[2],1200)) return HEADER_SPACE_ERROR(1200);
  offset=3; data=0;
  while (offset < 18) {
      if (MATCH(rawbuf[offset],1200)) {
        data = (data << 1) | 1;
      } 
      else if (MATCH(rawbuf[offset],600)) {
        data <<= 1;
      } 
      else return DATA_MARK_ERROR(1200);
      offset++;
      if (MATCH(rawbuf[offset],1200)) {
        data = (data << 1) | 1;
      } 
      else if (MATCH (rawbuf[offset],600)) {
        data <<= 1;
      } 
      else return DATA_SPACE_ERROR(1200);
      offset++;
  }
  if (!MATCH(rawbuf[1],6000))  return DATA_MARK_ERROR(6000);
  bits = 16;//set bit length
  value = data;//put remaining bits in value
  decode_type= static_cast<IRTYPES>DIRECTV;
  return true;
};

//See the comment at the top about different frequencies and lead out times
IRsendDirecTV::IRsendDirecTV (int khz, bool LongLeadOut){
  Freq=khz;
  if (LongLeadOut) LeadOut=50*600; else LeadOut=15*600;
};

void IRsendDirecTV::send(unsigned long data, bool Repeat) {
    enableIROut(Freq);
    if(Repeat) mark(3000); else mark(6000); 
    space(1200);//Send header
    for (int i = 0; i < 8; i++) {
      if (data & 0x8000) mark(1200); else mark(600);  
      data <<= 1;
      if (data & 0x8000) space(1200); else space(600);
      data <<= 1;
    };
    mark(600);  
    space(LeadOut);
};

//Create a custom class that combines this new protocol with all the others
class MyCustomSend: 
public virtual IRsend,
public virtual IRsendDirecTV
{
public:
  void send(IRTYPES Type, unsigned long data, int nbits);
};
void MyCustomSend::send(IRTYPES Type, unsigned long data, int nbits) {
  if (Type==DIRECTV)
    IRsendDirecTV::send(data,false);
  else
    IRsend::send(Type, data, nbits);
}
class MyCustomDecode: 
public virtual IRdecode,
public virtual IRdecodeDirecTV
{
public:
  virtual bool decode(void);    // Calls each decode routine individually
  void DumpResults(void);
};
bool MyCustomDecode::decode(void) {
  if (IRdecodeDirecTV::decode()) return true;
  return IRdecode::decode ();
}
void MyCustomDecode::DumpResults(void){
  if(decode_type==DIRECTV) {
    Serial.print(F("Decoded DirecTV: Value:")); Serial.print(value, HEX);
    if (Repeat) Serial.print(F(" repeat flag"));
  };
  IRdecode::DumpResults();
};

MyCustomDecode My_Decoder;

MyCustomSend My_Sender;

int RECV_PIN = 11;

IRrecv My_Receiver(RECV_PIN);
IRTYPES codeType;         // The type of code
unsigned long codeValue;  // The data bits
int codeBits;             // The length of the code in bits
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
      GotOne= true;  codeType=DIRECTV; codeValue=0x1234; codeBits=16;
    }
    if(GotOne) {
      My_Sender.send(codeType,codeValue,codeBits);
      Serial.print(F("Sent "));
      if (codeType== DIRECTV) Serial.print(F("DirecTV")); else Serial.print(Pnames(codeType));
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

