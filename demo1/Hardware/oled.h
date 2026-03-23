#ifndef __OLED_H
#define __OLED_H

#include "ch32v20x.h"

/* OLED的I2C从机地址 (SSD1306/SSD1315, 地址0x3C左移一位 = 0x78) */
#define OLED_ADDR   0x78

/* OLED屏幕分辨率 */
#define OLED_WIDTH  128
#define OLED_HEIGHT 64

void OLED_Init(void);       /* 初始化OLED显示屏 */
void OLED_Clear(void);      /* 清除整个屏幕 */
void OLED_Refresh(void);    /* 刷新屏幕(当前未使用) */

/* 设置光标位置, x=列(0-127), y=页(0-7) */
void OLED_SetPos(uint8_t x, uint8_t y);

/* 在(x,y)位置显示一个字符, size=16表示8x16大字, size=12表示6x8小字 */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size);

/* 在(x,y)位置显示字符串 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);

/* 在(x,y)位置显示有符号整数 */
void OLED_ShowNum(uint8_t x, uint8_t y, int32_t num, uint8_t size);

/* 在(x,y)位置显示浮点数(保留2位小数) */
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t size);

#endif
