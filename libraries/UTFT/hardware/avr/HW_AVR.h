void UTFT::_convert_float(char *buf, double num, int width, byte prec)
{
	dtostrf(num, width, prec, buf);
}
