#define SEND_TIMEOUT_MS (30 * 1000)

/*****************************************************************************
*
*  socket.c  - CC3000 Host Driver Implementation.
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

//*****************************************************************************
//
//! \addtogroup socket_api
//! @{
//
//*****************************************************************************

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "hci.h"
#include "socket.h"
#include "evnt_handler.h"
#include "netapp.h"
#include "debug.h"



//Enable this flag if and only if you must comply with BSD socket 
//close() function
#ifdef _API_USE_BSD_CLOSE
#define close(sd) closesocket(sd)
#endif

//Enable this flag if and only if you must comply with BSD socket read() and 
//write() functions
#ifdef _API_USE_BSD_READ_WRITE
#define read(sd, buf, len, flags) recv(sd, buf, len, flags)
#define write(sd, buf, len, flags) send(sd, buf, len, flags)
#endif

#define SOCKET_OPEN_PARAMS_LEN				(12)
#define SOCKET_CLOSE_PARAMS_LEN				(4)
#define SOCKET_ACCEPT_PARAMS_LEN			(4)
#define SOCKET_BIND_PARAMS_LEN				(20)
#define SOCKET_LISTEN_PARAMS_LEN			(8)
#define SOCKET_GET_HOST_BY_NAME_PARAMS_LEN	(9)
#define SOCKET_CONNECT_PARAMS_LEN			(20)
#define SOCKET_SELECT_PARAMS_LEN			(44)
#define SOCKET_SET_SOCK_OPT_PARAMS_LEN		(20)
#define SOCKET_GET_SOCK_OPT_PARAMS_LEN		(12)
#define SOCKET_RECV_FROM_PARAMS_LEN			(12)
#define SOCKET_SENDTO_PARAMS_LEN			(24)
#define SOCKET_MDNS_ADVERTISE_PARAMS_LEN	(12)
#define SOCKET_GET_MSS_VALUE_PARAMS_LEN		(4)

// The legnth of arguments for the SEND command: sd + buff_offset + len + flags, 
// while size of each parameter is 32 bit - so the total length is 16 bytes;

#define HCI_CMND_SEND_ARG_LENGTH	(16)


#define SELECT_TIMEOUT_MIN_MICRO_SECONDS  5000

#define HEADERS_SIZE_DATA       (SPI_HEADER_SIZE + 5)

#define SIMPLE_LINK_HCI_CMND_TRANSPORT_HEADER_SIZE  (SPI_HEADER_SIZE + SIMPLE_LINK_HCI_CMND_HEADER_SIZE)

#define MDNS_DEVICE_SERVICE_MAX_LENGTH 	(32)

#ifdef MDNS_ADVERTISE_HOST
extern UINT8 localIP[4];
#endif


//*****************************************************************************
//
//! HostFlowControlConsumeBuff
//!
//!  @param  sd  socket descriptor
//!
//!  @return 0 in case there are buffers available, 
//!          -1 in case of bad socket
//!          -2 if there are no free buffers present (only when 
//!          SEND_NON_BLOCKING is enabled)
//!
//!  @brief  if SEND_NON_BLOCKING not define - block until have free buffer 
//!          becomes available, else return immediately  with correct status 
//!          regarding the buffers available.
//
//*****************************************************************************
INT16 HostFlowControlConsumeBuff(INT16 sd)
{
#ifndef SEND_NON_BLOCKING
	/* wait in busy loop */

// Adafruit CC3k Host Driver Difference
// Allow defining a send timeout period.
// Noted 12-12-2014 by tdicola
#ifdef SEND_TIMEOUT_MS
	unsigned long startTime = millis();
#endif

	do
	{
		// In case last transmission failed then we will return the last failure 
		// reason here.
		// Note that the buffer will not be allocated in this case
		if (tSLInformation.slTransmitDataError != 0)
		{
			errno = tSLInformation.slTransmitDataError;
			tSLInformation.slTransmitDataError = 0;
			return errno;
		}

		if(SOCKET_STATUS_ACTIVE != get_socket_active_status(sd))
			return -1;

// Adafruit CC3k Host Driver Difference
// Implementation of send timeout.
// Noted 12-12-2014 by tdicola
#ifdef SEND_TIMEOUT_MS
		if ((millis() - startTime) > SEND_TIMEOUT_MS)
		{
			return -3; /* Timeout */
		}
#endif

		// Adafruit CC3k Host Driver Difference
		// Poll CC3k for available bytes with select during buffer wait.
		// Noted 04-06-2015 by tdicola
		// This is a bizarre change that majorly helps reliability in the later
		// Arduino 1.6.x IDE and its newer avr-gcc toolchain.  Without the select
		// call the CC3k will frequently get stuck in this loop waiting for a buffer
		// to be free to send data and the CC3k never sending the async event
		// that buffers are free.  It is unclear why calling select fixes this
		// issue but my suspicion is it might be making the CC3k realize there
		// are free buffers and it passing the async event along that will
		// eventually break out of this loop.  I tried to check if it was just a
		// timing issue by substituting a small delay for the select call, but 
		// the problem persists.  Calling select appears to be necessary to keep
		// the CC3k from hitting some race condition/issue here.
		if (tSLInformation.usNumberOfFreeBuffers == 0) {
			// Do a select() call on the socket to see if data is available to read.
			timeval timeout;
			fd_set fd_read;
			memset(&fd_read, 0, sizeof(fd_read));
			FD_SET(sd, &fd_read);
			timeout.tv_sec = 0;
			timeout.tv_usec = 5000;
			select(sd+1, &fd_read, NULL, NULL, &timeout);
			// Note the results of the select are ignored for now.  Attempts to
			// have smart behavior like returning an error when there is data
			// to read and no free buffers for sending just seem to cause more
			// problems particularly with server code examples.
		}

	} while(0 == tSLInformation.usNumberOfFreeBuffers);

	tSLInformation.usNumberOfFreeBuffers--;

	return 0;
#else

	// In case last transmission failed then we will return the last failure 
	// reason here.
	// Note that the buffer will not be allocated in this case
	if (tSLInformation.slTransmitDataError != 0)
	{
		errno = tSLInformation.slTransmitDataError;
		tSLInformation.slTransmitDataError = 0;
		return errno;
	}
	if(SOCKET_STATUS_ACTIVE != get_socket_active_status(sd))
		return -1;

	//If there are no available buffers, return -2. It is recommended to use  
	// select or receive to see if there is any buffer occupied with received data
	// If so, call receive() to release the buffer.
	if(0 == tSLInformation.usNumberOfFreeBuffers)
	{
		return -2;
	}
	else
	{
		tSLInformation.usNumberOfFreeBuffers--;
		return 0;
	}
