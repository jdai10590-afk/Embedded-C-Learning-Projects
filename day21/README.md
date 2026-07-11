# Day21：CAN 数据帧、ID 过滤与数据解析

## 1. 学习目标

Day21 学习 CAN 总线最基础的软件概念，并使用 C 语言模拟两个节点之间的 CAN 数据传输。

本次主要实现：

```text
定义经典 CAN 数据帧
设置标准 CAN ID
设置 DLC 数据长度
将业务数据打包成字节
模拟 CAN 帧发送和接收
根据 CAN ID 过滤消息
从 CAN 数据区解析业务数据
验证不匹配 ID 被过滤器拒绝
```

最终实现的通信流程：

```text
电机节点组装 CAN 帧
        ↓
模拟发送到 CAN 总线
        ↓
接收节点复制 CAN 帧
        ↓
根据 CAN ID 过滤
        ↓
解析转速、温度和故障码
```

---

## 2. CAN 是什么

CAN 的英文全称是：

```text
Controller Area Network
```

中文通常称为：

```text
控制器局域网
```

CAN 常用于：

```text
汽车电子
电机控制
电池管理系统 BMS
工业控制
机器人
新能源设备
嵌入式控制器通信
```

一条 CAN 总线上可以连接多个节点。

例如：

```text
电机控制器
电池管理系统
仪表节点
主控制器
故障诊断节点
```

所有节点都可以看到总线上的消息，但每个节点根据 CAN ID 决定是否处理。

---

## 3. CAN 和 UART 的区别

| 对比项 | UART | CAN |
|---|---|---|
| 通信结构 | 通常点对点 | 多节点总线 |
| 信号线 | TX、RX | CANH、CANL |
| 消息标识 | 通常没有 CAN ID | 每帧都有 CAN ID |
| 数据过滤 | 通常由软件协议处理 | 控制器可按 ID 过滤 |
| 总线竞争 | 通常不涉及 | 支持总线仲裁 |
| 错误检测 | 相对简单 | CRC、ACK、错误帧等 |
| 抗干扰能力 | 一般 | 较强 |

UART 更像：

```text
设备 A 直接向设备 B 发送字节流
```

CAN 更像：

```text
多个节点共用一条总线，
消息通过 CAN ID 区分类型和优先级。
```

---

## 4. 真实 CAN 总线结构

真实 CAN 总线通常使用两根差分信号线：

```text
CANH
CANL
```

典型结构：

```text
120Ω                            120Ω
终端电阻                         终端电阻
  │                                │
CANH ──────┬────────┬───────────── CANH
CANL ──────┴────────┴───────────── CANL
           │        │
         节点 A   节点 B
```

总线两端通常各接一个 120Ω 终端电阻。

断电后测量 CANH 和 CANL 之间的电阻，两个 120Ω 并联，通常约为：

```text
60Ω
```

本项目目前只模拟 CAN 软件协议，不连接真实 CAN 收发器。

---

## 5. 工程结构

```text
day21
├── Makefile
├── README.md
├── include
│   ├── button.h
│   ├── can.h
│   ├── config.h
│   ├── control.h
│   ├── debug.h
│   ├── event.h
│   ├── fault.h
│   ├── fault_code.h
│   ├── gpio.h
│   ├── ring_buffer.h
│   ├── sensor.h
│   ├── software_timer.h
│   ├── state_machine.h
│   ├── system.h
│   ├── system_type.h
│   └── uart.h
├── src
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
    └── day21_test
```

Day21 主要新增：

```text
include/can.h
src/can.c
```

主要修改：

```text
src/main.c
README.md
Makefile
```

Makefile 目标程序：

```makefile
TARGET = build/day21_test
```

---

## 6. 经典 CAN 数据长度

经典 CAN 一帧最多携带：

```text
8 字节
```

代码定义：

```c
#define CAN_MAX_DATA_LEN 8U
```

数据数组：

```c
uint8_t data[CAN_MAX_DATA_LEN];
```

等价于：

```c
uint8_t data[8];
```

有效下标为：

```text
data[0]～data[7]
```

CAN FD 的数据区可以达到 64 字节，但本次只学习经典 CAN。

---

## 7. 标准 CAN ID

标准 CAN 使用 11 位标识符。

ID 范围为：

```text
0x000～0x7FF
```

代码定义：

```c
#define CAN_STANDARD_ID_MAX 0x7FFU
```

本项目约定：

```c
#define CAN_ID_MOTOR_STATUS    0x101U
#define CAN_ID_BATTERY_STATUS  0x201U
```

含义：

