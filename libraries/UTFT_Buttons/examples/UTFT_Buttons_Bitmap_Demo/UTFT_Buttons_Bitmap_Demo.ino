// UTFT_Buttons_Bitmap_Demo (C)2013 Henning Karlsen
// web: http://www.henningkarlsen.com/electronics
//
// A small demo to demonstrate the use of some of the
// functions of the UTFT_Buttons add-on library.
//
// This demo was made for modules with a screen resolution 
// of 320x240 pixels, but should work on larger screens as
// well.
//
// This demo will not work on Arduino 2009/Uno/Leonardo
// due to the size of the images.
//
// This program requires both the UTFT and UTouch libraries
// in addition to the UTFT_Buttons add-on library.
//

// This code block is only needed to support multiple
// MCU architectures in a single sketch.
#if defined(__AVR__)
	#define imagedatatype  unsigned int
#elif defined(__PIC32MX__)
	#define imagedatatype  unsigned short
#elif defined(__arm__)
	#define imagedatatype  unsigned short
#endif
// End of multi-architecture block

#include <UTFT.h>
#include <UTouch.h>
#include <UTFT_Buttons.h>

// Declare which fonts we will be using
extern uint8_t BigFont[];

// Declare which bitmaps we will be using
extern imagedatatype cat[];
extern imagedatatype dog[];
extern imagedatatype bird[];
extern imagedatatype monkey[];

// Set up UTFT...
// Set the pins to the correct ones for your development board
// -----------------------------------------------------------
// Standard Arduino 2009/Uno/Leonardo shield   : NOT SUPPORTED DUE TO LACK OF MEMORY
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
// Standard Arduino 2009/Uno/Leonardo shield   : NOT SUPPORTED DUE TO LACK OF MEMORY
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
  myGLCD.setFont(BigFont);

  myTouch.InitTouch();
  myTouch.setPrecision(PREC_MEDIUM);
  
  myButtons.setTextFont(BigFont);
}

void loop()
{
  int but1, but2, but3, but4, but5, pressed_button;
  
  but1 = myButtons.addButton( 10,  10, 80,  60, cat);
  but2 = myButtons.addButton( 120,  10, 80,  60, dog);
  but3 = myButtons.addButton( 10,  80, 80,  60, bird);
  but4 = myButtons.addButton( 120,  80, 80,  60, monkey, BUTTON_NO_BORDER);
  but5 = myButtons.addButton( 10, 150, 190, 30, "Disable Dog");
  myButtons.drawButtons();

  myGLCD.print("You pressed:", 10, 200);
  myGLCD.setColor(VGA_BLACK);
  myGLCD.setBackColor(VGA_WHITE);
  myGLCD.print("None    ", 10, 220);

  while(1) 
  {
    if (myTouch.dataAvailable() == true)
    {
      pressed_button = myButtons.checkButtons();

      if (pressed_button==but5)
        if (myButtons.buttonEnabled(but2))
        {
          myButtons.disableButton(but2);
          myButtons.relabelButton(but5, "Enable Dog", true);
          myButtons.drawButton(but2);
        }
        else
        {
          myButtons.enableButton(but2);
          myButtons.relabelButton(but5, "Disable Dog", true);
          myButtons.drawButton(but2);
        }

      if (pressed_button==but1)
        myGLCD.print("Cat     ", 10, 220);
      if (pressed_button==but2)
        myGLCD.print("Dog     ", 10, 220);
      if (pressed_button==but3)
        myGLCD.print("Bird    ", 10, 220);
      if (pressed_button==but4)
        myGLCD.print("Monkey  ", 10, 220);
      if (pressed_button==-1)
        myGLCD.print("None    ", 10, 220);
    }
  }
}