#endif
}

//*****************************************************************************
//
//! socket
//!
//!  @param  domain    selects the protocol family which will be used for 
//!                    communication. On this version only AF_INET is supported
//!  @param  type      specifies the communication semantics. On this version 
//!                    only SOCK_STREAM, SOCK_DGRAM, SOCK_RAW are supported
//!  @param  protocol  specifies a particular protocol to be used with the 
//!                    socket IPPROTO_TCP, IPPROTO_UDP or IPPROTO_RAW are 
//!                    supported.
//!
//!  @return  On success, socket handle that is used for consequent socket 
//!           operations. On error, -1 is returned.
//!
//!  @brief  create an endpoint for communication
//!          The socket function creates a socket that is bound to a specific 
//!          transport service provider. This function is called by the 
//!          application layer to obtain a socket handle.
//
//*****************************************************************************

INT32 socket(INT32 domain, INT32 type, INT32 protocol)
{
	INT32 ret;
	UINT8 *ptr, *args;

	ret = EFAIL;
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in HCI packet structure
	args = UINT32_TO_STREAM(args, domain);
	args = UINT32_TO_STREAM(args, type);
	args = UINT32_TO_STREAM(args, protocol);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_SOCKET, ptr, SOCKET_OPEN_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_SOCKET, &ret);

	// Process the event 
	errno = ret;

	set_socket_active_status(ret, SOCKET_STATUS_ACTIVE);

	return(ret);
}

//*****************************************************************************
//
//! closesocket
//!
//!  @param  sd    socket handle.
//!
//!  @return  On success, zero is returned. On error, -1 is returned.
//!
//!  @brief  The socket function closes a created socket.
//
//*****************************************************************************

INT32 closesocket(INT32 sd)
{
	INT32 ret;
	UINT8 *ptr, *args;

	ret = EFAIL;
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in HCI packet structure
	args = UINT32_TO_STREAM(args, sd);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_CLOSE_SOCKET,
		ptr, SOCKET_CLOSE_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_CLOSE_SOCKET, &ret);
	errno = ret;

	// since 'close' call may result in either OK (and then it closed) or error 
	// mark this socket as invalid 
	set_socket_active_status(sd, SOCKET_STATUS_INACTIVE);

	return(ret);
}

//*****************************************************************************
//
//! accept
//!
//!  @param[in]   sd      socket descriptor (handle)              
//!  @param[out]  addr    the argument addr is a pointer to a sockaddr structure
//!                       This structure is filled in with the address of the  
//!                       peer socket, as known to the communications layer.        
//!                       determined. The exact format of the address returned             
//!                       addr is by the socket's address sockaddr. 
//!                       On this version only AF_INET is supported.
//!                       This argument returns in network order.
//!  @param[out] addrlen  the addrlen argument is a value-result argument: 
//!                       it should initially contain the size of the structure
//!                       pointed to by addr.
//!
//!  @return  For socket in blocking mode:
//!				      On success, socket handle. on failure negative
//!			      For socket in non-blocking mode:
//!				     - On connection establishment, socket handle
//!				     - On connection pending, SOC_IN_PROGRESS (-2)
//!			       - On failure, SOC_ERROR	(-1)
//!
//!  @brief  accept a connection on a socket:
//!          This function is used with connection-based socket types 
//!          (SOCK_STREAM). It extracts the first connection request on the 
//!          queue of pending connections, creates a new connected socket, and
//!          returns a new file descriptor referring to that socket.
//!          The newly created socket is not in the listening state. 
//!          The original socket sd is unaffected by this call. 
//!          The argument sd is a socket that has been created with socket(),
//!          bound to a local address with bind(), and is  listening for 
//!          connections after a listen(). The argument addr is a pointer 
//!          to a sockaddr structure. This structure is filled in with the 
//!          address of the peer socket, as known to the communications layer.
//!          The exact format of the address returned addr is determined by the 
//!          socket's address family. The addrlen argument is a value-result
//!          argument: it should initially contain the size of the structure
//!          pointed to by addr, on return it will contain the actual 
//!          length (in bytes) of the address returned.
//!
//! @sa     socket ; bind ; listen
//
//*****************************************************************************

INT32 accept(INT32 sd, sockaddr *addr, socklen_t *addrlen)
{
	INT32 ret;
	UINT8 *ptr, *args;
	tBsdReturnParams tAcceptReturnArguments;

	ret = EFAIL;
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_ACCEPT,
		ptr, SOCKET_ACCEPT_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_ACCEPT, &tAcceptReturnArguments);


	// need specify return parameters!!!
	// Adafruit CC3k Host Driver Difference
	// Bug fix to prevent writing to null memory pointer.
	// Noted 12-12-2014 by tdicola
	if (addr) memcpy(addr, &tAcceptReturnArguments.tSocketAddress, ASIC_ADDR_LEN);
	if (addrlen) *addrlen = ASIC_ADDR_LEN;
	errno = tAcceptReturnArguments.iStatus; 
	ret = errno;

	// if succeeded, iStatus = new socket descriptor. otherwise - error number 
	if(M_IS_VALID_SD(ret))
	{
		set_socket_active_status(ret, SOCKET_STATUS_ACTIVE);
	}
	else
	{
		set_socket_active_status(sd, SOCKET_STATUS_INACTIVE);
	}

	return(ret);
}

//*****************************************************************************
//
//! bind
//!
//!  @param[in]   sd      socket descriptor (handle)              
//!  @param[out]  addr    specifies the destination address. On this version 
//!                       only AF_INET is supported.
//!  @param[out] addrlen  contains the size of the structure pointed to by addr.
//!
//!  @return  	On success, zero is returned. On error, -1 is returned.
//!
//!  @brief  assign a name to a socket
//!          This function gives the socket the local address addr.
//!          addr is addrlen bytes long. Traditionally, this is called when a 
//!          socket is created with socket, it exists in a name space (address 
//!          family) but has no name assigned.
//!          It is necessary to assign a local address before a SOCK_STREAM
//!          socket may receive connections.
//!
//! @sa     socket ; accept ; listen
//
//*****************************************************************************

INT32 bind(INT32 sd, const sockaddr *addr, INT32 addrlen)
{
	INT32 ret;
	UINT8 *ptr, *args;

	ret = EFAIL;
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	addrlen = ASIC_ADDR_LEN;

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);
	args = UINT32_TO_STREAM(args, 0x00000008);
	args = UINT32_TO_STREAM(args, addrlen);
	ARRAY_TO_STREAM(args, ((UINT8 *)addr), addrlen);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_BIND,
		ptr, SOCKET_BIND_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_BIND, &ret);

	errno = ret;

	return(ret);
}

