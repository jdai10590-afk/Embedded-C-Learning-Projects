# Day26：I²C 寄存器读写、Repeated START 与连续多字节读取

## 1. 学习目标

Day25 已经完成了 I²C 基础通信，包括：

```text
SCL 和 SDA
START 与 STOP
7 位设备地址
读写方向位
ACK 与 NACK
单字节写入
单字节读取
错误设备地址处理
```

Day26 在此基础上模拟一个具有内部寄存器的 I²C 温度传感器，进一步学习：

```text
I²C 设备寄存器表
寄存器指针
Repeated START
单寄存器读取
配置寄存器写入
连续多字节读取
寄存器地址自动递增
温度高低字节组合
只读寄存器保护
非法寄存器地址保护
连续读取越界保护
总线忙状态保护
失败事务恢复
总线层、设备层和应用层分层
```

本次最终运行结果：

```text
[FINAL] START count          = 6
[FINAL] Repeated START count = 5
[FINAL] STOP count           = 6
[FINAL] ACK count            = 19
[FINAL] NACK count           = 5
[FINAL] Byte count           = 24
[FINAL] Register reads       = 6
[FINAL] Register writes      = 1
[FINAL] Device selected      = NO
[FINAL] Bus started          = NO
[FINAL] Config register      = 0xA5
[FINAL CHECK] All I2C register tests passed
```

---

## 2. 为什么需要 I²C 寄存器协议

真实 I²C 设备通常包含多个内部寄存器，例如：

```text
设备 ID
状态寄存器
测量结果
配置参数
中断状态
故障标志
```

主机仅发送设备地址和读方向：

```text
START
设备地址 + READ
```

只能告诉设备：

```text
主机准备读取数据
```

但不能告诉设备：

```text
主机想读取哪个寄存器
```

因此，读取设备内部寄存器时，通常需要先向从机发送寄存器地址，再切换到读方向。

---

## 3. Day26 模拟设备

本次模拟一个地址为 `0x48` 的 I²C 温度传感器。

### 设备地址

```c
#define I2C_DEVICE_DEFAULT_ADDRESS 0x48U
```

对应总线地址字节：

```text
写方向：0x90
读方向：0x91
```

计算方式：

```text
0x48 << 1 = 0x90

WRITE：
0x90 | 0 = 0x90

READ：
0x90 | 1 = 0x91
```

---

## 4. 模拟寄存器表

| 地址 | 寄存器 | 初始值 | 权限 |
|---|---|---:|---|
| `0x00` | DEVICE_ID | `0x42` | 只读 |
| `0x01` | STATUS | `0x01` | 只读 |
| `0x02` | TEMP_HIGH | `0x09` | 只读 |
| `0x03` | TEMP_LOW | `0xE6` | 只读 |
| `0x04` | CONFIG | `0x00` | 可读写 |

代码定义：

```c
#define I2C_DEVICE_REG_ID           0x00U
#define I2C_DEVICE_REG_STATUS       0x01U
#define I2C_DEVICE_REG_TEMP_HIGH    0x02U
#define I2C_DEVICE_REG_TEMP_LOW     0x03U
#define I2C_DEVICE_REG_CONFIG       0x04U

#define I2C_DEVICE_REGISTER_COUNT   5U
```

---

## 5. 设备 ID

设备 ID 定义：

```c
#define I2C_DEVICE_EXPECTED_ID 0x42U
```

设备 ID 用于确认：

```text
设备地址是否正确
I²C 通信是否成功
当前连接的设备型号是否正确
寄存器读取流程是否正确
```

本次读取结果：

```text
[DEVICE] ID register = 0x42
[CHECK] Device ID matched
```

---

## 6. 状态寄存器

状态寄存器初始值：

```text
0x01
```

bit0 表示温度数据是否已经准备完成。

定义：

```c
#define I2C_DEVICE_STATUS_READY 0x01U
```

检查方式：

```c
if((status &
    I2C_DEVICE_STATUS_READY) != 0U)
{
    /* 温度数据已经准备完成 */
}
```

实际输出：

```text
[DEVICE] Status register = 0x01
[CHECK] Temperature data is ready
```

---

