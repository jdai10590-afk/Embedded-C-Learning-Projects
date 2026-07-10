# Day16：UART 串口命令模拟与字符串解析

## 1. 学习目标

Day16 在 Day15 GPIO 寄存器模拟的基础上，增加 UART 命令处理模块。

本次学习的主要目标是：

1. 理解 UART 命令控制的基本流程；
2. 学习使用字符串表示串口命令；
3. 学习使用 `strcmp()` 比较字符串；
4. 根据不同命令执行不同操作；
5. 处理未知命令；
6. 使用 UART 命令控制模拟 GPIO 和系统状态。

本项目目前支持以下命令：

```text
STATUS
LED ON
LED OFF
LED TOGGLE
RESET
```

同时也能识别未知命令，例如：

```text
HELLO
```

---

## 2. 工程结构

```text
day16
├── Makefile
├── include
│   ├── config.h
│   ├── control.h
│   ├── debug.h
│   ├── event.h
│   ├── fault_code.h
│   ├── fault.h
│   ├── gpio.h
│   ├── sensor.h
│   ├── state_machine.h
│   ├── system.h
│   ├── system_type.h
│   └── uart.h
├── src
│   ├── control.c
│   ├── event.c
│   ├── fault.c
│   ├── gpio.c
│   ├── main.c
│   ├── sensor.c
│   ├── state_machine.c
│   ├── system.c
│   └── uart.c
└── build
    └── day16_test
```

相比 Day15，Day16 新增：

```text
include/uart.h
src/uart.c
```

并修改了：

```text
src/main.c
Makefile
```

---

## 3. UART 命令处理流程

Day16 当前采用字符串模拟串口命令。

整体流程为：

```text
main.c 模拟发送命令
        ↓
uart_process_command()
        ↓
strcmp() 比较字符串
        ↓
找到对应命令分支
        ↓
执行 GPIO 或系统操作
        ↓
打印 UART 回复
```

需要注意，目前还不是真实串口硬件接收，而是在电脑上模拟完整命令字符串。

后续学习环形缓冲区后，将进一步模拟字符逐个到达的串口接收过程。

---

## 4. uart.h

`uart.h` 用于声明 UART 命令处理函数：

```c
#ifndef UART_H
#define UART_H

#include "system_type.h"

void uart_process_command(SystemStatus *sys, const char *command);

#endif
```

函数声明：

```c
void uart_process_command(SystemStatus *sys, const char *command);
```

包含两个参数：

```text
SystemStatus *sys
系统状态变量的地址，函数可以通过它修改系统状态。

const char *command
指向只读字符串命令，例如 "LED ON"。
```

---

## 5. uart.c

UART 命令处理函数主要使用 `strcmp()` 判断命令内容。

核心结构如下：

```c
void uart_process_command(SystemStatus *sys, const char *command)
{
    if(strcmp(command, "STATUS") == 0)
    {
        /* 打印系统状态 */
    }
    else if(strcmp(command, "LED ON") == 0)
    {
        /* 打开 LED */
    }
    else if(strcmp(command, "LED OFF") == 0)
    {
        /* 关闭 LED */
    }
    else if(strcmp(command, "LED TOGGLE") == 0)
    {
        /* 翻转 LED */
    }
    else if(strcmp(command, "RESET") == 0)
    {
        /* 系统复位 */
    }
    else
    {
        /* 未知命令 */
    }
}
```

---

## 6. strcmp() 字符串比较

C 语言不能直接使用下面的方法比较字符串内容：

```c
command == "STATUS"
```

这种写法主要比较字符串地址，而不是每个字符的内容。

正确方法是使用：

```c
strcmp(command, "STATUS")
```

`strcmp()` 的返回规则为：

```text
两个字符串相同：返回 0
两个字符串不同：返回非 0
```

因此判断命令时需要写：

```c
if(strcmp(command, "STATUS") == 0)
```

它表示：

```text
如果收到的命令和 "STATUS" 完全相同。
```

使用 `strcmp()` 前需要包含：

```c
#include <string.h>
```

---

## 7. STATUS 命令

收到：

```text
STATUS
```

执行：

```c
system_print(sys);
```

用于打印当前系统状态，包括：

```text
LED 状态
GPIO 输出寄存器
LED 引脚电平
系统模式
电压、电流和温度
故障码
事件统计数据
```

---

## 8. LED ON 命令

收到：

```text
LED ON
```

执行：

```c
sys->led_state = LED_STATE_ON;
gpio_led_on(sys);
```

其中：

```c
sys->led_state = LED_STATE_ON;
```

用于更新软件层面的 LED 状态。

```c
gpio_led_on(sys);
```

用于把模拟 GPIO 寄存器中的 LED 位设置为 1。

执行后：

```text
LED state: 1
GPIO output reg: 0x00000001
LED pin level: 1
```

---

## 9. LED OFF 命令

收到：

