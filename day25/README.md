# Day25：I²C 基础通信、设备地址与 ACK/NACK

## 1. 学习目标

Day23 和 Day24 已经完成了 SPI 基础通信和 SPI 寄存器协议。

Day25 开始学习另一种常见的板级通信协议：I²C。

本次主要学习：

```text
SCL 与 SDA
开漏输出
上拉电阻
START 起始信号
STOP 停止信号
7 位设备地址
地址字节
读写方向位
ACK 应答
NACK 非应答
单字节写入
单字节读取
错误设备地址
错误通信顺序
通信状态和次数统计
```

本次使用 C 语言模拟一个地址为 `0x48` 的 I²C 从机。

模拟测试内容：

```text
设备地址：0x48
写入数据：0xA5
读取数据：0x5A
错误地址：0x50
I²C 时钟：100 kHz
```

最终运行结果：

```text
[FINAL] START count = 3
[FINAL] STOP count  = 3
[FINAL] ACK count   = 3
[FINAL] NACK count  = 2
[FINAL] Byte count  = 5
[FINAL] Bus started = NO
[FINAL] Addressed   = NO
[FINAL] Stored data = 0xA5
[FINAL CHECK] All I2C tests passed
```

---

## 2. 什么是 I²C

I²C 的英文全称是：

```text
Inter-Integrated Circuit
```

通常读作：

```text
I Square C
```

I²C 是一种同步串行通信协议，常用于连接：

```text
温度传感器
湿度传感器
EEPROM
RTC 实时时钟
电流检测芯片
电压检测芯片
OLED 显示屏
加速度计
陀螺仪
GPIO 扩展芯片
```

I²C 的主要特点：

```text
同步通信
串行通信
两根信号线
支持多个从机
通过地址选择设备
具有 ACK/NACK 应答机制
通常采用半双工通信
```

---

## 3. I²C 的两根信号线

I²C 主要使用两根信号线：

```text
SCL
SDA
```

### SCL

SCL 全称：

```text
Serial Clock
```

中文为：

```text
串行时钟线
```

SCL 通常由主机产生，用于确定：

```text
数据什么时候变化
数据什么时候被采样
每一位数据的传输节奏
```

### SDA

SDA 全称：

```text
Serial Data
```

中文为：

```text
串行数据线
```

SDA 是双向数据线，可以传输：

```text
主机发送给从机的数据
从机发送给主机的数据
```

由于发送和接收共用一根 SDA，因此 I²C 通常称为半双工通信。

---

## 4. 为什么 I²C 需要上拉电阻

I²C 的 SCL 和 SDA 通常采用开漏输出。

开漏输出不能主动输出高电平，只能：

```text
主动把总线拉低
释放总线
```

当所有设备都释放总线时，需要外部上拉电阻将线路拉到高电平。

连接关系可以理解为：

```text
              VCC
               |
            上拉电阻
               |
主机 SDA ------+------ 从机 SDA
```

SCL 也采用类似连接。

因此：

```text
设备拉低线路：逻辑 0
设备释放线路：由上拉电阻形成逻辑 1
```

---

## 5. 什么是开漏输出

可以把开漏引脚理解为一个连接到地的开关。

### 开关闭合

```text
线路接地
总线为低电平
```

### 开关断开

```text
设备释放线路
上拉电阻将线路拉到高电平
```

所以在 I²C 中：

```text
输出 0：主动拉低总线
输出 1：释放总线
```

这里的输出 1 并不是普通推挽 GPIO 主动输出高电平。

---

## 6. 为什么多个设备可以共用 SDA

所有设备都使用开漏输出时：

```text
只要有一个设备拉低 SDA
总线就是低电平
```

只有当所有设备都释放 SDA 时：

```text
总线才会被上拉电阻拉高
```

这种电气关系常称为：

```text
Wired-AND
线与
```

它能够避免多个设备同时驱动 SDA 时产生高低电平直接冲突。

---

## 7. I²C 总线空闲状态

I²C 总线空闲时：

```text
SCL = HIGH
SDA = HIGH
```

