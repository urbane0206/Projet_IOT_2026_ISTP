/*!
 * @file DFRobot_RGBLCD1602.cpp
 * @brief DFRobot_RGBLCD1602 class infrastructure, the implementation of basic methods
 * @copyright	Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @licence     The MIT License (MIT)
 * @maintainer [yangfeng](feng.yang@dfrobot.com)
 * @version  V1.0
 * @date  2021-09-24
 * @url https://github.com/DFRobot/DFRobot_RGBLCD1602
 */

#include <stdio.h>
#include <string.h>


#include "RGBLCD1602.h"

const uint8_t color_define[4][3] =
{
    {255, 255, 255},
    {255,   0,   0},
    {  0, 255,   0},
    {  0,   0, 255},
};


/*******************************public*******************************/
void DFRobot_RGBLCD1602(RGBLCD1602_t *lcd,I2C_HandleTypeDef *hi2c,uint8_t lcdAddr, uint8_t rgbAddr,uint8_t lcdCols,uint8_t lcdRows)
{

	lcd->lcdAddr = lcdAddr;
	lcd->rgbAddr = rgbAddr;
	lcd->cols = lcdCols;
	lcd->rows = lcdRows;

	lcd->hi2c=hi2c;
}


/**
 * @fn init
 * @brief initialize the LCD and master IIC
 */
void DFRobot_RGBLCD1602_init(RGBLCD1602_t *lcd)
{

  //_pWire->begin();
  if(lcd->rgbAddr == (0x60)){
	  lcd->REG_RED   =      0x04;
	  lcd->REG_GREEN =      0x03;
	  lcd->REG_BLUE  =      0x02;
	  lcd->REG_ONLY  =      0x02;

  } else if(lcd->rgbAddr == (0x60>>1)){ //30
	  lcd->REG_RED      =   0x06 ;       // pwm2
	  lcd->REG_GREEN    =   0x07 ;       // pwm1
	  lcd->REG_BLUE     =   0x08 ;       // pwm0
	  lcd->REG_ONLY     =   0x08 ;

  } else if(lcd->rgbAddr == (0x6B)){
	  lcd->REG_RED      =   0x06 ;       // pwm2
	  lcd->REG_GREEN    =   0x05 ;       // pwm1
	  lcd->REG_BLUE     =   0x04 ;       // pwm0
    lcd->REG_ONLY     =   0x04 ;

  } else if(lcd->rgbAddr == (0x2D)){
	  lcd->REG_RED      =   0x01 ;       // pwm2
	  lcd->REG_GREEN    =   0x02 ;       // pwm1
	  lcd->REG_BLUE     =   0x03 ;       // pwm0
	  lcd->REG_ONLY     =   0x01 ;
  } else

  {
	  lcd->REG_RED      =   0x00 ;       // pwm2
	  lcd->REG_GREEN    =   0x00 ;       // pwm1
	  lcd->REG_BLUE     =   0x00 ;       // pwm0
	  lcd->REG_ONLY     =   0x00 ;
  }


  lcd->showFunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;

  DFRobot_RGBLCD1602_begin(lcd,lcd->rows, LCD_5x8DOTS );


}

void DFRobot_RGBLCD1602_print(RGBLCD1602_t *lcd, char *str)
{
    while (*str)
    {
        DFRobot_RGBLCD1602_write(lcd, (uint8_t)*str++);
    }
}

void DFRobot_RGBLCD1602_clear(RGBLCD1602_t *lcd)
{
    DFRobot_RGBLCD1602_command(lcd,LCD_CLEARDISPLAY);        // DFRobot_RGBLCD1602_clear DFRobot_RGBLCD1602_display, set cursor position to zero
     HAL_Delay(2);          // this DFRobot_RGBLCD1602_command takes a long time!
}

void DFRobot_RGBLCD1602_home(RGBLCD1602_t *lcd)
{
    DFRobot_RGBLCD1602_command(lcd,LCD_RETURNHOME);        // set cursor position to zero
     HAL_Delay(2);        // this DFRobot_RGBLCD1602_command takes a long time!
}

void DFRobot_RGBLCD1602_noDisplay(RGBLCD1602_t *lcd)
{
    lcd->showControl &= ~LCD_DISPLAYON;
    DFRobot_RGBLCD1602_command(lcd,LCD_DISPLAYCONTROL | lcd->showControl);

}

