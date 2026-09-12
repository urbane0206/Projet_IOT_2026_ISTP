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
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lmic.h"
#include "hal.h"
#include "debug.h"
#include "math.h"
#include "cayenne_lpp.h"
#include "RGBLCD1602.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
} Color;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VREF_MV   3300.0f
#define ADC_MAX   4095.0f

/* ---------- DFR0026 (PT550), émetteur suiveur : Vout = Iph * 470 ---------- */
#define DFR0026_R_LOAD_OHM   470.0f
#define PT550_UA_PER_LUX     0.90f       /* à recaler avec un luxmètre */
#define DFR0026_MV_PER_LUX   (DFR0026_R_LOAD_OHM * PT550_UA_PER_LUX / 1000.0f)

#define Temps_eveille 30 //30s

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

// application router ID (LSBF)
static const u1_t APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// unique device ID (LSBF)
static const u1_t DEVEUI[8] = { 0x4C, 0x8F, 0x07, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };

// device-specific AES key (MSBF)
static const u1_t DEVKEY[16] = { 0xA1, 0x99, 0x31, 0x2B, 0xF2, 0xD4, 0x43, 0x61,
		0x56, 0x08, 0xBC, 0x1F, 0x51, 0x1B, 0xEA, 0x2F };

static osjob_t blinkjob;

static u1_t ledstate = 0;

static cayenne_lpp_t lpp;   // statique : buffer de 51 octets, pas sur la pile

RGBLCD1602_t Ecran_I2C; //creation de l'objet

volatile uint8_t pagestate = 0;

volatile uint8_t eveil = 1;

volatile uint32_t time_sleep=0;

Color Couleur_Attente = {0, 128, 255};
Color Couleur_connecte = {0, 255, 0};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
float lm94021_adc_to_tempC(u2_t adc) {
    // 1) code ADC -> tension en mV
    float v = (float)adc * VREF_MV / ADC_MAX;

    // 2) inversion de la parabole : 0.00176*x^2 + 5.506*x + (V - 870.6) = 0
    const float a = 0.00176f;
    const float b = 5.506f;
    float c = v - 870.6f;

    float disc = b*b - 4.0f*a*c;
    if (disc < 0.0f) disc = 0.0f;        // garde-fou

    float x = (-b + sqrtf(disc)) / (2.0f * a);

    return x + 30.0f;                    // T = x + 30
}

float LM35_GetTemperature(u2_t adc)
{
    return (330.0f * adc) / 4095.0f;
}

uint16_t dfr0026_adc_to_lux(u2_t adc) {
    float v_mv = (float)adc * VREF_MV / ADC_MAX;
    float lux  = v_mv / DFR0026_MV_PER_LUX;
    if (lux > 65535.0f) lux = 65535.0f;
    return (uint16_t)(lux + 0.5f);
}

/* ---------- ADC : lecture d'un canal ---------- */
static u2_t adc_read_channel(uint32_t channel) {
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel      = channel;
	sConfig.Rank         = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
	sConfig.SingleDiff   = ADC_SINGLE_ENDED;
	sConfig.OffsetNumber = ADC_OFFSET_NONE;
	sConfig.Offset       = 0;
	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	u2_t val = 0;
	HAL_ADC_Start(&hadc1);
	if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK)
		val = (u2_t) HAL_ADC_GetValue(&hadc1);
	HAL_ADC_Stop(&hadc1);
	return val;
}

static void blinkfunc(osjob_t *j) {
// toggle LED
	ledstate = !ledstate;
	//debug_led(ledstate);

// reschedule blink job
	DFRobot_RGBLCD1602_setRGB_control(&Ecran_I2C,ledstate,Couleur_Attente.red,Couleur_Attente.green,Couleur_Attente.blue);

	os_setTimedCallback(j, os_getTime() + ms2osticks(100), blinkfunc);
}

void RGBLCD1602_ECRAN_start_com(RGBLCD1602_t *lcd)
{
	DFRobot_RGBLCD1602_clear(lcd);

	DFRobot_RGBLCD1602_setCursor(lcd, 0, 0);
	DFRobot_RGBLCD1602_print(lcd, "  Lancement Com ");

	DFRobot_RGBLCD1602_setCursor(lcd, 0, 1);
	DFRobot_RGBLCD1602_print(lcd, "Attente downlink");
}


// provide application router ID (8 bytes, LSBF)
void os_getArtEui(u1_t *buf) {
	memcpy(buf, APPEUI, 8);
}
// provide device ID (8 bytes, LSBF)
void os_getDevEui(u1_t *buf) {
	memcpy(buf, DEVEUI, 8);
}
// provide device key (16 bytes)
void os_getDevKey(u1_t *buf) {
	memcpy(buf, DEVKEY, 16);
}
void initsensor() {
// Here you init your sensors
}

void initfunc(osjob_t *j) {
	// intialize sensor hardware
	initsensor();
	// reset MAC state
	LMIC_reset();
	// start joining
	LMIC_startJoining();

	//LMIC_setDrTxpow(DR_SF9, 14);
}
u2_t readsensor_temp() {
	//HAL_GPIO_WritePin(ALIM_TEMP_GPIO_Port, ALIM_TEMP_Pin, 1);
	//HAL_Delay(10);
	u2_t temp_val = adc_read_channel(ADC_CHANNEL_9);				///
	//HAL_GPIO_WritePin(ALIM_TEMP_GPIO_Port, ALIM_TEMP_Pin, 0);
	return temp_val;
}

u2_t readsensor_lux() {
	u2_t lux_val = adc_read_channel(ADC_CHANNEL_15);
	return lux_val;
}


