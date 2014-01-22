
/*
 Copyright (C) 2011 James Coliz, Jr. <maniacbug@ymail.com>
 Copyright (c) 2012 Greg Copeland

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Channel scanner
 *
 * Example to detect interference on the various channels available.
 * This is a good diagnostic tool to check whether you're picking a
 * good channel for your application.
 *
 * Inspired by cpixip.
 * See http://arduino.cc/forum/index.php/topic,54795.0.html
 */

#include <SPI.h>
#include "RF24.h"
#include "printf.h"

// Only display active frequencies
static const bool activeOnly = true ;

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 8 & 9

RF24 radio(9,10);

//
// Channel info
//

const short num_channels = 128;
short values[num_channels];
uint8_t signalMeter[55] ;

//
// Setup
//

void setup(void)
{
  //
  // Print preamble
  //

  Serial.begin(115200);
  printf_begin();
  printf("\n\rRF24/examples/scanner/\n\r");

  //
  // Setup and configure rf radio
  //

  radio.begin();
  radio.powerUp() ;
  radio.setAutoAck(false);

  // Get into standby mode
  radio.openReadingPipe( 0, 0xFFFFFFFFFFULL ) ;
  //  radio.setDataRate( RF24_250KBPS ) ; // may fallback to 1Mbps
  radio.setDataRate( RF24_1MBPS ) ; // may fallback to 1Mbps
  radio.startListening() ;
  radio.stopListening() ;
}

//
// Loop
//
void loop(void)
{
  // Clear measurement values
  memset( values, 0x00, num_channels ) ;
  printf( "Scanning all available frequencies..." ) ;

  // Repeatedly scan multiple channels
  for( int channel=0 ; channel < num_channels; channel++ ) {
    radio.setChannel( channel ) ;

    // Amplify the signal based on carrier bandwidth
    int ampFactor ;
    for( int amp=0; amp <= 300; amp++ ) {
      // Alternate data rates
      ampFactor = amp%3 ;
      switch( ampFactor ) {
      case 0:
	radio.setDataRate( RF24_250KBPS ) ;
	break ;

      case 1:
	radio.setDataRate( RF24_1MBPS ) ;
	break ;

      default:
	radio.setDataRate( RF24_2MBPS ) ;
	break ;
      }

      // Listen for carrier
      ampFactor++ ;
      radio.startListening() ;
      delayMicroseconds( 6 - ampFactor ) ;
      radio.stopListening() ;

      // Was carrier detected? If so, signal level based on bandwidth
      if( radio.testRPD() ) {
	values[channel] += ampFactor ;
      }
    }
  }

  // Now display our results
  printf( "Scan completed.\r\n" ) ;
  for( int channel=0 ; channel < num_channels; channel++ ) {
    if( !activeOnly || (activeOnly && values[channel] > 0) ) {
      memset( signalMeter, '*', min( values[channel], 54 ) ) ;
      signalMeter[min(values[channel], 54)] = 0x00 ;
      printf( "%03d (%4dMhz): %02d - %s\r\n",
	      channel,
	      2400+channel,
	      values[channel],
	      signalMeter ) ;

      // Reset the scanned value since its already beend displayed
      values[channel] = 0 ;
    }
  }
}

// vim:ai:cin:sts=2 sw=2 ft=cpp
