#ifndef MyConfig_h
#define MyConfig_h

/***
 * Configure Sensor Network
 */
 
// pick ONE of these - other board layouts can be easily added this way
#define MYSENSORS_SENSOR
//#define MYSENSORS_SERIAL_GATEWAY
//#define MYSENSORS_ETHERNET_MQTT_GATEWAY
 
#ifdef MYSENSORS_SENSOR
#define MYSENSORS_RF_NRF24
#define RF24_CE_PIN		   9
#define RF24_CS_PIN		   10
#define RF24_PA_LEVEL 	   RF24_PA_MAX
#endif

#ifdef MYSENSORS_SERIAL_GATEWAY
#define MYSENSORS_RF_NRF24
#define RF24_CE_PIN		   9
#define RF24_CS_PIN		   10
#define RF24_PA_LEVEL 	   RF24_PA_LOW
#endif

#ifdef MYSENSORS_ETHERNET_MQTT_GATEWAY
#define MYSENSORS_RF_NRF24
#define RF24_CE_PIN		   5
#define RF24_CS_PIN		   6
#define RF24_PA_LEVEL 	   RF24_PA_LOW
#endif

#ifdef MYSENSORS_RF_NRF24
#include "MyDriverNRF24.h"
typedef class MyDriverNRF24 MyDriverClass;
#define RF24_CHANNEL	   76             //RF channel for the sensor net, 0-127
#define RF24_DATARATE 	   RF24_250KBPS   //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
#define RF24_BASE_RADIO_ID ((uint64_t)0xA8A8E1FC00LL) // This is also act as base value for sensor nodeId addresses. Change this (or channel) if you have more than one sensor network.
#endif

/***
 * Enable/Disable debug logging
 */
#define DEBUG

#endif
