cat > day14/README.md <<'EOF'
# Day14：事件统计模块、故障次数记录与嵌入式小项目收尾

## 1. 学习目标

本项目用于学习嵌入式 C 工程中的事件统计模块设计方法。

相比 Day13 中已经实现了控制执行模块，Day14 进一步增加 `event` 事件统计模块，用于记录系统运行过程中的关键事件，例如系统运行周期数、故障周期数和进入故障次数。

Day14 的核心目标是理解：

```text
状态机负责状态判断
控制模块负责执行动作
事件模块负责统计运行事件
```

通过本次学习，可以进一步理解真实嵌入式系统中“状态判断、动作执行、事件记录”分离的模块化设计思想。

---

## 2. 工程结构

Day14 在 Day13 基础上继续修改，新增了 `event.h` 和 `event.c`。

```bash
day14
├── Makefile
├── include
│   ├── config.h
│   ├── control.h
│   ├── debug.h
│   ├── event.h
│   ├── fault_code.h
│   ├── fault.h
│   ├── sensor.h
│   ├── state_machine.h
│   ├── system.h
│   └── system_type.h
├── src
│   ├── control.c
│   ├── event.c
│   ├── fault.c
│   ├── main.c
│   ├── sensor.c
│   ├── state_machine.c
│   └── system.c
└── build
    └── day14_test
```

其中：

```text
event.h：声明事件统计函数
event.c：统计系统运行周期、故障周期和进入故障次数
system_type.h：增加事件统计相关变量
system.c：初始化和打印事件统计信息
main.c：在主循环中调用 event_update()
```

---

## 3. Day13 与 Day14 的区别

Day13 中，主循环已经包含控制执行模块：

```c
sensor_update(&sys, cycle);
fault_check(&sys);
state_machine_update(&sys);
control_update(&sys);
system_print(&sys);
```

Day13 可以实现：

```text
RUN 状态   -> LED ON
FAULT 状态 -> LED BLINK
```

Day14 在此基础上新增事件统计模块：

```c
event_update(&sys);
```

因此主循环变为：

```c
sensor_update(&sys, cycle);
fault_check(&sys);
state_machine_update(&sys);
control_update(&sys);
event_update(&sys);
system_print(&sys);
```

Day14 不仅可以判断状态和执行控制动作，还可以记录系统运行过程中的关键统计信息。

---

## 4. system_type.h 增加统计变量

Day14 在 `SystemStatus` 结构体中增加了事件统计相关成员。

完整结构体如下：

```c
#ifndef SYSTEM_TYPE_H
#define SYSTEM_TYPE_H

typedef enum
{
    SYS_MODE_INIT = 0,
    SYS_MODE_RUN,
    SYS_MODE_FAULT
} SystemMode;

typedef struct
{
    int led_state;
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

#endif
```

相比 Day13，新增了以下成员：

```c
SystemMode last_mode;

unsigned int cycle_count;
unsigned int fault_cycle_count;
unsigned int fault_enter_count;
```

含义如下：

```text
last_mode：上一轮系统模式
cycle_count：系统总运行周期数
fault_cycle_count：系统处于 FAULT 状态的周期数
fault_enter_count：系统从非故障状态进入故障状态的次数
```

---

## 5. system_init() 初始化统计变量

在 `system.c` 中，需要对新增的统计变量进行初始化。

```c
void system_init(SystemStatus *sys)
{
    sys->led_state = SYS_INIT_LED_STATE;
    sys->mode = SYS_MODE_INIT;
    sys->last_mode = SYS_MODE_INIT;

    sys->voltage = SYS_INIT_VOLTAGE;
    sys->current = SYS_INIT_CURRENT;
    sys->temperature = SYS_INIT_TEMPERATURE;

    sys->fault_code = FAULT_NONE;

    sys->cycle_count = 0;
    sys->fault_cycle_count = 0;
    sys->fault_enter_count = 0;

    DEBUG_PRINT("system init done\n");
}
```

其中：

```c
sys->last_mode = SYS_MODE_INIT;
sys->cycle_count = 0;
sys->fault_cycle_count = 0;
sys->fault_enter_count = 0;
```

表示系统刚启动时，上一状态为 `INIT`，所有统计计数清零。

---

## 6. system_print() 增加统计信息打印

Day14 在 `system_print()` 中增加了事件统计信息输出。

```c
printf("Cycle count: %u\n", sys->cycle_count);
printf("Fault cycle count: %u\n", sys->fault_cycle_count);
printf("Fault enter count: %u\n", sys->fault_enter_count);
```

这样每一轮运行后，都可以看到当前累计统计结果。

---

## 7. 新增 event.h

在 `include` 目录下新增 `event.h`。

```c
#ifndef EVENT_H
#define EVENT_H

#include "system_type.h"

void event_update(SystemStatus *sys);

#endif
```

该头文件声明了事件统计函数：

```c
void event_update(SystemStatus *sys);
```

