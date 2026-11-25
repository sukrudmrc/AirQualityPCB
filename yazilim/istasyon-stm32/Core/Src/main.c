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
#include "stdio.h"
#include "stdint.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
	STATE_Wait,
	SHT31_DATA_READ,
	CO_DATA_READ,
	PM_DATA_READ,
	MAKE_AVARAGE,
	SEND_LORA,
	CLEAR_BUFFER,

}System_States;

typedef struct{
	uint16_t Lora_Addres;
	uint8_t Lora_Channel;
}Lora_Destination;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SampleSize 10 							// kaç verinin ardarda okunacağını belirler
#define SHT31_ADDR (0x44 << 1)  				//sht31 addres
uint8_t sht31_read_command[2]= {0x2C , 0x06}; // 0x2C clock stretch enable , 0x06 high repeatibility
#define baslangıc 0xAA
#define final 0xAB
#define BufferSize 64


//	Sensörelerden okunan verilerin sonrasında ortalaması alınması için içinde saklanacağı değişken
volatile uint16_t temp_buff[SampleSize];
volatile uint16_t humidity_buff[SampleSize];
volatile uint16_t CO_buff[SampleSize];
volatile uint16_t PM25_buff[SampleSize];
volatile uint16_t PM10_buff[SampleSize];


volatile uint16_t avg_temperature_celsius; // SHT31 için ham sıcaklık ortalamasını veya °C'yi saklar
volatile uint16_t avg_humidity_percent;  	// SHT31 için ham nem ortalamasını veya %RH'yi saklar
volatile uint16_t avg_co_value;          	// CO sensörünün ortalama ham ADC değerini saklar
volatile uint16_t avg_pm25_ugm3;        	// PM2.5 için ortalama değeri (μg/m³) saklar
volatile uint16_t avg_pm10_ugm3;      	// PM10 için ortalama değeri (μg/m³) saklar
uint8_t byte;

Lora_Destination Current_Destination;
Lora_Destination Destination1;
Lora_Destination Destination2;
static volatile System_States state;

uint8_t veribuff[13];

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if (huart->Instance == USART1) {
		//HAL_PWR_DisableSleepOnExit();
		//HAL_ResumeTick();
		state=SHT31_DATA_READ;
		HAL_UART_Receive_IT(huart, &byte, 1);  // tekrar başlat
	}
}

void Lora_Send(uint8_t *message,uint8_t lenght,Lora_Destination *Destination){
	if(lenght < BufferSize){
		uint8_t tx_buffer[BufferSize+3];
		tx_buffer[0]=(Destination->Lora_Addres >> 8) & 0xFF;
		tx_buffer[1]= Destination->Lora_Addres & 0xFF;
		tx_buffer[2]= Destination->Lora_Channel;

		memcpy(&tx_buffer[3],message,lenght);
		HAL_UART_Transmit(&huart1, tx_buffer, lenght+3 ,HAL_MAX_DELAY);
}
}
void LedBlink(int time,int delay)
{
	for(int i=0;i<time;i++){
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 1);
	HAL_Delay(delay);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, 0);
	HAL_Delay(delay);
	}
}

