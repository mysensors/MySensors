/*
  UTFT_Buttons.h - Add-on Library for UTFT: Buttons
  Copyright (C)2013 Henning Karlsen. All right reserved
  
  This library adds simple but easy to use buttons to extend the use
  of the UTFT and UTouch libraries.

  You can always find the latest version of the library at 
  http://electronics.henningkarlsen.com/

  If you make any modifications or improvements to the code, I would 
  appreciate that you share the code with me so that I might include 
  it in the next release. I can be contacted through 
  http://electronics.henningkarlsen.com/contact.php.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the CC BY-NC-SA 3.0 license.
  Please see the included documents for further information.
*/

#ifndef UTFT_Buttons_h
#define UTFT_Buttons_h

#if defined(__AVR__)
	#include "Arduino.h"
#elif defined(__PIC32MX__)
	#include "WProgram.h"
#elif defined(__arm__)
	#include "Arduino.h"
#endif

#include <UTFT.h>
#include <UTouch.h>
#if ((!defined(UTFT_VERSION)) || (UTFT_VERSION<241))
	#error : You will need UTFT v2.41 or higher to use this add-on library...
#endif

#define MAX_BUTTONS	20	// Maximum number of buttons available at one time

// Define presets for button status
#define BUTTON_DISABLED			0x0001
#define BUTTON_SYMBOL			0x0002
#define BUTTON_SYMBOL_REP_3X	0x0004
#define BUTTON_BITMAP			0x0008	
#define BUTTON_NO_BORDER		0x0010
#define BUTTON_UNUSED			0x8000

typedef struct
{
  uint16_t			pos_x, pos_y, width, height;
  uint16_t			flags;
  char				*label;
  bitmapdatatype	data;
} button_type;

class UTFT_Buttons
{
	public:
		UTFT_Buttons(UTFT *ptrUTFT, UTouch *ptrUTouch);

		int		addButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, char *label, uint16_t flags=0);
		int		addButton(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bitmapdatatype data, uint16_t flags=0);
		void	drawButtons();
		void	drawButton(int buttonID);
		void	enableButton(int buttonID, boolean redraw=false);
		void	disableButton(int buttonID, boolean redraw=false);
		void	relabelButton(int buttonID, char *label, boolean redraw=false);
		boolean	buttonEnabled(int buttonID);
		void	deleteButton(int buttonID);
		void	deleteAllButtons();
		int		checkButtons();
		void	setTextFont(uint8_t* font);
		void	setSymbolFont(uint8_t* font);
		void	setButtonColors(word atxt, word iatxt, word brd, word brdhi, word back);

	protected:
		UTFT		*_UTFT;
		UTouch		*_UTouch;
		button_type	buttons[MAX_BUTTONS];
		word		_color_text, _color_text_inactive, _color_background, _color_border, _color_hilite;
		uint8_t		*_font_text, *_font_symbol;
};

#endif
