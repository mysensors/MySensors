#ifndef MyConfig_h
#define MyConfig_h

/***
 * Configure Sensor Network
 */
 
// pick ONE of these - other board layouts can be easily added this way
#define MYSENSORS_SENSOR
//#define MYSENSORS_SERIAL_GATEWAY
//#define MYSENSORS_ETHERNET_GATEWAY
//#define MYSENSORS_MQTT_GATEWAY

// Choose radio type by enabling one of the following
#define MYSENSORS_RF_NRF24
//#define MYSENSORS_RF_RF69

// Choose signing backend by enabling one of the following
#define MYSENSORS_SIGNING_DUMMY

// Define a suitable timeout for a signature verification session
// Consider the turnaround from a nonce being generated to a signed message being received
// which might vary, especially in networks with many hops. 5s ought to be enough for anyone.
#define VERIFICATION_TIMEOUT_MS 5000

#ifdef MYSENSORS_RF_NRF24
#include "MyRFDriverNRF24.h"
typedef class MyRFDriverNRF24 MyRFDriverClass;
#endif

#ifdef MYSENSORS_RF_RF69
#include "MyRFDriverRF69.h"
typedef class MyRFDriverRF69 MyRFDriverClass;
#endif

#ifdef MYSENSORS_SIGNING_DUMMY
#include "MySigningDriverDummy.h"
#define SIGNING_IDENTIFIER (0) // Reserved for dummy implementation, will not work with other backends
typedef class MySigningDriverDummy MySigningDriverClass;
#endif

/***
 * Enable/Disable debug logging
 */
#define DEBUG

#endif
