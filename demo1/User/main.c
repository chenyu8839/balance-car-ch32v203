#include "debug.h"     /* WCH调试库: 提供串口printf和延时函数 */
#include "oled.h"      /* OLED显示屏驱动 (I2C总线在PB0/PB1) */
#include "mpu6050.h"   /* MPU6050六轴传感器驱动 (I2C总线在PB6/PB7) */
#include "angle.h"     /* 互补滤波姿态解算 */

/* 主循环延时时间(毫秒), 同时也是姿态更新的时间间隔 */
#define LOOP_DELAY_MS   10
/* 将毫秒转换为秒, 作为互补滤波的dt参数 */
#define DT              (LOOP_DELAY_MS / 1000.0f)

/*********************************************************************
 * @fn      main
 * @brief   主程序 - MPU6050姿态角显示
 * @return  无返回值
 */
int main(void)
{
    MPU6050_Data_t mpu_data;   /* MPU6050原始数据存储结构体 */
    Angle_t *angle;            /* 指向姿态角数据的指针 */
    uint8_t mpu_ok;            /* MPU6050初始化结果: 0=成功, 1=失败 */
    uint16_t display_cnt = 0;  /* 显示计数器, 用于降低OLED刷新频率 */

    /* 配置NVIC中断优先级分组: 1位抢占优先级, 3位子优先级 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

    /* 更新系统时钟变量, 使其与实际时钟频率一致(96MHz) */
    SystemCoreClockUpdate();

    /* 初始化延时函数(基于SysTick定时器) */
    Delay_Init();

    /* 初始化串口1, 波特率115200, 用于printf调试输出 */
    USART_Printf_Init(115200);

    /* 通过串口打印系统信息 */
    printf("SystemClk:%d\r\n", SystemCoreClock);   /* 打印系统时钟, 应为96000000 */
    printf("Balance Car - Angle Display\r\n");      /* 打印程序标识 */

    /* 初始化OLED显示屏 (I2C总线: PB0=SCL, PB1=SDA) */
    OLED_Init();

    /* 在OLED上显示启动信息 */
    OLED_ShowString(0, 0, "Balance Car", 16);       /* 第0-1行, 8x16大字体 */
    OLED_ShowString(0, 2, "MPU6050 Init...", 12);   /* 第2行, 显示初始化中 */

    /* 初始化MPU6050传感器 (I2C总线: PB6=SCL, PB7=SDA) */
    mpu_ok = MPU6050_Init();

    /* 检查MPU6050是否初始化成功 */
    if(mpu_ok != 0)
    {
        OLED_ShowString(0, 3, "MPU6050 FAIL!", 12); /* OLED显示失败 */
        printf("MPU6050 init failed!\r\n");          /* 串口打印失败 */
        while(1);                                    /* 停机, 无传感器无法继续 */
    }

    /* MPU6050初始化成功 */
    OLED_ShowString(0, 2, "MPU6050 OK!    ", 12);   /* 显示成功 */

    /* 初始化姿态解算模块 */
    Angle_Init();

    /* 短暂延时让用户看到初始化结果 */
    Delay_Ms(500);

    /* 清屏, 准备进入角度显示界面 */
    OLED_Clear();

    /* 显示固定标签(只需写一次) */
    OLED_ShowString(0, 0, "Balance Car", 16);       /* 标题, 8x16大字体 */
    OLED_ShowString(0, 2, "Pitch:", 12);            /* 俯仰角标签(前后倾斜) */
    OLED_ShowString(0, 3, "Roll :", 12);            /* 横滚角标签(左右倾斜) */
    OLED_ShowString(0, 4, "Yaw  :", 12);            /* 偏航角标签(水平旋转) */
    OLED_ShowString(0, 6, "T:", 12);                /* 温度标签 */

    /* 主循环: 快速读取传感器 + 姿态解算, 慢速刷新OLED */
    while(1)
    {
        /* 读取MPU6050全部传感器数据(加速度+陀螺仪+温度) */
        MPU6050_ReadAll(&mpu_data);

        /* 用互补滤波更新姿态角, dt=0.01秒(10ms) */
        Angle_Update(mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z,
                     mpu_data.gyro_x,  mpu_data.gyro_y,  mpu_data.gyro_z,
                     DT);

        /* 获取当前姿态角 */
        angle = Angle_GetAngle();

        /* 每10次循环刷新一次OLED(约100ms刷新一次, 避免I2C通信占用太多时间) */
        display_cnt++;
        if(display_cnt >= 10)
        {
            display_cnt = 0;                                /* 计数器清零 */

            /* 显示俯仰角Pitch(前后倾斜角度) */
            OLED_ShowString(36, 2, "       ", 12);          /* 先清除旧数值 */
            OLED_ShowFloat(36, 2, angle->pitch, 12);        /* 显示角度值 */

            /* 显示横滚角Roll(左右倾斜角度) */
            OLED_ShowString(36, 3, "       ", 12);          /* 先清除旧数值 */
            OLED_ShowFloat(36, 3, angle->roll, 12);         /* 显示角度值 */

            /* 显示偏航角Yaw(水平旋转角度, 会漂移) */
            OLED_ShowString(36, 4, "       ", 12);          /* 先清除旧数值 */
            OLED_ShowFloat(36, 4, angle->yaw, 12);          /* 显示角度值 */

            /* 显示温度 */
            OLED_ShowString(12, 6, "       ", 12);          /* 先清除旧数值 */
            OLED_ShowFloat(12, 6, mpu_data.temp_deg, 12);   /* 显示温度值 */

            /* 串口打印角度数据, 方便电脑端调试 */
            printf("P:%6.1f R:%6.1f Y:%6.1f\r\n",
                   angle->pitch, angle->roll, angle->yaw);
        }

        /* 延时10ms, 姿态解算频率=100Hz */
        Delay_Ms(LOOP_DELAY_MS);
    }
}
