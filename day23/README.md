# Day23：SPI 基础与单字节全双工通信

## 1. 学习目标

Day23 使用 C 语言建立一个简化的 SPI 软件模型，重点理解 SPI 的基本通信过程，而不是直接配置真实单片机寄存器。

本次主要完成：

```text
理解 SPI 主机和从机
认识 SCK、MOSI、MISO、CS
理解同步通信
理解单字节全双工交换
使用占位字节读取从机数据
模拟 CS 片选和释放
检查未片选传输
检查重复片选和重复释放
检查无效时钟、无效模式和空指针
统计成功传输次数
```

最终验证结果：

```text
主机发送 0xA5
从机返回 0x5A

主机发送占位字节 0xFF
从机返回设备 ID 0x42

未拉低 CS 时传输被拒绝
失败传输不会增加成功计数
程序结束时从机处于释放状态
```

---

## 2. 什么是 SPI

SPI 的英文全称是：

```text
Serial Peripheral Interface
```

中文通常称为：

```text
串行外设接口
```

SPI 是一种同步串行通信方式。

“同步”表示主机和从机使用同一根时钟线决定数据传输节奏。

SPI 通常由主机产生时钟，从机根据主机提供的时钟完成数据接收和发送。

典型应用包括：

```text
Flash 存储器
显示屏
SD 卡
ADC/DAC 芯片
温度传感器
加速度计
陀螺仪
无线通信模块
```

---

## 3. SPI 主机和从机

SPI 通信通常包含：

```text
主机 Master
从机 Slave
```

主机负责：

```text
产生 SCK 时钟
控制 CS 片选
决定何时开始通信
决定传输多少个字节
```

从机负责：

```text
等待主机选择
按照主机时钟接收数据
同时向主机返回数据
```

例如：

```text
GD32 单片机：主机
外部 Flash：从机
```

---

## 4. SPI 的四根常见信号线

SPI 通常使用四根信号线：

```text
SCK
MOSI
MISO
CS
```

连接关系：

```text
SPI 主机                         SPI 从机
┌──────────┐                  ┌──────────┐
│          │──── SCK ────────>│          │
│          │──── MOSI ───────>│          │
│          │<─── MISO ────────│          │
│          │──── CS ─────────>│          │
└──────────┘                  └──────────┘
```

### SCK

SCK 表示：

```text
Serial Clock
串行时钟
```

通常由主机产生。

在常见的单线数据模式下，交换一个字节需要 8 个时钟周期。

### MOSI

MOSI 表示：

```text
Master Out Slave In
```

数据方向：

```text
主机 → 从机
```

主机发送命令、寄存器地址和配置数据时通常使用 MOSI。

### MISO

MISO 表示：

```text
Master In Slave Out
```

数据方向：

```text
从机 → 主机
```

从机返回设备 ID、状态和传感器数据时通常使用 MISO。

### CS

CS 表示：

```text
Chip Select
片选
```

也常写成：

```text
SS
NSS
```

多数 SPI 设备使用低电平有效片选：

```text
CS = 0：设备被选择
CS = 1：设备被释放
```

具体有效电平仍应以芯片数据手册为准。

---

## 5. 为什么需要 CS

一个 SPI 主机可以连接多个从机。

多个从机通常共用：

```text
SCK
MOSI
MISO
```

但每个从机通常拥有独立的 CS。

例如：

```text
CS_FLASH
CS_DISPLAY
CS_SENSOR
```

主机访问传感器时：

```text
CS_SENSOR = 0
CS_FLASH = 1
CS_DISPLAY = 1
```

这样只有被选中的传感器响应当前通信。

---

## 6. SPI 为什么是全双工

全双工表示：

```text
发送和接收可以同时进行
```

SPI 的一个时钟周期内：

```text
主机向从机移出一位数据
从机也向主机移出一位数据
```

完成 8 个时钟周期后，双方同时交换一个完整字节。

本次测试：

```text
主机发送：0xA5
从机返回：0x5A
```

