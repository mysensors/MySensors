#ifndef Config_h
#define Config_h

/***
 * Configure Sensor Network
 */
#define RF24_CHANNEL	   76             //RF channel for the sensor net, 0-127
#define RF24_DATARATE 	   RF24_2MBPS     //RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
#define RF24_PA_LEVEL 	   RF24_PA_MAX    //Senor PA Level == RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBM, and RF24_PA_HIGH=0dBm
#define RF24_PA_LEVEL_GW   RF24_PA_LEVEL  //Gateway PA Level, defaults to Sensor net PA Level.  Tune here if using an amplified nRF2401+ in your gateway.

/***
 * Enable/Disable debug logging
 */
//#define DEBUG

#endif
