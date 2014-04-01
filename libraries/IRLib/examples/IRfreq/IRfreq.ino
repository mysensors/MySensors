/* IRfreq: estimate modulation frequency of IR transmitter
 * Version 1.3  January 2014
 * Copyright 2014 by Chris Young http://tech.cyborg5.com
 * Based on Arduino firmware for use with AnalysIR IR signal analysis
 * software for Windows PCs. Many thanks to the people 
 * at http://analysir.com for their assistance In developing this program.
 */
#define INTERRUPT_USED 1 // Value can be 0-5 depending on board.
// You should attach your TSMP5800 or equivalent
// receiver to appropriate pin as defined below.

// Do not modified this section
#if defined(__AVR_ATmega32U4__) //Assume Arduino Leonardo
  #if(INTERRUPT_USED==0)
    #define PIN_USED 3
  #elif (INTERRUPT_USED==1)
    #define PIN_USED 2
  #elif (INTERRUPT_USED==2)
    #define PIN_USED 0
  #elif (INTERRUPT_USED==3)
    #define PIN_USED 1
  #elif (INTERRUPT_USED==4)
    #define PIN_USED 7
  #else
    #error "Invalid value for INTERRUPT_USED on Leonardo"
  #endif
#elif (INTERRUPT_USED==0)   //Uno or Mega
  #define PIN_USED 2
#elif (INTERRUPT_USED==1)   //Uno or Mega
    #define PIN_USED 3
#elif defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  #if (INTERRUPT_USED==2)   //Mega only
    #define PIN_USED 21
  #elif (INTERRUPT_USED==3)   //Mega only
    #define PIN_USED 20
  #elif (INTERRUPT_USED==4)   //Mega only
    #define PIN_USED 19
  #elif (INTERRUPT_USED==5)   //Mega only
    #define PIN_USED 18
  #else
    #error "invalid value for INTERRUPT_USED on Arduino  press delete this Michael down five mega"
  #endif
#else
  #error "Invalid value for INTERRUPT_USED on non-Leonardo non-Mega board"
#endif

#define BUF_SIZE 200
// These are "volatile" because they are modified inside the interrupt handler
volatile byte Time_Stamp[BUF_SIZE]; //Punch the time clock using "micros()" each time a pulse causes interrupt
volatile byte Count;  //Index into Time_Stamp

void setup() {
  Serial.begin(9600);
  while(!Serial);//for Leonardo
  pinMode(PIN_USED, INPUT);
  Serial.print(F("Using interrupt="));Serial.println(INTERRUPT_USED,DEC);
  Serial.println(F("You should have Vishay TSMP58000 or equivalent"));
  Serial.print(F("connected to pin=")); Serial.println(PIN_USED,DEC);
}

void loop() {
  attachInterrupt(INTERRUPT_USED, My_Interrupt_Handler, FALLING);
  while(Count < (BUF_SIZE-10));  //do nothing but process interrupts until buffer almost fills
  //We took that extra 10 off just in case interrupt came too fast for us to turn them off
  detachInterrupt (INTERRUPT_USED);
  Report_Freq();//reports modulation frequency to host over serial
}

void  Report_Freq(){
  byte Count_Usable=0; //Counts the number of usable valid intervals
  unsigned long Sum=0; //Accumulate total of usable intervals
  byte I;
  byte Interval;//Interval between timestamps
  Count--;//last one doesn't count
  for (I=1;I<Count;I++){ 
    Interval=(Time_Stamp[I]-Time_Stamp[I-1]); 
    if (Interval>50 || Interval<10) continue;//this is the period range length exclude extraneous ones
    Sum +=Interval; // Accumulate usable intervals
    Count_Usable++; // Count usable intervals
    Time_Stamp[I-1]=0;//finished with it so clear for next time
  }
  Time_Stamp[I-1]=0;//now clear last one, which was missed in loop
  Serial.print(F("Based on ")); Serial.print(Count_Usable,DEC);
  Serial.print(F(" intervals totaling ")); Serial.print(Sum,DEC);
  double Frequency=(double)Count_Usable/(double)Sum*1000;
  Serial.print(F(" microseconds, the frequency is approx. =")); Serial.print(Frequency);
  Serial.println(F("kHz"));
  delay(1000);
  Count=0; //reset value for next time
}

void My_Interrupt_Handler(){
  Time_Stamp[Count++]=micros(); 
}