这不是先完成发送，再单独开始接收，而是在同一组时钟周期内完成交换。

---

## 7. 主机只想读取时为什么仍要发送数据

SPI 时钟通常由主机产生。

如果主机不进行传输，就不会产生时钟，从机也无法将数据移出。

因此主机只想读取数据时，仍需发送一个占位字节，例如：

```text
0xFF
```

该字节通常称为：

```text
Dummy Byte
占位字节
```

本项目定义：

```c
#define SPI_DUMMY_BYTE 0xFFU
```

读取设备 ID 时：

```c
spi_transfer_byte(spi,
                  SPI_DUMMY_BYTE,
                  &device_id);
```

本次过程：

```text
主机发送 0xFF
主要作用：产生 8 个时钟

从机返回 0x42
实际含义：设备 ID
```

占位字节具体使用 `0x00` 还是 `0xFF`，应根据真实设备协议确定。

---

## 8. SPI 与 UART 的区别

| 对比项 | SPI | UART |
|---|---|---|
| 通信类型 | 同步 | 异步 |
| 时钟线 | 有 SCK | 无独立时钟线 |
| 常见信号 | SCK、MOSI、MISO、CS | TX、RX |
| 通信角色 | 主机、从机 | 通常点对点 |
| 设备选择 | 使用 CS | 通常没有片选 |
| 数据交换 | 通常全双工 | 通常全双工 |
| 常见用途 | 芯片间高速通信 | 调试、模块通信 |

SPI 依靠主机时钟同步。

UART 依靠双方预先约定的波特率通信。

---

## 9. SPI 模式

SPI 模式由两个参数决定：

```text
CPOL
CPHA
```

### CPOL

CPOL 表示：

```text
Clock Polarity
时钟极性
```

它决定 SCK 空闲时的电平。

```text
CPOL = 0：时钟空闲为低电平
CPOL = 1：时钟空闲为高电平
```

### CPHA

CPHA 表示：

```text
Clock Phase
时钟相位
```

它决定数据在第一个还是第二个有效时钟边沿采样。

四种组合：

| SPI 模式 | CPOL | CPHA |
|---|---:|---:|
| Mode 0 | 0 | 0 |
| Mode 1 | 0 | 1 |
| Mode 2 | 1 | 0 |
| Mode 3 | 1 | 1 |

本次使用：

```text
SPI Mode 0
```

---

## 10. 工程结构

Day23 工程结构：

```text
day23
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
│   ├── spi.h
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
│   ├── spi.c
│   ├── state_machine.c
│   ├── system.c
│   └── uart.c
└── build
    └── day23_test
```

Day23 主要新增：

```text
include/spi.h
src/spi.c
```

主要修改：

```text
src/main.c
README.md
Makefile
```

---

## 11. SPI 模式枚举

在 `spi.h` 中定义：

```c
typedef enum
{
    SPI_MODE_0 = 0,
    SPI_MODE_1,
    SPI_MODE_2,
    SPI_MODE_3
} SpiMode;
```

对应数值：

```text
SPI_MODE_0 = 0
SPI_MODE_1 = 1
SPI_MODE_2 = 2
SPI_MODE_3 = 3
```

相比直接传入数字：

```c
spi_init(&spi, 1000000U, 0);
```

使用枚举：

```c
spi_init(&spi,
         1000000U,
         SPI_MODE_0);
```

含义更加明确。

---

## 12. SPI 总线结构体

本次定义：

```c
typedef struct
{
    uint32_t clock_hz;
    SpiMode mode;
    uint8_t chip_selected;
    uint8_t slave_response;
    uint32_t transfer_count;
} SpiBus;
```

成员作用如下。

### clock_hz

```c
uint32_t clock_hz;
```

保存 SPI 时钟频率，单位为 Hz。

本次设置：

```text
1000000 Hz
```

也就是：

```text
1 MHz
```

### mode

```c
SpiMode mode;
```

保存当前 SPI 工作模式。

