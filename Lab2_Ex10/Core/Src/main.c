/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  *                   Lab 2 - Exercise 10 - Hieu ung dich chu A sang trai tren LED matrix
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* LED 7 doan common anode -> doan sang khi xuat muc 0 */
#define SEG_ON            GPIO_PIN_RESET
#define SEG_OFF           GPIO_PIN_SET

/* PNP: xuat muc 0 -> transistor dan -> LED 7 doan duoc cap nguon */
#define EN_ON             GPIO_PIN_RESET
#define EN_OFF            GPIO_PIN_SET
/* ULN2803 la mang transistor NPN: chan vao muc 1 -> cot duoc chon */
#define MAX_LED_MATRIX    8
#define MATRIX_COL_ON     GPIO_PIN_SET
#define MATRIX_COL_OFF    GPIO_PIN_RESET
#define MATRIX_ROW_ON     GPIO_PIN_SET
#define MATRIX_ROW_OFF    GPIO_PIN_RESET

#define MAX_LED           4
#define CLOCK_TICK_MS     1000
#define SWITCH_TIME_MS    (1000 / MAX_LED)         /* 250 ms */
#define MATRIX_SCAN_MS    10                       /* quet matrix   */
#define SHIFT_TIME_MS     200                      /* toc do chay chu */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int index_led = 0;
int led_buffer[MAX_LED] = {0, 0, 0, 0};
int dot_state = 0;

/* --- Dong ho: gio / phut / giay --- */
int hour = 15, minute = 8, second = 50;

/* ====================== SOFTWARE TIMER ======================
   Moi lan ngat TIM2 (10 ms) thi timer_run() dem lui 1 nhip.
   Dem ve 0 thi dung co timerX_flag cho main() biet "da den han". */
#define TIMER_CYCLE   10          /* chu ky ngat TIM2, don vi ms */

int timer0_counter = 0;
int timer0_flag    = 0;

void setTimer0(int duration)
{
  timer0_counter = duration / TIMER_CYCLE;   /* doi ms -> so nhip ngat */
  timer0_flag    = 0;
}

int timer1_counter = 0;
int timer1_flag    = 0;

void setTimer1(int duration)
{
  timer1_counter = duration / TIMER_CYCLE;   /* doi ms -> so nhip ngat */
  timer1_flag    = 0;
}

int timer2_counter = 0;
int timer2_flag    = 0;

void setTimer2(int duration)
{
  timer2_counter = duration / TIMER_CYCLE;   /* doi ms -> so nhip ngat */
  timer2_flag    = 0;
}

int timer3_counter = 0;
int timer3_flag    = 0;

void setTimer3(int duration)
{
  timer3_counter = duration / TIMER_CYCLE;   /* doi ms -> so nhip ngat */
  timer3_flag    = 0;
}

void timer_run(void)
{
  if (timer0_counter > 0) {
    timer0_counter--;
    if (timer0_counter == 0) timer0_flag = 1;
  }
  if (timer1_counter > 0) {
    timer1_counter--;
    if (timer1_counter == 0) timer1_flag = 1;
  }
  if (timer2_counter > 0) {
    timer2_counter--;
    if (timer2_counter == 0) timer2_flag = 1;
  }
  if (timer3_counter > 0) {
    timer3_counter--;
    if (timer3_counter == 0) timer3_flag = 1;
  }
}

/* Bang ma 7 doan, bit0..bit6 = doan a..g (kieu gfedcba).
   LED common anode: doan SANG khi chan tin hieu o muc 0. */