//*****************************************************************************
//
//! listen
//!
//!  @param[in]   sd      socket descriptor (handle)              
//!  @param[in]  backlog  specifies the listen queue depth. On this version
//!                       backlog is not supported.
//!  @return  	On success, zero is returned. On error, -1 is returned.
//!
//!  @brief  listen for connections on a socket
//!          The willingness to accept incoming connections and a queue
//!          limit for incoming connections are specified with listen(),
//!          and then the connections are accepted with accept.
//!          The listen() call applies only to sockets of type SOCK_STREAM
//!          The backlog parameter defines the maximum length the queue of
//!          pending connections may grow to. 
//!
//! @sa     socket ; accept ; bind
//!
//! @note   On this version, backlog is not supported
//
//*****************************************************************************

INT32 listen(INT32 sd, INT32 backlog)
{
	INT32 ret;
	UINT8 *ptr, *args;

	ret = EFAIL;
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);
	args = UINT32_TO_STREAM(args, backlog);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_LISTEN,
		ptr, SOCKET_LISTEN_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_LISTEN, &ret);
	errno = ret;

	return(ret);
}

//*****************************************************************************
//
//! gethostbyname
//!
//!  @param[in]   hostname     host name              
//!  @param[in]   usNameLen    name length 
//!  @param[out]  out_ip_addr  This parameter is filled in with host IP address. 
//!                            In case that host name is not resolved, 
//!                            out_ip_addr is zero.                  
//!  @return  	On success, positive is returned. On error, negative is returned
//!
//!  @brief  Get host IP by name. Obtain the IP Address of machine on network, 
//!          by its name.
//!
//!  @note  On this version, only blocking mode is supported. Also note that
//!		     the function requires DNS server to be configured prior to its usage.
//
//*****************************************************************************

#ifndef CC3000_TINY_DRIVER
// Adafruit CC3k Host Driver Difference
// Make hostname a const char pointer because it isn't modified and the Adafruit
// driver code needs it to be const to interface with Arduino's client library.
// Noted 12-12-2014 by tdicola
INT16 gethostbyname(const CHAR * hostname, UINT16 usNameLen, 
	UINT32* out_ip_addr)
{
	tBsdGethostbynameParams ret;
	UINT8 *ptr, *args;

	errno = EFAIL;

	if (usNameLen > HOSTNAME_MAX_LENGTH)
	{
		return errno;
	}

	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + SIMPLE_LINK_HCI_CMND_TRANSPORT_HEADER_SIZE);

	// Fill in HCI packet structure
	args = UINT32_TO_STREAM(args, 8);
	args = UINT32_TO_STREAM(args, usNameLen);
	ARRAY_TO_STREAM(args, hostname, usNameLen);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_GETHOSTNAME, ptr, SOCKET_GET_HOST_BY_NAME_PARAMS_LEN
		+ usNameLen - 1);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_EVNT_BSD_GETHOSTBYNAME, &ret);

	errno = ret.retVal;

	(*((INT32*)out_ip_addr)) = ret.outputAddress;

	return (errno);

}
#endif

//*****************************************************************************
//
//! connect
//!
//!  @param[in]   sd       socket descriptor (handle)         
//!  @param[in]   addr     specifies the destination addr. On this version
//!                        only AF_INET is supported.
//!  @param[out]  addrlen  contains the size of the structure pointed to by addr    
//!  @return  	On success, zero is returned. On error, -1 is returned
//!
//!  @brief  initiate a connection on a socket 
//!          Function connects the socket referred to by the socket descriptor 
//!          sd, to the address specified by addr. The addrlen argument 
//!          specifies the size of addr. The format of the address in addr is 
//!          determined by the address space of the socket. If it is of type 
//!          SOCK_DGRAM, this call specifies the peer with which the socket is 
//!          to be associated; this address is that to which datagrams are to be
//!          sent, and the only address from which datagrams are to be received.  
//!          If the socket is of type SOCK_STREAM, this call attempts to make a 
//!          connection to another socket. The other socket is specified  by 
//!          address, which is an address in the communications space of the
//!          socket. Note that the function implements only blocking behavior 
//!          thus the caller will be waiting either for the connection 
//!          establishment or for the connection establishment failure.
//!
//!  @sa socket
//
//*****************************************************************************

INT32 connect(INT32 sd, const sockaddr *addr, INT32 addrlen)
{
	INT32 ret;
	UINT8 *ptr, *args;

	ret = EFAIL;
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + SIMPLE_LINK_HCI_CMND_TRANSPORT_HEADER_SIZE);
	addrlen = 8;

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);
	args = UINT32_TO_STREAM(args, 0x00000008);
	args = UINT32_TO_STREAM(args, addrlen);
	ARRAY_TO_STREAM(args, ((UINT8 *)addr), addrlen);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_CONNECT,
		ptr, SOCKET_CONNECT_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_CONNECT, &ret);

	errno = ret;

	return((INT32)ret);
}


//*****************************************************************************
//
//! select
//!
//!  @param[in]   nfds       the highest-numbered file descriptor in any of the
//!                           three sets, plus 1.     
//!  @param[out]   writesds   socket descriptors list for write monitoring
//!  @param[out]   readsds    socket descriptors list for read monitoring  
//!  @param[out]   exceptsds  socket descriptors list for exception monitoring
//!  @param[in]   timeout     is an upper bound on the amount of time elapsed
//!                           before select() returns. Null means infinity 
//!                           timeout. The minimum timeout is 5 milliseconds,
//!                          less than 5 milliseconds will be set
//!                           automatically to 5 milliseconds.
//!  @return  	On success, select() returns the number of file descriptors
//!             contained in the three returned descriptor sets (that is, the
//!             total number of bits that are set in readfds, writefds,
//!             exceptfds) which may be zero if the timeout expires before
//!             anything interesting  happens.
//!             On error, -1 is returned.
//!                   *readsds - return the sockets on which Read request will
//!                              return without delay with valid data.
//!                   *writesds - return the sockets on which Write request 
//!                                 will return without delay.
//!                   *exceptsds - return the sockets which closed recently.
//!
//!  @brief  Monitor socket activity  
//!          Select allow a program to monitor multiple file descriptors,
//!          waiting until one or more of the file descriptors become 
//!         "ready" for some class of I/O operation 
//!
//!  @Note   If the timeout value set to less than 5ms it will automatically set
//!          to 5ms to prevent overload of the system
//!
//!  @sa socket
//
//*****************************************************************************