本次使用：

```text
SPI_MODE_0
```

### chip_selected

```c
uint8_t chip_selected;
```

表示设备是否已经被选择。

```text
0：设备未选择
1：设备已选择
```

需要区分：

```text
chip_selected = 1
```

表示逻辑上的“已选中”。

对于低电平有效 CS，它对应：

```text
CS 引脚电平 = 0
```

### slave_response

```c
uint8_t slave_response;
```

保存模拟从机下一次准备返回的数据。

真实 SPI 中，从机响应通常来自从机内部寄存器或测量结果。

### transfer_count

```c
uint32_t transfer_count;
```

记录成功完成的单字节交换次数。

失败传输不会增加该计数。

---

## 13. SPI 模式合法性检查

函数：

```c
int spi_mode_is_valid(SpiMode mode);
```

当前合法值为：

```text
0、1、2、3
```

核心代码：

```c
if((uint32_t)mode < SPI_MODE_COUNT)
{
    return 1;
}

return 0;
```

其中：

```c
#define SPI_MODE_COUNT 4U
```

强制传入：

```c
(SpiMode)5
```

会被判定为非法。

---

## 14. SPI 初始化

函数：

```c
int spi_init(SpiBus *spi,
             uint32_t clock_hz,
             SpiMode mode);
```

初始化时检查：

```text
spi 指针是否为空
clock_hz 是否为 0
mode 是否在 Mode 0～Mode 3 范围
```

初始化成功后：

```text
clock_hz = 1000000
mode = SPI_MODE_0
chip_selected = 0
slave_response = 0xFF
transfer_count = 0
```

调用示例：

```c
SpiBus spi;

spi_init(&spi,
         1000000U,
         SPI_MODE_0);
```

---

## 15. 为什么时钟不能为 0

SPI 是同步通信。

如果：

```text
clock_hz = 0
```

表示没有 SCK 时钟。

没有时钟时：

```text
主机无法移出数据
从机无法采样数据
从机也无法返回数据
```

所以：

```c
spi_init(&spi,
         0U,
         SPI_MODE_0);
```

会返回失败。

真实项目中还需要检查：

```text
主机外设允许的最高频率
从机芯片允许的最高频率
系统总线时钟
SPI 分频值
```

---

## 16. 选择从机

函数：

```c
int spi_select(SpiBus *spi);
```

成功后：

```c
spi->chip_selected = 1U;
```

表示：

```text
设备被选择
模拟 CS 拉低
```

正确通信顺序：

```text
初始化 SPI
    ↓
CS 拉低
    ↓
交换一个或多个字节
    ↓
CS 拉高
```

重复调用 `spi_select()` 会被拒绝。

---

## 17. 释放从机

函数：

```c
int spi_deselect(SpiBus *spi);
```

成功后：

```c
spi->chip_selected = 0U;
```

表示：

```text
设备被释放
模拟 CS 拉高
```

如果设备本来就没有被选择，再调用释放函数会返回失败。

---

## 18. 一次事务可以交换多个字节

SPI 事务不一定只有一个字节。

常见寄存器读取流程可能是：

```c
spi_select(&spi);

spi_transfer_byte(&spi,
                  command,
                  &rx_data);

spi_transfer_byte(&spi,
                  register_address,
                  &rx_data);

spi_transfer_byte(&spi,
                  SPI_DUMMY_BYTE,
                  &register_value);

spi_deselect(&spi);
```

对应：

```text
CS 拉低
    ↓
发送命令
    ↓
发送寄存器地址
    ↓
发送占位字节并读取寄存器值
    ↓
CS 拉高
```

很多真实设备要求整个命令过程中 CS 一直保持有效。

---

## 19. 设置模拟从机响应

函数：

```c
int spi_set_slave_response(
    SpiBus *spi,
    uint8_t response);
```

例如：

```c
spi_set_slave_response(&spi,
                       0x5AU);
```

表示从机下一次交换准备返回：

```text
0x5A
```

