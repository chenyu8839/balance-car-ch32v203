#include "i2c_soft.h"
#include "debug.h"

/* I2C时序延时, 控制SCL频率, 96MHz下约200-400kHz */
static void I2C_Delay(void)
{
    uint8_t i = 10;
    while(i--);
}

/* 将SDA引脚设为推挽输出模式(用于发送数据) */
static void I2C_SDA_OUT(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);
}

/* 将SDA引脚设为浮空输入模式(用于读取ACK和数据) */
static void I2C_SDA_IN(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = I2C_SDA_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);
}

/* 初始化I2C的GPIO引脚, SCL和SDA都设为推挽输出, 空闲状态拉高 */
void I2C_Soft_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* 使能GPIOB时钟 */
    RCC_APB2PeriphClockCmd(I2C_RCC, ENABLE);

    /* SCL引脚 - 推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = I2C_SCL_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(I2C_SCL_PORT, &GPIO_InitStructure);

    /* SDA引脚 - 推挽输出 */
    GPIO_InitStructure.GPIO_Pin   = I2C_SDA_PIN;
    GPIO_Init(I2C_SDA_PORT, &GPIO_InitStructure);

    /* 空闲状态: 两根线都拉高 */
    I2C_SCL_HIGH();
    I2C_SDA_HIGH();
}

/* I2C起始信号: SCL为高时, SDA从高变低 */
void I2C_Soft_Start(void)
{
    I2C_SDA_OUT();
    I2C_SDA_HIGH();
    I2C_SCL_HIGH();
    I2C_Delay();
    I2C_SDA_LOW();    /* SCL高电平期间, SDA拉低 = 起始条件 */
    I2C_Delay();
    I2C_SCL_LOW();    /* 拉低SCL, 准备发送数据 */
}

/* I2C停止信号: SCL为高时, SDA从低变高 */
void I2C_Soft_Stop(void)
{
    I2C_SDA_OUT();
    I2C_SCL_LOW();
    I2C_SDA_LOW();
    I2C_Delay();
    I2C_SCL_HIGH();
    I2C_Delay();
    I2C_SDA_HIGH();   /* SCL高电平期间, SDA拉高 = 停止条件 */
    I2C_Delay();
}

/* 等待从机应答ACK, 返回0表示收到ACK, 返回1表示超时无响应 */
uint8_t I2C_Soft_WaitAck(void)
{
    uint8_t timeout = 0;

    I2C_SDA_IN();     /* SDA切换为输入, 读取从机应答 */
    I2C_SDA_HIGH();
    I2C_Delay();
    I2C_SCL_HIGH();
    I2C_Delay();

    while(I2C_SDA_READ())
    {
        timeout++;
        if(timeout > 250)
        {
            I2C_Soft_Stop();
            return 1;  /* 超时, 从机无响应 */
        }
    }

    I2C_SCL_LOW();
    I2C_SDA_OUT();    /* SDA切回输出模式 */
    return 0;          /* 收到ACK应答 */
}

/* 发送一个字节, 高位先发(MSB first) */
void I2C_Soft_SendByte(uint8_t byte)
{
    uint8_t i;

    I2C_SDA_OUT();
    I2C_SCL_LOW();

    for(i = 0; i < 8; i++)
    {
        /* 根据当前最高位设置SDA电平 */
        if(byte & 0x80)
            I2C_SDA_HIGH();
        else
            I2C_SDA_LOW();

        byte <<= 1;      /* 左移一位, 准备发送下一位 */
        I2C_Delay();
        I2C_SCL_HIGH();   /* SCL拉高, 从机在上升沿采样数据 */
        I2C_Delay();
        I2C_SCL_LOW();    /* SCL拉低, 准备下一位 */
    }
}

/* 读取一个字节, ack=1发送ACK(继续读), ack=0发送NACK(最后一字节) */
uint8_t I2C_Soft_ReadByte(uint8_t ack)
{
    uint8_t i, byte = 0;

    I2C_SDA_IN();     /* SDA切为输入, 读取从机数据 */

    for(i = 0; i < 8; i++)
    {
        I2C_SCL_LOW();
        I2C_Delay();
        I2C_SCL_HIGH();   /* SCL拉高, 读取SDA上的数据位 */
        byte <<= 1;
        if(I2C_SDA_READ())
            byte |= 0x01; /* SDA为高, 该位为1 */
        I2C_Delay();
    }

    I2C_SCL_LOW();
    I2C_SDA_OUT();

    /* 发送应答信号 */
    if(ack)
        I2C_SDA_LOW();   /* ACK: SDA拉低, 告诉从机继续发 */
    else
        I2C_SDA_HIGH();  /* NACK: SDA保持高, 告诉从机停止 */

    I2C_Delay();
    I2C_SCL_HIGH();
    I2C_Delay();
    I2C_SCL_LOW();

    return byte;
}

/* 向I2C设备的指定寄存器写入一个字节数据 */
void I2C_Soft_WriteReg(uint8_t addr, uint8_t reg, uint8_t data)
{
    I2C_Soft_Start();
    I2C_Soft_SendByte(addr);      /* 发送设备地址 + 写标志(0) */
    I2C_Soft_WaitAck();
    I2C_Soft_SendByte(reg);       /* 发送寄存器地址 */
    I2C_Soft_WaitAck();
    I2C_Soft_SendByte(data);      /* 发送数据 */
    I2C_Soft_WaitAck();
    I2C_Soft_Stop();
}
