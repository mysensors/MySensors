case CPLD:
	if (orient==LANDSCAPE)
		orient = PORTRAIT;
	else
		orient = LANDSCAPE;

	LCD_Write_COM(0x0F);   
	LCD_Write_COM_DATA(0x01,0x0010);
	LCD_Write_COM(0x0F);   
	break;
