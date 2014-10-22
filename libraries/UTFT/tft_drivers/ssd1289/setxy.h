case SSD1289:
case SSD1289_8:
case SSD1289LATCHED:
	LCD_Write_COM_DATA(0x44,(x2<<8)+x1);
	LCD_Write_COM_DATA(0x45,y1);
	LCD_Write_COM_DATA(0x46,y2);
	LCD_Write_COM_DATA(0x4e,x1);
	LCD_Write_COM_DATA(0x4f,y1);
	LCD_Write_COM(0x22); 
	break;
