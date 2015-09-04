// UTFT_Bitmap_128x128 (C)2012 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program is a demo of the drawBitmap()-function.
//
// This demo was made to work on the 128x128 modules.
// Any other size displays may cause strange behaviour.
//
// This program requires the UTFT library.
//

#include <UTFT.h>

UTFT myGLCD(LPH9135,6,5,2,3,4);   // Remember to change the model parameter to suit your display module!

extern unsigned short icon1[0x400];
extern unsigned short icon2[0x400];
extern unsigned short tux[0x1000];

void setup()
{
  myGLCD.InitLCD(PORTRAIT);
}

void loop()
{
// Draw a 4 by 4 grid of a 32x32 icon.
  myGLCD.fillScr(255, 255, 255);
  for (int x=0; x<4; x++)
    for (int y=0; y<4; y++)
      myGLCD.drawBitmap (x*32, y*32, 32, 32, icon1);

  delay(5000);
  
// Draw a 64x64 icon in double size.
  myGLCD.fillScr(255, 255, 255);
  myGLCD.drawBitmap (0, 0, 64, 64, tux, 2);

  delay(5000);

// Draw a 2 by 2 grid of a 32x32 icon in double size.
  myGLCD.fillScr(255, 255, 255);
  for (int x=0; x<2; x++)
    for (int y=0; y<2; y++)
      myGLCD.drawBitmap (x*64, y*64, 32, 32, icon2, 2);

  delay(5000);
}

