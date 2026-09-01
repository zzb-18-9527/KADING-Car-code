# 油门低通滤波方案 — EMA (指数移动平均)

## 总结

对油门占空比 `duty_cycle` 施加一阶 EMA 低通滤波，滤除 ADC 噪声和踏板抖动，使电机输出平滑，提升驾驶体验。

## 当前状态分析

在 `Core/Src/main.c` 中，油门数据流为：

```
ADC_value[0] (0~4095) → gas (0~49) → duty_cycle (0~19) → 直接 motor_control()
```

**问题**：`duty_cycle` 直接写入 motor PWM 比较寄存器，任何 ADC 噪声或踏板机械抖动都会即时反映在电机输出上，导致起步/加速/减速时的突兀感。

## 方案选择

用户已选择 **EMA 低通滤波**。

### EMA 原理

```
filtered = filtered × (1-α) + raw × α
```

转换为整数实现（避免浮点运算, STM32F103 无 FPU）：

```c
// EMA_FACTOR = 4 对应 α = 1/4
filtered = (filtered × (EMA_FACTOR - 1) + raw) ÷ EMA_FACTOR
```

| EMA_FACTOR | α    | 平滑程度 | 推荐用途           |
|------------|------|----------|--------------------|
| 2          | 1/2  | 轻度     | 仅去除高频噪声     |
| **4**      | 1/4  | **中度** | **油门平滑（默认）** |
| 8          | 1/8  | 较强     | 有明显滞后感       |
| 16         | 1/16 | 重度     | 滞后明显           |

默认取 `EMA_FACTOR = 4`（α=1/4），兼顾平滑与响应速度。

## 修改内容

### 文件：`Core/Src/main.c`

共 **3 处修改**：

#### 修改 1 — 添加 EMA 常量定义（~第41行 USER CODE BEGIN PD）

```c
#define EMA_FACTOR   4       /* EMA 滤波系数: α = 1/EMA_FACTOR, 值越大越平滑 */
#define EMA_DIV(a)   ((a) / EMA_FACTOR)   /* 避免重复除法 */
```

#### 修改 2 — 添加滤波变量（~第69行，在 duty_cycle 附近）

```c
uint8_t duty_cycle = 0;
uint8_t filtered_duty_cycle = 0;   /* EMA 滤波后的油门值 */
```

#### 修改 3 — 在 duty_cycle 计算后插入 EMA 滤波（第143行之后）

在 `duty_cycle = ...` 之后、`OLED_Printf` 之前插入：

```c
/* EMA 低通滤波: filtered = (filtered × (EMA_FACTOR-1) + raw) / EMA_FACTOR */
filtered_duty_cycle = (uint8_t)(((uint16_t)filtered_duty_cycle * (EMA_FACTOR - 1) + (uint16_t)duty_cycle) / EMA_FACTOR);
```

#### 修改 4 — 将使用 duty_cycle 进行控制输出的位置替换为 filtered_duty_cycle

| 行号 | 原代码 | 改为 |
|------|--------|------|
| ~191 | `motor_control(forward, duty_cycle)` | `motor_control(forward, filtered_duty_cycle)` |
| ~194 | `if (duty_cycle < last_duty_cycle)` | `if (filtered_duty_cycle < last_duty_cycle)` |
| ~200 | `last_duty_cycle = duty_cycle` | `last_duty_cycle = filtered_duty_cycle` |
| ~203 | `g_engine_speed = duty_cycle` | `g_engine_speed = filtered_duty_cycle` |
| ~213 | `motor_control(back, duty_cycle)` | `motor_control(back, filtered_duty_cycle)` |

**说明**：OLED 调试显示（第144行）仍使用原始的 `duty_cycle`，方便观察踏板原始值。

## 验证方式

1. 编译：确认无警告/错误
2. 上电测试：快速踩下油门 → 观察电机是否平滑加速而非突兀弹射
3. 快速松油门 → 观察电机是否平滑减速
4. 调整 EMA_FACTOR 值（2 / 4 / 8）可改变滤波强度，找到最舒适的设定

## 设计与性能考量

- **计算量**：每次循环仅 2 次乘法 + 1 次除法（除法可用移位替换，但为可读性保留除法）
- **RAM 占用**：增加 1 字节 (`filtered_duty_cycle`)
- **ROM 占用**：可忽略
- **实时性**：零阻塞，纯计算
- **响应滞后**：EMA_FACTOR=4 时，阶跃响应到达 63% 约需 4 个采样周期（主循环约 30~50ms，即 120~200ms 滞后，人眼/手感可接受）
