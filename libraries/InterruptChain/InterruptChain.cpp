/*
 * IntereruptChain library v1.3.0 (20130601)
 *
 * Copyright 2011-2013 by Randy Simons http://randysimons.nl/
 *
 * License: GPLv3. See license.txt
 */
 
#include <InterruptChain.h>
     
void InterruptChainLink::init(InterruptCallback callbackIn, InterruptChainLink *nextIn) {
	callback = callbackIn;
	next = nextIn;
}

InterruptChainLink *InterruptChain::chain[MAX_INTERRUPTS] = {NULL};
byte InterruptChain::mode[MAX_INTERRUPTS] = {CHANGE};

void InterruptChain::setMode(byte interruptNr, byte modeIn) {     
    mode[interruptNr] = modeIn;
}



void InterruptChain::addInterruptCallback(byte interruptNr, InterruptCallback callback) {
  InterruptChainLink *prevLink = chain[interruptNr]; // Note: the chain-array is NULL initialized, so the first time prevLink is indeed NULL
  
  chain[interruptNr] = (InterruptChainLink *) malloc(sizeof(InterruptChainLink)); // malloc instead of new, due to the lack of new / delete support in AVR-libc
  chain[interruptNr]->init(callback, prevLink);      

  enable(interruptNr); 
}

void InterruptChain::enable(byte interruptNr) {
 switch (interruptNr) {
    case 0:
          attachInterrupt(0, InterruptChain::processInterrupt0, mode[0]);
          break;
    case 1:
          attachInterrupt(1, InterruptChain::processInterrupt1, mode[1]);
          break;
    case 2:
          attachInterrupt(2, InterruptChain::processInterrupt2, mode[2]);
          break;
    case 3:
          attachInterrupt(3, InterruptChain::processInterrupt3, mode[3]);
          break;
    case 4:
          attachInterrupt(4, InterruptChain::processInterrupt4, mode[4]);
          break;
    case 5:
          attachInterrupt(5, InterruptChain::processInterrupt5, mode[5]);
          break;
  }
}

void InterruptChain::disable(byte interruptNr) {
	detachInterrupt(interruptNr);
}

void InterruptChain::processInterrupt0() {
	InterruptChainLink *current = chain[0];
	while(current) {
		(current->callback)();
		current = current->next;
	}
}

void InterruptChain::processInterrupt1() {     
    InterruptChainLink *current = chain[1];
	while(current) {
		(current->callback)();
		current = current->next;
	}
}

void InterruptChain::processInterrupt2() {     
    InterruptChainLink *current = chain[2];
	while(current) {
		(current->callback)();
		current = current->next;
	}
}

void InterruptChain::processInterrupt3() {     
    InterruptChainLink *current = chain[3];
	while(current) {
		(current->callback)();
		current = current->next;
	}
}

void InterruptChain::processInterrupt4() {     
    InterruptChainLink *current = chain[4];
	while(current) {
		(current->callback)();
		current = current->next;
	}
}

void InterruptChain::processInterrupt5() {     
    InterruptChainLink *current = chain[5];
	while(current) {
		(current->callback)();
		current = current->next;
	}
}
