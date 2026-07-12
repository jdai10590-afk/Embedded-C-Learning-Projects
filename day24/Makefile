# Day24：SPI 寄存器读写协议与多字节事务

## 1. 学习目标

Day23 已经完成了 SPI 单字节全双工通信模型，包括：

```text
SPI 初始化
CS 片选控制
单字节发送与接收
Dummy Byte
非法参数检查
传输次数统计
```

Day24 在此基础上继续模拟一个带内部寄存器的 SPI 温度传感器，重点学习：

```text
SPI 寄存器读写协议
读写命令编码
多字节 SPI 事务
寄存器地址
只读与可写寄存器
温度高低字节拆分
温度高低字节组合
设备 ID 校验
非法寄存器保护
SPI 总线忙状态检查
读写次数与字节交换次数统计
```

最终测试结果：

```text
设备 ID读取成功：0x42
状态寄存器读取成功：0x01
配置寄存器初始值：0x00
温度原始值：2534
温度值：25.34℃
配置寄存器成功写入：0xA5
非法寄存器访问被拒绝
只读寄存器写入被拒绝
SPI 总线忙时新事务被拒绝
全部协议测试通过
```

---

## 2. 什么是 SPI 寄存器协议

真实 SPI 设备通常包含多个内部寄存器，例如：

```text
设备 ID
工作状态
配置参数
传感器测量值
故障状态
中断标志
```

主机不能只发送一个固定字节，而是需要通过：

```text
命令字节
寄存器地址
数据字节
```

指定访问哪个寄存器，以及执行读取还是写入。

Day24 模拟的设备内部寄存器如下：

| 地址 | 名称 | 功能 | 权限 |
|---|---|---|---|
| `0x00` | DEVICE_ID | 设备 ID | 只读 |
| `0x01` | STATUS | 状态寄存器 | 只读 |
| `0x02` | TEMP_HIGH | 温度高字节 | 只读 |
| `0x03` | TEMP_LOW | 温度低字节 | 只读 |
| `0x04` | CONFIG | 配置寄存器 | 可读写 |

---

## 3. 设备地址和寄存器地址的区别

SPI 通信中需要区分：

```text
设备选择
寄存器选择
```

设备选择由 CS 完成。

例如：

```text
CS_SENSOR = 0
```

表示选择温度传感器。

寄存器地址用于选择设备内部的某个数据。

例如：

```text
0x00：设备 ID
0x02：温度高字节
0x04：配置寄存器
```

因此：

```text
CS 决定访问哪个设备
寄存器地址决定访问设备内部哪个数据
```

---

## 4. 本次 SPI 命令格式

Day24 规定一个命令字节由以下部分组成：

```text
bit7：读写标志
bit6～bit0：寄存器地址
```

定义：

```c
#define SPI_DEVICE_READ_FLAG    0x80U
#define SPI_DEVICE_ADDRESS_MASK 0x7FU
```

二进制：

```text
0x80 = 1000 0000
0x7F = 0111 1111
```

### 读命令

```text
bit7 = 1
```

构造方式：

```c
command =
    register_address |
    SPI_DEVICE_READ_FLAG;
```

例如读取寄存器 `0x02`：

```text
0x02 | 0x80 = 0x82
```

因此：

```text
0x82
```

表示读取寄存器 `0x02`。

### 写命令

```text
bit7 = 0
```

构造方式：

```c
command =
    register_address &
    SPI_DEVICE_ADDRESS_MASK;
```

例如写入寄存器 `0x04`：

```text
0x04 & 0x7F = 0x04
```

因此：

```text
0x04
```

表示写入配置寄存器。

---

## 5. 为什么使用按位或设置读标志

寄存器地址：

```text
0x02 = 0000 0010
```

读标志：

```text
0x80 = 1000 0000
```

执行按位或：

```text
0000 0010
1000 0000
───────── OR
1000 0010
```

得到：

```text
0x82
```

表示：

```text
读取寄存器 0x02
```

---

## 6. 为什么使用掩码提取寄存器地址

从机收到命令：

```text
0x82
```

其中：

```text
bit7 = 1
表示读操作

bit6～bit0 = 0x02
表示寄存器地址
```

使用：

```c
address =
    command &
    SPI_DEVICE_ADDRESS_MASK;
```

计算：

```text
1000 0010
0111 1111
───────── AND
0000 0010
```

最终恢复寄存器地址：

```text
0x02
```

---

## 7. SPI 寄存器读取事务

