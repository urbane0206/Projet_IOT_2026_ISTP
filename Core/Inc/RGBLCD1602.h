/*!
 * @file DFRobot_RGBLCD1602.h
 * @brief DFRobot_RGBLCD1602 class infrastructure
 * @copyright	Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @maintainer [yangfeng](feng.yang@dfrobot.com)
 * @version  V1.0
 * @date  2021-09-24
 * @url https://github.com/DFRobot/DFRobot_RGBLCD1602
 */

#ifndef __DFRobot_RGBLCD1602_H__
#define __DFRobot_RGBLCD1602_H__
#include <inttypes.h>
#include "i2c.h"
#include "stdio.h"
#include <stdbool.h>
#include "string.h"

/*
typedef enum
{
    false = 0,
    true = 1
} bool;
*/

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
    uint8_t REG_GREEN;       // pwm1
    uint8_t REG_BLUE;       // pwm0
    uint8_t REG_ONLY;       // pwm0

} RGBLCD1602_t;


/*!
 *  @brief Device I2C Arress
 */
#define LCD_ADDRESS     (0x7c>>1)


/*
Change the RGBaddr value based on the hardware version
-----------------------------------------
       Moudule        | Version| RGBAddr|
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
 *  @brief color define
 */ 
#define WHITE 0
#define RED 1
#define GREEN 2
#define BLUE 3
#define REG_MODE1 0x00
#define REG_MODE2 0x01
#define REG_OUTPUT 0x08

/*!
 *  @brief commands
 */
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

/*!
 *  @brief flags for display entry mode
 */
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

/*!
 *  @brief flags for display on/off control
 */
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

/*!
 *  @brief flags for display/cursor shift
 */
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

/*!
 *  @brief flags for function set
 */
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

/*******************************public*******************************/

  /**
   * @fn DFRobot_RGBLCD1602
   * @brief Print des strings
   */
  void DFRobot_RGBLCD1602_print(RGBLCD1602_t *lcd, char *str);


  /**
   * @fn DFRobot_RGBLCD1602
   * @brief Constructor
   */
	void DFRobot_RGBLCD1602(RGBLCD1602_t *lcd,I2C_HandleTypeDef *hi2c,uint8_t lcdAddr, uint8_t rgbAddr,uint8_t lcdCols,uint8_t lcdRows);
