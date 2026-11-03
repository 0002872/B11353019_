/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 電影神槍手反應模擬器 (PC5/PC6/PC8 外接燈號版本)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_IDLE = 0,     // 初始閒置狀態，等待指令或按鍵
    STATE_WAITING,      // 黃色警戒燈亮起，隨機等待 1~4 秒
    STATE_GO,           // 綠色號令燈亮起，開始計時拔槍
    STATE_RESULT        // 結束並等待回報
} GameState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile GameState current_state = STATE_IDLE;
volatile uint32_t reaction_time = 0;       // 紀錄毫秒反應時間
volatile uint32_t random_delay_ticks = 0;  // 隨機等待時間計數
volatile uint32_t timeout_counter = 0;     // 綠燈亮起後的逾時計數 (1秒 = 1000ms)
char uart_rx_buf[10];                      // UART 接收緩衝區
uint8_t rx_data;                           // UART 單字元接收
uint32_t best_time = 9999;                 // 最佳成績紀錄 (預設最大值)
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 重新導向 printf 到 USART2 終端機
int __io_putchar(int ch) {
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}

// 啟動遊戲的準備函式（亮黃燈、隨機生成 1~4 秒等待時間）
void Start_Game_Preparation(void) {
    current_state = STATE_WAITING;

    // 關閉其他燈號，點亮外接黃色警戒燈 (PC6)
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET); // 綠燈滅
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);   // 黃燈亮
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET); // 紅燈滅

    // 隨機等待 1000ms ~ 4000ms (1~4秒)
    random_delay_ticks = (rand() % 3000) + 1000;
    printf("\r\n[SYSTEM] 專注！準備拔槍...\r\n");
}

// 觸發綠燈號令 (Go!)
void Trigger_Go_Signal(void) {
    current_state = STATE_GO;

    // 黃燈滅，外接綠色拔槍號令燈 (PC5) 亮起
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);

    reaction_time = 0;     // 毫秒計數歸零
    timeout_counter = 0;   // 逾時計數歸零

    // 啟動 TIM2 毫秒中斷計時器
    HAL_TIM_Base_Start_IT(&htim2);
    printf("\r\n>>> FIRE! (拔槍！) <<<\r\n");
}
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
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // 啟動 UART 接收中斷
  HAL_UART_Receive_IT(&huart2, &rx_data, 1);

  // 隨機亂數種子初始化
  srand(1234);

  // 開機自我測試：依序閃爍綠(PC5)、黃(PC6)、紅(PC8)三燈一次，確認外接電路正常！
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET); HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET); HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET); HAL_Delay(200);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_RESET);

  printf("\r\n========================================\r\n");
  printf("   電影神槍手反應模擬器 (PC5/6/8 版)    \r\n");
  printf("   請按下板上藍色按鍵(B1)開始遊戲       \r\n");
  printf("========================================\r\n");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      // 狀態 1：黃燈預備等待期間的非同步計時與隨機等待處理
      if (current_state == STATE_WAITING) {
          if (random_delay_ticks > 0) {
              HAL_Delay(1); // 延遲 1ms
              random_delay_ticks--;
              if (random_delay_ticks == 0) {
                  Trigger_Go_Signal(); // 時間到，轉為綠燈開火號令
              }
          }
      }

      // 狀態 2：綠燈亮起後的逾時檢查 (超過 1 秒未拔槍)
      if (current_state == STATE_GO) {
          if (timeout_counter >= 1000) { // 超過 1000ms (1秒)
              HAL_TIM_Base_Stop_IT(&htim2); // 停止計時器
              current_state = STATE_IDLE;

              // 逾時轉外接紅燈 (PC8)
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
              HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);

              printf("[TIMEOUT] 逾時未拔槍！勝敗乃兵家常事，少俠請重新來過。\r\n");
          }
      }
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
  RCC_OscInitStruct.PLL.PLLN = 85;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 169;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart2, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart2, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart2) != HAL_OK)
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PC5 PC6 PC8 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_8;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
// 外部中斷回呼函式：處理 PC13 (板上 B1 藍色按鍵)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == GPIO_PIN_13) {

        // 情況 A：系統處於初始狀態，按下 B1 啟動遊戲
        if (current_state == STATE_IDLE) {
            Start_Game_Preparation();
        }

        // 情況 B：黃燈預備等待期間按了按鍵 = 違規搶跑
        else if (current_state == STATE_WAITING) {
            current_state = STATE_IDLE;

            // 熄滅黃燈，點亮外接紅燈 (PC8)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_8, GPIO_PIN_SET);

            printf("[EARLY / FAIL] 犯規！號令燈尚未亮起，你偷跑了！\r\n");
        }

        // 情況 C：綠燈亮起後按下 = 成功拔槍開火
        else if (current_state == STATE_GO) {
            HAL_TIM_Base_Stop_IT(&htim2); // 停止計時器
            current_state = STATE_IDLE;

            // 熄滅外接綠燈 (PC5)
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);

            uint32_t final_time = reaction_time;
            if (final_time < best_time) {
                best_time = final_time;
            }

            printf("[SUCCESS] Reaction Time: %lu ms, SUCCESS!\r\n", final_time);
            printf("[RECORD] 當次成績: %lu ms | 歷史最佳 (Best Time): %lu ms\r\n", final_time, best_time);
        }
    }
}

// TIM2 週期性中斷回呼函式：每 1ms 觸發一次
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        if (current_state == STATE_GO) {
            reaction_time++;
            timeout_counter++;
        }
    }
}

// UART 接收完成中斷回呼函式
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        static uint8_t idx = 0;

        if (rx_data == '\r' || rx_data == '\n') {
            uart_rx_buf[idx] = '\0';
            if (strcmp(uart_rx_buf, "new") == 0) {
                if (current_state == STATE_IDLE) {
                    Start_Game_Preparation();
                }
            }
            idx = 0;
        } else {
            if (idx < sizeof(uart_rx_buf) - 1) {
                uart_rx_buf[idx++] = rx_data;
            }
        }
        HAL_UART_Receive_IT(&huart2, &rx_data, 1);
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
