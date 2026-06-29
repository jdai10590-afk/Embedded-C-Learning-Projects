# Day08：配置头文件 config.h、阈值宏管理与消除魔法数字

## 1. 学习目标

本项目用于学习嵌入式 C 工程中的配置头文件管理方法。

相比 Day07 中通过 `debug.h` 和 Makefile 控制 DEBUG 调试日志，Day08 进一步将系统初始值、传感器模拟值和故障判断阈值统一放入 `config.h` 中管理，从而减少 `.c` 文件中的魔法数字，提高代码可读性和可维护性。

## 2. 工程结构

```bash
day08
├── Makefile
├── include
│   ├── config.h
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
    └── day08_test