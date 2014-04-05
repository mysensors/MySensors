/* IRLibRData.h from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.4   March 2014
 * Copyright 2014 by Chris Young http://cyborg5.com
 *
 * This library is a major rewrite of IRemote by Ken Shirriff which was covered by
 * GNU LESSER GENERAL PUBLIC LICENSE which as I read it allows me to make modified versions.
 * That same license applies to this modified version. See his original copyright below.
 * The latest Ken Shirriff code can be found at https://github.com/shirriff/Arduino-IRremote
 * My purpose was to reorganize the code to make it easier to add or remove protocols.
 * As a result I have separated the act of receiving a set of raw timing codes from the act of decoding them
 * by making them separate classes. That way the receiving aspect can be more black box and implementers
 * of decoders and senders can just deal with the decoding of protocols. It also allows for alternative
 * types of receivers independent of the decoding. This makes porting to different hardware platforms easier.
 * Also added provisions to make the classes base classes that could be extended with new protocols
 * which would not require recompiling of the original library nor understanding of its detailed contents.
 * Some of the changes were made to reduce code size such as unnecessary use of long versus bool.
 * Some changes were just my weird programming style. Also extended debugging information added.
 */
/*
 * IRremote
 * Version 0.1 July, 2009
 * Copyright 2009 Ken Shirriff
 * For details, see http://www.righto.com/2009/08/multi-protocol-infrared-remote-library.html http://www.righto.com/
 *
 * Interrupt code based on NECIRrcv by Joe Knapp
 * http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1210243556
 * Also influenced by http://zovirl.com/2008/11/12/building-a-universal-remote-with-an-arduino/
 */

#ifndef IRLibRData_h
#define IRLibRData_h

/*
 * The structure contains a variety of variables needed by the receiver routines.
 * Typically this data would be part of the IRrecv class however the interrupt service routine
 * must have access to it and you cannot pass a parameter to such a routine. The data must be global.
 * You need not include this file unless you are creating a custom receiver class or extending
 * the provided IRrecv class.
 */

// receiver states
enum rcvstate_t {STATE_UNKNOWN, STATE_IDLE, STATE_MARK, STATE_SPACE, STATE_STOP, STATE_RUNNING};
// information for the interrupt handler
typedef struct {
  unsigned char recvpin;    // pin for IR data from detector
  rcvstate_t rcvstate;       // state machine
  bool blinkflag;         // TRUE to enable blinking of pin 13 on IR processing
  unsigned long timer;     // state timer, counts 50uS ticks.(and other uses)
  unsigned int rawbuf[RAWBUF]; // raw data
  unsigned char rawlen;         // counter of entries in rawbuf
} 
irparams_t;
extern volatile irparams_t irparams;
#endif
