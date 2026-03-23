#include "oled.h"
#include "i2c_soft.h"
#include "oledfont.h"
#include "debug.h"
#include <string.h>
#include <stdio.h>

/* 向OLED发送一个命令字节, 控制字节0x00表示命令模式 */
static void OLED_WriteCmd(uint8_t cmd)
{
    I2C_Soft_Start();
    I2C_Soft_SendByte(OLED_ADDR);   /* 设备地址 + 写 */
    I2C_Soft_WaitAck();
    I2C_Soft_SendByte(0x00);         /* 控制字节: 后续是命令 */
    I2C_Soft_WaitAck();
    I2C_Soft_SendByte(cmd);          /* 命令数据 */
    I2C_Soft_WaitAck();
    I2C_Soft_Stop();
}

/* 向OLED发送一个数据字节, 控制字节0x40表示数据模式 */
static void OLED_WriteData(uint8_t data)
{
    I2C_Soft_Start();
    I2C_Soft_SendByte(OLED_ADDR);   /* 设备地址 + 写 */
    I2C_Soft_WaitAck();
    I2C_Soft_SendByte(0x40);         /* 控制字节: 后续是显示数据 */
    I2C_Soft_WaitAck();
    I2C_Soft_SendByte(data);         /* 显示数据 */
    I2C_Soft_WaitAck();
    I2C_Soft_Stop();
}

/* 设置OLED的显示位置, x=列(0-127), y=页(0-7, 每页8像素高) */
void OLED_SetPos(uint8_t x, uint8_t y)
{
    OLED_WriteCmd(0xB0 + y);                  /* 设置页地址 */
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10);  /* 设置列地址高4位 */
    OLED_WriteCmd(x & 0x0F);                  /* 设置列地址低4位 */
}

/* 清屏: 向所有显存写入0x00 */
void OLED_Clear(void)
{
    uint8_t i, j;

    for(i = 0; i < 8; i++)           /* 遍历8个页 */
    {
        OLED_WriteCmd(0xB0 + i);      /* 设置当前页 */
        OLED_WriteCmd(0x00);          /* 列地址低位=0 */
        OLED_WriteCmd(0x10);          /* 列地址高位=0 */
        for(j = 0; j < 128; j++)     /* 遍历128列 */
        {
            OLED_WriteData(0x00);     /* 写0清除 */
        }
    }
}

/* 初始化OLED, 兼容SSD1306和SSD1315 */
void OLED_Init(void)
{
    /* 先初始化I2C总线 */
    I2C_Soft_Init();

    /* 等待OLED电源稳定 */
    Delay_Ms(100);

    OLED_WriteCmd(0xAE);  /* 关闭显示 */
    OLED_WriteCmd(0x20);  /* 设置内存寻址模式 */
    OLED_WriteCmd(0x02);  /* 页寻址模式 */
    OLED_WriteCmd(0xB0);  /* 设置起始页地址(第0页) */
    OLED_WriteCmd(0xC8);  /* COM扫描方向: 从下到上(翻转显示) */
    OLED_WriteCmd(0x00);  /* 设置列地址低位 */
    OLED_WriteCmd(0x10);  /* 设置列地址高位 */
    OLED_WriteCmd(0x40);  /* 设置显示起始行 = 0 */
    OLED_WriteCmd(0x81);  /* 设置对比度(亮度) */
    OLED_WriteCmd(0xCF);  /* 对比度值(0x00-0xFF, 越大越亮) */
    OLED_WriteCmd(0xA1);  /* 段重映射: 左右翻转 */
    OLED_WriteCmd(0xA6);  /* 正常显示(非反色) */
    OLED_WriteCmd(0xA8);  /* 设置多路复用比 */
    OLED_WriteCmd(0x3F);  /* 1/64占空比(64行) */
    OLED_WriteCmd(0xA4);  /* 显示跟随RAM内容 */
    OLED_WriteCmd(0xD3);  /* 设置显示偏移 */
    OLED_WriteCmd(0x00);  /* 无偏移 */
    OLED_WriteCmd(0xD5);  /* 设置显示时钟分频比 */
    OLED_WriteCmd(0x80);  /* 默认分频比 */
    OLED_WriteCmd(0xD9);  /* 设置预充电周期 */
    OLED_WriteCmd(0xF1);  /* 阶段1=15, 阶段2=1 */
    OLED_WriteCmd(0xDA);  /* 设置COM引脚硬件配置 */
    OLED_WriteCmd(0x12);  /* 交替COM引脚配置 */
    OLED_WriteCmd(0xDB);  /* 设置VCOMH反压级别 */
    OLED_WriteCmd(0x40);  /* 约0.77倍Vcc */
    OLED_WriteCmd(0x8D);  /* 电荷泵使能 */
    OLED_WriteCmd(0x14);  /* 开启电荷泵 */
    OLED_WriteCmd(0xAF);  /* 打开显示 */

    OLED_Clear();          /* 清屏 */
}

/* 在(x,y)位置显示一个ASCII字符, size=16大字(8x16), size=12小字(6x8) */
void OLED_ShowChar(uint8_t x, uint8_t y, char ch, uint8_t size)
{
    uint8_t i;
    uint8_t c = ch - ' ';  /* 字库从空格(0x20)开始, 计算偏移 */

    /* 超出屏幕宽度则换行 */
    if(x > OLED_WIDTH - 1)
    {
        x = 0;
        y += (size == 16) ? 2 : 1;
    }

    if(size == 16)
    {
        /* 8x16大字体: 上半部分(8字节) */
        OLED_SetPos(x, y);
        for(i = 0; i < 8; i++)
            OLED_WriteData(F8x16[c][i]);

        /* 8x16大字体: 下半部分(8字节) */
        OLED_SetPos(x, y + 1);
        for(i = 0; i < 8; i++)
            OLED_WriteData(F8x16[c][i + 8]);
    }
    else  /* size == 12, 6x8小字体 */
    {
        OLED_SetPos(x, y);
        for(i = 0; i < 6; i++)
            OLED_WriteData(F6x8[c][i]);
    }
}

/* 在(x,y)位置显示字符串, 自动换行 */
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size)
{
    uint8_t char_width = (size == 16) ? 8 : 6;  /* 字符宽度(像素) */

    while(*str != '\0')
    {
        OLED_ShowChar(x, y, *str, size);
        x += char_width;
        if(x > OLED_WIDTH - char_width)  /* 超出屏幕宽度, 换行 */
        {
            x = 0;
            y += (size == 16) ? 2 : 1;
        }
        str++;
    }
}

/* 在(x,y)位置显示有符号整数 */
void OLED_ShowNum(uint8_t x, uint8_t y, int32_t num, uint8_t size)
{
    char buf[12];
    sprintf(buf, "%ld", (long)num);
    OLED_ShowString(x, y, buf, size);
}

/* 在(x,y)位置显示浮点数, 保留2位小数 */
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t size)
{
    char buf[16];
    int32_t integer = (int32_t)num;             /* 整数部分 */
    int32_t decimal = (int32_t)((num - integer) * 100);  /* 小数部分(2位) */

    if(decimal < 0) decimal = -decimal;         /* 小数部分取绝对值 */
    if(num < 0 && integer == 0)
        sprintf(buf, "-%ld.%02ld", (long)integer, (long)decimal);
    else
        sprintf(buf, "%ld.%02ld", (long)integer, (long)decimal);

    OLED_ShowString(x, y, buf, size);
}
