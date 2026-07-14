# Day27：中断基础、ISR、volatile 与事件驱动

## 1. 学习目标

Day27 开始学习嵌入式系统中的中断机制。

本次使用纯 C 模拟：

```text
中断请求 IRQ
中断使能和禁止
中断挂起 pending
中断服务函数 ISR
中断调度
软件事件标志
主循环事件处理
volatile 共享变量
UART 接收中断
按键中断
多个中断同时发生
中断统计
非法操作保护
```

最终运行结果：

```text
[FINAL] Enabled flags       = 0x00000003
[FINAL] Pending flags       = 0x00000000
[FINAL] Event flags         = 0x00000000
[FINAL] IRQ requests        = 5
[FINAL] Handled IRQs        = 4
[FINAL] Button ISR count    = 2
[FINAL] UART RX ISR count   = 2
[FINAL] Button events       = 2
[FINAL] UART events         = 2
[FINAL] Last UART byte      = 'Z' (0x5A)
[FINAL CHECK] All interrupt tests passed
```

---

## 2. 什么是轮询

轮询是 CPU 主动、反复检查外设状态。

例如：

```c
while(1)
{
    if(button_is_pressed())
    {
        process_button();
    }

    if(uart_has_data())
    {
        process_uart();
    }

    if(adc_is_finished())
    {
        process_adc();
    }
}
```

CPU 会不断询问：

```text
按键按下了吗？
UART 收到数据了吗？
ADC 转换完成了吗？
```

即使没有事件发生，也要重复检查。

---

## 3. 轮询的优点

轮询的优点：

```text
程序结构简单
执行过程直观
容易调试
适合非常简单的系统
```

在外设数量少、实时性要求低的情况下，轮询仍然可以使用。

---

## 4. 轮询的缺点

轮询的缺点：

```text
浪费 CPU 时间
事件响应可能不及时
外设越多，主循环越复杂
阻塞操作可能导致事件漏处理
不适合高速或突发数据
系统功耗可能更高
```

例如主循环中存在一个长时间延时：

```c
delay_ms(1000);
```

这 1 秒内，CPU 可能无法及时检查 UART、按键和 ADC 状态。

---

## 5. 什么是中断

中断是外设在事件发生时，主动请求 CPU 响应。

基本过程：

```text
CPU 正在执行主程序
        ↓
外设事件发生
        ↓
外设产生中断请求
        ↓
CPU 暂停当前程序
        ↓
执行中断服务函数
        ↓
中断处理完成
        ↓
CPU 返回原程序继续执行
```

可以把中断理解为：

```text
CPU 正在工作
外设突然敲门
CPU 暂停当前任务
快速处理紧急事件
然后返回原位置继续工作
```

---

## 6. 什么是 IRQ

IRQ 是：

```text
Interrupt Request
中断请求
```

例如 UART 收到一个字节后，会产生 UART 接收中断请求。

本次模拟中：

```c
interrupt_request(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

表示按键外设产生一次中断请求。

调用后：

```text
BUTTON pending = 1
irq_request_count + 1
```

---

## 7. 什么是 ISR

ISR 是：

```text
Interrupt Service Routine
中断服务函数
```

本次定义了两个 ISR：

```c
void interrupt_button_isr(
    InterruptController *controller);

void interrupt_uart_rx_isr(
    InterruptController *controller);
