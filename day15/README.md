# Day15：位操作、GPIO 寄存器模拟与 LED 控制

## 1. 学习目标

本项目用于学习嵌入式 C 中的位操作和 GPIO 寄存器控制思想。

前面 Day14 已经完成了一个较完整的软件框架：

```text
sensor_update
fault_check
state_machine_update
control_update
event_update
system_print
```

Day15 在 Day14 基础上继续扩展，引入 GPIO 模拟模块，用一个变量模拟单片机中的 GPIO 输出寄存器。

本次学习重点是：

```text
1. 理解位操作的基本作用
2. 理解 BIT(n) 宏的含义
3. 使用 unsigned int 模拟 GPIO 输出寄存器
4. 新增 gpio.h 和 gpio.c 模块
5. 使用置 1、清 0、翻转、读取操作控制 LED
```

Day15 的核心目标是从普通软件状态：

```text
LED state: 1
```

进一步过渡到类似硬件寄存器的表示：

```text
GPIO output reg: 0x00000001
LED pin level: 1
```

---

## 2. 工程结构

Day15 在 Day14 基础上新增了 GPIO 模拟模块。

```bash
day15
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
│   └── system_type.h
├── src
│   ├── control.c
│   ├── event.c
│   ├── fault.c
│   ├── gpio.c
│   ├── main.c
│   ├── sensor.c
│   ├── state_machine.c
│   └── system.c
└── build
    └── day15_test
```

相比 Day14，Day15 主要新增和修改了：

```text
新增 include/gpio.h
新增 src/gpio.c
修改 include/config.h
修改 include/system_type.h
修改 src/control.c
修改 src/system.c
修改 src/main.c
修改 Makefile
```

---

## 3. Day14 与 Day15 的区别

Day14 中，LED 只是一个普通软件状态：

```text
LED state: 1
LED state: 2
```

Day15 中，增加了模拟 GPIO 输出寄存器：

```c
unsigned int gpio_output_reg;
```

通过这个变量，可以模拟真实单片机中的 GPIO 输出寄存器。

例如：

```text
GPIO output reg: 0x00000000  表示第 0 位为 0，LED 灭
GPIO output reg: 0x00000001  表示第 0 位为 1，LED 亮
```

因此 Day15 的重点是：

```text
用位操作控制 gpio_output_reg 中的某一位。
```

---

## 4. config.h 增加 GPIO 位操作宏

Day15 在 `config.h` 中新增了两个宏：

```c
#define LED_GPIO_PIN 0
#define BIT(n) (1U << (n))
```

其中：

```text
LED_GPIO_PIN 0
```

表示 LED 使用 GPIO 输出寄存器的第 0 位。

```text
BIT(n)
```

用于生成第 n 位对应的掩码。

例如：

```text
BIT(0) = 0000 0001 = 0x00000001
BIT(1) = 0000 0010 = 0x00000002
BIT(2) = 0000 0100 = 0x00000004
BIT(3) = 0000 1000 = 0x00000008
```

完整相关配置如下：

```c
#define LED_STATE_OFF 0
#define LED_STATE_ON 1
#define LED_STATE_BLINK 2

#define LED_GPIO_PIN 0
#define BIT(n) (1U << (n))
```

这里的 `1U` 表示无符号整数 1，`<<` 是左移运算符。

---

## 5. system_type.h 增加 GPIO 输出寄存器变量

Day15 在 `SystemStatus` 结构体中新增：

```c
unsigned int gpio_output_reg;
```

完整结构体如下：

```c
typedef struct
{
    int led_state;
    unsigned int gpio_output_reg;

    SystemMode mode;
    SystemMode last_mode;

    float voltage;
    float current;
    float temperature;

    unsigned int fault_code;

    unsigned int cycle_count;
    unsigned int fault_cycle_count;
    unsigned int fault_enter_count;
} SystemStatus;
```

其中：

```c
unsigned int gpio_output_reg;
```

用于模拟 GPIO 输出寄存器。

