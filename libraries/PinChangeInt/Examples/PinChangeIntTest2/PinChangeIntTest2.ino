//#define DISABLE_PCINT_MULTI_SERVICE
#define PINMODE
#define FLASH
#include <GetPSTR.h>
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

uint8_t pins[6]={ tPIN1, tPIN2, tPIN3, tPIN4, tPIN5, tPIN6 };
uint8_t ports[6]={ 0, 0, 0, 0, 0, 0 };

uint8_t latest_interrupted_pin;
uint8_t interrupt_count[20]={0}; // 20 possible arduino pins
uint8_t port;
uint8_t mode;

ByteBuffer printBuffer(200);
char charArray[16];
char numBuffer[5] = { 0, 0, 0, 0, 0 };
uint8_t printFull=0;

volatile boolean start=0;
volatile boolean initial=true;
long begintime=0;
long now=0;

void uint8ToHexString(char *outString, uint8_t theByte) {
  outString[0]='0'; outString[1]='x';
  uint8_t hinybble=theByte>>4;
  uint8_t lonybble=theByte & 0x0F;
  if (hinybble < 0x0a) outString[2]=hinybble+48;
  else outString[2]=hinybble+55;
  if (lonybble < 0x0a) outString[3]=lonybble+48;
  else outString[3]=lonybble+55;
  outString[4]=0;
}

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
    printBuffer.putString(getPSTR("-F-"));
  break;
  case RISING:
    printBuffer.putString(getPSTR("+R+"));
  break;
  case CHANGE:
    printBuffer.putString(getPSTR("*C*"));
  break;
  }
}

/*
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
  if (start !=1) printBuffer.putString(getPSTR(" no count"));
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
  printBuffer.putString(getPSTR("f1p")); printBuffer.putString(numBuffer); printBuffer.putString((char *) "-P");
  uint8ToString(numBuffer, digitalPinToPort(latest_interrupted_pin));
  printBuffer.putString(numBuffer);
  if (start !=1) printBuffer.putString(getPSTR(" (counting off)"));
  printBuffer.putString((char *) "\n");
};
*/

void quicfunc2() {
  //*led_port|=led_mask;
  //*led_port&=not_led_mask; // 2 micros to here (ie, 2 micros used to push registers and call subroutine)
  latest_interrupted_pin=PCintPort::arduinoPin;
  mode=PCintPort::pinmode;
  showMode();
  *led_port|=led_mask; // 73 micros to get here from above.  Used in "Rigol Timing Example"
  *led_port&=not_led_mask;
  //uint8ToString(numBuffer, PCintPort::s_count); printBuffer.putString(numBuffer);
  *led_port|=led_mask; // 73 micros to get here from above.  Second pulse in "Rigol Timing Example"
  *led_port&=not_led_mask;
  printBuffer.putString(getPSTR(" f2: P"));/*
  uint8ToHexString(numBuffer, *portInputRegister(3)); printBuffer.putString(numBuffer);// C port
  printBuffer.putString(getPSTR(" pin:")); uint8ToString(numBuffer, latest_interrupted_pin); printBuffer.putString(numBuffer);
  printBuffer.putString(getPSTR(" c")); uint8ToHexString(numBuffer, PCintPort::curr); printBuffer.putString(numBuffer);
  printBuffer.putString(getPSTR(" l")); uint8ToHexString(numBuffer, PCintPort::s_lastPinView); printBuffer.putString(numBuffer);
  printBuffer.putString(getPSTR(" r")); uint8ToHexString(numBuffer, PCintPort::s_portRisingPins); printBuffer.putString(numBuffer);
  printBuffer.putString(getPSTR(" f")); uint8ToHexString(numBuffer, PCintPort::s_portFallingPins); printBuffer.putString(numBuffer);
  printBuffer.putString(getPSTR(" m")); uint8ToHexString(numBuffer, PCintPort::s_pmask); printBuffer.putString(numBuffer);
  printBuffer.putString(getPSTR(" P")); printBuffer.put(PCintPort::s_PORT); printBuffer.putString("\r\n");
  printBuffer.putString(getPSTR("cp")); uint8ToHexString(numBuffer, PCintPort::s_changedPins); printBuffer.putString(numBuffer);
  printBuffer.putString(getPSTR(" cXORlpv")); uint8ToHexString(numBuffer, PCintPort::s_currXORlastPinView); printBuffer.putString(numBuffer); 
  printBuffer.putString(getPSTR(" rp_nCurr")); uint8ToHexString(numBuffer, PCintPort::s_portRisingPins_nCurr); printBuffer.putString(numBuffer); 
  printBuffer.putString(getPSTR(" fp_nNCurr")); uint8ToHexString(numBuffer, PCintPort::s_portFallingPins_nNCurr); printBuffer.putString(numBuffer);
  */printBuffer.putString("\r\n");
  if (PCintPort::pcint_multi > 0) {
    printBuffer.putString("MULTI!\n"); PCintPort::pcint_multi=0;
  }
  if (PCintPort::PCIFRbug > 0) { printBuffer.putString("ERROR: BUG- PCIFR should be reset!"); PCintPort::PCIFRbug=0; }
  //s_registers, if it existed, could be used to keep a running queue of the latest interrupts that have
  //been serviced by the PCint(). But generally I don't think it's necessary for debugging at this point (famous last words?)
  /*if (PCintPort::s_count > 2) {
    for (uint8_t i=0; i < PCintPort::s_count; i++) {
      uint8ToHexString(numBuffer, PCintPort::s_registers[i]); printBuffer.putString(numBuffer); printBuffer.putString(" ");
    }
  }
  PCintPort::s_count=0;*/
  /*
  if (start == 1) {
    printBuffer.putString(getPSTR("STOP Count off\n"));
    printBuffer.putString(getPSTR("Intr OFF: (")); uint8ToString(numBuffer, tPIN1), printBuffer.putString(numBuffer);
    printBuffer.putString((char *) " "); uint8ToString(numBuffer, tPIN3), printBuffer.putString(numBuffer);
    printBuffer.putString((char *) " "); uint8ToString(numBuffer, tPIN5), printBuffer.putString(numBuffer);
    printBuffer.putString((char *) ")\n");
    PCintPort::detachInterrupt(tPIN1); PCintPort::detachInterrupt(tPIN3); PCintPort::detachInterrupt(tPIN5); 
    start=0;
  } else {
    start=1;
    interrupt_count[latest_interrupted_pin]++;
    printBuffer.putString(getPSTR("START! p"));
    uint8ToString(numBuffer, latest_interrupted_pin);
    printBuffer.putString(numBuffer); printBuffer.putString((char *) "-P");
    // MIKE put the REAL PORT HERE
    uint8ToString(numBuffer, digitalPinToPort(latest_interrupted_pin));
    printBuffer.putString(numBuffer); printBuffer.putString((char *) "\n");
    if (! initial) {
      PCintPort::attachInterrupt(tPIN1, &quicfunc0, FALLING);
      PCintPort::attachInterrupt(tPIN3, &quicfunc0, CHANGE);
      PCintPort::attachInterrupt(tPIN5, &quicfunc1, CHANGE);
    } else {
      initial=false;
    }                                                                                            
  }*/
};

