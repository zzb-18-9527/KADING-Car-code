/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "OLED.h"
#include "motor.h"
#include "super_led.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define EMA_FACTOR   12       /* EMA 滤波系数: α = 1/EMA_FACTOR, 值越大越平滑但响应会变迟缓 */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV 
*/
	uint16_t ADC_value [3];	//ADC采样值
	uint8_t duty_cycle = 0; //油门CCR原始值0~19
	uint8_t filtered_duty_cycle = 0;   // EMA 滤波后输出值
	uint16_t filtered_accum = 0;       // EMA 历史值
	uint8_t gas = 0;//油门ADC前两位值
	uint8_t last_duty_cycle;  /* 上一次油门值，用于尾灯减速检测 */
	uint32_t tail_light_tick; /* 尾灯亮起的时刻，用于0.5秒延时熄灭 */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */
	
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
	
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_USART3_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
	OLED_Init();
	motor_init();
	super_led_init();
//启动ADC自动校准程序
	HAL_ADCEx_Calibration_Start(&hadc1);
// 启动ADC
	HAL_ADC_Start_DMA(&hadc1,(uint32_t*)&ADC_value,3);
	/*
	内置引脚（输入）
1.油门控制：ADC3.3V PA0
2.总开关 GPIO PB7
3.大灯开关GPIO PB6
4.后退GPIO PB5
5.前进GPIO PB4
6.左灯开关GPIO PB3
7.右灯开关GPIO PA15
8.角度传感器 PA2
9.电压检测 PA1
10.超声波检测模块PA12 
内置引脚（输出）
1.尾灯GPIO PB15
2.左转灯GPIO PB14
3.右转灯GPIO PB13
4.前大灯GPIO PB12
5.风扇 GPIO PA10
6.OLED SCL:PB1 SDA:PA6
7.串口 RX:PB11  TX:PB10
8.彩灯PB8满开2.7A
	*/
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		//油门值转换
	  gas =(ADC_value[0] / 1000) * 10 + ((ADC_value[0] / 100) % 10);//取稳定的前两位数
		duty_cycle = (gas > 9) ? (gas - 9) : 0;   /* gas<9时直接为0，避免uint8下溢到255导致100%占空比过流 */
		
		/* EMA 低通滤波（定点数，8位小数精度，消除整数截断导致的锁死问题） */
		//之前：整数运算会丢失余数导致数据偏小
		//改版：运算时保留小数，只在最后输出时取整
		filtered_accum = (uint16_t)(((uint32_t)filtered_accum * (EMA_FACTOR - 1) + (uint32_t)duty_cycle * 256) / EMA_FACTOR);
		filtered_duty_cycle = (uint8_t)((filtered_accum + 128) / 256);   /* 四舍五入取整输出 */
		
		//OLED屏幕显示
		//油门原始值
		OLED_Printf(0,0,OLED_8X16,"gas:%03d ",(duty_cycle));   /* %03d 显示完整3位，末尾空格防残影 */
		//电池电压值
		uint16_t voltage = ((uint32_t)ADC_value[1] * 250) / 4095; 
		OLED_Printf(0,17,OLED_8X16,"voltage:%02d.%dV",voltage/10,voltage%10);
		//角度传感器值
		uint16_t angle = ((uint32_t)ADC_value[2] * 360) / 4095;   // 角度0~360
		OLED_Printf(0,33,OLED_8X16,"angle:%03d",angle);
		OLED_Update();

//总控开关
if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_7) == GPIO_PIN_SET)
{
    //前大灯控制
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_SET) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    }
//	转向灯控制
		//左
		if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_3) == GPIO_PIN_SET) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    }
		//右
		if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_15) == GPIO_PIN_SET) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    }

		//油门控制（档把）
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_4) == GPIO_PIN_SET) {
        /* ========== 前进状态 ========== */
				//电门
        motor_control(forward, filtered_duty_cycle);

        /* 尾灯(刹车)逻辑: 油门减小(减速)时亮起，亮起后延时0.5秒再熄灭（非阻塞） */
        if (filtered_duty_cycle < last_duty_cycle) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
            tail_light_tick = HAL_GetTick();   /* 记录尾灯亮起的时刻 */
        } else if ((HAL_GetTick() - tail_light_tick) >= 500) {
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
        }
        last_duty_cycle = filtered_duty_cycle;   /* 记录本次油门值供下次比较 */

        /* 状态灯: 油门越大 → 颜色越红 */
        g_engine_speed = filtered_duty_cycle;             /* 设置油门值 0~19 */
        super_led_engine();                      /* 非阻塞刷新 */

        /* 主动散热(风扇) */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);

    } else if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_5) == GPIO_PIN_SET) {
        /* ========== 后退状态 ========== */
			//超声波传感器控制状态（油门以及状态灯）
			//当感应到障碍物急刹并且状态灯由绿转红
				if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_12) == GPIO_PIN_RESET)
				{
					motor_control(back, filtered_duty_cycle); 
					super_led_set_whole(0, 255, 0);
				}
        else if(HAL_GPIO_ReadPin(GPIOA,GPIO_PIN_12) == GPIO_PIN_SET)
				{
					motor_control(stop,0);
					super_led_set_whole(255, 0, 0);
				}

        /* 风扇: 倒车时关闭主动散热 */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);

        /* 倒车警示灯持续亮起 (左转，右转，尾灯) */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);
        

    } else {
        /* ========== 停止状态(手刹/默认) ========== */
        motor_control(stop, 0);

        /* 风扇: 驻车时开启散热 */
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_SET);

        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);   /* 刹车灯常亮 */
        Color_Wipe_NonBlocking();//炫彩流水灯
    }

}
else
{
//关闭大灯
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
//关闭灯带
		super_led_set_whole(0,0,0);
//关闭尾灯
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
//关闭左右转灯
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
//关闭风扇
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_10, GPIO_PIN_RESET);
}
	}

		
       
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
//  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s GPIO_PIN_SET line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
