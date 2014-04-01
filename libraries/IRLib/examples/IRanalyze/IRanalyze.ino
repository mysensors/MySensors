/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.3   January 2014
 * Copyright 2014 by Chris Young http://cyborg5.com
 * Based on original example sketch for IRremote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
/* IRanalyze receives repeated values from a remote and averages the results. Should help in
 * analyzing unknown protocols. You have to press the same key repeatedly. If you press a 
 * different key the totals reset and it computes new averages.
 */
#include <IRLib.h>
#include <IRLibRData.h>

int RECV_PIN = 11;

unsigned int Samples,i, LowSpace, LowMark, HighMark, HighSpace, interval,
            balm,aalm,bahm,aahm,bals,aals,bahs,aahs;
unsigned char bacm,aacm,bacs,aacs, Mark_Count,Space_Count;
unsigned int Accumulated_Time[RAWBUF];
unsigned long Mark_Avg, Space_Avg,baam,aaam,baas,aaas;
//Try this program with various receivers
IRrecv     My_Receiver(RECV_PIN);
//IRrecvLoop My_Receiver(RECV_PIN);
//Use interrupt=0. This is pin 2 on Arduino Uno and Mega, pin 3 on Leonardo
//IRrecvPCI  My_Receiver(0);

IRdecode My_Decoder;
IRTYPES Old_Type;
unsigned long Old_Value;
void setup()
{
  Serial.begin(9600);
  delay(1000);while(!Serial);
  My_Receiver.enableIRIn();
//  My_Receiver.Mark_Excess=50;//Try different values here
  Samples=0;Old_Value=0; Old_Type=UNKNOWN;
  Serial.println(F("Send a signal repeatedly. We will report averages and statistics."));
}
void Tab(void) {Serial.print("\t");};
void loop() {
  if (My_Receiver.GetResults(&My_Decoder)) {
    My_Decoder.decode();
    if( (My_Decoder.decode_type != Old_Type) || (My_Decoder.value != Old_Value)) {
      Serial.println(F("Resetting counters"));
      for(i=0;i<RAWBUF;i++) {
        Accumulated_Time[i]=0;
      };
      Samples=0;Old_Value= My_Decoder.value; Old_Type=My_Decoder.decode_type;
    };
    Samples++;
    Serial.print(F("\nSamples=")); Serial.println(Samples,DEC);
    LowSpace = LowMark=65535;
    HighSpace = HighMark=0;
    Mark_Avg= Space_Avg= Mark_Count= Space_Count=0;
    for(i=0;i<My_Decoder.rawlen;i++){
      Accumulated_Time[i]+=My_Decoder.rawbuf[i];
      My_Decoder.rawbuf[i]= Accumulated_Time[i]/Samples;//Put back average so DumpResults can report
    }
    My_Decoder.DumpResults();
    //Perform additional analysis
    for(i=3;i<My_Decoder.rawlen;i++){ //Compute low, high and average mark and space
      interval=My_Decoder.rawbuf[i];
      if(i % 2) {
        Mark_Avg += interval; LowMark=min(LowMark, interval);  HighMark=max(HighMark, interval);Mark_Count++;
      } else {
        Space_Avg += interval; LowSpace=min(LowSpace, interval);  HighSpace=max (HighSpace, interval);Space_Count++;
      }
      My_Decoder.rawbuf[i]= Accumulated_Time[i]/Samples;//Put back average so DumpResults can report
    }
    Mark_Avg /= Mark_Count; Space_Avg /= Space_Count;
    //Now compute below average highs and lows and above average highs and lows
    balm=bals=aalm=aals=32766;
    bahm=bahs=aahm=aahs=baam=baas=aaam=aaas=0;
    bacm=bacs=aacm=aacs=0;
    for(i=3;i<My_Decoder.rawlen;i++){
      interval=My_Decoder.rawbuf[i];
      if(i % 2) {
        if (interval>Mark_Avg) {
          aalm=min(aalm,interval); aahm=max(aahm,interval);aacm++;aaam+=interval;
        } else {
          balm=min(balm,interval); bahm=max(bahm,interval);bacm++;baam+=interval;
        }
      } else {
        if (interval>Space_Avg) {
          aals=min(aals,interval); aahs=max(aahs,interval);aacs++;aaas+=interval;
        } else {
          bals=min(bals,interval); bahs=max(bahs,interval);bacs++;baas+=interval;
        }
      }
    }
    baam/=bacm;baas/=bacs;aaam/=aacm;aaas/=aacs;
    Serial.println(F("\t #\tLow\tHigh\tAvg.")); 
    Serial.print(F("Mark\t"));Serial.print(Mark_Count,DEC);Tab();Serial.print(LowMark,DEC);Tab();Serial.print(HighMark,DEC);Tab();Serial.println(Mark_Avg,DEC);
    Serial.print(F("Space\t"));Serial.print(Space_Count,DEC);Tab();Serial.print(LowSpace,DEC);Tab();Serial.print(HighSpace,DEC);Tab();Serial.println(Space_Avg,DEC);

    Serial.println(F("\nMarks\t\t #\tLow\tHigh\tAvg.")); 
    Serial.print(F("Above Avg\t"));Serial.print(aacm,DEC);Tab();Serial.print(aalm,DEC);Tab();Serial.print(aahm,DEC);Tab();Serial.println(aaam,DEC);
    Serial.print(F("Belows Avg.\t"));Serial.print(bacm,DEC);Tab();Serial.print(balm,DEC);Tab();Serial.print(bahm,DEC);Tab();Serial.println(baam,DEC);
    
    Serial.println(F("\nSpaces\t\t #\tLow\tHigh\tAvg.")); 
    Serial.print(F("Above Avg.\t"));Serial.print(aacs,DEC);Tab();Serial.print(aals,DEC);Tab();Serial.print(aahs,DEC);Tab();Serial.println(aaas,DEC);
    Serial.print(F("Below Avg.\t"));Serial.print(bacs,DEC);Tab();Serial.print(bals,DEC);Tab();Serial.print(bahs,DEC);Tab();Serial.println(baas,DEC);

#define RATIO(n,d) Tab(); Serial.print(float(n)/float(d));
    Serial.print(F("\nRatios\t\tBelow\tAbove\n"));
    Serial.print(F("Head Mark"));RATIO(My_Decoder.rawbuf[1],baam);RATIO(My_Decoder.rawbuf[1],aaam);
    Serial.print(F("\nHead Space"));RATIO(My_Decoder.rawbuf[2],baas);RATIO(My_Decoder.rawbuf[2],aaas);
    Serial.print(F("\n\nMark Above/Below="));RATIO(aaam,baam);
    Serial.print(F("\nSpace Above/Below="));RATIO(aaas,baas);
    Serial.println();
    delay(500);
    My_Receiver.resume();
  };
}

