# Day06：头文件依赖追踪、.d 文件与完整增量编译

## 1. 学习目标

本项目用于学习 Makefile 中的头文件依赖追踪机制，理解 `.d` 依赖文件的作用，并通过 `-MMD -MP` 让 GCC 在编译 `.c` 文件时自动生成依赖文件。

相比 Day05 只能较好地处理 `.c` 文件变化，Day06 进一步解决了 `.h` 头文件修改后，相关 `.o` 文件能够自动重新编译的问题。

## 2. 工程结构

```bash
day06
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
    ├── fault.d
    ├── fault.o
    ├── main.d
    ├── main.o
    ├── sensor.d
    ├── sensor.o
    ├── system.d
    ├── system.o
    └── day06_test