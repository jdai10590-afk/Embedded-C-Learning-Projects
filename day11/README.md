# Day11：状态机升级、当前状态判断与状态转移日志

## 1. 学习目标

本项目用于学习嵌入式 C 工程中更完整的状态机设计方法。

相比 Day10 中直接根据 `fault_code` 判断系统进入 `RUN` 或 `FAULT` 模式，Day11 进一步引入“当前状态”的概念，使状态机根据当前系统模式 `mode` 和故障码 `fault_code` 共同决定下一步状态。

Day11 的核心变化是：状态机不再只是简单判断是否有故障，而是通过 `switch(sys->mode)` 判断当前状态，并根据不同状态执行不同的状态转移逻辑。

## 2. 工程结构

```bash
day11
├── Makefile
├── include
│   ├── config.h
│   ├── debug.h
│   ├── fault_code.h
│   ├── fault.h
│   ├── sensor.h
│   ├── state_machine.h
│   ├── system.h
│   └── system_type.h
├── src
│   ├── fault.c
│   ├── main.c
│   ├── sensor.c
│   ├── state_machine.c
│   └── system.c
└── build
    └── day11_test
```

## 3. Day10 与 Day11 的区别

Day10 中，状态机逻辑比较简单：

```c
if(sys->fault_code == FAULT_NONE)
{
    sys->mode = SYS_MODE_RUN;
}
else
{
    sys->mode = SYS_MODE_FAULT;
}
```

这种方式只根据故障码判断系统模式：

```text
无故障 -> RUN
有故障 -> FAULT
```

Day11 中，状态机升级为基于当前状态进行判断：

```c
switch(sys->mode)
```

也就是说，系统会先判断当前处于 `INIT`、`RUN` 还是 `FAULT` 状态，再根据故障码决定下一步状态。

Day11 的状态转移逻辑如下：

```text
INIT  + 无故障 -> RUN
INIT  + 有故障 -> FAULT
RUN   + 无故障 -> 保持 RUN
RUN   + 有故障 -> FAULT
FAULT + 无故障 -> RUN
FAULT + 有故障 -> 保持 FAULT
```

## 4. 系统模式定义

系统模式仍然定义在 `system_type.h` 中：

```c
typedef enum
{
    SYS_MODE_INIT = 0,
    SYS_MODE_RUN,
    SYS_MODE_FAULT
} SystemMode;
```

三个模式分别表示：

```text
SYS_MODE_INIT   初始化模式
SYS_MODE_RUN    正常运行模式
SYS_MODE_FAULT  故障模式
```

系统状态结构体如下：

```c
typedef struct
{
    int led_state;
    SystemMode mode;
    float voltage;
    float current;
    float temperature;
    unsigned int fault_code;
} SystemStatus;
```

其中：

```c
SystemMode mode;
```

用于保存当前系统工作模式。

## 5. 系统初始化

在 `system.c` 中，系统初始化时默认进入 `SYS_MODE_INIT` 模式：

```c
void system_init(SystemStatus *sys)
{
    sys->led_state = SYS_INIT_LED_STATE;
    sys->mode = SYS_MODE_INIT;
    sys->voltage = SYS_INIT_VOLTAGE;
    sys->current = SYS_INIT_CURRENT;
    sys->temperature = SYS_INIT_TEMPERATURE;
    sys->fault_code = FAULT_NONE;

    DEBUG_PRINT("system init done\n");
}
```

关键语句是：

```c
sys->mode = SYS_MODE_INIT;
```

这表示系统刚启动时处于初始化模式。

## 6. fault.c 职责调整

Day11 中，`fault.c` 只负责故障检测，不再负责修改系统模式。

修改后的 `fault_check()` 函数如下：

```c
#include "fault.h"
#include "fault_code.h"
#include "debug.h"
#include "config.h"

void fault_check(SystemStatus *sys)
{
    sys->fault_code = FAULT_NONE;

    if(sys->voltage < FAULT_LOW_VOLTAGE_TH)
    {
        sys->fault_code |= FAULT_LOW_VOLTAGE;
    }

    if(sys->current > FAULT_OVER_CURRENT_TH)
    {
        sys->fault_code |= FAULT_OVER_CURRENT;
    }

    if(sys->temperature > FAULT_OVER_TEMP_TH)
    {
        sys->fault_code |= FAULT_OVER_TEMP;
    }

    DEBUG_PRINT("fault check done\n");
}
```

