# Day09：枚举 enum、系统工作模式与 switch-case 状态打印

## 1. 学习目标

本项目用于学习 C 语言中的枚举 `enum`、系统工作模式定义以及 `switch-case` 状态打印方法。

相比 Day08 中使用 `config.h` 统一管理系统参数和故障阈值，Day09 进一步为系统增加了工作模式字段 `mode`，用于表示系统当前处于初始化、正常运行或故障状态。

## 2. 工程结构

```bash
day09
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
    └── day09_test