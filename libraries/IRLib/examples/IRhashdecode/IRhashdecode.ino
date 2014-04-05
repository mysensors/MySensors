/* Example program for from IRLib – an Arduino library for infrared encoding and decoding
 * Version 1.3   January 2014
 * Copyright 2014 by Chris Young http://cyborg5.com
 * Based on original example sketch for IRremote library 
 * Version 0.11 September, 2009
 * Copyright 2009 Ken Shirriff
 * http://www.righto.com/
 */
/*
 * IRhashdecode - decode an arbitrary IR code.
 * Instead of decoding using a standard encoding scheme
 * (e.g. Sony, NEC, RC5), the code is hashed to a 32-bit value.
 * This should produce a unique 32-bit number however that number cannot be used
 * to retransmit the same code. This is just a quick and dirty way to detect a unique code
 * for controlling a device when you don't really care what protocol or values
 * are being sent.
 */

#include <IRLib.h>

int RECV_PIN = 11;

IRrecv My_Receiver(RECV_PIN);
IRdecode My_Decoder;
IRdecodeHash My_Hash_Decoder;

void setup()
{
  My_Receiver.enableIRIn(); // Start the receiver
  Serial.begin(9600);
  delay(2000);while(!Serial);//delay for Leonardo
}

void loop() {
  if (My_Receiver.GetResults(&My_Decoder)) {//Puts results in My_Decoder
    My_Hash_Decoder.copyBuf(&My_Decoder);//copy the results to the hash decoder
    My_Decoder.decode();
    Serial.print("real decode type:");
    Serial.print(Pnames(My_Decoder.decode_type));
    Serial.print(" value: 0x");
    Serial.print(My_Decoder.value, HEX);
    My_Hash_Decoder.decode();
    Serial.print(", hash decode: 0x");
    Serial.println(My_Hash_Decoder.hash, HEX); // Do something interesting with this value
    delay(1000);
    My_Receiver.resume(); 
  }
}

