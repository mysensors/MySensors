#ifndef MyRFDriver_h
#define MyRFDriver_h

#include <stdint.h>

#ifdef DEBUG
#define debug(x,...) debugPrint(x, ##__VA_ARGS__)
#else
#define debug(x,...)
#endif

#define AUTO 0xFF // 0-254. Id 255 is reserved for auto initialization of nodeId.
#define NODE_SENSOR_ID 0xFF // Node child id is always created for when a node

// This is the nodeId for sensor net gateway receiver sketch (where all sensors should send their data).
#define GATEWAY_ADDRESS ((uint8_t)0)
#define BROADCAST_ADDRESS ((uint8_t)0xFF)

class MyRFDriver
{ 
public:
	// MyRFDriver constructor
	// different parameters would be needed depending on radio module (e.g. pins connected etc.)
	// keeping these parameters as #define's in MyConfig to streamline the driver interface
	MyRFDriver();
	// init()
	// different parameters would be needed depending on radio module (e.g. channel, power, crc, etc.)
	// keeping these parameters as #define's in MyConfig to streamline the driver interface
	virtual void init() = 0;
	// setAddress(address)
	// set the node address and start listening to that address (and the broadcast address)
	virtual void setAddress(uint8_t address) = 0;
	// getAddress()
	// returns the current address from the radio driver
	virtual uint8_t getAddress() = 0;
	// send(to, data, len)
	// reliable transmission of the data with given length (in bytes) to the destination address
	// returns true if successfully submitted
	virtual bool send(uint8_t to, const void* data, uint8_t len) = 0;
	// available(to)
	// returns true if a new packet arrived in the rx buffer
	// populates "to" parameter with the address the packet was sent to (either own address or broadcast)
	virtual bool available(uint8_t *to) = 0;
	// receive(data)
	// returns length of received packet
	// populates "data" parameter with received data
	virtual uint8_t receive(void* data) = 0;
	// powers down the radio
	virtual void powerDown() = 0;
};

#endif