这个函数只用于软件模拟。

真实主机不能直接决定从机返回的数据。

---

## 20. 单字节全双工传输

函数：

```c
int spi_transfer_byte(
    SpiBus *spi,
    uint8_t tx_data,
    uint8_t *rx_data);
```

参数：

```text
spi
SPI 总线状态。

tx_data
主机通过 MOSI 发出的字节。

rx_data
用于保存主机通过 MISO 接收到的字节。
```

调用：

```c
uint8_t rx_data;

spi_transfer_byte(&spi,
                  0xA5U,
                  &rx_data);
```

当前软件模型完成：

```c
*rx_data = spi->slave_response;
spi->slave_response = SPI_DUMMY_BYTE;
spi->transfer_count++;
```

---

## 21. 为什么使用输出指针

SPI 传输函数需要返回两个结果：

```text
传输是否成功
接收到的数据
```

一个 C 函数只能直接返回一个值，因此设计为：

```text
函数返回值：表示成功或失败
rx_data 指针：保存接收结果
```

调用方式：

```c
if(spi_transfer_byte(&spi,
                     0xA5U,
                     &rx_data))
{
    /* 使用 rx_data */
}
```

---

## 22. 为什么传输前必须检查 CS

初始化后：

```text
chip_selected = 0
```

如果直接执行：

```c
spi_transfer_byte(&spi,
                  0x33U,
                  &rx_data);
```

函数会返回失败。

因为从机尚未被选择。

正确顺序：

```c
spi_select(&spi);

spi_transfer_byte(&spi,
                  0x33U,
                  &rx_data);

spi_deselect(&spi);
```

---

## 23. 正常全双工传输测试

本次测试：

```text
主机发送：0xA5
从机返回：0x5A
```

代码过程：

```c
spi_set_slave_response(spi,
                       0x5AU);

spi_select(spi);

spi_transfer_byte(spi,
                  0xA5U,
                  &rx_data);

spi_deselect(spi);
```

运行结果：

```text
[SPI] CS LOW - Device selected
[SPI TX] Master sent     : 0xA5
[SPI RX] Master received : 0x5A
[SPI] CS HIGH - Device released
[SPI COUNT] Successful transfers = 1
```

说明：

```text
片选顺序正确
发送字节正确
接收字节正确
成功计数正确
```

---

## 24. 占位字节读取测试

模拟设备 ID：

```text
0x42
```

从机准备返回：

```c
spi_set_slave_response(spi,
                       0x42U);
```

主机发送：

```c
SPI_DUMMY_BYTE
```

其值为：

```text
0xFF
```

运行结果：

```text
[SPI TX] Dummy byte       : 0xFF
[SPI RX] Device ID        : 0x42
```

说明主机通过发送占位字节产生了时钟，并成功读取从机数据。

第二次成功交换后：

```text
transfer_count = 2
```

---

## 25. 未选择从机时传输测试

测试中没有拉低 CS，直接传输：

```c
spi_transfer_byte(spi,
                  0x33U,
                  &rx_data);
```

运行结果：

```text
[EXPECTED ERROR] Transfer rejected because CS is HIGH
[CHECK] Transfer count unchanged: 2
```

说明：

```text
未片选传输被拒绝
失败传输没有增加成功计数
```

---

## 26. 重复片选测试

第一次调用：

```c
spi_select(spi);
```

成功。

第二次再次调用：

```c
spi_select(spi);
```

由于设备已经被选择，返回失败。

结果：

```text
[SPI] First select succeeded
[EXPECTED ERROR] Duplicate select rejected
```

这可以帮助发现程序中的错误调用顺序。

---

## 27. 重复释放测试

第一次调用：

```c
spi_deselect(spi);
```

成功。

第二次再次调用：

```c
spi_deselect(spi);
```

由于设备已经处于释放状态，返回失败。

结果：

```text
[SPI] First deselect succeeded
[EXPECTED ERROR] Duplicate deselect rejected
```

---

