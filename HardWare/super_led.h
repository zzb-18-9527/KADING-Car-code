#ifndef __super_led_H
#define __super_led_H

#include <stdint.h>
#include "tim.h"

#define high 50 //“1”占空比
#define low  25 //“0”占空比
#define LED_NUM 150 //灯珠总数
#define DATA_LEN 24 //单个灯珠需要24bit长度的空间
#define WS2812_RST_NUM 250 //芯片复位需要持续的低电平
#define DATA_BUFF_LEN (LED_NUM * DATA_LEN) + WS2812_RST_NUM//所需缓存数组总长度

void super_led_set_single(uint8_t num,uint8_t R,uint8_t G,uint8_t B);
void super_led_init(void);
void Color_Wipe_NonBlocking(void);
void super_led_engine(void);
void super_led_set_whole(uint8_t R,uint8_t G,uint8_t B);

/* 全局控制变量 (供 main.c 实时赋值) */
extern uint8_t g_led_delay;      /* 流水灯速度 */
extern uint8_t g_engine_speed;   /* 引擎色灯油门值 0~19 */
extern uint8_t g_engine_delay;   /* 引擎色灯刷新间隔 */

	
#endif