```text
LED OFF
```

执行：

```c
sys->led_state = LED_STATE_OFF;
gpio_led_off(sys);
```

`gpio_led_off()` 内部通过以下位操作清除 LED 位：

```c
sys->gpio_output_reg &= ~BIT(LED_GPIO_PIN);
```

执行后：

```text
LED state: 0
GPIO output reg: 0x00000000
LED pin level: 0
```

---

## 10. LED TOGGLE 命令

收到：

```text
LED TOGGLE
```

首先执行：

```c
gpio_led_toggle(sys);
```

将 LED 对应的 GPIO 位翻转：

```text
原来为 0 -> 变成 1
原来为 1 -> 变成 0
```

然后通过：

```c
gpio_led_read(sys)
```

读取翻转后的 GPIO 电平，并同步更新 `led_state`：

```c
if(gpio_led_read(sys) == 1)
{
    sys->led_state = LED_STATE_ON;
}
else
{
    sys->led_state = LED_STATE_OFF;
}
```

这样可以保证：

```text
软件状态 led_state
模拟硬件状态 gpio_output_reg
```

始终保持一致。

---

## 11. RESET 命令

收到：

```text
RESET
```

执行：

```c
system_init(sys);
gpio_init(sys);
```

复位后：

```text
系统模式恢复 INIT
LED 状态恢复 OFF
GPIO 输出寄存器清零
故障码清零
运行统计数据清零
```

---

## 12. 未知命令处理

如果收到程序不支持的命令，例如：

```text
HELLO
```

前面的所有命令分支都不会成立，最终进入：

```c
else
{
    printf("[UART] Unknown command: %s\n", command);
}
```

输出：

```text
[UART] Unknown command: HELLO
```

未知命令处理可以避免程序收到错误输入后执行错误操作。

---

## 13. main.c 命令模拟

Day16 在 `main.c` 中依次模拟发送：

```c
uart_process_command(&sys, "STATUS");

uart_process_command(&sys, "LED ON");
uart_process_command(&sys, "STATUS");

uart_process_command(&sys, "LED OFF");
uart_process_command(&sys, "STATUS");

uart_process_command(&sys, "LED TOGGLE");
uart_process_command(&sys, "STATUS");

uart_process_command(&sys, "RESET");
uart_process_command(&sys, "STATUS");

uart_process_command(&sys, "HELLO");
```

每执行一条控制命令后，再通过 `STATUS` 检查系统状态是否正确变化。

---

## 14. 编译运行

进入 Day16 目录：

```bash
cd /root/Embedded_14Days/day16
```

清理旧编译文件：

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

---

## 15. 运行结果

程序能够正确实现：

```text
STATUS
打印初始系统状态。

LED ON
LED 状态变为 1，GPIO 第 0 位置 1。

LED OFF
LED 状态变为 0，GPIO 第 0 位清零。

LED TOGGLE
LED 从关闭状态翻转为打开状态。

RESET
系统恢复 INIT，GPIO 和统计数据清零。

HELLO
输出 Unknown command。
```

---

## 16. 调试过程中发现的问题

第一次执行 `LED OFF` 后出现：

```text
LED state: 0
GPIO output reg: 0x00000001
LED pin level: 1
```

这说明软件状态已经变为关闭，但模拟 GPIO 位没有真正清零。

检查 `gpio_led_off()` 后，将核心代码修正为：

```c
sys->gpio_output_reg &= ~BIT(LED_GPIO_PIN);
```

修正后结果为：

```text
LED state: 0
GPIO output reg: 0x00000000
LED pin level: 0
```

这个问题说明，软件状态和模拟硬件状态需要同时检查，不能只看一个变量。

---

## 17. Day16 核心理解

Day16 最核心的知识是：

```text
UART 接收到字符串命令后，
程序需要比较命令内容，
再执行对应的控制操作。
```

核心字符串判断格式为：

```c
if(strcmp(command, "命令内容") == 0)
{
    /* 执行对应操作 */
}
```

当前流程仍是完整字符串模拟。

真实 UART 通常是一个字符一个字符到达，例如：

```text
'L'
'E'
'D'
' '
'O'
'N'
'\n'
```

因此下一步需要学习环形缓冲区，将逐个到达的数据先缓存起来，再组成完整命令。

---

## 18. 今日收获

通过 Day16 的学习，掌握了：

1. UART 命令处理模块的基本结构；
2. `const char *` 字符串参数；
3. `strcmp()` 字符串比较方法；
4. 根据命令执行不同分支；
5. 使用 UART 命令控制 GPIO；
6. 同步软件状态和模拟硬件状态；
7. RESET 系统复位逻辑；
8. 未知命令处理方法；
9. 调试 LED 状态与 GPIO 寄存器不一致的问题。

Day16 的核心可以总结为：

```text
Day15：使用位操作模拟 GPIO 输出
Day16：使用 UART 字符串命令控制系统
```
