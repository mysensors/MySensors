#include "PinChangeInt.h"


#ifndef LIBCALL_PINCHANGEINT // LIBCALL_PINCHANGEINT ***********************************************
volatile uint8_t PCintPort::curr=0;
#ifndef NO_PIN_NUMBER
volatile uint8_t PCintPort::arduinoPin=0;
#endif
#ifndef NO_PIN_STATE
volatile uint8_t PCintPort::pinState=0;
#endif
#ifdef PINMODE
volatile uint8_t PCintPort::pinmode=0;
volatile uint8_t PCintPort::s_portRisingPins=0;
volatile uint8_t PCintPort::s_portFallingPins=0;
volatile uint8_t PCintPort::s_lastPinView=0;
volatile uint8_t PCintPort::s_pmask=0;
volatile char	 PCintPort::s_PORT='x';
volatile uint8_t PCintPort::s_changedPins=0;
volatile uint8_t PCintPort::s_portRisingPins_nCurr=0;
volatile uint8_t PCintPort::s_portFallingPins_nNCurr=0;
volatile uint8_t PCintPort::s_currXORlastPinView=0;
volatile uint8_t PCintPort::s_count=0;
volatile uint8_t PCintPort::pcint_multi=0;
volatile uint8_t PCintPort::PCIFRbug=0;
#endif

#ifdef FLASH
#define PINLED 13
volatile uint8_t *led_port;
uint8_t led_mask;
uint8_t not_led_mask;
boolean ledsetup_run=false;
void PCintPort::ledsetup(void) {
	if (! ledsetup_run) {
		led_port=portOutputRegister(digitalPinToPort(PINLED));
		led_mask=digitalPinToBitMask(PINLED);
		not_led_mask=led_mask^0xFF;
		pinMode(PINLED, OUTPUT); digitalWrite(PINLED, LOW);
		ledsetup_run=true;
	}
};
#endif


// ATMEGA 644 
//
#if defined(__AVR_ATmega644P__) || defined(__AVR_ATmega644__) // Sanguino, Mosquino uino bobino bonanafannafofino, me my momino...

#ifndef NO_PORTA_PINCHANGES
PCintPort portA=PCintPort(1, 0,PCMSK0); // port PB==2  (from Arduino.h, Arduino version 1.0)
#endif
#ifndef NO_PORTB_PINCHANGES
PCintPort portB=PCintPort(2, 1,PCMSK1); // port PB==2  (from Arduino.h, Arduino version 1.0)
#endif
#ifndef NO_PORTC_PINCHANGES
PCintPort portC=PCintPort(3, 2,PCMSK2); // port PC==3  (also in pins_arduino.c, Arduino version 022)
#endif
#ifndef NO_PORTD_PINCHANGES
PCintPort portD=PCintPort(4, 3,PCMSK3); // port PD==4
#endif

#else // others

#ifndef NO_PORTB_PINCHANGES
PCintPort portB=PCintPort(2, 0,PCMSK0); // port PB==2  (from Arduino.h, Arduino version 1.0)
#endif
#ifndef NO_PORTC_PINCHANGES  // note: no PORTC on MEGA
PCintPort portC=PCintPort(3, 1,PCMSK1); // port PC==3  (also in pins_arduino.c, Arduino version 022)
#endif
#ifndef NO_PORTD_PINCHANGES  // note: no PORTD on MEGA
PCintPort portD=PCintPort(4, 2,PCMSK2); // port PD==4
#endif

#endif // defined __AVR_ATmega644__

#ifdef __USE_PORT_JK
#ifndef NO_PORTJ_PINCHANGES
PCintPort portJ=PCintPort(10,1,PCMSK1); // port PJ==10 
#endif
#ifndef NO_PORTK_PINCHANGES
PCintPort portK=PCintPort(11,2,PCMSK2); // port PK==11
#endif
#endif // USE_PORT_JK

static PCintPort *lookupPortNumToPort( int portNum ) {
    PCintPort *port = NULL;

        switch (portNum) {
#ifndef NO_PORTA_PINCHANGES
        case 1:
                port=&portA;
                break;
#endif
#ifndef NO_PORTB_PINCHANGES
        case 2:
                port=&portB;
                break;
#endif
#ifndef NO_PORTC_PINCHANGES
        case 3:
                port=&portC;
                break;
#endif
#ifndef NO_PORTD_PINCHANGES
        case 4:
                port=&portD;
                break;
#endif
#ifdef __USE_PORT_JK

#ifndef NO_PORTJ_PINCHANGES
        case 10:
                port=&portJ;
                break;
#endif

#ifndef NO_PORTK_PINCHANGES
        case 11:
                port=&portK;
                break;
#endif

#endif
    }

    return port;
}