static osjob_t reportjob;
// report sensor value every minute
static void reportfunc(osjob_t *j) {
	// read sensor temp
	u2_t temp_val = readsensor_temp();
	float sensor_temp = LM35_GetTemperature (temp_val);	//lm94021_adc_to_tempC(temp_val);
	debug_time();
	debug_valfloat("Onboard sensor temp -> ", sensor_temp, 4);
	debug_str(" °C\r\n");

	// read sensor lux
	u2_t lux_val = readsensor_lux();
	uint16_t sensor_lux = dfr0026_adc_to_lux(lux_val);
	debug_time();
	debug_valdec("Onboard sensor lux -> ", sensor_lux);
	debug_str(" lux \r\n");

	// encodage Cayenne LPP
	cayenne_lpp_reset(&lpp);
	cayenne_lpp_add_temperature(&lpp, 1, sensor_temp);        // canal 1 : 0.1 °C
	cayenne_lpp_add_luminosity(&lpp, 2, sensor_lux);		  // canal 2 : lux
	// prepare and schedule data for transmission
	LMIC_setTxData2(1, lpp.buffer, lpp.cursor, 0);            // port 1, 8 octets, unconfirmed

	// Affichage sur l'écran I2C
	char Texte[20];
	if (pagestate ==0)
	{
		sprintf(Texte,"Lum  : %u lux   ",sensor_lux);
		DFRobot_RGBLCD1602_setCursor(&Ecran_I2C, 0, 0);
		DFRobot_RGBLCD1602_print(&Ecran_I2C,Texte);

		sprintf(Texte,"Temp : %0.2f C     ",sensor_temp);
		DFRobot_RGBLCD1602_setCursor(&Ecran_I2C, 0, 1);
		DFRobot_RGBLCD1602_print(&Ecran_I2C,Texte);
	}
	else {
		DFRobot_RGBLCD1602_clear(&Ecran_I2C);

		sprintf(Texte,"  Connexion OK  ");
		DFRobot_RGBLCD1602_setCursor(&Ecran_I2C, 0, 0);
		DFRobot_RGBLCD1602_print(&Ecran_I2C,Texte);
	}

	if (eveil ==1)
	{//on regarde si l'écran est retro éclairé
		u4_t sec = ( osticks2ms(os_getTime()) / 1000 );
		if (sec >= (time_sleep + Temps_eveille) )
		{
			DFRobot_RGBLCD1602_setRGB(&Ecran_I2C, 0,0,0);
			eveil =0;

		}
		else {
			DFRobot_RGBLCD1602_setRGB(&Ecran_I2C, Couleur_connecte.red,Couleur_connecte.green,Couleur_connecte.blue);
		}
	}
	// reschedule job in 15 seconds
	//os_setTimedCallback(j, os_getTime() + sec2osticks(15), reportfunc);
}

//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////
void onEvent(ev_t ev) {
	debug_event(ev);   // source unique du nom de l'event
	switch (ev) {
	case EV_JOINING:
		blinkfunc(&blinkjob);
		break;
	case EV_JOINED:
		// kick-off periodic sensor job
		os_clearCallback(&blinkjob);
		//debug_led(1);
		DFRobot_RGBLCD1602_setRGB(&Ecran_I2C, 0,255,0);
		LMIC_setAdrMode(0);
		LMIC_setLinkCheckMode(0);
		LMIC_setDrTxpow(DR_SF7, 20);
		reportfunc(&reportjob);
		break;
	case EV_TXCOMPLETE:
		if (LMIC.txrxFlags & TXRX_ACK)
			debug_str("  -> received ack\r\n");
		if (LMIC.dataLen) {
			debug_time();
		    debug_val("Payload RX ->  ", LMIC.dataLen); //nombre de bits reçu
		    debug_val(", Port : ", LMIC.frame[LMIC.dataBeg - 1]);
		    debug_str(", Data : ");
		    debug_buf(LMIC.frame + LMIC.dataBeg, LMIC.dataLen);
		    debug_char('\n');
		}
		os_setTimedCallback(&reportjob, os_getTime() + ms2osticks(200), reportfunc); //200ms callback du job
		break;
	case EV_JOIN_FAILED:
	case EV_SCAN_TIMEOUT:
	case EV_BEACON_FOUND:
	case EV_BEACON_MISSED:
	case EV_BEACON_TRACKED:
	case EV_RFU1:
	case EV_REJOIN_FAILED:
	case EV_LOST_TSYNC:
	case EV_RESET:
	case EV_RXCOMPLETE:
	case EV_LINK_DEAD:
	case EV_LINK_ALIVE:
	case EV_TXSTART:
		// nom deja logge par debug_event() en haut, rien a ajouter
		break;
	default:
		debug_str("  -> unknown event\r\n");
		break;
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
  MX_SPI3_Init();
  MX_ADC1_Init();
  MX_TIM16_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

	//HAL_GPIO_WritePin(ALIM_TEMP_GPIO_Port, ALIM_TEMP_Pin, 1);

	HAL_TIM_Base_Start_IT(&htim16);   // <----------- change to your setup

	__HAL_SPI_ENABLE(&hspi3);        // <----------- change to your setup

	osjob_t initjob;

	// initialize runtime env
	os_init();
	// initialize debug library
	debug_init();
	// setup initial job
	//os_setCallback(&hellojob, hellofunc);

	RGBLCD1602_ECRAN_I2C_Init(&Ecran_I2C,&hi2c1,0,128,255);

	os_setCallback(&initjob, initfunc);
	// execute scheduled jobs and events
	RGBLCD1602_ECRAN_start_com(&Ecran_I2C);

	os_runloop();
	// (not reached)
	return 0;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
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
	while (1) {
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
