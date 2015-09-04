case ST7735:
	LCD_Write_COM(0x11);//Sleep exit 
	delay(12);
 
	//ST7735R Frame Rate
	LCD_Write_COM(0xB1); 
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x2C);
	LCD_Write_DATA(0x2D); 
	LCD_Write_COM(0xB2); 
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x2C);
	LCD_Write_DATA(0x2D); 
	LCD_Write_COM(0xB3); 
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x2C);
	LCD_Write_DATA(0x2D); 
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x2C);
	LCD_Write_DATA(0x2D); 

	LCD_Write_COM(0xB4); //Column inversion 
	LCD_Write_DATA(0x07); 
 
	//ST7735R Power Sequence
	LCD_Write_COM(0xC0); 
	LCD_Write_DATA(0xA2);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x84); 
	LCD_Write_COM(0xC1);
	LCD_Write_DATA(0xC5); 
	LCD_Write_COM(0xC2); 
	LCD_Write_DATA(0x0A);
	LCD_Write_DATA(0x00); 
	LCD_Write_COM(0xC3); 
	LCD_Write_DATA(0x8A);
	LCD_Write_DATA(0x2A); 
	LCD_Write_COM(0xC4); 
	LCD_Write_DATA(0x8A);
	LCD_Write_DATA(0xEE); 
 
	LCD_Write_COM(0xC5); //VCOM 
	LCD_Write_DATA(0x0E); 
 
	LCD_Write_COM(0x36); //MX, MY, RGB mode 
	LCD_Write_DATA(0xC8); 

	//ST7735R Gamma Sequence
	LCD_Write_COM(0xe0); 
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x1a); 
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x18); 
	LCD_Write_DATA(0x2f);
	LCD_Write_DATA(0x28); 
	LCD_Write_DATA(0x20);
	LCD_Write_DATA(0x22); 
	LCD_Write_DATA(0x1f);
	LCD_Write_DATA(0x1b); 
	LCD_Write_DATA(0x23);
	LCD_Write_DATA(0x37);
	LCD_Write_DATA(0x00); 

	LCD_Write_DATA(0x07); 
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x10); 
	LCD_Write_COM(0xe1); 
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x1b); 
	LCD_Write_DATA(0x0f);
	LCD_Write_DATA(0x17); 
	LCD_Write_DATA(0x33);
	LCD_Write_DATA(0x2c); 
	LCD_Write_DATA(0x29);
	LCD_Write_DATA(0x2e); 
	LCD_Write_DATA(0x30);
	LCD_Write_DATA(0x30); 
	LCD_Write_DATA(0x39);
	LCD_Write_DATA(0x3f); 
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x07); 
	LCD_Write_DATA(0x03);
	LCD_Write_DATA(0x10);  

	LCD_Write_COM(0x2a);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x7f);
	LCD_Write_COM(0x2b);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x9f);

	LCD_Write_COM(0xF0); //Enable test command  
	LCD_Write_DATA(0x01); 
	LCD_Write_COM(0xF6); //Disable ram power save mode 
	LCD_Write_DATA(0x00); 
 
	LCD_Write_COM(0x3A); //65k mode 
	LCD_Write_DATA(0x05); 
	LCD_Write_COM(0x29);//Display on
	break;