## 7. 什么是寄存器指针

设备内部定义：

```c
uint8_t register_pointer;
uint8_t register_pointer_valid;
```

寄存器指针表示：

```text
下一次读取或写入要访问的寄存器位置
```

例如主机发送寄存器地址：

```text
0x02
```

设备保存：

```c
device->register_pointer = 0x02U;
device->register_pointer_valid = 1U;
```

之后进入读取方向时，设备从：

```c
device->registers[
    device->register_pointer]
```

返回数据。

此时读取的是：

```text
TEMP_HIGH = 0x09
```

---

## 8. 为什么需要 register_pointer_valid

设备刚初始化时，主机还没有发送寄存器地址。

此时：

```c
register_pointer_valid = 0U;
```

表示当前寄存器指针不可用。

发送合法寄存器地址后：

```c
register_pointer_valid = 1U;
```

这样可以防止错误流程：

```text
未设置寄存器地址
直接进入读方向
```

否则从机无法确定应该返回哪个寄存器的数据。

---

## 9. 什么是 Repeated START

Repeated START 中文通常称为：

```text
重复起始信号
```

普通 START 发生在总线空闲状态：

```text
started = 0
```

Repeated START 发生在事务尚未通过 STOP 结束时：

```text
started = 1
```

Day26 新增接口：

```c
int i2c_repeated_start(
    I2cBus *bus);
```

调用成功后：

```text
started 保持为 1
addressed 清零
current_address 清零
direction 恢复默认值
repeated_start_count 加 1
```

---

## 10. 为什么 Repeated START 后清除 addressed

在寄存器读取的第一阶段：

```text
START
设备地址 + WRITE
寄存器地址
```

地址匹配后：

```text
addressed = 1
direction = WRITE
```

产生 Repeated START 后，需要重新发送：

```text
设备地址 + READ
```

因此，旧的地址匹配状态必须清除：

```c
bus->addressed = 0U;
```

否则程序可能错误地认为已经完成了读地址阶段。

---

## 11. 为什么使用 Repeated START

一种寄存器读取方式可以写成：

```text
START
设备地址 + WRITE
寄存器地址
STOP

START
设备地址 + READ
数据
STOP
```

但更常见的方式是：

```text
START
设备地址 + WRITE
寄存器地址
Repeated START
设备地址 + READ
数据
STOP
```

Repeated START 的优势：

```text
寄存器地址设置和数据读取保持为一个连续事务
中间不会释放总线
其他主机不能插入事务
设备内部寄存器指针状态不容易被破坏
符合许多芯片数据手册规定的读取时序
```

---

## 12. 单寄存器读取流程

读取设备 ID 寄存器 `0x00`：

```text
START
    ↓
设备地址 0x48 + WRITE
    ↓
从机 ACK
    ↓
发送寄存器地址 0x00
    ↓
从机 ACK
    ↓
Repeated START
    ↓
设备地址 0x48 + READ
    ↓
从机 ACK
    ↓
读取设备 ID 0x42
    ↓
主机 NACK
    ↓
STOP
```

总线上的 8 位字段：

```text
0x90：写地址字节
0x00：寄存器地址
0x91：读地址字节
0x42：寄存器数据
```

一次单寄存器读取产生：

```text
普通 START：1
Repeated START：1
STOP：1
ACK：3
NACK：1
Byte：4
```

---

## 13. 为什么单字节读取后发送 NACK

主机读取数据时：

```text
从机是发送方
主机是接收方
```

读取最后一个字节后，主机发送 NACK，表示：

```text
当前字节已经收到
不再继续读取下一个字节
```

因此：

```text
最后一个数据字节后发送 NACK
```

属于正常协议行为，不代表通信失败。

---

## 14. 配置寄存器写入流程

向 CONFIG 寄存器 `0x04` 写入 `0xA5`：

```text
START
    ↓
设备地址 0x48 + WRITE
    ↓
从机 ACK
    ↓
发送寄存器地址 0x04
    ↓
从机 ACK
    ↓
发送数据 0xA5
    ↓
从机 ACK
    ↓
STOP
```

总线上的字节：

```text
0x90
0x04
0xA5
```

写入成功后：