因为没有任何设备主动拉低线路，上拉电阻会把两根信号线保持在高电平。

Day25 软件模型中使用：

```c
bus.started = 0U;
```

表示当前没有事务正在进行。

---

## 8. START 起始信号

I²C 通信开始前，主机需要产生 START。

START 的定义：

```text
SCL 保持高电平时
SDA 从高电平变为低电平
```

简化时序：

```text
SCL：──────── HIGH ────────

SDA：──── HIGH ──┐
                 └── LOW ──
                 ↑
               START
```

START 表示：

```text
一段新的 I²C 通信即将开始
接下来主机会发送设备地址
```

程序接口：

```c
int i2c_start(I2cBus *bus);
```

成功后：

```text
started = 1
addressed = 0
start_count + 1
```

Day25 暂时不支持 Repeated START。

---

## 9. STOP 停止信号

通信结束后，主机需要产生 STOP。

STOP 的定义：

```text
SCL 保持高电平时
SDA 从低电平变为高电平
```

简化时序：

```text
SCL：──────── HIGH ────────

SDA：──── LOW ───┐
                 └── HIGH ─
                 ↑
                STOP
```

STOP 表示：

```text
当前 I²C 事务结束
总线重新回到空闲状态
```

程序接口：

```c
int i2c_stop(I2cBus *bus);
```

成功后：

```text
started = 0
addressed = 0
stop_count + 1
```

---

## 10. 数据传输和 START/STOP 的区别

普通数据传输时通常要求：

```text
SCL 为低电平时改变 SDA
SCL 为高电平时采样 SDA
```

START 和 STOP 是特殊情况。

它们都是在：

```text
SCL 为高电平时
```

由 SDA 的变化产生。

判断方式：

```text
SDA 高到低：START
SDA 低到高：STOP
```

---

## 11. I²C 使用地址选择设备

SPI 通常使用不同的 CS 片选线选择设备：

```text
CS_FLASH
CS_SENSOR
CS_DISPLAY
```

I²C 主要使用设备地址选择从机。

例如：

```text
温度传感器：0x48
EEPROM：0x50
RTC：0x68
```

多个设备可以共用：

```text
SCL
SDA
```

主机发送地址后，只有地址匹配的从机返回 ACK。

最重要的区别：

```text
SPI 使用 CS 选择设备
I²C 使用设备地址选择设备
```

---

## 12. 7 位设备地址

常见 I²C 设备使用 7 位地址。

Day25 模拟设备地址：

```c
#define I2C_DEVICE_DEFAULT_ADDRESS 0x48U
```

`0x48` 的二进制形式：

```text
100 1000
```

7 位地址的理论数值范围：

```text
0x00～0x7F
```

定义：

```c
#define I2C_7BIT_ADDRESS_MAX 0x7FU
```

Day25 只检查地址是否位于 7 位范围内。

真实 I²C 规范中还有部分保留地址，使用真实芯片时需要查看数据手册。

---

## 13. 地址字节的组成

I²C 设备地址本身是 7 位，但总线上发送的是一个 8 位地址字节。

地址字节由以下部分组成：

```text
bit7～bit1：7 位设备地址
bit0：读写方向位
```

结构：

```text
bit7 bit6 bit5 bit4 bit3 bit2 bit1 bit0
 └──────── 7 位地址 ─────────┘  R/W
```

生成公式：

```text
地址字节 = 7 位地址左移 1 位 | 读写方向
```

C 语言实现：

```c
*address_byte =
    (uint8_t)(
        (address << 1) |
        direction);
```

---

## 14. 读写方向位

地址字节最低位 bit0 表示通信方向：

```text
bit0 = 0：主机写入从机
bit0 = 1：主机读取从机
```

程序使用枚举表示：

```c
typedef enum
{
    I2C_DIRECTION_WRITE = 0,
    I2C_DIRECTION_READ = 1
} I2cDirection;
```

枚举值与地址字节最低位直接对应。

---

## 15. 地址 0x48 的写地址字节

设备地址：

```text
0x48
```

