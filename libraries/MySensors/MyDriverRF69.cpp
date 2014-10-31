#include "MyDriver.h"
#include "MyDriverRF69.h"

MyDriverRF69::MyDriverRF69() : MyDriver() {
	driver = new RH_RF69(RF69_CS_PIN, RF69_INTERRUPT_PIN);
}

void MyDriverRF69::init() {
	// Start up the radio library
	manager = new RHReliableDatagram(*driver, _address);
	driver->setFrequency(RF69_FREQUENCY);
	driver->setTxPower(RF69_TRANSMIT_POWER)
	
}

void MyDriverRF69::setAddress(uint8_t address) {
	_address = address;
	manager->setThisAddress(_address);
}

uint8_t MyDriverRF69::getAddress() {
	return _address;
}

bool MyDriverRF69::send(uint8_t to, const void* data, uint8_t len) {
	// Make sure radio has powered up
	uint8_t status = manager->sendtoWait((uint8_t *) data, len, to);
	if(status==RH_ROUTER_ERROR_NONE) {
		return true;
	} else {
		return false;
	}
}

bool MyDriverRF69::available(uint8_t *to) {
	return manager->available();
}

uint8_t MyDriverRF69::receive(void* data) {
	uint8_t len = 256;
	uint8_t from;
	manager->recvfromAck((uint8_t *) data, &len, &from);
}

void MyDriverRF69::powerDown() {
	driver->sleep();
}