const uint8_t SEG_CODE[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

void display7SEG(int num)
{
  if (num < 0 || num > 9) return;                 /* chong tran bang    */
  for (int i = 0; i < 7; i++) {                   /* SEG0..SEG6 = PB0..PB6 */
    HAL_GPIO_WritePin(SEG0_GPIO_Port, (uint16_t)(SEG0_Pin << i),
                      ((SEG_CODE[num] >> i) & 1) ? SEG_ON : SEG_OFF);
  }
}

void update7SEG(int index)
{
  /* B1: TAT het 4 LED truoc. Neu bo buoc nay, du lieu cua LED cu con
         nam tren bus SEG trong luc LED moi da sang -> bong mo (ghosting). */
  HAL_GPIO_WritePin(EN0_GPIO_Port, EN0_Pin | EN1_Pin | EN2_Pin | EN3_Pin, EN_OFF);

  /* B2: dat du lieu  ->  B3: bat DUNG 1 LED */
  switch (index) {
    case 0:  display7SEG(led_buffer[0]);
             HAL_GPIO_WritePin(EN0_GPIO_Port, EN0_Pin, EN_ON);  break;
    case 1:  display7SEG(led_buffer[1]);
             HAL_GPIO_WritePin(EN1_GPIO_Port, EN1_Pin, EN_ON);  break;
    case 2:  display7SEG(led_buffer[2]);
             HAL_GPIO_WritePin(EN2_GPIO_Port, EN2_Pin, EN_ON);  break;
    case 3:  display7SEG(led_buffer[3]);
             HAL_GPIO_WritePin(EN3_GPIO_Port, EN3_Pin, EN_ON);  break;
    default: break;
  }
}

/* Doi gio:phut thanh 4 chu so cho 4 LED 7 doan.
   So 1 chu so tu dong co so 0 o truoc, vi du 8 gio -> "08". */
void updateClockBuffer(void)
{
  led_buffer[0] = hour   / 10;
  led_buffer[1] = hour   % 10;
  led_buffer[2] = minute / 10;
  led_buffer[3] = minute % 10;
}

/* ===================== LED MATRIX 8x8 =====================
   ENM0..ENM7 qua ULN2803 chon 1 COT   (tich cuc muc 1)
   ROW0..ROW7 = PB8..PB15 la 8 duong du lieu (muc 1 thi diem sang) */
int index_led_matrix = 0;

/* Chu 'A': moi phan tu la MOT COT, bit i cua byte = ROW i (bit0 = ROW0).
          c0 c1 c2 c3 c4 c5 c6 c7
   ROW0    .  .  .  #  .  .  .  .
   ROW1    .  .  #  .  #  .  .  .
   ROW2    .  #  .  .  .  #  .  .
   ROW3    .  #  .  .  .  #  .  .
   ROW4    .  #  #  #  #  #  .  .      <- net ngang
   ROW5    .  #  .  .  .  #  .  .
   ROW6    .  #  .  .  .  #  .  .
   ROW7    .  .  .  .  .  .  .  .
   Net chu day 1 diem va chi chiem 5 cot (c1..c5) cho thanh manh. */
uint8_t matrix_buffer[MAX_LED_MATRIX] = {
  0x00, 0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00, 0x00
};

void updateLEDMatrix(int index)
{
  uint8_t data = matrix_buffer[index];

  /* B1: tat TAT CA cot (neu khong se bi keo vet ngang) */
  HAL_GPIO_WritePin(ENM0_GPIO_Port,
                    ENM0_Pin | ENM1_Pin | ENM2_Pin | ENM3_Pin |
                    ENM4_Pin | ENM5_Pin | ENM6_Pin | ENM7_Pin, MATRIX_COL_OFF);

  /* B2: dat du lieu 8 hang ROW0..ROW7 */
  for (int r = 0; r < 8; r++) {
    HAL_GPIO_WritePin(ROW0_GPIO_Port, (uint16_t)(ROW0_Pin << r),
                      ((data >> r) & 1) ? MATRIX_ROW_ON : MATRIX_ROW_OFF);
  }

  /* B3: bat DUNG 1 cot */
  switch (index) {
    case 0: HAL_GPIO_WritePin(ENM0_GPIO_Port, ENM0_Pin, MATRIX_COL_ON); break;
    case 1: HAL_GPIO_WritePin(ENM1_GPIO_Port, ENM1_Pin, MATRIX_COL_ON); break;
    case 2: HAL_GPIO_WritePin(ENM2_GPIO_Port, ENM2_Pin, MATRIX_COL_ON); break;
    case 3: HAL_GPIO_WritePin(ENM3_GPIO_Port, ENM3_Pin, MATRIX_COL_ON); break;
    case 4: HAL_GPIO_WritePin(ENM4_GPIO_Port, ENM4_Pin, MATRIX_COL_ON); break;
    case 5: HAL_GPIO_WritePin(ENM5_GPIO_Port, ENM5_Pin, MATRIX_COL_ON); break;
    case 6: HAL_GPIO_WritePin(ENM6_GPIO_Port, ENM6_Pin, MATRIX_COL_ON); break;
    case 7: HAL_GPIO_WritePin(ENM7_GPIO_Port, ENM7_Pin, MATRIX_COL_ON); break;
    default: break;
  }
}

/* Dich chu sang TRAI 1 cot: cot i lay du lieu cua cot i+1,
   cot dau cu quay ve lam cot cuoi -> chu chay vong khong bao gio het. */
void shiftLEDMatrixLeft(void)
{
  uint8_t first = matrix_buffer[0];
  for (int i = 0; i < MAX_LED_MATRIX - 1; i++) {
    matrix_buffer[i] = matrix_buffer[i + 1];
  }
  matrix_buffer[MAX_LED_MATRIX - 1] = first;
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
  /* USER CODE BEGIN 2 */
  updateClockBuffer();
  HAL_TIM_Base_Start_IT(&htim2);

  setTimer0(CLOCK_TICK_MS);        /* dong ho + DOT        */
  setTimer1(SWITCH_TIME_MS);       /* quet LED 7 doan      */
  setTimer2(MATRIX_SCAN_MS);       /* quet LED matrix      */
  setTimer3(SHIFT_TIME_MS);        /* dich chu sang trai   */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* ----- Timer 0: dong ho va DOT ----- */
    if (timer0_flag == 1) {
      setTimer0(CLOCK_TICK_MS);

        second++;
        if (second >= 60) { second = 0; minute++; }
        if (minute >= 60) { minute = 0; hour++;   }
        if (hour   >= 24) { hour   = 0;           }
        updateClockBuffer();

        dot_state = !dot_state;
        HAL_GPIO_WritePin(DOT_GPIO_Port, DOT_Pin,
                          dot_state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }

    /* ----- Timer 1: quet 4 LED 7 doan ----- */
    if (timer1_flag == 1) {
      setTimer1(SWITCH_TIME_MS);
      update7SEG(index_led++);
      if (index_led >= MAX_LED) index_led = 0;
    }

    /* ----- Timer 2: quet 8 cot LED matrix ----- */
    if (timer2_flag == 1) {
      setTimer2(MATRIX_SCAN_MS);
      updateLEDMatrix(index_led_matrix++);
      if (index_led_matrix >= MAX_LED_MATRIX) index_led_matrix = 0;
    }

    /* ----- Timer 3: cu 200 ms dich chu sang trai 1 cot ----- */
    if (timer3_flag == 1) {
      setTimer3(SHIFT_TIME_MS);
      shiftLEDMatrixLeft();
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
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ENM0_Pin|ENM1_Pin|DOT_Pin
                          |LED_RED_Pin|EN0_Pin|EN1_Pin
                          |EN2_Pin|EN3_Pin|ENM2_Pin
                          |ENM3_Pin|ENM4_Pin|ENM5_Pin
                          |ENM6_Pin|ENM7_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, SEG0_Pin|SEG1_Pin|SEG2_Pin
                          |SEG3_Pin|SEG4_Pin|SEG5_Pin
                          |SEG6_Pin|ROW0_Pin|ROW1_Pin
                          |ROW2_Pin|ROW3_Pin|ROW4_Pin
                          |ROW5_Pin|ROW6_Pin|ROW7_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : ENM0_Pin ENM1_Pin DOT_Pin LED_RED_Pin EN0_Pin EN1_Pin EN2_Pin EN3_Pin ENM2_Pin ENM3_Pin ENM4_Pin ENM5_Pin ENM6_Pin ENM7_Pin */
  GPIO_InitStruct.Pin = ENM0_Pin|ENM1_Pin|DOT_Pin
                          |LED_RED_Pin|EN0_Pin|EN1_Pin
                          |EN2_Pin|EN3_Pin|ENM2_Pin
                          |ENM3_Pin|ENM4_Pin|ENM5_Pin
                          |ENM6_Pin|ENM7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : SEG0_Pin SEG1_Pin SEG2_Pin SEG3_Pin SEG4_Pin SEG5_Pin SEG6_Pin ROW0_Pin ROW1_Pin ROW2_Pin ROW3_Pin ROW4_Pin ROW5_Pin ROW6_Pin ROW7_Pin */
  GPIO_InitStruct.Pin = SEG0_Pin|SEG1_Pin|SEG2_Pin
                          |SEG3_Pin|SEG4_Pin|SEG5_Pin
                          |SEG6_Pin|ROW0_Pin|ROW1_Pin
                          |ROW2_Pin|ROW3_Pin|ROW4_Pin
                          |ROW5_Pin|ROW6_Pin|ROW7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9;
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

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2) {
    timer_run();
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
