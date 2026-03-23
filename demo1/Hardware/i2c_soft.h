#ifndef __I2C_SOFT_H
#define __I2C_SOFT_H

#include "ch32v20x.h"

/* OLED专用软件I2C引脚定义: PB0=SCL, PB1=SDA */

#define I2C_SCL_PORT    GPIOB
#define I2C_SCL_PIN     GPIO_Pin_0
#define I2C_SDA_PORT    GPIOB
#define I2C_SDA_PIN     GPIO_Pin_1
#define I2C_RCC         RCC_APB2Periph_GPIOB

/* SCL和SDA电平控制宏 */
#define I2C_SCL_HIGH()  GPIO_SetBits(I2C_SCL_PORT, I2C_SCL_PIN)
#define I2C_SCL_LOW()   GPIO_ResetBits(I2C_SCL_PORT, I2C_SCL_PIN)
#define I2C_SDA_HIGH()  GPIO_SetBits(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_SDA_LOW()   GPIO_ResetBits(I2C_SDA_PORT, I2C_SDA_PIN)
#define I2C_SDA_READ()  GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN)

void I2C_Soft_Init(void);              /* 初始化I2C的GPIO引脚 */
void I2C_Soft_Start(void);             /* 发送I2C起始信号 */
void I2C_Soft_Stop(void);              /* 发送I2C停止信号 */
uint8_t I2C_Soft_WaitAck(void);        /* 等待从机应答, 返回0=收到ACK, 1=超时NACK */
void I2C_Soft_SendByte(uint8_t byte);  /* 发送一个字节, 高位先发 */
uint8_t I2C_Soft_ReadByte(uint8_t ack);/* 读取一个字节, ack=1发送ACK继续读 */

/* 向I2C设备的寄存器写入一个字节 */
void I2C_Soft_WriteReg(uint8_t addr, uint8_t reg, uint8_t data);

#endif
