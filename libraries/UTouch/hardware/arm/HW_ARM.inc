// Results becomes inaccurate if direct port manipulation is used hence the use of digitalWrite() and digitalRead()

void UTouch::touch_WriteData(byte data)
{
	byte temp;

	temp=data;
	cbi(P_CLK, B_CLK);

	for(byte count=0; count<8; count++)
	{
		if(temp & 0x80)
			digitalWrite(T_DIN, HIGH);
		else
			digitalWrite(T_DIN, LOW);
		temp = temp << 1; 
		digitalWrite(T_CLK, LOW);
		digitalWrite(T_CLK, HIGH);
	}
}

word UTouch::touch_ReadData()
{
	word data = 0;

	for(byte count=0; count<12; count++)
	{
		data <<= 1;
		digitalWrite(T_CLK, HIGH);
		digitalWrite(T_CLK, LOW);
		if (digitalRead(T_DOUT))
			data++;
	}
	return(data);
}