本次读取一个寄存器需要两个字节交换。

完整流程：

```text
CS 拉低
    ↓
发送读命令
    ↓
从机解析命令
    ↓
发送 Dummy Byte
    ↓
接收寄存器值
    ↓
CS 拉高
```

例如读取设备 ID：

```text
寄存器地址：0x00
读命令：0x80
设备 ID：0x42
```

事务：

```text
CS LOW
TX 0x80
TX 0xFF / RX 0x42
CS HIGH
```

其中：

```text
第一次交换
主机发送命令，从机返回占位数据

第二次交换
主机发送 Dummy Byte，从机返回寄存器值
```

---

## 8. SPI 寄存器写入事务

写入配置寄存器同样需要两个字节交换。

完整流程：

```text
CS 拉低
    ↓
发送写命令
    ↓
发送写入数据
    ↓
CS 拉高
```

例如：

```text
配置寄存器地址：0x04
写入数据：0xA5
```

事务：

```text
CS LOW
TX 0x04
TX 0xA5
CS HIGH
```

写入成功后：

```text
CONFIG = 0xA5
```

---

## 9. 为什么整个事务期间 CS 必须保持有效

一次寄存器读取通常包含多个字节：

```text
命令字节
Dummy Byte
返回数据
```

正确时序：

```text
CS 拉低
发送命令
发送 Dummy Byte
接收数据
CS 拉高
```

错误时序：

```text
CS 拉低
发送命令
CS 拉高

CS 再拉低
发送 Dummy Byte
CS 再拉高
```

很多真实设备会在 CS 拉高时认为：

```text
当前事务已经结束
协议状态被复位
```

因此 Dummy Byte 可能被当成一条新命令，而不是上一条读取命令的继续。

---

## 10. Day24 工程结构

```text
day24
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
    └── day24_test
```

Day24 新增：

```text
include/spi_device.h
src/spi_device.c
```

主要修改：

```text
src/main.c
README.md
Makefile
```

---

## 11. 模块分层

当前 SPI 代码分为三层：

```text
main.c
应用测试层
        ↓
spi_device.c
设备协议层
        ↓
spi.c
SPI 总线层
```

### main.c

负责：

```text
初始化设备
读取设备 ID
读取状态
读取温度
写配置
验证测试结果
```

### spi_device.c

负责：

```text
设备寄存器表
读写命令协议
寄存器权限
温度数据解析
多字节事务
```

### spi.c

负责：

```text
SPI 初始化
片选控制
单字节交换
总线状态
传输次数
```

这种分层方式可以避免：

```text
应用代码直接处理底层 SPI 细节
```

---

## 12. 模拟设备结构体

定义：

```c
typedef struct
{
    uint8_t registers[SPI_DEVICE_REGISTER_COUNT];
    uint32_t read_count;
    uint32_t write_count;
} SpiDevice;
```

### registers

```c
uint8_t registers[5];
```

用于模拟设备内部寄存器。

对应关系：

```text
registers[0]：设备 ID
registers[1]：状态寄存器
registers[2]：温度高字节
registers[3]：温度低字节
registers[4]：配置寄存器
```

### read_count

```c
uint32_t read_count;
```

统计成功完成的寄存器读取次数。

### write_count

```c
uint32_t write_count;
```

统计成功完成的寄存器写入次数。

---

## 13. 设备初始化

函数：

```c
int spi_device_init(SpiDevice *device);
```

初始化后寄存器内容：

| 地址 | 初始值 | 含义 |
|---|---:|---|
| `0x00` | `0x42` | 设备 ID |
| `0x01` | `0x01` | 温度数据就绪 |
| `0x02` | `0x09` | 温度高字节 |
| `0x03` | `0xE6` | 温度低字节 |
| `0x04` | `0x00` | 默认配置 |

计数器：

```text
read_count = 0
write_count = 0
```

---

## 14. 设备 ID

定义：

```c
#define SPI_DEVICE_EXPECTED_ID 0x42U
```

设备 ID 用于确认：

```text
SPI 总线连接是否正常
CS 是否选择了正确设备
SPI 模式是否正确
读命令是否正确
设备型号是否匹配
```

主机读取后检查：

```c
if(device_id == SPI_DEVICE_EXPECTED_ID)
{
    /* 设备识别成功 */
}
```

实际输出：

```text
[DEVICE] Device ID = 0x42
[CHECK] Device ID matched
```

---

## 15. 状态寄存器

定义：