```text
CONFIG = 0xA5
```

---

## 15. 为什么写事务完成后才更新寄存器

配置寄存器不是在事务刚开始时就修改，而是在以下步骤全部成功后才更新：

```text
START 成功
设备地址发送成功
地址 ACK
寄存器地址发送成功
寄存器地址 ACK
数据发送成功
数据 ACK
STOP 成功
```

之后才执行：

```c
device->registers[
    register_address] =
    register_value;
```

这样可以避免：

```text
I²C 事务中途失败
但软件模拟寄存器已经错误改变
```

---

## 16. 连续多字节读取

温度使用两个寄存器保存：

```text
0x02：TEMP_HIGH
0x03：TEMP_LOW
```

连续读取流程：

```text
START
    ↓
设备地址 + WRITE
    ↓
寄存器地址 0x02
    ↓
Repeated START
    ↓
设备地址 + READ
    ↓
读取 0x09
    ↓
主机 ACK
    ↓
读取 0xE6
    ↓
主机 NACK
    ↓
STOP
```

第一次读取后发送 ACK：

```text
表示主机还要继续读取
```

第二次读取后发送 NACK：

```text
表示当前是最后一个字节
```

---

## 17. 中间字节 ACK，最后字节 NACK

连续读取代码逻辑：

```c
if((index + 1U) < length)
{
    master_response =
        I2C_RESPONSE_ACK;
}
else
{
    master_response =
        I2C_RESPONSE_NACK;
}
```

例如读取两个字节：

```text
index = 0
还有下一个字节
发送 ACK

index = 1
已经是最后一个字节
发送 NACK
```

---

## 18. 寄存器指针自动递增

读取 TEMP_HIGH 时：

```text
register_pointer = 0x02
```

设备返回：

```text
registers[0x02] = 0x09
```

之后自动执行：

```c
device->register_pointer++;
```

寄存器指针变成：

```text
0x03
```

下一次读取返回：

```text
registers[0x03] = 0xE6
```

这种行为称为：

```text
寄存器自动递增
Auto Increment
```

真实芯片是否支持自动递增，需要查看芯片数据手册。

---

## 19. 温度高低字节

默认温度原始值：

```c
#define I2C_DEVICE_DEFAULT_TEMP_RAW 2534U
```

十六进制：

```text
2534 = 0x09E6
```

因此：

```text
高字节 = 0x09
低字节 = 0xE6
```

---

## 20. 温度高字节拆分

代码：

```c
device->registers[
    I2C_DEVICE_REG_TEMP_HIGH] =
    (uint8_t)(
        (temperature_raw >> 8) &
        0x00FFU);
```

计算：

```text
0x09E6 >> 8 = 0x0009
```

得到：

```text
0x09
```

---

## 21. 温度低字节拆分

代码：

```c
device->registers[
    I2C_DEVICE_REG_TEMP_LOW] =
    (uint8_t)(
        temperature_raw &
        0x00FFU);
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

## 22. 温度高低字节组合

读取结果：

```text
high_byte = 0x09
low_byte  = 0xE6
```

组合代码：

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

得到：

```text
temperature_raw = 2534
```

---

## 23. 温度换算

温度比例：

```c
#define I2C_DEVICE_TEMP_SCALE 100.0f
```

换算：

```c
temperature_c =
    (float)temperature_raw /
    I2C_DEVICE_TEMP_SCALE;
```

计算：

```text
2534 ÷ 100.0 = 25.34℃
```

实际输出：

```text
[TEMPERATURE] High byte = 0x09
[TEMPERATURE] Low byte  = 0xE6
[TEMPERATURE] Raw value = 2534
[TEMPERATURE] Celsius   = 25.34 C
```

---

## 24. 工程结构

```text
day26
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
│   ├── i2c_device.h
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
│   ├── i2c_device.c
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
    └── day26_test
```

Day26 新增：

```text
include/i2c_device.h
src/i2c_device.c
```

修改：

```text
include/i2c.h
src/i2c.c
src/main.c
README.md
Makefile
```

---

## 25. 软件分层

当前 I²C 工程分为三层：

```text
main.c
应用测试层
        ↓
i2c_device.c
设备寄存器协议层
        ↓
