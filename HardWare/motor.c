#include "stm32f1xx_hal.h"
#include "tim.h"


/**
	* 函    数：电机PWM初始化
  * 参    数：无
  * 返 回 值：无
  * 说    明：无
  */
void motor_init(void)
{
	// 启动通道 1 ,2
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
}

/**
  * 函    数：控制电机转动
  * 参    数：电机控制模式，PWM参数
  * 返 回 值：无
  * 说    明：5K HZ 占空比0~20 死区1.5uS
		//#define forward 1
    //#define back 0
    //#define stop 2
    //#define free 3
			chanel是上半桥
  */
void motor_control(uint8_t state,uint8_t compare)
{
	switch(state)
	{
		case 0 :
//		倒车
//		设置左半桥
			__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_1,compare);
//		设置右半桥
   		__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_2,0);
			break;
		case 1 :
//    前进
//		设置左半桥
			__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_1,0);
//		设置右半桥
   		__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_2,compare);
			break;
		case 2 :
//		刹车
//		设置左半桥
			__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_1,0);
//		设置右半桥
   		__HAL_TIM_SetCompare(&htim1,TIM_CHANNEL_2,0);
			break;
	}
	
}
