# Day03：多文件拆分、函数声明与 Makefile 多源文件编译

## 1. 学习目标

本项目用于练习嵌入式 C 工程中的多文件拆分、`.h` 头文件声明、`.c` 源文件实现，以及 Makefile 对多个源文件的编译管理。

## 2. 工程结构

```bash
day03
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
    └── day03_test