i2c.c
I²C 总线层
```

### main.c

负责：

```text
初始化总线和设备
执行测试
显示测试结果
检查最终计数
```

### i2c_device.c

负责：

```text
设备寄存器表
寄存器指针
寄存器读写权限
配置寄存器写入
单寄存器读取
连续多字节读取
温度数据处理
```

### i2c.c

负责：

```text
START
Repeated START
STOP
设备地址
读写方向
ACK/NACK
单字节发送
单字节接收
总线状态
通信计数
```

---

## 26. I2cRegisterDevice 结构体

定义：

```c
typedef struct
{
    I2cDevice bus_device;

    uint8_t registers[
        I2C_DEVICE_REGISTER_COUNT];

    uint8_t register_pointer;
    uint8_t register_pointer_valid;

    uint32_t register_read_count;
    uint32_t register_write_count;
} I2cRegisterDevice;
```

含义：

```text
bus_device：
基础 I²C 从机对象

registers：
设备内部寄存器数组

register_pointer：
当前寄存器位置

register_pointer_valid：
寄存器指针是否有效

register_read_count：
成功读取的寄存器数量

register_write_count：
成功写入的寄存器数量
```

---

## 27. 为什么复用 I2cDevice

Day25 已经定义：

```c
typedef struct
{
    uint8_t address;
    uint8_t stored_data;
    uint8_t read_data;
} I2cDevice;
```

Day26 将它作为成员：

```c
I2cDevice bus_device;
```

这样可以继续复用 Day25 的总线功能：

```text
设备地址匹配
ACK/NACK
单字节写入
单字节读取
```

同时在外层增加：

```text
寄存器数组
寄存器指针
寄存器读写计数
```

---

## 28. 单寄存器读取接口

接口：

```c
int i2c_register_device_read_register(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t register_address,
    uint8_t *register_value);
```

它内部复用了连续读取接口：

```c
return i2c_register_device_read_registers(
    bus,
    device,
    register_address,
    register_value,
    1U);
```

也就是说：

```text
单寄存器读取
等于
连续读取一个字节
```

---

## 29. 连续读取接口

接口：

```c
int i2c_register_device_read_registers(
    I2cBus *bus,
    I2cRegisterDevice *device,
    uint8_t start_register,
    uint8_t *buffer,
    size_t length);
```

参数：

```text
bus：
I²C 总线

device：
寄存器设备

start_register：
起始寄存器地址

buffer：
接收数据缓冲区

length：
读取字节数量
```

例如：

```c
uint8_t temperature_bytes[2];

i2c_register_device_read_registers(
    &bus,
    &device,
    I2C_DEVICE_REG_TEMP_HIGH,
    temperature_bytes,
    2U);
```

结果：

```text
temperature_bytes[0] = 0x09
temperature_bytes[1] = 0xE6
```

---

## 30. 为什么 length 使用 size_t

`size_t` 是 C 语言中用于表示：

```text
数组长度
元素数量
内存大小
缓冲区大小
```

的无符号整数类型。

定义来源：

```c
#include <stddef.h>
```

---

## 31. 连续读取范围检查

当前合法寄存器范围：

```text
0x00～0x04
```

例如从 `0x03` 开始读取 3 个字节，将访问：

```text
0x03
0x04
0x05
```

其中：

```text
0x05
```

已经越界。

代码提前计算：

```c
remaining_registers =
    I2C_DEVICE_REGISTER_COUNT -
    start_register;
