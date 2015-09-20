case ILI9341_S4P:
	LCD_Write_COM(0x11);//sleep out 
	delay(20);
  //LCD_Write_COM(0x01); //reset
  //delay(15);
  LCD_Write_COM(0x28); //display off
  delay(5);
  LCD_Write_COM(0xCF); //power control b
  LCD_Write_DATA(0x00);
  LCD_Write_DATA(0x83); //83 81 AA
  LCD_Write_DATA(0x30);
  LCD_Write_COM(0xED); //power on seq control
  LCD_Write_DATA(0x64); //64 67
  LCD_Write_DATA(0x03);
  LCD_Write_DATA(0x12);
  LCD_Write_DATA(0x81);
  LCD_Write_COM(0xE8); //timing control a
  LCD_Write_DATA(0x85);
  LCD_Write_DATA(0x01);
  LCD_Write_DATA(0x79); //79 78
  LCD_Write_COM(0xCB); //power control a
  LCD_Write_DATA(0x39);
  LCD_Write_DATA(0X2C);
  LCD_Write_DATA(0x00);
  LCD_Write_DATA(0x34);
  LCD_Write_DATA(0x02);
  LCD_Write_COM(0xF7); //pump ratio control
  LCD_Write_DATA(0x20);
  LCD_Write_COM(0xEA); //timing control b
  LCD_Write_DATA(0x00);
  LCD_Write_DATA(0x00);
  LCD_Write_COM(0xC0); //power control 2
  LCD_Write_DATA(0x26); //26 25
  LCD_Write_COM(0xC1); //power control 2
  LCD_Write_DATA(0x11);
  LCD_Write_COM(0xC5); //vcom control 1
  LCD_Write_DATA(0x35);
  LCD_Write_DATA(0x3E);
  LCD_Write_COM(0xC7); //vcom control 2
  LCD_Write_DATA(0xBE); //BE 94
  LCD_Write_COM(0xB1); //frame control
  LCD_Write_DATA(0x00);
  LCD_Write_DATA(0x1B); //1B 70
  LCD_Write_COM(0xB6); //display control
  LCD_Write_DATA(0x0A);
  LCD_Write_DATA(0x82);
  LCD_Write_DATA(0x27);
  LCD_Write_DATA(0x00);
  LCD_Write_COM(0xB7); //emtry mode
  LCD_Write_DATA(0x07);
  LCD_Write_COM(0x3A); //pixel format
  LCD_Write_DATA(0x55); //16bit
  LCD_Write_COM(0x36); //mem access
  LCD_Write_DATA((1<<3)|(1<<6));
  //LCD_Write_DATA((1<<3)|(1<<7)); //rotate 180
  LCD_Write_COM(0x29); //display on
  delay(5);
	break;
