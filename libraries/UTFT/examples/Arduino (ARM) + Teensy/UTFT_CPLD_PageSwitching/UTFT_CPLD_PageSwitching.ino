// UTFT_CPLD_PageSwitching (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program is a demo of page switching on CPLD-based display modules..
//
// This demo was made for modules with a screen resolution 
// of 800x480 pixels.
//
// This program requires the UTFT library.
//
// NOTE: The display will be black for a short while during the start
//

#include <UTFT.h>

// Declare which fonts we will be using
extern uint8_t BigFont[];
extern uint8_t SevenSegNumFont[];

// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Teensy 3.x TFT Test Board                   : <display model>,23,22, 3, 4
// ElecHouse TFT LCD/SD Shield for Arduino Due : <display model>,22,23,31,33
//
// Remember to change the model parameter to suit your display module!
UTFT myGLCD(EHOUSE50CPLD,22,23,31,33);

void setup()
{
  myGLCD.InitLCD();
}

void loop()
{
  myGLCD.setBackColor(VGA_TRANSPARENT);

  myGLCD.setBrightness(0);  
  for (byte pg=0; pg<8; pg++)
  {
    myGLCD.setWritePage(pg);
    myGLCD.clrScr();
    for (int ln=0; ln<480; ln+=2)
    {
      switch (pg)
      {
        case 0:
          myGLCD.setColor(ln/2, 0, 0);
          break;
        case 1:
          myGLCD.setColor(0, ln/2, 0);
          break;
        case 2:
          myGLCD.setColor(0, 0, ln/2);
          break;
        case 3:
          myGLCD.setColor(ln/4, ln/2, 0);
          break;
        case 4:
          myGLCD.setColor(0, ln/2, ln/2);
          break;
        case 5:
          myGLCD.setColor(ln/2, 0, ln/2);
          break;
        case 6:
          myGLCD.setColor(ln/2, ln/2, 0);
          break;
        case 7:
          myGLCD.setColor(0, ln/2, ln/4);
          break;
      }
      myGLCD.drawLine(0, ln, 799, ln);
      myGLCD.drawLine(0, ln+1, 799, ln+1);
    }
    myGLCD.setColor(VGA_WHITE);
    myGLCD.setFont(BigFont);
    myGLCD.print("This is page:", CENTER, 200);
    myGLCD.setFont(SevenSegNumFont);
    myGLCD.printNumI(pg, CENTER, 240);
  }
  myGLCD.setBrightness(16);  

  while(1)
  {
    for (byte pg=0; pg<8; pg++)
    {
      myGLCD.setDisplayPage(pg);
      delay(500);
    }
  }
}

