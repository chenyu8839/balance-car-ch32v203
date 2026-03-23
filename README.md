# CH32V203 二轮自平衡小车

基于 **CH32V203 RISC-V MCU** 的二轮自平衡小车项目，从零开始逐步实现平衡控制。

## 硬件配置

| 模块 | 型号/规格 | 说明 |
|------|----------|------|
| 主控芯片 | CH32V203 (RISC-V, 96MHz) | WCH 沁恒微电子 |
| 姿态传感器 | MPU6050 | 6轴(加速度计+陀螺仪), I2C通信 |
| OLED显示屏 | 0.96寸 SSD1315/SSD1306 | 128×64, I2C通信 |
| 电机驱动 | TB6612FNG | 双路H桥, 支持PWM调速 |
| 电机 | 直流编码电机 | R3底盘, 13线霍尔编码器 |
| 电源 | 12V锂电池 | MP1584(DC-DC) + AMS1117-3.3(LDO) |
| 蓝牙(v2) | SPP透传模块 | 兼容HC-05/06, UART通信 |

## 引脚分配

| 功能 | SCL | SDA |
|------|-----|-----|
| OLED (软件I2C) | PB0 | PB1 |
| MPU6050 (软件I2C) | PB6 | PB7 |

## 项目结构

```
demo1/
├── Core/                    # RISC-V 内核文件
│   ├── core_riscv.c
│   └── core_riscv.h
├── Debug/                   # WCH 调试库(串口printf, 延时)
│   ├── debug.c
│   └── debug.h
├── Hardware/                # 自定义硬件驱动 ★
│   ├── i2c_soft.c/h        # OLED专用软件I2C (PB0/PB1)
│   ├── oled.c/h             # OLED显示驱动 (SSD1306/SSD1315)
│   ├── oledfont.h           # 字库数据 (6x8 + 8x16 ASCII)
│   ├── mpu6050.c/h          # MPU6050六轴传感器驱动 (PB6/PB7)
│   └── angle.c/h            # 互补滤波姿态解算 (Pitch/Roll/Yaw)
├── Ld/                      # 链接脚本
│   └── Link.ld
├── Peripheral/              # CH32V20x 官方外设库
│   ├── inc/                 # 头文件
│   └── src/                 # 源文件
├── Startup/                 # 启动汇编文件
├── User/                    # 用户代码 ★
│   ├── main.c               # 主程序
│   ├── ch32v20x_conf.h      # 外设库配置
│   ├── ch32v20x_it.c/h      # 中断服务函数
│   └── system_ch32v20x.c/h  # 系统时钟配置
├── .cproject                # MounRiver 工程配置
└── .project                 # Eclipse 工程文件
```

## 开发环境

- **IDE**: MounRiver Studio (基于Eclipse, WCH官方)
- **编译器**: RISC-V GCC
- **烧录工具**: WCHISPTool
- **系统时钟**: HSE 8MHz × 12 = 96MHz

## 开发进度

- [x] 1. 开发环境搭建 (MounRiver Studio)
- [x] 2. PCB设计
- [x] 3. 创建工程
- [x] 4. OLED显示屏驱动 (软件I2C)
- [x] 5. MPU6050传感器驱动 (软件I2C)
- [x] 6. 姿态解算 (互补滤波: Pitch/Roll/Yaw)
- [ ] 7. 电机驱动 (TB6612FNG + PWM)
- [ ] 8. 编码器读取 (13线霍尔编码器)
- [ ] 9. PID控制算法
- [ ] 10. 整合测试与调试优化
- [ ] 11. 蓝牙遥控 (v2)

## 已实现功能

### OLED显示
- SSD1306/SSD1315 驱动, 支持字符/字符串/整数/浮点数显示
- 软件I2C通信 (PB0=SCL, PB1=SDA)

### MPU6050传感器
- 加速度计: ±2g (灵敏度 16384 LSB/g)
- 陀螺仪: ±2000°/s (灵敏度 16.4 LSB/°/s)
- 采样率: 100Hz, 低通滤波: 42Hz
- 突发读取14字节, 保证数据一致性

### 互补滤波姿态解算
- 融合加速度计(低频稳定)和陀螺仪(高频响应)
- 滤波系数 α=0.98 (98%信任陀螺仪)
- 输出: Pitch(俯仰角), Roll(横滚角), Yaw(偏航角)
- 解算频率: 100Hz

```
angle = 0.98 × (angle + gyro×dt) + 0.02 × accel_angle
```

## 构建与烧录

1. 用 MounRiver Studio 打开 `demo1/.project`
2. 确保 Hardware 目录已加入 Include 路径
3. 编译: `Ctrl+B`
4. 烧录: 使用 WCHISPTool 下载 hex 文件

## License

MIT
