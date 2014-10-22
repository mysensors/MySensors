
/*
 * Touch Display Scene Controller build using a touch enabled tft display
 * attached to a ATMega 2560. This example fetches time from controller and sends
 * in scene commands to the controller. 
 * Short press sends V_SCENE_ON and long press (>0.5sec) sends V_SCENE_OFF.
 *
 * Henrik Ekblad <henrik.ekblad@mysensors.org>
 *
 * This example uses:
 * UTFT library: http://www.henningkarlsen.com/electronics/library.php?id=51
 * Convert your own images here: http://www.henningkarlsen.com/electronics/t_imageconverter565.php
 *
 * The 3v3 output on the ATMega2560 is especially bad. I had to use a step down 
 * on the 5V output to get solid transmissions.
 * 
 * The shield does not seem like sharing SPI with RF24 so you'll have to
 * activate SOFTSPI-define in MySenors/util/RF24_config.h
 *
 * Connect radio 
 * ----------------------------------
 * Radio               Arduino Pin
 * ----------------------------------
 * GND                 GND  
 * 5V  -> Step down -> 3V3   (see buying guide for help finding step-down)
 * SCK                 14 
 * MOSI                15 
 * MISO                16 
 * CE                  17   
 * CSN                 18
 *
 */

#include <DigitalIO.h>
#include <SPI.h>
#include <Time.h>  
#include <MySensor.h>  
#include <stdarg.h>
#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>
#include <avr/pgmspace.h>
#include "arial_bold.c"
#include "ArialNumFontPlus.c"
#include "logo.c"

#define CHILD_ID 1

int LONG_PRESS = 500; // Number of millisecons to trinnger scene off for button push
int RESEND_DEBOUNCE = 2000; // Number of millisecons interval between sending of same 
                            // scene number again. 
                            // This is used to avoid unwanted re-trigger when using 
                            // a sensetive touch diplay.

// Add your buttons here. Max is 5 if you still want time at the top.
char *buttons[] = {
    "Good Morning", 
    "Clean Up!", 
    "All Lights Off", 
    "Music On/Off"
  };
    
const int buttonCount = sizeof(buttons)/sizeof(char *);
const int padding = 10;
const int topBarHeight = 60;

MySensor gw(17,18);
MyMessage on(CHILD_ID, V_SCENE_ON);
MyMessage off(CHILD_ID, V_SCENE_OFF);

UTFT myGLCD(ITDB32S,38,39,40,41);
UTouch  myTouch( 6, 5, 4, 3, 2);
UTFT_Buttons  myButtons(&myGLCD, &myTouch);
boolean timeReceived = false;
unsigned long lastTimeUpdate=0, lastRequest=0;
char timeBuf[20];


void setup()  
{ 
  gw.begin(NULL, AUTO, false, 0);

  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setFont((uint8_t*)ArialNumFontPlus);
  myGLCD.setColor(100, 255, 100);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawBitmap (0, 0, 60, 60, (unsigned int*)logo);
    
  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  
  myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_BLUE);
  myButtons.setTextFont((uint8_t*)arial_bold);
  int height = (myGLCD.getDisplayYSize()-topBarHeight-(padding*(buttonCount+2)))/buttonCount;
  
  // Add buttons
  for (int i=0; i<buttonCount;i++) {
    int y = topBarHeight + padding+(i*(height+padding));
    myButtons.addButton( padding, y, myGLCD.getDisplayXSize()-padding*2, height, buttons[i]);
  }
  myButtons.drawButtons();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Scene Ctrl", "1.0");
  gw.present(CHILD_ID, S_SCENE_CONTROLLER);
  // Request time from controller. 
  gw.requestTime(receiveTime); 
}



int lastPressedButton = -1;
unsigned long lastPressedButtonTime = 0;

void loop()      
{ 
  gw.process();
  unsigned long now = millis();
  
  if (myTouch.dataAvailable()) {
    unsigned long startPress = millis();
    int pressedButton = myButtons.checkButtons();

    if (pressedButton>=0) {
      bool longPress = millis()-now>LONG_PRESS;
      
      if (pressedButton != lastPressedButton || now-lastPressedButtonTime > RESEND_DEBOUNCE) {
        if (longPress) {
          gw.send(off.set(pressedButton));
          Serial.print("Long pressed: ");
        } else {
          gw.send(on.set(pressedButton));
          Serial.print("Pressed: ");    
        }
        Serial.println(pressedButton);    
        lastPressedButton = pressedButton;    
        lastPressedButtonTime=now;  
      }
    }
  }

  // If no time has been received yet, request it every 10 second from controller
  // When time has been received, request update every hour
  if ((!timeReceived && now-lastRequest > 10*1000)
    || (timeReceived && now-lastRequest > 60*1000*60)) {
    // Request time from controller. 
    Serial.println("requesting time");
    gw.requestTime(receiveTime);  
    lastRequest = now;
  }

  // Update time every second
  if (timeReceived && now-lastTimeUpdate > 1000) {
    printTime();  
    lastTimeUpdate = now;
  }
}



// This is called when a new time value was received
void receiveTime(unsigned long time) {
  // Ok, set incoming time 
  setTime(time);
  timeReceived = true;
}

void printTime() {
  sprintf(timeBuf, "%02d:%02d:%02d", hour(), minute(), second());
  myGLCD.print(timeBuf, 60,7);
}


