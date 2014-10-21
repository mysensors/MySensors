case S6D0164:
	LCD_Write_COM_DATA(0x36,x2); 
	LCD_Write_COM_DATA(0x37,x1);
	LCD_Write_COM_DATA(0x38,y2);
	LCD_Write_COM_DATA(0x39,y1); 
	LCD_Write_COM_DATA(0x20,x1);
	LCD_Write_COM_DATA(0x21,y1); 	
	LCD_Write_COM(0x22);
	break;
