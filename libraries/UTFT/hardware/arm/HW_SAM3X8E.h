// *** Hardwarespecific functions ***
void UTFT::_hw_special_init()
{
#ifdef EHOUSE_DUE_SHIELD
    pinMode(24, OUTPUT); digitalWrite(24, HIGH); // Set the TFT_RD pin permanently HIGH as it is not supported by UTFT
#endif
}

void UTFT::LCD_Writ_Bus(char VH,char VL, byte mode)
{   
	switch (mode)
	{
	case 1:
		if (display_serial_mode==SERIAL_4PIN)
		{
			if (VH==1)
				sbi(P_SDA, B_SDA);
			else
				cbi(P_SDA, B_SDA);
			pulse_low(P_SCL, B_SCL);
		}
		else
		{
			if (VH==1)
				sbi(P_RS, B_RS);
			else
				cbi(P_RS, B_RS);
		}

		if (VL & 0x80)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		if (VL & 0x40)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		if (VL & 0x20)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		if (VL & 0x10)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		if (VL & 0x08)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		if (VL & 0x04)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		if (VL & 0x02)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		if (VL & 0x01)
			sbi(P_SDA, B_SDA);
		else
			cbi(P_SDA, B_SDA);
		pulse_low(P_SCL, B_SCL);
		break;
	case 8:
#if defined(CTE_DUE_SHIELD) || defined(EHOUSE_DUE_SHIELD)
		REG_PIOC_CODR=0xFF000;
		REG_PIOC_SODR=(VH<<12) & 0xFF000;
		pulse_low(P_WR, B_WR);
		REG_PIOC_CODR=0xFF000;
		REG_PIOC_SODR=(VL<<12) & 0xFF000;
		pulse_low(P_WR, B_WR);
#else
		REG_PIOA_CODR=0x0000C000;
		REG_PIOD_CODR=0x0000064F;
		REG_PIOA_SODR=(VH & 0x06)<<13;
		(VH & 0x01) ? REG_PIOB_SODR = 0x4000000 : REG_PIOB_CODR = 0x4000000;
		REG_PIOD_SODR=((VH & 0x78)>>3) | ((VH & 0x80)>>1);
		pulse_low(P_WR, B_WR);

		REG_PIOA_CODR=0x0000C000;
		REG_PIOD_CODR=0x0000064F;
		REG_PIOA_SODR=(VL & 0x06)<<13;
		(VL & 0x01) ? REG_PIOB_SODR = 0x4000000 : REG_PIOB_CODR = 0x4000000;
		REG_PIOD_SODR=((VL & 0x78)>>3) | ((VL & 0x80)>>1);
		pulse_low(P_WR, B_WR);
#endif
		break;
	case 16:
#if defined(CTE_DUE_SHIELD)
        REG_PIOC_CODR=0xFF1FE;
		REG_PIOC_SODR=(VL<<1) & 0x1FE;
		REG_PIOC_SODR=(VH<<12) & 0xFF000;
#elif defined(EHOUSE_DUE_SHIELD)
		PIOC->PIO_ODSR = ((PIOC->PIO_ODSR&(~0x000FF3FC)) | ((((uint32_t)VL)<<2) | (((uint32_t)VH)<<12)));
#else
		REG_PIOA_CODR=0x0000C080;
		REG_PIOC_CODR=0x0000003E;
		REG_PIOD_CODR=0x0000064F;
		REG_PIOA_SODR=((VH & 0x06)<<13) | ((VL & 0x40)<<1);
		(VH & 0x01) ? REG_PIOB_SODR = 0x4000000 : REG_PIOB_CODR = 0x4000000;
		REG_PIOC_SODR=((VL & 0x01)<<5) | ((VL & 0x02)<<3) | ((VL & 0x04)<<1) | ((VL & 0x08)>>1) | ((VL & 0x10)>>3);
		REG_PIOD_SODR=((VH & 0x78)>>3) | ((VH & 0x80)>>1) | ((VL & 0x20)<<5) | ((VL & 0x80)<<2);
#endif
		pulse_low(P_WR, B_WR);
		break;
	case LATCHED_16:
		asm("nop");		// Mode is unsupported
		break;
	}
}

