#include "MyDriver.h"
#include "MyDriverNRF24.h"

MyDriverNRF24::MyDriverNRF24() : MyDriver() {
	rf24 = new RF24(RF24_CE_PIN, RF24_CS_PIN);
}

void MyDriverNRF24::init() {
	// Start up the radio library
	rf24->begin();

	if (!rf24->isPVariant()) {
		debug(PSTR("check wires\n"));
		while(1);
	}
	rf24->setAutoAck(1);
	rf24->setAutoAck(BROADCAST_PIPE,false); // Turn off auto ack for broadcast
	rf24->setChannel(RF24_CHANNEL);
	rf24->setPALevel(RF24_PA_LEVEL);
	rf24->setDataRate(RF24_DATARATE);
	rf24->setRetries(5,15);
	rf24->setCRCLength(RF24_CRC_16);
	rf24->enableDynamicPayloads();
	rf24->enableDynamicAck();

	// All nodes listen to broadcast pipe (for FIND_PARENT_RESPONSE messages)
	rf24->openReadingPipe(BROADCAST_PIPE, TO_ADDR(BROADCAST_ADDRESS));
}

void MyDriverNRF24::setAddress(uint8_t address) {
	_address = address;
	rf24->openReadingPipe(WRITE_PIPE, TO_ADDR(address));
	rf24->openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(address));
	rf24->startListening();
}

uint8_t MyDriverNRF24::getAddress() {
	return _address;
}

bool MyDriverNRF24::send(uint8_t to, const void* data, uint8_t len) {
	// Make sure radio has powered up
	rf24->powerUp();
	rf24->stopListening();
	rf24->openWritingPipe(TO_ADDR(to));
	bool ok = rf24->write(data, len, to == BROADCAST_ADDRESS);
	rf24->startListening();
	return ok;
}

bool MyDriverNRF24::available(uint8_t *to) {
	uint8_t pipe = 255;
	boolean avail = rf24->available(&pipe); 
	if (pipe == CURRENT_NODE_PIPE)
		*to = _address;
	else if (pipe == BROADCAST_PIPE)
		*to = BROADCAST_ADDRESS;
	return (rf24->available() && pipe < 6);
}

uint8_t MyDriverNRF24::receive(void* data) {
	uint8_t len = rf24->getDynamicPayloadSize();
	rf24->read(data, len);
}

void MyDriverNRF24::powerDown() {
	rf24->powerDown();
}
