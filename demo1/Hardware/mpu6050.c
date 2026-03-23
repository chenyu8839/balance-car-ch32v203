#include "mpu6050.h"
#include "debug.h"

/*==========================================================
 * MPU6050专用I2C总线 (PB6=SCL, PB7=SDA)
 * 与OLED的I2C总线(PB0/PB1)完全独立
 *==========================================================*/

/* SCL和SDA电平控制宏 */
#define MPU_SCL_HIGH()  GPIO_SetBits(MPU_SCL_PORT, MPU_SCL_PIN)     /* SCL拉高 */
#define MPU_SCL_LOW()   GPIO_ResetBits(MPU_SCL_PORT, MPU_SCL_PIN)   /* SCL拉低 */
#define MPU_SDA_HIGH()  GPIO_SetBits(MPU_SDA_PORT, MPU_SDA_PIN)     /* SDA拉高 */
#define MPU_SDA_LOW()   GPIO_ResetBits(MPU_SDA_PORT, MPU_SDA_PIN)   /* SDA拉低 */
#define MPU_SDA_READ()  GPIO_ReadInputDataBit(MPU_SDA_PORT, MPU_SDA_PIN) /* 读取SDA电平 */

/* I2C时序延时, 控制SCL频率, 96MHz下约200-400kHz */
static void MPU_I2C_Delay(void)
{
    uint8_t i = 10;   /* 延时计数值, 调整此值可改变I2C速率 */
    while(i--);        /* 空循环延时 */
}

/* 将SDA引脚设为推挽输出模式(用于发送数据) */
static void MPU_SDA_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;           /* GPIO配置结构体 */
    GPIO_InitStructure.GPIO_Pin   = MPU_SDA_PIN;   /* 选择SDA引脚(PB7) */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;  /* 推挽输出模式 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  /* 输出速率50MHz */
    GPIO_Init(MPU_SDA_PORT, &GPIO_InitStructure);  /* 应用配置到GPIOB */
}

/* 将SDA引脚设为浮空输入模式(用于读取ACK和数据) */
static void MPU_SDA_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;           /* GPIO配置结构体 */
    GPIO_InitStructure.GPIO_Pin   = MPU_SDA_PIN;   /* 选择SDA引脚(PB7) */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING; /* 浮空输入模式 */
    GPIO_Init(MPU_SDA_PORT, &GPIO_InitStructure);  /* 应用配置到GPIOB */
}

/* 初始化MPU6050专用I2C的GPIO引脚 */
static void MPU_I2C_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;           /* GPIO配置结构体 */

    /* 使能GPIOB外设时钟 */
    RCC_APB2PeriphClockCmd(MPU_RCC, ENABLE);

    /* SCL引脚(PB6) - 推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = MPU_SCL_PIN;   /* 选择SCL引脚(PB6) */
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;  /* 推挽输出模式 */
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  /* 输出速率50MHz */
    GPIO_Init(MPU_SCL_PORT, &GPIO_InitStructure);  /* 应用配置 */

    /* SDA引脚(PB7) - 推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = MPU_SDA_PIN;   /* 选择SDA引脚(PB7) */
    GPIO_Init(MPU_SDA_PORT, &GPIO_InitStructure);  /* 应用配置 */

    /* 空闲状态: 两根线都拉高 */
    MPU_SCL_HIGH();   /* SCL空闲为高电平 */
    MPU_SDA_HIGH();   /* SDA空闲为高电平 */
}

/* I2C起始信号: SCL为高时, SDA从高变低 */
static void MPU_I2C_Start(void)
{
    MPU_SDA_OUT();     /* SDA切为输出模式 */
    MPU_SDA_HIGH();    /* SDA先拉高 */
    MPU_SCL_HIGH();    /* SCL拉高 */
    MPU_I2C_Delay();   /* 保持一段时间 */
    MPU_SDA_LOW();     /* SCL高电平期间, SDA拉低 = 起始条件 */
    MPU_I2C_Delay();   /* 保持一段时间 */
    MPU_SCL_LOW();     /* 拉低SCL, 准备发送数据 */
}