| CAN ID | 消息类型 |
|---|---|
| `0x101` | 电机状态消息 |
| `0x201` | 电池状态消息 |

这些 ID 是项目通信协议的约定，不是 CAN 标准强制规定的固定含义。

---

## 8. CAN ID 的作用

CAN ID 主要用于：

```text
表示消息类型
接收过滤
参与总线仲裁
区分消息优先级
```

CAN ID 通常表示“消息是什么”，而不是简单表示“发给哪个设备”。

例如：

```text
0x101：电机状态
```

任何需要电机状态的节点都可以接收和处理该帧。

---

## 9. CAN ID 与仲裁优先级

多个节点可能同时尝试发送 CAN 帧。

CAN 总线通过逐位仲裁决定哪个节点优先发送。

CAN 中：

```text
0：显性位
1：隐性位
```

在其他条件一致时，数值较小的 CAN ID 通常优先级更高。

例如：

```text
0x101 优先于 0x201
0x201 优先于 0x301
```

仲裁失败的节点不会破坏当前帧，而是等待总线空闲后重新发送。

---

## 10. DLC

DLC 的英文是：

```text
Data Length Code
```

表示 CAN 数据区中有多少个有效字节。

例如：

```text
DLC = 0：没有有效数据
DLC = 2：data[0]～data[1] 有效
DLC = 4：data[0]～data[3] 有效
DLC = 8：data[0]～data[7] 有效
```

结构体成员：

```c
uint8_t dlc;
```

注意：

```text
数组容量是 8 字节
DLC 表示本帧实际使用多少字节
```

---

## 11. CAN 数据帧结构

```c
typedef struct
{
    uint32_t id;
    uint8_t dlc;
    uint8_t data[CAN_MAX_DATA_LEN];
} CanFrame;
```

成员含义：

```text
id
保存 CAN 标识符。

dlc
保存有效数据长度。

data
保存最多 8 个数据字节。
```

例如：

```text
ID   = 0x101
DLC  = 4
DATA = 05 DC 41 00
```

对应：

```text
id = 0x101
dlc = 4

data[0] = 0x05
data[1] = 0xDC
data[2] = 0x41
data[3] = 0x00
```

---

## 12. 为什么使用 stdint.h

`can.h` 中包含：

```c
#include <stdint.h>
```

该头文件提供固定宽度整数类型：

| 类型 | 位数 | 常见范围 |
|---|---:|---:|
| `uint8_t` | 8 位 | 0～255 |
| `int8_t` | 8 位 | -128～127 |
| `uint16_t` | 16 位 | 0～65535 |
| `uint32_t` | 32 位 | 0～4294967295 |

例如：

```c
uint8_t data;
```

明确表示：

```text
该变量占 8 位，即一个字节
```

这种类型在通信协议和寄存器开发中非常常见。

---

## 13. CAN 接收过滤器

当前实现了一个简单的精确 ID 过滤器：

```c
typedef struct
{
    uint32_t accepted_id;
} CanFilter;
```

初始化：

```c
can_filter_init(&motor_status_filter,
                CAN_ID_MOTOR_STATUS);
```

初始化后：

```text
accepted_id = 0x101
```

接收到：

```text
ID = 0x101
```

返回：

```text
接受
```

接收到：

```text
ID = 0x201
```

返回：

```text
拒绝
```

真实 CAN 控制器通常还支持：

```text
掩码过滤
列表过滤
标准帧过滤
扩展帧过滤
接收 FIFO 分配
```

---

## 14. CAN 帧初始化

函数：

```c
void can_frame_init(CanFrame *frame,
                    uint32_t id);
```

主要完成：

```text
检查结构体指针
检查标准帧 ID 范围
设置 CAN ID
设置 DLC 为 0
清空 8 字节数据区
```

调用：

```c
CanFrame frame;

can_frame_init(&frame,
               CAN_ID_MOTOR_STATUS);
```

初始化后：

```text
id = 0x101
dlc = 0
data[0～7] = 0
```

---

## 15. 为什么要检查空指针

代码：

```c
if(frame == NULL)
{
    return;
}
```

如果传入：

```c
can_frame_init(NULL, 0x101U);
```

函数没有合法结构体地址可以操作。

如果继续访问：

```c
frame->id
```

可能造成程序崩溃。

因此模块函数在使用指针前应进行必要检查。

---

## 16. 设置 CAN 数据区

函数：

```c
int can_frame_set_data(CanFrame *frame,
                       const uint8_t *data,
                       uint8_t dlc);
```

成功返回：

```text
1
```

失败返回：

```text
0
```

失败情况包括：

