/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
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
#include "cmsis_os.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "shell.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK_SHELL_STACK_LENGTH 256
#define TASK_SHELL_PRIORITY 2
#define TASK_LED_PRIORITY 1

#define TASK_BIDON1_PRIORITY 2
#define TASK_BIDON2_PRIORITY 1

#define STACK_SIZE 1000
#define LED_DELAY 500

#define DELAY_1 500
#define DELAY_2 1500

#define LONG_MAX 500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
TaskHandle_t h_task_shell;
TaskHandle_t xHandleToggleLED;
TaskHandle_t xHandleBidon;

SemaphoreHandle_t sem1 = NULL;

TaskHandle_t xHandleTaskGiveNotif = NULL;
TaskHandle_t xHandleTaskTakeNotif = NULL;

TaskHandle_t xHandleTaskGiveQueue = NULL;
TaskHandle_t xHandleTaskTakeQueue = NULL;

QueueHandle_t maQueue = NULL;

int requestLED = 0;
int freqLED = 0;
int argLED;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
void codeLED_Toggle (void * pvParameters);

void bidon1(void * pvParameters);
void bidon2(void * pvParameters);

void codeTaskGiveNotif (void * pvParameters);
void codeTaskTakeNotif (void * pvParameters);

void codeTaskGiveQueue (void * pvParameters);
void codeTaskTakeQueue (void * pvParameters);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int ch)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)&ch, 1, HAL_MAX_DELAY);

	return ch;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t hptw = pdFALSE;
	// Give la notification, pour réveiller la tâche du shell
	// Seulement si c'est une interruption de l'USART1
	if (USART1 == huart->Instance)
	{
		vTaskNotifyGiveFromISR(h_task_shell, &hptw);
	}

	portYIELD_FROM_ISR(hptw)
}

int fonction(int argc, char ** argv)
{
	printf("argc =%d\r\n",argc);

	for (int i=0;i<argc;i++)
	{
		printf("L'argument numero %d = %s \r\n",i,argv[i]);
	}


	return 0;
}

int led(int argc, char ** argv)
{

	if (argc !=2)
	{
		printf("ERREUR : Pas le bon nombre d'arguments\r\n");
	}
	else{
		argLED = atoi(argv[1]);
		if (argLED == 0)
		{
			//requestLED = 0;
			vTaskSuspend(xHandleToggleLED);
			if(HAL_GPIO_ReadPin(GPIO_LED_GPIO_Port, GPIO_LED_Pin)== SET)
			{
				HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, RESET);
			}
			printf("LED eteinte\r\n");

		}
		else
		{
			vTaskResume(xHandleToggleLED);
			freqLED = atoi(argv[1]);
			requestLED = 1;
			printf("LED allumee\r\n");
			printf("Frequence : %dHz\r\n", 1000/freqLED);

		}
	}

	return 0;
}

void task_shell(void * unused)
{
	shell_init();
	shell_add('f', fonction, "Une fonction inutile");
	shell_add('l', led, "J'adore clignoter");
	shell_run();

	// Il ne faut jamais retourner d'une tâche
	// Mais on n'arrive jamais là parce que shell_run est une boucle infinie
}


volatile unsigned count;

void configureTimerForRunTimeStats(void)
{
    count = 0;
    HAL_TIM_Base_Start_IT(&htim10);
}

