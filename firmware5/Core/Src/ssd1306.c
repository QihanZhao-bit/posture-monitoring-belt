/*
 * ssd1306.c
 *
 *  Created on: Nov 20, 2025
 *      Author: QIHAN ZHAO
 */


#include "ssd1306.h"
#include "fonts.h"

extern I2C_HandleTypeDef hi2c1;

static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];
static uint8_t CurrentX = 0;
static uint8_t CurrentY = 0;

static void ssd1306_WriteCommand(uint8_t cmd)
{
    HAL_I2C_Mem_Write(&hi2c1,
                      SSD1306_I2C_ADDR,
                      0x00,
                      I2C_MEMADD_SIZE_8BIT,
                      &cmd, 1, 10);
}

void ssd1306_Init(void)
{
    HAL_Delay(100);

    ssd1306_WriteCommand(0xAE);
    ssd1306_WriteCommand(0x20); // Memory addr mode
    ssd1306_WriteCommand(0x10);
    ssd1306_WriteCommand(0xB0);
    ssd1306_WriteCommand(0xC8);
    ssd1306_WriteCommand(0x00);
    ssd1306_WriteCommand(0x10);
    ssd1306_WriteCommand(0x40);
    ssd1306_WriteCommand(0x81);
    ssd1306_WriteCommand(0xFF);
    ssd1306_WriteCommand(0xA1);
    ssd1306_WriteCommand(0xA6);
    ssd1306_WriteCommand(0xA8);
    ssd1306_WriteCommand(0x3F);
    ssd1306_WriteCommand(0xA4);
    ssd1306_WriteCommand(0xD3);
    ssd1306_WriteCommand(0x00);
    ssd1306_WriteCommand(0xD5);
    ssd1306_WriteCommand(0xF0);
    ssd1306_WriteCommand(0xD9);
    ssd1306_WriteCommand(0x22);
    ssd1306_WriteCommand(0xDA);
    ssd1306_WriteCommand(0x12);
    ssd1306_WriteCommand(0xDB);
    ssd1306_WriteCommand(0x20);
    ssd1306_WriteCommand(0x8D);
    ssd1306_WriteCommand(0x14);
    ssd1306_WriteCommand(0xAF);

    ssd1306_Fill(Black);
    ssd1306_UpdateScreen();
}

void ssd1306_Fill(SSD1306_COLOR color)
{
    memset(SSD1306_Buffer, (color == Black) ? 0x00 : 0xFF,
           sizeof(SSD1306_Buffer));
}

void ssd1306_UpdateScreen(void)
{
    for (uint8_t page = 0; page < 8; page++)
    {
        ssd1306_WriteCommand(0xB0 + page);
        ssd1306_WriteCommand(0x00);
        ssd1306_WriteCommand(0x10);

        HAL_I2C_Mem_Write(&hi2c1,
                          SSD1306_I2C_ADDR,
                          0x40,
                          I2C_MEMADD_SIZE_8BIT,
                          &SSD1306_Buffer[SSD1306_WIDTH * page],
                          SSD1306_WIDTH,
                          100);
    }
}

void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;

    if (color == White)
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |=  (1 << (y % 8));
    else
        SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
}

void ssd1306_SetCursor(uint8_t x, uint8_t y)
{
    CurrentX = x;
    CurrentY = y;
}

void ssd1306_WriteChar(char ch)
{
    for (uint8_t i = 0; i < 5; i++)
    {
        uint8_t line = Font5x7[ch - 32][i];
        for (uint8_t j = 0; j < 7; j++)
        {
            if (line & (1 << j))
                ssd1306_DrawPixel(CurrentX + i, CurrentY + j, White);
            else
                ssd1306_DrawPixel(CurrentX + i, CurrentY + j, Black);
        }
    }
    CurrentX += 6;
}

void ssd1306_WriteString(const char* str)
{
    while (*str)
    {
        ssd1306_WriteChar(*str++);
    }
}
