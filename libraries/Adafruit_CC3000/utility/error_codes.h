/*****************************************************************************
*
*  error_codes.h  - CC3000 Host Driver Implementation.
*  Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/
*
* Adapted for use with the Arduino/AVR by KTOWN (Kevin Townsend) 
* & Limor Fried for Adafruit Industries
* This library works with the Adafruit CC3000 breakout 
*	----> https://www.adafruit.com/products/1469
* Adafruit invests time and resources providing this open source code,
* please support Adafruit and open-source hardware by purchasing
* products from Adafruit!
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions
*  are met:
*
*    Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*****************************************************************************/
#ifndef __ERROR_CODES__
#define __ERROR_CODES__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef	__cplusplus
extern "C" {
#endif

//
// Error numbers
//
#define ERROR_WIFI_ALREADY_DISCONNECTED -129
#define NOT_ENOUGH_SOCKETS       -128
#define SOCKET_ALREADY_EXISTS    -127
#define NOT_SUPPORTED            -126
#define TCP_OPEN_FAILED          -124
#define BAD_SOCKET_DATA          -123
#define SOCKET_NOT_FOUND         -122
#define SOCKET_TIMED_OUT         -121
#define BAD_IP_HEADER            -120
#define NEED_TO_LISTEN           -119
#define RECV_TIMED_OUT           -118
#define NEED_TO_SEND             -114
#define UNABLE_TO_SEND           -113
#define DHCP_ERROR               -100
#define DHCP_LEASE_EXPIRED       -99
#define ARP_REQUEST_FAILED       -95
#define DHCP_LEASE_RENEWING      -92
#define IGMP_ERROR               -91
#define INVALID_VALUE            -85
#define DNS_ID_ERROR             -75
#define DNS_OPCODE_ERROR         -74
#define DNS_RCODE_ERROR          -73
#define DNS_COUNT_ERROR          -72
#define DNS_TYPE_ERROR           -71
#define DNS_CLASS_ERROR          -70
#define DNS_NOT_FOUND            -69
#define SOCKET_BUFFER_TOO_SMALL  -68
#define REASSEMBLY_ERROR         -64
#define REASSEMBLY_TIMED_OUT     -63
#define BAD_REASSEMBLY_DATA      -62
#define UNABLE_TO_TCP_SEND       -60
#define ERROR_WIFI_NOT_CONNECTED -59
#define SEND_FAILED_ARP_IN_PROCESS -58
#define RECV_FAILED_SOCKET_INACTIVE -57

//
// Return the same error code to all socket 
// calls which fail due to socket's inactivity
//
#define ERROR_SOCKET_INACTIVE   RECV_FAILED_SOCKET_INACTIVE 

//
// TCP function error codes
//
#define TCP_ERROR       -1
#define TCP_TOO_LONG    -2
#define TCP_BAD_HEADER  -3
#define TCP_BAD_CSUM    -4
#define TCP_BAD_FCS     -5
#define TCP_NO_CONNECT  -6

//
// UDP function error codes
//
#define UDP_ERROR       -1
#define UDP_TOO_LONG    -2
#define UDP_BAD_CSUM    -4
#define UDP_BAD_FCS     -5

//
// Raw error codes
//
#define RAW_ERROR       -1
#define RAW_TOO_LONG    -2

//
// SimpleLink error codes
//
#define SL_INVALID_INTERFACE            -1
#define SL_NO_MORE_DATA_TO_READ         -2
#define SL_OUT_OF_RESOURCES             (-150)
#define SL_NOT_ENOUGH_SPACE             (-151)
#define SL_INCORRECT_IF                 (-152)
#define SL_NOTHING_TO_SEND              (-153)
#define SL_WILL_SEND_LATER              (100)       // This is not an error - just an indication that we can't send data now
#define SL_TX_ALLOC_FAILED              (-161)

#define SL_INVALID_COMMAND_ARGUMENTS    (-170)

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* __ERROR_CODES__ */
