# Day13：控制执行模块、LED 状态控制与主循环动作输出

## 1. 学习目标

本项目用于学习嵌入式 C 工程中的控制执行模块设计方法。

相比 Day12 中已经实现了主循环、多周期传感器数据变化和状态机连续转移，Day13 进一步增加 `control` 控制模块，根据系统当前模式 `mode` 控制 LED 输出状态。

Day13 的核心思想是：

```text
state_machine.c：负责判断系统处于什么状态
control.c：负责根据系统状态执行控制动作
```

通过本次学习，可以进一步理解真实嵌入式系统中“状态判断”和“控制执行”分离的设计思想。

---

## 2. 工程结构

Day13 在 Day12 基础上继续修改，新增了 `control.h` 和 `control.c`。

```bash
day13
├── Makefile
├── include
│   ├── config.h
│   ├── control.h
│   ├── debug.h
│   ├── fault_code.h
│   ├── fault.h
│   ├── sensor.h
│   ├── state_machine.h
│   ├── system.h
│   └── system_type.h
├── src
│   ├── control.c
│   ├── fault.c
│   ├── main.c
│   ├── sensor.c
│   ├── state_machine.c
│   └── system.c
└── build
    └── day13_test
```

其中：

- `control.h`：声明控制更新函数；
- `control.c`：根据系统模式控制 LED 状态；
- `state_machine.c`：根据故障码更新系统模式；
- `fault.c`：负责故障检测；
- `sensor.c`：负责周期性模拟传感器数据；
- `system.c`：负责系统初始化和状态打印；
- `config.h`：统一管理系统配置、传感器数据和 LED 状态宏。

---

## 3. Day12 与 Day13 的区别

Day12 中，程序已经可以连续运行多个周期，并观察状态机变化：

```text
INIT -> RUN
RUN keep
RUN -> FAULT
FAULT keep
FAULT -> RUN
```

但是 Day12 主要停留在“状态判断”和“状态打印”层面。

Day13 增加了控制执行模块：

```text
RUN 状态   -> LED ON
FAULT 状态 -> LED BLINK
INIT 状态  -> LED OFF
```

也就是说，Day13 开始让状态机结果影响实际控制输出。

---

## 4. config.h 增加 LED 状态宏

在 `config.h` 中增加 LED 状态定义：

```c
#define LED_STATE_OFF 0
#define LED_STATE_ON 1
#define LED_STATE_BLINK 2
```

含义如下：

```text
LED_STATE_OFF   表示 LED 熄灭
LED_STATE_ON    表示 LED 常亮
LED_STATE_BLINK 表示 LED 闪烁或报警状态
```

完整相关配置如下：

```c
#ifndef CONFIG_H
#define CONFIG_H

#define SYS_INIT_LED_STATE 0
#define SYS_INIT_VOLTAGE 12.5f
#define SYS_INIT_CURRENT 1.2f
#define SYS_INIT_TEMPERATURE 35.6f

#define LED_STATE_OFF 0
#define LED_STATE_ON 1
#define LED_STATE_BLINK 2

#define SENSOR_SIM_VOLTAGE 9.5f
#define SENSOR_SIM_CURRENT 2.5f
#define SENSOR_SIM_TEMPERATURE 75.0f

#define SENSOR_NORMAL_VOLTAGE 12.0f
#define SENSOR_NORMAL_CURRENT 1.0f
#define SENSOR_NORMAL_TEMPERATURE 35.0f

#define SENSOR_FAULT_VOLTAGE 9.5f
#define SENSOR_FAULT_CURRENT 2.5f
#define SENSOR_FAULT_TEMPERATURE 75.0f

#define FAULT_LOW_VOLTAGE_TH 10.0f
#define FAULT_OVER_CURRENT_TH 2.0f
#define FAULT_OVER_TEMP_TH 60.0f

#endif
```

---

## 5. 新增 control.h

在 `include` 目录下新增：

```text
control.h
```

内容如下：

