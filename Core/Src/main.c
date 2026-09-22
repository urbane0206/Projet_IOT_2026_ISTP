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

typedef enum {
	PAGE_1 = 0,
	PAGE_2,
	PAGE_3,
	PAGE_4,
	PAGE_5,
	PAGE_6,
	PAGE_7,
	PAGE_8,
	PAGE_9,
	PAGE_ERROR,
	PAGE_CLEARED
} page_t;

typedef enum {
	AUTO,
	OVERRIDE_ON,
	OVERRIDE_OFF
} rly_mode_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define VREF_MV   3300.0f
#define ADC_MAX   4095.0f

/* ---------- DFR0026 (PT550), émetteur suiveur : Vout = Iph * 470 ---------- */
#define DFR0026_R_LOAD_OHM   470.0f
#define PT550_UA_PER_LUX     0.90f       /* à recaler avec un luxmètre */
#define DFR0026_MV_PER_LUX   (DFR0026_R_LOAD_OHM * PT550_UA_PER_LUX / 1000.0f)

#define WAKE_UP_TIME 10

#define LIGHT_HOLD_TIME 30

#define ONE_Hz    1000
#define TWO_Hz    500
#define THREE_Hz  333
#define FOUR_Hz   250
#define FIVE_Hz   200
#define SIX_Hz    167
#define SEVEN_Hz  143
#define EIGHT_Hz  125
#define NINE_Hz   111
#define TEN_Hz    100
#define FIFTEEN_Hz      67
#define TWENTY_Hz       50
#define TWENTY_FIVE_Hz  40
#define THIRTY_Hz       33
#define THIRTY_FIVE_Hz  29
#define FORTY_Hz        25
#define FORTY_FIVE_Hz   22
#define FIFTY_Hz        20

#define SCREEN_CURSOR_ZERO 0
#define SCREEN_UP 0
#define SCREEN_DOWN 1

#define NORMAL_BUF 0
#define ERROR_BUF_UP 1
#define ERROR_BUF_DOWN 2

#define ON 1
#define OFF 0

#define MAX_TEMP_THR 45.0f
#define FAN_THR_ON 30
#define FAN_THR_OFF 28

#define FLAME_TH_ON      1400
#define FLAME_TH_OFF     1000
#define FLAME_CONFIRM_N  3

#define LM35_MV_PER_DEG 10.0f

#define SEUIL_LUMI 1000

#define ADC_BUF_SIZE 300

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */


static const u1_t APPEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };// application router ID (LSBF)
static const u1_t DEVEUI[8] = { 0x4C, 0x8F, 0x07, 0xD0, 0x7E, 0xD5, 0xB3, 0x70 };// unique device ID (LSBF)
static const u1_t DEVKEY[16] = { 0xA1, 0x99, 0x31, 0x2B, 0xF2, 0xD4, 0x43, 0x61,
		0x56, 0x08, 0xBC, 0x1F, 0x51, 0x1B, 0xEA, 0x2F };// device-specific AES key (MSBF)
static const char *rly_mode_str[] = { "AUTO", "OVR ON", "OVR OFF" };

static osjob_t blinkjob;
static osjob_t backlightjob;
static osjob_t lightjob;
static osjob_t reportjob;
static osjob_t uptimejob;

osjob_t initjob;
osjob_t shortpressjob;
osjob_t longpressjob;
osjob_t pir_on_job;
osjob_t pir_off_job;
osjob_t periodic_job;

volatile uint8_t eveil = 1;
volatile uint32_t t_since_press = 0;
volatile uint32_t sweep[512];
volatile uint16_t adc_buf[ADC_BUF_SIZE];
volatile page_t pagestate = 0;

static uint32_t blink_rgb;
static uint32_t blink_ms;
static uint8_t  ledstate = 0;
static uint8_t  error = 0;
static uint8_t relay1_state = 0;
static uint8_t relay2_state = 0;
static rly_mode_t relay1_mode = 0;
static rly_mode_t relay2_mode = 0;
static cayenne_lpp_t lpp;   // statique : buffer de 51 octets, pas sur la pile

char lcd_text[20];
char lcd_error_text_up[20];
char lcd_error_text_down[20];