INT16 select(INT32 nfds, fd_set *readsds, fd_set *writesds, fd_set *exceptsds, 
struct timeval *timeout)
{
	UINT8 *ptr, *args;
	tBsdSelectRecvParams tParams;
	UINT32 is_blocking;

	if( timeout == NULL)
	{
		is_blocking = 1; /* blocking , infinity timeout */
	}
	else
	{
		is_blocking = 0; /* no blocking, timeout */
	}

	// Fill in HCI packet structure
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, nfds);
	args = UINT32_TO_STREAM(args, 0x00000014);
	args = UINT32_TO_STREAM(args, 0x00000014);
	args = UINT32_TO_STREAM(args, 0x00000014);
	args = UINT32_TO_STREAM(args, 0x00000014);
	args = UINT32_TO_STREAM(args, is_blocking);
	args = UINT32_TO_STREAM(args, ((readsds) ? *(UINT32*)readsds : 0));
	args = UINT32_TO_STREAM(args, ((writesds) ? *(UINT32*)writesds : 0));
	args = UINT32_TO_STREAM(args, ((exceptsds) ? *(UINT32*)exceptsds : 0));

	if (timeout)
	{
		if ( 0 == timeout->tv_sec && timeout->tv_usec < 
			SELECT_TIMEOUT_MIN_MICRO_SECONDS)
		{
			timeout->tv_usec = SELECT_TIMEOUT_MIN_MICRO_SECONDS;
		}
		args = UINT32_TO_STREAM(args, timeout->tv_sec);
		args = UINT32_TO_STREAM(args, timeout->tv_usec);
	}

	// Initiate a HCI command
	hci_command_send(HCI_CMND_BSD_SELECT, ptr, SOCKET_SELECT_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_EVNT_SELECT, &tParams);

	// Update actually read FD
	if (tParams.iStatus >= 0)
	{
		if (readsds)
		{
			memcpy(readsds, &tParams.uiRdfd, sizeof(tParams.uiRdfd));
		}

		if (writesds)
		{
			memcpy(writesds, &tParams.uiWrfd, sizeof(tParams.uiWrfd)); 
		}

		if (exceptsds)
		{
			memcpy(exceptsds, &tParams.uiExfd, sizeof(tParams.uiExfd)); 
		}

		return(tParams.iStatus);

	}
	else
	{
		errno = tParams.iStatus;
		return(-1);
	}
}

//*****************************************************************************
//
//! setsockopt
//!
//!  @param[in]   sd          socket handle
//!  @param[in]   level       defines the protocol level for this option
//!  @param[in]   optname     defines the option name to Interrogate
//!  @param[in]   optval      specifies a value for the option
//!  @param[in]   optlen      specifies the length of the option value
//!  @return  	On success, zero is returned. On error, -1 is returned
//!
//!  @brief  set socket options
//!          This function manipulate the options associated with a socket.
//!          Options may exist at multiple protocol levels; they are always
//!          present at the uppermost socket level.
//!          When manipulating socket options the level at which the option 
//!          resides and the name of the option must be specified.  
//!          To manipulate options at the socket level, level is specified as 
//!          SOL_SOCKET. To manipulate options at any other level the protocol 
//!          number of the appropriate protocol controlling the option is 
//!          supplied. For example, to indicate that an option is to be 
//!          interpreted by the TCP protocol, level should be set to the 
//!          protocol number of TCP; 
//!          The parameters optval and optlen are used to access optval - 
//!          use for setsockopt(). For getsockopt() they identify a buffer
//!          in which the value for the requested option(s) are to 
//!          be returned. For getsockopt(), optlen is a value-result 
//!          parameter, initially containing the size of the buffer 
//!          pointed to by option_value, and modified on return to 
//!          indicate the actual size of the value returned. If no option 
//!          value is to be supplied or returned, option_value may be NULL.
//!
//!  @Note   On this version the following two socket options are enabled:
//!          The only protocol level supported in this version
//!          is SOL_SOCKET (level).
//!           1. SOCKOPT_RECV_NONBLOCK (optname)
//!           SOCKOPT_RECV_NONBLOCK sets the recv and recvfrom 
//!           non-blocking modes on or off.
//!           In that case optval should be SOCK_ON or SOCK_OFF (optval).
//!
//!           2. SOCKOPT_RECV_TIMEOUT (optname)
//!           SOCKOPT_RECV_TIMEOUT configures recv and recvfrom timeout 
//!           in milliseconds.
//!           In that case optval should be pointer to UINT32.
//!		       
//!           3. SOCKOPT_ACCEPT_NONBLOCK (optname). sets the socket accept 
//!           non-blocking mode on or off.
//!           In that case optval should be SOCK_ON or SOCK_OFF (optval).
//!
//!  @sa getsockopt
//
//*****************************************************************************

#ifndef CC3000_TINY_DRIVER
INT16 setsockopt(INT32 sd, INT32 level, INT32 optname, const void *optval,
	socklen_t optlen)
{
	INT32 ret;
	UINT8 *ptr, *args;

	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);
	args = UINT32_TO_STREAM(args, level);
	args = UINT32_TO_STREAM(args, optname);
	args = UINT32_TO_STREAM(args, 0x00000008);
	args = UINT32_TO_STREAM(args, optlen);
	ARRAY_TO_STREAM(args, ((UINT8 *)optval), optlen);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_SETSOCKOPT,
		ptr, SOCKET_SET_SOCK_OPT_PARAMS_LEN  + optlen);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_SETSOCKOPT, &ret);

	if (ret >= 0)
	{
		return (0);
	}
	else
	{
		errno = ret;
		return ret;
	}
}
#endif