可以简单理解为：

```text
gpio_output_reg 是一排虚拟开关。
每一位都可以代表一个 GPIO 引脚。
第 0 位现在用来控制 LED。
```

---

## 6. 新增 gpio.h

在 `include` 目录下新增 `gpio.h`。

```c
#ifndef GPIO_H
#define GPIO_H

#include "system_type.h"

void gpio_init(SystemStatus *sys);
void gpio_led_on(SystemStatus *sys);
void gpio_led_off(SystemStatus *sys);
void gpio_led_toggle(SystemStatus *sys);
int gpio_led_read(const SystemStatus *sys);

#endif
```

各函数作用如下：

```text
gpio_init：初始化 GPIO 模拟寄存器
gpio_led_on：设置 LED 位为 1
gpio_led_off：清除 LED 位为 0
gpio_led_toggle：翻转 LED 位
gpio_led_read：读取 LED 当前电平
```

---

## 7. 新增 gpio.c

在 `src` 目录下新增 `gpio.c`。

```c
#include "gpio.h"
#include "config.h"
#include "debug.h"

void gpio_init(SystemStatus *sys)
{
    sys->gpio_output_reg = 0;
    DEBUG_PRINT("gpio init done\n");
}

void gpio_led_on(SystemStatus *sys)
{
    sys->gpio_output_reg |= BIT(LED_GPIO_PIN);
    DEBUG_PRINT("gpio: LED bit set\n");
}

void gpio_led_off(SystemStatus *sys)
{
    sys->gpio_output_reg &= ~BIT(LED_GPIO_PIN);
    DEBUG_PRINT("gpio: LED bit clear\n");
}

void gpio_led_toggle(SystemStatus *sys)
{
    sys->gpio_output_reg ^= BIT(LED_GPIO_PIN);
    DEBUG_PRINT("gpio: LED bit toggle\n");
}

int gpio_led_read(const SystemStatus *sys)
{
    if(sys->gpio_output_reg & BIT(LED_GPIO_PIN))
    {
        return 1;
    }

    return 0;
}
```

---

## 8. gpio.c 核心位操作解释

### 8.1 LED 打开

```c
sys->gpio_output_reg |= BIT(LED_GPIO_PIN);
```

含义是：

```text
把 LED 对应的第 0 位置 1。
```

如果原来是：

```text
0000 0000
```

执行后变为：

```text
0000 0001
```

因此 LED 被打开。

---

### 8.2 LED 关闭

```c
sys->gpio_output_reg &= ~BIT(LED_GPIO_PIN);
```

含义是：

```text
把 LED 对应的第 0 位清 0。
```

如果原来是：

```text
0000 0001
```

执行后变为：

```text
0000 0000
```

因此 LED 被关闭。

---

### 8.3 LED 翻转

```c
sys->gpio_output_reg ^= BIT(LED_GPIO_PIN);
```

含义是：

```text
如果 LED 位原来是 1，就变成 0；
如果 LED 位原来是 0，就变成 1。
```

这可以用来模拟 LED 闪烁。

---

### 8.4 读取 LED 状态

```c
sys->gpio_output_reg & BIT(LED_GPIO_PIN)
```

含义是：

```text
检查 GPIO 输出寄存器中 LED 对应的那一位是不是 1。
```

如果是 1，函数返回 1；如果是 0，函数返回 0。

---

## 9. 修改 control.c

Day15 修改 `control.c`，让控制模块不仅修改 `led_state`，还调用 GPIO 模拟函数。