uint8_t SDS_sensor(volatile uint16_t *pm25, volatile uint16_t *pm10) {
    uint8_t ch;
    uint8_t SDS_DATA[10];

    // İlk byte olarak 0xAA gelene kadar bekle
    do {
        if (HAL_UART_Receive(&huart2, &ch,1,100) != HAL_OK) {

            return 0; // timeout veya hata
        }
    } while (ch != 0xAA);

    SDS_DATA[0] = ch;

    // Kalan 9 byte'ı oku (toplam 10 byte olacak)
    if (HAL_UART_Receive(&huart2, &SDS_DATA[1], 9, 100) != HAL_OK) {
        return 0;
    }

    // Baş ve son byte kontrolü
    if (SDS_DATA[1] != 0xC0 || SDS_DATA[9] != 0xAB) {
        return 0;
    }

    // Checksum hesapla (Byte 2-7 toplamı)
    uint8_t sum = 0;
    for (int i = 2; i <= 7; i++) {
        sum += SDS_DATA[i];
    }
    if (SDS_DATA[8] != sum) {
        return 0;
    }

    // PM2.5 ve PM10 hesapla
    *pm25 = ((uint16_t)SDS_DATA[3] << 8 | SDS_DATA[2]) / 10;
    *pm10 = ((uint16_t)SDS_DATA[5] << 8 | SDS_DATA[4]) / 10;


    return 1;
}
void Veri_LowHigh(){
	veribuff[1] = avg_pm25_ugm3 & 0xFF;
	veribuff[2]= (avg_pm25_ugm3 >> 8) & 0xFF;
	veribuff[3] = avg_pm10_ugm3 & 0xFF;
	veribuff[4]= (avg_pm10_ugm3 >> 8) & 0xFF;
	veribuff[5] = avg_co_value & 0xFF;
	veribuff[6]= (avg_co_value >> 8) & 0xFF;
	veribuff[7]= avg_temperature_celsius & 0xFF;
	veribuff[8]= (avg_temperature_celsius >> 8) & 0xFF;
	veribuff[9]= avg_humidity_percent & 0xFF;
	veribuff[10]= (avg_humidity_percent >> 8) & 0xFF;

}
void CheckSum(){
	veribuff[11]= 0;
	for(int i=1;i<=10;i++){
		veribuff[11] += veribuff[i];
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
	Destination1.Lora_Addres=0x0002;
	Destination1.Lora_Channel=0x02;

	Destination2.Lora_Addres=0x0001;
	Destination2.Lora_Channel=0x02;

	Current_Destination = Destination1;

	veribuff[0]=baslangıc;
	veribuff[12]=final;

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
  MX_ADC1_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UART_Receive_IT(&huart1, &byte, 1);
  LedBlink(3, 100);
  state=STATE_Wait;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  switch(state){
	  case STATE_Wait:
		  LedBlink(3,200);
	  break;
	 case SHT31_DATA_READ:			//okunana datalar ortlama alındıktan sorna anlamdırlacaka.
	  for(int i=0;i<SampleSize;i++)
	  {

		  uint8_t sht31_raw_data[6];
		  HAL_I2C_Master_Transmit(&hi2c1, SHT31_ADDR, sht31_read_command, 2, HAL_MAX_DELAY); // ölçüm istegi yolla
		  HAL_Delay(20);	//20ms datanin hazirlanmasi datasheet degeri 15ms

		  HAL_I2C_Master_Receive(&hi2c1, SHT31_ADDR, sht31_raw_data, 6, HAL_MAX_DELAY); 	// gelen verileri raw_dataya kaydet

		  temp_buff[i] = 		(sht31_raw_data[0] << 8) | sht31_raw_data[1];				//high bit ile low bitler OR yöntemi ile 16bit buffere kaydedildi
		  humidity_buff[i] =	(sht31_raw_data[3] << 8) | sht31_raw_data[4];
	  }
	  state = CO_DATA_READ;
	break;

	  case CO_DATA_READ:
		  for(int i=0;i<SampleSize;i++)
		  {
			  HAL_ADC_Start(&hadc1);								//adc yi başlat

			  if(HAL_ADC_PollForConversion(&hadc1,100) == HAL_OK)	//okumanın bitmesini bekle
			  {
				  CO_buff[i] = HAL_ADC_GetValue(&hadc1);			//değeri buffere ekle
			  }
			  else{
				  i-=1;
			  }
		  }
		  state = PM_DATA_READ;
		  break;

	  case PM_DATA_READ:
		  for(int i=0;i<SampleSize;i++)
		  {
			 while(!SDS_sensor(&PM25_buff[i],&PM10_buff[i]));// eğer Sensörden veri gelmiyorsa burda döngüye giriyor

		  }
		  state = MAKE_AVARAGE;
		  break;
	  case MAKE_AVARAGE:
		  // TÜm değerleri toplamak için gerekli değişkenler

		  uint32_t sum_temp = 0;
		  uint32_t sum_humidity = 0;
		  uint32_t sum_co = 0;
		  uint32_t sum_pm25 = 0;
		  uint32_t sum_pm10 = 0;

		  // 1. Adım: Tüm örneklerin toplamını al
		  for (int k = 0; k < SampleSize; k++)
		  {
			  sum_temp += temp_buff[k];
			  sum_humidity += humidity_buff[k];
			  sum_co += CO_buff[k];
			  sum_pm25 += PM25_buff[k];
			  sum_pm10 += PM10_buff[k];
		  }


		avg_temperature_celsius = sum_temp / SampleSize; 	// SHT31
		avg_humidity_percent = sum_humidity / SampleSize;

		avg_co_value = sum_co / SampleSize;					//CO

		avg_pm25_ugm3 = sum_pm25 / SampleSize; 		//PM
		avg_pm10_ugm3 = sum_pm10 / SampleSize;



		state = SEND_LORA;


		  break;

	  case SEND_LORA:
		  Veri_LowHigh();
		  CheckSum();
		  Lora_Send(veribuff,sizeof(veribuff),&Current_Destination);
			 state = CLEAR_BUFFER;
		  break;

	  case CLEAR_BUFFER:
		  for(int i=0;i<SampleSize;i++)
		  {
			  temp_buff[i]=0;
			  humidity_buff[i]=0;
			  CO_buff[i]=0;
			  PM25_buff[i]=0;
			  PM10_buff[i]=0;
		  }

		  state = STATE_Wait;
		  break;

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
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL3;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
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
  huart2.Init.BaudRate = 9600;
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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7|GPIO_PIN_11|GPIO_PIN_12, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA7 PA11 PA12 */
  GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_11|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PB13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
