#ifndef __SSD1306_H__
#define __SSD1306_H__

#include "stm32l4xx_hal.h"
#include <stdint.h>
#include <string.h>

#define SSD1306_I2C_ADDR      (0x3C << 1)   // OLED I2C address
#define SSD1306_WIDTH         128
#define SSD1306_HEIGHT        64

typedef enum {
    Black = 0x00,
    White = 0x01
} SSD1306_COLOR;

// Public functions
void ssd1306_Init(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);

void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);
void ssd1306_SetCursor(uint8_t x, uint8_t y);
void ssd1306_WriteChar(char ch);
void ssd1306_WriteString(const char* str);

#endif