左移一位：

```text
0x48 << 1 = 0x90
```

写方向：

```text
WRITE = 0
```

所以：

```text
0x90 | 0 = 0x90
```

最终写地址字节：

```text
0x90
```

表示：

```text
访问地址为 0x48 的设备
主机准备向从机写数据
```

---

## 16. 地址 0x48 的读地址字节

设备地址：

```text
0x48
```

左移一位：

```text
0x48 << 1 = 0x90
```

读方向：

```text
READ = 1
```

所以：

```text
0x90 | 1 = 0x91
```

最终读地址字节：

```text
0x91
```

表示：

```text
访问地址为 0x48 的设备
主机准备读取从机数据
```

---

## 17. 0x48、0x90 和 0x91 的区别

同一个 I²C 设备可能在资料中看到：

```text
0x48
0x90
0x91
```

它们的含义分别是：

```text
0x48：真正的 7 位设备地址
0x90：设备地址 0x48 对应的写地址字节
0x91：设备地址 0x48 对应的读地址字节
```

程序中通常保存：

```c
#define DEVICE_ADDRESS 0x48U
```

发送时再根据方向生成：

```c
(device_address << 1) | direction
```

不能把 `0x90` 当成 7 位地址再次左移。

---

## 18. 什么是 ACK

ACK 全称：

```text
Acknowledge
```

中文含义：

```text
应答
```

I²C 每传输完 8 位数据后，第 9 个时钟用于应答。

ACK 时：

```text
接收方主动把 SDA 拉低
```

所以：

```text
SDA = 0
```

表示：

```text
当前字节已经成功接收
可以继续通信
```

程序枚举：

```c
typedef enum
{
    I2C_RESPONSE_ACK = 0,
    I2C_RESPONSE_NACK = 1
} I2cResponse;
```

---

## 19. 什么是 NACK

NACK 全称：

```text
Not Acknowledge
```

中文含义：

```text
非应答
```

NACK 时：

```text
接收方没有拉低 SDA
SDA 保持高电平
```

所以：

```text
SDA = 1
```

NACK 可能表示：

```text
没有设备响应该地址
从机拒绝当前数据
主机不再继续读取
通信发生异常
```

NACK 不一定代表错误，需要结合它出现的位置判断。

---

## 20. 地址阶段的 ACK/NACK

主机发送设备地址后：

```text
地址匹配的从机负责发送 ACK
```

例如：

```text
主机发送地址：0x48
从机地址：0x48
结果：ACK
```

如果主机访问：

```text
0x50
```

但总线上只有地址为：

```text
0x48
```

的从机，则没有设备响应，结果为：

```text
NACK
```

---

## 21. 写数据时的 ACK

主机向从机写数据时：

```text
主机是发送方
从机是接收方
```

从机成功接收一个字节后发送 ACK。

例如：

```text
主机发送 0xA5
从机接收成功
从机返回 ACK
```

---

## 22. 读取数据时的 ACK/NACK

主机读取数据时：

```text
从机是发送方
主机是接收方
```

所以数据字节结束后，由主机发送 ACK 或 NACK。

规则：

```text
还需要继续读取后续字节：
主机发送 ACK

当前已经是最后一个字节：
主机发送 NACK
```

Day25 是单字节读取，因此读取 `0x5A` 后主机发送 NACK。

此处 NACK 表示：

```text
当前字节已经接收
本次读取结束
```

不是通信失败。

---

## 23. Day25 工程结构

```text
day25
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
│   ├── i2c.h
│   ├── ring_buffer.h
│   ├── sensor.h
│   ├── software_timer.h
│   ├── spi.h
│   ├── spi_device.h
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
│   ├── i2c.c
│   ├── main.c
│   ├── ring_buffer.c
│   ├── sensor.c
│   ├── software_timer.c
│   ├── spi.c
│   ├── spi_device.c
│   ├── state_machine.c
│   ├── system.c
│   └── uart.c
└── build
    └── day25_test
```

Day25 新增：

```text
include/i2c.h
src/i2c.c
```

