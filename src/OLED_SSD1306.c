#include "OLED_SSD1306.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//INSERT HERE MCU STD LIB HEADER FILE
#include <stm32l4xx_hal.h>

uint8_t invertColors = 0;
uint16_t width = 0;
uint16_t height = 0;
uint8_t* frame = NULL;
uint8_t oled_address = 0;
I2C_HandleTypeDef* oled_hi2c;

HAL_StatusTypeDef OLED_1306_DrawRectangle(uint8_t sx, uint8_t sy, uint8_t ex, uint8_t ey, uint8_t color){
	HAL_StatusTypeDef status = HAL_OK;

	status |= OLED_1306_DrawHorLine(sx, sy, ex, color); //top
	status |= OLED_1306_DrawHorLine(sx, ey, ex, color); //bottom

	status |= OLED_1306_DrawVerLine(sx, sy, ey, color); //left
	status |= OLED_1306_DrawVerLine(ex, sy, ey+1, color); //right

	return status;
}

HAL_StatusTypeDef OLED_1306_DrawHorLine(uint8_t sx, uint8_t sy, uint8_t ex, uint8_t color){
	HAL_StatusTypeDef status = HAL_OK;

	for(uint32_t i = sx;i<ex;i++){
		status |= OLED_1306_DrawPixel(i, sy, color);
	}

	return status;
}

HAL_StatusTypeDef OLED_1306_DrawVerLine(uint8_t sx, uint8_t sy, uint8_t ey, uint8_t color){
	HAL_StatusTypeDef status = HAL_OK;

	for(uint32_t i = sy;i<ey;i++){
		status |= OLED_1306_DrawPixel(sx, i, color);
	}

	return status;
}

HAL_StatusTypeDef OLED_1306_FillScreen(uint8_t color){
	if(frame == NULL) return HAL_ERROR;

	memset(frame, color == BLACK ? 0 : 255, width*((height + 7) / 8));

	return HAL_OK;
}

HAL_StatusTypeDef OLED_1306_Init(I2C_HandleTypeDef* hi2c, uint16_t addr, uint16_t w, uint8_t h, uint8_t inv){
	HAL_StatusTypeDef status = HAL_OK;

	inv = invertColors;
	oled_hi2c = hi2c;
	oled_address = addr<<1;
	width = w;
	height = h;

	frame = (uint8_t*)malloc(width*((height + 7) / 8));

	if (frame == NULL) return HAL_ERROR;

	status |= OLED_1306_FillScreen(BLACK);

	/*Init Procedure*/
	status |= OLED_1306_SendCmd(OLED_SSD1306_DISPLAYOFF);

	status |= OLED_1306_SendCmd(OLED_SSD1306_MEMORYMODE);
	status |= OLED_1306_SendCmd(0x0);
	status |= OLED_1306_SendCmd(0xB0);
	status |= OLED_1306_SendCmd(OLED_SSD1306_COMSCANDEC);
	status |= OLED_1306_SendCmd(OLED_SSD1306_SEGREMAP);

	status |= OLED_1306_SendCmd(OLED_SSD1306_SETLOWCOLUMN);
	status |= OLED_1306_SendCmd(OLED_SSD1306_SETHIGHCOLUMN);
	status |= OLED_1306_SendCmd(OLED_SSD1306_SETSTARTLINE);

	status |= OLED_1306_SendCmd(OLED_SSD1306_SETCONTRAST);
	status |= OLED_1306_SendCmd(0xFF); //contrast

	if(invertColors)
		status |= OLED_1306_SendCmd(OLED_SSD1306_INVERTDISPLAY);
	else
		status |= OLED_1306_SendCmd(OLED_SSD1306_NORMALDISPLAY);

	status |= OLED_1306_SendCmd(OLED_SSD1306_SETMULTIPLEX);
	if(height == 128 || height == 64)
		status |= OLED_1306_SendCmd(0x3F);
	else
		status |= OLED_1306_SendCmd(0x1F);

	//status |= OLED_1306_SendCmd(OLED_SSD1306_DISPLAYALLON);
	status |= OLED_1306_SendCmd(OLED_SSD1306_DISPLAYALLON_RESUME);

	status |= OLED_1306_SendCmd(OLED_SSD1306_SETCOMPINS);
	if(height == 128 || height == 64)
		status |= OLED_1306_SendCmd(0x12);
	else
		status |= OLED_1306_SendCmd(0x02);


	status |= OLED_1306_SendCmd(OLED_SSD1306_SETDISPLAYOFFSET);
	status |= OLED_1306_SendCmd(0x0);

	status |= OLED_1306_SendCmd(OLED_SSD1306_SETDISPLAYCLOCKDIV);
	status |= OLED_1306_SendCmd(0xF0); //clock div

	status |= OLED_1306_SendCmd(OLED_SSD1306_SETPRECHARGE);
	status |= OLED_1306_SendCmd(0x22);

	status |= OLED_1306_SendCmd(OLED_SSD1306_SETVCOMDETECT);
	status |= OLED_1306_SendCmd(0x20);

	status |= OLED_1306_SendCmd(OLED_SSD1306_CHARGEPUMP);
	status |= OLED_1306_SendCmd(0x14);

	status |= OLED_1306_SendCmd(OLED_SSD1306_DISPLAYON);

	status |= OLED_1306_Display();

	if(status == HAL_OK) return status;

	OLED_1306_Deinit();
	return status;
}

HAL_StatusTypeDef OLED_1306_Deinit(){
	if(frame == NULL)
		return HAL_ERROR;

	free(frame);

	return HAL_OK;
}

HAL_StatusTypeDef OLED_1306_DrawPixel(int16_t x, int16_t y, uint16_t color){
	if ((x >= 0) && (x < width) && (y >= 0) && (y < height)){
		if(color == WHITE){
			frame[x + (y / 8) * width] |= (1 << (y & 7));
		}
		else if(color == BLACK){
			frame[x + (y / 8) * width] &= ~(1 << (y & 7));
		}
		else if(color == INVERSE){
			frame[x + (y / 8) * width] ^= (1 << (y & 7));
		}
		else{
			return HAL_ERROR;
		}
		return HAL_OK;
	}
	return HAL_ERROR;
}

HAL_StatusTypeDef OLED_1306_Display(){
	HAL_StatusTypeDef status = HAL_OK;
	for(uint8_t i = 0; i < height/8; i++) {
		status |= OLED_1306_SendCmd(0xB0+i);
		status |= OLED_1306_SendCmd(0x00);
		status |= OLED_1306_SendCmd(0x10);
		status |= HAL_I2C_Mem_Write(oled_hi2c, oled_address, 0x40,1, &(frame[width*i]), width, HAL_MAX_DELAY);
	}

	return status;
}

HAL_StatusTypeDef OLED_1306_SendCmd(uint8_t cmd){
	HAL_StatusTypeDef status = HAL_OK;

	status = HAL_I2C_Mem_Write(oled_hi2c, oled_address,0x0, 1, &cmd, 1, HAL_MAX_DELAY);

	return status;
}