## 28. 无效配置测试

本次测试了三个非法情况。

### 时钟为 0

```c
spi_init(&test_spi,
         0U,
         SPI_MODE_0);
```

结果：

```text
[EXPECTED ERROR] Zero SPI clock rejected
```

### 模式非法

```c
spi_init(&test_spi,
         1000000U,
         (SpiMode)5);
```

结果：

```text
[EXPECTED ERROR] Invalid SPI mode rejected
```

### 空指针

```c
spi_init(NULL,
         1000000U,
         SPI_MODE_0);
```

结果：

```text
[EXPECTED ERROR] NULL SPI pointer rejected
```

---

## 29. 传输次数统计

`transfer_count` 只记录成功传输。

本次成功传输：

```text
测试一：0xA5 与 0x5A 交换
测试二：0xFF 与 0x42 交换
```

所以最终：

```text
Successful transfers = 2
```

以下操作不会增加计数：

```text
未选择从机时传输
重复片选
重复释放
无效初始化
```

---

## 30. 完整运行结果

```text
===== Day23 SPI Full-Duplex Test =====
[SPI CONFIG] Clock = 1000000 Hz
[SPI CONFIG] Mode = 0
[SPI CONFIG] Dummy byte = 0xFF
[SPI CONFIG] Initial CS state = HIGH / released

----- Test 1: Full-Duplex Transfer -----
[SPI] CS LOW - Device selected
[SPI TX] Master sent     : 0xA5
[SPI RX] Master received : 0x5A
[SPI] CS HIGH - Device released
[SPI COUNT] Successful transfers = 1

----- Test 2: Dummy Byte Read -----
[SPI] CS LOW - Device selected
[SPI TX] Dummy byte       : 0xFF
[SPI RX] Device ID        : 0x42
[SPI] CS HIGH - Device released
[SPI COUNT] Successful transfers = 2

----- Test 3: Transfer Without CS -----
[EXPECTED ERROR] Transfer rejected because CS is HIGH
[CHECK] Transfer count unchanged: 2

----- Test 4: Chip Select Sequence -----
[SPI] First select succeeded
[EXPECTED ERROR] Duplicate select rejected
[SPI] First deselect succeeded
[EXPECTED ERROR] Duplicate deselect rejected
[SPI COUNT] Successful transfers = 2

----- Test 5: Invalid Configuration -----
[EXPECTED ERROR] Zero SPI clock rejected
[EXPECTED ERROR] Invalid SPI mode rejected
[EXPECTED ERROR] NULL SPI pointer rejected

----- Final SPI Status -----
[SPI STATUS] Successful transfers = 2
[SPI STATUS] Device selected = NO

===== SPI Test Finished =====
```

编译过程没有出现：

```text
warning
error
```

---

## 31. 编译运行

进入 Day23：

```bash
cd /root/Embedded_14Days/day23
```

清理旧文件：

```bash
make clean
```

重新编译：

```bash
make
```

运行：

```bash
make run
```

筛选关键结果：

```bash
make run | grep -E \
"SPI CONFIG|SPI TX|SPI RX|SPI COUNT|SPI STATUS|EXPECTED"
```

---

## 32. 自动验证

验证正常全双工返回值：

```bash
make run | grep -c \
"Master received : 0x5A"
```

预期：

```text
1
```

验证设备 ID：

```bash
make run | grep -c \
"Device ID        : 0x42"
```

预期：

```text
1
```

验证预期错误数量：

```bash
make run | grep -c \
"\[EXPECTED ERROR\]"
```

预期：

```text
6
```

六个预期错误为：

```text
未片选传输
重复选择
重复释放
零时钟
无效模式
空指针
```

验证最终释放状态：

```bash
make run | grep -c \
"Device selected = NO"
```

预期：

```text
1
```

---

## 33. 本次遇到的错误

编写 `spi.h` 时，曾误将终端命令写进头文件：

```text
cat > include/spi.h <<'EOF'
```

编译器报错：

