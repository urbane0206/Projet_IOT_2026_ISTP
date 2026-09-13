/*!
 * @file RGBLCD1602.h
 * @brief DFRobot_RGBLCD1602 driver ported to STM32 HAL
 * @copyright	Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @maintainer [yangfeng](feng.yang@dfrobot.com)
 * @version  V1.0
 * @date  2021-09-24
 * @url https://github.com/DFRobot/DFRobot_RGBLCD1602
 *
 * Modif : système de couleur simplifié -> RGBLCD1602_setColor(lcd, 0xRRGGBB)
 */

#ifndef __DFRobot_RGBLCD1602_H__
#define __DFRobot_RGBLCD1602_H__

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "i2c.h"

typedef struct
{
    I2C_HandleTypeDef *hi2c;

    uint8_t lcdAddr;
    uint8_t rgbAddr;

    uint8_t cols;
    uint8_t rows;

    uint8_t numLines;
    uint8_t currLine;

    uint8_t showFunction;
    uint8_t showControl;
    uint8_t showMode;

    uint8_t REG_RED;       // pwm2
    uint8_t REG_GREEN;     // pwm1
    uint8_t REG_BLUE;      // pwm0
    uint8_t REG_ONLY;      // pwm0

} RGBLCD1602_t;


/*!
 *  @brief Device I2C Address
 */
#define LCD_ADDRESS     (0x7C >> 1)


/*
Change the RGBAddr value based on the hardware version
-----------------------------------------
       Module         | Version| RGBAddr|
-----------------------------------------
  LCD1602 Module      |  V1.0  | 0x60   |
-----------------------------------------
  LCD1602 Module      |  V1.1  | 0x6B   |
-----------------------------------------
  LCD1602 RGB Module  |  V1.0  | 0x60   |
-----------------------------------------
  LCD1602 RGB Module  |  V2.0  | 0x2D   |
-----------------------------------------
*/
#define RGBAddr  0x2D


/*!
 *  @brief Couleurs du rétroéclairage, format 0xRRGGBB.
 *         Utilisables avec RGBLCD1602_setColor(), ou n'importe quel hexa perso.
 */
#define LCD_COLOR_OFF      0x000000
#define LCD_COLOR_WHITE    0xFFFFFF
#define LCD_COLOR_RED      0xFF0000
#define LCD_COLOR_GREEN    0x00FF00
#define LCD_COLOR_BLUE     0x0000FF
#define LCD_COLOR_YELLOW   0xFFFF00
#define LCD_COLOR_CYAN     0x00FFFF
#define LCD_COLOR_MAGENTA  0xFF00FF
#define LCD_COLOR_ORANGE   0xFF6000
#define LCD_COLOR_PURPLE   0x8000FF
#define LCD_COLOR_PINK     0xFF2080

/*!
 *  @brief registres du contrôleur RGB (PCA9633)
 */
#define REG_MODE1   0x00
#define REG_MODE2   0x01
#define REG_OUTPUT  0x08

/*!
 *  @brief commands
 */
#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

/*!
 *  @brief flags for display entry mode
 */
#define LCD_ENTRYRIGHT           0x00
#define LCD_ENTRYLEFT            0x02
#define LCD_ENTRYSHIFTINCREMENT  0x01
#define LCD_ENTRYSHIFTDECREMENT  0x00

/*!
 *  @brief flags for display on/off control
 */
#define LCD_DISPLAYON   0x04
#define LCD_DISPLAYOFF  0x00
#define LCD_CURSORON    0x02
#define LCD_CURSOROFF   0x00
#define LCD_BLINKON     0x01
#define LCD_BLINKOFF    0x00

/*!
 *  @brief flags for display/cursor shift
 */
#define LCD_DISPLAYMOVE  0x08
#define LCD_CURSORMOVE   0x00
#define LCD_MOVERIGHT    0x04
#define LCD_MOVELEFT     0x00

/*!
 *  @brief flags for function set
 */
#define LCD_8BITMODE  0x10
#define LCD_4BITMODE  0x00
#define LCD_2LINE     0x08
#define LCD_1LINE     0x00
#define LCD_5x10DOTS  0x04
#define LCD_5x8DOTS   0x00


/*******************************helpers "ECRAN"*******************************/

/**
 * @brief Init complète de l'écran + couleur de départ.
 * @param rgb couleur 0xRRGGBB (ex : LCD_COLOR_WHITE)
 */
void RGBLCD1602_ECRAN_I2C_Init(RGBLCD1602_t *lcd, I2C_HandleTypeDef *hi2c, uint32_t rgb);

/**
 * @brief Règle la couleur du rétroéclairage.
 * @param rgb couleur 0xRRGGBB (ex : LCD_COLOR_GREEN ou 0x20A0FF)
 */
void RGBLCD1602_setColor(RGBLCD1602_t *lcd, uint32_t rgb);

/**
 * @brief Allume la couleur rgb si activation != 0, sinon éteint le rétroéclairage.
 */
void RGBLCD1602_setColor_control(RGBLCD1602_t *lcd, uint8_t activation, uint32_t rgb);

/**
 * @brief Affiche "Nucleo L476RG" sur la première ligne et "LCD 1602 RGB" sur la seconde
 */
void RGBLCD1602_ECRAN_splash_screen(RGBLCD1602_t *lcd);