```

然后检查：

```c
if(length > remaining_registers)
{
    return 0;
}
```

越界操作在产生 START 前被拒绝，不影响总线计数。

---

## 32. 只读寄存器保护

当前只有：

```text
CONFIG，地址 0x04
```

允许写入。

权限检查：

```c
int i2c_device_register_is_writable(
    uint8_t register_address)
{
    if(register_address ==
       I2C_DEVICE_REG_CONFIG)
    {
        return 1;
    }

    return 0;
}
```

以下寄存器均为只读：

```text
DEVICE_ID
STATUS
TEMP_HIGH
TEMP_LOW
```

程序尝试向 DEVICE_ID 写入 `0x99` 时，操作被拒绝。

最终设备 ID 仍为：

```text
0x42
```

---

## 33. 非法操作保护

Day26 测试了：

```text
访问非法寄存器 0x20
连续读取超出寄存器范围
写入只读 DEVICE_ID
读取长度为 0
总线忙时开始新事务
```

这些操作都被正确拒绝。

测试结果：

```text
[EXPECTED ERROR] Invalid register 0x20 rejected
[EXPECTED ERROR] Out-of-range continuous read rejected
[EXPECTED ERROR] Read-only device ID write rejected
[EXPECTED ERROR] Zero-length read rejected
[EXPECTED ERROR] Register read rejected while bus busy
```

---

## 34. 失败操作为什么不改变计数

非法寄存器和只读权限在 START 前检查。

因此失败操作不会：

```text
产生 START
产生 Repeated START
产生 STOP
增加 ACK/NACK
增加字节计数
增加寄存器读写计数
修改设备寄存器
```

测试输出：

```text
[CHECK] All counters unchanged
```

---

## 35. 总线忙状态保护

完整寄存器操作要求：

```text
事务开始前总线处于空闲状态
```

检查代码：

```c
if(bus->started != 0U)
{
    return 0;
}
```

程序手动产生 START，模拟总线正在使用：

```text
[I2C] Temporary bus marked busy
```

此时再次调用寄存器读取，操作被拒绝：

```text
[EXPECTED ERROR] Register read rejected while bus busy
```

---

## 36. 失败事务恢复

设备层定义内部函数：

```c
static void
i2c_register_device_abort_transaction(
    I2cBus *bus,
    I2cRegisterDevice *device);
```

当事务中途失败时：

```text
寄存器指针设为无效
如果总线已经开始，则尝试发送 STOP
```

目的：

```text
避免总线一直处于 started 状态
避免后续事务全部失败
避免继续使用不可信的寄存器指针
```

---

## 37. 为什么内部辅助函数使用 static

定义：

```c
static void
i2c_register_device_abort_transaction(...);
```

`static` 表示该函数只在：

```text
i2c_device.c
```

内部可见。

它属于模块内部实现细节，不需要暴露给应用层。

---

## 38. 实际运行结果

```text
===== Day26 I2C Register Protocol Test =====
[I2C CONFIG] Clock = 100000 Hz
[DEVICE CONFIG] 7-bit address = 0x48
[DEVICE CONFIG] Register count = 5
[DEVICE CONFIG] Expected ID = 0x42

----- Test 1: Repeated START State -----
[EXPECTED ERROR] Repeated START rejected on idle bus
[I2C] Normal START generated
[I2C] Repeated START generated
[STATE] Bus started = YES
[STATE] Addressed   = NO
[CHECK] Repeated START state is correct
[I2C] STOP generated
[CHECK] Temporary bus returned to idle

----- Test 2: Device Information -----
[DEVICE] ID register     = 0x42
[CHECK] Device ID matched
[DEVICE] Status register = 0x01
[CHECK] Temperature data is ready
[COUNT] Register reads = 2
[COUNT] Repeated START = 2

----- Test 3: Continuous Temperature Read -----
[TEMPERATURE] High byte = 0x09
[TEMPERATURE] Low byte  = 0xE6
[TEMPERATURE] Raw value = 2534
[TEMPERATURE] Celsius   = 25.34 C
[CHECK] Temperature value matched
[COUNT] Register reads = 4
[COUNT] ACK count      = 10
[COUNT] NACK count     = 3

----- Test 4: Configuration Register -----
[CONFIG] Before write = 0x00
[CONFIG] Write value  = 0xA5
[CONFIG] After write  = 0xA5
[CHECK] Configuration write verified
[COUNT] Register reads  = 6
[COUNT] Register writes = 1

----- Test 5: Invalid Register Operations -----
[EXPECTED ERROR] Invalid register 0x20 rejected
[EXPECTED ERROR] Out-of-range continuous read rejected
[EXPECTED ERROR] Read-only device ID write rejected
[EXPECTED ERROR] Zero-length read rejected
[CHECK] Device ID remains 0x42
[CHECK] All counters unchanged