主要修改：

```text
src/main.c
README.md
Makefile
```

---

## 24. 软件分层

Day25 暂时使用两层结构：

```text
main.c
应用测试层
        ↓
i2c.c
I²C 总线和模拟设备层
```

### main.c

负责：

```text
初始化总线和设备
生成地址字节
执行单字节写入
执行单字节读取
测试错误地址
测试错误调用顺序
检查最终统计结果
```

### i2c.c

负责：

```text
地址合法性检查
方向合法性检查
ACK/NACK 参数检查
地址字节生成
START
STOP
地址匹配
单字节写入
单字节读取
状态更新
通信次数统计
```

Day26 将进一步拆分为：

```text
main.c
        ↓
i2c_device.c
        ↓
i2c.c
```

---

## 25. I²C 总线结构体

定义：

```c
typedef struct
{
    uint32_t clock_hz;

    uint8_t started;
    uint8_t addressed;
    uint8_t current_address;

    I2cDirection direction;

    uint32_t start_count;
    uint32_t stop_count;
    uint32_t ack_count;
    uint32_t nack_count;
    uint32_t byte_count;
} I2cBus;
```

---

## 26. clock_hz

```c
uint32_t clock_hz;
```

保存 I²C 时钟频率。

Day25 使用：

```c
#define I2C_STANDARD_CLOCK_HZ 100000U
```

即：

```text
100000 Hz = 100 kHz
```

---

## 27. started

```c
uint8_t started;
```

表示是否已经产生 START。

```text
0：总线空闲
1：事务已经开始
```

初始化后：

```text
started = 0
```

调用 `i2c_start()` 后：

```text
started = 1
```

调用 `i2c_stop()` 后：

```text
started = 0
```

---

## 28. addressed

```c
uint8_t addressed;
```

表示是否已有设备成功响应当前地址。

```text
0：没有设备匹配
1：设备已经返回 ACK
```

只有：

```text
started = 1
addressed = 1
```

时，才允许继续进行数据读写。

---

## 29. current_address

```c
uint8_t current_address;
```

保存当前主机发送的 7 位地址。

例如：

```text
current_address = 0x48
```

这里保存的是 7 位地址，而不是：

```text
0x90
0x91
```

---

## 30. direction

```c
I2cDirection direction;
```

保存当前事务方向：

```text
I2C_DIRECTION_WRITE
I2C_DIRECTION_READ
```

当前为 WRITE 时，只允许调用：

```c
i2c_write_byte()
```

当前为 READ 时，只允许调用：

```c
i2c_read_byte()
```

---

## 31. I²C 统计成员

### start_count

记录成功产生 START 的次数。

### stop_count

记录成功产生 STOP 的次数。

### ack_count

记录出现 ACK 的次数。

### nack_count

记录出现 NACK 的次数。

### byte_count

记录完成的 8 位字段数量，包括：

```text
地址字节
数据字节
```

ACK/NACK 使用第 9 个时钟，不算新的数据字节，因此不计入 `byte_count`。

---

## 32. 模拟从机结构体

定义：

```c
typedef struct
{
    uint8_t address;
    uint8_t stored_data;
    uint8_t read_data;
} I2cDevice;
```

### address

保存设备的 7 位地址：

```text
0x48
```

### stored_data

保存主机成功写入的数据。

初始化值：

```text
0x00
```

写入成功后：

```text
0xA5
```

### read_data

保存从机准备返回的数据。

Day25 默认：

```text
0x5A
```

---

## 33. 为什么 stored_data 和 read_data 分开

主机写入设备的数据不一定等于从机返回的数据。

Day25 规定：

```text
主机写入：0xA5
从机读取返回：0x5A
```

因此分别使用：

```text
stored_data
read_data
```

保存。

Day26 会改成设备寄存器数组。

---

## 34. 地址字节生成函数

接口：

```c
int i2c_build_address_byte(
    uint8_t address,
    I2cDirection direction,
    uint8_t *address_byte);
```

核心代码：