它的作用是根据当前系统状态更新事件统计信息。

---

## 8. 新增 event.c

在 `src` 目录下新增 `event.c`。

完整代码如下：

```c
#include "event.h"
#include "fault_code.h"
#include "debug.h"

void event_update(SystemStatus *sys)
{
    sys->cycle_count++;

    if(sys->mode == SYS_MODE_FAULT)
    {
        sys->fault_cycle_count++;
    }

    if((sys->last_mode != SYS_MODE_FAULT) && (sys->mode == SYS_MODE_FAULT))
    {
        sys->fault_enter_count++;
        DEBUG_PRINT("event: enter FAULT count +1\n");
    }

    sys->last_mode = sys->mode;

    DEBUG_PRINT("event update done\n");
}
```

---

## 9. event_update() 逻辑解释

### 9.1 统计总运行周期数

```c
sys->cycle_count++;
```

每执行一轮主循环，系统运行周期数加 1。

当前程序一共运行 5 个周期，所以最终：

```text
Cycle count = 5
```

---

### 9.2 统计故障周期数

```c
if(sys->mode == SYS_MODE_FAULT)
{
    sys->fault_cycle_count++;
}
```

只要当前周期系统处于 `FAULT` 状态，故障周期数就加 1。

当前 Day14 的运行过程为：

```text
Cycle 0：RUN
Cycle 1：RUN
Cycle 2：FAULT
Cycle 3：FAULT
Cycle 4：RUN
```

其中 Cycle 2 和 Cycle 3 处于 `FAULT` 状态，所以最终：

```text
Fault cycle count = 2
```

---

### 9.3 统计进入故障次数

```c
if((sys->last_mode != SYS_MODE_FAULT) && (sys->mode == SYS_MODE_FAULT))
{
    sys->fault_enter_count++;
    DEBUG_PRINT("event: enter FAULT count +1\n");
}
```

这段代码表示：

```text
上一轮不是 FAULT
当前轮变成 FAULT
```

才算真正进入故障一次。

当前状态变化为：

```text
Cycle 0：INIT -> RUN
Cycle 1：RUN keep
Cycle 2：RUN -> FAULT
Cycle 3：FAULT keep
Cycle 4：FAULT -> RUN
```

只有 Cycle 2 是从 `RUN` 进入 `FAULT`，所以：

```text
Fault enter count = 1
```

---

### 9.4 更新上一状态

```c
sys->last_mode = sys->mode;
```

事件统计完成后，将当前模式保存为 `last_mode`，供下一轮判断状态变化使用。

这个变量非常关键。

如果没有 `last_mode`，程序只能知道当前是否处于故障状态，无法判断是不是“刚刚进入故障”。

---

## 10. fault_cycle_count 和 fault_enter_count 的区别

Day14 最重要的理解是区分：

```text
fault_cycle_count：故障周期数
fault_enter_count：进入故障次数
```

它们不是一个概念。

例如当前程序中：

```text
Cycle 2：FAULT
Cycle 3：FAULT
```

系统在故障状态下停留了 2 个周期，所以：

```text
fault_cycle_count = 2
```

但是系统只在 Cycle 2 发生了一次：

```text
RUN -> FAULT
```

所以：

```text
fault_enter_count = 1
```

可以理解为：

```text
故障持续多久，统计的是 fault_cycle_count
故障触发了几次，统计的是 fault_enter_count
```

真实嵌入式系统中，这个区别很重要。

例如一个故障持续了 10 秒，并不代表报警触发了 10 次。只有系统从正常状态进入故障状态的那一刻，才算真正触发一次故障进入事件。

---

## 11. main.c 主循环修改

Day14 的 `main.c` 中需要包含事件模块头文件：

```c
#include "event.h"
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
#include "event.h"

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
        event_update(&sys);
        system_print(&sys);
    }

    return 0;
}
```

相比 Day13，新增了：

```c
event_update(&sys);
```

---

## 12. Day14 主循环执行顺序

Day14 的主循环执行顺序为：

```text
1. sensor_update：更新传感器数据
2. fault_check：检测故障并生成 fault_code
3. state_machine_update：根据 fault_code 更新系统 mode
4. control_update：根据 mode 更新 led_state
5. event_update：统计运行事件
6. system_print：打印系统状态和统计结果
```

对应代码为：

```c
sensor_update(&sys, cycle);
fault_check(&sys);
state_machine_update(&sys);
control_update(&sys);
event_update(&sys);
system_print(&sys);
```

这个顺序不能随便乱。

`event_update()` 要放在 `state_machine_update()` 后面，因为事件统计需要根据更新后的系统模式进行判断。

---

## 13. Makefile 修改

Day14 从 Day13 复制而来，因此需要修改 Makefile 中的目标文件名。

将：

```makefile
TARGET = build/day13_test
```

修改为：

```makefile
TARGET = build/day14_test
```