----- Test 6: Busy Bus Protection -----
[I2C] Temporary bus marked busy
[EXPECTED ERROR] Register read rejected while bus busy
[I2C] Temporary bus released
[CHECK] Busy bus protection verified

----- Final I2C Register Status -----
[FINAL] START count          = 6
[FINAL] Repeated START count = 5
[FINAL] STOP count           = 6
[FINAL] ACK count            = 19
[FINAL] NACK count           = 5
[FINAL] Byte count           = 24
[FINAL] Register reads       = 6
[FINAL] Register writes      = 1
[FINAL] Device selected      = NO
[FINAL] Bus started          = NO
[FINAL] Config register      = 0xA5
[FINAL CHECK] All I2C register tests passed

===== I2C Register Test Finished =====
```

编译过程中没有：

```text
warning
error
```

---

## 39. START、Repeated START 和 STOP 计数

主总线共完成：

```text
5 次读取事务
1 次写入事务
```

所有事务都有普通 START：

```text
START = 6
```

只有读取事务需要 Repeated START：

```text
Repeated START = 5
```

所有事务最终都有 STOP：

```text
STOP = 6
```

---

## 40. 为什么寄存器读取次数是 6

读取内容：

```text
DEVICE_ID：1
STATUS：1
TEMP_HIGH：1
TEMP_LOW：1
CONFIG 写入前：1
CONFIG 写入后：1
```

总计：

```text
1 + 1 + 1 + 1 + 1 + 1 = 6
```

注意：

```text
温度读取是一次 I²C 事务
但读取了两个寄存器
```

所以：

```text
事务数增加 1
寄存器读取数增加 2
```

---

## 41. 为什么 Repeated START 是 5

发生寄存器读取的事务：

```text
读取 DEVICE_ID
读取 STATUS
连续读取温度
读取写入前 CONFIG
读取写入后 CONFIG
```

每个读取事务使用一次 Repeated START。

所以：

```text
Repeated START = 5
```

配置寄存器写入不需要切换到读方向，因此不使用 Repeated START。

---

## 42. 为什么 Byte count 是 24

### 四次单寄存器读取

单次读取包含：

```text
写地址字节
寄存器地址
读地址字节
寄存器数据
```

每次 4 个字节。

四次：

```text
4 × 4 = 16
```

### 一次连续温度读取

包含：

```text
写地址字节
寄存器地址
读地址字节
TEMP_HIGH
TEMP_LOW
```

共：

```text
5
```

### 一次 CONFIG 写入

包含：

```text
写地址字节
寄存器地址
配置数据
```

共：

```text
3
```

总计：

```text
16 + 5 + 3 = 24
```

---

## 43. 为什么 NACK count 是 5

共有 5 次读取事务。

每次读取事务的最后一个字节后，主机都发送 NACK：

```text
DEVICE_ID 后 NACK
STATUS 后 NACK
TEMP_LOW 后 NACK
写入前 CONFIG 后 NACK
写入后 CONFIG 后 NACK
```

所以：

```text
NACK = 5
```

这些 NACK 都是正常的读取结束信号。

---

## 44. 为什么 ACK count 是 19

### 四次单寄存器读取

每次 ACK：

```text
写地址 ACK
寄存器地址 ACK
读地址 ACK
```

每次 3 个 ACK。

四次：

```text
4 × 3 = 12
```

### 一次温度连续读取

ACK：

```text
写地址 ACK
寄存器地址 ACK
读地址 ACK
TEMP_HIGH 后主机 ACK
```

共：

```text
4
```

### 一次配置寄存器写入

ACK：

```text
写地址 ACK
寄存器地址 ACK
配置数据 ACK
```

共：

```text
3
```

总计：

```text
12 + 4 + 3 = 19
```

---

## 45. 编译运行

进入 Day26：

```bash
cd /root/Embedded_14Days/day26
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
"Repeated START|DEVICE|TEMPERATURE|CONFIG|EXPECTED|FINAL"
```

---

## 46. 自动验证

验证最终结果：

```bash
make run | grep -c \
"All I2C register tests passed"
```

预期：

```text
1
```

验证温度：

```bash
make run | grep -c \
"Celsius   = 25.34 C"
```

预期：

```text
1
```

验证配置：

```bash
make run | grep -c \
"After write  = 0xA5"
```

预期：

```text
1
```

验证 Repeated START 数量：

```bash
make run | grep -c \
"Repeated START count = 5"
```

预期：

```text
1
```

---

## 47. 本次核心代码

普通 START：

```c
i2c_start(bus);
```

Repeated START：

```c
i2c_repeated_start(bus);
```

STOP：

```c
i2c_stop(bus);
```

设置寄存器指针：

```c
device->register_pointer =
    start_register;