```c
*address_byte =
    (uint8_t)(
        (uint8_t)(
            address << I2C_ADDRESS_SHIFT) |
        ((uint8_t)direction &
         I2C_RW_MASK));
```

地址 `0x48` 加写方向：

```text
(0x48 << 1) | 0 = 0x90
```

地址 `0x48` 加读方向：

```text
(0x48 << 1) | 1 = 0x91
```

---

## 35. 总线初始化函数

接口：

```c
int i2c_init(
    I2cBus *bus,
    uint32_t clock_hz);
```

初始化内容：

```text
保存时钟频率
总线设置为空闲
清除设备匹配状态
清除当前地址
方向设置为 WRITE
所有统计次数清零
```

初始化后：

```text
clock_hz = 100000
started = 0
addressed = 0
current_address = 0
start_count = 0
stop_count = 0
ack_count = 0
nack_count = 0
byte_count = 0
```

以下参数会被拒绝：

```text
bus = NULL
clock_hz = 0
```

---

## 36. 从机初始化函数

接口：

```c
int i2c_device_init(
    I2cDevice *device,
    uint8_t address,
    uint8_t read_data);
```

初始化后：

```text
address = 0x48
stored_data = 0x00
read_data = 0x5A
```

以下情况会失败：

```text
device = NULL
地址超出 7 位范围
```

---

## 37. START 函数

接口：

```c
int i2c_start(I2cBus *bus);
```

成功后：

```text
started = 1
addressed = 0
current_address = 0
start_count + 1
```

Day25 暂时不支持重复 START。

因此：

```c
i2c_start(&bus);
i2c_start(&bus);
```

第二次调用会失败。

---

## 38. STOP 函数

接口：

```c
int i2c_stop(I2cBus *bus);
```

成功后：

```text
started = 0
addressed = 0
current_address = 0
stop_count + 1
```

没有 START 时直接调用 STOP，会返回失败。

---

## 39. 发送地址函数

接口：

```c
int i2c_send_address(
    I2cBus *bus,
    const I2cDevice *device,
    uint8_t address,
    I2cDirection direction,
    I2cResponse *response);
```

函数执行：

```text
检查参数
检查是否已经 START
检查地址和方向
生成地址字节
增加 byte_count
保存当前地址和方向
比较主机地址与从机地址
返回 ACK 或 NACK
```

---

## 40. 函数返回值和 ACK/NACK 的区别

`i2c_send_address()` 有两种结果。

### 函数返回值

表示 C 函数调用是否合法：

```text
参数是否正确
是否已经 START
地址是否有效
方向是否有效
```

### response

表示协议层响应：

```text
ACK
NACK
```

例如地址不匹配：

```text
函数正常执行
地址字节已经发送
从机没有响应
response = NACK
函数仍返回 1
```

所以：

```text
NACK 不等于 C 函数执行失败
```

---

## 41. 单字节写入函数

接口：

```c
int i2c_write_byte(
    I2cBus *bus,
    I2cDevice *device,
    uint8_t data,
    I2cResponse *response);
```

正确条件：

```text
已经 START
地址匹配成功
当前方向为 WRITE
传入设备地址正确
```

成功后：

```text
device->stored_data = data
byte_count + 1
ack_count + 1
response = ACK
```

错误情况：

```text
没有 START
没有地址 ACK
当前方向为 READ
传入了错误设备对象
```

---

## 42. 单字节读取函数

接口：

```c
int i2c_read_byte(
    I2cBus *bus,
    const I2cDevice *device,
    uint8_t *data,
    I2cResponse master_response);
```

正确条件：

```text
已经 START
地址匹配成功
当前方向为 READ
设备对象正确
ACK/NACK 参数合法
```

成功后：

```text
*data = device->read_data
byte_count + 1
```

主机发送 ACK 时：

```text
ack_count + 1
```

主机发送 NACK 时：

```text
nack_count + 1
```

Day25 单字节读取使用：

```c
I2C_RESPONSE_NACK
```

表示不再继续读取。

---

## 43. 单字节写入流程

完整事务：

