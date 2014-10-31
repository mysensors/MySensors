#ifndef MyCommDriver_h
#define MyCommDriver_h

#include "MyConfig.h"
#include "MyDriver.h"
#include <stdint.h>
#include <RH_RF69.h>
#include <RHReliableDatagram.h>

#define RF69_FREQUENCY	868;
#define RF69_TRANSMIT_POWER	14;
#define RF69_MODEM_CONFIG   RH_RF69::GFSK_Rb250Fd250;
#define RF69_INTERRUPT_PIN	2;
#define RF69_CS_PIN	10;

class MyDriverRF69 : public MyDriver
{ 
public:
	MyDriverRF69();
	void init();
	void setAddress(uint8_t address);
	uint8_t getAddress();
	bool send(uint8_t to, const void* data, uint8_t len);
	bool available(uint8_t *to);
	uint8_t receive(void* data);
	void powerDown();
private:
	RH_RF69 *driver = NULL;
	RHReliableDatagram *manager = NULL;
	uint8_t _address;
};

#endif