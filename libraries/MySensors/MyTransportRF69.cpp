#include "MyTransport.h"
#include "MyTransportRF69.h"

MyTransportRF69::MyTransportRF69()
	:
	MyTransport(),
	radio()
{
}

void MyTransportRF69::init() {
	// Start up the radio library
	radio->initialize(FREQUENCY,_address,NETWORKID);
#ifdef IS_RFM69HW
    radio->setHighPower(); //uncomment only for RFM69HW!
#endif
    // radio->encrypt(ENCRYPTKEY);
	
}

void MyTransportRF69::setAddress(uint8_t address) {
	_address = address;
	radio->setAddress(address);
}

uint8_t MyTransportRF69::getAddress() {
	return _address;
}

bool MyTransportRF69::send(uint8_t to, const void* data, uint8_t len) {
	// Make sure radio has powered up
	return radio->sendWithRetry(to,data,len);
}

bool MyTransportRF69::available(uint8_t *to) {
	if (radio->TARGETID == BROADCAST_ADDRESS)
		*to = BROADCAST_ADDRESS;
	else
		*to = _address;
	return radio->receiveDone();
}

uint8_t MyTransportRF69::receive(void* data) {
	// for (byte i = 0; i < radio->DATALEN; i++){
		// data[i]= (void)radio->DATA[i];
	// }
	memcpy(data,(const void *)radio->DATA, radio->DATALEN);
	if (radio->ACKRequested())
    {
      radio->sendACK();
    }
	return radio->DATALEN;
}	

void MyTransportRF69::powerDown() {
	radio->sleep();
}