```text
START
    ↓
发送地址 0x48 + WRITE
    ↓
从机 ACK
    ↓
主机发送 0xA5
    ↓
从机 ACK
    ↓
STOP
```

总线上地址字节：

```text
0x90
```

程序调用顺序：

```c
i2c_start(&bus);

i2c_send_address(
    &bus,
    &device,
    0x48U,
    I2C_DIRECTION_WRITE,
    &response);

i2c_write_byte(
    &bus,
    &device,
    0xA5U,
    &response);

i2c_stop(&bus);
```

写入成功后：

```text
stored_data = 0xA5
```

---

## 44. 单字节读取流程

完整事务：

```text
START
    ↓
发送地址 0x48 + READ
    ↓
从机 ACK
    ↓
从机返回 0x5A
    ↓
主机发送 NACK
    ↓
STOP
```

总线上地址字节：

```text
0x91
```

程序调用顺序：

```c
i2c_start(&bus);

i2c_send_address(
    &bus,
    &device,
    0x48U,
    I2C_DIRECTION_READ,
    &response);

i2c_read_byte(
    &bus,
    &device,
    &read_data,
    I2C_RESPONSE_NACK);

i2c_stop(&bus);
```

读取结果：

```text
read_data = 0x5A
```

---

## 45. 错误地址测试

模拟设备地址：

```text
0x48
```

主机故意访问：

```text
0x50
```

写地址字节：

```text
0x50 << 1 = 0xA0
```

由于没有地址为 `0x50` 的设备响应，所以得到：

```text
NACK
```

地址没有匹配时继续写数据会被拒绝。

原来存储的数据仍保持：

```text
0xA5
```

---

## 46. 错误调用顺序测试

Day25 测试了以下错误：

```text
没有 START 就发送地址
没有 START 就产生 STOP
事务中重复 START
未发送地址就写数据
READ 方向调用写函数
WRITE 方向调用读函数
```

这些错误均被正确拒绝。

使用了独立临时总线：

```c
I2cBus test_bus;
```

因此错误顺序测试不会影响主总线最终计数。

---

## 47. 无效参数测试

Day25 还测试了：

```text
NULL 总线指针
时钟频率为 0
NULL 设备指针
非法方向值
NULL 地址字节输出指针
```

这些属于函数参数错误，函数返回 0。

它们与协议层 NACK 不同。

---

## 48. 实际运行结果

```text
===== Day25 I2C Basic Communication Test =====
[I2C CONFIG] Clock = 100000 Hz
[DEVICE CONFIG] 7-bit address = 0x48
[DEVICE CONFIG] Read data = 0x5A

----- Test 1: Address Byte Build -----
[ADDRESS] 7-bit device address = 0x48
[ADDRESS] Write address byte   = 0x90
[ADDRESS] Read address byte    = 0x91
[CHECK] Address bytes are correct
[CHECK] Write byte bit0 = 0
[CHECK] Read byte bit0 = 1

----- Test 2: Single Byte Write -----
[I2C] START
[I2C] Address byte 0x90 -> ACK
[I2C] Write data 0xA5 -> ACK
[I2C] STOP
[DEVICE] Stored data = 0xA5
[CHECK] Single byte write verified

----- Test 3: Single Byte Read -----
[I2C] START
[I2C] Address byte 0x91 -> ACK
[I2C] Read data 0x5A
[I2C] Master response -> NACK
[I2C] STOP
[CHECK] Single byte read verified

----- Test 4: Wrong Device Address -----
[I2C] START
[I2C] Address 0x50 (byte 0xA0) -> NACK
[EXPECTED] Wrong address was not acknowledged
[EXPECTED ERROR] Data write rejected after address NACK
[I2C] STOP
[CHECK] Stored data unchanged: 0xA5

----- Test 5: Invalid Sequences -----
[EXPECTED ERROR] Address rejected before START
[EXPECTED ERROR] STOP rejected before START
[I2C TEST] First START succeeded
[EXPECTED ERROR] Repeated START rejected
[EXPECTED ERROR] Write rejected before address
[EXPECTED ERROR] Write rejected in READ direction
[EXPECTED ERROR] Read rejected in WRITE direction
[CHECK] Invalid sequence test completed

----- Test 6: Invalid Parameters -----
[EXPECTED ERROR] NULL bus rejected
[EXPECTED ERROR] Zero clock rejected
[EXPECTED ERROR] NULL device rejected
[EXPECTED ERROR] Invalid direction rejected
[EXPECTED ERROR] NULL address output rejected

----- Final I2C Status -----
[FINAL] START count = 3
[FINAL] STOP count  = 3
[FINAL] ACK count   = 3
[FINAL] NACK count  = 2
[FINAL] Byte count  = 5
[FINAL] Bus started = NO
[FINAL] Addressed   = NO
[FINAL] Stored data = 0xA5
[FINAL CHECK] All I2C tests passed

===== I2C Test Finished =====
```

