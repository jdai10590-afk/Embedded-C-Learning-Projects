# Day07：条件编译、DEBUG 开关与日志打印模块

## 1. 学习目标

本项目用于学习 C 语言中的条件编译、DEBUG 宏开关以及调试日志打印模块。

相比 Day06 中重点学习头文件依赖追踪和 `.d` 文件，Day07 进一步在工程中加入 `debug.h`，通过 Makefile 控制是否开启调试打印，实现 DEBUG 模式和非 DEBUG 模式的切换。

## 2. 工程结构

```bash
day07
├── Makefile
├── include
│   ├── debug.h
│   ├── fault_code.h
│   ├── fault.h
│   ├── sensor.h
│   ├── system.h
│   └── system_type.h
├── src
│   ├── fault.c
│   ├── main.c
│   ├── sensor.c
│   └── system.c
└── build
    ├── fault.d
    ├── fault.o
    ├── main.d
    ├── main.o
    ├── sensor.d
    ├── sensor.o
    ├── system.d
    ├── system.o
    └── day07_test
```

## 3. debug.h 内容

```c
#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) printf("[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...)
#endif

#endif
```

其中：

* `#ifdef DEBUG` 表示如果定义了 `DEBUG`，则启用调试打印；
* `DEBUG_PRINT(fmt, ...)` 是一个可变参数宏；
* 当 DEBUG 开启时，`DEBUG_PRINT` 等价于带 `[DEBUG]` 前缀的 `printf`；
* 当 DEBUG 关闭时，`DEBUG_PRINT` 什么都不输出。

## 4. Makefile DEBUG 开关

```makefile
DEBUG ?= 1

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
endif
```

其中：

* `DEBUG ?= 1` 表示默认开启 DEBUG；
* `-DDEBUG` 表示在编译时定义 `DEBUG` 宏；
* 如果执行 `make DEBUG=0`，则不会添加 `-DDEBUG`，调试打印关闭。

## 5. 调试打印示例

在 `system.c` 中加入：

```c
DEBUG_PRINT("system init done\n");
```

在 `sensor.c` 中加入：

```c
DEBUG_PRINT("sensor update done\n");
```

在 `fault.c` 中加入：

```c
DEBUG_PRINT("fault check done\n");
```

## 6. DEBUG 开启测试

执行：

```bash
cd /root/Embedded_14Days/day07
make clean
make
make run
```

运行结果包含调试信息：

```bash
[DEBUG] system init done
[DEBUG] sensor update done
[DEBUG] fault check done
LED state: 0
Voltage: 9.50 V
Current: 2.50 A
temperature: 75.00 C
Fault code: 0x0007
Fault: Low voltage
Fault: Over current
Fault: Over temperature
```

说明 DEBUG 模式开启成功。

## 7. DEBUG 关闭测试

执行：

```bash
make clean
make DEBUG=0
make run
```

运行结果不包含 `[DEBUG]` 信息：

```bash
LED state: 0
Voltage: 9.50 V
Current: 2.50 A
temperature: 75.00 C
Fault code: 0x0007
Fault: Low voltage
Fault: Over current
Fault: Over temperature
```

说明 DEBUG 模式关闭成功。

## 8. 今日收获

通过 Day07 的学习，主要掌握了以下内容：

1. 理解了条件编译的基本作用；
2. 学会了使用 `#ifdef DEBUG` 控制代码是否参与编译；
3. 学会了编写 `debug.h` 调试打印模块；
4. 理解了 `DEBUG_PRINT(fmt, ...)` 可变参数宏的作用；
5. 学会了通过 Makefile 中的 `-DDEBUG` 控制 DEBUG 开关；
6. 验证了 DEBUG 开启和关闭两种运行效果。

Day07 的核心可以总结为：

```text
DEBUG 开启时，调试日志参与编译并输出；
DEBUG 关闭时，调试日志不输出。
```

这一步是嵌入式工程调试中非常常见的做法。
