/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>

#include "lcd5110.h"
#include "rtc.h"
#include "keyb.h"
#include "queue.h"
#include "interface.h"
#include "problem.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// ds3231 buffer
uint8_t aTxBuffer[18];
// time variables (tim == timer)
uint8_t sec, min, hour, day, date, month, year, tim_sec, tim_min;
/*
 * tim, alrm - timer/alarm flags
 * difficulty - problem difficulty
 * hash - number of hashes left to press to get a hint
 * hint - hint flag
 * i - inputt index
 */
uint8_t tim = 0, alrm = 0, i = 0, difficulty = 0, hash = 4, hint = 0;
// array that stores all the input
uint8_t input[7] = {0, 0, 0, 0, 0, 0, 0};
// problems related numbers
int num1, num2, num3, num4, enum1, enum2, unum1, unum2;
// cmenu - current menu
menu cmenu = main_menu;
// keyboard variable
keyboard board;
// display variable
LCD5110_display lcd1, lcd2;

// alarm queue initialization
queue xq = {{}, 0};
queue* q = &xq;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
// in case of getting interrupt
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	//check what button was pressed
	keyboard_on_input(&board);
}
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
  MX_I2C1_Init();
  MX_SPI2_Init();
  MX_SPI3_Init();
  /* USER CODE BEGIN 2 */
  // first lcd5110 display init
  lcd1.hw_conf.spi_handle = &hspi2;
  lcd1.hw_conf.spi_cs_pin =  LCD1_CS_Pin;
  lcd1.hw_conf.spi_cs_port = LCD1_CS_GPIO_Port;
  lcd1.hw_conf.rst_pin =  LCD1_RST_Pin;
  lcd1.hw_conf.rst_port = LCD1_RST_GPIO_Port;
  lcd1.hw_conf.dc_pin =  LCD1_DC_Pin;
  lcd1.hw_conf.dc_port = LCD1_DC_GPIO_Port;
  lcd1.def_scr = lcd5110_def_scr;
  LCD5110_init(&lcd1.hw_conf, LCD5110_NORMAL_MODE, 0x40, 2, 3);

  // second lcd5110 display init
  lcd2.hw_conf.spi_handle = &hspi3;
  lcd2.hw_conf.spi_cs_pin =  LCD2_CS_Pin;
  lcd2.hw_conf.spi_cs_port = LCD2_CS_GPIO_Port;
  lcd2.hw_conf.rst_pin =  LCD2_RST_Pin;
  lcd2.hw_conf.rst_port = LCD2_RST_GPIO_Port;
  lcd2.hw_conf.dc_pin =  LCD2_DC_Pin;
  lcd2.hw_conf.dc_port = LCD2_DC_GPIO_Port;
  lcd2.def_scr = lcd5110_def_scr;
  LCD5110_init(&lcd2.hw_conf, LCD5110_NORMAL_MODE, 0x40, 2, 3);

  // keyboard initialization
  uint16_t output_pins[] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11};
	uint16_t input_pins[] = {GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3};
	keyboard new_board = {GPIOD, GPIOD, input_pins, output_pins, *on_number, *on_choice};
	board = new_board;
	keyboard_init(&board);

	// reset system
	reset_alarm();
	reset_timer();

  // turn off buzzer
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, 1);
  // turn on displays
	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // get aTxBuffer
    get_buffer();

    // turn all BCD numbers into decimal
    sec = toDEC(aTxBuffer[0]);
    min = toDEC(aTxBuffer[1]);
    hour = toDEC(aTxBuffer[2]);
    day = toDEC(aTxBuffer[3]);
    date = toDEC(aTxBuffer[4]);
    month = toDEC(aTxBuffer[5]);
    year = toDEC(aTxBuffer[6]);
    tim_sec = toDEC(aTxBuffer[7]);
    tim_min = toDEC(aTxBuffer[8]);

    //timer activation handler
    on_timer();

    //alarm activation handler
    on_alarm();

    //first display info
    display1();

    //second display info
    display2();

    HAL_Delay(100);

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
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

  /**Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
