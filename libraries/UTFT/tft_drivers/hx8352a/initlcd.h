case HX8352A:
	LCD_Write_COM(0x83);           
	LCD_Write_DATA(0x02);  //TESTM=1 
             
	LCD_Write_COM(0x85);  
	LCD_Write_DATA(0x03);  //VDC_SEL=011
	LCD_Write_COM(0x8B);  
	LCD_Write_DATA(0x01);
	LCD_Write_COM(0x8C);  
	LCD_Write_DATA(0x93); //STBA[7]=1,STBA[5:4]=01,STBA[1:0]=11
        
	LCD_Write_COM(0x91);  
	LCD_Write_DATA(0x01); //DCDC_SYNC=1
        
	LCD_Write_COM(0x83);  
	LCD_Write_DATA(0x00); //TESTM=0
	//Gamma Setting

	LCD_Write_COM(0x3E);  
	LCD_Write_DATA(0xB0);
	LCD_Write_COM(0x3F);  
	LCD_Write_DATA(0x03);
	LCD_Write_COM(0x40);  
	LCD_Write_DATA(0x10);
	LCD_Write_COM(0x41);  
	LCD_Write_DATA(0x56);
	LCD_Write_COM(0x42);  
	LCD_Write_DATA(0x13);
	LCD_Write_COM(0x43);  
	LCD_Write_DATA(0x46);
	LCD_Write_COM(0x44);  
	LCD_Write_DATA(0x23);
	LCD_Write_COM(0x45);  
	LCD_Write_DATA(0x76);
	LCD_Write_COM(0x46);  
	LCD_Write_DATA(0x00);
	LCD_Write_COM(0x47);  
	LCD_Write_DATA(0x5E);
	LCD_Write_COM(0x48);  
	LCD_Write_DATA(0x4F);
	LCD_Write_COM(0x49);  
	LCD_Write_DATA(0x40);	
	//**********Power On sequence************
        
	LCD_Write_COM(0x17);  
	LCD_Write_DATA(0x91);
       
	LCD_Write_COM(0x2B);  
	LCD_Write_DATA(0xF9);
	delay(10);
        
	LCD_Write_COM(0x1B);  
	LCD_Write_DATA(0x14);
        
	LCD_Write_COM(0x1A);  
	LCD_Write_DATA(0x11);
              
	LCD_Write_COM(0x1C);  
	LCD_Write_DATA(0x06);
        
	LCD_Write_COM(0x1F);  
	LCD_Write_DATA(0x42);
	delay(20);
        
	LCD_Write_COM(0x19);  
	LCD_Write_DATA(0x0A);
     
	LCD_Write_COM(0x19);  
	LCD_Write_DATA(0x1A);
	delay(40);
        
        
	LCD_Write_COM(0x19);  
	LCD_Write_DATA(0x12);
	delay(40);
        
	LCD_Write_COM(0x1E);  
	LCD_Write_DATA(0x27);
	delay(100);	   
        
        
	//**********DISPLAY ON SETTING***********
        
	LCD_Write_COM(0x24);  
	LCD_Write_DATA(0x60);
        
	LCD_Write_COM(0x3D);  
	LCD_Write_DATA(0x40);
        
	LCD_Write_COM(0x34);  
	LCD_Write_DATA(0x38);
        
	LCD_Write_COM(0x35);  
	LCD_Write_DATA(0x38);
        
	LCD_Write_COM(0x24);  
	LCD_Write_DATA(0x38);
	delay(40);
        
	LCD_Write_COM(0x24);  
	LCD_Write_DATA(0x3C);
        
	LCD_Write_COM(0x16);  
	LCD_Write_DATA(0x1C);
        
	LCD_Write_COM(0x01);  
	LCD_Write_DATA(0x06);
        
	LCD_Write_COM(0x55);  
	LCD_Write_DATA(0x00); 

	LCD_Write_COM(0x02);           
	LCD_Write_DATA(0x00);
	LCD_Write_COM(0x03);           
	LCD_Write_DATA(0x00);
	LCD_Write_COM(0x04);           
	LCD_Write_DATA(0x00);
	LCD_Write_COM(0x05);           
	LCD_Write_DATA(0xef);
        
	LCD_Write_COM(0x06);           
	LCD_Write_DATA(0x00);
	LCD_Write_COM(0x07);           
	LCD_Write_DATA(0x00);
	LCD_Write_COM(0x08);           
	LCD_Write_DATA(0x01);
	LCD_Write_COM(0x09);           
	LCD_Write_DATA(0x8f);

	LCD_Write_COM(0x22);
	break;