```c
#include "control.h"
#include "config.h"
#include "debug.h"
#include "gpio.h"

void control_update(SystemStatus *sys)
{
    switch(sys->mode)
    {
        case SYS_MODE_INIT:
            sys->led_state = LED_STATE_OFF;
            gpio_led_off(sys);
            DEBUG_PRINT("control: LED OFF in INIT mode\n");
            break;

        case SYS_MODE_RUN:
            sys->led_state = LED_STATE_ON;
            gpio_led_on(sys);
            DEBUG_PRINT("control: LED ON in RUN mode\n");
            break;

        case SYS_MODE_FAULT:
            sys->led_state = LED_STATE_BLINK;
            gpio_led_toggle(sys);
            DEBUG_PRINT("control: LED BLINK in FAULT mode\n");
            break;

        default:
            sys->led_state = LED_STATE_OFF;
            gpio_led_off(sys);
            DEBUG_PRINT("control: LED OFF in UNKNOWN mode\n");
            break;
    }
}
```

对应关系为：

```text
INIT  状态 -> LED OFF   -> GPIO 第 0 位清 0
RUN   状态 -> LED ON    -> GPIO 第 0 位置 1
FAULT 状态 -> LED BLINK -> GPIO 第 0 位翻转
```

---

## 10. 修改 system.c

Day15 在 `system.c` 中增加 GPIO 寄存器初始化和打印。

在 `system_init()` 中新增：

```c
sys->gpio_output_reg = 0;
```

表示系统启动时，GPIO 输出寄存器清零。

在 `system_print()` 中新增：

```c
printf("GPIO output reg: 0x%08X\n", sys->gpio_output_reg);
printf("LED pin level: %u\n",
       (sys->gpio_output_reg & BIT(LED_GPIO_PIN)) ? 1U : 0U);
```

其中：

```text
GPIO output reg
```

用于显示整个模拟 GPIO 输出寄存器的值。

```text
LED pin level
```

用于显示 LED 对应 GPIO 位的实际电平。

---

## 11. 修改 main.c

Day15 在 `main.c` 中增加 GPIO 初始化。

新增头文件：

```c
#include "gpio.h"
```

在系统初始化后调用：

```c
gpio_init(&sys);
```

完整流程如下：

```c
#include <stdio.h>
#include "system_type.h"
#include "system.h"
#include "sensor.h"
#include "fault.h"
#include "state_machine.h"
#include "control.h"
#include "event.h"
#include "gpio.h"

int main(void)
{
    SystemStatus sys;

    system_init(&sys);
    gpio_init(&sys);

    for(int cycle = 0; cycle < 5; cycle++)
    {
        printf("\n===== Cycle %d =====\n", cycle);

        sensor_update(&sys, cycle);
        fault_check(&sys);
        state_machine_update(&sys);
        control_update(&sys);
        event_update(&sys);
        system_print(&sys);
    }

    return 0;
}
```

Day15 的主循环仍然是：

```text
sensor_update
fault_check
state_machine_update
control_update
event_update
system_print
```

只是 `control_update()` 内部开始真正操作模拟 GPIO 寄存器。

---

## 12. Makefile 修改

Day15 从 Day14 复制而来，因此需要修改目标文件名：

```makefile
TARGET = build/day15_test
```

由于 Makefile 使用：

```makefile
SRCS = $(wildcard src/*.c)
```

所以新增的：

```text
src/gpio.c
```

会自动参与编译。

---

## 13. 编译运行

进入 Day15 目录：

```bash
cd /root/Embedded_14Days/day15
```

清理旧文件：

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

也可以只查看关键输出：

```bash
make run | grep -E "=====|state:|control:|gpio:|GPIO output reg|LED pin level|LED state|System mode"
```

---

## 14. 运行结果

关键运行结果如下：