这样模块职责更加清晰：

```text
fault.c：负责故障检测，生成 fault_code
state_machine.c：负责根据当前 mode 和 fault_code 更新系统模式
```

## 7. 状态机核心代码

Day11 的核心修改在 `state_machine.c` 中：

```c
#include "state_machine.h"
#include "fault_code.h"
#include "debug.h"

void state_machine_update(SystemStatus *sys)
{
    switch(sys->mode)
    {
        case SYS_MODE_INIT:
            if(sys->fault_code == FAULT_NONE)
            {
                sys->mode = SYS_MODE_RUN;
                DEBUG_PRINT("state: INIT -> RUN\n");
            }
            else
            {
                sys->mode = SYS_MODE_FAULT;
                DEBUG_PRINT("state: INIT -> FAULT\n");
            }
            break;

        case SYS_MODE_RUN:
            if(sys->fault_code != FAULT_NONE)
            {
                sys->mode = SYS_MODE_FAULT;
                DEBUG_PRINT("state: RUN -> FAULT\n");
            }
            else
            {
                DEBUG_PRINT("state: RUN keep\n");
            }
            break;

        case SYS_MODE_FAULT:
            if(sys->fault_code == FAULT_NONE)
            {
                sys->mode = SYS_MODE_RUN;
                DEBUG_PRINT("state: FAULT -> RUN\n");
            }
            else
            {
                DEBUG_PRINT("state: FAULT keep\n");
            }
            break;

        default:
            sys->mode = SYS_MODE_FAULT;
            DEBUG_PRINT("state: UNKNOWN -> FAULT\n");
            break;
    }

    DEBUG_PRINT("state machine update done\n");
}
```

## 8. 状态转移逻辑说明

当系统处于 `SYS_MODE_INIT` 状态时，如果没有故障，系统进入 `SYS_MODE_RUN`；如果存在故障，系统进入 `SYS_MODE_FAULT`。

当系统处于 `SYS_MODE_RUN` 状态时，如果检测到故障，系统切换到 `SYS_MODE_FAULT`；如果没有故障，系统保持 `RUN` 状态。

当系统处于 `SYS_MODE_FAULT` 状态时，如果故障清除，系统恢复到 `SYS_MODE_RUN`；如果故障仍然存在，系统保持 `FAULT` 状态。

`default` 分支用于处理异常状态。如果 `mode` 出现未知值，系统默认进入 `SYS_MODE_FAULT`，这样可以保证异常情况下系统进入相对安全的故障模式。

## 9. main.c 调用流程

`main.c` 中的整体流程如下：

```c
#include "system_type.h"
#include "system.h"
#include "sensor.h"
#include "fault.h"
#include "state_machine.h"

int main(void)
{
    SystemStatus sys;

    system_init(&sys);
    sensor_update(&sys);
    fault_check(&sys);
    state_machine_update(&sys);
    system_print(&sys);

    return 0;
}
```

程序执行顺序为：

```text
系统初始化
传感器数据更新
故障检测
状态机更新
系统状态打印
```

其中：

```c
state_machine_update(&sys);
```

必须放在：

```c
fault_check(&sys);
```

后面，因为状态机需要根据故障检测结果更新系统模式。

## 10. Makefile 修改

Day11 从 Day10 复制而来，因此需要将目标文件名修改为：

```makefile
TARGET = build/day11_test
```

由于 Makefile 使用了自动查找源文件：

```makefile
SRCS = $(wildcard src/*.c)
```

因此 `src` 目录下的所有 `.c` 文件都会自动参与编译。

## 11. FAULT 模式验证

默认情况下，`config.h` 中的传感器模拟值为：