```c
#define SPI_DEVICE_STATUS_READY 0x01U
```

表示状态寄存器的 bit0：

```text
bit0 = 1：温度数据已经准备完成
bit0 = 0：温度数据还未准备完成
```

检查方式：

```c
if((status &
    SPI_DEVICE_STATUS_READY) != 0U)
{
    /* 数据已就绪 */
}
```

实际输出：

```text
[DEVICE] Status = 0x01
[CHECK] Temperature data is ready
```

按位与可以只检查目标状态位，而不影响其他状态位。

---

## 16. 寄存器地址合法性检查

函数：

```c
int spi_device_register_is_valid(
    uint8_t register_address);
```

当前寄存器数量：

```c
#define SPI_DEVICE_REGISTER_COUNT 5U
```

因此合法地址：

```text
0x00
0x01
0x02
0x03
0x04
```

核心判断：

```c
if(register_address <
   SPI_DEVICE_REGISTER_COUNT)
{
    return 1;
}
```

非法地址示例：

```text
0x05
0x20
0x7F
```

---

## 17. 寄存器写权限检查

函数：

```c
int spi_device_register_is_writable(
    uint8_t register_address);
```

当前只有：

```text
0x04 CONFIG
```

允许写入。

以下寄存器为只读：

```text
0x00 DEVICE_ID
0x01 STATUS
0x02 TEMP_HIGH
0x03 TEMP_LOW
```

核心判断：

```c
if(register_address ==
   SPI_DEVICE_REG_CONFIG)
{
    return 1;
}
```

---

## 18. 为什么设备 ID 不能写入

设备 ID 一般由芯片制造商固定。

如果允许主机随意修改：

```text
设备身份将失去意义
驱动无法确认芯片型号
软件测试会产生错误结果
```

因此尝试：

```c
spi_device_write_register(
    &spi,
    &device,
    SPI_DEVICE_REG_ID,
    0x99U);
```

会返回失败。

实际输出：

```text
[EXPECTED ERROR] Read-only device ID write rejected
[CHECK] Device ID remains 0x42
```

---

## 19. 温度原始值

定义：

```c
#define SPI_DEVICE_DEFAULT_TEMP_RAW 2534U
#define SPI_DEVICE_TEMP_SCALE       100.0f
```

本次规定：

```text
一个原始单位 = 0.01℃
```

因此：

```text
2534 ÷ 100.0 = 25.34℃
```

换算代码：

```c
temperature_c =
    (float)temperature_raw /
    SPI_DEVICE_TEMP_SCALE;
```

---

## 20. 为什么温度需要两个寄存器

单个寄存器是 8 位：

```text
最大无符号值 = 255
```

温度原始值：

```text
2534
```

超过单字节范围，因此需要两个寄存器保存：

```text
TEMP_HIGH
TEMP_LOW
```

2534 转换为十六进制：

```text
2534 = 0x09E6
```

所以：

```text
高字节 = 0x09
低字节 = 0xE6
```

---

## 21. 温度高字节拆分

代码：

```c
device->registers[
    SPI_DEVICE_REG_TEMP_HIGH] =
    (uint8_t)(
        (temperature_raw >> 8) &
        0xFFU);
```

以 `0x09E6` 为例：

```text
0x09E6 >> 8 = 0x0009
```

得到：

```text
0x09
```

---

## 22. 温度低字节拆分

代码：

```c
device->registers[
    SPI_DEVICE_REG_TEMP_LOW] =
    (uint8_t)(
        temperature_raw &
        0xFFU);
```

计算：

```text
0x09E6 & 0x00FF = 0x00E6
```

得到：

```text
0xE6
```

---

## 23. 温度高低字节组合

读取后得到：

```text
high_byte = 0x09
low_byte  = 0xE6
```

组合：

```c
temperature_raw =
    (uint16_t)(
        ((uint16_t)high_byte << 8) |
        (uint16_t)low_byte);
```

计算：

```text
0x09 << 8 = 0x0900
0x0900 | 0x00E6 = 0x09E6
```

十进制：

```text
0x09E6 = 2534
```

温度：

```text
2534 ÷ 100.0 = 25.34℃
```

---

## 24. 设置模拟温度

函数：

```c
int spi_device_set_temperature_raw(
    SpiDevice *device,
    uint16_t temperature_raw);
```

该函数用于模拟传感器产生新的温度数据。

例如：

```c
spi_device_set_temperature_raw(
    &device,
    3128U);
```

