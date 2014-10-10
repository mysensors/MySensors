// ---------------------------------------------------------------------------
// Created by GHPS on 5/06/2012.
// Copyright 2012 - Under creative commons license 3.0:
//        Attribution-ShareAlike CC BY-SA
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// Thread Safe: No
// Extendable: Yes
//
// @file LiquidCrystal_I2C_ByVac.c
// This file implements a basic liquid crystal library that comes as standard
// in the Arduino SDK but using the extension board BV4218/BV4208 from ByVac.
// 
// @brief 
// This is a basic implementation of the LiquidCrystal library of the
// Arduino SDK. The original library has been reworked in such a way that 
// this class implements the all methods to command an LCD based
// on the Hitachi HD44780 and compatible chipsets using I2C extension
// backpack BV4218 from ByVac.
//
// The functionality provided by this class and its base class is identical
// to the original functionality of the Arduino LiquidCrystal library.
//
// @author GHPS - ghps-et-users-sourceforge-net
// ---------------------------------------------------------------------------
#ifndef LiquidCrystal_I2C_ByVac_h
#define LiquidCrystal_I2C_ByVac_h
#include <inttypes.h>
#include <Print.h>

#include <../Wire/Wire.h>
#include "LCD.h"


class LiquidCrystal_I2C_ByVac : public LCD 
{
public:
   
   /*!
    @method     
    @abstract   Class constructor. 
    @discussion Initializes class variables and defines the I2C address of the
    LCD. The constructor does not initialize the LCD.
    
    @param      lcd_Addr[in] I2C address of the IO expansion module. For BV4218,
    the address can be configured using the address commands (to be implemented).
    */
   LiquidCrystal_I2C_ByVac (uint8_t lcd_Addr);

   /*!
    @function
    @abstract   LCD initialization and associated HW.
    @discussion Initializes the LCD to a given size (col, row). This methods
    initializes the LCD, therefore, it MUST be called prior to using any other
    method from this class or parent class.
    
    The begin method can be overloaded if necessary to initialize any HW that 
    is implemented by a library and can't be done during construction, here
    we use the Wire class.
    
    @param      cols[in] the number of columns that the display has
    @param      rows[in] the number of rows that the display has
    @param      charsize[in] size of the characters of the LCD: LCD_5x8DOTS or
    LCD_5x10DOTS.
    */
   virtual void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);
   
   /*!
    @function
    @abstract   Send a particular value to the LCD.
    @discussion Sends a particular value to the LCD for writing to the LCD or
    as an LCD command.
    
    Users should never call this method.
    
    @param      value[in] Value to send to the LCD.
    @param      mode[in] DATA - write to the LCD CGRAM, COMMAND - write a 
    command to the LCD.
    */
   virtual void send(uint8_t value, uint8_t mode);
   
   
   /*!
    @function
    @abstract   Switch-on/off the LCD backlight.
    @discussion Switch-on/off the LCD backlight.
    
    @param      value: backlight mode (HIGH|LOW)
    */
   void setBacklight ( uint8_t value );
 
   /*!
    @function
    @abstract   Switch-on/off the LCD contrast.
    @discussion Switch-on/off the LCD contrast.

    
    @param      value: contrast mode (HIGH|LOW)
    */
   void setContrast ( uint8_t value );
 
private:
   
   /*!
    @method     
    @abstract   Initializes the LCD class
    @discussion Initializes the LCD class and IO expansion module.
    */
   int  init();
   
   /*!
    @function
    @abstract   Initialises class private variables
    @discussion This is the class single point for initialising private variables.
    
    @param      lcd_Addr[in] I2C address of the IO expansion module. For BV4218,
    the address can be configured using the address commands.
    */
    
   uint8_t _Addr;             // I2C Address of the IO expander
   
};

#endif
