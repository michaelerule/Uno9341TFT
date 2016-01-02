#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"
////////////////////////////////////////////////////////////////////////////////// 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//定时器 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/14
//版本：V1.4
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//********************************************************************************
//V1.1 20120904
//1,增加TIM3_PWM_Init函数。
//2,增加LED0_PWM_VAL宏定义，控制TIM3_CH2脉宽
//V1.2 20120904
//1,新增TIM5_Cap_Init函数
//2,新增TIM5_IRQHandler中断服务函数	
//V1.3 20120908
//1,新增TIM4_PWM_Init函数	
//V1.4 20120914
//1,增加TIM6_Int_Init函数。
//2,增加TIM6_IRQHandler函数
////////////////////////////////////////////////////////////////////////////////// 

//通过改变TIM3->CCR2的值来改变占空比，从而控制LED0的亮度
#define LED0_PWM_VAL TIM3->CCR2    
//TIM4 CH1作为PWM DAC的输出通道 
#define PWM_DAC_VAL  TIM4->CCR1 

void TIM3_Int_Init(u16 arr,u16 psc);
void TIM3_PWM_Init(u16 arr,u16 psc);
void TIM5_Cap_Init(u16 arr,u16 psc);
void TIM4_PWM_Init(u16 arr,u16 psc);
void TIM6_Int_Init(u16 arr,u16 psc);
void Tim3_Init(void);
void Tim3_PWM(u8 Value);
#endif























