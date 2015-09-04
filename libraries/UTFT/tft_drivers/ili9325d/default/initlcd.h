case ILI9325D_8:
case ILI9325D_16:
	LCD_Write_COM_DATA(0xE5, 0x78F0); // set SRAM internal timing
	LCD_Write_COM_DATA(0x01, 0x0100); // set Driver Output Control  
	LCD_Write_COM_DATA(0x02, 0x0200); // set 1 line inversion  
	LCD_Write_COM_DATA(0x03, 0x1030); // set GRAM write direction and BGR=1.  
	LCD_Write_COM_DATA(0x04, 0x0000); // Resize register  
	LCD_Write_COM_DATA(0x08, 0x0207); // set the back porch and front porch  
	LCD_Write_COM_DATA(0x09, 0x0000); // set non-display area refresh cycle ISC[3:0]  
	LCD_Write_COM_DATA(0x0A, 0x0000); // FMARK function  
	LCD_Write_COM_DATA(0x0C, 0x0000); // RGB interface setting  
	LCD_Write_COM_DATA(0x0D, 0x0000); // Frame marker Position  
	LCD_Write_COM_DATA(0x0F, 0x0000); // RGB interface polarity  
	//*************Power On sequence ****************//  
	LCD_Write_COM_DATA(0x10, 0x0000); // SAP, BT[3:0], AP, DSTB, SLP, STB  
	LCD_Write_COM_DATA(0x11, 0x0007); // DC1[2:0], DC0[2:0], VC[2:0]  
	LCD_Write_COM_DATA(0x12, 0x0000); // VREG1OUT voltage  
	LCD_Write_COM_DATA(0x13, 0x0000); // VDV[4:0] for VCOM amplitude  
	LCD_Write_COM_DATA(0x07, 0x0001);  
	delay(200); // Dis-charge capacitor power voltage  
	LCD_Write_COM_DATA(0x10, 0x1690); // SAP, BT[3:0], AP, DSTB, SLP, STB  
	LCD_Write_COM_DATA(0x11, 0x0227); // Set DC1[2:0], DC0[2:0], VC[2:0]  
	delay(50); // Delay 50ms  
	LCD_Write_COM_DATA(0x12, 0x000D); // 0012  
	delay(50); // Delay 50ms  
	LCD_Write_COM_DATA(0x13, 0x1200); // VDV[4:0] for VCOM amplitude  
	LCD_Write_COM_DATA(0x29, 0x000A); // 04  VCM[5:0] for VCOMH  
	LCD_Write_COM_DATA(0x2B, 0x000D); // Set Frame Rate  
	delay(50); // Delay 50ms  
	LCD_Write_COM_DATA(0x20, 0x0000); // GRAM horizontal Address  
	LCD_Write_COM_DATA(0x21, 0x0000); // GRAM Vertical Address  
	// ----------- Adjust the Gamma Curve ----------//  
	LCD_Write_COM_DATA(0x30, 0x0000);  
	LCD_Write_COM_DATA(0x31, 0x0404);  
	LCD_Write_COM_DATA(0x32, 0x0003);  
	LCD_Write_COM_DATA(0x35, 0x0405);  
	LCD_Write_COM_DATA(0x36, 0x0808);  
	LCD_Write_COM_DATA(0x37, 0x0407);  
	LCD_Write_COM_DATA(0x38, 0x0303);  
	LCD_Write_COM_DATA(0x39, 0x0707);  
	LCD_Write_COM_DATA(0x3C, 0x0504);  
	LCD_Write_COM_DATA(0x3D, 0x0808);  
	//------------------ Set GRAM area ---------------//  
	LCD_Write_COM_DATA(0x50, 0x0000); // Horizontal GRAM Start Address  
	LCD_Write_COM_DATA(0x51, 0x00EF); // Horizontal GRAM End Address  
	LCD_Write_COM_DATA(0x52, 0x0000); // Vertical GRAM Start Address  
	LCD_Write_COM_DATA(0x53, 0x013F); // Vertical GRAM Start Address  
	LCD_Write_COM_DATA(0x60, 0xA700); // Gate Scan Line  
	LCD_Write_COM_DATA(0x61, 0x0001); // NDL,VLE, REV   
	LCD_Write_COM_DATA(0x6A, 0x0000); // set scrolling line  
	//-------------- Partial Display Control ---------//  
	LCD_Write_COM_DATA(0x80, 0x0000);  
	LCD_Write_COM_DATA(0x81, 0x0000);  
	LCD_Write_COM_DATA(0x82, 0x0000);  
	LCD_Write_COM_DATA(0x83, 0x0000);  
	LCD_Write_COM_DATA(0x84, 0x0000);  
	LCD_Write_COM_DATA(0x85, 0x0000);  
	//-------------- Panel Control -------------------//  
	LCD_Write_COM_DATA(0x90, 0x0010);  
	LCD_Write_COM_DATA(0x92, 0x0000);  
	LCD_Write_COM_DATA(0x07, 0x0133); // 262K color and display ON        
	break;
