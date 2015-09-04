case S6D1121_8:
case S6D1121_16:
	LCD_Write_COM_DATA(0x46,(x2 << 8) | x1);
	LCD_Write_COM_DATA(0x47,y2);
	LCD_Write_COM_DATA(0x48,y1);
	LCD_Write_COM_DATA(0x20,x1);
	LCD_Write_COM_DATA(0x21,y1);
	LCD_Write_COM(0x22);
	break;