/* I2C停止信号: SCL为高时, SDA从低变高 */
static void MPU_I2C_Stop(void)
{
    MPU_SDA_OUT();     /* SDA切为输出模式 */
    MPU_SCL_LOW();     /* SCL先拉低 */
    MPU_SDA_LOW();     /* SDA先拉低 */
    MPU_I2C_Delay();   /* 保持一段时间 */
    MPU_SCL_HIGH();    /* SCL拉高 */
    MPU_I2C_Delay();   /* 保持一段时间 */
    MPU_SDA_HIGH();    /* SCL高电平期间, SDA拉高 = 停止条件 */
    MPU_I2C_Delay();   /* 保持一段时间 */
}

/* 等待从机应答ACK, 返回0表示收到ACK, 返回1表示超时无响应 */
static uint8_t MPU_I2C_WaitAck(void)
{
    uint8_t timeout = 0;   /* 超时计数器 */

    MPU_SDA_IN();      /* SDA切为输入, 读取从机应答 */
    MPU_SDA_HIGH();    /* 释放SDA总线(上拉) */
    MPU_I2C_Delay();   /* 等待从机准备好 */
    MPU_SCL_HIGH();    /* SCL拉高, 采样SDA状态 */
    MPU_I2C_Delay();   /* 保持一段时间 */

    while(MPU_SDA_READ())  /* SDA为低=ACK, SDA为高=NACK */
    {
        timeout++;         /* 累加超时计数 */
        if(timeout > 250)  /* 超过250次仍无ACK */
        {
            MPU_I2C_Stop(); /* 发送停止信号, 释放总线 */
            return 1;       /* 返回1表示超时, 从机无响应 */
        }
    }

    MPU_SCL_LOW();     /* SCL拉低, 结束本次应答检测 */
    MPU_SDA_OUT();     /* SDA切回输出模式 */
    return 0;           /* 返回0表示收到ACK应答 */
}

/* 发送一个字节, 高位先发(MSB first) */
static void MPU_I2C_SendByte(uint8_t byte)
{
    uint8_t i;         /* 循环变量, 8位数据 */

    MPU_SDA_OUT();     /* SDA切为输出模式 */
    MPU_SCL_LOW();     /* SCL拉低, 准备放数据 */

    for(i = 0; i < 8; i++)   /* 逐位发送, 从最高位开始 */
    {
        /* 根据当前最高位设置SDA电平 */
        if(byte & 0x80)       /* 判断最高位是否为1 */
            MPU_SDA_HIGH();    /* 最高位为1, SDA拉高 */
        else
            MPU_SDA_LOW();     /* 最高位为0, SDA拉低 */

        byte <<= 1;           /* 左移一位, 准备发送下一位 */
        MPU_I2C_Delay();       /* 数据建立时间 */
        MPU_SCL_HIGH();        /* SCL拉高, 从机在上升沿采样数据 */
        MPU_I2C_Delay();       /* 保持高电平 */
        MPU_SCL_LOW();         /* SCL拉低, 准备下一位数据 */
    }
}

/* 读取一个字节, ack=1发送ACK(继续读), ack=0发送NACK(最后一字节) */
static uint8_t MPU_I2C_ReadByte(uint8_t ack)
{
    uint8_t i, byte = 0;  /* 循环变量和接收缓冲 */

    MPU_SDA_IN();          /* SDA切为输入, 读取从机数据 */

    for(i = 0; i < 8; i++)    /* 逐位读取, 从最高位开始 */
    {
        MPU_SCL_LOW();         /* SCL拉低, 从机准备数据 */
        MPU_I2C_Delay();       /* 等待从机放好数据 */
        MPU_SCL_HIGH();        /* SCL拉高, 读取SDA上的数据位 */
        byte <<= 1;           /* 接收缓冲左移一位 */
        if(MPU_SDA_READ())     /* 读取SDA电平 */
            byte |= 0x01;     /* SDA为高, 该位为1 */
        MPU_I2C_Delay();       /* 保持高电平 */
    }

    MPU_SCL_LOW();         /* SCL拉低 */
    MPU_SDA_OUT();         /* SDA切回输出, 准备发送应答 */

    /* 发送应答信号 */
    if(ack)
        MPU_SDA_LOW();     /* ACK: SDA拉低, 告诉从机继续发 */
    else
        MPU_SDA_HIGH();    /* NACK: SDA保持高, 告诉从机停止 */

    MPU_I2C_Delay();       /* 应答建立时间 */
    MPU_SCL_HIGH();        /* SCL拉高, 从机采样应答信号 */
    MPU_I2C_Delay();       /* 保持高电平 */
    MPU_SCL_LOW();         /* SCL拉低, 完成应答 */

    return byte;           /* 返回读取到的字节 */
}

