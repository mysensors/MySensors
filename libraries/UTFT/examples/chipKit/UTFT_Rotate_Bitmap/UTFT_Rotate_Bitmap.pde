// UTFT_Rotate_Bitmap (C)2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program is a demo of the drawBitmap()-function.
//
// This program requires the UTFT library.
//

#include <UTFT.h>

// Set the pins to the correct ones for your development shield
// ------------------------------------------------------------
// My chipKit Uno32/uC32 shield                : <display model>,38,39,40,41
// My chipKit Max32 shield                     : <display model>,82,83,84,85
// AquaLEDSource All in One Super Screw Shield : <display model>,82,83,84,85
//
// Remember to change the model parameter to suit your display module!
UTFT myGLCD(ITDB32S,82,83,84,85);

extern unsigned short biohazard[0x1000];

void setup()
{
  myGLCD.InitLCD(LANDSCAPE);
  myGLCD.fillScr(255, 255, 255);
  myGLCD.setColor(0, 0, 0);
}

void loop()
{
    for (int i=0; i<360; i+=5)
    {
      myGLCD.drawBitmap (10, 10, 64, 64, biohazard, i, 32, 32);
    }
}

