case ST7735S:
	LCD_Write_COM(0x11);//Sleep exit 
	delay(120);
 
	//ST7735R Frame Rate
	LCD_Write_COM(0xB1); 
	LCD_Write_DATA(0x05);
	LCD_Write_DATA(0x3C);
	LCD_Write_DATA(0x3C); 
	LCD_Write_COM(0xB2); 
	LCD_Write_DATA(0x05);
	LCD_Write_DATA(0x3C);
	LCD_Write_DATA(0x3C); 
	LCD_Write_COM(0xB3); 
	LCD_Write_DATA(0x05);
	LCD_Write_DATA(0x3C);
	LCD_Write_DATA(0x3C); 
	LCD_Write_DATA(0x05);
	LCD_Write_DATA(0x3C);
	LCD_Write_DATA(0x3C); 

	LCD_Write_COM(0xB4); //Column inversion 
	LCD_Write_DATA(0x03); 
 
	//ST7735R Power Sequence
	LCD_Write_COM(0xC0); 
	LCD_Write_DATA(0x28);
	LCD_Write_DATA(0x08);
	LCD_Write_DATA(0x04); 
	LCD_Write_COM(0xC1);
	LCD_Write_DATA(0xC0); 
	LCD_Write_COM(0xC2); 
	LCD_Write_DATA(0x0D);
	LCD_Write_DATA(0x00); 
	LCD_Write_COM(0xC3); 
	LCD_Write_DATA(0x8D);
	LCD_Write_DATA(0x2A); 
	LCD_Write_COM(0xC4); 
	LCD_Write_DATA(0x8D);
	LCD_Write_DATA(0xEE); 
 
	LCD_Write_COM(0xC5); //VCOM 
	LCD_Write_DATA(0x1A); 
 
	LCD_Write_COM(0x36); //MX, MY, RGB mode 
	LCD_Write_DATA(0xC0); 

	//ST7735R Gamma Sequence
	LCD_Write_COM(0xE0); 
	LCD_Write_DATA(0x03);
	LCD_Write_DATA(0x22); 
	LCD_Write_DATA(0x07);
	LCD_Write_DATA(0x0A); 
	LCD_Write_DATA(0x2E);
	LCD_Write_DATA(0x30); 
	LCD_Write_DATA(0x25);
	LCD_Write_DATA(0x2A); 
	LCD_Write_DATA(0x28);
	LCD_Write_DATA(0x26); 
	LCD_Write_DATA(0x2E);
	LCD_Write_DATA(0x3A);
	LCD_Write_DATA(0x00); 
	LCD_Write_DATA(0x01); 
	LCD_Write_DATA(0x03);
	LCD_Write_DATA(0x13); 
	LCD_Write_COM(0xE1); 
	LCD_Write_DATA(0x04);
	LCD_Write_DATA(0x16); 
	LCD_Write_DATA(0x06);
	LCD_Write_DATA(0x0D); 
	LCD_Write_DATA(0x2D);
	LCD_Write_DATA(0x26); 
	LCD_Write_DATA(0x23);
	LCD_Write_DATA(0x27); 
	LCD_Write_DATA(0x27);
	LCD_Write_DATA(0x25); 
	LCD_Write_DATA(0x2D);
	LCD_Write_DATA(0x3B); 
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x01); 
	LCD_Write_DATA(0x04);
	LCD_Write_DATA(0x13);  

	//LCD_Write_COM(0x2A);
	//LCD_Write_DATA(0x00);
	//LCD_Write_DATA(0x00);
	//LCD_Write_DATA(0x00);
	//LCD_Write_DATA(0x7F);
	//LCD_Write_COM(0x2B);
	//LCD_Write_DATA(0x00);
	//LCD_Write_DATA(0x00);
	//LCD_Write_DATA(0x00);
	//LCD_Write_DATA(0x9F);

	LCD_Write_COM(0x3A); //65k mode 
	LCD_Write_DATA(0x05); 
	LCD_Write_COM(0x29);//Display on
	break;