/*==========================================================
 * MPU6050寄存器读写函数
 *==========================================================*/

/* 向MPU6050指定寄存器写入一个字节 */
static void MPU_WriteReg(uint8_t reg, uint8_t data)
{
    MPU_I2C_Start();                       /* 发送起始信号 */
    MPU_I2C_SendByte(MPU6050_ADDR);        /* 发送设备地址 + 写标志(0) */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */
    MPU_I2C_SendByte(reg);                 /* 发送寄存器地址 */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */
    MPU_I2C_SendByte(data);                /* 发送要写入的数据 */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */
    MPU_I2C_Stop();                        /* 发送停止信号 */
}

/* 从MPU6050指定寄存器读取一个字节 */
static uint8_t MPU_ReadReg(uint8_t reg)
{
    uint8_t data;                          /* 存储读取到的数据 */

    MPU_I2C_Start();                       /* 发送起始信号 */
    MPU_I2C_SendByte(MPU6050_ADDR);        /* 发送设备地址 + 写标志(0) */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */
    MPU_I2C_SendByte(reg);                 /* 发送要读取的寄存器地址 */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */

    MPU_I2C_Start();                       /* 重复起始信号(不释放总线) */
    MPU_I2C_SendByte(MPU6050_ADDR | 0x01); /* 发送设备地址 + 读标志(1) */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */
    data = MPU_I2C_ReadByte(0);            /* 读取一字节, NACK(最后一字节) */
    MPU_I2C_Stop();                        /* 发送停止信号 */

    return data;                           /* 返回读取到的数据 */
}

/* 从MPU6050连续读取多个字节(突发读取) */
static void MPU_ReadBytes(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;                             /* 循环变量 */

    MPU_I2C_Start();                       /* 发送起始信号 */
    MPU_I2C_SendByte(MPU6050_ADDR);        /* 发送设备地址 + 写标志(0) */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */
    MPU_I2C_SendByte(reg);                 /* 发送起始寄存器地址 */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */

    MPU_I2C_Start();                       /* 重复起始信号 */
    MPU_I2C_SendByte(MPU6050_ADDR | 0x01); /* 发送设备地址 + 读标志(1) */
    MPU_I2C_WaitAck();                     /* 等待从机应答 */

    for(i = 0; i < len; i++)               /* 连续读取len个字节 */
    {
        if(i == len - 1)
            buf[i] = MPU_I2C_ReadByte(0);  /* 最后一字节: 发送NACK */
        else
            buf[i] = MPU_I2C_ReadByte(1);  /* 非最后字节: 发送ACK继续读 */
    }

    MPU_I2C_Stop();                        /* 发送停止信号 */
}

/*==========================================================
 * MPU6050公开API函数
 *==========================================================*/

/**
 * 读取MPU6050设备ID(WHO_AM_I寄存器)
 * 正常连接时应返回0x68
 */
uint8_t MPU6050_GetID(void)
{
    return MPU_ReadReg(MPU6050_WHO_AM_I);  /* 读取0x75寄存器 */
}

/**
 * 初始化MPU6050传感器
 * 返回值: 0=成功, 1=设备未找到
 *
 * 配置参数:
 * - 陀螺仪量程:   +/- 2000 度/秒  (灵敏度: 16.4 LSB/度/秒)
 * - 加速度计量程: +/- 2g          (灵敏度: 16384 LSB/g)
 * - 采样率:       100Hz (1kHz / (1+9))
 * - 低通滤波器:   带宽42Hz, 延迟4.8ms
 */
