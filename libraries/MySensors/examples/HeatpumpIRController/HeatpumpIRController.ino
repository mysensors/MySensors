/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Toni A - https://github.com/ToniA/arduino-heatpumpir
 *
 * DESCRIPTION
 * Heatpump controller
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// HeatpumpIR libraries,
#include <FujitsuHeatpumpIR.h>
#include <PanasonicCKPHeatpumpIR.h>
#include <PanasonicHeatpumpIR.h>
#include <CarrierHeatpumpIR.h>
#include <MideaHeatpumpIR.h>
#include <MitsubishiHeatpumpIR.h>
#include <SamsungHeatpumpIR.h>
#include <SharpHeatpumpIR.h>
#include <DaikinHeatpumpIR.h>

// Timer library, https://github.com/JChristensen/Timer
#include <Timer.h>

// MySensors libraries
#include <SPI.h>
#include <MySensors.h>

// Child ID's of this node
#define CHILD_1  1

// MySensors definitions
MyMessage irMsg(CHILD_1, V_IR_RECEIVE);

// -- Hacks for Domoticz start
#define CHILD_2  2
#define CHILD_3  3
// Domoticz does not yet properly support the S_IR sensor type
MyMessage textMsg(CHILD_2, V_TEXT);
MyMessage sendMsg(CHILD_3, V_LIGHT);
// -- Hacks for Domoticz end

// Array with all supported heatpump models
HeatpumpIR *heatpumpIR[] = { new PanasonicCKPHeatpumpIR(), // 0, keep this if you don't remove the timer for cancelling Panasonic CKP messages
                             new PanasonicDKEHeatpumpIR(), // 1
                             new PanasonicJKEHeatpumpIR(), // 2
                             new PanasonicNKEHeatpumpIR(), // 3
                             new CarrierHeatpumpIR(),      // 4
                             new MideaHeatpumpIR(),        // 5
                             new FujitsuHeatpumpIR(),      // 6
                             new MitsubishiFDHeatpumpIR(), // 7
                             new MitsubishiFEHeatpumpIR(), // 8
                             new SamsungHeatpumpIR(),      // 9
                             new SharpHeatpumpIR(),        // 10
                             new DaikinHeatpumpIR()        // 11
                           };

// IR led on PWM output-capable digital pin 3
IRSender irSender(3);

// Timer for sending Panasonic CKP series timer cancellation commands
Timer timer;
int8_t panasonicCKPTimer = 0;

// Number of supported models
byte models = 0;


void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("HeatpumpIR sensor starting up..."));

  models = sizeof(heatpumpIR) / sizeof(HeatpumpIR*);
  Serial.print(F("Number of supported models: ")); Serial.println(models);

  // The TEXT sensor is not created in Domoticz before data is sent
  send(textMsg.setSensor(CHILD_2).set("00000000"));
  // -- Hacks for Domoticz end
}

void presentation() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Heatpump Sensor", "1.0");

  // Register a sensors to the MySensors Gateway
  present(CHILD_1, S_IR, "IR sender");

  // -- Hacks for Domoticz start
  present(CHILD_2, S_INFO, "IR data");
  present(CHILD_3, S_LIGHT, "IR send");
}

void loop()
{
  timer.update();
}


// Handle incoming messages from the MySensors Gateway
void receive(const MyMessage &message) {

    const char *irData;

  // V_IR type message
  if (message.type==V_IR_SEND) {
    Serial.println(F("Received IR send command..."));
    irData = message.getString();
    Serial.print(F("Code: 0x"));
    Serial.println(irData);
    sendHeatpumpIRCommand(irData);
  }
  // -- Hacks for Domoticz start
  else if (message.type==V_LIGHT) {
    // When the button is pressed on Domoticz, request the value of the TEXT sensor
    Serial.println(F("Requesting IR code from Domoticz..."));
    request(CHILD_2, V_TEXT, 0);
  } else if (message.type==V_TEXT) {
    // TEXT sensor value is received as a result of the previous step
    Serial.println(F("IR code received from Domoticz..."));
    Serial.print(F("Code: 0x"));

    irData = message.getString();
    Serial.println(irData);

    sendHeatpumpIRCommand(irData);

    // Set the Domoticz switch back to 'OFF' state
    send(sendMsg.setSensor(CHILD_3).set(0));
  }
  // -- Hacks for Domoticz end
}


// Decode the IR command and send the IR command to the heatpump
void sendHeatpumpIRCommand(const char *irCommandString) {

  // irCommandString is an 8-digit hex digit
  long irCommand = 0;
  int sscanfStatus = sscanf(irCommandString, "%lx", &irCommand);

  if (sscanfStatus == 1) {
    Serial.print(F("IR code conversion OK: 0x"));
    Serial.println(irCommand, HEX);
  } else {
    Serial.println(F("Failed to convert IR hex code to number"));
  }

/*
The heatpump command is packed into a 32-bit hex number, see
libraries\HeatpumpIR\HeatpumpIR.h for the constants

12345678
  3 MODEL
   4 POWER
    5 OPERATING MODE
     6 FAN SPEED
      78 TEMPERATURE IN HEX

00213416 (as an example of a valid code)
  2 = PanasonicJKE
   1 = Power ON
    3 = COOL
     4 = FAN 4
      16 = Temperature 22 degrees (0x16 = 22)
 */

  byte model = (irCommand & 0x00F00000) >> 20;
  byte power = (irCommand & 0x00010000) >> 16;
  byte mode  = (irCommand & 0x0000F000) >> 12;
  byte fan   = (irCommand & 0x00000F00) >> 8;
  byte temp  = (irCommand & 0x000000FF);

  const char* buf;
  Serial.print(F("Model: "));

  buf = heatpumpIR[model]->model();
  // 'model' is a PROGMEM pointer, so need to write a byte at a time
  while (char modelChar = pgm_read_byte(buf++))
  {
    Serial.print(modelChar);
  }
  Serial.println();

  Serial.print(F("Model #: ")); Serial.println(model);
  Serial.print(F("Power: ")); Serial.println(power);
  Serial.print(F("Mode: ")); Serial.println(mode);
  Serial.print(F("Fan: ")); Serial.println(fan);
  Serial.print(F("Temp: ")); Serial.println(temp);

  // Heatpump models start from 0, i.e. model number is always less than the number of different models
  if (model < models) {
    // This is a supported model

    // Cancel the timer on Panasonic CKP heatpump
    if (model == 0) {
      Serial.println(F("Cancelling timer on Panasonic CKP heatpump..."));
      timer.stop(panasonicCKPTimer);
    }

    Serial.println(F("All OK - sending IR command to heatpump..."));
    heatpumpIR[model]->send(irSender, power, mode, fan, temp, VDIR_UP, HDIR_AUTO);

    if (model == 0) {
      Serial.println(F("Scheduling timer cancellation on Panasonic CKP heatpump..."));
      panasonicCKPTimer = timer.after(120000, panasonicCancelTimer); // Called after 2 minutes
    }
  }
}


// Cancel the timer on the Panasonic CKP heatpump
void panasonicCancelTimer()
{
  PanasonicCKPHeatpumpIR *panasonicCKPHeatpumpIR = new PanasonicCKPHeatpumpIR();

  panasonicCKPHeatpumpIR->sendPanasonicCKPCancelTimer(irSender);
  Serial.println(F("The TIMER led should now be OFF"));
}
