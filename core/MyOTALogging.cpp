/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
#include "MyOTALogging.h"

#ifdef MY_OTA_LOG_SENDER_FEATURE
// global variables
static bool inOTALog = false;

void OTALog(uint8_t logNode, bool enableAck, const char *fmt, ... )
{
	// Avoid recursion
	if (inOTALog==true) {
		return;
	}
	inOTALog = true;

	MyMessage msg(NODE_SENSOR_ID, I_LOG_MESSAGE);
	char fmtBuffer[MY_SERIAL_OUTPUT_SIZE];
	va_list args;

	// create message
	va_start (args, fmt );
	int n = vsnprintf_P(fmtBuffer, sizeof(fmtBuffer), fmt, args);
	va_end (args);

	// Check number of chars
	if (n<1) {
		// Nothing to send
		inOTALog = false;
		return;
	}

	// Add \n to the end of string
	if (n>(int)(sizeof(fmtBuffer)-2)) {
		// String is truncated
		n = sizeof(fmtBuffer)-2;
	}

	// add LF if not set
	if (fmtBuffer[n-1]!='\n') {
		fmtBuffer[n++]='\n';
		fmtBuffer[n++]=0;
	}

	// Configure message
	msg.sender = getNodeId();
	msg.setDestination(logNode);
	mSetCommand(msg, C_INTERNAL);
	mSetRequestAck(msg, enableAck);

	// Send package
	for (int pos = 0; pos<n; pos+=MAX_PAYLOAD) {
		uint8_t length = strlen(&fmtBuffer[pos]);
		if (length>MAX_PAYLOAD) {
			length = MAX_PAYLOAD;
		}
		(void)_sendRoute(msg.set((char*)&fmtBuffer[pos]));
	}
	inOTALog = false;
}
#endif


#ifdef MY_OTA_LOG_RECEIVER_FEATURE
// Global variables
char OTALogfmtBuffer[MY_SERIAL_OUTPUT_SIZE];
int OTALogfmtBufferPos = 0;
uint8_t OTALogBufferNode = BROADCAST_ADDRESS;
uint8_t OTALogBufferSensor = 0;

void OTALogPrintPrefix()
{
	char prefix[37];
	// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
	snprintf_P(prefix, sizeof(prefix),
	           PSTR("%" PRId8 ";%" PRId8 ";%" PRId8 ";0;%" PRId8 ";%" PRIu32 " "),
	           OTALogBufferNode, OTALogBufferSensor, C_INTERNAL, I_LOG_MESSAGE, hwMillis());
	MY_SERIALDEVICE.print(prefix);
}

void OTALogFlushBuffer()
{
	OTALogfmtBuffer[0] = 0;
	OTALogfmtBufferPos = 0;
	OTALogBufferNode = BROADCAST_ADDRESS;
}

inline void OTALogPrint(const MyMessage &message)
{
	// Ignore log messages via broadcast
	if (message.destination == BROADCAST_ADDRESS) {
		return;
	}

	// FLush buffer, when node id changes
	if ((OTALogBufferNode!=BROADCAST_ADDRESS) && ((OTALogBufferNode != message.sender) ||
	        (OTALogBufferSensor != message.sensor))) {
		OTALogPrintPrefix();
		MY_SERIALDEVICE.print(OTALogfmtBuffer);
		MY_SERIALDEVICE.println("...");
		OTALogFlushBuffer();
	}

	// Add data to buffer
	const char *str = message.getString();
	strncpy(&OTALogfmtBuffer[OTALogfmtBufferPos], str,
	        sizeof(OTALogfmtBuffer)-OTALogfmtBufferPos);
	OTALogfmtBufferPos += strlen(str);

	// Store node ID and sensor ID
	OTALogBufferNode = message.sender;
	OTALogBufferSensor = message.sensor;

	// Print out buffered lines ending with \n
	char *EOLpos;
	while (EOLpos = strchr(OTALogfmtBuffer,'\n'), EOLpos != NULL) {
		// Add end of string
		EOLpos[0]=0;
		// Print out line
		OTALogPrintPrefix();
		MY_SERIALDEVICE.println(OTALogfmtBuffer);
		// Check if more content in buffer
		int lenAfterEOL = (size_t)&OTALogfmtBuffer[OTALogfmtBufferPos]-(size_t)EOLpos-2;
		if (lenAfterEOL>0) {
			// More lines, move string to the beginning of the buffer
			strcpy((char*)&OTALogfmtBuffer[0], (char*)&EOLpos[1]);
			// calculate OTALogfmtBufferPos
			OTALogfmtBufferPos -= (size_t)EOLpos-(size_t)&OTALogfmtBuffer[0]+2;
			// Security check
			if ((OTALogfmtBufferPos<=0) || (OTALogfmtBufferPos>=(int)sizeof(OTALogfmtBuffer))) {
				MY_SERIALDEVICE.print("Sec:");
				MY_SERIALDEVICE.println(OTALogfmtBufferPos);
				OTALogFlushBuffer();
			}
		} else {
			// End of message, prepare new one
			OTALogFlushBuffer();
		}
	}
}
#endif