uint8_t i;
char hexBuffer[5];
void setup() {
  int8_t returncode=1;
  Serial.begin(115200);
  Serial.println("Test");
  delay(500);
  for (i=5; i < 6; i++) {
    pinMode(pins[i], INPUT); digitalWrite(pins[i], HIGH);
    ports[i]=digitalPinToPort(pins[i]);
    switch (pins[i]) {
    /*case tPIN1:
        #if PCINT_VERSION > 2100
        returncode=PCintPort::attachInterrupt(pins[i], &quicfunc0, FALLING);
        #else
        PCintPort::attachInterrupt(pins[i], &quicfunc0, FALLING);
        #endif
        Serial.println(getPSTR("FIRST FAILURE OK."));
    break;
    case tPIN3:
        #if PCINT_VERSION > 2100
        returncode=PCintPort::attachInterrupt(pins[i], &quicfunc0, CHANGE);
        #else
        PCintPort::attachInterrupt(pins[i], &quicfunc0, CHANGE);
        #endif
    break;
    case tPIN2:
    case tPIN4:
        #if PCINT_VERSION > 2100
        returncode=PCintPort::attachInterrupt(pins[i], &quicfunc0, RISING);
        #else
        PCintPort::attachInterrupt(pins[i], &quicfunc0, RISING);
        #endif
    break;
    case tPIN5:
        #if PCINT_VERSION > 2100
        returncode=PCintPort::attachInterrupt(pins[i], &quicfunc1, CHANGE);
        #else
        PCintPort::attachInterrupt(pins[i], &quicfunc1, CHANGE);
        #endif
    break;*/
    case tPIN6:
        #if PCINT_VERSION > 2100
        returncode=PCintPort::attachInterrupt(pins[i], &quicfunc2, FALLING);
        #else
        PCintPort::attachInterrupt(pins[i], &quicfunc2, FALLING);
        #endif
    break;
    }
    #if PCINT_VERSION > 2100
    Serial.print(getPSTR("setup(): Interrupt attach "));
    if (returncode != 1) Serial.print(getPSTR("unsuccessful "));
    else       Serial.print(getPSTR("GOOD "));
    Serial.print(pins[i], DEC);
    Serial.print(getPSTR(":pin, code: ")); Serial.println(returncode, DEC);
    #endif
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
      Serial.println(getPSTR("!Some output lost due to full buffer!"));
    }
    for (i=0; i < 20; i++) {
      if (interrupt_count[i] != 0) {
        count=interrupt_count[i];
        interrupt_count[i]=0;
        Serial.print(getPSTR("Count for pin "));
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

