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
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "hc05_driver.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
// Command definitions
#define CMD_LED_ON    "ledon"
#define CMD_LED_OFF   "ledoff"
#define CMD_HELP      "help"
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
HC05_HandleTypeDef hc05; // HC-05 Bluetooth module driver structure
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
// Command dispatcher function prototypes
void ProcessBluetoothCommand(const char* command);
void SendCommandResponse(const char* response);
void ShowAvailableCommands(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/**
  * @brief  Printf redirect to UART2 (Virtual COM Port)
  * @param  ch: character to send
  * @param  f: file pointer
  * @retval character sent
  */
int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

/**
  * @brief  Process received Bluetooth commands
  * @param  command: received command string
  * @retval None
  */
void ProcessBluetoothCommand(const char* command)
{
  // Convert command to lowercase for case-insensitive comparison
  char cmd_lower[HC05_BUFFER_SIZE];
  strncpy(cmd_lower, command, sizeof(cmd_lower) - 1);
  cmd_lower[sizeof(cmd_lower) - 1] = '\0';

  // Convert to lowercase
  for(int i = 0; cmd_lower[i]; i++){
    if(cmd_lower[i] >= 'A' && cmd_lower[i] <= 'Z'){
      cmd_lower[i] = cmd_lower[i] + 32;
    }
  }

  printf("Processing command: '%s'\r\n", cmd_lower);

  // Command dispatcher
  if (strcmp(cmd_lower, CMD_LED_ON) == 0) {
    // Turn ON LED2 (User LED on Nucleo board)
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
    SendCommandResponse("LED ON - User LED activated");
    printf("Command executed: LED turned ON\r\n");
  }
  else if (strcmp(cmd_lower, CMD_LED_OFF) == 0) {
    // Turn OFF LED2 (User LED on Nucleo board)
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
    SendCommandResponse("LED OFF - User LED deactivated");
    printf("Command executed: LED turned OFF\r\n");
  }
  else if (strcmp(cmd_lower, CMD_HELP) == 0) {
    // Show available commands
    ShowAvailableCommands();
  }
  else {
    // Unknown command
    char error_msg[128];
    snprintf(error_msg, sizeof(error_msg), "Unknown command: '%s'. Type 'help' for available commands.", command);
    SendCommandResponse(error_msg);
    printf("Unknown command received: '%s'\r\n", command);
  }
}

/**
  * @brief  Send response back via Bluetooth
  * @param  response: response string to send
  * @retval None
  */
void SendCommandResponse(const char* response)
{
  char full_response[HC05_BUFFER_SIZE + 50];
  snprintf(full_response, sizeof(full_response), "[STM32]: %s\r\n", response);
  HC05_SendData(&hc05, full_response);
}

/**
  * @brief  Show available commands via Bluetooth
  * @retval None
  */
void ShowAvailableCommands(void)
{
  HC05_SendData(&hc05, "\r\n=== STM32 Nucleo Commands ===\r\n");
  HC05_SendData(&hc05, "ledon  - Turn ON user LED\r\n");
  HC05_SendData(&hc05, "ledoff - Turn OFF user LED\r\n");
  HC05_SendData(&hc05, "help   - Show this help\r\n");
  HC05_SendData(&hc05, "=============================\r\n");
  printf("Help commands sent via Bluetooth\r\n");
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
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* Wait for system stabilization */
    HAL_Delay(1000);

    /* Initialize HC-05 module on UART1 */
    if (HC05_Init(&hc05, &huart1, GPIOA, GPIO_PIN_8) == HC05_OK) {
      printf("HC-05 initialized successfully\r\n");
    } else {
      printf("HC-05 initialization error\r\n");
    }

    /* Optional HC-05 module configuration */
    HAL_Delay(1000);

    // Example: set device name
    if (HC05_SetName(&hc05, "STM32_HC05") == HC05_OK) {
      printf("Name set: STM32_HC05\r\n");
    }

    // Example: set pairing PIN
    if (HC05_SetPIN(&hc05, "1234") == HC05_OK) {
      printf("PIN set: 1234\r\n");
    }

    printf("System ready - Waiting for Bluetooth commands...\r\n");
    printf("Available commands: ledon, ledoff, help\r\n");

    /* Send welcome message via Bluetooth */
    HAL_Delay(500);
    HC05_SendData(&hc05, "\r\n*** STM32 Nucleo HC-05 Controller ***\r\n");
    HC05_SendData(&hc05, "System ready! Type 'help' for commands.\r\n");

    /* Test UART - send test character every 30 seconds */
    uint32_t last_test = 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
   while (1)
   {
	   // Periodic test: send heartbeat via Bluetooth every 30 seconds
	    if (HAL_GetTick() - last_test > 30000) {
	      printf("Sending heartbeat via Bluetooth...\r\n");
	      HC05_SendData(&hc05, "[STM32]: Heartbeat - System running OK\r\n");
	      last_test = HAL_GetTick();
	    }

	    // Check if data is available from Bluetooth
	    if (HC05_DataAvailable(&hc05)) {
	      char received_data[HC05_BUFFER_SIZE];

	      // Read received data
	      if (HC05_ReceiveData(&hc05, received_data, sizeof(received_data)) == HC05_OK) {
	        // Print received command via Virtual COM Port
	        printf("BT RX: '%s'\r\n", received_data);

	        // Process the received command through dispatcher
	        ProcessBluetoothCommand(received_data);
	      }
	    }

	    HAL_Delay(10); // Small pause to avoid overloading the loop
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
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 7;
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
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  /* Enable UART1 interrupt */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
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
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|HC05_Enable_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin HC05_Enable_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|HC05_Enable_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  UART interrupt callback function
  * @param  huart: UART handle
  * @retval None
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART1) {
    // Call HC-05 driver interrupt handler
    HC05_IRQHandler(&hc05);
  }
}

/**
  * @brief  This function handles USART1 global interrupt.
  * @param  None
  * @retval None
  */
void USART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&huart1);
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