//*****************************************************************************
//
//! getsockopt
//!
//!  @param[in]   sd          socket handle
//!  @param[in]   level       defines the protocol level for this option
//!  @param[in]   optname     defines the option name to Interrogate
//!  @param[out]   optval      specifies a value for the option
//!  @param[out]   optlen      specifies the length of the option value
//!  @return  	On success, zero is returned. On error, -1 is returned
//!
//!  @brief  set socket options
//!          This function manipulate the options associated with a socket.
//!          Options may exist at multiple protocol levels; they are always
//!          present at the uppermost socket level.
//!          When manipulating socket options the level at which the option 
//!          resides and the name of the option must be specified.  
//!          To manipulate options at the socket level, level is specified as 
//!          SOL_SOCKET. To manipulate options at any other level the protocol 
//!          number of the appropriate protocol controlling the option is 
//!          supplied. For example, to indicate that an option is to be 
//!          interpreted by the TCP protocol, level should be set to the 
//!          protocol number of TCP; 
//!          The parameters optval and optlen are used to access optval - 
//!          use for setsockopt(). For getsockopt() they identify a buffer
//!          in which the value for the requested option(s) are to 
//!          be returned. For getsockopt(), optlen is a value-result 
//!          parameter, initially containing the size of the buffer 
//!          pointed to by option_value, and modified on return to 
//!          indicate the actual size of the value returned. If no option 
//!          value is to be supplied or returned, option_value may be NULL.
//!
//!  @Note   On this version the following two socket options are enabled:
//!    			 The only protocol level supported in this version
//!          is SOL_SOCKET (level).
//!           1. SOCKOPT_RECV_NONBLOCK (optname)
//!           SOCKOPT_RECV_NONBLOCK sets the recv and recvfrom 
//!           non-blocking modes on or off.
//!           In that case optval should be SOCK_ON or SOCK_OFF (optval).
//!
//!           2. SOCKOPT_RECV_TIMEOUT (optname)
//!           SOCKOPT_RECV_TIMEOUT configures recv and recvfrom timeout 
//!           in milliseconds.
//!           In that case optval should be pointer to UINT32.
//!
//!           3. SOCKOPT_ACCEPT_NONBLOCK (optname). sets the socket accept 
//!           non-blocking mode on or off.
//!           In that case optval should be SOCK_ON or SOCK_OFF (optval).
//!
//!  @sa setsockopt
//
//*****************************************************************************

INT16 getsockopt (INT32 sd, INT32 level, INT32 optname, void *optval, socklen_t *optlen)
{
	UINT8 *ptr, *args;
	tBsdGetSockOptReturnParams  tRetParams;

	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);
	args = UINT32_TO_STREAM(args, level);
	args = UINT32_TO_STREAM(args, optname);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_GETSOCKOPT,
		ptr, SOCKET_GET_SOCK_OPT_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_CMND_GETSOCKOPT, &tRetParams);

	if (((INT8)tRetParams.iStatus) >= 0)
	{
		*optlen = 4;
		memcpy(optval, tRetParams.ucOptValue, 4);
		return (0);
	}
	else
	{
		errno = tRetParams.iStatus;
		return errno;
	}
}

//*****************************************************************************
//
//!  simple_link_recv
//!
//!  @param sd       socket handle
//!  @param buf      read buffer
//!  @param len      buffer length
//!  @param flags    indicates blocking or non-blocking operation
//!  @param from     pointer to an address structure indicating source address
//!  @param fromlen  source address structure size
//!
//!  @return         Return the number of bytes received, or -1 if an error
//!                  occurred
//!
//!  @brief          Read data from socket
//!                  Return the length of the message on successful completion.
//!                  If a message is too long to fit in the supplied buffer,
//!                  excess bytes may be discarded depending on the type of
//!                  socket the message is received from
//
//*****************************************************************************
INT16 simple_link_recv(INT32 sd, void *buf, INT32 len, INT32 flags, sockaddr *from,
	socklen_t *fromlen, INT32 opcode)
{
	UINT8 *ptr, *args;
	tBsdReadReturnParams tSocketReadEvent;

	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in HCI packet structure
	args = UINT32_TO_STREAM(args, sd);
	args = UINT32_TO_STREAM(args, len);
	args = UINT32_TO_STREAM(args, flags);

	// Generate the read command, and wait for the 
	hci_command_send(opcode,  ptr, SOCKET_RECV_FROM_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(opcode, &tSocketReadEvent);

	// Adafruit CC3k Host Driver Difference
	// Extra debug output.
	// Noted 12-12-2014 by tdicola
	DEBUGPRINT_F("\n\r\tRecv'd data... Socket #");
	DEBUGPRINT_DEC(tSocketReadEvent.iSocketDescriptor);
	DEBUGPRINT_F(" Bytes: 0x");
	DEBUGPRINT_HEX(tSocketReadEvent.iNumberOfBytes);
	DEBUGPRINT_F(" Flags: 0x");
	DEBUGPRINT_HEX(tSocketReadEvent.uiFlags);
	DEBUGPRINT_F("\n\r");

	// In case the number of bytes is more then zero - read data
	if (tSocketReadEvent.iNumberOfBytes > 0)
	{
		// Wait for the data in a synchronous way. Here we assume that the bug is 
		// big enough to store also parameters of receive from too....
		// Adafruit CC3k Host Driver Difference
		// Fix compiler error with explicit cast from void to UINT8 pointer.
		// Noted 12-12-2014 by tdicola
		SimpleLinkWaitData((UINT8*)buf, (UINT8 *)from, (UINT8 *)fromlen);
	}

	errno = tSocketReadEvent.iNumberOfBytes;

// Adafruit CC3k Host Driver Difference
// Extra debug output.
// Noted 12-12-2014 by tdicola
#if (DEBUG_MODE == 1)
	for (UINT8 i=0; i<errno; i++) {
	  uart_putchar(((UINT8 *)buf)[i]);
	}
#endif
	
	return(tSocketReadEvent.iNumberOfBytes);
}

//*****************************************************************************
//
//!  recv
//!
//!  @param[in]  sd     socket handle
//!  @param[out] buf    Points to the buffer where the message should be stored
//!  @param[in]  len    Specifies the length in bytes of the buffer pointed to 
//!                     by the buffer argument.
//!  @param[in] flags   Specifies the type of message reception. 
//!                     On this version, this parameter is not supported.
//!
//!  @return         Return the number of bytes received, or -1 if an error
//!                  occurred
//!
//!  @brief          function receives a message from a connection-mode socket
//!
//!  @sa recvfrom
//!
//!  @Note On this version, only blocking mode is supported.
//
//*****************************************************************************

INT16 recv(INT32 sd, void *buf, INT32 len, INT32 flags)
{
	return(simple_link_recv(sd, buf, len, flags, NULL, NULL, HCI_CMND_RECV));
}

//*****************************************************************************
//
//!  recvfrom
//!
//!  @param[in]  sd     socket handle
//!  @param[out] buf    Points to the buffer where the message should be stored
//!  @param[in]  len    Specifies the length in bytes of the buffer pointed to 
//!                     by the buffer argument.
//!  @param[in] flags   Specifies the type of message reception. 
//!                     On this version, this parameter is not supported.
//!  @param[in] from   pointer to an address structure indicating the source
//!                    address: sockaddr. On this version only AF_INET is
//!                    supported.
//!  @param[in] fromlen   source address tructure size
//!
//!  @return         Return the number of bytes received, or -1 if an error
//!                  occurred
//!
//!  @brief         read data from socket
//!                 function receives a message from a connection-mode or
//!                 connectionless-mode socket. Note that raw sockets are not
//!                 supported.
//!
//!  @sa recv
//!
//!  @Note On this version, only blocking mode is supported.
//
//*****************************************************************************
INT16 recvfrom(INT32 sd, void *buf, INT32 len, INT32 flags, sockaddr *from,
	socklen_t *fromlen)
{
	return(simple_link_recv(sd, buf, len, flags, from, fromlen,
		HCI_CMND_RECVFROM));
}

//*****************************************************************************
//
//!  simple_link_send
//!
//!  @param sd       socket handle
//!  @param buf      write buffer
//!  @param len      buffer length
//!  @param flags    On this version, this parameter is not supported
//!  @param to       pointer to an address structure indicating destination
//!                  address
//!  @param tolen    destination address structure size
//!
//!  @return         Return the number of bytes transmitted, or -1 if an error
//!                  occurred, or -2 in case there are no free buffers available
//!                 (only when SEND_NON_BLOCKING is enabled)
//!
//!  @brief          This function is used to transmit a message to another
//!                  socket
//
//*****************************************************************************
INT16 simple_link_send(INT32 sd, const void *buf, INT32 len, INT32 flags,
	const sockaddr *to, INT32 tolen, INT32 opcode)
{    
	UINT8 uArgSize,  addrlen;
	UINT8 *ptr, *pDataPtr, *args;
	UINT32 addr_offset;
	INT16 res;
	tBsdReadReturnParams tSocketSendEvent;

	// Check the bsd_arguments
	if (0 != (res = HostFlowControlConsumeBuff(sd)))
	{
		return res;
	}

	//Update the number of sent packets
	tSLInformation.NumberOfSentPackets++;

	// Allocate a buffer and construct a packet and send it over spi
	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_DATA);

	// Update the offset of data and parameters according to the command
	switch(opcode)
	{ 
	case HCI_CMND_SENDTO:
		{
			addr_offset = len + sizeof(len) + sizeof(len);
			addrlen = 8;
			uArgSize = SOCKET_SENDTO_PARAMS_LEN;
			pDataPtr = ptr + HEADERS_SIZE_DATA + SOCKET_SENDTO_PARAMS_LEN;
			break;
		}

	case HCI_CMND_SEND:
		{
			tolen = 0;
			to = NULL;
			uArgSize = HCI_CMND_SEND_ARG_LENGTH;
			pDataPtr = ptr + HEADERS_SIZE_DATA + HCI_CMND_SEND_ARG_LENGTH;
			break;
		}

	default:
		{
			// Adafruit CC3k Host Driver Difference
			// Break out of function for unknown operation to prevent compiler warnings.
			// Noted 04-08-2015 by tdicola
			return -1;
		}
	}

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);
	args = UINT32_TO_STREAM(args, uArgSize - sizeof(sd));
	args = UINT32_TO_STREAM(args, len);
	args = UINT32_TO_STREAM(args, flags);

	if (opcode == HCI_CMND_SENDTO)
	{
		args = UINT32_TO_STREAM(args, addr_offset);
		args = UINT32_TO_STREAM(args, addrlen);
	}

	// Copy the data received from user into the TX Buffer
	ARRAY_TO_STREAM(pDataPtr, ((UINT8 *)buf), len);

	// In case we are using SendTo, copy the to parameters
	if (opcode == HCI_CMND_SENDTO)
	{	
		ARRAY_TO_STREAM(pDataPtr, ((UINT8 *)to), tolen);
	}

	// Initiate a HCI command
	hci_data_send(opcode, ptr, uArgSize, len,(UINT8*)to, tolen);

	if (opcode == HCI_CMND_SENDTO)
		SimpleLinkWaitEvent(HCI_EVNT_SENDTO, &tSocketSendEvent);
	else
		SimpleLinkWaitEvent(HCI_EVNT_SEND, &tSocketSendEvent);

	return	(len);
}