```text
===== Cycle 0 =====
[DEBUG] state: INIT -> RUN
[DEBUG] gpio: LED bit set
[DEBUG] control: LED ON in RUN mode
LED state: 1
GPIO output reg: 0x00000001
LED pin level: 1
System mode: RUN

===== Cycle 1 =====
[DEBUG] state: RUN keep
[DEBUG] gpio: LED bit set
[DEBUG] control: LED ON in RUN mode
LED state: 1
GPIO output reg: 0x00000001
LED pin level: 1
System mode: RUN

===== Cycle 2 =====
[DEBUG] state: RUN -> FAULT
[DEBUG] gpio: LED bit toggle
[DEBUG] control: LED BLINK in FAULT mode
LED state: 2
GPIO output reg: 0x00000000
LED pin level: 0
System mode: FAULT

===== Cycle 3 =====
[DEBUG] state: FAULT keep
[DEBUG] gpio: LED bit toggle
[DEBUG] control: LED BLINK in FAULT mode
LED state: 2
GPIO output reg: 0x00000001
LED pin level: 1
System mode: FAULT

===== Cycle 4 =====
[DEBUG] state: FAULT -> RUN
[DEBUG] gpio: LED bit set
[DEBUG] control: LED ON in RUN mode
LED state: 1
GPIO output reg: 0x00000001
LED pin level: 1
System mode: RUN
```

---

## 15. 运行结果分析

### Cycle 0

系统从 `INIT` 进入 `RUN`：

```text
state: INIT -> RUN
```

控制模块执行：

```text
gpio: LED bit set
```

GPIO 输出寄存器变为：

```text
GPIO output reg: 0x00000001
```

说明第 0 位被置 1，LED 当前为亮。

---

### Cycle 1

系统保持 `RUN` 状态：

```text
state: RUN keep
```

LED 继续保持打开：

```text
GPIO output reg: 0x00000001
LED pin level: 1
```

---

### Cycle 2

系统从 `RUN` 进入 `FAULT`：

```text
state: RUN -> FAULT
```

控制模块执行：

```text
gpio: LED bit toggle
```

GPIO 输出寄存器从：

```text
0x00000001
```

翻转为：

```text
0x00000000
```

LED 当前为灭。

---

### Cycle 3

系统保持 `FAULT` 状态：

```text
state: FAULT keep
```

控制模块再次执行：

```text
gpio: LED bit toggle
```

GPIO 输出寄存器从：

```text
0x00000000
```

翻转为：

```text
0x00000001
```

LED 当前为亮。

---

### Cycle 4

系统从 `FAULT` 恢复到 `RUN`：

```text
state: FAULT -> RUN
```

控制模块执行：

```text
gpio: LED bit set
```

GPIO 输出寄存器保持：

```text
0x00000001
```

LED 恢复常亮状态。

---

## 16. Day15 核心理解

Day15 最核心的理解是：

```text
GPIO 控制本质上就是控制寄存器中的某一位。
```

本项目中：

```text
gpio_output_reg 模拟 GPIO 输出寄存器
LED_GPIO_PIN 表示 LED 使用第 0 位
BIT(LED_GPIO_PIN) 用于找到 LED 对应的那一位
```

核心位操作为：

```c
reg |= BIT(n);     // 把第 n 位置 1
reg &= ~BIT(n);    // 把第 n 位清 0
reg ^= BIT(n);     // 把第 n 位翻转
reg & BIT(n);      // 读取第 n 位
```

这些操作是以后学习 STM32、GD32、ARM 寄存器和外设驱动的基础。

---

## 17. 今日收获

通过 Day15 的学习，主要掌握了以下内容：

1. 理解了 GPIO 寄存器模拟的基本思想；
2. 学会了使用 `unsigned int` 模拟 GPIO 输出寄存器；
3. 理解了 `BIT(n)` 宏和左移运算；
4. 学会了使用位操作实现 LED 打开、关闭、翻转和读取；
5. 理解了 `|=`、`&= ~`、`^=`、`&` 在寄存器控制中的作用；
6. 学会了新增 `gpio.h` 和 `gpio.c` 模块；
7. 理解了控制模块如何调用 GPIO 模块；
8. 从软件状态控制进一步过渡到寄存器控制思想。

Day15 的核心可以总结为：

```text
Day14：统计系统运行事件
Day15：用 GPIO 寄存器模拟 LED 控制
```

这一步标志着第二阶段开始：从纯软件状态机过渡到无硬件外设模拟。

---

## 18. 项目源码

Day15 代码位于：

```text
Embedded-C-Learning-Projects/day15
```

GitHub 路径为：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects/tree/main/day15
```
