#include "angle.h"
#include <math.h>

/*==========================================================
 * 互补滤波姿态解算
 *
 * 原理:
 *   加速度计: 能算出静态角度, 但有高频噪声(震动敏感)
 *   陀螺仪:   能测角速度积分得角度, 但会长期漂移
 *   互补滤波 = 高通(陀螺仪) + 低通(加速度计)
 *   angle = alpha * (angle + gyro*dt) + (1-alpha) * accel_angle
 *
 * alpha越大越信任陀螺仪(响应快但漂移), 越小越信任加速度计(稳定但噪声大)
 *==========================================================*/

#define ANGLE_PI        3.14159265f                  /* 圆周率 */
#define ANGLE_RAD2DEG   (180.0f / ANGLE_PI)          /* 弧度转角度系数 */
#define GYRO_SCALE      (2000.0f / 32768.0f)         /* 陀螺仪灵敏度: +/-2000度/秒量程 */
#define ALPHA           0.98f                        /* 互补滤波系数: 98%信任陀螺仪 */

static Angle_t s_angle = {0.0f, 0.0f, 0.0f};        /* 当前姿态角 */
static uint8_t s_first_run = 1;                      /* 首次运行标志 */

/* 初始化姿态解算模块, 清零所有角度 */
void Angle_Init(void)
{
    s_angle.pitch = 0.0f;   /* 俯仰角清零 */
    s_angle.roll  = 0.0f;   /* 横滚角清零 */
    s_angle.yaw   = 0.0f;   /* 偏航角清零 */
    s_first_run   = 1;      /* 重置首次运行标志 */
}

/**
 * 更新姿态角(互补滤波)
 *
 * 加速度计算角度:
 *   roll  = atan2(ay, az)                    横滚角: Y轴和Z轴的夹角
 *   pitch = atan2(-ax, sqrt(ay^2 + az^2))    俯仰角: X轴和YZ平面的夹角
 *
 * 陀螺仪积分:
 *   angle += gyro_rate * dt
 *
 * 互补滤波融合:
 *   angle = 0.98 * (angle + gyro*dt) + 0.02 * accel_angle
 */
void Angle_Update(int16_t ax, int16_t ay, int16_t az,
                  int16_t gx, int16_t gy, int16_t gz,
                  float dt)
{
    float accel_pitch, accel_roll;                   /* 加速度计算出的角度 */
    float gyro_roll_rate, gyro_pitch_rate, gyro_yaw_rate; /* 陀螺仪角速度(度/秒) */

    /*--- 第1步: 用加速度计计算静态角度 ---*/

    /* 横滚角 = atan2(ay, az), 结果转为角度制 */
    accel_roll  = atan2f((float)ay, (float)az) * ANGLE_RAD2DEG;

    /* 俯仰角 = atan2(-ax, sqrt(ay^2 + az^2)), 结果转为角度制 */
    accel_pitch = atan2f(-(float)ax,
                         sqrtf((float)ay * (float)ay + (float)az * (float)az))
                  * ANGLE_RAD2DEG;

    /*--- 第2步: 将陀螺仪原始值转换为角速度(度/秒) ---*/
    /* 原始值 * (2000/32768) = 角速度, 其中2000是量程, 32768是16位有符号最大值 */
    gyro_roll_rate  = (float)gx * GYRO_SCALE;        /* X轴陀螺仪 = 横滚角速度 */
    gyro_pitch_rate = (float)gy * GYRO_SCALE;        /* Y轴陀螺仪 = 俯仰角速度 */
    gyro_yaw_rate   = (float)gz * GYRO_SCALE;        /* Z轴陀螺仪 = 偏航角速度 */

    /*--- 第3步: 首次运行用加速度计角度初始化 ---*/
    if(s_first_run)
    {
        s_angle.pitch = accel_pitch;                  /* 直接用加速度计角度 */
        s_angle.roll  = accel_roll;                   /* 直接用加速度计角度 */
        s_angle.yaw   = 0.0f;                        /* 偏航角无法从加速度计获得 */
        s_first_run   = 0;                           /* 清除首次运行标志 */
        return;                                       /* 首次直接返回 */
    }

    /*--- 第4步: 互补滤波融合 ---*/
    /* pitch = 98%*(上次角度 + 陀螺仪积分) + 2%*(加速度计角度) */
    s_angle.pitch = ALPHA * (s_angle.pitch + gyro_pitch_rate * dt)
                  + (1.0f - ALPHA) * accel_pitch;

    /* roll = 98%*(上次角度 + 陀螺仪积分) + 2%*(加速度计角度) */
    s_angle.roll  = ALPHA * (s_angle.roll + gyro_roll_rate * dt)
                  + (1.0f - ALPHA) * accel_roll;

    /* yaw只能用陀螺仪积分, 没有磁力计无法校正, 会缓慢漂移 */
    s_angle.yaw  += gyro_yaw_rate * dt;
}

/* 获取当前姿态角指针 */
Angle_t* Angle_GetAngle(void)
{
    return &s_angle;   /* 返回姿态角结构体的指针 */
}
