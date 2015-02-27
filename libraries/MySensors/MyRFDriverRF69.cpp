#include "MyRFDriver.h"
#include "MyRFDriverRF69.h"

MyRFDriverRF69::MyRFDriverRF69()
	:
	MyRFDriver(),
	radio()
{
}

void MyRFDriverRF69::init() {
	// Start up the radio library
	radio->initialize(FREQUENCY,_address,NETWORKID);
#ifdef IS_RFM69HW
    radio->setHighPower(); //uncomment only for RFM69HW!
#endif
    // radio->encrypt(ENCRYPTKEY);
	
}

void MyRFDriverRF69::setAddress(uint8_t address) {
	_address = address;
	radio->setAddress(address);
}

uint8_t MyRFDriverRF69::getAddress() {
	return _address;
}

bool MyRFDriverRF69::send(uint8_t to, const void* data, uint8_t len) {
	// Make sure radio has powered up
	return radio->sendWithRetry(to,data,len);
}

bool MyRFDriverRF69::available(uint8_t *to) {
	if (radio->TARGETID == BROADCAST_ADDRESS)
		*to = BROADCAST_ADDRESS;
	else
		*to = _address;
	return radio->receiveDone();
}

uint8_t MyRFDriverRF69::receive(void* data) {
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

void MyRFDriverRF69::powerDown() {
	radio->sleep();
}
