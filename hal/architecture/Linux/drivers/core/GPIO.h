/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef GPIO_h
#define	GPIO_h

#include <pthread.h>
#include <stdint.h>

#define INPUT 0
#define OUTPUT 1

#define LOW 0
#define HIGH 1

#define MAX_PIN 50

/**
 * @brief GPIO class
 */
class GPIOClass
{

public:
	/**
	 * @brief GPIOClass constructor.
	 */
	GPIOClass();

	/**
	 * @brief GPIOClass destructor.
	 */
	~GPIOClass();
	/**
	 * @brief Configures the specified pin to behave either as an input or an output.
	 *
	 * @param pin The number of the pin.
	 * @param mode INPUT or OUTPUT.
	 */
	void pinMode(uint8_t pin, uint8_t mode);
	/**
	 * @brief Write a high or a low value for the given pin.
	 *
	 * @param pin number.
	 * @param value HIGH or LOW.
	 */
	void digitalWrite(uint8_t pin, uint8_t value);
	/**
	 * @brief Reads the value from a specified pin.
	 *
	 * @param pin The number of the pin.
	 * @return HIGH or LOW.
	 */
	uint8_t digitalRead(uint8_t pin);
	/**
	 * @brief Arduino compatibility function, returns the same given pin.
	 *
	 * @param pin The number of the pin.
	 * @return The same parameter pin number.
	 */
	uint8_t digitalPinToInterrupt(uint8_t pin);

	void attachInterrupt(uint8_t pin, void (*func)(), uint8_t mode);
	void detachInterrupt(uint8_t pin);
	void interrupts();
	void noInterrupts();

private:
	// One request per pin.
	// We could use one request for all pins, but then we would have to know which pins we need all at once.
	// Alternatively, we could release and re-request every time a new pin gets used.
	// However, then we would have to keep track of which pins have been used with their settings.
	// One request per pin is just easier.
	struct gpiod_line_request *requests[MAX_PIN] = {nullptr};
	struct gpiod_chip *chip = nullptr;

	// attachInterrupt stuff
	pthread_mutex_t intMutex = PTHREAD_MUTEX_INITIALIZER;
	volatile bool interruptsEnabled = true;
	pthread_t *threadIds[MAX_PIN] = {NULL};

	struct ThreadArgs {
		void (*func)();
		unsigned int pin;
		struct gpiod_line_request *gpiod_line_request;
	};

	static int _piHiPri(const int pri);
	static void _interruptHandlerDetachInterrupt(void *arg);
	static void _interruptHandlerEventBufferRelease(void *arg);
	static void *_interruptHandler(void *args);
};

extern GPIOClass GPIO;

#endif
