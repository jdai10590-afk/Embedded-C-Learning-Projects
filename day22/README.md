# Day22：ADC 采样、电压换算与数字滤波

## 1. 学习目标

Day22 使用 C 语言模拟 12 位 ADC 采样和数字滤波过程。

本次主要实现：

```text
ADC 原始值范围检查
ADC 数字量转换为实际电压
一组采样数据的批量平均
连续采样数据的滑动平均
滑动窗口循环覆盖
非法 ADC 数据拒绝
```

程序最终验证：

```text
ADC=0    → 0.000 V
ADC=2048 → 1.650 V
ADC=4095 → 3.300 V

8 个样本的整数平均值为 2049
滑动平均窗口大小为 4
非法值 5000 被全部拒绝
```

---

## 2. 什么是 ADC

ADC 的英文全称是：

```text
Analog-to-Digital Converter
```

中文为：

```text
模数转换器
```

ADC 用于把连续变化的模拟电压转换为程序能够处理的数字量。

例如使用 12 位 ADC 和 3.3 V 参考电压：

```text
0 V       → ADC 约为 0
1.65 V    → ADC 约为 2048
3.3 V     → ADC 约为 4095
```

处理流程：

```text
传感器模拟电压
        ↓
ADC 硬件采样
        ↓
ADC 原始数字量
        ↓
软件换算和滤波
        ↓
实际电压或传感器数值
```

---

## 3. 工程结构

```text
day22
├── Makefile
├── README.md
├── include
│   ├── adc.h
│   ├── button.h
│   ├── can.h
│   ├── config.h
│   ├── control.h
│   ├── debug.h
│   ├── event.h
│   ├── fault.h
│   ├── gpio.h
│   ├── ring_buffer.h
│   ├── sensor.h
│   ├── software_timer.h
│   ├── state_machine.h
│   ├── system.h
│   ├── system_type.h
│   └── uart.h
├── src
│   ├── adc.c
│   ├── button.c
│   ├── can.c
│   ├── control.c
│   ├── event.c
│   ├── fault.c
│   ├── gpio.c
│   ├── main.c
│   ├── ring_buffer.c
│   ├── sensor.c
│   ├── software_timer.c
│   ├── state_machine.c
│   ├── system.c
│   └── uart.c
└── build
    └── day22_test
```

Day22 主要新增：

```text
include/adc.h
src/adc.c
```

主要修改：

```text
src/main.c
README.md
Makefile
```

---

## 4. ADC 配置参数

`adc.h` 中定义：

```c
#define ADC_RESOLUTION_BITS 12U

#define ADC_MAX_VALUE \
    ((1U << ADC_RESOLUTION_BITS) - 1U)

#define ADC_REFERENCE_VOLTAGE 3.3f

#define ADC_MOVING_AVERAGE_WINDOW 4U
```

参数含义：

| 参数 | 数值 | 作用 |
|---|---:|---|
| `ADC_RESOLUTION_BITS` | 12 | ADC 分辨率 |
| `ADC_MAX_VALUE` | 4095 | ADC 最大数字量 |
| `ADC_REFERENCE_VOLTAGE` | 3.3 V | ADC 参考电压 |
| `ADC_MOVING_AVERAGE_WINDOW` | 4 | 滑动平均窗口 |

---

## 5. 为什么 12 位 ADC 最大值是 4095

12 位可以表示：

```text
2¹² = 4096
```

个数字状态。

由于从 0 开始计数，因此范围为：

```text
0～4095
```

代码：

```c
(1U << 12) - 1U
```

计算过程：

```text
1U << 12 = 4096
4096 - 1 = 4095
```

如果以后改成 10 位 ADC：

```text
2¹⁰ - 1 = 1023
```

宏会自动得到新的最大值。

---

## 6. ADC 原始值合法性检查

函数：

```c
int adc_raw_is_valid(uint16_t raw_value);
```

判断范围：

```text
0～4095：合法
大于 4095：非法
```

核心代码：

```c
if(raw_value <= ADC_MAX_VALUE)
{
    return 1;
}

return 0;
```

虽然 `uint16_t` 可以保存到 65535，但 12 位 ADC 的有效范围仍然只有 0～4095。

---

## 7. ADC 原始值转电压

函数：

```c
int adc_raw_to_voltage(uint16_t raw_value,
                       float *voltage);
```

换算公式：

```text
电压 =
ADC原始值 ÷ ADC最大值 × 参考电压
```

代码：

```c
*voltage =
    ((float)raw_value /
     (float)ADC_MAX_VALUE) *
    ADC_REFERENCE_VOLTAGE;
```

例如：

```text
raw_value = 2048
ADC_MAX_VALUE = 4095
Vref = 3.3 V
```

计算：

```text
2048 ÷ 4095 × 3.3
≈ 1.6504 V
```

保留三位小数：

```text
1.650 V
```

---

## 8. 为什么必须转换为 float

如果写成：

```c
raw_value / ADC_MAX_VALUE
```

由于两边都是整数，会执行整数除法。

例如：

```text
2048 ÷ 4095 = 0
```

