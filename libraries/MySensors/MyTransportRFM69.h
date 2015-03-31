#ifndef MyTransportRF69_h
#define MyTransportRF69_h

#include "MyConfig.h"
#include "MyTransport.h"
#include <stdint.h>
#include "utility/RFM69.h"
#include <SPI.h>


class MyTransportRFM69 : public MyTransport
{ 
public:
	MyTransportRFM69(uint8_t freqBand=RFM69_FREQUENCY, uint8_t networkId=RFM69_NETWORKID, uint8_t slaveSelectPin=RF69_SPI_CS, uint8_t interruptPin=RF69_IRQ_PIN, bool isRFM69HW=false, uint8_t interruptNum=RF69_IRQ_NUM);
	void init();
	void setAddress(uint8_t address);
	uint8_t getAddress();
	bool send(uint8_t to, const void* data, uint8_t len);
	bool available(uint8_t *to);
	uint8_t receive(void* data);
	void powerDown();
private:
	RFM69 radio;
	uint8_t _address;
	uint8_t _freqBand;
	uint8_t _networkId;
};

#endif
