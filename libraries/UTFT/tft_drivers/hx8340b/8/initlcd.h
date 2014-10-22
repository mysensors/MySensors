case HX8340B_8:
	LCD_Write_COM_DATA(0x26,0x0084); //PT=10,GON=0, DTE=0, D=0100
	delay(40);
	LCD_Write_COM_DATA(0x26,0x00B8); //PT=10,GON=1, DTE=1, D=1000
	delay(40);
	LCD_Write_COM_DATA(0x26,0x00BC); //PT=10,GON=1, DTE=1, D=1100
	delay(20);
	// LCD_Write_COM_DATA(0x0001,0x0000);     // PTL='1' Enter Partail mode

	//Driving ability Setting
	LCD_Write_COM_DATA(0x60,0x0000);
	LCD_Write_COM_DATA(0x61,0x0006);
	LCD_Write_COM_DATA(0x62,0x0000);
	LCD_Write_COM_DATA(0x63,0x00C8);
	delay(20);

	//Gamma Setting
	LCD_Write_COM_DATA(0x73,0x0070);
	LCD_Write_COM_DATA(0x40,0x0000);
	LCD_Write_COM_DATA(0x41,0x0040);
	LCD_Write_COM_DATA(0x42,0x0045);
	LCD_Write_COM_DATA(0x43,0x0001);
	LCD_Write_COM_DATA(0x44,0x0060);
	LCD_Write_COM_DATA(0x45,0x0005);
	LCD_Write_COM_DATA(0x46,0x000C);
	LCD_Write_COM_DATA(0x47,0x00D1);
	LCD_Write_COM_DATA(0x48,0x0005);

	LCD_Write_COM_DATA(0x50,0x0075);
	LCD_Write_COM_DATA(0x51,0x0001);
	LCD_Write_COM_DATA(0x52,0x0067);
	LCD_Write_COM_DATA(0x53,0x0014);
	LCD_Write_COM_DATA(0x54,0x00F2);
	LCD_Write_COM_DATA(0x55,0x0007);
	LCD_Write_COM_DATA(0x56,0x0003);
	LCD_Write_COM_DATA(0x57,0x0049);
	delay(20);

	//Power Setting
	LCD_Write_COM_DATA(0x1F,0x0003); //VRH=4.65V     VREG1?GAMMA? 00~1E  080421    
	LCD_Write_COM_DATA(0x20,0x0000); //BT (VGH~15V,VGL~-12V,DDVDH~5V)
	LCD_Write_COM_DATA(0x24,0x0024); //VCOMH(VCOM High voltage3.2V)     0024/12    080421    11~40
	LCD_Write_COM_DATA(0x25,0x0034); //VCOML(VCOM Low voltage -1.2V)    0034/4A    080421    29~3F 
	//****VCOM offset**///
	LCD_Write_COM_DATA(0x23,0x002F); //VMF(no offset)                            
	delay(20);

	//##################################################################
	// Power Supply Setting
	LCD_Write_COM_DATA(0x18,0x0044); //I/P_RADJ,N/P_RADJ Noraml mode 60Hz
	LCD_Write_COM_DATA(0x21,0x0001); //OSC_EN='1' start osc
	LCD_Write_COM_DATA(0x01,0x0000); //SLP='0' out sleep
	LCD_Write_COM_DATA(0x1C,0x0003); //AP=011
	LCD_Write_COM_DATA(0x19,0x0006); // VOMG=1,PON=1, DK=0,
	delay(20);

	//##################################################################
	// Display ON Setting
	LCD_Write_COM_DATA(0x26,0x0084); //PT=10,GON=0, DTE=0, D=0100
	delay(40);
	LCD_Write_COM_DATA(0x26,0x00B8); //PT=10,GON=1, DTE=1, D=1000
	delay(40);
	LCD_Write_COM_DATA(0x26,0x00BC); //PT=10,GON=1, DTE=1, D=1100
	delay(20);

	//SET GRAM AREA
	LCD_Write_COM_DATA(0x02,0x0000); 
	LCD_Write_COM_DATA(0x03,0x0000); 
	LCD_Write_COM_DATA(0x04,0x0000);
	LCD_Write_COM_DATA(0x05,0x00AF);
	LCD_Write_COM_DATA(0x06,0x0000);
	LCD_Write_COM_DATA(0x07,0x0000);
	LCD_Write_COM_DATA(0x08,0x0000);
	LCD_Write_COM_DATA(0x09,0x00DB);
	delay(20);
	LCD_Write_COM_DATA(0x16,0x0008);  //MV MX MY ML SET  0028
	LCD_Write_COM_DATA(0x17,0x0005);//COLMOD Control Register (R17h)
	LCD_Write_COM(0x21);
	LCD_Write_COM(0x22);
	break;
