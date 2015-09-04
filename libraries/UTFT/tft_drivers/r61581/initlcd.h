case R61581:
		LCD_Write_COM(0xB0);		
		LCD_Write_DATA(0x1E);	    

		LCD_Write_COM(0xB0);
		LCD_Write_DATA(0x00);

		LCD_Write_COM(0xB3);
		LCD_Write_DATA(0x02);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x10);

		LCD_Write_COM(0xB4);
		LCD_Write_DATA(0x00);//0X10

// 		LCD_Write_COM(0xB9); //PWM Settings for Brightness Control
// 		LCD_Write_DATA(0x01);// Disabled by default. 
// 		LCD_Write_DATA(0xFF); //0xFF = Max brightness
// 		LCD_Write_DATA(0xFF);
// 		LCD_Write_DATA(0x18);

		LCD_Write_COM(0xC0);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0x3B);//
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x01);
		LCD_Write_DATA(0x00);//NW
		LCD_Write_DATA(0x43);

		LCD_Write_COM(0xC1);
		LCD_Write_DATA(0x08);
		LCD_Write_DATA(0x15);//CLOCK
		LCD_Write_DATA(0x08);
		LCD_Write_DATA(0x08);

		LCD_Write_COM(0xC4);
		LCD_Write_DATA(0x15);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0x01);

		LCD_Write_COM(0xC6);
		LCD_Write_DATA(0x02);

		LCD_Write_COM(0xC8);
		LCD_Write_DATA(0x0c);
		LCD_Write_DATA(0x05);
		LCD_Write_DATA(0x0A);//0X12
		LCD_Write_DATA(0x6B);//0x7D
		LCD_Write_DATA(0x04);
		LCD_Write_DATA(0x06);//0x08
		LCD_Write_DATA(0x15);//0x0A
		LCD_Write_DATA(0x10);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x60);//0x23
 
		LCD_Write_COM(0x36);
		LCD_Write_DATA(0x0A);

		LCD_Write_COM(0x0C);
		LCD_Write_DATA(0x55);

		LCD_Write_COM(0x3A);
		LCD_Write_DATA(0x55);

		LCD_Write_COM(0x38);
 
		LCD_Write_COM(0xD0);
		LCD_Write_DATA(0x07);
		LCD_Write_DATA(0x07);//VCI1
		LCD_Write_DATA(0x14);//VRH 0x1D
		LCD_Write_DATA(0xA2);//BT 0x06

		LCD_Write_COM(0xD1);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0x5A);//VCM  0x5A
		LCD_Write_DATA(0x10);//VDV

		LCD_Write_COM(0xD2);
		LCD_Write_DATA(0x03);
		LCD_Write_DATA(0x04);//0x24
		LCD_Write_DATA(0x04);

		LCD_Write_COM(0x11);
		delay(150);

		LCD_Write_COM(0x2A);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x01);
		LCD_Write_DATA(0xDF);//320

		LCD_Write_COM(0x2B);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x00);
		LCD_Write_DATA(0x01);
		LCD_Write_DATA(0x3F);//480

 
		delay(100);

		LCD_Write_COM(0x29);
		delay(30);

		LCD_Write_COM(0x2C);
		delay(30);
		break;