//(uint8_t RGBAddr,uint8_t lcdCols  =16  ,uint8_t lcdRows  =2  ,TwoWire *pWire  =&Wire  ,uint8_t lcdAddr  =LCD_ADDRESS  );
  /**
   * @fn init
   * @brief initialize the LCD and master IIC
   */ 
	void DFRobot_RGBLCD1602_init(RGBLCD1602_t *lcd);

  /**
   * @fn clear
   * @brief clear the display and return the cursor to the initial position (position 0)
   */
	void DFRobot_RGBLCD1602_clear(RGBLCD1602_t *lcd);

  /**
   * @fn home
   * @brief return the cursor to the initial position (0,0)
   */
	void DFRobot_RGBLCD1602_home(RGBLCD1602_t *lcd);

    /**
     * @fn noDisplay
     * @brief Turn off the display
     */
	void DFRobot_RGBLCD1602_noDisplay(RGBLCD1602_t *lcd);

  /**
   * @fn display
   * @brief Turn on the display
   */
	void DFRobot_RGBLCD1602_display(RGBLCD1602_t *lcd);

  /**
   * @fn stopBlink
   * @brief Turn  off the blinking showCursor
   */
	void DFRobot_RGBLCD1602_stopBlink(RGBLCD1602_t *lcd);

  /**
   * @fn blink
   * @brief Turn on  the blinking showCursor
   */
	void DFRobot_RGBLCD1602_blink(RGBLCD1602_t *lcd);

  /**
   * @fn noCursor
   * @brief Turn off the underline showCursor 
   */
	void DFRobot_RGBLCD1602_noCursor(RGBLCD1602_t *lcd);

  /**
   * @fn cursor
   * @brief Turn on the underline showCursor 
   */
	void DFRobot_RGBLCD1602_cursor(RGBLCD1602_t *lcd);

  /**
   * @fn scrollDisplayLeft
   * @brief scroll left to display
   */
  void DFRobot_RGBLCD1602_scrollDisplayLeft(RGBLCD1602_t *lcd);

  /**
   * @fn scrollDisplayRight
   * @brief scroll right to display
   */
  void DFRobot_RGBLCD1602_scrollDisplayRight(RGBLCD1602_t *lcd);
 
  /**
   * @fn leftToRight
   * @brief This is for text that flows Left to Right
   */
  void DFRobot_RGBLCD1602_leftToRight(RGBLCD1602_t *lcd);
 
  /**
   * @fn rightToLeft
   * @brief This is for text that flows Right to Left
   */
  void DFRobot_RGBLCD1602_rightToLeft(RGBLCD1602_t *lcd);

  /**
   * @fn noAutoscroll
   * @brief This will 'left justify' text from the showCursor
   */
  void DFRobot_RGBLCD1602_noAutoscroll(RGBLCD1602_t *lcd);
 
  /**
   * @fn autoscroll
   * @brief This will 'right justify' text from the showCursor
   */
  void DFRobot_RGBLCD1602_autoscroll(RGBLCD1602_t *lcd);
   
  /**
   * @fn customSymbol
   * @brief Allows us to fill the first 8 CGRAM locations with custom characters
   * @param location substitute character range (0-7)
   * @param charmap  character array the size is 8 bytes
   */
  void DFRobot_RGBLCD1602_customSymbol(RGBLCD1602_t *lcd,uint8_t location, uint8_t charmap[]);

  /**
   * @fn setCursor
   * @brief set cursor position
   * @param col columns optional range 0-15
   * @param row rows optional range 0-1，0 is the first row, 1 is the second row
   */
  void DFRobot_RGBLCD1602_setCursor(RGBLCD1602_t *lcd,uint8_t col, uint8_t row);
  
  /**
   * @fn setRGB
   * @brief set RGB
   * @param r  red   range(0-255)
   * @param g  green range(0-255)
   * @param b  blue  range(0-255)
   */
  void DFRobot_RGBLCD1602_setRGB(RGBLCD1602_t *lcd,uint8_t r, uint8_t g, uint8_t b);

  /**
   * @fn setPWM
   * @brief set backlight PWM output
   * @param color  backlight color  Preferences：REG_RED\REG_GREEN\REG_BLUE
   * @param pwm  color intensity   range(0-255)
   */
  void DFRobot_RGBLCD1602_setPWM(RGBLCD1602_t *lcd,uint8_t color, uint8_t pwm); // set pwm

  /**
   * @fn setColor
   * @brief backlight color
   * @param color  backlight color  Preferences： WHITE\RED\GREEN\BLUE
   */
  void DFRobot_RGBLCD1602_setColor(RGBLCD1602_t *lcd,uint8_t color);

  /**
   * @fn closeBacklight
   * @brief close the backlight
   */
  void DFRobot_RGBLCD1602_closeBacklight(RGBLCD1602_t *lcd);

  /**
   * @fn setColorWhite
   * @brief set the backlight to white
   */
  void DFRobot_RGBLCD1602_setColorWhite(RGBLCD1602_t *lcd);

  /**
   * @fn write
   * @brief write character
   * @param data the written data
   */
  size_t DFRobot_RGBLCD1602_write(RGBLCD1602_t *lcd, uint8_t value);
  //virtual size_t write(uint8_t data);

  /**
   * @fn command
   * @brief send command
   * @param data the sent command
   */
  void DFRobot_RGBLCD1602_command(RGBLCD1602_t *lcd,uint8_t data);

  /**
   * @fn setBacklight
   * @brief set the backlight
   * @param mode  true indicates the backlight is turned on and set to white, false indicates the backlight is turned off
   */
  void DFRobot_RGBLCD1602_setBacklight(RGBLCD1602_t *lcd,bool mode);
  //using Print::write;
  

  /**
   * @fn begin
   * @brief the initialization function
   * @param row rows optional range 0-1，0 is the first row, 1 is the second row
   * @param charSize  character size LCD_5x8DOTS\LCD_5x10DOTS
   */
  void DFRobot_RGBLCD1602_begin(RGBLCD1602_t *lcd,uint8_t rows, uint8_t charSize);
  	  	  // (uint8_t rows, uint8_t charSize = LCD_5x8DOTS)
  /**
   * @fn send
   * @brief set cursor
   * @param data the data to send
   * @param len length of the data
   */
  void DFRobot_RGBLCD1602_send(RGBLCD1602_t *lcd,uint8_t *data, uint8_t len);

  /**
   * @fn setReg
   * @brief Configure related registers
   * @param addr  The address of the register to be operated on
   * @param data  The data to be written
   */
  void DFRobot_RGBLCD1602_setReg(RGBLCD1602_t *lcd,uint8_t addr, uint8_t data);

  /**
   * @fn DFRobot_RGBLCD1602
   * @brief Init l'écran et active le backlight
   */
  void RGBLCD1602_ECRAN_I2C_Init(RGBLCD1602_t *lcd, I2C_HandleTypeDef *hi2c, uint8_t red, uint8_t green, uint8_t blue);

  /**
   * @fn DFRobot_RGBLCD1602
   * @brief active ou non la couleur du RGB du backlight
   */
  void DFRobot_RGBLCD1602_setRGB_control(RGBLCD1602_t *lcd, uint8_t activation, uint8_t red, uint8_t green, uint8_t blue);

  /**
   * @fn DFRobot_RGBLCD1602
   * @brief Affiche "Nucleo L476RG" sur la première ligne et "LCD 1602 RGB" sur la seconde ligne
   */
  void RGBLCD1602_ECRAN_splash_screen(RGBLCD1602_t *lcd);


#endif