表示：

```text
31.28℃
```

3128 转为十六进制：

```text
3128 = 0x0C38
```

对应：

```text
高字节 = 0x0C
低字节 = 0x38
```

---

## 25. 读取寄存器函数

接口：

```c
int spi_device_read_register(
    SpiBus *spi,
    SpiDevice *device,
    uint8_t register_address,
    uint8_t *register_value);
```

函数检查：

```text
spi 是否为空
device 是否为空
register_value 是否为空
寄存器地址是否合法
SPI 总线是否空闲
```

完整事务：

```text
选择设备
发送读命令
准备寄存器响应
发送 Dummy Byte
读取寄存器值
释放设备
```

只有完整事务成功后：

```c
device->read_count++;
```

---

## 26. 一次寄存器读取为什么有两次字节交换

读取设备 ID：

```text
第一次交换：
TX 0x80
RX 0xFF

第二次交换：
TX 0xFF
RX 0x42
```

因此：

```text
一次寄存器读取
等于两次 SPI 单字节交换
```

所以：

```text
SpiDevice.read_count + 1
SpiBus.transfer_count + 2
```

两种计数含义不同：

```text
read_count
统计上层寄存器读取次数

transfer_count
统计底层字节交换次数
```

---

## 27. 写寄存器函数

接口：

```c
int spi_device_write_register(
    SpiBus *spi,
    SpiDevice *device,
    uint8_t register_address,
    uint8_t register_value);
```

函数检查：

```text
指针是否合法
寄存器地址是否合法
寄存器是否可写
SPI 总线是否空闲
```

完整事务：

```text
选择设备
发送写命令
发送写入数据
释放设备
更新设备寄存器
```

一次成功写入：

```text
SpiDevice.write_count + 1
SpiBus.transfer_count + 2
```

---

## 28. 为什么完整事务成功后才修改寄存器

代码在 SPI 事务全部完成后，才执行：

```c
device->registers[
    register_address] =
    register_value;
```

这样可以避免：

```text
SPI 传输失败
但寄存器却已经被软件修改
```

正确逻辑：

```text
通信成功
    ↓
设备接受数据
    ↓
寄存器更新
```

---

## 29. 异常事务恢复

辅助函数：

```c
static void spi_device_abort_transaction(
    SpiBus *spi);
```

当事务进行到一半失败时，尝试执行：

```c
spi_deselect(spi);
```

目的是恢复：

```text
CS 高电平
SPI 总线空闲
```

否则可能出现：

```text
chip_selected = 1
后续事务全部失败
```

这种处理属于驱动错误恢复机制。

---

## 30. 为什么内部辅助函数使用 static

定义：

```c
static void spi_device_abort_transaction(
    SpiBus *spi);
```

`static` 表示该函数只在：

```text
spi_device.c
```

内部可见。

它属于模块内部实现细节，不需要暴露给：

```text
main.c
其他驱动模块
```

---

## 31. SPI 总线忙状态检查

读写函数中检查：

```c
if(spi->chip_selected != 0U)
{
    return 0;
}
```

当：

```text
chip_selected = 1
```

表示已经有一个事务正在进行。

此时设备驱动不会强行开始新事务，而是直接返回失败。

实际测试：

```text
[SPI] Bus manually selected
[EXPECTED ERROR] New transaction rejected while bus busy
[SPI] Busy bus state released
```

以后进入 RTOS 后，共享 SPI 总线通常还需要使用：

```text
互斥锁
信号量
```

防止多个任务同时访问。

---

## 32. 命令编码测试

实际输出：

```text
[COMMAND] Read ID register        : 0x80
[COMMAND] Read temperature high   : 0x82
[COMMAND] Write config register   : 0x04
[CHECK] Read command bit7 = 1
[CHECK] Write command bit7 = 0
```

计算：

```text
读 ID：
0x00 | 0x80 = 0x80

读温度高字节：
0x02 | 0x80 = 0x82

写配置：
0x04 & 0x7F = 0x04
```

---

## 33. 设备信息读取测试

程序读取：

```text
设备 ID
状态寄存器
配置寄存器
```

实际输出：

```text
[DEVICE] Device ID = 0x42
[CHECK] Device ID matched
[DEVICE] Status    = 0x01
[CHECK] Temperature data is ready
[DEVICE] Config    = 0x00
```

三次寄存器读取后：

```text
Register reads = 3
SPI byte transfers = 6
```

因为：

```text
3 × 2 = 6
```

