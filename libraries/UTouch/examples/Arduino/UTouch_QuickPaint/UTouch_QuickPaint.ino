// UTouch_QuickPaint (C)2013-2014 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// This program is a quick demo of how to use the library.
//
// This program requires the UTFT library and a display
// module with at least 320x240 pixels resolution.
//
// It is assumed that the display module is connected to an
// appropriate shield or that you know how to change the pin 
// numbers in the setup.
//

#include <UTFT.h>
#include <UTouch.h>

// Initialize display
// ------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : <display model>,19,18,17,16
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Teensy 3.x TFT Test Board                   : <display model>,23,22, 3, 4
// ElecHouse TFT LCD/SD Shield for Arduino Due : <display model>,22,23,31,33
//
// Remember to change the model parameter to suit your display module!
UTFT    myGLCD(ITDB32S,38,39,40,41);

// Initialize touchscreen
// ----------------------
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino Uno/2009 Shield            : 15,10,14, 9, 8
// Standard Arduino Mega/Due shield            :  6, 5, 4, 3, 2
// CTE TFT LCD/SD Shield for Arduino Due       :  6, 5, 4, 3, 2
// Teensy 3.x TFT Test Board                   : 26,31,27,28,29
// ElecHouse TFT LCD/SD Shield for Arduino Due : 25,26,27,29,30
//
UTouch  myTouch( 6, 5, 4, 3, 2);

// Declare which fonts we will be using
extern uint8_t BigFont[];

int color = 0;
word colorlist[] = {VGA_WHITE, VGA_BLACK, VGA_RED, VGA_BLUE, VGA_GREEN, VGA_FUCHSIA, VGA_YELLOW, VGA_AQUA};
int  bsize = 4;

void drawColorMarkerAndBrushSize(int col)
{
  myGLCD.setColor(VGA_BLACK);
  myGLCD.fillRect(25, 0, 31, 239);
  myGLCD.fillRect(myGLCD.getDisplayXSize()-31, 161, myGLCD.getDisplayXSize()-1, 191);
  myGLCD.setColor(VGA_WHITE);
  myGLCD.drawPixel(25, (col*30)+15);
  for (int i=1; i<7; i++)
    myGLCD.drawLine(25+i, ((col*30)+15)-i, 25+i, ((col*30)+15)+i);
  
  if (color==1)
    myGLCD.setColor(VGA_WHITE);
  else
    myGLCD.setColor(colorlist[col]);
  if (bsize==1)
    myGLCD.drawPixel(myGLCD.getDisplayXSize()-15, 177);
  else
    myGLCD.fillCircle(myGLCD.getDisplayXSize()-15, 177, bsize);
    
  myGLCD.setColor(colorlist[col]);
}

void setup()
{
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setFont(BigFont);

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_HI);
  
  myGLCD.setColor(VGA_WHITE);
  myGLCD.drawLine(32,0,32,myGLCD.getDisplayYSize()-1);
  myGLCD.drawLine(myGLCD.getDisplayXSize()-32,0,myGLCD.getDisplayXSize()-32,myGLCD.getDisplayYSize()-1);
  myGLCD.print("C", myGLCD.getDisplayXSize()-24, 8);
  myGLCD.print("L", myGLCD.getDisplayXSize()-24, 24);
  myGLCD.print("E", myGLCD.getDisplayXSize()-24, 40);
  myGLCD.print("A", myGLCD.getDisplayXSize()-24, 56);
  myGLCD.print("R", myGLCD.getDisplayXSize()-24, 72);
  myGLCD.print("+", myGLCD.getDisplayXSize()-24, 136);
  myGLCD.print("-", myGLCD.getDisplayXSize()-24, 200);
  myGLCD.fillRect(myGLCD.getDisplayXSize()-32,96,myGLCD.getDisplayXSize()-1,128);
  myGLCD.drawLine(myGLCD.getDisplayXSize()-32,160,myGLCD.getDisplayXSize()-1,160);
  myGLCD.drawLine(myGLCD.getDisplayXSize()-32,192,myGLCD.getDisplayXSize()-1,192);
  myGLCD.drawLine(myGLCD.getDisplayXSize()-32,224,myGLCD.getDisplayXSize()-1,224);
  for (int i=0; i<8; i++)
  {
    myGLCD.setColor(colorlist[i]);
    myGLCD.fillRect(0, (i*30), 24, (((i+1)*30)-1));
  }
  drawColorMarkerAndBrushSize(color);
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
    {
      if (x>(31+bsize) and (x<myGLCD.getDisplayXSize()-(31+bsize)))
      {
        if (bsize==1)
          myGLCD.drawPixel(x, y);
        else
          myGLCD.fillCircle(x, y, bsize);
      }
      else
      {
        if (x<(30+bsize))
        {
          if (y<240)
          {
            color = y / 30;
            drawColorMarkerAndBrushSize(color);
            while (myTouch.dataAvailable()) {};
            delay(50);
         }
        }
        else
        {
          if (y<96)
          {
            myGLCD.setColor(VGA_BLACK);
            myGLCD.fillRect(33, 0, myGLCD.getDisplayXSize()-33, myGLCD.getDisplayYSize()-1);
            myGLCD.setColor(colorlist[color]);
          }
          if ((y>128) and (y<160))
          {
            if (bsize<7)
            {
              bsize++;
              drawColorMarkerAndBrushSize(color);
              while (myTouch.dataAvailable()) {};
              delay(50);
            }
          }
          if ((y>160) and (y<192))
          {
              bsize=4;
              drawColorMarkerAndBrushSize(color);
              while (myTouch.dataAvailable()) {};
              delay(50);
          }
          if ((y>192) and (y<224))
          {
            if (bsize>1)
            {
              bsize--;
              drawColorMarkerAndBrushSize(color);
              while (myTouch.dataAvailable()) {};
              delay(50);
            }
          }
        }
      }
    }
  }
}