void PCintPort::enable(PCintPin* p, PCIntvoidFuncPtr userFunc, uint8_t mode) {
	// Enable the pin for interrupts by adding to the PCMSKx register.
	// ...The final steps; at this point the interrupt is enabled on this pin.
	p->mode=mode;
	p->PCintFunc=userFunc;
	portPCMask |= p->mask;
	if ((p->mode == RISING) || (p->mode == CHANGE)) portRisingPins |= p->mask;
	if ((p->mode == FALLING) || (p->mode == CHANGE)) portFallingPins |= p->mask;
	PCICR |= PCICRbit;
}

int8_t PCintPort::addPin(uint8_t arduinoPin, PCIntvoidFuncPtr userFunc, uint8_t mode)
{
	PCintPin* tmp;

	// Add to linked list, starting with firstPin. If pin already exists, just enable.
	if (firstPin != NULL) {
		tmp=firstPin;
		do {
			if (tmp->arduinoPin == arduinoPin) { enable(tmp, userFunc, mode); return(0); }
			if (tmp->next == NULL) break;
			tmp=tmp->next;
		} while (true);
	}

	// Create pin p:  fill in the data.
	PCintPin* p=new PCintPin;
	if (p == NULL) return(-1);
	p->arduinoPin=arduinoPin;
	p->mode = mode;
	p->next=NULL;
	p->mask = digitalPinToBitMask(arduinoPin); // the mask

	if (firstPin == NULL) firstPin=p;
	else tmp->next=p;

#ifdef DEBUG
	Serial.print("addPin. pin given: "); Serial.print(arduinoPin, DEC);
	int addr = (int) p;
	Serial.print(" instance addr: "); Serial.println(addr, HEX);
	Serial.print("userFunc addr: "); Serial.println((int)p->PCintFunc, HEX);
#endif

	enable(p, userFunc, mode);
#ifdef DEBUG
	Serial.print("addPin. pin given: "); Serial.print(arduinoPin, DEC), Serial.print (" pin stored: ");
	int addr = (int) p;
	Serial.print(" instance addr: "); Serial.println(addr, HEX);
#endif
	return(1);
}

/*
 * attach an interrupt to a specific pin using pin change interrupts.
 */
int8_t PCintPort::attachInterrupt(uint8_t arduinoPin, PCIntvoidFuncPtr userFunc, int mode)
{
	PCintPort *port;
	uint8_t portNum = digitalPinToPort(arduinoPin);
	if ((portNum == NOT_A_PORT) || (userFunc == NULL)) return(-1);

	port=lookupPortNumToPort(portNum);
	// Added by GreyGnome... must set the initial value of lastPinView for it to be correct on the 1st interrupt.
	// ...but even then, how do you define "correct"?  Ultimately, the user must specify (not provisioned for yet).
	port->lastPinView=port->portInputReg;

#ifdef DEBUG
	Serial.print("attachInterrupt FUNC: "); Serial.println(arduinoPin, DEC);
#endif
	// map pin to PCIR register
	return(port->addPin(arduinoPin,userFunc,mode));
}

void PCintPort::detachInterrupt(uint8_t arduinoPin)
{
	PCintPort *port;
	PCintPin* current;
	uint8_t mask;
#ifdef DEBUG
	Serial.print("detachInterrupt: "); Serial.println(arduinoPin, DEC);
#endif
	uint8_t portNum = digitalPinToPort(arduinoPin);
	if (portNum == NOT_A_PORT) return;
	port=lookupPortNumToPort(portNum);
	mask=digitalPinToBitMask(arduinoPin);
	current=port->firstPin;
	//PCintPin* prev=NULL;
	while (current) {
		if (current->mask == mask) { // found the target
			uint8_t oldSREG = SREG;
			cli(); // disable interrupts
			port->portPCMask &= ~mask; // disable the mask entry.
			if (port->portPCMask == 0) PCICR &= ~(port->PCICRbit);
			port->portRisingPins &= ~current->mask; port->portFallingPins &= ~current->mask;
			// Link the previous' next to the found next. Then remove the found.
			//if (prev != NULL) prev->next=current->next; // linked list skips over current.
			//else firstPin=current->next; // at the first pin; save the new first pin
			SREG = oldSREG; // Restore register; reenables interrupts
			return;
		}
		//prev=current;
		current=current->next;
	}
}

