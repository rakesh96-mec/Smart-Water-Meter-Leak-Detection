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
#include <stdio.h>
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

volatile uint32_t pulseCount = 0;

uint32_t previousPulseCount = 0;
uint32_t pulsesPerMinute = 0;
uint32_t flowRate_mL_min = 0;

uint32_t DESIGN_FLOW_RATE_ML_MIN = 60000;

uint8_t MINOR_THRESHOLD_PERCENT = 50;
uint8_t MAJOR_THRESHOLD_PERCENT = 75;
uint32_t MINOR_HYSTERESIS = 100;

uint32_t LEAK_CONFIRM_TIME_SEC = 10;

uint32_t minorThreshold = 0;
uint32_t majorThreshold = 0;

uint32_t minorLeakSeconds = 0;
uint32_t majorLeakSeconds = 0;

uint8_t minorRegion = 0;
uint8_t majorRegion = 0;

uint8_t rxData = 0;
volatile uint8_t resetRequest = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */


/* Private user code ---------------------------------------------------------*/
typedef enum
{
    STATUS_NORMAL = 0,

    STATUS_MINOR_MONITORING,

    STATUS_MAJOR_MONITORING,

    STATUS_MINOR_LEAK,

    STATUS_MAJOR_LEAK

} LeakStatus_t;

LeakStatus_t leakStatus = STATUS_NORMAL;


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
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  minorThreshold =
      (DESIGN_FLOW_RATE_ML_MIN * MINOR_THRESHOLD_PERCENT) / 100;

  majorThreshold =
      (DESIGN_FLOW_RATE_ML_MIN * MAJOR_THRESHOLD_PERCENT) / 100;

    /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      char msg[150];

      uint32_t volume_mL = pulseCount * 250;

      pulsesPerMinute = pulseCount - previousPulseCount;
      previousPulseCount = pulseCount;

      flowRate_mL_min = pulsesPerMinute * 250 * 60;

      /*----------------------------------------------------
          Leak Detection State Machine
      -----------------------------------------------------*/

      if(flowRate_mL_min == 0)
      {
          leakStatus = STATUS_NORMAL;

          minorLeakSeconds = 0;
          majorLeakSeconds = 0;
      }

      /*--------------- MINOR REGION ----------------*/

      else if(flowRate_mL_min < minorThreshold)
      {
          majorLeakSeconds = 0;

          if(minorLeakSeconds < LEAK_CONFIRM_TIME_SEC)
          {
              leakStatus = STATUS_MINOR_MONITORING;
          }

          minorLeakSeconds++;

          if(minorLeakSeconds >= LEAK_CONFIRM_TIME_SEC)
          {
              leakStatus = STATUS_MINOR_LEAK;
          }
      }

      /*--------------- NORMAL REGION ----------------*/

      else if(flowRate_mL_min <= majorThreshold)
      {
          leakStatus = STATUS_NORMAL;

          minorLeakSeconds = 0;
          majorLeakSeconds = 0;
      }

      /*--------------- MAJOR REGION ----------------*/

      else
      {
          minorLeakSeconds = 0;

          if(majorLeakSeconds < LEAK_CONFIRM_TIME_SEC)
          {
              leakStatus = STATUS_MAJOR_MONITORING;
          }

          majorLeakSeconds++;

          if(majorLeakSeconds >= LEAK_CONFIRM_TIME_SEC)
          {
              leakStatus = STATUS_MAJOR_LEAK;
          }
      }
      char *statusText;

      switch(leakStatus)
      {
      case STATUS_MINOR_MONITORING:
          statusText = "MINOR MONITORING";
          break;

      case STATUS_MAJOR_MONITORING:
          statusText = "MAJOR MONITORING";
          break;

      case STATUS_MINOR_LEAK:
          statusText = "MINOR LEAK";
          break;

      case STATUS_MAJOR_LEAK:
          statusText = "MAJOR LEAK";
          break;

      default:
          statusText = "NORMAL";
          break;
      }
      if (HAL_UART_Receive(&huart2, &rxData, 1, 1) == HAL_OK)
      {
          if (rxData == 'R')
          {
              leakStatus = STATUS_NORMAL;
              minorLeakSeconds = 0;
              majorLeakSeconds = 0;
          }
      }
      sprintf(msg,
              "%lu,%lu,%s\r\n",
              pulseCount,
              volume_mL,
              statusText);

      HAL_UART_Transmit(&huart2,
                        (uint8_t*)msg,
                        strlen(msg),
                        HAL_MAX_DELAY);

      HAL_Delay(1000);
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == GPIO_PIN_0)
    {
        pulseCount++;
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }
}

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
