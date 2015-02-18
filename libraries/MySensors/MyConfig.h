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
//#define MYSENSORS_SIGNING_ATSHA204
//#define MYSENSORS_SIGNING_ATSHA204SOFT

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

#ifdef MYSENSORS_SIGNING_ATSHA204
#ifdef MYSENSORS_SENSOR
#define ATSHA204_PIN 17 //A3
#endif

#ifdef MYSENSORS_SERIAL_GATEWAY
#define ATSHA204_PIN 17 //A3
#endif

#if defined(MYSENSORS_ETHERNET_GATEWAY) || defined(MYSENSORS_MQTT_GATEWAY)
#define ATSHA204_PIN 17 //A3
#endif

#include "MySigningDriverAtsha204.h"
#define SIGNING_IDENTIFIER (1) // SHA256-based HMAC (ATSHA204 specific)
typedef class MySigningDriverAtsha204 MySigningDriverClass;
#endif

#ifdef MYSENSORS_SIGNING_ATSHA204SOFT
// Key to use for HMAC calculation
static uint8_t hmacKey[32] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// Pick an unconnected analog pin for RANDOMSEED_PIN
#ifdef MYSENSORS_SENSOR
#define RANDOMSEED_PIN 7 //A7
#endif

#ifdef MYSENSORS_SERIAL_GATEWAY
#define RANDOMSEED_PIN 7 //A7
#endif

#if defined(MYSENSORS_ETHERNET_GATEWAY) || defined(MYSENSORS_MQTT_GATEWAY)
#define RANDOMSEED_PIN 7 //A7
#endif

#include "MySigningDriverAtsha204Soft.h"
#define SIGNING_IDENTIFIER (1) // SHA256-based HMAC (ATSHA204 specific)
typedef class MySigningDriverAtsha204Soft MySigningDriverClass;
#endif

/***
 * Enable/Disable debug logging
 */
#define DEBUG

#endif