unsigned long getRunTimeCounterValue(void)
{
	return count;
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
  MX_USART1_UART_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */

	BaseType_t TaskGiveNotif;
	BaseType_t TaskTakeNotif;

	BaseType_t TaskGiveQueue;
	BaseType_t TaskTakeQueue;

	sem1 = xSemaphoreCreateBinary();
	configASSERT(sem1 != NULL);

	if (xTaskCreate(task_shell, "Shell", TASK_SHELL_STACK_LENGTH, NULL, TASK_SHELL_PRIORITY, &h_task_shell) != pdPASS)
	{
		printf("Error task shell\r\n");
		Error_Handler();
	}

	/*if (xTaskCreate(codeLED_Toggle, "LED Toggle", STACK_SIZE, (void *) LED_DELAY, TASK_LED_PRIORITY, &xHandleToggleLED ) != pdPASS)
	{
		printf("Error task ledToggle\r\n");
		Error_Handler();
	}*/

	if (xTaskCreate(bidon1, "bidon1", STACK_SIZE, (void *) DELAY_1, TASK_BIDON1_PRIORITY, &xHandleBidon ) != pdPASS)
	{
		printf("Error task bidon\r\n");
		Error_Handler();
	}

	if (xTaskCreate(bidon2, "bidon2", STACK_SIZE, (void *) DELAY_1, TASK_BIDON2_PRIORITY, &xHandleBidon ) != pdPASS)
	{
		printf("Error task bidon\r\n");
		Error_Handler();
	}

	vQueueAddToRegistry(sem1, "Semaphore");

	//TaskGiveNotif = xTaskCreate(codeTaskGiveNotif, "Task Give", STACK_SIZE, (void *) DELAY_1, tskIDLE_PRIORITY+2, &xHandleTaskGiveNotif );
	//TaskTakeNotif = xTaskCreate(codeTaskTakeNotif, "Task Take", STACK_SIZE, (void *) DELAY_2, tskIDLE_PRIORITY+1, &xHandleTaskTakeNotif );
	//configASSERT(TaskTakeNotif == pdPASS);

	//TaskGiveQueue = xTaskCreate(codeTaskGiveQueue, "Task Give", STACK_SIZE, (void *) DELAY_1, tskIDLE_PRIORITY+2, &xHandleTaskGiveQueue );
	//TaskTakeQueue = xTaskCreate(codeTaskTakeQueue, "Task Take", STACK_SIZE, (void *) DELAY_2, tskIDLE_PRIORITY+1, &xHandleTaskTakeQueue);
	//configASSERT(TaskTakeQueue == pdPASS);

	vTaskStartScheduler();


  /* USER CODE END 2 */

  /* Call init function for freertos objects (in freertos.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
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
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 432;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void codeLED_Toggle(void* pvParameters)
{
	for(;;)
	{
		while (requestLED == 1)
		{
			HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, SET);
			vTaskDelay(freqLED);
			HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, RESET);
			vTaskDelay(freqLED);
		}
	}

}

void bidon1(void* pvParameters)
{
	while(1){
		xSemaphoreGive(sem1);
		vTaskDelay(1000);
		printf("le sem ne fonctionne pas\r\n");
	}
}

void bidon2(void* pvParameters)
{
	while(1){
		xSemaphoreTake(sem1, DELAY_1);
	}
}

void codeTaskGiveNotif(void* pvParameters)
{
	int delay = (int) pvParameters;
	for(;;)
	{
		printf("I give\r\n");
		xTaskNotifyGive(xHandleTaskTakeNotif);
		printf("Notification given\r\n");
		vTaskDelay(delay);
	}
}

void codeTaskTakeNotif(void* pvParameters)
{
	int errorDelay = (int) pvParameters;
	for(;;)
	{
		printf("I take\r\n");
		if(ulTaskNotifyTake(pdTRUE, errorDelay) == pdTRUE)
		{
			printf("Notification taken\r\n");
		}
		else
		{
			printf("Error\r\n");
			NVIC_SystemReset();
		}
	}
}

void codeTaskGiveQueue(void* pvParameters)
{
	int c=0;
	static char msg[] = "msg 00";
	int delay = (int) pvParameters;
	for(;;)
	{
		delay+=100;
		sprintf(msg, "msg %02d \r\n", c);
		xQueueSend(maQueue, (const void*) msg, portMAX_DELAY);
		c++;
		printf("Message given\r\n");
		vTaskDelay(delay);
	}
}

void codeTaskTakeQueue(void* pvParameters)
{
	char msgBuf[LONG_MAX];
	int errorDelay = (int) pvParameters;
	for(;;)
	{
		printf("I take\r\n");
		if(xQueueReceive(maQueue, (void*) msgBuf, errorDelay) == pdTRUE)
		{
			printf(msgBuf);
			printf("Message taken\r\n");


		}
		else
		{
			printf("ERROR : max delay reached\r\n");
			NVIC_SystemReset();
		}
	}
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{

	if(GPIO_Pin == Blue_Button_Pin)
	{

		if(HAL_GPIO_ReadPin(GPIO_LED_GPIO_Port, GPIO_LED_Pin)==SET)
		{
			HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, RESET);
		}
		else
		{
			HAL_GPIO_WritePin(GPIO_LED_GPIO_Port, GPIO_LED_Pin, SET);
		}
	}

}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
