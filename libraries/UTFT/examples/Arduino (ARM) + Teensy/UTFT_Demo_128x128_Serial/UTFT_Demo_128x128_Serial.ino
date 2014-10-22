// UTFT_Demo_128x128_Serial (C)2013 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program is a demo of how to use most of the functions
// of the library with a supported display modules.
//
// This demo was made to work on the 128x128 modules.
// Any other size displays may cause strange behaviour.
//
// This program requires the UTFT library.
//

#include <UTFT.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];

// Declare an instance of the class
UTFT myGLCD(LPH9135,6,5,2,3,4);   // Remember to change the model parameter to suit your display module!

void setup()
{
  randomSeed(analogRead(0));
  
// Setup the LCD
  myGLCD.InitLCD(PORTRAIT);
  myGLCD.setFont(SmallFont);
}

void loop()
{
  byte buf[126];
  int x, x2;
  int y, y2;
  int r;
  
// Clear the screen and draw the frame
  myGLCD.clrScr();
  myGLCD.setContrast(64);

  myGLCD.setColor(255, 0, 0);
  myGLCD.fillRect(0,0,127,12);
  myGLCD.setColor(64, 64, 64);
  myGLCD.fillRect(0,117,127,127);
  myGLCD.setColor(255, 255, 255);
  myGLCD.setBackColor(255,0,0);
  myGLCD.print("Universal TFT", CENTER, 0);
  myGLCD.setBackColor(64,64,64);
  myGLCD.setColor(255,255,0);
  myGLCD.print("H.Karlsen", LEFT, 116);
  myGLCD.print("(C)2013", RIGHT, 116);
  myGLCD.setColor(0,255,0);
  myGLCD.drawRect(0,13,127,116);

// Draw crosshairs
  myGLCD.setColor(0,0,255);
  myGLCD.drawLine(63,14,63,115);
  myGLCD.drawLine(1,63,126,63);
  for (int i=3; i<128; i+=10)
    myGLCD.drawLine(i, 61, i, 65);
  for (int i=14; i<118; i+=10)
    myGLCD.drawLine(61, i, 65, i);
  
// Draw sin-, cos- and tan-lines  
  myGLCD.setColor(0,255,255);
  myGLCD.setBackColor(0,0,0);
  myGLCD.print("Sin", 2, 14);
  for (int i=1; i<126; i++)
  {
    myGLCD.drawPixel(i,63+(sin(((i*2.85)*3.14)/180)*45));
  }
  
  myGLCD.setColor(255,0,0);
  myGLCD.print("Cos", 2, 26);
  for (int i=1; i<126; i++)
  {
    myGLCD.drawPixel(i,63+(cos(((i*2.85)*3.14)/180)*45));
  }

  myGLCD.setColor(255,255,0);
  myGLCD.print("Tan", 2, 38);
  for (int i=1; i<126; i++)
  {
    myGLCD.drawPixel(i,63+(tan(((i*2.85)*3.14)/180)));
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);
  myGLCD.setColor(0,0,255);
  myGLCD.drawLine(63,14,63,115);
  myGLCD.drawLine(1,63,126,63);

// Draw a moving sinewave
  x=1;
  for (int i=1; i<3654; i++)
  {
    x++;
    if (x==127)
      x=1;
    if (i>127)
    {
      if ((x==63)||(buf[x-1]==63))
        myGLCD.setColor(0,0,255);
      else
        myGLCD.setColor(0,0,0);
      myGLCD.drawPixel(x,buf[x-1]);
    }
    myGLCD.setColor(0,255,255);
    y=63+(sin(((i*1.3)*3.14)/180)*45);
    myGLCD.drawPixel(x,y);
    buf[x-1]=y;
    delay(1);
  }

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);
  
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
    myGLCD.fillRect(10+(i*10),10+(i*10), 60+(i*10), 60+(i*10));
  }
  
  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);
  
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
    myGLCD.fillRoundRect(70-(i*10),10+(i*10), 120-(i*10), 60+(i*10));
  }
  
  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);
  
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
    myGLCD.fillCircle(30+(i*10),35+(i*10), 25);
  }
  
  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);

  // Draw some lines in a pattern
  myGLCD.setColor (255,0,0);
  for (int i=11; i<115; i+=3)
  {
    myGLCD.drawLine(1, i, i-10, 115);
  }
  myGLCD.setColor (255,0,0);
  for (int i=112; i>14; i-=3)
  {
    myGLCD.drawLine(126, i, i+14, 14);
  }
  myGLCD.setColor (0,255,255);
  for (int i=115; i>14; i-=3)
  {
    myGLCD.drawLine(1, i, 116-i, 14);
  }
  myGLCD.setColor (0,255,255);
  for (int i=14; i<115; i+=3)
  {
    myGLCD.drawLine(126, i, 140-i, 115);
  }
  
  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);

// Draw some random circles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=22+random(85);
    y=35+random(59);
    r=random(20);
    myGLCD.drawCircle(x, y, r);
  }

  delay(2000);
  
  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);

// Draw some random rectangles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(124);
    y=15+random(101);
    x2=2+random(124);
    y2=15+random(101);
    myGLCD.drawRect(x, y, x2, y2);
  }
  
  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);

// Draw some random rounded rectangles
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(124);
    y=15+random(101);
    x2=2+random(124);
    y2=15+random(101);
    myGLCD.drawRoundRect(x, y, x2, y2);
  }
  
  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);

// Draw some random lines
  for (int i=0; i<100; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    x=2+random(124);
    y=15+random(101);
    x2=2+random(124);
    y2=15+random(101);
    myGLCD.drawLine(x, y, x2, y2);
  }
  
  delay(2000);

  myGLCD.setColor(0,0,0);
  myGLCD.fillRect(1,14,126,115);

// Draw some random pixels
  for (int i=0; i<5000; i++)
  {
    myGLCD.setColor(random(255), random(255), random(255));
    myGLCD.drawPixel(2+random(124), 15+random(101));
  }
  
  delay (2000);
  
// Set up the "Finished"-screen
  myGLCD.setContrast(0);
  myGLCD.fillScr(0,0,255);
  myGLCD.setColor(255,0,0);
  myGLCD.fillRoundRect(2, 40, 125, 88);
  
  myGLCD.setColor(255,255,255);
  myGLCD.setBackColor(255,0,0);
  myGLCD.print("That's it!", CENTER, 46);
  myGLCD.print("Restarting in a", CENTER, 66);
  myGLCD.print("few seconds...", CENTER, 76);
  
  myGLCD.setColor(0,0,0);
  myGLCD.setBackColor(0,0,255);
  myGLCD.print("Runtime: (msecs)", CENTER, 108);
  myGLCD.printNumI(millis(), CENTER, 118);
  
  myGLCD.setContrast(64);
  
  delay (10000);
  
// Fade screen to black
  for (int i=64; i>0; i--)
  {
    myGLCD.setContrast(i);
    delay(100);
  }
}