// common code for isr handler. "port" is the PCINT number.
// there isn't really a good way to back-map ports and masks to pins.
void PCintPort::PCint() {
	uint8_t thisChangedPin; //MIKE

	#ifdef FLASH
	if (*led_port & led_mask) *led_port&=not_led_mask;
	else *led_port|=led_mask;
    #endif
	#ifndef DISABLE_PCINT_MULTI_SERVICE
	uint8_t pcifr;
	while (true) {
	#endif
		// get the pin states for the indicated port.
		#ifdef PINMODE
		PCintPort::s_lastPinView=lastPinView;
		intrCount++;
		PCintPort::s_count=intrCount;
		#endif
		// OLD v. 2.01 technique: Test 1: 3163; Test 7: 3993
		// From robtillaart online: ------------ (starting v. 2.11beta)
		// uint8_t changedPins = PCintPort::curr ^ lastPinView;
		// lastPinView = PCintPort::curr;
		// uint8_t fastMask = changedPins & ((portRisingPins & PCintPort::curr ) | ( portFallingPins & ~PCintPort::curr ));
		// NEW v. 2.11 technique: Test 1: 3270 Test 7: 3987
		// -------------------------------------
		// was: uint8_t changedPins = PCintPort::curr ^ lastPinView;
		// makes test 6 of the PinChangeIntSpeedTest go from 3867 to 3923.  Not good.
		uint8_t changedPins = (PCintPort::curr ^ lastPinView) &
							  ((portRisingPins & PCintPort::curr ) | ( portFallingPins & ~PCintPort::curr ));

		#ifdef PINMODE
		PCintPort::s_currXORlastPinView=PCintPort::curr ^ lastPinView;
		PCintPort::s_portRisingPins_nCurr=portRisingPins & PCintPort::curr;
		PCintPort::s_portFallingPins_nNCurr=portFallingPins & ~PCintPort::curr;
		#endif
		lastPinView = PCintPort::curr;

		PCintPin* p = firstPin;
		while (p) {
			// Trigger interrupt if the bit is high and it's set to trigger on mode RISING or CHANGE
			// Trigger interrupt if the bit is low and it's set to trigger on mode FALLING or CHANGE
			thisChangedPin=p->mask & changedPins; // PinChangeIntSpeedTest makes this 3673... weird.  But GOOD!!!
			if (p->mask & changedPins) {
				#ifndef NO_PIN_STATE
				PCintPort::pinState=PCintPort::curr & p->mask ? HIGH : LOW;
				#endif
				#ifndef NO_PIN_NUMBER
				PCintPort::arduinoPin=p->arduinoPin;
				#endif
				#ifdef PINMODE
				PCintPort::pinmode=p->mode;
				PCintPort::s_portRisingPins=portRisingPins;
				PCintPort::s_portFallingPins=portFallingPins;
				PCintPort::s_pmask=p->mask;
				PCintPort::s_changedPins=changedPins;
				#endif
				p->PCintFunc();
			}
			p=p->next;
		}
	#ifndef DISABLE_PCINT_MULTI_SERVICE
		pcifr = PCIFR & PCICRbit;
		if (pcifr == 0) break;
		PCIFR |= PCICRbit;
		#ifdef PINMODE
		PCintPort::pcint_multi++;
		if (PCIFR & PCICRbit) PCintPort::PCIFRbug=1; // PCIFR & PCICRbit should ALWAYS be 0 here!
		#endif
		PCintPort::curr=portInputReg;
	}
	#endif
}

#ifndef NO_PORTA_PINCHANGES
ISR(PCINT0_vect) {
	#ifdef PINMODE
	PCintPort::s_PORT='A';
	#endif
	PCintPort::curr = portA.portInputReg;
	portA.PCint();
}
#define PORTBVECT PCINT1_vect
#define PORTCVECT PCINT2_vect
#define PORTDVECT PCINT3_vect
#else
#define PORTBVECT PCINT0_vect
#define PORTCVECT PCINT1_vect
#define PORTDVECT PCINT2_vect
#endif

#ifndef NO_PORTB_PINCHANGES
ISR(PORTBVECT) {
	#ifdef PINMODE
	PCintPort::s_PORT='B';
	#endif
	PCintPort::curr = portB.portInputReg;
	portB.PCint();
}
#endif

#ifndef NO_PORTC_PINCHANGES
ISR(PORTCVECT) {
	#ifdef PINMODE
	PCintPort::s_PORT='C';
	#endif
	PCintPort::curr = portC.portInputReg;
	portC.PCint();
}
#endif

#ifndef NO_PORTD_PINCHANGES
ISR(PORTDVECT){ 
	#ifdef PINMODE
	PCintPort::s_PORT='D';
	#endif
	PCintPort::curr = portD.portInputReg;
	portD.PCint();
}
#endif

#ifdef __USE_PORT_JK
#ifndef NO_PORTJ_PINCHANGES
ISR(PCINT1_vect) {
	#ifdef PINMODE
	PCintPort::s_PORT='J';
	#endif
	PCintPort::curr = portJ.portInputReg;
	portJ.PCint();
}
#endif

#ifndef NO_PORTK_PINCHANGES
ISR(PCINT2_vect){ 
	#ifdef PINMODE
	PCintPort::s_PORT='K';
	#endif
	PCintPort::curr = portK.portInputReg;
	portK.PCint();
}
#endif

#endif // __USE_PORT_JK

#ifdef GET_PCINT_VERSION
uint16_t getPCIntVersion () {
	return ((uint16_t) PCINT_VERSION);
}
#endif // GET_PCINT_VERSION
#endif // #ifndef LIBCALL_PINCHANGEINT *************************************************************