```text
frame 是 NULL
DLC 大于 8
DLC 大于 0 但 data 是 NULL
```

核心判断：

```c
if(dlc > CAN_MAX_DATA_LEN)
{
    return 0;
}
```

经典 CAN 数据长度不能超过 8 字节。

---

## 17. 为什么先清空数据区

设置新数据前执行：

```c
for(index = 0U;
    index < CAN_MAX_DATA_LEN;
    index++)
{
    frame->data[index] = 0U;
}
```

假设旧数据为：

```text
11 22 33 44 55 66 77 88
```

新帧只使用两个字节：

```text
AA BB
```

清空后会变成：

```text
AA BB 00 00 00 00 00 00
```

虽然 DLC 为 2 时只有前两个字节有效，但清零有利于调试和避免残留数据造成误解。

---

## 18. 电机状态协议

本项目规定 `0x101` 电机状态帧的数据布局：

| 数据字节 | 含义 |
|---|---|
| `data[0]` | 电机转速高字节 |
| `data[1]` | 电机转速低字节 |
| `data[2]` | 电机温度 |
| `data[3]` | 电机故障码 |

测试数据：

```text
转速：1500 rpm
温度：65℃
故障码：0
```

最终帧：

```text
ID   = 0x101
DLC  = 4
DATA = 05 DC 41 00
```

---

## 19. 为什么转速需要两个字节

一个字节最大表示：

```text
255
```

而测试转速为：

```text
1500 rpm
```

一个字节无法保存，所以使用 `uint16_t`：

```c
uint16_t speed_rpm;
```

1500 的十六进制为：

```text
1500 = 0x05DC
```

拆分为：

```text
高字节：0x05
低字节：0xDC
```

---

## 20. 提取高字节

代码：

```c
payload[0] =
    (uint8_t)((speed_rpm >> 8) & 0xFFU);
```

当：

```text
speed_rpm = 0x05DC
```

右移 8 位：

```text
0x05DC >> 8 = 0x0005
```

再与 `0xFF`：

```text
0x0005 & 0x00FF = 0x05
```

所以：

```text
payload[0] = 0x05
```

---

## 21. 提取低字节

代码：

```c
payload[1] =
    (uint8_t)(speed_rpm & 0xFFU);
```

计算：

```text
0x05DC & 0x00FF = 0x00DC
```

所以：

```text
payload[1] = 0xDC
```

最终：

```text
1500 → 05 DC
```

---

## 22. 温度和故障码

温度参数：

```c
int8_t temperature_c;
```

使用有符号类型，是因为温度可能小于 0℃。

例如：

```text
-20℃
0℃
65℃
100℃
```

都可以使用 `int8_t` 表示。

打包：

```c
payload[2] = (uint8_t)temperature_c;
```

故障码：

```c
payload[3] = fault_code;
```

本次测试：

```text
temperature = 65 = 0x41
fault_code = 0 = 0x00
```

---

## 23. 电机状态打包函数

函数：

```c
int can_pack_motor_status(CanFrame *frame,
                          uint16_t speed_rpm,
                          int8_t temperature_c,
                          uint8_t fault_code);
```

调用：

```c
can_pack_motor_status(&motor_tx_frame,
                      1500U,
                      65,
                      0U);
```

生成：

```text
ID   = 0x101
DLC  = 4
DATA = 05 DC 41 00
```

“打包”表示：

```text
将程序中的业务变量，
按照通信协议转换成连续的字节数据。
```

---

## 24. 电机状态解析函数

函数：

```c
int can_parse_motor_status(const CanFrame *frame,
                           uint16_t *speed_rpm,
                           int8_t *temperature_c,
                           uint8_t *fault_code);
```

调用：

```c
can_parse_motor_status(&rx_frame,
                       &motor_speed_rpm,
                       &motor_temperature_c,
                       &motor_fault_code);
```

解析成功后：

```text
motor_speed_rpm = 1500
motor_temperature_c = 65
motor_fault_code = 0
```

使用指针输出参数，是因为一个函数需要同时返回多个解析结果。

---

## 25. 解析前为什么检查 CAN ID

代码：

```c
if(frame->id != CAN_ID_MOTOR_STATUS)
{
    return 0;
}
```

因为不同 CAN ID 对应不同的数据格式。

例如：

```text
0x101：电机状态
0x201：电池状态
```

不能把电池帧按照电机协议解析。

正确流程：

```text
检查 CAN ID
        ↓
选择对应协议
        ↓
解析数据
```

---

## 26. 解析前为什么检查 DLC

代码：

