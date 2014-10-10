// PinChangeIntTest
// version 1.0 Wed Feb 15 07:25:09 CST 2012
// Version 1.1 Fri Jun 22 19:10:50 CDT 2012 minor tweaks to eliminate compiler warnings.  Also, there were bugfixes in ByteBuffer.
//                                          I had some "cli()" without "sei()" in there.
// See the Wiki at http://code.google.com/p/arduino-pinchangeint/wiki for more information.
// This sketch requires the ByteBuffer library, which is found in the PinChangeInt zipfile.
//-------- define these in your sketch, if applicable ----------------------------------------------------------
//-------- This must go ahead of the #include statement --------------------------------------------------------
// You can reduce the memory footprint of this handler by declaring that there will be no pin change interrupts
// on any one or two of the three ports.  If only a single port remains, the handler will be declared inline
// reducing the size and latency of the handler.
// #define NO_PORTB_PINCHANGES // to indicate that port b will not be used for pin change interrupts
// #define NO_PORTC_PINCHANGES // to indicate that port c will not be used for pin change interrupts
// #define NO_PORTD_PINCHANGES // to indicate that port d will not be used for pin change interrupts
// You can reduce the code size by 20-50 bytes, and you can speed up the interrupt routine
// slightly by declaring that you don't care if the static variables PCintPort::pinState and/or
// PCintPort::arduinoPin are set and made available to your interrupt routine.
// #define NO_PIN_STATE        // to indicate that you don't need the pinState
// #define NO_PIN_NUMBER       // to indicate that you don't need the arduinoPin
// if there is only one PCInt vector in use the code can be inlined
// reducing latency and code size
// define DISABLE_PCINT_MULTI_SERVICE below to limit the handler to servicing a single interrupt per invocation.
// #define       DISABLE_PCINT_MULTI_SERVICE
// The following is intended for testing purposes.  If defined, then a variable PCintPort::pinMode can be read
// in your interrupt subroutine.  It is not defined by default:
// #define PINMODE
//-------- define the above in your sketch, if applicable ------------------------------------------------------
#define PINMODE
#define FLASH
#include <ByteBuffer.h>
#include <PinChangeInt.h>

// This example demonstrates a configuration of 6 interrupting pins and 3 interrupt functions.
// A variety of interrupting pins have been chosen, so as to test all PORTs on the Arduino.
// The pins are as follows:
#define tPIN1 2  // port D
#define tPIN2 3
#define tPIN3 11 // Port B
#define tPIN4 12
#define tPIN5 A3 // Port C, also can be given as "17"
#define tPIN6 A4 // starts and stops the count
// All pins send interrupts.  Arduino pins 2 and A4 (tPIN1,6) interrupt on FALLING.
// Arduino pins 3 and 12 (tPIN2,4) interrupt on RISING.
// Arduino pins 11 and A3 (tPIN5) interrupts on CHANGE.
// quicfunc0 is attached to Arduino pins 2, 3, 11, and 12 (tPIN1-4)
// quicfunc1 is attached to Arduino pin A3 (tPIN5)
// quicfunc2 is attached to Arduino pin A4 (tPIN6).
// NOTE:
// For the Analog Input pins used as digital input pins, you can use numbers such as 14, 15, 16, etc.
// or you can use A0, A1, A2, etc. (the Arduino code comes with #define's for the Analog Input pin
// names and will properly recognize e.g., pinMode(A0, INPUT));

// HOW IT WORKS
// The interrupt on Arduino pin A4 (tPIN6) will, when triggered, start the counting of interrupts.  
// The array interrupt_count0[20] is updated in the interrupts; each cell keeps track of the number
// of interrupts on one of the 20 available interrupt pins on the Arduino.  Every second in the main
// loop the array is scanned and registered interrupts are reported for all pins interrupted since
// the previous second.  If no interrupts, the output is quiet.

// tPIN6 is special.  Not only does it start the counting of the interrups, but it turns on and off
// interrupts on pins 2, 11, and A3/17 (tPIN1, tPIN3, tPIN5).  All pins start by interrupting, but after
// the count is turned on and then turned off, the 3 pins are detached from interrupts.
// Everytime thereafter when the count is turned off the 3 pins are detached.  They are reattached
// when turned on.

