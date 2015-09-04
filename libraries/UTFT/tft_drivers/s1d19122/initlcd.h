case S1D19122:
	//************* Start Initial Sequence **********//

	int i,R,G,B;
	LCD_Write_COM(0x11);
	LCD_Write_COM(0x13);
	LCD_Write_COM(0x29);
    
	//--------------  Display Control ---------//
	LCD_Write_COM(0xB0);

	LCD_Write_DATA(0x05);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0xF0);
	LCD_Write_DATA(0x0A);
	LCD_Write_DATA(0x41);
	LCD_Write_DATA(0x02); 
	LCD_Write_DATA(0x0A);
	LCD_Write_DATA(0x30);
	LCD_Write_DATA(0x31);
	LCD_Write_DATA(0x36);
	LCD_Write_DATA(0x37);
	LCD_Write_DATA(0x40);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x3F);
	LCD_Write_DATA(0x40);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x81);
	LCD_Write_DATA(0x04);
	LCD_Write_DATA(0x05);
	LCD_Write_DATA(0x64);

	// ----------- Gamma  Curve  Set3 Postive----------//
	LCD_Write_COM(0xFC);

	LCD_Write_DATA(0x88);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x42);
	LCD_Write_DATA(0x42);
	LCD_Write_DATA(0x22);
	LCD_Write_DATA(0x11);
	LCD_Write_DATA(0x11);
	LCD_Write_DATA(0x22);
	LCD_Write_DATA(0x99);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0xBB);
	LCD_Write_DATA(0xBB);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0x33);
	LCD_Write_DATA(0x33);
	LCD_Write_DATA(0x11);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0xC0);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);

	// ----------- Gamma  Curve  Set3 Negative----------//
	LCD_Write_COM(0xFD);

	LCD_Write_DATA(0x88);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x42);
	LCD_Write_DATA(0x42);
	LCD_Write_DATA(0x22);
	LCD_Write_DATA(0x11);
	LCD_Write_DATA(0x11);
	LCD_Write_DATA(0x22);
	LCD_Write_DATA(0x99);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0xBB);
	LCD_Write_DATA(0xBB);
	LCD_Write_DATA(0xAA);
	LCD_Write_DATA(0x33);
	LCD_Write_DATA(0x33);
	LCD_Write_DATA(0x11);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x03);

	// ----------- EVRSER Regulator Voltage Setting---------//
	LCD_Write_COM(0xBE);

	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x15);
	LCD_Write_DATA(0x16);
	LCD_Write_DATA(0x08);
	LCD_Write_DATA(0x09);
	LCD_Write_DATA(0x15);
	LCD_Write_DATA(0x10);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);

	// -----------Module Definiton Setting---------//
	LCD_Write_COM(0xC0);

	LCD_Write_DATA(0x0E);
	LCD_Write_DATA(0x01);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);

	// -----------PWRDEF Power Ability Ddfinition----------//
	LCD_Write_COM(0xC1);

	LCD_Write_DATA(0x2F);
	LCD_Write_DATA(0x23);
	LCD_Write_DATA(0xB4);
	LCD_Write_DATA(0xFF);
	LCD_Write_DATA(0x24);
	LCD_Write_DATA(0x03);
	LCD_Write_DATA(0x20);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x02);
	LCD_Write_DATA(0x20);
	LCD_Write_DATA(0x20);
	LCD_Write_DATA(0x00);

	// -----------Other Setting----------//
	LCD_Write_COM(0xC2);
	LCD_Write_DATA(0x03);
	LCD_Write_COM(0x26);
	LCD_Write_DATA(0x08);
	LCD_Write_COM(0x35);
   
	LCD_Write_COM(0x36);
	LCD_Write_DATA(0x64);
	LCD_Write_COM(0x3A);
	LCD_Write_DATA(0x05);
	LCD_Write_COM(0x2A);
	LCD_Write_DATA(0x01,0x3f);
	LCD_Write_COM(0x2B);
	LCD_Write_DATA(0xEF);
	LCD_Write_COM(0x2c);

	// -----------RGB Setting----------//
	LCD_Write_COM(0x2D);
	R=0;
	G=0;
	B=0;   
    
	for(i=0;i<32;i++)
	{ 
		LCD_Write_DATA(R);
		R=R+2;
	}
	for(i=0;i<64;i++)
	{ 
		LCD_Write_DATA(G);
		G=G+1;
	} 
	for(i=0;i<32;i++)
	{ 
		LCD_Write_DATA(B);
		B=B+2;
	}    
	break;
