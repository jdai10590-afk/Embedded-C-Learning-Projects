# Day04：GCC 编译流程、目标文件 .o 与 Makefile 分步编译

## 1. 学习目标

本项目用于学习 GCC 的基本编译流程，理解 `.c` 源文件、`.o` 目标文件和最终可执行文件之间的关系，并使用 Makefile 实现分步编译。

相比 Day03 的一次性编译方式，Day04 将多个 `.c` 文件分别编译成 `.o` 文件，然后再将多个 `.o` 文件链接成最终可执行程序。

## 2. 工程结构

```bash
day04
├── Makefile
├── include
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
    ├── fault.o
    ├── main.o
    ├── sensor.o
    ├── system.o
    └── day04_test