void DFRobot_RGBLCD1602_display(RGBLCD1602_t *lcd)
{
    lcd->showControl |= LCD_DISPLAYON;
    DFRobot_RGBLCD1602_command(lcd,LCD_DISPLAYCONTROL | lcd->showControl);

}

void DFRobot_RGBLCD1602_stopBlink(RGBLCD1602_t *lcd)
{
    lcd->showControl &= ~LCD_BLINKON;
    DFRobot_RGBLCD1602_command(lcd,LCD_DISPLAYCONTROL | lcd->showControl);

}
void DFRobot_RGBLCD1602_blink(RGBLCD1602_t *lcd)
{
    lcd->showControl |= LCD_BLINKON;
    DFRobot_RGBLCD1602_command(lcd,LCD_DISPLAYCONTROL | lcd->showControl);

}

void DFRobot_RGBLCD1602_noCursor(RGBLCD1602_t *lcd)
{
    lcd->showControl &= ~LCD_CURSORON;
    DFRobot_RGBLCD1602_command(lcd,LCD_DISPLAYCONTROL | lcd->showControl);

}

void DFRobot_RGBLCD1602_cursor(RGBLCD1602_t *lcd)
{
    lcd->showControl |= LCD_CURSORON;
    DFRobot_RGBLCD1602_command(lcd,LCD_DISPLAYCONTROL | lcd->showControl);
    //
}

