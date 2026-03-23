#ifndef __MPU6050_H
#define __MPU6050_H

#include "ch32v20x.h"

/* MPU6050的I2C地址 (AD0接低: 0x68, 左移1位: 0xD0) */
#define MPU6050_ADDR        0xD0

/* MPU6050寄存器地址定义 */
#define MPU6050_SMPLRT_DIV  0x19  /* 采样率分频器 */
#define MPU6050_CONFIG       0x1A  /* 低通滤波器配置 */
#define MPU6050_GYRO_CONFIG  0x1B  /* 陀螺仪量程配置 */
#define MPU6050_ACCEL_CONFIG 0x1C  /* 加速度计量程配置 */
#define MPU6050_ACCEL_XOUT_H 0x3B  /* 加速度X轴高字节 */
#define MPU6050_TEMP_OUT_H   0x41  /* 温度高字节 */
#define MPU6050_GYRO_XOUT_H  0x43  /* 陀螺仪X轴高字节 */
#define MPU6050_PWR_MGMT_1   0x6B  /* 电源管理寄存器1 */
#define MPU6050_WHO_AM_I     0x75  /* 设备ID寄存器 */

/* MPU6050专用I2C引脚定义 (PB6=SCL, PB7=SDA) */
#define MPU_SCL_PORT    GPIOB
#define MPU_SCL_PIN     GPIO_Pin_6
#define MPU_SDA_PORT    GPIOB
#define MPU_SDA_PIN     GPIO_Pin_7
#define MPU_RCC         RCC_APB2Periph_GPIOB

/* MPU6050原始数据结构体 */
typedef struct {
    int16_t accel_x;    /* 加速度计X轴原始值 */
    int16_t accel_y;    /* 加速度计Y轴原始值 */
    int16_t accel_z;    /* 加速度计Z轴原始值 */
    int16_t gyro_x;     /* 陀螺仪X轴原始值 */
    int16_t gyro_y;     /* 陀螺仪Y轴原始值 */
    int16_t gyro_z;     /* 陀螺仪Z轴原始值 */
    int16_t temp_raw;   /* 温度原始值 */
    float   temp_deg;   /* 温度(摄氏度) */
} MPU6050_Data_t;

uint8_t MPU6050_Init(void);     /* 初始化MPU6050, 返回0=成功, 1=失败 */
uint8_t MPU6050_GetID(void);    /* 读取设备ID(正常应返回0x68) */

/* 一次性读取全部传感器数据(加速度+陀螺仪+温度) */
void MPU6050_ReadAll(MPU6050_Data_t *data);

/* 单独读取加速度计数据 */
void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az);

/* 单独读取陀螺仪数据 */
void MPU6050_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz);

#endif
