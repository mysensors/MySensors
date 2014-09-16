case ILI9486:
	LCD_Write_COM(0x11);		// Sleep OUT
	delay(50);
 
	LCD_Write_COM(0xF2);		// ?????
	LCD_Write_DATA(0x1C);
	LCD_Write_DATA(0xA3);
	LCD_Write_DATA(0x32);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0xb2);
	LCD_Write_DATA(0x12);
	LCD_Write_DATA(0xFF);
	LCD_Write_DATA(0x12);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xF1);		// ?????
	LCD_Write_DATA(0x36);
	LCD_Write_DATA(0xA4);

	LCD_Write_COM(0xF8);		// ?????
	LCD_Write_DATA(0x21);
	LCD_Write_DATA(0x04);

	LCD_Write_COM(0xF9);		// ?????
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x08);

	LCD_Write_COM(0xC0);		// Power Control 1
	LCD_Write_DATA(0x0d);
	LCD_Write_DATA(0x0d);

	LCD_Write_COM(0xC1);		// Power Control 2
	LCD_Write_DATA(0x43);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xC2);		// Power Control 3
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xC5);		// VCOM Control
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x48);

	LCD_Write_COM(0xB6);		// Display Function Control
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x22);		// 0x42 = Rotate display 180 deg.
	LCD_Write_DATA(0x3B);

	LCD_Write_COM(0xE0);		// PGAMCTRL (Positive Gamma Control)
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x24);
	LCD_Write_DATA(0x1c);
	LCD_Write_DATA(0x0a);
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x08);
	LCD_Write_DATA(0x43);
	LCD_Write_DATA(0x88);
	LCD_Write_DATA(0x32);
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x06);
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x07);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xE1);		// NGAMCTRL (Negative Gamma Control)
	LCD_Write_DATA(0x0F);
	LCD_Write_DATA(0x38);
	LCD_Write_DATA(0x30);
	LCD_Write_DATA(0x09);
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x4e);
	LCD_Write_DATA(0x77);
	LCD_Write_DATA(0x3c);
	LCD_Write_DATA(0x07);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x05);
	LCD_Write_DATA(0x23);
	LCD_Write_DATA(0x1b);
	LCD_Write_DATA(0x00); 

	LCD_Write_COM(0x20);		// Display Inversion OFF
	LCD_Write_DATA(0x00);//C8 	 

	LCD_Write_COM(0x36);		// Memory Access Control
	LCD_Write_DATA(0x0A);

	LCD_Write_COM(0x3A);		// Interface Pixel Format
	LCD_Write_DATA(0x55); 

	LCD_Write_COM(0x2A);		// Column Addess Set
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0xDF);

	LCD_Write_COM(0x002B);		// Page Address Set
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x3f);	 
	delay(50);
	LCD_Write_COM(0x0029);		// Display ON
	LCD_Write_COM(0x002C);		// Memory Write
	break;
