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
 * without actually modifying or recompiling the library itself. It implements a 36 bit
 * Samsung protocol that is used on a Blu-ray player that I own.
 * Because a 36 bit value will not fit in the value field (only 32 bits) we have to create
 * a second value field.  This is a modified version of the IRecord example.
 */
#include <IRLib.h>
#include <IRLibMatch.h>
/* Although it is not necessary for this example to run, everything in this section
 * this section of the code could be moved to the actual IRLib.h file
 */
class IRdecodeSamsung36: public virtual IRdecodeBase
{
public:
  bool decode(void); 
  unsigned int value2;
  void Reset(void);
private:
  bool GetBit(void);
  int offset;
  unsigned long data;
};

class IRsendSamsung36: public virtual IRsendBase
{
public:
  void send(unsigned long data, unsigned long data2);
private:
  void PutBits (unsigned long data, int nbits);
};
/* If moving this code to IRLib.h instead of the line below at about 
   line 60 you should search for the line that says...
#define ADDITIONAL (number) 
   and change the code to read as follows‘
#define NECX 7
#define SAMSUNG36 8
#define HASH_CODE 9
#define LAST_PROTOCOL HASH_CODE
   Also at approximately line 160 look for the line that says...
   , public virtual IRdecodeADDITIONAL 
   and change it to read...
public virtual IRdecodeNECx,     //Be sure to add the comma
public virtual IRdecodeSamsung36 //No comma here
   And at about line 238 change to read...
public virtual IRsendNECx,      //Be sure to add the comma
public virtual IRsendSamsung36  //No comma here
  Which will add this protocol to the master send and decode classes
*/
#define SAMSUNG36 (LAST_PROTOCOL+1)

/* Although it is not necessary for this example to run, everything in this section
 * this section of the code could be moved to the actual IRLib.cpp file
 */
/*
 * According to http://www.hifi-remote.com/johnsfine/DecodeIR.html#Samsung36
 * The IRP notation for this protocol is:
 * {38k,500}<1,-1|1,-3>(9,-9,D:8,S:8,1,-9,E:4,F:8,-68u,~F:8,1,-118)+ 
 * This means it uses 38k frequency. Base timing is multiples of 500.
 * A "0" is mark(500) space(500). A "1" is mark (500) space(1500)
 * The header is mark(4500) space(4500).
 * The header is followed by 16 bits (8 device, 8 sub device)
 * This is followed by a mark(500) space(4500).
 * This is followed by 12 more bits (4+8)
 * This is followed by 68us ofspace. Followed by eight more bits
 * and a final stop bit.
*/
/* Because not all of the data bits are contiguous in the stream
 * we created this little routine to get one data bit.
 * We then call it in a loop as needed.
 */
bool IRdecodeSamsung36::GetBit(void) {
  if (!MATCH(rawbuf[offset],500)) return DATA_MARK_ERROR(500);
  offset++;
  if (MATCH(rawbuf[offset],1500)) 
    data = (data << 1) | 1;
  else if (MATCH(rawbuf[offset],500)) 
    data <<= 1;
  else return DATA_SPACE_ERROR(1500);
  offset++;
  return true;
};
bool IRdecodeSamsung36::decode(void) {
  IRLIB_ATTEMPT_MESSAGE(F("Samsung36"));
  if (rawlen != 78) return RAW_COUNT_ERROR;
  if (!MATCH(rawbuf[1],4500))  return HEADER_MARK_ERROR(4500);
  if (!MATCH(rawbuf[2],4500)) return HEADER_SPACE_ERROR(4500);
  offset=3; data=0;
  //Get first 16 bits
  while (offset < 16*2+2) if(!GetBit()) return false;
  //Skip middle header
  if (!MATCH(rawbuf[offset],500))  return DATA_MARK_ERROR(500);
  offset++;
  if (!MATCH(rawbuf[offset],4500)) return DATA_SPACE_ERROR(4400);
  //save first 16 bits in "value2" and reset data
  offset++; value2=data; data=0;
  //12 bits into this second segment there is a 68us space
  //so we find one of the raw values to eliminate that
  rawbuf[62]=rawbuf[62]-68;
  //this gets remaining 20 bits
  while(offset<77)if(!GetBit()) return false;
  bits =36;//set bit length
  value = data;//put remaining 12 bits in value
  decode_type= static_cast<IRTYPES>SAMSUNG36;
  return true;
};
//The Reset method clears out the data however we have extra data to clear
void IRdecodeSamsung36::Reset(void) {
  IRdecodeBase::Reset();//respect your parents
  value2=0;
};
//Similar to GetBits, this routine sends just one bit of data.
//Need this because data bits are not contiguous in the stream
void IRsendSamsung36::PutBits (unsigned long data, int nbits){
   for (int i = 0; i < nbits; i++) {
      if (data & 0x80000000) {
        mark(500);  space(1500);
      } else {
        mark(500);  space(500);
      };
      data <<= 1;
   }
}

