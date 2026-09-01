#include "stm32f1xx_hal.h"
#include "tim.h"
#include "super_led.h"

/* DMA 颜色数据缓存 */
uint16_t dma_buffer[DATA_BUFF_LEN] = {0};

/* 彩虹流水灯状态 */
static uint16_t wipe_step = 0;
static uint32_t wipe_tick = 0;

/* 引擎色灯状态 */
static uint32_t engine_tick = 0;

/* 全局控制变量 */
uint8_t g_led_delay = 80;      /* 颜色流动速度(ms)，值越大越慢 */
uint8_t g_engine_speed = 0;    /* 油门值 0~19 */
uint8_t g_engine_delay = 50;   /* 引擎色灯刷新间隔(ms) */

/**
 * 设置单颗 LED 颜色
 * @param num LED 索引（0 ~ LED_NUM-1）
 * @param R,G,B RGB 颜色分量
 */
void super_led_set_single(uint8_t num, uint8_t R, uint8_t G, uint8_t B)
{
    if (num >= LED_NUM) return;
    uint32_t color = (G << 16) | (R << 8) | B;
    for (int i = 0; i < 24; i++) {
        if (color & (1 << (23 - i)))
            dma_buffer[num * 24 + i] = high;
        else
            dma_buffer[num * 24 + i] = low;
    }
}

/**
 * 设置全部 LED 为同一颜色
 */
void super_led_set_whole(uint8_t R, uint8_t G, uint8_t B)
{
    for (uint8_t i = 0; i < LED_NUM; i++) {
        super_led_set_single(i, R, G, B);
    }
}

/**
 * 初始化 WS2812 灯带，启动 TIM4 CH3 PWM-DMA
 */
void super_led_init(void)
{
    /* 全部 LED 灭 */
    for (uint8_t i = 0; i < LED_NUM; i++) {
        super_led_set_single(i, 0, 0, 0);
    }
    /* 复位周期（低电平） */
    for (uint16_t i = LED_NUM * DATA_LEN; i < LED_NUM * DATA_LEN + WS2812_RST_NUM; i++) {
        dma_buffer[i] = 0;
    }
    HAL_TIM_PWM_Start_DMA(&htim4, TIM_CHANNEL_3, (uint32_t *)&dma_buffer, DATA_BUFF_LEN);
}

/* 9 色调色板（紫 → 紫罗兰 → 品红 → 中等紫罗兰红 → 深紫罗兰 → 深品红 → 深紫罗兰 → 紫 → 靛蓝） */
static const uint8_t color_sequence[9][3] = {
    {221, 160, 221},
    {218, 112, 214},
    {255, 0,   255},
    {186, 85,  211},
    {138, 43,  226},
    {148, 0,   211},
    {153, 50,  204},
    {128, 0,   128},
    {75,  0,   130},
};

/**
 * 非阻塞彩虹流动灯
 * 所有灯常亮，颜色随时间流动变化
 */
void Color_Wipe_NonBlocking(void)
{
    if (HAL_GetTick() - wipe_tick < g_led_delay) return;
    wipe_tick = HAL_GetTick();

    for (uint8_t i = 0; i < LED_NUM; i++) {
        uint8_t idx = (i + wipe_step) % 9;
        super_led_set_single(i, color_sequence[idx][0],
                                color_sequence[idx][1],
                                color_sequence[idx][2]);
    }

    wipe_step = (wipe_step + 1) % 9;
}

/**
 * 引擎转速色灯（非阻塞）
 * 根据 g_engine_speed（0~19）线性映射颜色：
 *   0 → 黄色 (255,255,0)    19 → 红色 (255,0,0)
 */
void super_led_engine(void)
{
    if (HAL_GetTick() - engine_tick < g_engine_delay) return;
    engine_tick = HAL_GetTick();

    uint8_t G = 255 - (g_engine_speed * 255 / 19);
    super_led_set_whole(255, G, 0);
}
