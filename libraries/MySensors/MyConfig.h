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
//#define MYSENSORS_RF_RF69





#ifdef MYSENSORS_RF_NRF24
#include "MyRFDriverNRF24.h"
typedef class MyRFDriverNRF24 MyRFDriverClass;
#endif

#ifdef MYSENSORS_RF_RF69
#include "MyRFDriverRF69.h"
typedef class MyRFDriverRF69 MyRFDriverClass;
#endif

/***
 * Enable/Disable debug logging
 */
#define DEBUG

#endif
