# Day05：Makefile 自动化、通配符、模式规则与增量编译

## 1. 学习目标

本项目用于学习 Makefile 的自动化写法，包括 `wildcard`、`patsubst`、模式规则、自动变量 `$<` 和 `$@`，并验证增量编译机制。

相比 Day04 中手动列出每个 `.o` 文件和每条编译规则，Day05 使用 Makefile 自动查找 `src` 目录下的所有 `.c` 文件，并自动生成对应的 `.o` 目标文件。

## 2. 工程结构

```bash
day05
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
    └── day05_test