```

中间字节 ACK：

```c
master_response =
    I2C_RESPONSE_ACK;
```

最后字节 NACK：

```c
master_response =
    I2C_RESPONSE_NACK;
```

寄存器自动递增：

```c
device->register_pointer++;
```

温度组合：

```c
((uint16_t)high_byte << 8) |
(uint16_t)low_byte
```

---

## 48. Day25 与 Day26 的区别

### Day25

学习：

```text
设备地址
单字节写入
单字节读取
ACK/NACK
```

基本流程：

```text
START
Address
Data
STOP
```

### Day26

学习：

```text
设备内部寄存器
寄存器指针
Repeated START
连续多字节读取
寄存器权限
```

读取流程：

```text
START
Address + WRITE
Register Address
Repeated START
Address + READ
Data
STOP
```

---

## 49. SPI 寄存器读取与 I²C 寄存器读取对比

### SPI

```text
CS LOW
发送读命令和寄存器地址
发送 Dummy Byte
接收寄存器数据
CS HIGH
```

### I²C

```text
START
设备地址 + WRITE
寄存器地址
Repeated START
设备地址 + READ
接收寄存器数据
STOP
```

SPI 使用：

```text
CS 选择设备
```

I²C 使用：

```text
设备地址选择设备
```

---

## 50. 当前实现的局限

Day26 仍是软件模拟，尚未涉及：

```text
真实 SCL 与 SDA 电平
GPIO 开漏配置
外部上拉电阻
I²C 外设寄存器
发送寄存器空标志
接收寄存器非空标志
BUSY 标志
硬件超时
时钟拉伸
总线仲裁
总线恢复
多主机通信
中断方式
DMA 方式
真实传感器时序
CRC 校验
设备上电延时
```

当前重点是理解：

```text
寄存器读取协议
Repeated START
多字节读取
驱动分层
错误保护
```

---

## 51. 今日收获

通过 Day26 学习，掌握了：

1. I²C 设备内部寄存器；
2. 寄存器地址；
3. 寄存器指针；
4. 寄存器指针有效状态；
5. Repeated START；
6. 普通 START 和 Repeated START 的区别；
7. Repeated START 后重新发送地址；
8. 先写寄存器地址再读取数据；
9. 单寄存器读取流程；
10. 配置寄存器写入流程；
11. 连续多字节读取；
12. 中间字节 ACK；
13. 最后字节 NACK；
14. 寄存器自动递增；
15. 16 位温度高低字节拆分；
16. 16 位温度高低字节组合；
17. 温度原始值换算；
18. 只读和可写寄存器；
19. 非法地址保护；
20. 连续读取越界保护；
21. 零长度读取保护；
22. 总线忙状态保护；
23. 失败事务恢复；
24. 通信成功后再更新寄存器；
25. I²C 总线层、设备层和应用层分层；
26. 通信事务数和寄存器读取数的区别；
27. START、Repeated START、STOP、ACK、NACK 和字节计数。

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
Day26：I²C 寄存器读写、Repeated START 与连续读取
```

---

## 52. 下一步学习计划

Day27 将学习：

```text
中断基础与事件驱动
```

计划内容：

```text
轮询和中断的区别
中断请求
中断服务函数 ISR
中断标志
中断使能
中断优先级
共享变量
volatile
中断中只做必要工作
主循环处理事件
按键中断模拟
UART 接收中断模拟
中断计数
```

重点理解：

```text
中断负责快速响应和记录事件
主循环负责处理耗时任务
```

---

## 53. 项目仓库

项目地址：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects
```

Day26 目录：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects/tree/main/day26
```