void DFRobot_RGBLCD1602_scrollDisplayLeft(RGBLCD1602_t *lcd)
{
    DFRobot_RGBLCD1602_command(lcd,LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void DFRobot_RGBLCD1602_scrollDisplayRight(RGBLCD1602_t *lcd)
{
    DFRobot_RGBLCD1602_command(lcd,LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void DFRobot_RGBLCD1602_leftToRight(RGBLCD1602_t *lcd)
{
    lcd->showMode |= LCD_ENTRYLEFT;
    DFRobot_RGBLCD1602_command(lcd,LCD_ENTRYMODESET | lcd->showMode);

}

void DFRobot_RGBLCD1602_rightToLeft(RGBLCD1602_t *lcd)
{
    lcd->showMode &= ~LCD_ENTRYLEFT;
    DFRobot_RGBLCD1602_command(lcd,LCD_ENTRYMODESET | lcd->showMode);

}

void DFRobot_RGBLCD1602_noAutoscroll(RGBLCD1602_t *lcd)
{
    lcd->showMode &= ~LCD_ENTRYSHIFTINCREMENT;
    DFRobot_RGBLCD1602_command(lcd,LCD_ENTRYMODESET | lcd->showMode);

}

void DFRobot_RGBLCD1602_autoscroll(RGBLCD1602_t *lcd)
{
    lcd->showMode |= LCD_ENTRYSHIFTINCREMENT;
    DFRobot_RGBLCD1602_command(lcd,LCD_ENTRYMODESET | lcd->showMode);

}

void DFRobot_RGBLCD1602_customSymbol(RGBLCD1602_t *lcd,uint8_t location, uint8_t charmap[])
{

    location &= 0x7; // we only have 8 locations 0-7
    DFRobot_RGBLCD1602_command(lcd,LCD_SETCGRAMADDR | (location << 3));
    
    uint8_t data[9];
    
    data[0] = 0x40;

    for(int i=0; i<8; i++)
    {
        data[i+1] = charmap[i];
    }
    DFRobot_RGBLCD1602_send(lcd,data, 9);
}

void DFRobot_RGBLCD1602_setCursor(RGBLCD1602_t *lcd,uint8_t col, uint8_t row)
{

    col = (row == 0 ? col|0x80 : col|0xc0);

    uint8_t data[3] = {0x80, col};

    DFRobot_RGBLCD1602_send(lcd,data, 2);

}

void DFRobot_RGBLCD1602_setRGB(RGBLCD1602_t *lcd,uint8_t r, uint8_t g, uint8_t b)
{
  uint16_t temp_r,temp_g,temp_b;
  if(lcd->rgbAddr == 0x60>>1){
    temp_r = (uint16_t)r*192/255;
    temp_g = (uint16_t)g*192/255;
    temp_b = (uint16_t)b*192/255;
    DFRobot_RGBLCD1602_setReg(lcd,lcd->REG_RED, temp_r);
    DFRobot_RGBLCD1602_setReg(lcd,lcd->REG_GREEN, temp_g);
    DFRobot_RGBLCD1602_setReg(lcd,lcd->REG_BLUE, temp_b);

  } else{

    DFRobot_RGBLCD1602_setReg(lcd,lcd->REG_RED, r);
    DFRobot_RGBLCD1602_setReg(lcd,lcd->REG_GREEN, g);
    DFRobot_RGBLCD1602_setReg(lcd,lcd->REG_BLUE, b);

    if(lcd->rgbAddr == 0x6B){
      DFRobot_RGBLCD1602_setReg(lcd,0x07, 0xFF);
    }

  }

}


void DFRobot_RGBLCD1602_setPWM(RGBLCD1602_t *lcd,uint8_t color, uint8_t pwm)
{
	  DFRobot_RGBLCD1602_setReg(lcd,color, pwm);
	  if(lcd->rgbAddr==0x6B)
	  {
		  DFRobot_RGBLCD1602_setReg(lcd,0x07, pwm);
	  }
}


void DFRobot_RGBLCD1602_setColor(RGBLCD1602_t *lcd,uint8_t color)
{
    if(color > 3)
    {
    	return ;//stop
    }
    else
    {
    	DFRobot_RGBLCD1602_setRGB(lcd,color_define[color][0], color_define[color][1], color_define[color][2]);
    }
}


void DFRobot_RGBLCD1602_closeBacklight(RGBLCD1602_t *lcd)
{
	DFRobot_RGBLCD1602_setRGB(lcd,0, 0, 0);
}

void DFRobot_RGBLCD1602_setColorWhite(RGBLCD1602_t *lcd)
{
	DFRobot_RGBLCD1602_setRGB(lcd,255, 255, 255);
}



/*inline*/ size_t DFRobot_RGBLCD1602_write(RGBLCD1602_t *lcd,uint8_t value)
{

    uint8_t data[3] = {0x40, value};

    DFRobot_RGBLCD1602_send(lcd,data, 2);

    return 1; // assume sucess
}

inline void DFRobot_RGBLCD1602_command(RGBLCD1602_t *lcd,uint8_t value)
{
    uint8_t data[3] = {0x80, value};

    DFRobot_RGBLCD1602_send(lcd,data, 2);
}



void DFRobot_RGBLCD1602_setBacklight(RGBLCD1602_t *lcd,bool mode)
{
	if(mode){
		DFRobot_RGBLCD1602_setColorWhite(lcd);		// turn backlight on
	}else{
		DFRobot_RGBLCD1602_closeBacklight(lcd);		// turn backlight off
	}
}


/*******************************private*******************************/
void DFRobot_RGBLCD1602_begin(RGBLCD1602_t *lcd, uint8_t rows, uint8_t charSize)
{


    if (rows > 1) {
    	lcd->showFunction|= LCD_2LINE;
    }

    lcd->numLines = rows;
    lcd->currLine = 0;

    ///< for some 1 line DFRobot_RGBLCD1602_displays you can select a 10 pixel high font
    if ((charSize != 0) && (rows == 1)) {
    	lcd->showFunction |= LCD_5x10DOTS;
    }

    ///< SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    ///< according to datasheet, we need at least 40ms after power rises above 2.7V
    ///< before DFRobot_RGBLCD1602_sending DFRobot_RGBLCD1602_commands. Arduino can turn on way befer 4.5V so we'll wait 50
    HAL_Delay(50);

    ///< this is according to the hitachi HD44780 datasheet
    ///< page 45 figure 23

    ///< DFRobot_RGBLCD1602_send function set DFRobot_RGBLCD1602_command sequence
    DFRobot_RGBLCD1602_command(lcd,LCD_FUNCTIONSET | lcd->showFunction);
    HAL_Delay(5);  // wait more than 4.1ms
	
	///< second try
    DFRobot_RGBLCD1602_command(lcd,LCD_FUNCTIONSET | lcd->showFunction);
    HAL_Delay(5);

    ///< third go
    DFRobot_RGBLCD1602_command(lcd,LCD_FUNCTIONSET | lcd->showFunction);

    ///< turn the DFRobot_RGBLCD1602_display on with no cursor or blinking default
    lcd->showControl = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    DFRobot_RGBLCD1602_display(lcd);

    ///< DFRobot_RGBLCD1602_clear it off
    DFRobot_RGBLCD1602_clear(lcd);

    ///< Initialize to default text direction (for romance languages)
    lcd->showMode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    ///< set the entry mode
    DFRobot_RGBLCD1602_command(lcd,LCD_ENTRYMODESET | lcd->showMode);
    
    if(lcd->rgbAddr == (0xc0>>1)){
      ///< backlight init
      DFRobot_RGBLCD1602_setReg(lcd,REG_MODE1, 0);
      ///< set LEDs controllable by both PWM and GRPPWM registers
      DFRobot_RGBLCD1602_setReg(lcd,REG_OUTPUT, 0xFF);
      ///< set MODE2 values
      ///< 0010 0000 -> 0x20  (DMBLNK to 1, ie blinky mode)
      DFRobot_RGBLCD1602_setReg(lcd,REG_MODE2, 0x20);
    }else if(lcd->rgbAddr == (0x60>>1)){
       DFRobot_RGBLCD1602_setReg(lcd,0x01, 0x00);
       DFRobot_RGBLCD1602_setReg(lcd,0x02, 0xfF);
       DFRobot_RGBLCD1602_setReg(lcd,0x04, 0x15);
    }else if(lcd->rgbAddr==0x6B){
        DFRobot_RGBLCD1602_setReg(lcd,0x2F, 0x00);
        DFRobot_RGBLCD1602_setReg(lcd,0x00, 0x20);
        DFRobot_RGBLCD1602_setReg(lcd,0x01, 0x00);
        DFRobot_RGBLCD1602_setReg(lcd,0x02, 0x01);
        DFRobot_RGBLCD1602_setReg(lcd,0x03, 4);
    }
    DFRobot_RGBLCD1602_setColorWhite(lcd);

}
/*
void DFRobot_RGBLCD1602_send(uint8_t *data, uint8_t len)
{
    _pWire->beginTransmission(lcd->lcdAddr);        // transmit to device #4
    for(int i=0; i<len; i++) {
        _pWire->write(data[i]);
		delayMicroseconds(100);
    }
    _pWire->endTransmission();                     // stop transmitting
}
*/

void DFRobot_RGBLCD1602_send(RGBLCD1602_t *lcd,uint8_t *data,uint8_t len)
{
    HAL_I2C_Master_Transmit(lcd->hi2c, lcd->lcdAddr << 1,data,len,HAL_MAX_DELAY);
}




void DFRobot_RGBLCD1602_setReg(RGBLCD1602_t *lcd,uint8_t addr, uint8_t data)
{

	uint8_t buffer[2];

	    buffer[0] = addr;
	    buffer[1] = data;

	    HAL_I2C_Master_Transmit(lcd->hi2c, lcd->rgbAddr<< 1,buffer,2,HAL_MAX_DELAY);

}

void RGBLCD1602_ECRAN_I2C_Init(RGBLCD1602_t *lcd, I2C_HandleTypeDef *hi2c, uint8_t red, uint8_t green, uint8_t blue)
{
	DFRobot_RGBLCD1602(lcd, hi2c, LCD_ADDRESS, RGBAddr, 16, 2);

	DFRobot_RGBLCD1602_init(lcd);

	HAL_Delay(10);
	DFRobot_RGBLCD1602_clear(lcd);

	DFRobot_RGBLCD1602_setRGB(lcd, red, green, blue);

	HAL_Delay(10);
}

void DFRobot_RGBLCD1602_setRGB_control(RGBLCD1602_t *lcd, uint8_t activation, uint8_t red, uint8_t green, uint8_t blue)
{
	if (activation == 0)
	{
		DFRobot_RGBLCD1602_setRGB(lcd, 0,0,0);
	}
	else
		DFRobot_RGBLCD1602_setRGB(lcd, red, green, blue);
}

void RGBLCD1602_ECRAN_splash_screen(RGBLCD1602_t *lcd)
{
	DFRobot_RGBLCD1602_setCursor(lcd, 0, 0);
	DFRobot_RGBLCD1602_print(lcd, "Nucleo L476RG");

	DFRobot_RGBLCD1602_setCursor(lcd, 0, 1);
	DFRobot_RGBLCD1602_print(lcd, "LCD 1602 RGB");
}