```c
#define SENSOR_SIM_VOLTAGE 9.5f
#define SENSOR_SIM_CURRENT 2.5f
#define SENSOR_SIM_TEMPERATURE 75.0f
```

故障判断阈值为：

```c
#define FAULT_LOW_VOLTAGE_TH 10.0f
#define FAULT_OVER_CURRENT_TH 2.0f
#define FAULT_OVER_TEMP_TH 60.0f
```

由于：

```text
9.5 < 10.0
2.5 > 2.0
75.0 > 60.0
```

所以会触发低电压、过电流和过温故障。

执行：

```bash
cd /root/Embedded_14Days/day11
make clean
make
make run
```

运行结果如下：

```text
[DEBUG] system init done
[DEBUG] sensor update done
[DEBUG] fault check done
[DEBUG] state: INIT -> FAULT
[DEBUG] state machine update done
LED state: 0
System mode: FAULT
Voltage: 9.50 V
Current: 2.50 A
temperature: 75.00 C
Fault code: 0x0007
Fault: Low voltage
Fault: Over current
Fault: Over temperature
```

其中关键输出是：

```text
[DEBUG] state: INIT -> FAULT
```

说明系统从初始化模式成功切换到了故障模式。

## 12. RUN 模式验证

为了验证正常运行模式，将 `config.h` 中的传感器模拟值临时改成正常范围：

```c
#define SENSOR_SIM_VOLTAGE 12.0f
#define SENSOR_SIM_CURRENT 1.0f
#define SENSOR_SIM_TEMPERATURE 35.0f
```

重新执行：

```bash
make clean
make
make run
```

运行结果如下：

```text
[DEBUG] system init done
[DEBUG] sensor update done
[DEBUG] fault check done
[DEBUG] state: INIT -> RUN
[DEBUG] state machine update done
LED state: 0
System mode: RUN
Voltage: 12.00 V
Current: 1.00 A
temperature: 35.00 C
Fault code: 0x0000
System normal
```

其中关键输出是：

```text
[DEBUG] state: INIT -> RUN
```

说明系统从初始化模式成功切换到了正常运行模式。

验证完成后，需要将 `config.h` 恢复为默认故障值：

```c
#define SENSOR_SIM_VOLTAGE 9.5f
#define SENSOR_SIM_CURRENT 2.5f
#define SENSOR_SIM_TEMPERATURE 75.0f
```

## 13. 今日遇到的问题

一开始运行 Day11 时，输出结果是：

```text
[DEBUG] state: FAULT keep
```

而不是预期的：

```text
[DEBUG] state: INIT -> FAULT
```

这说明在进入 `state_machine_update()` 之前，系统模式已经被提前改成了 `SYS_MODE_FAULT`。

通过命令检查：

```bash
grep -R "sys->mode" -n day11
```

发现 `fault.c` 中还保留了修改系统模式的代码：

```c
if(sys->fault_code == FAULT_NONE)
{
    sys->mode = SYS_MODE_RUN;
}
else
{
    sys->mode = SYS_MODE_FAULT;
}
```

这段代码会导致 `fault.c` 在状态机执行前提前修改 `mode`。

解决方法是删除这段代码，使 `fault.c` 只负责故障检测，系统模式切换完全交给 `state_machine.c`。

## 14. 今日收获

通过 Day11 的学习，主要掌握了以下内容：

1. 理解了状态机不仅是简单的 `if-else`；
2. 学会了根据当前状态和故障码决定下一状态；
3. 学会了使用 `switch(sys->mode)` 组织状态机逻辑；
4. 理解了 `INIT -> RUN` 和 `INIT -> FAULT` 的状态转移过程；
5. 理解了 `RUN keep`、`RUN -> FAULT`、`FAULT keep`、`FAULT -> RUN` 的含义；
6. 学会了使用 DEBUG 日志观察状态转移过程；
7. 进一步理解了模块职责分离的重要性。

Day11 的核心可以总结为：

```text
Day10：根据 fault_code 直接决定系统模式
Day11：根据当前 mode + fault_code 决定下一状态
```

这一步让状态机更接近真实嵌入式项目中的写法。