// Output is copied to a buffer, because we can't do a Serial.print() statement in an interrupt
// routine.  The main loop checks  for entries in the buffer and prints them if found.
// Output looks like this:
// -F- - an interrupt triggered by a falling signal occurred.
// +R+ - an interrupt triggered by a rising signal occurred.
// *C* - an interrupt triggered by a change in signal occurred.
// f#p#-P# - f# shows the interrupt subroutine that was called: 0, 1, or 2
//         - p# shows the pin number that triggered the interrupt
//         - P# shows the port that this pin number is attached to. 2 is PORTB, 3 is PORTC, 4 is PORTD

// HOW TO CONNECT
// Each pin gets a momentary contact switch connected to it.  One side of the switch should connect
// to ground.  The other side of the switch connects to the Arduino pin.  For my purposes, I am using
// two rotary encoders.  Each encoder contains 3 switches.  But 6 regular pushbuttons would work, too.

/* WHAT TO LOOK FOR
 Output is sent to the serial line, so the Arduino IDE's serial terminal should be opened.
 Upon startup, press tPINS1-5.  You will see output like this:
-F-f0p2-P4 (counting off)
..*C*f0p11-P2 (counting off)
+R+f0p3-P4 (counting off)
 This shows that
 1. an interrupt was triggered on a falling signal (*F*).  It called (f0) function 0, which is quicfunc0.
    The triggering pin was (p2) Arduuino pin 2, which is on (P4) Port 4 (PORTD).  Counting of this interrupt is
    off, so you will not see any output from the main loop.
 2. Two dots appeared.  Dots came from iterations of loop(), so these 2 dots show that the two interrupts happened 2 seconds apart.
 3. an interrupt was triggered on a change in signal (*C*).  It called quicfunc0, from Arduino pin 11, on Port 2 (PORTB).
    The interrupt was not counted.
 4. an interrupt was triggered on a rising signal (+R+).  It called quicfunc0, from Arduino pin 3, on Purt 4 (PORTD).
    The pin should have started out at the high level, so likely the signal fell during onother interrupt, and now
    the rise has been caught.
    
 Now press the button attached to tPIN6 (in our case, A4 or D18).  You will see something like this:
-F-START! f2p18-P3
.Count for pin A4 is 1
 This shows that
 1. The counting machanism (START!) was triggered by a folling signal (-F-) on pin 18 (p18) which is in Port 3 (P3) (which == PORTC) and
    function f2 was called (f2).
 2. A dot appeared, which came from loop() because a second passed.
 3. The count for p18 or A4 was displayed.
 
 Now you will see messages for all the pins that you manipulate, for example:
*C*f0p11-P2
+R+f0p3-P4
*C*f0p11-P2
+R+f0p3-P4
*C*f0p11-P2
.Count for pin D3 is 6
Count for pin D11 is 9
.+R+f0p3-P4
-F-f0p2-P4
.Count for pin D2 is 1
Count for pin D3 is 1
 These codes reflect the interrupts, as described above.  This output will take place until you press tPIN6:
-F-f2: STOP! Counting off.
Interrupt OFF on tPIN1 (2) tPIN3 (11) tPIN5 (17)
 Then you will see output like this:
.....................+R+f0p12-P2 (counting off)
.+R+f0p12-P2 (counting off)
+R+f0p12-P2 (counting off)
+R+f0p12-P2 (counting off)
 and tPIN1, tPIN3, and tPIN5 will not trigger interrupts.
*/
// NOTES
// Output overwrites:
// It's possible during moderately fast interrupts to see your print output get garbled; eg,
// +R+f0p12-P2 (+R+f0p12-P2 (counting +R+f0p12-P2 (cou+R+f0p12-P+R+f0p12
// This is because the print of the buffer takes place inside a while loop, and it can
// be interrupted and new data inserted into the buffer at a midpoint of the buffer's text.
// Just by spinning my rotary encoders I can readily generate over 200 interrupts per second
// on a pin, which is easily fast enough to overrun Serial output at 115,200 bps.
// The lesson here?  ...Interrupts are tricky, and interrupt service routines should be fast.
// Just sayin'.