---

## 34. 温度读取测试

程序读取：

```text
0x02：温度高字节
0x03：温度低字节
```

实际输出：

```text
[TEMPERATURE] High byte  = 0x09
[TEMPERATURE] Low byte   = 0xE6
[TEMPERATURE] Raw value  = 2534
[TEMPERATURE] Celsius    = 25.34 C
[CHECK] Temperature raw value matched
```

温度读取增加两次寄存器读取：

```text
read_count：3 → 5
```

每个寄存器读取包含两次字节交换：

```text
transfer_count：6 → 10
```

---

## 35. 配置寄存器写入测试

流程：

```text
读取写入前配置
写入新配置
读取写入后配置
验证结果
```

实际输出：

```text
[WRITE] Config before = 0x00
[WRITE] New value     = 0xA5
[WRITE] Config after  = 0xA5
[CHECK] Configuration write verified
```

此时：

```text
Register writes = 1
SPI byte transfers = 16
```

---

## 36. 非法地址测试

程序尝试读取：

```text
0x20
```

但合法范围只有：

```text
0x00～0x04
```

因此输出：

```text
[EXPECTED ERROR] Invalid register 0x20 rejected
```

该操作在开始 SPI 事务之前就被拒绝，所以不会增加任何计数。

---

## 37. 只读寄存器保护测试

程序尝试：

```text
向设备 ID 寄存器写入 0x99
```

设备 ID 是只读寄存器，所以输出：

```text
[EXPECTED ERROR] Read-only device ID write rejected
[CHECK] Device ID remains 0x42
```

说明：

```text
写入失败
设备 ID 未改变
写计数未增加
SPI 字节交换计数未增加
```

---

## 38. 失败操作不改变计数

非法操作前记录：

```text
transfer_count
read_count
write_count
```

失败后重新比较。

实际输出：

```text
[CHECK] All counters unchanged
```

说明只有完整成功的事务才计数。

---

## 39. 最终计数分析

最终：

```text
SPI byte transfers = 16
Register reads     = 7
Register writes    = 1
```

七次寄存器读取：

```text
1. 读取设备 ID
2. 读取状态
3. 读取初始配置
4. 读取温度高字节
5. 读取温度低字节
6. 写入前读取配置
7. 写入后读取配置
```

每次读取两个字节：

```text
7 × 2 = 14
```

一次寄存器写入两个字节：

```text
1 × 2 = 2
```

总计：

```text
14 + 2 = 16
```

---

## 40. 完整运行结果

```text
===== Day24 SPI Register Protocol Test =====
[SPI CONFIG] Clock = 1000000 Hz
[SPI CONFIG] Mode = 0
[DEVICE CONFIG] Expected ID = 0x42
[DEVICE CONFIG] Register count = 5

----- Test 1: Command Encoding -----
[COMMAND] Read ID register        : 0x80
[COMMAND] Read temperature high   : 0x82
[COMMAND] Write config register   : 0x04
[CHECK] Read command bit7 = 1
[CHECK] Write command bit7 = 0

----- Test 2: Device Information -----
[DEVICE] Device ID = 0x42
[CHECK] Device ID matched
[DEVICE] Status    = 0x01
[CHECK] Temperature data is ready
[DEVICE] Config    = 0x00
[COUNT] Register reads = 3
[COUNT] SPI byte transfers = 6

----- Test 3: Temperature Read -----
[TEMPERATURE] High byte  = 0x09
[TEMPERATURE] Low byte   = 0xE6
[TEMPERATURE] Raw value  = 2534
[TEMPERATURE] Celsius    = 25.34 C
[CHECK] Temperature raw value matched
[COUNT] Register reads = 5
[COUNT] SPI byte transfers = 10

----- Test 4: Configuration Write -----
[WRITE] Config before = 0x00
[WRITE] New value     = 0xA5
[WRITE] Config after  = 0xA5
[CHECK] Configuration write verified
[COUNT] Register writes = 1
[COUNT] SPI byte transfers = 16

----- Test 5: Invalid Operations -----
[EXPECTED ERROR] Invalid register 0x20 rejected
[EXPECTED ERROR] Read-only device ID write rejected
[CHECK] Device ID remains 0x42
[CHECK] All counters unchanged

----- Test 6: Busy SPI Bus -----
[SPI] Bus manually selected
[EXPECTED ERROR] New transaction rejected while bus busy
[SPI] Busy bus state released
[CHECK] Busy rejection changed no counters

----- Final Protocol Status -----
[FINAL] SPI byte transfers = 16
[FINAL] Register reads     = 7
[FINAL] Register writes    = 1
[FINAL] Device selected    = NO
[FINAL] Config register    = 0xA5
[FINAL CHECK] All protocol tests passed

===== SPI Register Test Finished =====
```

