
// Please read Bounce.h for information about the liscence and authors

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Bounce2.h"



Bounce::Bounce() {
	this->interval_millis = 10;
	
}

void Bounce::attach(int pin) {
 this->pin = pin;
 debouncedState = unstableState = digitalRead(pin);
 #ifdef BOUNCE_LOCK_OUT
 previous_millis = 0;
 #else
 previous_millis = millis();
 #endif
}



void Bounce::interval(unsigned long interval_millis)
{
  this->interval_millis = interval_millis;
  
}


bool Bounce::update()
{

#ifdef BOUNCE_LOCK_OUT
    stateChanged = false;
	// Ignore everything if we are locked out
	if (millis() - previous_millis >= interval_millis) {
		uint8_t currentState = digitalRead(pin);
		if (debouncedState != currentState ) {
			previous_millis = millis();
			debouncedState = currentState;
			stateChanged = true;
		}
	}
	return stateChanged;

#else
	// Lire l'etat de l'interrupteur dans une variable temporaire.
	uint8_t currentState = digitalRead(pin);
	stateChanged = false;

	// Redemarrer le compteur timeStamp tant et aussi longtemps que
	// la lecture ne se stabilise pas.
	if ( currentState != unstableState ) {
			previous_millis = millis();
	} else 	if ( millis() - previous_millis >= interval_millis ) {
				// Rendu ici, la lecture est stable

				// Est-ce que la lecture est diffÃ©rente de l'etat emmagasine de l'interrupteur?
				if ( currentState != debouncedState ) {
						debouncedState = currentState;
						stateChanged = true;
						
				}

	}
	 
	unstableState = currentState;
	return stateChanged;
#endif

}

uint8_t Bounce::read()
{
	return debouncedState;
}