小数部分会被丢弃，最终电压会错误地得到：

```text
0 V
```

正确写法：

```c
(float)raw_value /
(float)ADC_MAX_VALUE
```

这样才会执行浮点除法。

---

## 9. 批量平均滤波

函数：

```c
int adc_average_raw(const uint16_t *samples,
                    uint8_t sample_count,
                    uint16_t *average_value);
```

测试样本：

```text
2048, 2055, 2038, 2060,
2044, 2052, 2040, 2058
```

总和：

```text
16395
```

平均值：

```text
16395 ÷ 8 = 2049.375
```

由于使用整数除法，小数部分被截断：

```text
average_raw = 2049
```

对应电压约为：

```text
1.651 V
```

---

## 10. 为什么累加和使用 uint32_t

单个 ADC 原始值使用：

```c
uint16_t
```

但多个采样值相加以后，结果可能超过 65535。

例如 255 个最大采样值：

```text
4095 × 255 = 1044225
```

因此累加变量使用：

```c
uint32_t sum;
```

避免溢出。

---

## 11. 滑动平均滤波器

结构体：

```c
typedef struct
{
    uint16_t buffer[ADC_MOVING_AVERAGE_WINDOW];

    uint32_t sum;

    uint8_t index;
    uint8_t count;
} AdcMovingAverage;
```

成员含义：

```text
buffer
保存最近 4 个采样值。

sum
保存当前窗口内所有有效采样值之和。

index
记录下一次需要替换的位置。

count
记录当前已经保存了多少个有效采样值。
```

---

## 12. 滑动平均初始化

函数：

```c
void adc_moving_average_init(
    AdcMovingAverage *filter);
```

初始化后：

```text
buffer[0～3] = 0
sum = 0
index = 0
count = 0
```

使用前必须初始化：

```c
AdcMovingAverage filter;

adc_moving_average_init(&filter);
```

否则结构体内部可能包含随机值。

---

## 13. 滑动平均更新公式

每来一个新数据，执行：

```text
新总和 = 旧总和 - 被替换的旧值 + 新值
```

代码：

```c
old_value =
    filter->buffer[filter->index];

filter->sum -= old_value;

filter->buffer[filter->index] =
    raw_value;

filter->sum += raw_value;
```

这种方法不需要每次重新遍历整个数组，效率更高。

---

## 14. index 的循环过程

窗口大小为 4，因此：

```text
index = 0 → 写 buffer[0]
index = 1 → 写 buffer[1]
index = 2 → 写 buffer[2]
index = 3 → 写 buffer[3]
index = 0 → 再次写 buffer[0]
```

代码：

```c
filter->index++;

if(filter->index >=
   ADC_MOVING_AVERAGE_WINDOW)
{
    filter->index = 0U;
}
```

这是一种简单的环形缓冲思想。

---

## 15. count 的作用

滤波器启动时，窗口还没有装满。

例如第一次只有一个数据：

```text
2048
```

应该计算：

```text
2048 ÷ 1 = 2048
```

不能直接除以窗口大小 4，否则会得到错误结果：

```text
2048 ÷ 4 = 512
```

因此使用：

```c
filtered_value =
    filter->sum / filter->count;
```

`count` 的变化：

```text
第 1 个样本：count = 1
第 2 个样本：count = 2
第 3 个样本：count = 3
第 4 个样本：count = 4
后续样本：count 保持 4
```

---

## 16. 滑动平均测试数据

输入：

```text
2048, 2055, 2038, 2060,
2070, 2025, 2050, 2045
```

滤波结果：

| 次数 | 原始值 | 滤波值 | 有效数量 |
|---:|---:|---:|---:|
| 1 | 2048 | 2048 | 1 |
| 2 | 2055 | 2051 | 2 |
| 3 | 2038 | 2047 | 3 |
| 4 | 2060 | 2050 | 4 |
| 5 | 2070 | 2055 | 4 |
| 6 | 2025 | 2048 | 4 |
| 7 | 2050 | 2051 | 4 |
| 8 | 2045 | 2047 | 4 |

原始数据从：

```text
2070 → 2025
```

变化了 45。

滤波数据从：

```text
2055 → 2048
```

只变化了 7。

说明滑动平均能够降低采样波动。

---

## 17. 数字滤波的代价

滑动平均能够让数据更稳定，但也会带来响应延迟。

窗口越大：

```text
滤波效果越明显
数据越平稳
响应速度越慢
```

窗口越小：

```text
响应速度越快
滤波效果越弱
```

所以实际项目中，需要根据传感器变化速度和系统要求选择窗口大小。

---

## 18. 非法 ADC 数据处理

当前 ADC 合法范围：

```text
0～4095
```

测试非法值：

```text
5000
```

程序验证：

```text
电压换算拒绝 5000
批量平均拒绝包含 5000 的数组
滑动平均拒绝 5000
```

实际输出：

```text
[INVALID] Raw value 5000 rejected during voltage conversion
[INVALID] Sample array rejected because it contains value 5000
[INVALID] Raw value 5000 rejected by moving average filter
```