volatile float sensor_temp = 0;
volatile uint16_t sensor_lux = 0;
volatile uint8_t flame_detected = 0;
volatile uint8_t flame_cnt = 0;
volatile u2_t flame_raw = 0;

uint16_t m_sw = 0;
uint16_t pir_state = 0;
uint16_t error_cnt = 0;
uint16_t temp_high_error = 0;
uint16_t effraction_error = 0;
uint16_t incendie_error = 0;

RGBLCD1602_t Ecran_I2C; //creation de l'objet

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
/* USER CODE BEGIN PFP */
static const char *timesinceboot(void);
static void backlight_cmd(uint32_t rgb, uint8_t cmd);
static void draw_lcd(uint8_t cursor_y, uint8_t buf_selector);
static void clear_error(void);
static void change_page(void);
static void check_for_error(void);
static void relay1_cmd(uint8_t cmd);
static void relay2_cmd(uint8_t cmd);
static void relay_mode_selector(void);
static void flame_update(u2_t raw);
static void blinkfunc(osjob_t *j);
static void uptimefunc(osjob_t *j);
static void periodic_func(osjob_t *j);

void buzz_start(void);
void buzz_stop(void);
void blink_start(uint32_t rgb, uint32_t period_ms);
void blink_stop(void);
void alarm_start(uint32_t rgb, uint32_t blink_period_ms, const char *alarm_msg);
void alarm_stop(void);
void lcd_manager(uint8_t page);

float LM35_GetTemperature(u2_t adc);

uint16_t dfr0026_adc_to_lux(u2_t adc) ;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {

	if (hadc == &hadc1) {

		u2_t temp_val;
		u2_t lux_val;
		u2_t flame_val;
		uint32_t holder = 0;
		uint32_t l;

		// Temperature
		for(l = 0; l < 298; l += 3){
			holder += adc_buf[l];
		}
		temp_val = holder/100;
		sensor_temp = LM35_GetTemperature (temp_val);

		// Luminosite
		holder = 0;
		for(l = 1; l < 299; l += 3){
			holder += adc_buf[l];
		}
		lux_val = holder/100;
		sensor_lux = dfr0026_adc_to_lux(lux_val);

		// Flame
		holder = 0;
		for(l = 2; l < 300; l += 3){
			holder += adc_buf[l];
		}
		flame_val = holder/100;
		flame_raw = flame_val;
		flame_update(flame_val);
	}

}

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
    float v_mv = (float)adc * VREF_MV / ADC_MAX;
    return v_mv / LM35_MV_PER_DEG;
}

static void flame_update(u2_t raw) {
    u2_t th = flame_detected ? FLAME_TH_OFF : FLAME_TH_ON;
    uint8_t cond = flame_detected ? (raw < th) : (raw > th);

    if (cond) {
        if (++flame_cnt >= FLAME_CONFIRM_N) {
            flame_detected = !flame_detected;
            flame_cnt = 0;
        }
    } else {
        flame_cnt = 0;
    }
}

uint16_t dfr0026_adc_to_lux(u2_t adc) {
    float v_mv = (float)adc * VREF_MV / ADC_MAX;
    float lux  = v_mv / DFR0026_MV_PER_LUX;
    if (lux > 65535.0f) lux = 65535.0f;
    return (uint16_t)(lux + 0.5f);
}

void RGBLCD1602_ECRAN_start_com(RGBLCD1602_t *lcd, u1_t datarate, u4_t freq)
{
	char ligne[20];

	DFRobot_RGBLCD1602_clear(lcd);

	DFRobot_RGBLCD1602_setCursor(lcd, 0, 0);
	DFRobot_RGBLCD1602_print(lcd, "Connexion LoRa...");

	/* ex : "SF7   868.1 MHz" */
	snprintf(ligne, sizeof ligne, "SF%-2u-> %lu.%lu MHz", (unsigned)(12 - datarate), (unsigned long)(freq / 1000000),(unsigned long)((freq / 100000) % 10));
	DFRobot_RGBLCD1602_setCursor(lcd, 0, 1);
	DFRobot_RGBLCD1602_print(lcd, ligne);
}