由于 Makefile 使用自动查找源文件：

```makefile
SRCS = $(wildcard src/*.c)
```

所以新增的：

```text
src/event.c
```

会自动参与编译。

---

## 14. 编译运行

进入 Day14 目录：

```bash
cd /root/Embedded_14Days/day14
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

如果只想查看状态变化和统计结果，可以执行：

```bash
make run | grep -E "=====|state:|control:|event:|Cycle count|Fault cycle count|Fault enter count"
```

---

## 15. 运行结果

关键运行结果如下：

```text
===== Cycle 0 =====
[DEBUG] state: INIT -> RUN
[DEBUG] control: LED ON in RUN mode
[DEBUG] event update done
Cycle count: 1
Fault cycle count: 0
Fault enter count: 0

===== Cycle 1 =====
[DEBUG] state: RUN keep
[DEBUG] control: LED ON in RUN mode
[DEBUG] event update done
Cycle count: 2
Fault cycle count: 0
Fault enter count: 0

===== Cycle 2 =====
[DEBUG] state: RUN -> FAULT
[DEBUG] control: LED BLINK in FAULT mode
[DEBUG] event: enter FAULT count +1
[DEBUG] event update done
Cycle count: 3
Fault cycle count: 1
Fault enter count: 1

===== Cycle 3 =====
[DEBUG] state: FAULT keep
[DEBUG] control: LED BLINK in FAULT mode
[DEBUG] event update done
Cycle count: 4
Fault cycle count: 2
Fault enter count: 1

===== Cycle 4 =====
[DEBUG] state: FAULT -> RUN
[DEBUG] control: LED ON in RUN mode
[DEBUG] event update done
Cycle count: 5
Fault cycle count: 2
Fault enter count: 1
```

最终统计结果为：

```text
Cycle count: 5
Fault cycle count: 2
Fault enter count: 1
```

---

## 16. 运行结果分析

当前程序的 5 个周期分别为：

```text
Cycle 0：正常数据，INIT -> RUN
Cycle 1：正常数据，RUN keep
Cycle 2：故障数据，RUN -> FAULT
Cycle 3：故障数据，FAULT keep
Cycle 4：正常数据，FAULT -> RUN
```

所以：

```text
系统总共运行 5 个周期
系统在 Cycle 2 和 Cycle 3 处于 FAULT 状态
系统只在 Cycle 2 进入过一次 FAULT 状态
```

因此最终统计结果为：

```text
Cycle count = 5
Fault cycle count = 2
Fault enter count = 1
```

---

## 17. Day14 的核心理解

Day14 最重要的理解是：

```text
事件统计不是简单地看当前状态，而是要结合当前状态和上一状态。
```

其中：

```c
sys->mode
```

表示当前状态。

```c
sys->last_mode
```

表示上一轮状态。

通过比较两者，就可以判断系统是否发生了状态切换。

例如：

```c
if((sys->last_mode != SYS_MODE_FAULT) && (sys->mode == SYS_MODE_FAULT))
```

这句代码就是在判断：

```text
系统是否刚刚进入 FAULT 状态
```

这种写法在真实嵌入式项目中很常见，比如：

```text
故障进入事件
故障恢复事件
按键按下事件
通信断开事件
电机启动事件
电机停止事件
```

本质上都可以用“当前状态”和“上一状态”进行判断。

---

## 18. 第一阶段小结

到 Day14 为止，已经完成了一个简化嵌入式 C 小项目。

项目主线从最基础的 C 程序逐步扩展到多模块工程：

```text
system：系统初始化和状态打印
sensor：传感器数据模拟
fault：故障检测
state_machine：系统状态机
control：控制执行
event：事件统计
debug：调试日志
config：参数配置
```

主循环流程也逐步完善为：

```text
system_init
sensor_update
fault_check
state_machine_update
control_update
event_update
system_print
```

这已经具备了一个嵌入式应用程序的基本框架。

---

## 19. 今日收获

通过 Day14 的学习，主要掌握了以下内容：

1. 学会了新增 `event.h` 和 `event.c` 事件统计模块；
2. 学会了在结构体中增加运行统计变量；
3. 理解了 `last_mode` 的作用；
4. 理解了故障周期数和进入故障次数的区别；
5. 学会了通过当前状态和上一状态判断状态切换事件；
6. 进一步理解了嵌入式系统中事件统计模块的作用；
7. 完成了 14 天嵌入式 C 第一阶段小项目收尾。

Day14 的核心可以总结为：

```text
Day13：根据系统状态执行控制动作
Day14：根据系统状态变化统计运行事件
```

这一步让程序从“判断状态、执行动作”进一步发展到“记录事件、分析运行过程”。

---

## 20. 项目源码

Day14 代码位于：

```text
Embedded-C-Learning-Projects/day14
```

GitHub 路径为：

```text
https://github.com/jdai10590-afk/Embedded-C-Learning-Projects/tree/main/day14
```