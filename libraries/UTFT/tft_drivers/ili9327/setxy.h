case ILI9327:
	LCD_Write_COM(0x2a);
  	LCD_Write_DATA(0x00,x1>>8);
  	LCD_Write_DATA(0x00,x1);
  	LCD_Write_DATA(0x00,x2>>8);
  	LCD_Write_DATA(0x00,x2);
  	LCD_Write_COM(0x2b);
  	LCD_Write_DATA(0x00,y1>>8);
  	LCD_Write_DATA(0x00,y1);
  	LCD_Write_DATA(0x00,y2>>8);
  	LCD_Write_DATA(0x00,y2);
  	LCD_Write_COM(0x2c); 							 
	break;
