#ifndef __TOUCH_H__
#define __TOUCH_H__
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//测试硬件：单片机STM32F103RBT6,主频72M  单片机工作电压3.3V
//QDtech-TFT液晶驱动 for STM32 IO模拟
//xiao冯@ShenZhen QDtech co.,LTD
//公司网站:www.qdtech.net
//淘宝网站：http://qdtech.taobao.com
//我司提供技术支持，任何技术问题欢迎随时交流学习
//固话(传真) :+86 0755-23594567 
//手机:15989313508（冯工） 
//邮箱:QDtech2008@gmail.com 
//Skype:QDtech2008
//技术交流QQ群:324828016
//创建日期:2013/5/13
//版本：V1.1
//版权所有，盗版必究。
//Copyright(C) 深圳市全动电子技术有限公司 2009-2019
//All rights reserved
//////////////////////////////////////////////////////////////////////////////////
//=========================================触摸屏触接线=========================================//
/***************************************************************************************************
//不使用触摸或者模块本身不带触摸，则可不连接
//MO(MISO)	接PC2	//SPI总线输出
//MI(MOSI)	接PC3	//SPI总线输入
//PEN		接PC1	//触摸屏中断信号
//TCS		接PC13	//触摸IC片选
//CLK		接PC0	//SPI总线时钟
**************************************************************************************************/	

#define TP_PRES_DOWN 0x80  //触屏被按下	  
#define TP_CATH_PRES 0x40  //有按键按下了 	  
										    
//触摸屏控制器
typedef struct
{
	u8 (*init)(void);			//初始化触摸屏控制器
	u8 (*scan)(u8);				//扫描触摸屏.0,屏幕扫描;1,物理坐标;	 
	void (*adjust)(void);		//触摸屏校准
	u16 x0;						//原始坐标(第一次按下时的坐标)
	u16 y0;
	u16 x; 						//当前坐标(此次扫描时,触屏的坐标)
	u16 y;						   	    
	u8  sta;					//笔的状态 
								//b7:按下1/松开0; 
	                            //b6:0,没有按键按下;1,有按键按下.         			  
////////////////////////触摸屏校准参数/////////////////////////								
	float xfac;					
	float yfac;
	short xoff;
	short yoff;	   
//新增的参数,当触摸屏的左右上下完全颠倒时需要用到.
//touchtype=0的时候,适合左右为X坐标,上下为Y坐标的TP.
//touchtype=1的时候,适合左右为Y坐标,上下为X坐标的TP.
	u8 touchtype;
}_m_tp_dev;

extern _m_tp_dev tp_dev;	 	//触屏控制器在touch.c里面定义

//与触摸屏芯片连接引脚	   
//与触摸屏芯片连接引脚	   
#define PEN  PCin(1)   //PC1  INT
#define DOUT PCin(2)   //PC2  MISO
#define TDIN PCout(3)  //PC3  MOSI
#define TCLK PCout(0)  //PC0  SCLK
#define TCS  PCout(13) //PC13 CS    
     

void TP_Write_Byte(u8 num);						//向控制芯片写入一个数据
u16 TP_Read_AD(u8 CMD);							//读取AD转换值
u16 TP_Read_XOY(u8 xy);							//带滤波的坐标读取(X/Y)
u8 TP_Read_XY(u16 *x,u16 *y);					//双方向读取(X+Y)
u8 TP_Read_XY2(u16 *x,u16 *y);					//带加强滤波的双方向坐标读取
void TP_Drow_Touch_Point(u16 x,u16 y,u16 color);//画一个坐标校准点
void TP_Draw_Big_Point(u16 x,u16 y,u16 color);	//画一个大点
u8 TP_Scan(u8 tp);								//扫描
void TP_Save_Adjdata(void);						//保存校准参数
u8 TP_Get_Adjdata(void);						//读取校准参数
void TP_Adjust(void);							//触摸屏校准
u8 TP_Init(void);								//初始化
																 
void TP_Adj_Info_Show(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac);//显示校准信息
 		  
#endif

