// Pins:
// We want to use pins from each of ports B, C and D.  So choose wisely.  Ports are shown in
// this diagram of the ATmega328P chip.  PD0 means "Port D, pin 0".  PC3 means "Port C, Pin 3",
// PB2 means "Port B, pin 2" and so on.  The corresponding Arduino pins are in parentheses.
// So PB2 is Arduino pin D 10, for example.
/*
                  +-\/-+
            PC6  1|    |28  PC5 (AI 5)
      (D 0) PD0  2|    |27  PC4 (AI 4)
      (D 1) PD1  3|    |26  PC3 (AI 3)
      (D 2) PD2  4|    |25  PC2 (AI 2)
 PWM+ (D 3) PD3  5|    |24  PC1 (AI 1)
      (D 4) PD4  6|    |23  PC0 (AI 0)
            VCC  7|    |22  GND
            GND  8|    |21  AREF
            PB6  9|    |20  AVCC
            PB7 10|    |19  PB5 (D 13)
 PWM+ (D 5) PD5 11|    |18  PB4 (D 12)
 PWM+ (D 6) PD6 12|    |17  PB3 (D 11) PWM
      (D 7) PD7 13|    |16  PB2 (D 10) PWM
      (D 8) PB0 14|    |15  PB1 (D 9) PWM
                  +----+
*/

uint8_t pins[6]={ tPIN1, tPIN2, tPIN3, tPIN4, tPIN5, tPIN6 };
uint8_t ports[6]={ 0, 0, 0, 0, 0, 0 };

uint8_t latest_interrupted_pin;
uint8_t interrupt_count[20]={0}; // 20 possible arduino pins
uint8_t port;
uint8_t mode;

ByteBuffer printBuffer(80);
char charArray[16];
char numBuffer[4] = { 0, 0, 0, 0 };
uint8_t printFull=0;

volatile boolean start=0;
volatile boolean initial=true;
long begintime=0;
long now=0;

void uint8ToString(char *outString, uint8_t number) {
  uint8_t hundreds=0;
  uint8_t tens=0;
  uint8_t ones=0;

  while (number >= 100 ) {
    hundreds++;
    number-=100;
  }
  while (number >= 10 ) {
    tens++;
    number-=10;
  }
  ones=number;
  ones+=48;
  if (hundreds > 0) { hundreds+=48; tens+=48; outString[0]=hundreds; outString[1]=tens; outString[2]=ones; outString[3]=0; }
  else if (tens > 0) {  tens+=48; outString[0]=tens; outString[1]=ones; outString[2]=0; }
  else { outString[0]=ones; outString[1]=0; };
}

void showMode() {
  switch (mode) {
  case FALLING:
    printBuffer.putString((char *) "-F-");
  break;
  case RISING:
    printBuffer.putString((char *) "+R+");
  break;
  case CHANGE:
    printBuffer.putString((char *) "*C*");
  break;
  }
}

void quicfunc0() {
  latest_interrupted_pin=PCintPort::arduinoPin;
  mode=PCintPort::pinmode;
  showMode();
  if (start==1) {
    interrupt_count[latest_interrupted_pin]++;
  }
  uint8ToString(numBuffer, latest_interrupted_pin);
  printBuffer.putString((char *) "f0p"); printBuffer.putString(numBuffer); printBuffer.putString((char *) "-P");
  uint8ToString(numBuffer, digitalPinToPort(latest_interrupted_pin));
  printBuffer.putString(numBuffer);
  if (start !=1) printBuffer.putString((char *) " (counting off)");
  printBuffer.putString((char *) "\n");
};

void quicfunc1() {
  latest_interrupted_pin=PCintPort::arduinoPin;
  mode=PCintPort::pinmode;
  showMode();
  if (start==1) {
    interrupt_count[latest_interrupted_pin]++;
  }
  uint8ToString(numBuffer, latest_interrupted_pin);
  printBuffer.putString((char *) "f1p"); printBuffer.putString(numBuffer); printBuffer.putString((char *) "-P");
  uint8ToString(numBuffer, digitalPinToPort(latest_interrupted_pin));
  printBuffer.putString(numBuffer);
  if (start !=1) printBuffer.putString((char *) " (counting off)");
  printBuffer.putString((char *) "\n");
};