//*****************************************************************************
//
//!  send
//!
//!  @param sd       socket handle
//!  @param buf      Points to a buffer containing the message to be sent
//!  @param len      message size in bytes
//!  @param flags    On this version, this parameter is not supported
//!
//!  @return         Return the number of bytes transmitted, or -1 if an
//!                  error occurred
//!
//!  @brief          Write data to TCP socket
//!                  This function is used to transmit a message to another 
//!                  socket.
//!
//!  @Note           On this version, only blocking mode is supported.
//!
//!  @sa             sendto
//
//*****************************************************************************

INT16 send(INT32 sd, const void *buf, INT32 len, INT32 flags)
{
	return(simple_link_send(sd, buf, len, flags, NULL, 0, HCI_CMND_SEND));
}

//*****************************************************************************
//
//!  sendto
//!
//!  @param sd       socket handle
//!  @param buf      Points to a buffer containing the message to be sent
//!  @param len      message size in bytes
//!  @param flags    On this version, this parameter is not supported
//!  @param to       pointer to an address structure indicating the destination
//!                  address: sockaddr. On this version only AF_INET is
//!                  supported.
//!  @param tolen    destination address structure size
//!
//!  @return         Return the number of bytes transmitted, or -1 if an
//!                  error occurred
//!
//!  @brief          Write data to TCP socket
//!                  This function is used to transmit a message to another 
//!                  socket.
//!
//!  @Note           On this version, only blocking mode is supported.
//!
//!  @sa             send
//
//*****************************************************************************

INT16 sendto(INT32 sd, const void *buf, INT32 len, INT32 flags, const sockaddr *to,
	socklen_t tolen)
{
	return(simple_link_send(sd, buf, len, flags, to, tolen, HCI_CMND_SENDTO));
}

#ifndef MDNS_ADVERTISE_HOST
//*****************************************************************************
//
//!  mdnsAdvertiser
//!
//!  @param[in] mdnsEnabled         flag to enable/disable the mDNS feature
//!  @param[in] deviceServiceName   Service name as part of the published
//!                                 canonical domain name
//!  @param[in] deviceServiceNameLength   Length of the service name - up to 32 chars
//!
//!
//!  @return   On success, zero is returned, return SOC_ERROR if socket was not
//!            opened successfully, or if an error occurred.
//!
//!  @brief    Set CC3000 in mDNS advertiser mode in order to advertise itself.
//
//*****************************************************************************

