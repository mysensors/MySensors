// UTFT_Buttons_Demo_320x240 (C)2013 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// A small demo to demonstrate the use of some of the
// functions of the UTFT_Buttons add-on library.
//
// This demo was made for modules with a screen resolution 
// of 320x240 pixels, but should work on larger screens as
// well.
//
// This program requires both the UTFT and UTouch libraries
// in addition to the UTFT_Buttons add-on library.
//

#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>

// Declare which fonts we will be using
extern uint8_t SmallFont[];
extern uint8_t BigFont[];
extern uint8_t Dingbats1_XL[];

// Set up UTFT...
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino 2009/Uno/Leonardo shield   : <display model>,19,18,17,16
// Standard Arduino Mega/Due shield            : <display model>,38,39,40,41
// CTE TFT LCD/SD Shield for Arduino Due       : <display model>,25,26,27,28
// Standard chipKit Uno32/uC32                 : <display model>,34,35,36,37
// Standard chipKit Max32                      : <display model>,82,83,84,85
// AquaLEDSource All in One Super Screw Shield : <display model>,82,83,84,85
//
// Remember to change the model parameter to suit your display module!
UTFT          myGLCD(ITDB32S,38,39,40,41);

// Set up UTouch...
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino 2009/Uno/Leonardo shield   : 15,10,14,9,8
// Standard Arduino Mega/Due shield            : 6,5,4,3,2
// CTE TFT LCD/SD Shield for Arduino Due       : 6,5,4,3,2
// Standard chipKit Uno32/uC32                 : 20,21,22,23,24
// Standard chipKit Max32                      : 62,63,64,65,66
// AquaLEDSource All in One Super Screw Shield : 62,63,64,65,66
UTouch        myTouch(6,5,4,3,2);

// Finally we set up UTFT_Buttons :)
UTFT_Buttons  myButtons(&myGLCD, &myTouch);

void setup()
{
  myGLCD.InitLCD();
  myGLCD.clrScr();
  myGLCD.setFont(SmallFont);

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  
  myButtons.setTextFont(BigFont);
  myButtons.setSymbolFont(Dingbats1_XL);
}

void loop()
{
  int but1, but2, but3, but4, butX, butY, pressed_button;
  boolean default_colors = true;
  
  but1 = myButtons.addButton( 10,  20, 300,  30, "Button 1");
  but2 = myButtons.addButton( 10,  60, 300,  30, "Button 2");
  but3 = myButtons.addButton( 10, 100, 300,  30, "Button 3");
  but4 = myButtons.addButton( 10, 140, 300,  30, "Button 4", BUTTON_DISABLED);
  butX = myButtons.addButton(279, 199,  40,  40, "a", BUTTON_SYMBOL);
  butY = myButtons.addButton(  0, 199, 100,  40, "I", BUTTON_SYMBOL | BUTTON_SYMBOL_REP_3X);
  myButtons.drawButtons();

  myGLCD.print("You pressed:", 110, 205);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.print("None    ", 110, 220);

  while(1) 
  {
    if (myTouch.dataAvailable() == true)
    {
      pressed_button = myButtons.checkButtons();

      if (pressed_button==butX)
      {
        if (myButtons.buttonEnabled(but4))
          myButtons.disableButton(but4, true);
        else
          myButtons.enableButton(but4, true);
      }
      else if (pressed_button==butY)
      {
        if (default_colors)
        {
          myButtons.setButtonColors(VGA_YELLOW, VGA_RED, VGA_YELLOW, VGA_BLUE, VGA_GRAY);
          myButtons.relabelButton(butY, "_");
          myButtons.drawButtons();
          default_colors=false;
        }
        else
        {
          myButtons.setButtonColors(VGA_WHITE, VGA_GRAY, VGA_WHITE, VGA_RED, VGA_BLUE);
          myButtons.relabelButton(butY, "I");
          myButtons.drawButtons();
          default_colors=true;
        }
      }
      if (pressed_button==but1)
        myGLCD.print("Button 1", 110, 220);
      if (pressed_button==but2)
        myGLCD.print("Button 2", 110, 220);
      if (pressed_button==but3)
        myGLCD.print("Button 3", 110, 220);
      if (pressed_button==but4)
        myGLCD.print("Button 4", 110, 220);
      if (pressed_button==-1)
        myGLCD.print("None    ", 110, 220);
    }
  }
}