void set_tone_frequency(uint32_t freq1_hz, uint32_t freq2_hz) {
    if (freq1_hz == 0 || freq2_hz == 0) {
    	HAL_TIM_OC_Stop(&htim2, TIM_CHANNEL_1); // Silence
        return;
    }
     uint32_t arr;
     uint32_t sweep_size = freq2_hz - freq1_hz;

    // Calcul de la période : 1 000 000 / freq - 1


    for(uint32_t i = freq1_hz; i < freq2_hz; i++) {

        arr = (1000000 / (2*i)) - 1;

    	sweep[i-freq1_hz] = arr;
    }

    // Mise à jour des registres
	HAL_TIM_OC_Start(&htim2, TIM_CHANNEL_1);
	HAL_TIM_Base_Start(&htim6); // 500hz
	HAL_DMA_Start(htim6.hdma[TIM_DMA_ID_UPDATE], (uint32_t)sweep, (uint32_t)&TIM2->ARR, sweep_size);
	__HAL_TIM_ENABLE_DMA(&htim6, TIM_DMA_UPDATE);
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

static void uptimefunc(osjob_t *j) {
	lcd_manager(pagestate);
	os_setTimedCallback(&uptimejob, os_getTime() + sec2osticks(1), uptimefunc);
}

void alarm_start(uint32_t rgb, uint32_t blink_period_ms, const char *alarm_msg) {
	error = 1;

	blink_rgb = rgb;
	blink_ms  = blink_period_ms;
	ledstate  = 0;

	if (alarm_msg != NULL) {
			strncpy(lcd_error_text_down, alarm_msg, sizeof(lcd_error_text_down) - 1);
			lcd_error_text_down[sizeof(lcd_error_text_down) - 1] = '\0';
		} else {
			//Fall back si alarm_msg = NULL
			snprintf(lcd_error_text_down, sizeof(lcd_error_text_down), "  ALARM ACTIVE");
		}

	lcd_manager(PAGE_ERROR);
	blink_start(rgb, blink_period_ms);
	buzz_start();
}

void buzz_start(void) {
	set_tone_frequency(1800, 2100);
}

void buzz_stop(void) {
	set_tone_frequency(0, 0);
}

void alarm_stop(void) {
	error = 0;
	blink_stop();
	buzz_stop();
}

static void blinkfunc(osjob_t *j) {
	ledstate = !ledstate;
	backlight_cmd(blink_rgb, ledstate);
	os_setTimedCallback(j, os_getTime() + ms2osticks(blink_ms), blinkfunc);
}

void blink_start(uint32_t rgb, uint32_t period_ms) {
	blink_rgb = rgb;
	blink_ms  = period_ms;
	ledstate  = 0;
	blinkfunc(&blinkjob);
}

void blink_stop(void) {
	os_clearCallback(&blinkjob);
}

void initfunc(osjob_t *j) {
	// reset MAC state
	LMIC_reset();
	// start joining
	LMIC_startJoining();

	LMIC_setDrTxpow(DR_SF7, 20);
}

u1_t  readsensor_mag_sw() {
	u1_t mag_sw_val = HAL_GPIO_ReadPin(MAG_SW_GPIO_Port, MAG_SW_Pin);
	return mag_sw_val;
}

static void periodic_func(osjob_t *j){
	check_for_error();
	relay_mode_selector();

	// read sensor magnetic sw
	u1_t mag_sw_state = readsensor_mag_sw();
	m_sw = mag_sw_state;



	os_setTimedCallback(&periodic_job, os_getTime() + ms2osticks(500), periodic_func);
}

static void lightoff(osjob_t *j) {
	relay2_state = 0;
	relay2_cmd(relay2_state);
}

static void light_wake(void) {
	relay2_state = 1;
	relay2_cmd(relay2_state);
	os_setTimedCallback(&lightjob, os_getTime() + sec2osticks(LIGHT_HOLD_TIME), lightoff);
}

static void backlightoff(osjob_t *j) {
	RGBLCD1602_setColor(&Ecran_I2C, LCD_COLOR_OFF);
	eveil = 0;
}

static void backlight_wake(uint32_t rgb) {
	eveil = 1;
	RGBLCD1602_setColor(&Ecran_I2C, rgb);
	os_setTimedCallback(&backlightjob, os_getTime() + sec2osticks(WAKE_UP_TIME), backlightoff);
}

static void backlight_cmd(uint32_t rgb, uint8_t cmd) {
	if (cmd) {
		RGBLCD1602_setColor(&Ecran_I2C, rgb);
	} else {
		RGBLCD1602_setColor(&Ecran_I2C, LCD_COLOR_OFF);
	}
}

static void draw_lcd(uint8_t cursor_y, uint8_t buf_selector) {
    char line_buf[17]; // 16 caractères + '\0'

    // %-16.16s : tronque à 16 max et complète avec des espaces si plus court
    if        (buf_selector == 1) {
    	snprintf(line_buf, sizeof(line_buf), "%-16.16s", lcd_error_text_up);
    } else if (buf_selector == 2) {
    	snprintf(line_buf, sizeof(line_buf), "%-16.16s", lcd_error_text_down);
    } else {
    	snprintf(line_buf, sizeof(line_buf), "%-16.16s", lcd_text);
    }

    DFRobot_RGBLCD1602_setCursor(&Ecran_I2C, SCREEN_CURSOR_ZERO, cursor_y);
    DFRobot_RGBLCD1602_print(&Ecran_I2C, line_buf);
}


void lcd_manager(uint8_t page) {

	if (error) {
		page = PAGE_ERROR;
	}

	switch (page) {
	case PAGE_1:
		// Ligne 0 : Titre
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 1  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);

		// Ligne 1 : Valeur
		snprintf(lcd_text, sizeof(lcd_text), "TEMP : %0.1f Deg", sensor_temp);
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_2 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 2  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), "LUM  : %u lux", sensor_lux);
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_3 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 3  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), "FEUX : %s", flame_detected ? "DANGER" : "AUCUN");
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_4 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 4  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), "PORTE : %s", m_sw ? "FERMEE" : "OUVERTE");
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_5 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 5  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), "MOUVEMENT : %s", pir_state ? "OUI" : "NON");
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_6 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 6  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), "RELAIS 1:%s", rly_mode_str[relay1_mode]);
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_7 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 7  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), "RELAIS 2:%s", rly_mode_str[relay2_mode]);
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_8 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 8  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof lcd_text, "SF%-2u-> %lu.%lu MHz", (unsigned)(12 - LMIC.datarate), (unsigned long)(LMIC.freq / 1000000),(unsigned long)((LMIC.freq / 100000) % 10));
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_9 :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), "     DATA 9  ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), "UP-TIME %s", timesinceboot());
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		break;
	case PAGE_ERROR :
		// TOP
		snprintf(lcd_error_text_up, sizeof(lcd_error_text_up), "   ERREUR : %02d", error_cnt);
		draw_lcd(SCREEN_UP,ERROR_BUF_UP);
		// BOTTOM
		//snprintf(lcd_error_text_down, sizeof(lcd_error_text_down), "     AUCUNE");
		draw_lcd(SCREEN_DOWN,ERROR_BUF_DOWN);
		break;
	case PAGE_CLEARED :
		// TOP
		snprintf(lcd_text, sizeof(lcd_text), " ERREUR CLEARED ");
		draw_lcd(SCREEN_UP,NORMAL_BUF);
		// BOTTOM
		snprintf(lcd_text, sizeof(lcd_text), " ");
		draw_lcd(SCREEN_DOWN,NORMAL_BUF);
		os_setTimedCallback(&uptimejob, os_getTime() + sec2osticks(2), uptimefunc);
		break;
	}

}

