#ifndef MyConfig_h
#define MyConfig_h

/***
 * Configure Sensor Network
 */
 
// pick ONE of these - other board layouts can be easily added this way
#define MYSENSORS_SENSOR
//#define MYSENSORS_SERIAL_GATEWAY
//#define MYSENSORS_ETHERNET_MQTT_GATEWAY

// Choose radio type by enabling one of the following
#define MYSENSORS_RF_NRF24



#ifdef MYSENSORS_RF_NRF24
#include "MyDriverNRF24.h"
typedef class MyDriverNRF24 MyDriverClass;
#endif

/***
 * Enable/Disable debug logging
 */
#define DEBUG

#endif
