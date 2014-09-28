#ifndef MyCommDriver_h
#define MyCommDriver_h

#include "MyConfig.h"
#include "MyDriver.h"
#include <stdint.h>
#include "utility/RF24.h"
#include "utility/RF24_config.h"

#define TO_ADDR(x) (RF24_BASE_RADIO_ID + x)

#define WRITE_PIPE ((uint8_t)0)
#define CURRENT_NODE_PIPE ((uint8_t)1)
#define BROADCAST_PIPE ((uint8_t)2)

class MyDriverNRF24 : public MyDriver
{ 
public:
	MyDriverNRF24();
	void init();
	void setAddress(uint8_t address);
	uint8_t getAddress();
	bool send(uint8_t to, const void* data, uint8_t len);
	bool available(uint8_t *to);
	uint8_t receive(void* data);
	void powerDown();
private:
	RF24 *rf24;
	uint8_t _address;
};

#endif