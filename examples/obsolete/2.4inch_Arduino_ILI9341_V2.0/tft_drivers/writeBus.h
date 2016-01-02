//Write 8Bit data into LCD register
//使用8位并行数据总线模式 向液晶屏指令寄存器发送数据
void LCD_WR_REG(u8 data)
{ 
	LCD_RS_CLR;
	LCD_CS_CLR;
	DATAOUT(data);
	LCD_WR_CLR;
	LCD_WR_SET;
	LCD_CS_SET;	
}
//Write 8Bit data into LCD DataRegister
//使用8位并行数据总线模式 向液晶屏数据寄存器发送数据
void LCD_WR_DATA(u8 data)
{
	LCD_RS_SET;
	LCD_CS_CLR;
	DATAOUT(data);
	LCD_WR_CLR;
	LCD_WR_SET;
	LCD_CS_SET;
}
//Write 16Bit data into LCD GRAM
//使用8位并行数据总线模式 向液晶屏GRAM发送一个16位颜色数据
void LCD_WR_DATA_16Bit(u16 color)
{
	u8 DH,DL;
	DH=color>>8;
	DL=color;
	LCD_CS_CLR;
	LCD_RD_SET;
	LCD_RS_SET; 
	DATAOUT(DH);//高8位
	LCD_WR_CLR;
	LCD_WR_SET;	
	DATAOUT(DL);//低8位	
	LCD_WR_CLR;
	LCD_WR_SET;	 
	LCD_CS_SET;
}