```c
#ifndef CONTROL_H
#define CONTROL_H

#include "system_type.h"

void control_update(SystemStatus *sys);

#endif
```

该头文件声明了控制更新函数：

```c
void control_update(SystemStatus *sys);
```

该函数用于根据当前系统模式更新控制输出，例如 LED 状态。

---

## 6. 新增 control.c

在 `src` 目录下新增：

```text
control.c
```

内容如下：

```c
#include "control.h"
#include "config.h"
#include "debug.h"

void control_update(SystemStatus *sys)
{
    switch(sys->mode)
    {
        case SYS_MODE_INIT:
            sys->led_state = LED_STATE_OFF;
            DEBUG_PRINT("control: LED OFF in INIT mode\n");
            break;

        case SYS_MODE_RUN:
            sys->led_state = LED_STATE_ON;
            DEBUG_PRINT("control: LED ON in RUN mode\n");
            break;

        case SYS_MODE_FAULT:
            sys->led_state = LED_STATE_BLINK;
            DEBUG_PRINT("control: LED BLINK in FAULT mode\n");
            break;

        default:
            sys->led_state = LED_STATE_OFF;
            DEBUG_PRINT("control: LED OFF in UNKNOWN mode\n");
            break;
    }
}
```

这段代码的作用是：

```text
根据 sys->mode 的不同值，设置 sys->led_state。
```

对应关系为：

```text
SYS_MODE_INIT  -> LED_STATE_OFF
SYS_MODE_RUN   -> LED_STATE_ON
SYS_MODE_FAULT -> LED_STATE_BLINK
```

---

## 7. control_update() 逻辑解释

### 7.1 INIT 模式

```c
case SYS_MODE_INIT:
    sys->led_state = LED_STATE_OFF;
    DEBUG_PRINT("control: LED OFF in INIT mode\n");
    break;
```

系统处于初始化模式时，LED 设置为熄灭状态。

---

### 7.2 RUN 模式

```c
case SYS_MODE_RUN:
    sys->led_state = LED_STATE_ON;
    DEBUG_PRINT("control: LED ON in RUN mode\n");
    break;
```

系统处于正常运行模式时，LED 设置为常亮。

---

### 7.3 FAULT 模式

```c
case SYS_MODE_FAULT:
    sys->led_state = LED_STATE_BLINK;
    DEBUG_PRINT("control: LED BLINK in FAULT mode\n");
    break;
```

系统处于故障模式时，LED 设置为闪烁状态，用于模拟故障报警。

---

### 7.4 UNKNOWN 模式

```c
default:
    sys->led_state = LED_STATE_OFF;
    DEBUG_PRINT("control: LED OFF in UNKNOWN mode\n");
    break;
```

如果系统模式出现异常值，默认关闭 LED。

---

## 8. main.c 调用流程

Day13 的 `main.c` 中需要包含控制模块头文件：

```c
#include "control.h"
```

完整主流程如下：

```c
#include <stdio.h>
#include "system_type.h"
#include "system.h"
#include "sensor.h"
#include "fault.h"
#include "state_machine.h"
#include "control.h"

int main(void)
{
    SystemStatus sys;

    system_init(&sys);

    for(int cycle = 0; cycle < 5; cycle++)
    {
        printf("\n===== Cycle %d =====\n", cycle);

        sensor_update(&sys, cycle);
        fault_check(&sys);
        state_machine_update(&sys);
        control_update(&sys);
        system_print(&sys);
    }

    return 0;
}
```

相比 Day12，新增了：

```c
control_update(&sys);
```

该函数放在：

```c
state_machine_update(&sys);
```

后面。

原因是控制模块需要根据最新的系统模式执行控制动作。

---

## 9. 主循环执行顺序

Day13 的主循环顺序为：

```text
1. sensor_update：更新传感器数据
2. fault_check：检测故障并生成 fault_code
3. state_machine_update：根据 fault_code 更新系统 mode
4. control_update：根据 mode 更新 led_state
5. system_print：打印系统状态
```