```c
if(frame->dlc < 4U)
{
    return 0;
}
```

电机状态协议需要：

```text
data[0]
data[1]
data[2]
data[3]
```

共 4 个有效字节。

如果收到：

```text
DLC = 2
```

说明数据不完整，不能按照当前协议解析。

---

## 27. 高低字节重新组合

解析代码：

```c
*speed_rpm =
    (uint16_t)(
        ((uint16_t)frame->data[0] << 8) |
        (uint16_t)frame->data[1]);
```

接收数据：

```text
data[0] = 0x05
data[1] = 0xDC
```

高字节左移：

```text
0x05 << 8 = 0x0500
```

再与低字节按位或：

```text
0x0500 | 0x00DC = 0x05DC
```

最终：

```text
0x05DC = 1500
```

---

## 28. 字节序

本次协议规定：

```text
高字节在前
低字节在后
```

即：

```text
data[0]：高字节
data[1]：低字节
```

这种格式通常称为：

```text
大端序
Big Endian
```

发送端和接收端必须使用相同字节序。

如果接收端反过来解析：

```text
05 DC → 0xDC05
```

就无法得到正确的 1500。

---

## 29. CAN 帧打印

函数：

```c
void can_print_frame(const char *direction,
                     const CanFrame *frame);
```

发送调用：

```c
can_print_frame("TX", &motor_tx_frame);
```

接收调用：

```c
can_print_frame("RX", &rx_frame);
```

输出：

```text
[CAN TX] ID=0x101 DLC=4 DATA=05 DC 41 00
[CAN RX] ID=0x101 DLC=4 DATA=05 DC 41 00
```

其中：

```text
TX：Transmit，发送
RX：Receive，接收
```

---

## 30. 十六进制格式控制

打印 CAN ID 使用：

```c
%03X
```

表示：

```text
至少显示三位十六进制
```

例如：

```text
001
101
7FF
```

打印一个数据字节使用：

```c
%02X
```

表示：

```text
至少显示两位十六进制
```

例如：

```text
00
05
41
DC
FF
```

---

## 31. 模拟总线接收

主程序中：

```c
rx_frame = *bus_frame;
```

表示将总线上的完整结构体复制到接收帧。

复制内容包括：

```text
id
dlc
data[0]～data[7]
```

结构体可以直接整体赋值：

```c
frame_b = frame_a;
```

复制后，修改 `rx_frame` 不会影响原来的发送帧。

---

## 32. 接收处理流程

接收函数的逻辑为：

```text
复制总线帧
        ↓
打印 RX 数据
        ↓
检查过滤器
        ↓
CAN ID 匹配？
   ┌────┴────┐
   │         │
  不匹配     匹配
   │         │
  拒绝      解析业务数据
```

过滤必须发生在解析之前。

---

## 33. 测试一：电机状态帧

发送：

```text
ID   = 0x101
DLC  = 4
DATA = 05 DC 41 00
```

过滤器：

```text
accepted_id = 0x101
```

因此：

```text
0x101 == 0x101
```

帧通过过滤器。

运行结果：

```text
[CAN TX] ID=0x101 DLC=4 DATA=05 DC 41 00
[CAN RX] ID=0x101 DLC=4 DATA=05 DC 41 00
[CAN RX] ID=0x101 accepted by filter
```

---

## 34. 电机数据解析结果

运行结果：

```text
[PARSE] Motor speed       = 1500 rpm
[PARSE] Motor temperature = 65 C
[PARSE] Motor fault code  = 0
```

说明：

```text
CAN ID 正确
DLC 正确
字节打包正确
字节顺序正确
接收解析正确
```

---

## 35. 测试二：电池状态帧

发送：

```text
ID   = 0x201
DLC  = 4
DATA = 04 E4 00 78
```

过滤器仍然只接受：

```text
0x101
```

因此：

```text
0x201 != 0x101
```

帧被拒绝。

运行结果：

```text
[CAN TX] ID=0x201 DLC=4 DATA=04 E4 00 78
[CAN RX] ID=0x201 DLC=4 DATA=04 E4 00 78
[CAN RX] ID=0x201 rejected by filter
```

电池帧不会被错误地交给电机状态解析函数。

---

## 36. 电池模拟数据

本次电池数据：

```text
04 E4 00 78
```

可以约定：

```text
data[0～1]：电池电压，单位 0.01 V
data[2～3]：电池电流，单位 0.01 A
```

则：

```text
0x04E4 = 1252
1252 × 0.01 V = 12.52 V
```

以及：

```text
0x0078 = 120
120 × 0.01 A = 1.20 A
```