void UTFT::_set_direction_registers(byte mode)
{
	if (mode!=LATCHED_16)
	{
#if defined(CTE_DUE_SHIELD)
		if (mode==16)
		{
			REG_PIOC_OER=0x000FF1FE;
		}
		else
			REG_PIOC_OER=0x000FF000;
#elif defined(EHOUSE_DUE_SHIELD)
		if (mode==16)
		{
			REG_PIOC_OER=0x000FF3FC;
			REG_PIOC_OWER=0x000FF3FC;
		}
		else
			REG_PIOC_OER=0x000FF000;
#else
		REG_PIOA_OER=0x0000c000; //PA14,PA15 enable
		REG_PIOB_OER=0x04000000; //PB26 enable
		REG_PIOD_OER=0x0000064f; //PD0-3,PD6,PD9-10 enable
		if (mode==16)
		{
			REG_PIOA_OER=0x00000080; //PA7 enable
			REG_PIOC_OER=0x0000003e; //PC1 - PC5 enable
		}
#endif
	}
	else
	{
		asm("nop");		// Mode is unsupported
	}
}

void UTFT::_fast_fill_16(int ch, int cl, long pix)
{
	long blocks;

#if defined(CTE_DUE_SHIELD)
    REG_PIOC_CODR=0xFF1FE;
	REG_PIOC_SODR=(cl<<1) & 0x1FE;
	REG_PIOC_SODR=(ch<<12) & 0xFF000;
#elif defined(EHOUSE_DUE_SHIELD)
	PIOC->PIO_ODSR = ((PIOC->PIO_ODSR&(~0x000FF3FC)) | ((((uint32_t)cl)<<2) | (((uint32_t)ch)<<12)));
#else
	REG_PIOA_CODR=0x0000C080;
	REG_PIOC_CODR=0x0000003E;
	REG_PIOD_CODR=0x0000064F;
	REG_PIOA_SODR=((ch & 0x06)<<13) | ((cl & 0x40)<<1);
	(ch & 0x01) ? REG_PIOB_SODR = 0x4000000 : REG_PIOB_CODR = 0x4000000;
	REG_PIOC_SODR=((cl & 0x01)<<5) | ((cl & 0x02)<<3) | ((cl & 0x04)<<1) | ((cl & 0x08)>>1) | ((cl & 0x10)>>3);
	REG_PIOD_SODR=((ch & 0x78)>>3) | ((ch & 0x80)>>1) | ((cl & 0x20)<<5) | ((cl & 0x80)<<2);
#endif

	blocks = pix/16;
	for (int i=0; i<blocks; i++)
	{
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);
	}
	if ((pix % 16) != 0)
		for (int i=0; i<(pix % 16)+1; i++)
		{
			pulse_low(P_WR, B_WR);
		}
}

void UTFT::_fast_fill_8(int ch, long pix)
{
	long blocks;

#if defined(CTE_DUE_SHIELD) || defined(EHOUSE_DUE_SHIELD)
    REG_PIOC_CODR=0xFF000;
	REG_PIOC_SODR=(ch<<12) & 0xFF000;
#else
	REG_PIOA_CODR=0x0000C000;
	REG_PIOD_CODR=0x0000064F;
	REG_PIOA_SODR=(ch & 0x06)<<13;
	(ch & 0x01) ? REG_PIOB_SODR = 0x4000000 : REG_PIOB_CODR = 0x4000000;
	REG_PIOD_SODR=((ch & 0x78)>>3) | ((ch & 0x80)>>1);
#endif

	blocks = pix/16;
	for (int i=0; i<blocks; i++)
	{
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
	}
	if ((pix % 16) != 0)
		for (int i=0; i<(pix % 16)+1; i++)
		{
			pulse_low(P_WR, B_WR);pulse_low(P_WR, B_WR);
		}
}