对应代码为：

```c
sensor_update(&sys, cycle);
fault_check(&sys);
state_machine_update(&sys);
control_update(&sys);
system_print(&sys);
```

这个顺序不能随便乱。

如果把 `control_update()` 放在 `state_machine_update()` 前面，那么控制模块拿到的就是旧的系统模式，LED 状态可能无法及时反映最新故障状态。

---

## 10. Makefile 修改

Day13 从 Day12 复制而来，因此需要将 Makefile 中的目标文件名修改为：

```makefile
TARGET = build/day13_test
```

由于 Makefile 中使用了自动查找源文件：

```makefile
SRCS = $(wildcard src/*.c)
```

因此新增的：

```text
src/control.c
```

会自动参与编译。

---

## 11. 编译运行

进入 Day13 目录：

```bash
cd /root/Embedded_14Days/day13
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

运行结果中可以看到：

```text
[DEBUG] control: LED ON in RUN mode
[DEBUG] control: LED BLINK in FAULT mode
```

这说明 `control_update()` 已经被成功调用。

---

## 12. 运行结果

完整运行结果中，关键变化如下：

```text
===== Cycle 0 =====
[DEBUG] state: INIT -> RUN
[DEBUG] control: LED ON in RUN mode
LED state: 1
System mode: RUN

===== Cycle 1 =====
[DEBUG] state: RUN keep
[DEBUG] control: LED ON in RUN mode
LED state: 1
System mode: RUN

===== Cycle 2 =====
[DEBUG] state: RUN -> FAULT
[DEBUG] control: LED BLINK in FAULT mode
LED state: 2
System mode: FAULT

===== Cycle 3 =====
[DEBUG] state: FAULT keep
[DEBUG] control: LED BLINK in FAULT mode
LED state: 2
System mode: FAULT

===== Cycle 4 =====
[DEBUG] state: FAULT -> RUN
[DEBUG] control: LED ON in RUN mode
LED state: 1
System mode: RUN
```

---

## 13. 运行结果分析

### 13.1 Cycle 0

Cycle 0 中，传感器数据为正常值，系统从 `INIT` 进入 `RUN`：

```text
state: INIT -> RUN
```

控制模块根据 `RUN` 模式设置：

```text
LED state: 1
```

即 LED 常亮。

---

### 13.2 Cycle 1

Cycle 1 中，系统仍然正常：

```text
state: RUN keep
```

控制模块继续保持：

```text
LED state: 1
```

---

### 13.3 Cycle 2

Cycle 2 中，传感器数据变为故障值，系统从 `RUN` 切换到 `FAULT`：

```text
state: RUN -> FAULT
```

控制模块根据 `FAULT` 模式设置：

```text
LED state: 2
```

即 LED 闪烁，模拟故障报警。

---

### 13.4 Cycle 3

Cycle 3 中，故障仍然存在：

```text
state: FAULT keep
```

控制模块继续保持：

```text
LED state: 2
```

---

### 13.5 Cycle 4

Cycle 4 中，传感器恢复正常，系统从 `FAULT` 恢复到 `RUN`：

```text
state: FAULT -> RUN
```

控制模块将 LED 重新设置为：

```text
LED state: 1
```

即 LED 常亮。

---

## 14. 今日收获

通过 Day13 的学习，主要掌握了以下内容：

1. 理解了控制执行模块的作用；
2. 学会了新增 `control.h` 和 `control.c`；
3. 学会了根据系统模式控制 LED 状态；
4. 理解了状态机模块和控制模块的职责区别；
5. 理解了主循环中“状态更新”和“动作执行”的顺序；
6. 进一步理解了真实嵌入式系统中状态机和控制输出之间的关系。

Day13 的核心可以总结为：

```text
state_machine.c 负责决定系统状态；
control.c 负责根据系统状态执行控制动作。
```

这一步让程序从“状态判断”进一步发展到“根据状态执行控制”，更接近真实嵌入式项目结构。