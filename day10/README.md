# Day10：状态机 FSM、系统模式切换与状态机模块封装

## 1. 学习目标

本项目用于学习嵌入式 C 工程中的状态机思想，并将系统模式切换逻辑独立封装为 `state_machine` 模块。

相比 Day09 中直接在 `fault.c` 中根据故障码切换系统模式，Day10 将模式切换逻辑从故障检测模块中分离出来，使 `fault.c` 只负责生成故障码，`state_machine.c` 负责根据故障码更新系统模式。

## 2. 工程结构

```bash
day10
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
    └── day10_test