void quicfunc2() {
  latest_interrupted_pin=PCintPort::arduinoPin;
  mode=PCintPort::pinmode;
  showMode();
  if (start == 1) {
    printBuffer.putString((char *) "f2: STOP! Counting off.\n");
    printBuffer.putString((char *) "Interrupt OFF on tPIN1 ("); uint8ToString(numBuffer, tPIN1), printBuffer.putString(numBuffer);
    printBuffer.putString((char *) ") tPIN3 (");uint8ToString(numBuffer, tPIN3), printBuffer.putString(numBuffer);
    printBuffer.putString((char *) ") tPIN5 (");uint8ToString(numBuffer, tPIN5), printBuffer.putString(numBuffer);
    printBuffer.putString((char *) ")\n");
    PCintPort::detachInterrupt(tPIN1); PCintPort::detachInterrupt(tPIN3); PCintPort::detachInterrupt(tPIN5); 
    start=0;
  } else {
    start=1;
    interrupt_count[latest_interrupted_pin]++;
    printBuffer.putString((char *) "START! f2p");
    uint8ToString(numBuffer, latest_interrupted_pin);
    printBuffer.putString(numBuffer); printBuffer.putString((char *) "-P");
    uint8ToString(numBuffer, digitalPinToPort(latest_interrupted_pin));
    printBuffer.putString(numBuffer); printBuffer.putString((char *) "\n");
    if (! initial) {
      PCintPort::attachInterrupt(tPIN1, &quicfunc0, FALLING);
      PCintPort::attachInterrupt(tPIN3, &quicfunc0, CHANGE);
      PCintPort::attachInterrupt(tPIN5, &quicfunc1, CHANGE);
    } else {
      initial=false;
    }                                                                                                
  }
};

uint8_t i;
void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("Test");
  delay(500);
  for (i=0; i < 7; i++) {
    pinMode(pins[i], INPUT); digitalWrite(pins[i], HIGH);
    ports[i]=digitalPinToPort(pins[i]);
    switch (pins[i]) {
    case tPIN1:
        PCintPort::attachInterrupt(pins[i], &quicfunc0, FALLING);
    break;
    case tPIN3:
        PCintPort::attachInterrupt(pins[i], &quicfunc0, CHANGE);
    break;
    case tPIN2:
    case tPIN4:
        PCintPort::attachInterrupt(pins[i], &quicfunc0, RISING);
    break;
    case tPIN5:
        PCintPort::attachInterrupt(pins[i], &quicfunc1, CHANGE);
    break;
    case tPIN6:
        PCintPort::attachInterrupt(pins[i], &quicfunc2, FALLING);
    break;
    }
  }
  //Serial.println(printBuffer.getCapacity(), DEC);
  //Serial.println("*---------------------------------------*");
  Serial.print("*---*");
  delay(250);
  begintime=millis();
}

void loop() {
  now=millis();
  uint8_t count;
  char outChar;
  // uint8_t bufsize;
  //if (printBuffer.getSize() != 0) { Serial.print("SZ:"); Serial.println (printBuffer.getSize(), DEC); };
  //bufsize=printBuffer.getSize();
  //if (bufsize > 0) { Serial.print("S:"); Serial.println(bufsize); }
  while ((outChar=(char)printBuffer.get()) != 0) Serial.print(outChar);
  if ((now - begintime) > 1000) {
    Serial.print(".");
    if (printBuffer.checkError()) {
      Serial.println("NOTICE: Some output lost due to filled buffer.");
    }
    for (i=0; i < 20; i++) {
      if (interrupt_count[i] != 0) {
        count=interrupt_count[i];
        interrupt_count[i]=0;
        Serial.print("Count for pin ");
        if (i < 14) {
          Serial.print("D");
          Serial.print(i, DEC);
        } else {
          Serial.print("A");
          Serial.print(i-14, DEC);
        }
        Serial.print(" is ");
        Serial.println(count, DEC);
      }
    }
    begintime=millis();
  }
}

