// UTFT_Demo_220x176_Serial (C)2012 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program is a demo of how to use most of the functions
// of the library with a supported display modules.
//
// This demo was made for serial modules with a screen resolution 
// of 220x176 pixels.
//
// This program requires the UTFT library.
//

#include <UTFT.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];

UTFT myGLCD(ITDB22SP,11,10,9,12);   // Remember to change the model parameter to suit your display module!

void setup()
{
  randomSeed(analogRead(0));
  
// Setup the LCD
  myGLCD.InitLCD();
  myGLCD.setFont(SmallFont);
}

void loop()
{
  int buf[218];
  int x, x2;
  int y, y2;
  int r;

// Clear the screen and draw the frame
  myGLCD.clrScr();

  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0, 0, 219, 13);
  myGLCD.setColor(64, 64, 64);
  myGLCD.fillRect(0, 162, 219, 175);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("** Universal TFT Library **", CENTER, 1);
  myGLCD.setBackColor(64, 64, 64);
  myGLCD.setColor(255,255,0);
  myGLCD.print("> elec.henningkarlsen.com <", CENTER, 163);

  myGLCD.setColor(0, 0, 255);
  myGLCD.drawRect(0, 14, 219, 161);

// Draw crosshairs
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawLine(109, 15, 109, 160);
  myGLCD.drawLine(1, 88, 218, 88);

  for (int i=9; i<210; i+=10)
    myGLCD.drawLine(i, 86, i, 90);
  for (int i=19; i<155; i+=10)
    myGLCD.drawLine(107, i, 111, i);

// Draw sin-, cos- and tan-lines  
  myGLCD.setColor(0,255,255);
  myGLCD.print("Sin", 5, 15);
  for (int i=1; i<218; i++)
  {
    myGLCD.drawPixel(i,88+(sin(((i*1.65)*3.14)/180)*70));
  }
  
  myGLCD.setColor(255,0,0);
  myGLCD.print("Cos", 5, 27);
  for (int i=1; i<218; i++)
  {
    myGLCD.drawPixel(i,88+(cos(((i*1.65)*3.14)/180)*70));
  }

  myGLCD.setColor(255,255,0);
  myGLCD.print("Tan", 5, 39);
  for (int i=1; i<218; i++)
  {
    myGLCD.drawPixel(i,88+(tan(((i*1.65)*3.14)/180)));
  }

  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);
  myGLCD.setColor(0, 0, 255);
  myGLCD.setBackColor(0, 0, 0);
  myGLCD.drawLine(109, 15, 109, 160);
  myGLCD.drawLine(1, 88, 218, 88);

// Draw a moving sinewave
  x=1;
  for (int i=1; i<(218*20); i++) 
  {
    x++;
    if (x==219)
      x=1;
    if (i>219)
    {
      if ((x==109)||(buf[x-1]==88))
        myGLCD.setColor(0,0,255);
      else
        myGLCD.setColor(0,0,0);
      myGLCD.drawPixel(x,buf[x-1]);
    }
    myGLCD.setColor(0,255,255);
    y=88+(sin(((i*1.6)*3.14)/180)*(65-(i / 100)));
    myGLCD.drawPixel(x,y);
    buf[x-1]=y;
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

// Draw some filled rectangles
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillRect(44+(i*15), 23+(i*15), 88+(i*15), 63+(i*15));
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

// Draw some filled, rounded rectangles
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillRoundRect(132-(i*15), 23+(i*15), 172-(i*15), 63+(i*15));
  }
  
  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

// Draw some filled circles
  for (int i=1; i<6; i++)
  {
    switch (i)
    {
      case 1:
        myGLCD.setColor(255,0,255);
        break;
      case 2:
        myGLCD.setColor(255,0,0);
        break;
      case 3:
        myGLCD.setColor(0,255,0);
        break;
      case 4:
        myGLCD.setColor(0,0,255);
        break;
      case 5:
        myGLCD.setColor(255,255,0);
        break;
    }
    myGLCD.fillCircle(64+(i*15),43+(i*15), 20);
  }
  
  delay(2000);
    
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

// Draw some lines in a pattern
  myGLCD.setColor (255,0,0);
  for (int i=15; i<160; i+=5)
  {
    myGLCD.drawLine(1, i, (i*1.44)-10, 160);
  }
  myGLCD.setColor (255,0,0);
  for (int i=160; i>15; i-=5)
  {
    myGLCD.drawLine(218, i, (i*1.44)-12, 15);
  }
  myGLCD.setColor (0,255,255);
  for (int i=160; i>15; i-=5)
  {
    myGLCD.drawLine(1, i, 232-(i*1.44), 15);
  }
  myGLCD.setColor (0,255,255);
  for (int i=15; i<160; i+=5)
  {
    myGLCD.drawLine(218, i, 231-(i*1.44), 160);
  }
  
  delay(2000);
  
    myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,161);

// Draw some random circles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=22+random(176);
    y=35+random(105);
    r=random(20);
    myGLCD.drawCircle(x, y, r);
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

// Draw some random rectangles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(216);
    y=16+random(143);
    x2=2+random(216);
    y2=16+random(143);
    myGLCD.drawRect(x, y, x2, y2);
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

// Draw some random rounded rectangles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(216);
    y=16+random(143);
    x2=2+random(216);
    y2=16+random(143);
    myGLCD.drawRoundRect(x, y, x2, y2);
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(216);
    y=16+random(143);
    x2=2+random(216);
    y2=16+random(143);
    myGLCD.drawLine(x, y, x2, y2);
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,15,218,160);

  for (int i=0; i<10000; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    myGLCD.drawPixel(2+random(216), 16+random(143));
  }

  delay(2000);

  myGLCD.fillScr(0, 0, 255);
  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRoundRect(40, 57, 179, 119);
  
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255, 0, 0);
  myGLCD.print("That's it!", CENTER, 62);
  myGLCD.print("Restarting in a", CENTER, 88);
  myGLCD.print("few seconds...", CENTER, 101);
  
  myGLCD.setColor(0, 255, 0);
  myGLCD.setBackColor(0, 0, 255);
  myGLCD.print("Runtime: (msecs)", CENTER, 146);
  myGLCD.printNumI(millis(), CENTER, 161);

  delay (10000);
}