/*******************************public (DFRobot)*******************************/

void RGBLCD1602_ECRAN_start_com(RGBLCD1602_t *lcd, u1_t datarate, u4_t freq);

/**
 * @brief Constructor
 */
void DFRobot_RGBLCD1602(RGBLCD1602_t *lcd, I2C_HandleTypeDef *hi2c, uint8_t lcdAddr, uint8_t rgbAddr, uint8_t lcdCols, uint8_t lcdRows);

/**
 * @brief initialize the LCD and master IIC
 */
void DFRobot_RGBLCD1602_init(RGBLCD1602_t *lcd);

/**
 * @brief print a string
 */
void DFRobot_RGBLCD1602_print(RGBLCD1602_t *lcd, char *str);

/**
 * @brief clear the display and return the cursor to position 0
 */
void DFRobot_RGBLCD1602_clear(RGBLCD1602_t *lcd);

/**
 * @brief return the cursor to (0,0)
 */
void DFRobot_RGBLCD1602_home(RGBLCD1602_t *lcd);

/**
 * @brief Turn off the display
 */
void DFRobot_RGBLCD1602_noDisplay(RGBLCD1602_t *lcd);

/**
 * @brief Turn on the display
 */
void DFRobot_RGBLCD1602_display(RGBLCD1602_t *lcd);

/**
 * @brief Turn off the blinking cursor
 */
void DFRobot_RGBLCD1602_stopBlink(RGBLCD1602_t *lcd);

/**
 * @brief Turn on the blinking cursor
 */
void DFRobot_RGBLCD1602_blink(RGBLCD1602_t *lcd);

/**
 * @brief Turn off the underline cursor
 */
void DFRobot_RGBLCD1602_noCursor(RGBLCD1602_t *lcd);

/**
 * @brief Turn on the underline cursor
 */
void DFRobot_RGBLCD1602_cursor(RGBLCD1602_t *lcd);

/**
 * @brief scroll display left
 */
void DFRobot_RGBLCD1602_scrollDisplayLeft(RGBLCD1602_t *lcd);

/**
 * @brief scroll display right
 */
void DFRobot_RGBLCD1602_scrollDisplayRight(RGBLCD1602_t *lcd);

/**
 * @brief text flows Left to Right
 */
void DFRobot_RGBLCD1602_leftToRight(RGBLCD1602_t *lcd);

/**
 * @brief text flows Right to Left
 */
void DFRobot_RGBLCD1602_rightToLeft(RGBLCD1602_t *lcd);

/**
 * @brief 'left justify' text from the cursor
 */
void DFRobot_RGBLCD1602_noAutoscroll(RGBLCD1602_t *lcd);

/**
 * @brief 'right justify' text from the cursor
 */
void DFRobot_RGBLCD1602_autoscroll(RGBLCD1602_t *lcd);

/**
 * @brief fill one of the 8 CGRAM locations with a custom character
 * @param location range 0-7
 * @param charmap  8-byte array
 */
void DFRobot_RGBLCD1602_customSymbol(RGBLCD1602_t *lcd, uint8_t location, uint8_t charmap[]);

/**
 * @brief set cursor position
 * @param col 0-15
 * @param row 0-1
 */
void DFRobot_RGBLCD1602_setCursor(RGBLCD1602_t *lcd, uint8_t col, uint8_t row);

/**
 * @brief set backlight RGB (composantes séparées, 0-255)
 */
void DFRobot_RGBLCD1602_setRGB(RGBLCD1602_t *lcd, uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief set backlight PWM output
 * @param color  REG_RED / REG_GREEN / REG_BLUE
 * @param pwm    0-255
 */
void DFRobot_RGBLCD1602_setPWM(RGBLCD1602_t *lcd, uint8_t color, uint8_t pwm);

/**
 * @brief close the backlight
 */
void DFRobot_RGBLCD1602_closeBacklight(RGBLCD1602_t *lcd);

/**
 * @brief set the backlight to white
 */
void DFRobot_RGBLCD1602_setColorWhite(RGBLCD1602_t *lcd);

/**
 * @brief backlight on (white) / off
 */
void DFRobot_RGBLCD1602_setBacklight(RGBLCD1602_t *lcd, bool mode);

/**
 * @brief write one character
 */
size_t DFRobot_RGBLCD1602_write(RGBLCD1602_t *lcd, uint8_t value);

/**
 * @brief send a command
 */
void DFRobot_RGBLCD1602_command(RGBLCD1602_t *lcd, uint8_t data);


/*******************************private*******************************/

/**
 * @brief the initialization sequence (HD44780)
 * @param rows      1 or 2
 * @param charSize  LCD_5x8DOTS / LCD_5x10DOTS
 */
void DFRobot_RGBLCD1602_begin(RGBLCD1602_t *lcd, uint8_t rows, uint8_t charSize);

/**
 * @brief raw I2C send to the LCD controller
 */
void DFRobot_RGBLCD1602_send(RGBLCD1602_t *lcd, uint8_t *data, uint8_t len);

/**
 * @brief write one register of the RGB controller
 */
void DFRobot_RGBLCD1602_setReg(RGBLCD1602_t *lcd, uint8_t addr, uint8_t data);


#endif /* __DFRobot_RGBLCD1602_H__ */