编译过程没有：

```text
warning
error
```

---

## 41. 编译运行

进入 Day24：

```bash
cd /root/Embedded_14Days/day24
```

清理构建文件：

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
"COMMAND|DEVICE|TEMPERATURE|WRITE|EXPECTED|FINAL"
```

---

## 42. 自动验证

验证最终检查：

```bash
make run | grep -c \
"All protocol tests passed"
```

预期：

```text
1
```

验证温度：

```bash
make run | grep -c \
"Celsius    = 25.34 C"
```

预期：

```text
1
```

验证配置写入：

```bash
make run | grep -c \
"Config after  = 0xA5"
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
3
```

三个预期错误：

```text
非法寄存器地址
写入只读设备 ID
总线忙时启动新事务
```

验证最终片选状态：

```bash
make run | grep -c \
"Device selected    = NO"
```

预期：

```text
1
```

---

## 43. 本次核心代码

构造读命令：

```c
command =
    register_address |
    SPI_DEVICE_READ_FLAG;
```

构造写命令：

```c
command =
    register_address &
    SPI_DEVICE_ADDRESS_MASK;
```

温度高字节：

```c
(temperature_raw >> 8) & 0xFFU
```

温度低字节：

```c
temperature_raw & 0xFFU
```

组合高低字节：

```c
((uint16_t)high_byte << 8) |
(uint16_t)low_byte
```

温度换算：

```c
(float)temperature_raw / 100.0f
```

状态位检查：

```c
(status & SPI_DEVICE_STATUS_READY) != 0U
```

总线忙检查：

```c
spi->chip_selected != 0U
```

---

## 44. 当前实现的局限

本次仍然是软件模拟，尚未涉及真实硬件中的：

```text
SPI 外设寄存器
GPIO 复用配置
真实 SCK 时钟
真实 MOSI/MISO 电平
发送缓冲区空标志
接收缓冲区非空标志
SPI 忙标志
硬件超时
芯片上电延时
真实设备时序
连续地址读取
Burst Read
CRC 校验
SPI 中断
SPI DMA
RTOS 总线互斥锁
```

当前从机寄存器只是：

```c
uint8_t registers[5];
```

但整体驱动分层和寄存器访问思想与真实 SPI 设备驱动相似。

---

## 45. 今日收获

通过 Day24 学习，掌握了：

1. SPI 寄存器协议的基本结构；
2. 设备选择和寄存器选择的区别；
3. 使用命令字节区分读写操作；
4. 使用 bit7 表示读写标志；
5. 使用 bit6～bit0 表示寄存器地址；
6. 使用按位或设置读标志；
7. 使用掩码提取寄存器地址；
8. SPI 多字节事务；
9. CS 在完整事务中保持有效；
10. 两次字节交换完成一次寄存器读取；
11. 两次字节交换完成一次寄存器写入；
12. 使用数组模拟设备寄存器；
13. 设备 ID 的作用；
14. 状态寄存器的作用；
15. 使用按位与检查状态位；
16. 区分只读寄存器和可写寄存器；
17. 非法寄存器地址检查；
18. 温度高低字节拆分；
19. 温度高低字节组合；
20. 温度原始值换算；
21. SPI 字节交换计数；
22. 设备寄存器读写计数；
23. 失败操作不修改计数；
24. 总线忙状态检查；
25. 异常事务后的 CS 恢复；
26. `static` 内部辅助函数；
27. 应用层、设备层和总线层分层设计；
28. 从 SPI 基础通信过渡到 SPI 设备驱动。

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
```

---

## 46. 下一步学习计划

Day25 将开始学习：

```text
I²C 基础通信
```

计划内容：

```text
SCL 与 SDA
开漏输出
上拉电阻
START 起始信号
STOP 停止信号
7 位设备地址
读写方向位
ACK 应答
NACK 非应答
单字节写入
单字节读取
非法地址检查
总线忙状态
```

Day25 将重点理解：

```text
SPI 使用 CS 选择设备
I²C 使用设备地址选择设备
```

---

## 47. 项目仓库

项目地址：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects
```

Day24 目录：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects/tree/main/day24
```