void shortpressfunc(osjob_t *j) {   /* appui bouton court, appelé via os_setCallback */
	if (error) {
		pagestate = PAGE_ERROR;
		alarm_stop();
	}
	change_page();
	backlight_wake(LCD_COLOR_WHITE);
	lcd_manager(pagestate);
	debug_time();
	debug_str("Change page cmd\r\n");
}

void longpressfunc(osjob_t *j) {   /* appui bouton long, appelé via os_setCallback seulement */
	//alarm_start(LCD_COLOR_RED, FIVE_Hz, FIVE_Hz, "    TEST MODE");
	backlight_wake(LCD_COLOR_WHITE);
	switch(pagestate) {
	case PAGE_1 : break;
	case PAGE_2 : break;
	case PAGE_3 : break;
	case PAGE_4 : break;
	case PAGE_5 : break;
	case PAGE_6 :
		relay1_mode = (rly_mode_t)((relay1_mode + 1) % 3);
		relay_mode_selector();
		lcd_manager(pagestate);
		break;
	case PAGE_7 :
		relay2_mode = (rly_mode_t)((relay2_mode + 1) % 3);
		relay_mode_selector();
		lcd_manager(pagestate);
		break;
	case PAGE_8 : break;
	case PAGE_9 : break;
	case PAGE_ERROR :
		clear_error();
		backlight_wake(LCD_COLOR_WHITE);
		break;
	default : break;

	}

	debug_time();
	debug_str("long press cmd\r\n");
}