非法数据不会进入滑动平均滤波器，也不会污染：

```text
buffer
sum
index
count
```

---

## 19. 实际运行结果

```text
===== Day22 ADC Sampling and Filter Test =====
[ADC CONFIG] Resolution = 12 bits
[ADC CONFIG] Maximum value = 4095
[ADC CONFIG] Reference voltage = 3.3 V
[ADC CONFIG] Moving average window = 4

----- Test 1: ADC Voltage Conversion -----
[ADC] Raw =    0, Voltage = 0.000 V
[ADC] Raw = 2048, Voltage = 1.650 V
[ADC] Raw = 4095, Voltage = 3.300 V

----- Test 2: Batch Average Filter -----
[SAMPLES] 2048, 2055, 2038, 2060, 2044, 2052, 2040, 2058
[AVERAGE] Raw average = 2049
[AVERAGE] Voltage     = 1.651 V

----- Test 3: Moving Average Filter -----
[SAMPLE 1] Raw=2048 (1.650 V), Filtered=2048 (1.650 V), Count=1
[SAMPLE 2] Raw=2055 (1.656 V), Filtered=2051 (1.653 V), Count=2
[SAMPLE 3] Raw=2038 (1.642 V), Filtered=2047 (1.650 V), Count=3
[SAMPLE 4] Raw=2060 (1.660 V), Filtered=2050 (1.652 V), Count=4
[SAMPLE 5] Raw=2070 (1.668 V), Filtered=2055 (1.656 V), Count=4
[SAMPLE 6] Raw=2025 (1.632 V), Filtered=2048 (1.650 V), Count=4
[SAMPLE 7] Raw=2050 (1.652 V), Filtered=2051 (1.653 V), Count=4
[SAMPLE 8] Raw=2045 (1.648 V), Filtered=2047 (1.650 V), Count=4

----- Test 4: Invalid ADC Data -----
[INVALID] Raw value 5000 rejected during voltage conversion
[INVALID] Sample array rejected because it contains value 5000
[INVALID] Raw value 5000 rejected by moving average filter

===== ADC Test Finished =====
```

---

## 20. 编译运行

进入 Day22：

```bash
cd /root/Embedded_14Days/day22
```

清理：

```bash
make clean
```

编译：

```bash
make
```

运行：

```bash
make run
```

筛选关键结果：

```bash
make run | grep -E "ADC CONFIG|ADC\]|AVERAGE|SAMPLE|INVALID"
```

---

## 21. 自动验证

验证 ADC 中点电压：

```bash
make run | grep -c "2048, Voltage = 1.650 V"
```

预期：

```text
1
```

验证批量平均：

```bash
make run | grep -c "Raw average = 2049"
```

预期：

```text
1
```

验证非法数据被拒绝的次数：

```bash
make run | grep -c "\[INVALID\]"
```

预期：

```text
3
```

---

## 22. 核心代码总结

ADC 最大值：

```c
(1U << ADC_RESOLUTION_BITS) - 1U
```

原始值转电压：

```c
((float)raw_value /
 (float)ADC_MAX_VALUE) *
ADC_REFERENCE_VOLTAGE
```

批量平均：

```c
sum / sample_count
```

滑动平均总和更新：

```c
new_sum =
    old_sum - oldest_value + new_value;
```

---

## 23. 当前实现的局限

当前程序属于 ADC 软件模拟，尚未涉及真实硬件中的：

```text
ADC GPIO 模拟输入配置
ADC 时钟配置
采样周期配置
规则通道配置
ADC 校准
软件触发
连续转换
扫描模式
ADC 中断
DMA 自动搬运
多通道采样
内部温度传感器
参考电压校准
```

当前滤波算法也比较基础，后续还可以学习：

```text
中值滤波
限幅滤波
一阶低通滤波
加权平均滤波
卡尔曼滤波
```

---

## 24. 今日收获

通过 Day22 学习，掌握了：

1. ADC 的基本作用；
2. ADC 分辨率的含义；
3. 12 位 ADC 的数字范围；
4. 参考电压的作用；
5. ADC 原始值到电压的换算；
6. 整数除法和浮点除法的区别；
7. 使用 `float` 保存电压；
8. 检查 ADC 数据范围；
9. 批量平均滤波；
10. 使用更宽类型保存累加和；
11. 滑动平均滤波；
12. 使用数组保存最近采样值；
13. 使用 `index` 实现循环覆盖；
14. 使用 `count` 处理启动阶段；
15. 使用旧和减旧值加新值提高效率；
16. 滤波稳定性和响应速度之间的关系；
17. 拒绝非法数据，避免污染滤波器；
18. 为真实 ADC 和 DMA 采样打下基础。

当前学习路线：

```text
Day15：GPIO 寄存器与位操作
Day16：UART 完整命令解析
Day17：Ring Buffer 与 UART 字节流
Day18：系统 Tick 与软件定时器
Day19：按键边沿检测与软件消抖
Day20：按键短按、长按与持续按住检测
Day21：CAN 数据帧、ID 过滤与数据解析
Day22：ADC 采样、电压换算与数字滤波
```