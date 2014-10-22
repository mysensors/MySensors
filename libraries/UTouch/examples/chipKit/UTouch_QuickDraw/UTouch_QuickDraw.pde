// UTouch_QuickDraw (C)2010-2012 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program is a quick demo of how to use the library.
//
// This program requires the UTFT library.
//
// It is assumed that the display module is connected to an
// appropriate shield or that you know how to change the pin 
// numbers in the setup.
//

#include <UTFT.h>
#include <UTouch.h>

// Uncomment the next line for chipKit Uno32
//UTFT        myGLCD(ITDB24D,34,35,36,37);   // Remember to change the model parameter to suit your display module!
//UTouch      myTouch(20,21,22,23,24);

// Uncomment the next line for chipKit Max32
UTFT        myGLCD(ITDB32S,82,83,84,85);   // Remember to change the model parameter to suit your display module!
UTouch      myTouch(62,63,64,65,66);

void setup()
{
  myGLCD.InitLCD();
  myGLCD.clrScr();

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
}

void loop()
{
  long x, y;
  
  while (myTouch.dataAvailable() == true)
  {
    myTouch.read();
    x = myTouch.getX();
    y = myTouch.getY();
    if ((x!=-1) and (y!=-1))
      myGLCD.drawPixel (x, y);
  }
}

