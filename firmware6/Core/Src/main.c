/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "NanoEdgeAI.h"

//#include "fast_math.h"
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
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
#define MPU_COUNT 4

#define MPU6050_ADDR   (0x68 << 1)
#define TCA9548A_ADDR  (0x70 << 1)

#define PWR_MGMT_1     0x6B
#define SMPLRT_DIV     0x19
#define GYRO_CONFIG    0x1B
#define ACCEL_CONFIG   0x1C
#define ACCEL_XOUT_H   0x3B
#define GYRO_XOUT_H    0x43

#define ACCEL_RESOLUTION   0.000061f
#define GYRO_RESOLUTION    0.0076f

#define DEG_TO_RAD         0.01745329252f
#define RAD_TO_DEG         57.29577951f

#define NEAI_INPUT_SIZE 12

float neai_input[NEAI_INPUT_SIZE];

int class_id = 0;
float output_class_buffer[NEAI_NUMBER_OF_CLASSES];   // Good / Bad

/* Raw data */
int16_t Ax_raw[MPU_COUNT], Ay_raw[MPU_COUNT], Az_raw[MPU_COUNT];
int16_t Gx_raw[MPU_COUNT], Gy_raw[MPU_COUNT], Gz_raw[MPU_COUNT];

/* Converted physical values */
float ax_g[MPU_COUNT], ay_g[MPU_COUNT], az_g[MPU_COUNT];
float gx_dps[MPU_COUNT], gy_dps[MPU_COUNT], gz_dps[MPU_COUNT];

/* Accelerometer Euler angles */
float acc_roll[MPU_COUNT], acc_pitch[MPU_COUNT];

/* Gyroscope Euler angles */
float gyro_roll[MPU_COUNT]  = {0};
float gyro_pitch[MPU_COUNT] = {0};
float gyro_yaw[MPU_COUNT]   = {0};

/* Kalman final Euler angles */
float kalman_roll[MPU_COUNT]  = {0};
float kalman_pitch[MPU_COUNT] = {0};
float yaw_angle[MPU_COUNT]    = {0};

float euler_output[12];
/* Kalman covariance matrix for each MPU6050 */
float P_prev[MPU_COUNT][2][2] = {0};

/* Noise covariance matrices from experiment */
float Q[2][2] = {
    {0.0027f, 0.0001f},
    {0.0001f, 0.0023f}
};

float R[2][2] = {
    {0.32f, 0.01f},
    {0.01f, 0.28f}
};

uint32_t last_time = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

HAL_StatusTypeDef TCA9548A_SelectChannel(uint8_t channel)
{
    uint8_t data = 1 << channel;
    return HAL_I2C_Master_Transmit(&hi2c1, TCA9548A_ADDR, &data, 1, HAL_MAX_DELAY);
}

void MPU6050_Init(void)
{
    uint8_t data;

    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1, 1, &data, 1, HAL_MAX_DELAY);

    data = 0x07;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, SMPLRT_DIV, 1, &data, 1, HAL_MAX_DELAY);

    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, GYRO_CONFIG, 1, &data, 1, HAL_MAX_DELAY);

    data = 0x00;
    HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG, 1, &data, 1, HAL_MAX_DELAY);
}

void MPU6050_Read_Accel(int16_t *Ax, int16_t *Ay, int16_t *Az)
{
    uint8_t Rec_Data[6];

    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H, 1,
                     Rec_Data, 6, HAL_MAX_DELAY);

    *Ax = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    *Ay = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    *Az = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);
}

void MPU6050_Read_Gyro(int16_t *Gx, int16_t *Gy, int16_t *Gz)
{
    uint8_t Rec_Data[6];

    HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, GYRO_XOUT_H, 1,
                     Rec_Data, 6, HAL_MAX_DELAY);

    *Gx = (int16_t)(Rec_Data[0] << 8 | Rec_Data[1]);
    *Gy = (int16_t)(Rec_Data[2] << 8 | Rec_Data[3]);
    *Gz = (int16_t)(Rec_Data[4] << 8 | Rec_Data[5]);
}

void MPU6050_Read_All(void)
{
    for (uint8_t ch = 0; ch < MPU_COUNT; ch++)
    {
        TCA9548A_SelectChannel(ch);
        HAL_Delay(2);

        MPU6050_Read_Accel(&Ax_raw[ch], &Ay_raw[ch], &Az_raw[ch]);
        MPU6050_Read_Gyro(&Gx_raw[ch], &Gy_raw[ch], &Gz_raw[ch]);

        ax_g[ch] = (float)Ax_raw[ch] * ACCEL_RESOLUTION;
        ay_g[ch] = (float)Ay_raw[ch] * ACCEL_RESOLUTION;
        az_g[ch] = (float)Az_raw[ch] * ACCEL_RESOLUTION;

        gx_dps[ch] = (float)Gx_raw[ch] * GYRO_RESOLUTION;
        gy_dps[ch] = (float)Gy_raw[ch] * GYRO_RESOLUTION;
        gz_dps[ch] = (float)Gz_raw[ch] * GYRO_RESOLUTION;
    }
}

void Calculate_Accelerometer_Euler(uint8_t i)
{
    acc_roll[i] = atan2f(ay_g[i], az_g[i]) * RAD_TO_DEG;

    acc_pitch[i] = -atan2f(ax_g[i],
                           sqrtf(ay_g[i] * ay_g[i] + az_g[i] * az_g[i]))
                           * RAD_TO_DEG;
}