void pir_on_func(osjob_t *j) {   /* declanchement on pir, appelé via os_setCallback seulement */
	pir_state = 1;
	debug_time();
	debug_str("PIR on cmd\r\n");
}

void pir_off_func(osjob_t *j) {   /* declanchement off pir, appelé via os_setCallback seulement */
	pir_state = 0;
	debug_time();
	debug_str("PIR off cmd\r\n");
}

static void clear_error(void) {
	incendie_error = 0;
	effraction_error = 0;
	temp_high_error = 0;
	error_cnt = 0;
	flame_detected = 0;
	snprintf(lcd_error_text_down, sizeof(lcd_error_text_down), " ");
	alarm_stop();
	os_clearCallback(&uptimejob);
	lcd_manager(PAGE_CLEARED);
}

static void change_page(void) {
	if (eveil) {

		pagestate ++;

		if (pagestate > PAGE_ERROR) {
			pagestate = 0;
		}
	}
}

static void relay1_cmd(uint8_t cmd) {
	HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, cmd);
}

static void relay2_cmd(uint8_t cmd) {
	HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, cmd);
}

static void check_for_error(void) {
	if (sensor_temp > MAX_TEMP_THR) {
		temp_high_error = 1;
		error_cnt ++;
		if (!error) alarm_start(LCD_COLOR_RED, FIVE_Hz, "     TEMP HIGH");
	}

	if (m_sw && !relay2_state && sensor_lux > SEUIL_LUMI) {
		effraction_error = 1;
		error_cnt ++;
		if (!error) alarm_start(LCD_COLOR_RED, FIVE_Hz, "EFFRACTION DETEC");
	}

	if (flame_detected){
		incendie_error = 1;
		error_cnt ++;
		if (!error) alarm_start(LCD_COLOR_RED, FIVE_Hz, "    INCENDIE");
	}
}

static void relay1_automation(void){
	//declanchement ventilation
	if (sensor_temp > FAN_THR_ON) {
		relay1_state = ON;
		relay1_cmd(relay1_state);
	} else if(sensor_temp <= FAN_THR_OFF){
		relay1_state = OFF;
		relay1_cmd(relay1_state);
	}
}

static void relay2_automation(void){
	//declanchement lumiere
	if (pir_state && sensor_lux < SEUIL_LUMI) { //allume que si lumiere dans le local insuffisant.
		light_wake();
	}
}

static void relay_mode_selector(void) {
	if (relay1_mode == AUTO) {
		relay1_automation();
	} else if (relay1_mode == OVERRIDE_ON) {
		relay1_state = ON;
		relay1_cmd(relay1_state);
	} else if (relay1_mode == OVERRIDE_OFF) {
		relay1_state = OFF;
		relay1_cmd(relay1_state);
	}

	if (relay2_mode == AUTO) {
		relay2_automation();
	} else if (relay2_mode == OVERRIDE_ON) {
		os_clearCallback(&lightjob);
		relay2_state = ON;
		relay2_cmd(relay2_state);
	} else if (relay2_mode == OVERRIDE_OFF) {
		os_clearCallback(&lightjob);
		relay2_state = OFF;
		relay2_cmd(relay2_state);
	}
}


static const char *timesinceboot(void) {
	static char buf[12];                       /* "hhh:mm:ss" + '\0' */
	u4_t total_sec = osticks2ms(os_getTime()) / 1000;
	u4_t h = total_sec / 3600;
	u4_t m = (total_sec / 60) % 60;
	u4_t s = total_sec % 60;

	snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",(unsigned long)h, (unsigned long)m, (unsigned long)s);
	return buf;
}

