// *** Hardwarespecific functions ***
void UTFT::_hw_special_init()
{
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
		cport(PORTC, 0xBF);
		cport(PORTD, 0x60);
		cport(PORTE, 0xBF);
		PORTC += ((VH & 0x20)<<1);
		PORTD += ((VH & 0x40)<<1) + (VH & 0x10) + ((VH & 0x08)>>3) + ((VH & 0x04)>>1) + ((VH & 0x03)<<2);
		PORTE += ((VH & 0x80)>>1);
		pulse_low(P_WR, B_WR);

		cport(PORTC, 0xBF);
		cport(PORTD, 0x60);
		cport(PORTE, 0xBF);
		PORTC += ((VL & 0x20)<<1);
		PORTD += ((VL & 0x40)<<1) + (VL & 0x10) + ((VL & 0x08)>>3) + ((VL & 0x04)>>1) + ((VL & 0x03)<<2);
		PORTE += ((VL & 0x80)>>1);
		pulse_low(P_WR, B_WR);
		break;
	case 16:
		cport(PORTB, 0x0F);
		cport(PORTC, 0x3F);
		cport(PORTD, 0x20);
		cport(PORTE, 0xBF);
		cport(PORTF, 0x3F);

		PORTB |= ((VL & 0x0F)<<4);
		PORTC |= ((VL & 0x20)<<2) + ((VH & 0x20)<<1);
		PORTD |= ((VH & 0x40)<<1) + (VH & 0x10) + ((VH & 0x08)>>3) + ((VH & 0x04)>>1) + ((VH & 0x03)<<2) + ((VL & 0x10)<<2);
		PORTE |= ((VH & 0x80)>>1);
		PORTF |= ((VL & 0x80)>>1) + ((VL & 0x40)<<1);

		pulse_low(P_WR, B_WR);
		break;
	case LATCHED_16:
		cport(PORTC, 0xBF);
		cport(PORTD, 0x60);
		cport(PORTE, 0xBF);
		PORTC += ((VH & 0x20)<<1);
		PORTD += ((VH & 0x40)<<1) + (VH & 0x10) + ((VH & 0x08)>>3) + ((VH & 0x04)>>1) + ((VH & 0x03)<<2);
		PORTE += ((VH & 0x80)>>1);
		cbi(P_ALE, B_ALE);
		pulse_high(P_ALE, B_ALE);
		cbi(P_CS, B_CS);
		cport(PORTC, 0xBF);
		cport(PORTD, 0x60);
		cport(PORTE, 0xBF);
		PORTC += ((VL & 0x20)<<1);
		PORTD += ((VL & 0x40)<<1) + (VL & 0x10) + ((VL & 0x08)>>3) + ((VL & 0x04)>>1) + ((VL & 0x03)<<2);
		PORTE += ((VL & 0x80)>>1);
		pulse_low(P_WR, B_WR);
		sbi(P_CS, B_CS);
		break;
	}
}

void UTFT::_set_direction_registers(byte mode)
{
	switch (mode)
	{
	case 8:
	case LATCHED_16:
		DDRC |= 0x40;
		DDRD |= 0x9F;
		DDRE |= 0x40;
		break;
	case 16:
		DDRB |= 0xF0;
		DDRC |= 0xC0;
		DDRD |= 0xDF;
		DDRE |= 0x40;
		DDRF |= 0xC0;
		break;
	}
}

void UTFT::_fast_fill_16(int ch, int cl, long pix)
{
	long blocks;

	cport(PORTB, 0x0F);
	cport(PORTC, 0x3F);
	cport(PORTD, 0x20);
	cport(PORTE, 0xBF);
	cport(PORTF, 0x3F);

	PORTB |= ((cl & 0x0F)<<4);
	PORTC |= ((cl & 0x20)<<2) + ((ch & 0x20)<<1);
	PORTD |= ((ch & 0x40)<<1) + (ch & 0x10) + ((ch & 0x08)>>3) + ((ch & 0x04)>>1) + ((ch & 0x03)<<2) + ((cl & 0x10)<<2);
	PORTE |= ((ch & 0x80)>>1);
	PORTF |= ((cl & 0x80)>>1) + ((cl & 0x40)<<1);

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

	cport(PORTC, 0xBF);
	cport(PORTD, 0x60);
	cport(PORTE, 0xBF);

	PORTC |= ((ch & 0x20)<<1);
	PORTD |= ((ch & 0x40)<<1) + (ch & 0x10) + ((ch & 0x08)>>3) + ((ch & 0x04)>>1) + ((ch & 0x03)<<2);
	PORTE |= ((ch & 0x80)>>1);

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
