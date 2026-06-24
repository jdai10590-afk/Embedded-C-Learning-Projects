# Day01：嵌入式 C 工程结构与故障码实操

## 1. 学习目标

本项目用于练习嵌入式 C 语言最基础的工程结构，包括：

- include 目录管理头文件
- src 目录管理源文件
- 使用结构体描述系统状态
- 使用宏定义和位运算表示故障码
- 使用 GCC 完成编译和运行

## 2. 工程结构

```bash
day01
├── include
│   ├── fault_code.h
│   └── system_type.h
└── src
    └── main.c