编译过程没有：

```text
warning
error
```

---

## 49. 最终计数分析

主总线完成了三个事务。

### 单字节写入

```text
START：1
STOP：1
地址字节：1
数据字节：1
ACK：2
NACK：0
```

### 单字节读取

```text
START：1
STOP：1
地址字节：1
数据字节：1
ACK：1
NACK：1
```

其中 NACK 是主机结束单字节读取。

### 错误地址访问

```text
START：1
STOP：1
地址字节：1
ACK：0
NACK：1
```

最终：

```text
START = 3
STOP  = 3
ACK   = 3
NACK  = 2
BYTE  = 5
```

---

## 50. 为什么 Byte count 是 5

单字节写入：

```text
地址字节 0x90
数据字节 0xA5
共 2 个字节
```

单字节读取：

```text
地址字节 0x91
数据字节 0x5A
共 2 个字节
```

错误地址：

```text
地址字节 0xA0
共 1 个字节
```

因此：

```text
2 + 2 + 1 = 5
```

ACK 和 NACK 是第 9 个时钟，不是独立的 8 位数据字节，所以不计入 `byte_count`。

---

## 51. 为什么 ACK count 是 3

单字节写入：

```text
地址 ACK：1
数据 ACK：1
```

单字节读取：

```text
地址 ACK：1
```

错误地址没有 ACK。

总计：

```text
2 + 1 = 3
```

---

## 52. 为什么 NACK count 是 2

第一个 NACK：

```text
单字节读取结束
主机发送 NACK
```

第二个 NACK：

```text
访问错误地址 0x50
从机没有响应
```

因此：

```text
NACK count = 2
```

两个 NACK 的含义不同：

```text
读取结束 NACK：正常协议行为
错误地址 NACK：没有设备响应
```

---

## 53. 编译运行

进入 Day25：