INT16 mdnsAdvertiser(UINT16 mdnsEnabled, CHAR * deviceServiceName, UINT16 deviceServiceNameLength)
{
	INT8 ret;
	UINT8 *pTxBuffer, *pArgs;

	if (deviceServiceNameLength > MDNS_DEVICE_SERVICE_MAX_LENGTH)
	{
		return EFAIL;
	}

	pTxBuffer = tSLInformation.pucTxCommandBuffer;
	pArgs = (pTxBuffer + SIMPLE_LINK_HCI_CMND_TRANSPORT_HEADER_SIZE);

	// Fill in HCI packet structure
	pArgs = UINT32_TO_STREAM(pArgs, mdnsEnabled);
	pArgs = UINT32_TO_STREAM(pArgs, 8);
	pArgs = UINT32_TO_STREAM(pArgs, deviceServiceNameLength);
	ARRAY_TO_STREAM(pArgs, deviceServiceName, deviceServiceNameLength);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_MDNS_ADVERTISE, pTxBuffer, SOCKET_MDNS_ADVERTISE_PARAMS_LEN + deviceServiceNameLength);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_EVNT_MDNS_ADVERTISE, &ret);

	return ret;

}
#else
INT16 mdnsAdvertiser(UINT16 mdnsEnabled, CHAR * deviceServiceName, UINT16 deviceServiceNameLength)
{
    sockaddr tSocketAddr;
    INT32 mdnsSocket = -1;
    INT device_name_len;
    CHAR mdnsResponse[220];
    INT16 mdnsResponseLength;
    CHAR *mdnsResponsePtr;

    if(deviceServiceName != NULL)
    {
        device_name_len = strlen(deviceServiceName);
    }
    else
    {
        return EFAIL;
    }

    if (deviceServiceNameLength > MDNS_DEVICE_SERVICE_MAX_LENGTH)
	{
		return EFAIL;
	}

    mdnsSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(mdnsSocket < 0)
    {
        return -1;
    }

    //Send mDNS data to 224.0.0.251
    tSocketAddr.sa_family = AF_INET;

    // the destination port 5353
    tSocketAddr.sa_data[0] = 0x14;
    tSocketAddr.sa_data[1] = 0xe9;

    tSocketAddr.sa_data[2] = 0xe0;
    tSocketAddr.sa_data[3] = 0x00;
    tSocketAddr.sa_data[4] = 0x00;
    tSocketAddr.sa_data[5] = 0xfb;

	memset(mdnsResponse, 0, sizeof(mdnsResponse));
	mdnsResponsePtr = mdnsResponse;

	// mDNS header
	mdnsResponse[2] = 0x84;	                       // DNS flags
	mdnsResponse[7] = 0x5;	                       // number of answers
	mdnsResponsePtr += 12;

	// answer 1 - the device service name
	*mdnsResponsePtr++ = 12;	                   // size of _device-info
	memcpy(mdnsResponsePtr, "_device-info", 12);   // _device-info
	mdnsResponsePtr += 12;
	*mdnsResponsePtr++ = 4;	                       // size of _udp
	memcpy(mdnsResponsePtr, "_udp", 4);	           // _udp
	mdnsResponsePtr += 4;
	*mdnsResponsePtr++ = 5;	                       // size of local
	memcpy(mdnsResponsePtr, "local", 5);	       // local
	mdnsResponsePtr += 7;
	*mdnsResponsePtr = 0xc;	                       // PTR type
	mdnsResponsePtr += 2;
	*mdnsResponsePtr = 0x1;	                       // class IN
	mdnsResponsePtr += 3;
	*mdnsResponsePtr++ = 0x11;	                       // TTL = 4500 seconds
	*mdnsResponsePtr = 0x94;	                       // TTL = 4500 seconds
	mdnsResponsePtr += 4;	                           // domain and its length - filled during invoke of mDNS advertiser
	*mdnsResponsePtr++ = 0xc0;
	*mdnsResponsePtr++ = 0x0c;	                       // points to rest of the domain

	// answer 2 - the device-info service
	*mdnsResponsePtr++ = 9;	                           // size of _services
	memcpy(mdnsResponsePtr, "_services", 9);	       // _services
	mdnsResponsePtr += 9;
	*mdnsResponsePtr++ = 7;	                           // size of _dns-sd
	memcpy(mdnsResponsePtr, "_dns-sd", 7);	           // _dns-sd
	mdnsResponsePtr += 7;
	*mdnsResponsePtr++ = 4;	                           // size of _udp
	memcpy(mdnsResponsePtr, "_udp", 4);	               // _udp
	mdnsResponsePtr += 4;
	*mdnsResponsePtr++ = 5;	                           // size of local
	memcpy(mdnsResponsePtr, "local", 5);	           // local
	mdnsResponsePtr += 7;
	*mdnsResponsePtr = 0xc;	                           // PTR type
	mdnsResponsePtr += 2;
	*mdnsResponsePtr = 0x1;	                           // class IN
	mdnsResponsePtr += 3;
	*mdnsResponsePtr++ = 0x11;	                       // TTL = 4500 seconds
	*mdnsResponsePtr = 0x94;	                       // TTL = 4500 seconds
	mdnsResponsePtr += 2;
	*mdnsResponsePtr++ = 2;	                           // size of PTR
	*mdnsResponsePtr++ = 0xc0;
	*mdnsResponsePtr++ = 0x0c;	                       // points to rest of the domain

	// answer 3 - TXT record of the service
	*mdnsResponsePtr++ = 0xc0;
	*mdnsResponsePtr = 0x2f;	                       // points to device service name
	mdnsResponsePtr += 2;
	*mdnsResponsePtr++ = 0x10;	                       // TXT type
	*mdnsResponsePtr++ = 0x80;	                       // class UNICAST
	*mdnsResponsePtr = 0x1;	                           // class IN
	mdnsResponsePtr += 3;
	*mdnsResponsePtr++ = 0x11;	                       // TTL = 4500 seconds
	*mdnsResponsePtr = 0x94;	                       // TTL = 4500 seconds
	mdnsResponsePtr += 2;
	*mdnsResponsePtr++ = 36;	                       // size of TXT
	*mdnsResponsePtr++ = 10;	                       // size of dev=CC3000
	memcpy(mdnsResponsePtr, "dev=CC3000", 10);	       // _device-info
	mdnsResponsePtr += 10;
	*mdnsResponsePtr++ = 24;	                       // size of vendor=Texas-Instruments
	memcpy(mdnsResponsePtr, "vendor=Texas-Instruments", 24);	// _udp
	mdnsResponsePtr += 24;

	// answer 4 - SRV record of the service
	*mdnsResponsePtr++ = 0xc0;
	*mdnsResponsePtr = 0x2f;	                       // points to device service name
	mdnsResponsePtr += 2;
	*mdnsResponsePtr++ = 0x21;	                       // SRV type
	*mdnsResponsePtr++ = 0x80;	                       // class UNICAST
	*mdnsResponsePtr = 0x1;	                           // class IN
	mdnsResponsePtr += 3;
	*mdnsResponsePtr++ = 0x11;	                       // TTL = 4500 seconds
	*mdnsResponsePtr = 0x94;	                       // TTL = 4500 seconds
	mdnsResponsePtr += 2;

	//data length to be filled later in hook_sl_cmd_parser function
	mdnsResponsePtr += 5;
	*mdnsResponsePtr++ = 0x4;	                       // high portion of port 1234
	*mdnsResponsePtr++ = 0xd2;	                       // low portion of port 1234

	//size should be according to device_name (input parameter from API)
	mdnsResponsePtr += 1;                              //leave free slot for device_name length
	*mdnsResponsePtr++ = 0xc0;
	*mdnsResponsePtr++ = 0x1e;	                       // points to local

	// answer 5 - ADDRESS record of the service
	*mdnsResponsePtr++ = 0xc0;
	*mdnsResponsePtr =
	  (UINT16)(mdnsResponsePtr - mdnsResponse) - 4; //adding the required offset in hook_sl_cmd_parser function
	mdnsResponsePtr += 2;
	*mdnsResponsePtr++ = 0x1;	                       // Address type
	*mdnsResponsePtr++ = 0x80;	                       // class UNICAST
	*mdnsResponsePtr = 0x1;	                       // class IN
	mdnsResponsePtr += 3;
	*mdnsResponsePtr++ = 0x11;	                       // TTL = 4500 seconds
	*mdnsResponsePtr = 0x94;	                       // TTL = 4500 seconds
	mdnsResponsePtr += 2;
	*mdnsResponsePtr++ = 4;	                       // size of Address

	mdnsResponseLength = (UINT16)(mdnsResponsePtr - mdnsResponse);

	//
	// Move to the domain and its length
	//
	mdnsResponsePtr = &mdnsResponse[46];

	//
	// Domain length
	//
	*mdnsResponsePtr++ = 3 + device_name_len;

	//
	// Size of device service name
	//
	*mdnsResponsePtr++ = device_name_len;

	//
	// Now we need to insert the device service name here
	// (so push the rest accordingly).
	//
	memmove(mdnsResponsePtr + device_name_len,
			mdnsResponsePtr,
			mdnsResponseLength - 48);

	//
	// Device service name.
	//
	memcpy(mdnsResponsePtr, deviceServiceName, device_name_len);

	//
	// Start handling Host Domain Name (Type = 1)
	// DNS IE starts at constant offset: 62
	// First answer starts at constant offset: 74
	// Second answer starts at offset which depends on device_name length:
	//     112 + device_name length

	// Third answer starts at offset which depends on device_name length:
	//     154 + device_name length

	// Forth answer starts at offset which depends on device_name length:
	//     202 + device_name length

	// Fifth answer starts at offset which depends on device_name length
	//     +SRV target: 223 + device_name length * 2 */

	//
	// Fill SRV Data Length -> Fourth answer,
	// 10 bytes offset (Domain Name, Type, Class, TTL) + 1 byte (Fill LSB bits)
	// => (202 + device_name length + 11) - 62 (base offset) = 151 +
	// device_name length.
	//
	// Move to data length
	//
	mdnsResponsePtr = &mdnsResponse[151 + device_name_len];

	//
	//Data Length: Priority (2 bytes) + Weight (2 bytes) + Port (2 bytes) +
	//Target size (1 byte) + 2 bytes (PTR + Offset of .local) = 9 bytes
	//
	*mdnsResponsePtr = 9 + device_name_len;

	//
	//Fill SRV Target -> 7 bytes offset from Data Length.
	//Derived from: (Priority 2 bytes, Weight 2 bytes, Port 2 bytes)
	//
	mdnsResponsePtr =
	  &mdnsResponse[158 + device_name_len];// Move to the domain and its length
	*mdnsResponsePtr++ = device_name_len;  // Domain length

	/*now we need to insert the device service name here
	(so push the rest accordingly)*/
	memmove(mdnsResponsePtr + device_name_len,
			mdnsResponsePtr,
			mdnsResponseLength - 158);
	//
	// Device service name
	//
	memcpy(mdnsResponsePtr,
		   ((char *)deviceServiceName),
		   device_name_len);
	//
	// Move to the end of the packet
	//
	mdnsResponsePtr =
	  &mdnsResponse[mdnsResponseLength + device_name_len + device_name_len];

	//
	//End handling Host Domain Name (Type = 1)
	//
	*mdnsResponsePtr++ = localIP[3];
	*mdnsResponsePtr++ = localIP[2];
	*mdnsResponsePtr++ = localIP[1];
	*mdnsResponsePtr++ = localIP[0];

	//
	// Add the length of the device name to the ADDRESS record
	//
	*(mdnsResponsePtr - 15) += device_name_len;

	mdnsResponseLength = (UINT16)(mdnsResponsePtr - mdnsResponse);

	//
	// Send the mDNS response packet.
	//
	sendto(mdnsSocket,
		   mdnsResponse,
		   sizeof(mdnsResponse),
		   0,
		   (const sockaddr*)&tSocketAddr,
		   mdnsResponseLength);

    closesocket(mdnsSocket);
    mdnsSocket = 0xFFFFFFFF;

    return mdnsSocket;
}
#endif

//*****************************************************************************
//
//!  getmssvalue
//!
//!  @param[in] sd         socket descriptor
//!
//!  @return   On success, returns the MSS value of a TCP connection
//!
//!  @brief    Returns the MSS value of a TCP connection according to the socket descriptor
//
//*****************************************************************************
UINT16 getmssvalue (INT32 sd)
{
	UINT8 *ptr, *args;
	UINT16 ret;

	ptr = tSLInformation.pucTxCommandBuffer;
	args = (ptr + HEADERS_SIZE_CMD);

	// Fill in temporary command buffer
	args = UINT32_TO_STREAM(args, sd);

	// Initiate a HCI command
	hci_command_send(HCI_CMND_GETMSSVALUE, ptr, SOCKET_GET_MSS_VALUE_PARAMS_LEN);

	// Since we are in blocking state - wait for event complete
	SimpleLinkWaitEvent(HCI_EVNT_GETMSSVALUE, &ret);

	return ret;
}