// report sensor value every 5sec
static void reportfunc(osjob_t *j) {
	// read sensor temp
	debug_time();
	debug_valfloat("Onboard sensor temp -> ", sensor_temp, 4);
	debug_str(" °C\r\n");

	// read sensor lux
	debug_time();
	debug_valdec("Onboard sensor lux -> ", sensor_lux);
	debug_str(" lux \r\n");

	// read sensor flame
	debug_time();
	debug_valdec("Onboard sensor flame raw -> ", flame_raw);
	debug_str("\r\n");
	debug_time();
	debug_valdec("Onboard sensor flame -> ", flame_detected);
	debug_str("\r\n");

	// read sensor magnetic sw
	debug_time();
	debug_valdec("Onboard sensor mag sw -> ", m_sw);
	debug_str("\r\n");

	// encodage Cayenne LPP
	cayenne_lpp_reset(&lpp);
	cayenne_lpp_add_temperature(&lpp, 1, sensor_temp);         // canal 1 : 0.1 °C
	cayenne_lpp_add_luminosity(&lpp, 2, sensor_lux);		   // canal 2 : lux
	cayenne_lpp_add_digital_input(&lpp, 3, flame_detected ? 1 : 0); // canal 3 : flame
	cayenne_lpp_add_digital_input(&lpp, 4, m_sw);			   // canal 4 : etat porte
	cayenne_lpp_add_presence(&lpp, 5, pir_state);			   // canal 5 : detecteur presence
	cayenne_lpp_add_digital_output(&lpp, 6, relay1_mode);      // canal 6 : etat relais 1
	cayenne_lpp_add_digital_output(&lpp, 7, relay2_mode);      // canal 7 : etat relais 2
	cayenne_lpp_add_digital_output(&lpp, 8, error_cnt);        // canal erreur : nb erreur
	cayenne_lpp_add_digital_output(&lpp, 9, incendie_error);   // canal erreur : incendie
	cayenne_lpp_add_digital_output(&lpp, 10, effraction_error);// canal erreur : effraction
	cayenne_lpp_add_digital_output(&lpp, 11, temp_high_error); // canal erreur : over temp of the local technique
	// prepare and schedule data for transmission
	LMIC_setTxData2(1, lpp.buffer, lpp.cursor, 0);             // port 1, 8 octets, unconfirmed
}

//////////////////////////////////////////////////
// LMIC EVENT CALLBACK
//////////////////////////////////////////////////
void onEvent(ev_t ev) {
	debug_event(ev);   // source unique du nom de l'event
	switch (ev) {
	case EV_JOINING:
		blink_start(LCD_COLOR_BLUE, ONE_Hz);
		break;
	case EV_JOINED:
		t_since_press = ( osticks2ms(os_getTime()) / 1000 );
		// kick-off periodic sensor job
		blink_stop();
		LMIC_setAdrMode(0);
		LMIC_setLinkCheckMode(0);
		backlight_wake(LCD_COLOR_WHITE);
		lcd_manager(pagestate);
		reportfunc(&reportjob);
		periodic_func(&periodic_job);
		uptimefunc(&uptimejob);
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
		os_setTimedCallback(&reportjob, os_getTime() + ms2osticks(1000), reportfunc);
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
		if (LMIC.opmode & OP_JOINING)
			RGBLCD1602_ECRAN_start_com(&Ecran_I2C, LMIC.datarate, LMIC.freq);
		break;
	default:
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
  MX_DMA_Init();
  MX_SPI3_Init();
  MX_ADC1_Init();
  MX_TIM16_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_TIM6_Init();
  MX_TIM15_Init();
  /* USER CODE BEGIN 2 */
	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

	HAL_TIM_Base_Start_IT(&htim16);   // <----------- change to your setup

	__HAL_SPI_ENABLE(&hspi3);        // <----------- change to your setup

	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, ADC_BUF_SIZE);

	HAL_TIM_Base_Start(&htim15);

	// initialize runtime env
	os_init();
	// initialize debug library
	debug_init();

	RGBLCD1602_ECRAN_I2C_Init(&Ecran_I2C,&hi2c1,LCD_COLOR_OFF);

	os_setCallback(&initjob, initfunc);
	// execute scheduled jobs and events
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