```bash
cd /root/Embedded_14Days/day25
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

筛选关键输出：

```bash
make run | grep -E \
"ADDRESS|START|STOP|ACK|NACK|Stored data|FINAL"
```

---

## 54. 自动验证

检查最终测试：

```bash
make run | grep -c \
"All I2C tests passed"
```

预期：

```text
1
```

检查写地址字节：

```bash
make run | grep -c \
"Write address byte   = 0x90"
```

预期：

```text
1
```

检查读地址字节：

```bash
make run | grep -c \
"Read address byte    = 0x91"
```

预期：

```text
1
```

检查读取数据：

```bash
make run | grep -c \
"Read data 0x5A"
```

预期：

```text
1
```

检查最终写入数据：

```bash
make run | grep -c \
"Stored data = 0xA5"
```

结果可能大于 1，因为测试过程和最终状态都打印了该数据。

---

## 55. 本次核心代码

生成地址字节：

```c
(address << 1) | direction
```

写方向：

```c
I2C_DIRECTION_WRITE
```

读方向：

```c
I2C_DIRECTION_READ
```

ACK：

```c
I2C_RESPONSE_ACK
```

NACK：

```c
I2C_RESPONSE_NACK
```

检查总线是否已经开始：

```c
bus->started != 0U
```

检查设备是否已经匹配：

```c
bus->addressed != 0U
```

检查当前方向：

```c
bus->direction == I2C_DIRECTION_WRITE
```

或者：

```c
bus->direction == I2C_DIRECTION_READ
```

---

## 56. SPI 和 I²C 对比

| 对比项 | SPI | I²C |
|---|---|---|
| 信号线 | SCK、MOSI、MISO、CS | SCL、SDA |
| 设备选择 | CS 片选 | 设备地址 |
| 数据方向 | 发送和接收分开 | SDA 双向 |
| 通信方式 | 通常全双工 | 通常半双工 |
| 应答机制 | 由具体设备协议决定 | 标准 ACK/NACK |
| 上拉电阻 | 通常不需要 | 通常需要 |
| 多设备连接 | 每个设备通常需要独立 CS | 多个设备共用 SCL/SDA |
| 协议流程 | 相对直接 | START、地址、ACK、STOP |
| 速度 | 通常更高 | 通常较低 |

核心区别：

```text
SPI 使用 CS 选择从机
I²C 使用地址选择从机
```

---

## 57. 当前实现的局限

Day25 仍然是软件模拟，尚未逐位模拟：

```text
真实 SCL 电平
真实 SDA 电平
GPIO 开漏模式
外部上拉电阻
时钟高低电平延时
SDA 数据建立时间
SDA 数据保持时间
时钟拉伸
总线仲裁
总线恢复
Repeated START
真实 I²C 外设寄存器
中断通信
DMA 通信
超时处理
多从机链表
10 位设备地址
```

当前模拟重点是理解：

```text
I²C 通信状态
地址字节
ACK/NACK
正确调用顺序
```

---

## 58. 今日收获

通过 Day25 学习，掌握了：

1. I²C 的基本概念；
2. SCL 和 SDA 的作用；
3. 开漏输出；
4. 上拉电阻；
5. 线与关系；
6. 总线空闲状态；
7. START 起始信号；
8. STOP 停止信号；
9. 7 位设备地址；
10. 地址字节的组成；
11. 地址左移一位的原因；
12. 读写方向位；
13. `0x48`、`0x90` 和 `0x91` 的区别；
14. ACK 的含义；
15. NACK 的含义；
16. 地址阶段的应答；
17. 写数据阶段的应答；
18. 读数据结束时的 NACK；
19. 单字节写入流程；
20. 单字节读取流程；
21. 错误设备地址处理；
22. 错误通信顺序检查；
23. 函数失败和协议 NACK 的区别；
24. START、STOP、ACK、NACK 计数；
25. 地址字节和数据字节计数；
26. I²C 总线状态结构体；
27. 简单 I²C 从机模拟；
28. SPI 与 I²C 的主要区别。

当前学习路线：

```text
Day15：GPIO 寄存器与位操作
Day16：UART 命令解析
Day17：Ring Buffer 与 UART 字节流
Day18：系统 Tick 与软件定时器
Day19：按键边沿检测与软件消抖
Day20：按键短按、长按与持续按住
Day21：CAN 数据帧、ID 过滤与解析
Day22：ADC 采样、电压换算与数字滤波
Day23：SPI 基础与单字节全双工通信
Day24：SPI 寄存器读写协议与多字节事务
Day25：I²C 基础通信、设备地址与 ACK/NACK
```

---

## 59. 下一步学习计划

Day26 将继续学习：

```text
I²C 寄存器读写协议
```

计划加入：

```text
I²C 设备寄存器数组
寄存器地址
配置寄存器写入
设备 ID 读取
温度高低字节
Repeated START
连续多字节读取
读写权限检查
非法寄存器地址
事务失败恢复
```

典型寄存器读取流程：

```text
START
设备地址 + WRITE
寄存器地址
Repeated START
设备地址 + READ
读取寄存器数据
NACK
STOP
```

Day26 的重点是理解为什么读取 I²C 寄存器通常需要：

```text
先写寄存器地址
再使用 Repeated START 切换到读取方向
```

---

## 60. 项目仓库

项目地址：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects
```

Day25 目录：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects/tree/main/day25
```