uint8_t MPU6050_Init(void)
{
    uint8_t id;                            /* 设备ID存储变量 */

    /* 初始化MPU6050专用I2C的GPIO引脚 */
    MPU_I2C_Init();

    /* 等待MPU6050上电稳定 */
    Delay_Ms(100);

    /* 唤醒MPU6050: 清除SLEEP位, 选择X轴陀螺仪作为时钟源(更稳定) */
    MPU_WriteReg(MPU6050_PWR_MGMT_1, 0x01);
    Delay_Ms(50);                          /* 等待时钟切换稳定 */

    /* 验证设备ID, 确认MPU6050已正确连接 */
    id = MPU6050_GetID();
    if(id != 0x68)                         /* WHO_AM_I应该返回0x68 */
    {
        printf("MPU6050 NOT found! ID=0x%02X\r\n", id);  /* 串口打印错误信息 */
        return 1;                          /* 返回1表示初始化失败 */
    }

    /* 设置采样率: 采样率 = 1kHz / (1 + SMPLRT_DIV) = 1000/(1+9) = 100Hz */
    MPU_WriteReg(MPU6050_SMPLRT_DIV, 0x09);

    /* 设置数字低通滤波器(DLPF): 带宽42Hz, 延迟4.8ms, 内部采样1kHz */
    MPU_WriteReg(MPU6050_CONFIG, 0x03);

    /* 设置陀螺仪量程: FS_SEL=3, 即 +/- 2000度/秒 */
    MPU_WriteReg(MPU6050_GYRO_CONFIG, 0x18);

    /* 设置加速度计量程: AFS_SEL=0, 即 +/- 2g */
    MPU_WriteReg(MPU6050_ACCEL_CONFIG, 0x00);

    printf("MPU6050 Init OK! ID=0x%02X\r\n", id);  /* 串口打印成功信息 */
    return 0;                              /* 返回0表示初始化成功 */
}

/**
 * 一次性读取全部传感器数据(加速度+陀螺仪+温度)
 * 突发读取14字节, 比逐个读取更快且数据一致性更好
 */
void MPU6050_ReadAll(MPU6050_Data_t *data)
{
    uint8_t buf[14];  /* 14字节缓冲: 加速度XYZ(6) + 温度(2) + 陀螺仪XYZ(6) */

    /* 从ACCEL_XOUT_H(0x3B)开始连续读取14个字节 */
    MPU_ReadBytes(MPU6050_ACCEL_XOUT_H, buf, 14);

    /* 合并高低字节(MPU6050为大端序: 高字节在前) */
    data->accel_x  = (int16_t)((buf[0]  << 8) | buf[1]);   /* 加速度X轴 */
    data->accel_y  = (int16_t)((buf[2]  << 8) | buf[3]);   /* 加速度Y轴 */
    data->accel_z  = (int16_t)((buf[4]  << 8) | buf[5]);   /* 加速度Z轴 */
    data->temp_raw = (int16_t)((buf[6]  << 8) | buf[7]);   /* 温度原始值 */
    data->gyro_x   = (int16_t)((buf[8]  << 8) | buf[9]);   /* 陀螺仪X轴 */
    data->gyro_y   = (int16_t)((buf[10] << 8) | buf[11]);  /* 陀螺仪Y轴 */
    data->gyro_z   = (int16_t)((buf[12] << 8) | buf[13]);  /* 陀螺仪Z轴 */

    /* 温度换算公式: 温度(摄氏度) = 原始值 / 340 + 36.53 */
    data->temp_deg = (float)data->temp_raw / 340.0f + 36.53f;
}

/**
 * 单独读取加速度计数据(3轴)
 */
void MPU6050_ReadAccel(int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];   /* 6字节缓冲: 加速度XYZ各2字节 */

    /* 从ACCEL_XOUT_H(0x3B)开始连续读取6个字节 */
    MPU_ReadBytes(MPU6050_ACCEL_XOUT_H, buf, 6);

    /* 合并高低字节 */
    *ax = (int16_t)((buf[0] << 8) | buf[1]);  /* 加速度X轴 */
    *ay = (int16_t)((buf[2] << 8) | buf[3]);  /* 加速度Y轴 */
    *az = (int16_t)((buf[4] << 8) | buf[5]);  /* 加速度Z轴 */
}

/**
 * 单独读取陀螺仪数据(3轴)
 */
void MPU6050_ReadGyro(int16_t *gx, int16_t *gy, int16_t *gz)
{
    uint8_t buf[6];   /* 6字节缓冲: 陀螺仪XYZ各2字节 */

    /* 从GYRO_XOUT_H(0x43)开始连续读取6个字节 */
    MPU_ReadBytes(MPU6050_GYRO_XOUT_H, buf, 6);

    /* 合并高低字节 */
    *gx = (int16_t)((buf[0] << 8) | buf[1]);  /* 陀螺仪X轴 */
    *gy = (int16_t)((buf[2] << 8) | buf[3]);  /* 陀螺仪Y轴 */
    *gz = (int16_t)((buf[4] << 8) | buf[5]);  /* 陀螺仪Z轴 */
}
