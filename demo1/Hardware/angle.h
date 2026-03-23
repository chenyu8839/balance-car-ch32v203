#ifndef __ANGLE_H
#define __ANGLE_H

#include "ch32v20x.h"

/* 姿态角数据结构体 */
typedef struct {
    float pitch;    /* 俯仰角(前后倾斜), 单位:度, 正值=前倾, 负值=后仰 */
    float roll;     /* 横滚角(左右倾斜), 单位:度, 正值=右倾, 负值=左倾 */
    float yaw;      /* 偏航角(水平旋转), 单位:度, 仅陀螺仪积分, 会漂移 */
} Angle_t;

/* 初始化姿态解算模块 */
void Angle_Init(void);

/**
 * 更新姿态角(互补滤波算法)
 * 需要周期性调用, dt为两次调用之间的时间间隔(秒)
 *
 * @param ax,ay,az  加速度计原始值
 * @param gx,gy,gz  陀螺仪原始值
 * @param dt         时间间隔(秒), 例如0.01表示10ms
 */
void Angle_Update(int16_t ax, int16_t ay, int16_t az,
                  int16_t gx, int16_t gy, int16_t gz,
                  float dt);

/* 获取当前姿态角指针 */
Angle_t* Angle_GetAngle(void);

#endif