```

真实单片机中的中断服务函数可能类似：

```c
void USART0_IRQHandler(void)
{
    /* 读取 UART 数据寄存器 */
    /* 清除 UART 中断标志 */
}
```

---

## 8. ISR 的主要职责

ISR 应只完成必要操作：

```text
确认中断源
读取关键数据
清除硬件中断标志
设置软件事件
更新必要计数
快速退出
```

例如 UART ISR：

```text
读取 UART 数据寄存器
保存接收字节
清除 UART pending
设置 UART 软件事件
退出 ISR
```

---

## 9. ISR 中不应该做什么

ISR 中不适合执行：

```text
长时间延时
等待循环
大量 printf
复杂命令解析
文件操作
Flash 擦写
复杂浮点计算
大规模数据处理
阻塞式通信
```

错误示例：

```c
void uart_rx_isr(void)
{
    delay_ms(1000);

    parse_command();

    save_to_flash();

    printf("Long interrupt processing...\n");
}
```

可能造成：

```text
其他中断响应延迟
主循环长时间停止
UART 数据丢失
实时性下降
中断嵌套复杂
系统出现卡顿
```

---

## 10. 正确的 ISR 设计

正确思路：

```c
void uart_rx_isr(void)
{
    received_byte =
        uart_data_register;

    uart_event_pending = 1U;
}
```

主循环处理复杂业务：

```c
if(uart_event_pending != 0U)
{
    uart_event_pending = 0U;

    process_uart_byte(
        received_byte);
}
```

核心原则：

```text
ISR 负责快速响应和记录事件
主循环负责完整业务处理
```

---

## 11. 中断源定义

Day27 模拟两个中断源：

```c
#define INTERRUPT_SOURCE_BUTTON  \
    (1UL << 0)

#define INTERRUPT_SOURCE_UART_RX \
    (1UL << 1)
```

对应二进制：

```text
BUTTON  = 0000 0001
UART_RX = 0000 0010
```

两个中断同时存在：

```text
0000 0001
OR
0000 0010
-----------
0000 0011
```

所以：

```text
INTERRUPT_SOURCE_ALL = 0x03
```

---

## 12. 为什么中断源使用位标志

使用位标志可以在一个整数中保存多个状态。

例如：

```c
controller->enabled_flags |=
    INTERRUPT_SOURCE_BUTTON;
```

表示使能按键中断。

再执行：

```c
controller->enabled_flags |=
    INTERRUPT_SOURCE_UART_RX;
```

最终：

```text
enabled_flags = 0x03
```

表示两个中断都已经使能。

---

## 13. 按位或设置标志

设置位：

```c
flags |= source;
```

例如：

```text
原 flags = 0000 0001
source   = 0000 0010
```

按位或：

```text
0000 0001
OR
0000 0010
-----------
0000 0011
```

其他已经存在的标志不会被破坏。

---

## 14. 按位与检查标志

检查 UART 中断是否已经使能：

```c
if((controller->enabled_flags &
    INTERRUPT_SOURCE_UART_RX) != 0U)
{
    /* UART RX 已使能 */
}
```

例如：

```text
enabled_flags = 0000 0011
UART_RX       = 0000 0010
```

按位与：

```text
0000 0011
AND
0000 0010
-----------
0000 0010
```

结果不为 0，说明该位已经设置。

---

## 15. 按位取反清除标志

禁止一个中断：

```c
controller->enabled_flags &=
    ~source;
```

例如清除 BUTTON：

```text
source       = 0000 0001
~source      = 1111 1110
enabled      = 0000 0011
```

按位与：

```text
0000 0011
AND
1111 1110
-----------
0000 0010
```

最终只保留 UART 中断使能。

---

## 16. 中断控制器结构体

定义：

```c
typedef struct
{
    volatile uint32_t enabled_flags;
    volatile uint32_t pending_flags;
    volatile uint32_t event_flags;

    volatile uint8_t uart_rx_data_register;
    volatile uint8_t uart_rx_event_data;

    volatile uint32_t irq_request_count;
    volatile uint32_t handled_irq_count;
    volatile uint32_t button_isr_count;
    volatile uint32_t uart_rx_isr_count;
} InterruptController;
```

---

## 17. enabled_flags

`enabled_flags` 表示：

```text
CPU 是否允许处理某个中断
```

某一位为 1：

```text
该中断已经使能
```

某一位为 0：

```text
该中断被禁止
```

需要注意：

```text
中断被禁止
不代表外设事件不能发生
```

外设事件仍然可以设置 pending。

---

## 18. pending_flags

`pending_flags` 表示：

```text
某个硬件事件已经发生
正在等待 ISR 处理
```

例如按键事件发生：

```c
controller->pending_flags |=
    INTERRUPT_SOURCE_BUTTON;
