case SSD1963_800ALT:
	LCD_Write_COM(0xE2);		//PLL multiplier, set PLL clock to 120M
	LCD_Write_DATA(0x23);	    //N=0x36 for 6.5M, 0x23 for 10M crystal
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x04);
	LCD_Write_COM(0xE0);		// PLL enable
	LCD_Write_DATA(0x01);
	delay(10);
	LCD_Write_COM(0xE0);
	LCD_Write_DATA(0x03);
	delay(10);
	LCD_Write_COM(0x01);		// software reset
	delay(100);
	LCD_Write_COM(0xE6);		//PLL setting for PCLK, depends on resolution
	LCD_Write_DATA(0x04);
	LCD_Write_DATA(0x93);
	LCD_Write_DATA(0xE0);

	LCD_Write_COM(0xB0);		//LCD SPECIFICATION
	LCD_Write_DATA(0x00);	// 0x24
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x03);		//Set HDP	799
	LCD_Write_DATA(0x1F);
	LCD_Write_DATA(0x01);		//Set VDP	479
	LCD_Write_DATA(0xDF);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xB4);		//HSYNC
	LCD_Write_DATA(0x03);		//Set HT	928
	LCD_Write_DATA(0xA0);
	LCD_Write_DATA(0x00);		//Set HPS	46
	LCD_Write_DATA(0x2E);
	LCD_Write_DATA(0x30);		//Set HPW	48
	LCD_Write_DATA(0x00);		//Set LPS	15
	LCD_Write_DATA(0x0F);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xB6);		//VSYNC
	LCD_Write_DATA(0x02);		//Set VT	525
	LCD_Write_DATA(0x0D);
	LCD_Write_DATA(0x00);		//Set VPS	16
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x10);		//Set VPW	16
	LCD_Write_DATA(0x00);		//Set FPS	8
	LCD_Write_DATA(0x08);

	LCD_Write_COM(0xBA);
	LCD_Write_DATA(0x05);		//GPIO[3:0] out 1

	LCD_Write_COM(0xB8);
	LCD_Write_DATA(0x07);	    //GPIO3=input, GPIO[2:0]=output
	LCD_Write_DATA(0x01);		//GPIO0 normal

	LCD_Write_COM(0x36);		//rotation
	LCD_Write_DATA(0x22);		// -- Set to 0x21 to rotate 180 degrees

	LCD_Write_COM(0xF0);		//pixel data interface
	LCD_Write_DATA(0x03);


	delay(10);

	setXY(0, 0, 799, 479);
	LCD_Write_COM(0x29);		//display on

	LCD_Write_COM(0xBE);		//set PWM for B/L
	LCD_Write_DATA(0x06);
	LCD_Write_DATA(0xF0);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0xF0);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xD0); 
	LCD_Write_DATA(0x0D);	

	LCD_Write_COM(0x2C); 
	break;