```text
expected '=', ',', ';', 'asm'
or '__attribute__' before '>' token
```

原因是：

```text
cat > include/spi.h <<'EOF'
```

属于 Bash 终端命令，不是 C 语言代码。

正确规则：

```text
cat > 文件 <<'EOF'
以及结尾的 EOF
只在终端中执行。

真正写入 .h 或 .c 文件的内容，
只能是两个 EOF 标记之间的代码。
```

`make clean` 只能删除构建产物，不能自动修复源文件内容。

修复源文件后重新执行：

```bash
make clean
make
make run
```

程序成功通过全部测试。

---

## 34. 当前实现的局限

本次只是 SPI 软件逻辑模拟，尚未涉及真实硬件中的：

```text
SPI 外设时钟使能
GPIO 复用配置
SCK 引脚配置
MOSI 引脚配置
MISO 引脚配置
CS GPIO 输出
主从模式配置
数据位宽配置
时钟分频
CPOL 和 CPHA 寄存器设置
发送缓冲区空标志
接收缓冲区非空标志
忙状态检查
SPI 中断
SPI DMA
真实移位寄存器
多从机片选管理
```

当前 `slave_response` 只是模拟从机下一次返回的数据。

后续将建立更接近真实设备的：

```text
寄存器地址
读命令
写命令
设备 ID
状态寄存器
温度高低字节
```

---

## 35. 今日核心代码

SPI 模式检查：

```c
(uint32_t)mode < SPI_MODE_COUNT
```

选择从机：

```c
spi->chip_selected = 1U;
```

释放从机：

```c
spi->chip_selected = 0U;
```

全双工交换：

```c
*rx_data = spi->slave_response;
```

恢复默认从机响应：

```c
spi->slave_response = SPI_DUMMY_BYTE;
```

成功传输计数：

```c
spi->transfer_count++;
```

---

## 36. 今日收获

通过 Day23 学习，掌握了：

1. SPI 的基本作用；
2. SPI 同步通信的含义；
3. SPI 主机和从机的职责；
4. SCK 的作用；
5. MOSI 的数据方向；
6. MISO 的数据方向；
7. CS 的片选作用；
8. 低电平有效片选；
9. 多个 SPI 从机的选择方式；
10. SPI 全双工通信的本质；
11. 单字节传输实际上是单字节交换；
12. 主机读取数据时仍需产生时钟；
13. 占位字节的作用；
14. SPI Mode 0～Mode 3；
15. CPOL 的基本含义；
16. CPHA 的基本含义；
17. 使用枚举表示 SPI 模式；
18. 使用结构体保存 SPI 总线状态；
19. 初始化 SPI 时检查参数；
20. 传输前检查片选状态；
21. 防止重复选择和重复释放；
22. 使用输出指针返回接收数据；
23. 只统计成功传输；
24. 检查零时钟、非法模式和空指针；
25. 理解软件模拟与真实硬件驱动的区别；
26. 定位终端命令误写入 C 文件的问题。

当前学习路线：

```text
Day15：GPIO 寄存器与位操作
Day16：UART 完整命令解析
Day17：Ring Buffer 与 UART 字节流
Day18：系统 Tick 与软件定时器
Day19：按键边沿检测与软件消抖
Day20：按键短按、长按与持续按住
Day21：CAN 数据帧、ID 过滤与解析
Day22：ADC 采样、电压换算与数字滤波
Day23：SPI 基础与单字节全双工通信
```

---

## 37. 下一步学习计划

Day24 继续学习：

```text
SPI 模式与寄存器读写协议
```

计划模拟一个 SPI 温度传感器：

```text
寄存器 0x00：设备 ID
寄存器 0x01：状态
寄存器 0x02：温度高字节
寄存器 0x03：温度低字节
```

主要学习：

```text
SPI 多字节事务
命令字节
寄存器地址
读写位
CS 在完整事务中保持有效
高低字节组合
设备 ID 验证
非法寄存器检查
```