```

状态：

```text
BUTTON pending = 1
```

ISR 处理完成后清除：

```c
controller->pending_flags &=
    ~INTERRUPT_SOURCE_BUTTON;
```

---

## 19. event_flags

`event_flags` 表示：

```text
ISR 已经完成快速响应
主循环还有业务需要处理
```

例如按键 ISR：

```c
controller->event_flags |=
    INTERRUPT_EVENT_BUTTON;
```

主循环处理完成后：

```c
controller->event_flags &=
    ~INTERRUPT_EVENT_BUTTON;
```

---

## 20. enabled、pending 和 event 的区别

### enabled

表示：

```text
是否允许进入 ISR
```

### pending

表示：

```text
硬件中断已经发生
等待 ISR 处理
```

### event

表示：

```text
ISR 已经完成
等待主循环处理业务
```

完整关系：

```text
外设事件
    ↓
pending = 1
    ↓
检查 enabled
    ↓
进入 ISR
    ↓
pending = 0
event = 1
    ↓
主循环处理
    ↓
event = 0
```

---

## 21. pending 的生命周期

以按键中断为例。

### 事件发生前

```text
pending = 0
event = 0
```

### 外设提交请求

```text
pending = 1
event = 0
```

### ISR 执行后

```text
pending = 0
event = 1
```

### 主循环处理后

```text
pending = 0
event = 0
```

因此：

```text
pending 由硬件产生、由 ISR 清除
event 由 ISR 产生、由主循环清除
```

---

## 22. 什么是中断使能

使能按键中断：

```c
interrupt_enable(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

内部操作：

```c
controller->enabled_flags |=
    INTERRUPT_SOURCE_BUTTON;
```

使能 UART RX 中断：

```c
interrupt_enable(
    &controller,
    INTERRUPT_SOURCE_UART_RX);
```

最终：

```text
enabled_flags = 0x00000003
```

---

## 23. 什么是中断禁止

禁止按键中断：

```c
interrupt_disable(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

内部操作：

```c
controller->enabled_flags &=
    ~INTERRUPT_SOURCE_BUTTON;
```

禁止中断不会自动清除：

```text
pending 标志
```

---

## 24. 中断未使能时的行为

初始化后：

```text
BUTTON enabled = 0
```

产生按键请求：

```c
interrupt_request(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

状态变成：

```text
enabled = 0
pending = 1
```

执行调度：

```c
interrupt_dispatch(
    &controller);
```

不会进入 ISR。

因此：

```text
Handled in disabled state = 0
```

但 pending 会继续保留。

---

## 25. 使能后处理历史 pending

在 pending 已经存在时使能中断：

```c
interrupt_enable(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

再次执行：

```c
interrupt_dispatch(
    &controller);
```

调度器发现：

```text
pending = 1
enabled = 1
```

于是进入按键 ISR。

运行结果：

```text
[CHECK] Stored button IRQ entered ISR
```

---

## 26. 手动清除 pending

某些情况下，程序可能决定丢弃一个未处理请求。

调用：

```c
interrupt_clear_pending(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

清除后：

```text
BUTTON pending = 0
```

以后即使重新使能按键中断，该请求也不会再进入 ISR。

---

## 27. 什么是中断调度

真实 MCU 中，中断控制器和 CPU 会自动选择并进入 ISR。

Linux 普通 C 程序没有真实 MCU 中断，因此使用：

```c
interrupt_dispatch(
    &controller);
```

模拟中断调度。

核心逻辑：

```c
if(button_pending &&
   button_enabled)
{
    interrupt_button_isr(
        controller);
}

if(uart_pending &&
   uart_enabled)
{
    interrupt_uart_rx_isr(
        controller);
}
```

---

## 28. interrupt_dispatch 返回值

`interrupt_dispatch()` 返回本次实际执行的 ISR 数量。

没有中断可处理：

```text
返回 0
```

只处理一个中断：

```text
返回 1
```

按键和 UART 同时处理：

```text
返回 2
```

它表示的是：

```text
本次调度执行的 ISR 数量
```

累计处理数保存在：

```c
handled_irq_count
```

---

## 29. 按键 ISR

按键 ISR：

```c
void interrupt_button_isr(
    InterruptController *controller)
{
    if(controller == NULL)
    {
        return;
    }

    if(interrupt_is_pending(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        return;
    }

    interrupt_clear_pending(
        controller,
        INTERRUPT_SOURCE_BUTTON);

    controller->event_flags |=
        INTERRUPT_EVENT_BUTTON;

    controller->button_isr_count++;
    controller->handled_irq_count++;
}
```

它只完成：

```text
检查 pending
清除 pending
设置软件事件
增加 ISR 计数
快速退出
```

---

## 30. UART 接收 ISR

UART ISR：

```c
void interrupt_uart_rx_isr(
    InterruptController *controller)
{
    controller->uart_rx_event_data =
        controller->uart_rx_data_register;

    interrupt_clear_pending(
        controller,
        INTERRUPT_SOURCE_UART_RX);

    controller->event_flags |=
        INTERRUPT_EVENT_UART_RX;

    controller->uart_rx_isr_count++;
    controller->handled_irq_count++;
}
```

主要过程：

```text
读取硬件数据寄存器
保存给主循环
清除 UART pending
设置 UART 软件事件
更新统计
```

---

## 31. UART 的两个数据变量

定义：

```c
volatile uint8_t uart_rx_data_register;
volatile uint8_t uart_rx_event_data;
```

### uart_rx_data_register

模拟：

```text
UART 硬件接收数据寄存器
```

UART 收到字节后，数据先进入这里。

### uart_rx_event_data

模拟：

```text
ISR 保存给主循环的数据
```

数据流：

```text
UART 收到字节
        ↓
uart_rx_data_register
        ↓
UART RX ISR
        ↓
uart_rx_event_data
        ↓
主循环处理
```

---

## 32. UART 收到字符 A

模拟代码：

```c
interrupt_uart_receive_byte(
    &controller,
    (uint8_t)'A');
```

第一阶段：

```text
uart_rx_data_register = 'A'
UART pending = 1
irq_request_count + 1
```

ISR 执行后：

```text
uart_rx_event_data = 'A'
UART pending = 0
UART event = 1
```

主循环最终输出：

```text
[MAIN] Processing UART byte 'A' (0x41)
```

---

## 33. ASCII 字符与十六进制

字符：

```text
'A'
```

ASCII 十六进制值：

```text
0x41
```

字符：

```text
'Z'
```

ASCII 十六进制值：

```text
0x5A
```

本次最终收到的 UART 字节：

```text
'Z' (0x5A)
```

---

## 34. 软件事件处理函数

主循环处理函数：

```c
static int process_pending_events(
    InterruptController *controller,
    ApplicationEventStats *stats);
```

它负责：

```text
检查 BUTTON event
处理按键业务
清除 BUTTON event

检查 UART event
读取 UART 数据
处理 UART 业务
清除 UART event
```

ISR 不直接调用复杂业务函数。

---

## 35. 为什么主循环统计不需要 volatile

应用统计结构：

```c
typedef struct
{
    uint32_t button_event_count;
    uint32_t uart_event_count;
    uint8_t last_uart_byte;
} ApplicationEventStats;
```

这些成员只在主循环中修改，没有被 ISR 访问。

因此当前不需要使用：

```c
volatile
```

只有可能被 ISR、DMA、硬件寄存器或其他并发执行单元修改的变量，才需要重点考虑 `volatile`。

---

## 36. 什么是 volatile

例如：

```c
volatile uint32_t event_flags;
```

`volatile` 告诉编译器：

```text
该变量可能在当前代码看不到的位置被修改
每次访问都应重新从内存读取
不要长期缓存或假设它保持不变
```

中断共享变量常见写法：

```c
volatile uint8_t event_pending;
```

---

## 37. 没有 volatile 可能出现的问题

例如：

```c
while(uart_event_pending == 0U)
{
    /* 等待中断修改变量 */
}
```

如果没有 `volatile`，编译器可能认为：

```text
循环内部没有修改 uart_event_pending
变量值不会变化
```

从而进行不合适的优化。

使用：

```c
volatile uint8_t uart_event_pending;
```

可以告诉编译器，每次循环都重新读取变量。

---

## 38. volatile 不能保证线程安全

必须明确：

```text
volatile 不等于线程安全
volatile 不等于原子操作
volatile 不等于临界区
volatile 不等于互斥锁
volatile 不会自动防止竞争条件
```

它主要解决：

```text
编译器可见性和优化问题
```

---

## 39. shared_counter++ 不一定是原子操作

代码：

```c
shared_counter++;
```

可能被拆分为：

```text
读取 shared_counter
加 1
写回 shared_counter
```

如果中断在中间发生，也修改了相同变量，就可能出现竞争问题。

后续将学习：

```text
关闭中断
临界区
原子操作
互斥锁
```

---

## 40. 多个中断同时发生

本次同时产生：

```text
BUTTON IRQ
UART RX IRQ
```

pending：

```text
BUTTON  = 0x01
UART_RX = 0x02
```

组合后：

```text
pending_flags = 0x03
```

运行结果：

```text
[STATE] Pending flags = 0x00000003
[DISPATCH] Handled interrupts = 2
```

---

## 41. 两个 ISR 执行后的状态

调度器依次执行：

```text
BUTTON ISR
UART RX ISR
```

两个 ISR 执行后：

```text
pending_flags = 0x00
event_flags   = 0x03
```

其中：

```text
event bit0：按键事件
event bit1：UART 事件
```

主循环随后一次处理两个事件。

---

## 42. 当前调度顺序

当前调度器先检查：

```text
BUTTON
```

再检查：

```text
UART RX
```

这只是软件代码中的固定顺序。

它还不是真正的：

```text
硬件中断优先级
抢占优先级
中断嵌套
NVIC 优先级
```

后续会专门学习中断优先级和临界区。

---

## 43. 中断请求数和处理数的区别

本次中断请求总数：

```text
5
```

实际进入 ISR：

```text
4
```

五次请求分别是：

```text
1. 按键中断未使能时产生请求
2. UART 收到字符 A
3. 多中断测试中的按键请求
4. 多中断测试中的 UART 字符 Z
5. 按键中断被禁止时产生请求
```

第五个请求没有进入 ISR，而是被手动清除。

所以：

```text
IRQ requests = 5
Handled IRQs = 4
```

---

## 44. ISR 和软件事件统计

实际执行：

```text
Button ISR = 2 次
UART ISR   = 2 次
```

主循环处理：

```text
Button event = 2 次
UART event   = 2 次
```

说明：

```text
每次成功执行 ISR
都产生了一个对应软件事件
每个软件事件都被主循环处理
```

---

## 45. 同类多次请求可能合并

当前 pending 只是一个位标志。

连续执行三次：

```c
interrupt_request(
    &controller,
    INTERRUPT_SOURCE_BUTTON);

interrupt_request(
    &controller,
    INTERRUPT_SOURCE_BUTTON);

interrupt_request(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

结果可能是：

```text
irq_request_count = 3
BUTTON pending = 1
```

调度器只能知道：

```text
至少发生过一次 BUTTON 请求
```

可能只执行一次 ISR。

因此：

```text
同类多个请求可能被合并
```

---

## 46. 为什么 UART 不能长期只用一个字节变量

当前 UART 使用：

```c
uart_rx_event_data
```

一次只能保存一个字节。

如果 ISR 处理前快速收到多个字节：

```text
A
B
C
```

后面的字节可能覆盖前面的字节。

正确方案：

```text
UART ISR
    ↓
把每个字节写入 Ring Buffer
    ↓
主循环从 Ring Buffer 读取
    ↓
命令解析
```

后续会把 Day17 的 Ring Buffer 和 UART 中断结合起来。

---

## 47. 工程结构

```text
day27
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
│   ├── interrupt.h
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
│   ├── interrupt.c
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
    └── day27_test
```

Day27 新增：

```text
include/interrupt.h
src/interrupt.c
```

修改：

```text
src/main.c
README.md
Makefile
```

---

## 48. 软件分层

当前结构：

```text
main.c
应用层和主循环事件处理
        ↓
interrupt.c
中断控制、ISR 和调度
        ↓
interrupt.h
宏、结构体和接口
```

### main.c

负责：

```text
模拟外设事件
调用中断调度
处理软件事件
统计应用事件
检查最终结果
```

### interrupt.c

负责：

```text
中断使能
中断禁止
pending 管理
ISR
软件事件标志
中断调度
中断统计
```

---

## 49. 实际运行结果

```text
===== Day27 Interrupt and Event Test =====

----- Test 1: Initial State -----
[STATE] Enabled flags = 0x00000000
[STATE] Pending flags = 0x00000000
[STATE] Event flags   = 0x00000000
[CHECK] Initial interrupt state is correct

----- Test 2: Pending While Disabled -----
[STATE] Button enabled = NO
[IRQ] Button request generated
[STATE] Button pending = YES
[DISPATCH] Handled in disabled state = 0
[CHECK] Disabled IRQ remained pending
[IRQ] Button interrupt enabled
[DISPATCH] Handled after enabling = 1
[CHECK] Stored button IRQ entered ISR
[MAIN] Processing button event
[MAIN] Processed events = 1
[CHECK] Button event processed by main loop

----- Test 3: UART RX Interrupt -----
[IRQ] UART RX interrupt enabled
[UART HARDWARE] Received 'A'
[STATE] UART pending = YES
[DISPATCH] UART ISR count this time = 1
[CHECK] UART ISR saved byte correctly
[MAIN] Processing UART byte 'A' (0x41)
[MAIN] Processed events = 1
[CHECK] UART event processed by main loop

----- Test 4: Multiple Interrupts -----
[IRQ] Button and UART requests generated
[STATE] Pending flags = 0x00000003
[DISPATCH] Handled interrupts = 2
[STATE] Event flags = 0x00000003
[CHECK] Both ISRs completed
[MAIN] Processing button event
[MAIN] Processing UART byte 'Z' (0x5A)
[MAIN] Processed events = 2
[CHECK] Both events processed by main loop

----- Test 5: Interrupt Disable -----
[IRQ] Button interrupt disabled
[IRQ] Button request generated while disabled
[DISPATCH] Handled while disabled = 0
[CHECK] Disabled interrupt did not enter ISR
[IRQ] Disabled button pending cleared manually
[CHECK] Cleared IRQ was not processed later

----- Test 6: Invalid Operations -----
[EXPECTED ERROR] NULL init rejected
[EXPECTED ERROR] NULL enable rejected
[EXPECTED ERROR] NONE source rejected
[EXPECTED ERROR] Combined source rejected
[EXPECTED ERROR] Invalid IRQ source rejected
[EXPECTED ERROR] NULL UART controller rejected
[EXPECTED ERROR] Combined event rejected
[CHECK] Invalid operations changed no state

----- Final Interrupt Status -----
[FINAL] Enabled flags       = 0x00000003
[FINAL] Pending flags       = 0x00000000
[FINAL] Event flags         = 0x00000000
[FINAL] IRQ requests        = 5
[FINAL] Handled IRQs        = 4
[FINAL] Button ISR count    = 2
[FINAL] UART RX ISR count   = 2
[FINAL] Button events       = 2
[FINAL] UART events         = 2
[FINAL] Last UART byte      = 'Z' (0x5A)
[FINAL CHECK] All interrupt tests passed

===== Interrupt Test Finished =====
```

编译过程中没有：

```text
warning
error
```

---

## 50. 编译运行

进入 Day27：

```bash
cd /root/Embedded_14Days/day27
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
make run | grep -E \
"IRQ|ISR|Pending|Event|FINAL"
```

---

## 51. 自动验证

检查最终结果：

```bash
make run | grep -c \
"All interrupt tests passed"
```

预期：

```text
1
```

检查请求数量：

```bash
make run | grep -c \
"IRQ requests        = 5"
```

预期：

```text
1
```

检查处理数量：

```bash
make run | grep -c \
"Handled IRQs        = 4"
```

预期：

```text
1
```

检查最后 UART 字节：

```bash
make run | grep -c \
"Last UART byte      = 'Z' (0x5A)"
```

预期：

```text
1
```

---

## 52. Day27 核心代码

提交中断请求：

```c
interrupt_request(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

使能中断：

```c
interrupt_enable(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

禁止中断：

```c
interrupt_disable(
    &controller,
    INTERRUPT_SOURCE_BUTTON);
```

执行调度：

```c
interrupt_dispatch(
    &controller);
```

设置软件事件：

```c
controller->event_flags |=
    INTERRUPT_EVENT_BUTTON;
```

清除软件事件：

```c
interrupt_clear_event(
    &controller,
    INTERRUPT_EVENT_BUTTON);
```

---

## 53. 与真实单片机的对应关系

本次模拟代码：

```text
interrupt_request()
```

对应真实硬件：

```text
外设设置中断标志
向 NVIC 提交 IRQ
```

本次：

```text
interrupt_dispatch()
```

对应：

```text
CPU 和中断控制器自动选择 ISR
```

本次：

```text
interrupt_button_isr()
```

对应：

```text
EXTI_IRQHandler()
GPIO_IRQHandler()
```

本次：

```text
interrupt_uart_rx_isr()
```

对应：

```text
USART_IRQHandler()
UART_IRQHandler()
```

---

## 54. 当前实现的局限

Day27 仍然是软件模拟，尚未涉及：

```text
真实 CPU 上下文保存
真实硬件中断向量表
NVIC
中断优先级
中断嵌套
抢占
全局中断开关
真实 UART 寄存器
真实 GPIO EXTI
临界区
原子操作
Ring Buffer 接收
DMA
FreeRTOS 中断接口
```

当前重点是建立正确的中断思维：

```text
硬件事件
→ pending
→ ISR
→ 软件事件
→ 主循环
```

---

## 55. 今日收获

通过 Day27 学习，掌握了：

1. 轮询的工作方式；
2. 轮询的优缺点；
3. 中断基本概念；
4. IRQ 中断请求；
5. ISR 中断服务函数；
6. 中断使能；
7. 中断禁止；
8. pending 挂起标志；
9. 软件事件标志；
10. 中断源位标志；
11. 按位或设置标志；
12. 按位与检查标志；
13. 按位取反清除标志；
14. 中断调度；
15. 未使能中断的 pending 保留；
16. 使能后处理历史 pending；
17. 手动丢弃 pending；
18. 按键 ISR；
19. UART 接收 ISR；
20. UART 硬件寄存器模拟；
21. ISR 到主循环的数据传递；
22. 多个中断同时发生；
23. ISR 与主循环分工；
24. volatile 的基本作用；
25. volatile 的局限；
26. 请求次数与处理次数的区别；
27. ISR 次数与软件事件次数的关系；
28. 同类中断请求可能合并；
29. 单字节变量可能被覆盖；
30. 事件驱动程序结构。

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
Day24：SPI 寄存器读写协议
Day25：I²C 基础通信与 ACK/NACK
Day26：I²C 寄存器协议与 Repeated START
Day27：中断、ISR、volatile 与事件驱动
```

---

## 56. 下一步学习计划

Day28 将继续学习：

```text
中断共享变量与临界区
```

主要内容：

```text
volatile 深入理解
共享变量
读-改-写操作
竞争条件 Race Condition
原子操作
临界区
进入和退出临界区
模拟全局中断开关
保护共享计数器
丢失更新问题
安全读取 ISR 数据
```

重点问题：

```text
为什么 volatile 不能保证 shared_counter++ 安全
主循环读取多字节变量时，中断突然修改怎么办
什么时候需要暂时关闭中断
临界区为什么必须尽量短
```

后续 Day29 会把：

```text
UART 中断
+
Ring Buffer
+
主循环命令解析
```

组合成更接近真实项目的 UART 接收框架。

---

## 57. 项目仓库

项目地址：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects
```

Day27 目录：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects/tree/main/day27
```