void Calculate_Gyroscope_Euler(uint8_t i, float dt)
{
    float r = kalman_roll[i] * DEG_TO_RAD;
    float p = kalman_pitch[i] * DEG_TO_RAD;

    float gx = gx_dps[i];
    float gy = gy_dps[i];
    float gz = gz_dps[i];

    float cos_p = cosf(p);

    if (fabsf(cos_p) < 0.001f)
    {
        cos_p = 0.001f;
    }

    /*
       ZYX Euler angle rate transformation:

       [dr/dt]   [1  sin(p)sin(r)/cos(p)   cos(r)sin(p)/cos(p)] [gx]
       [dp/dt] = [0          cos(r)              -sin(r)       ] [gy]
       [dy/dt]   [0       sin(r)/cos(p)        cos(r)/cos(p)   ] [gz]
    */

    float dr_dt = gx
                + (sinf(p) * sinf(r) / cos_p) * gy
                + (cosf(r) * sinf(p) / cos_p) * gz;

    float dp_dt = cosf(r) * gy
                - sinf(r) * gz;

    float dy_dt = (sinf(r) / cos_p) * gy
                + (cosf(r) / cos_p) * gz;

    gyro_roll[i]  = kalman_roll[i]  + dr_dt * dt;
    gyro_pitch[i] = kalman_pitch[i] + dp_dt * dt;
    gyro_yaw[i]   = gyro_yaw[i]     + dy_dt * dt;

    yaw_angle[i] = gyro_yaw[i];
}

void Kalman_Filter_Update(uint8_t i)
{
    float P_curr[2][2];
    float K[2][2];

    /* Step 1: prediction error covariance update */
    P_curr[0][0] = P_prev[i][0][0] + Q[0][0];
    P_curr[0][1] = P_prev[i][0][1] + Q[0][1];
    P_curr[1][0] = P_prev[i][1][0] + Q[1][0];
    P_curr[1][1] = P_prev[i][1][1] + Q[1][1];

    /* Step 2: Kalman gain update */
    K[0][0] = P_curr[0][0] / (P_curr[0][0] + R[0][0]);
    K[0][1] = P_curr[0][1] / (P_curr[0][1] + R[0][1]);
    K[1][0] = P_curr[1][0] / (P_curr[1][0] + R[1][0]);
    K[1][1] = P_curr[1][1] / (P_curr[1][1] + R[1][1]);

    /* Step 3: state update */
    kalman_roll[i] = gyro_roll[i]
                   + K[0][0] * (acc_roll[i] - gyro_roll[i])
                   + K[0][1] * (acc_pitch[i] - gyro_pitch[i]);

    kalman_pitch[i] = gyro_pitch[i]
                    + K[1][0] * (acc_roll[i] - gyro_roll[i])
                    + K[1][1] * (acc_pitch[i] - gyro_pitch[i]);

    /* Step 4: posterior error covariance update */
    P_prev[i][0][0] = (1.0f - K[0][0]) * P_curr[0][0];
    P_prev[i][0][1] = (1.0f - K[0][1]) * P_curr[0][1];
    P_prev[i][1][0] = (1.0f - K[1][0]) * P_curr[1][0];
    P_prev[i][1][1] = (1.0f - K[1][1]) * P_curr[1][1];
}

void Update_All_Euler_Angles(float dt)
{
    for (uint8_t i = 0; i < MPU_COUNT; i++)
    {
        Calculate_Accelerometer_Euler(i);
        Calculate_Gyroscope_Euler(i, dt);
        Kalman_Filter_Update(i);
    }
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
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  setvbuf(stdout, NULL, _IONBF, 0);

  for (uint8_t ch = 0; ch < MPU_COUNT; ch++)
  {
      TCA9548A_SelectChannel(ch);
      HAL_Delay(5);
      MPU6050_Init();
      HAL_Delay(10);
  }

  enum neai_state neai_status;
  neai_status = neai_classification_init();

  if (neai_status != NEAI_OK)
  {
      printf("NanoEdge AI init failed\r\n");
  }
  else
  {
      printf("NanoEdge AI init OK\r\n");
  }

  last_time = HAL_GetTick();



  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      uint32_t now = HAL_GetTick();
      float dt = (now - last_time) / 1000.0f;
      last_time = now;

      if (dt <= 0.0f || dt > 1.0f)
      {
          dt = 0.05f;
      }

      MPU6050_Read_All();
      Update_All_Euler_Angles(dt);

      euler_output[0]  = kalman_roll[0];
      euler_output[1]  = kalman_pitch[0];
      euler_output[2]  = yaw_angle[0];

      euler_output[3]  = kalman_roll[1];
      euler_output[4]  = kalman_pitch[1];
      euler_output[5]  = yaw_angle[1];

      euler_output[6]  = kalman_roll[2];
      euler_output[7]  = kalman_pitch[2];
      euler_output[8]  = yaw_angle[2];

      euler_output[9]  = kalman_roll[3];
      euler_output[10] = kalman_pitch[3];
      euler_output[11] = yaw_angle[3];

      for (uint8_t i = 0; i < NEAI_INPUT_SIZE; i++)
      {
          neai_input[i] = euler_output[i];
      }

      neai_classification(neai_input, output_class_buffer, &class_id);

      if (class_id == 0)
      {
          printf("Good Posture\r\n");
      }
      else if (class_id == 1)
      {
          printf("Bad Posture\r\n");
      }

      HAL_Delay(50);
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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
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

  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x10D19CE4;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, left_Pin|right_Pin|centre_Pin|down_Pin
                          |up_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : left_Pin right_Pin centre_Pin down_Pin
                           up_Pin */
  GPIO_InitStruct.Pin = left_Pin|right_Pin|centre_Pin|down_Pin
                          |up_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LD3_Pin */
  GPIO_InitStruct.Pin = LD3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD3_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