本次没有实现电池解析，因为接收节点只关注 `0x101` 电机状态帧。

---

## 37. 实际运行结果

```text
===== Day21 CAN Frame Simulation =====
[CAN FILTER] Accepted ID = 0x101

----- Test 1: Motor Status Frame -----
[CAN TX] ID=0x101 DLC=4 DATA=05 DC 41 00
[CAN RX] ID=0x101 DLC=4 DATA=05 DC 41 00
[CAN RX] ID=0x101 accepted by filter
[PARSE] Motor speed       = 1500 rpm
[PARSE] Motor temperature = 65 C
[PARSE] Motor fault code  = 0

----- Test 2: Battery Status Frame -----
[CAN TX] ID=0x201 DLC=4 DATA=04 E4 00 78
[CAN RX] ID=0x201 DLC=4 DATA=04 E4 00 78
[CAN RX] ID=0x201 rejected by filter

===== CAN Test Finished =====
```

结果完全符合预期。

---

## 38. 自动验证

验证接受帧数量：

```bash
make run | grep -c "accepted by filter"
```

预期：

```text
1
```

验证拒绝帧数量：

```bash
make run | grep -c "rejected by filter"
```

预期：

```text
1
```

验证转速解析：

```bash
make run | grep -c "1500 rpm"
```

预期：

```text
1
```

---

## 39. 编译运行

进入 Day21：

```bash
cd /root/Embedded_14Days/day21
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

筛选 CAN 结果：

```bash
make run | grep -E "CAN TX|CAN RX|PARSE|FILTER"
```

---

## 40. 本次核心流程

```text
业务变量
    ↓
按照协议拆成字节
    ↓
设置 CAN ID 和 DLC
    ↓
生成 CAN 数据帧
    ↓
模拟发送到总线
    ↓
接收节点复制帧
    ↓
按照 CAN ID 过滤
    ↓
按照相同协议解析字节
    ↓
恢复业务变量
```

---

## 41. 本次核心代码

提取高字节：

```c
(speed_rpm >> 8) & 0xFFU
```

提取低字节：

```c
speed_rpm & 0xFFU
```

组合高低字节：

```c
((uint16_t)data[0] << 8) |
(uint16_t)data[1]
```

精确 ID 过滤：

```c
frame->id == filter->accepted_id
```

检查经典 CAN 长度：

```c
dlc <= CAN_MAX_DATA_LEN
```

---

## 42. 当前实现的局限

当前程序属于 CAN 软件概念模拟，尚未包含真实硬件功能：

```text
CAN 控制器寄存器
CAN 波特率计算
CAN GPIO 复用
CAN 收发器
CANH 和 CANL 电平
硬件发送邮箱
接收 FIFO
中断接收
错误状态寄存器
总线关闭 Bus-Off
自动重发
实际总线仲裁
```

当前过滤器也只支持：

```text
单个 CAN ID 精确匹配
```

后续可以扩展：

```text
多个 ID 列表
ID 掩码过滤
扩展帧
远程帧
CAN 错误处理
接收队列
```

---

## 43. 今日收获

通过 Day21 学习，掌握了：

1. CAN 总线的基本概念；
2. CAN 与 UART 的主要区别；
3. CANH、CANL 和终端电阻的作用；
4. 标准 CAN 的 11 位 ID；
5. 经典 CAN 最多 8 字节数据；
6. DLC 的含义；
7. CAN ID 的消息标识作用；
8. CAN ID 与仲裁优先级的基本关系；
9. 使用结构体描述 CAN 数据帧；
10. 使用 `stdint.h` 固定宽度整数类型；
11. 初始化 CAN 数据帧；
12. 检查空指针和数据长度；
13. 根据 CAN ID 过滤帧；
14. 将 16 位数据拆成两个 8 位字节；
15. 使用位移提取高字节；
16. 使用按位与提取低字节；
17. 使用左移和按位或重新组合数据；
18. 理解大端字节序；
19. 打包电机转速、温度和故障码；
20. 从 CAN 数据帧恢复业务变量；
21. 先过滤再解析；
22. 使用结构体整体赋值模拟接收缓冲区；
23. 使用返回值判断打包和解析是否成功；
24. 使用命令验证程序运行结果。

当前学习路线：

```text
Day15：GPIO 寄存器与位操作
Day16：UART 完整命令解析
Day17：Ring Buffer 与 UART 字节流
Day18：系统 Tick 与软件定时器
Day19：按键边沿检测与软件消抖
Day20：按键短按、长按与持续按住检测
Day21：CAN 数据帧、ID 过滤与数据解析
```