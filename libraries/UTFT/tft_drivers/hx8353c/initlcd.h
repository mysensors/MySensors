case HX8353C:
	LCD_Write_COM(0xB9);//SETEXTC
		LCD_Write_DATA(0xFF);
		LCD_Write_DATA(0x83);
		LCD_Write_DATA(0x53);

		LCD_Write_COM(0xB0);//RADJ
		LCD_Write_DATA(0x3C);
		LCD_Write_DATA(0x01);

		LCD_Write_COM(0xB6);//VCOM
		LCD_Write_DATA(0x94);
		LCD_Write_DATA(0x6C);
		LCD_Write_DATA(0x50);   

		LCD_Write_COM(0xB1);//PWR
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x01);
		LCD_Write_DATA(0x1B);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0x01);
		LCD_Write_DATA(0x08);
		LCD_Write_DATA(0x77);
		LCD_Write_DATA(0x89);

		LCD_Write_COM(0xE0); //Gamma setting for tpo Panel
		LCD_Write_DATA(0x50);
		LCD_Write_DATA(0x77);
		LCD_Write_DATA(0x40);
		LCD_Write_DATA(0x08);
		LCD_Write_DATA(0xBF);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0x0F);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x01);
		LCD_Write_DATA(0x73);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x72);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0xB0);
		LCD_Write_DATA(0x0F);
		LCD_Write_DATA(0x08);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x0F);

		LCD_Write_COM(0x3A);   
		LCD_Write_DATA(0x05);  //05 
		LCD_Write_COM(0x36);    
		LCD_Write_DATA(0xC0); //83  //0B 

		LCD_Write_COM(0x11); // SLPOUT  
		delay(150);

		LCD_Write_COM(0x29);    // display on

		delay(150);
		LCD_Write_COM(0x2D);  //Look up table

		for(int j=0;j<32;j++)
		{ LCD_Write_DATA(2*j); } //Red
		for(int j=0;j<64;j++)
		{ LCD_Write_DATA(1*j); } //Green
		for(int j=0;j<32;j++)
		{ LCD_Write_DATA(2*j); } //Blue 

		LCD_Write_COM(0x2c);  
		delay(150);	 
	break;
