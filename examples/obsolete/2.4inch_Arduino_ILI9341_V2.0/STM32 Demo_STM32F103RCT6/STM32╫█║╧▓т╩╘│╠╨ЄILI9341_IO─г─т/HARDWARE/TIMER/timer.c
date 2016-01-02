#include "timer.h"
#include "led.h"
#include "usart.h"
#include "sys.h"  	 

void Tim3_Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

	/* Time Base configuration */
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = 999;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; // 输出模式
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 1000; // 占空比参数
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC3Init(TIM3, &TIM_OCInitStructure);

    TIM_ARRPreloadConfig(TIM3, ENABLE); // 这个记得要开
    TIM_Cmd(TIM3, ENABLE);
}

//输入范围0-100
void Tim3_PWM(u8 Value)
{
 	TIM_OCInitTypeDef TIM_OCStructure;	 	//计数器输出比较初始化结构体

	TIM_OCStructure.TIM_OCMode = TIM_OCMode_PWM1;	   //PWM1模式
	TIM_OCStructure.TIM_OutputState = TIM_OutputState_Enable; 
	TIM_OCStructure.TIM_Pulse = Value*10;		//脉冲值 赋给CCR寄存器
	TIM_OCStructure.TIM_OCPolarity = TIM_OCPolarity_High;//比较匹配输出高电平
	TIM_OC3Init(TIM3, &TIM_OCStructure);  //使以上参数有效
}