void IRsendSamsung36::send(unsigned long data, unsigned long data2) {
   data <<= 32-20;
   data2 <<= 32-16;
   enableIROut(38);
   mark(4500); space(4500);//Send header
   PutBits (data2, 16);//Send data2 16 bits
   mark (500); space (4500);//Send break
   PutBits (data, 12);//Send 12 bits
   space(68);//Send tiny break
   data <<= 12;
   PutBits (data, 8);mark(500); //Final eight bits and one stop bit
   space(118*500);//Lead out is 118 times the base time 500
};
/*
 * This concludes the portion that you could move to IRLib.cpp
 * Additionally at apprx. line 39 add F("Samsung36"), to the list
 * between NECx and hash code.
 * Also add "case SAMSUNG36:IRsendSamsung36::send(data,nbits); break;
 * at about line 238.  Also at about 400 add
 *   if (IRdecodeSamsung36::decode()) return true;
 */

/*
 * The remaining code is unique to the example sketch.
 * We will create a custom class which is a combination
 * of all other protocols and our new protocol.
 */

class MyCustomSend: 
public virtual IRsend,
public virtual IRsendSamsung36
{
public:
//Note: data2 is the number of bits for non-Samsung36 protocols
  void send(IRTYPES Type, unsigned long data, int data2);
};
void MyCustomSend::send(IRTYPES Type, unsigned long data, int data2) {
  if (Type==SAMSUNG36)
    IRsendSamsung36::send(data,data2);
  else
    IRsend::send(Type, data, data2);
}
class MyCustomDecode: 
public virtual IRdecode,
public virtual IRdecodeSamsung36
{
public:
  virtual bool decode(void);    // Calls each decode routine individually
  void DumpResults(void);
};
bool MyCustomDecode::decode(void) {
  if (IRdecodeSamsung36::decode()) return true;
  return IRdecode::decode ();
}
void MyCustomDecode::DumpResults(void){
  if(decode_type==SAMSUNG36) {
    Serial.print(F("Decoded Samsung36: Value:")); Serial.print(value, HEX);
    Serial.print(F(": Value2:")); Serial.print(value2, HEX);
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

void setup()
{
  GotOne=false;
  codeType=UNKNOWN; 
  codeValue=0; 
  codeBits=0;//Stores data2 on Samsung36. Stores number of bits on another protocols
  Serial.begin(9600);
  delay(2000); while(!Serial);
  Serial.println(F("Send a code from your remote and we will record it."));
  Serial.println(F("Type any character and press enter. We will send the recorded code."));
  My_Receiver.enableIRIn(); // Start the receiver
}
void loop() {
  if (Serial.available()>0) {
    unsigned char c=Serial.read();
    if (c=='p') {//Send a test pattern
      GotOne= true;  codeType=SAMSUNG36; codeValue=0x12345; codeBits=0x6789;
    }
    if(GotOne) {
      My_Sender.send(codeType,codeValue,codeBits);
      Serial.print(F("Sent "));
      if (codeType==SAMSUNG36) {
        Serial.print(F("Samsung36 Value:0x"));
        Serial.print(codeValue, HEX);
        Serial.print(F(" Value2:0x"));
        Serial.println(codeBits, HEX);
      } else {
        Serial.print(Pnames(codeType));
        Serial.print(F(" Value:0x"));
        Serial.print(codeValue, HEX);
        Serial.print(F(" Bits:"));
        Serial.println(codeBits, DEC);
      };
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

