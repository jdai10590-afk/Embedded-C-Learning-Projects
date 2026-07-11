# Day20：按键短按、长按与持续按住检测

## 1. 学习目标

Day20 在 Day19 按键消抖与边沿检测的基础上，进一步识别不同的按键操作类型。

本次主要实现：

```text
稳定按下时记录按下时间
计算按键持续按住时间
未达到长按阈值就释放，识别为短按
持续按住达到长按阈值，识别为长按
使用标志位防止长按事件重复触发
长按以后释放，不再误判为短按
短按事件控制 LED 翻转
```

本次设置：

```text
消抖时间：30 ms
长按阈值：1000 ms
按键采样周期：10 ms
```

最终识别结果：

```text
170 ms：第一次稳定按下
470 ms：第一次稳定释放，识别为短按

770 ms：第二次稳定按下
1770 ms：持续按住达到 1000 ms，识别为长按
2070 ms：长按以后稳定释放
```

---

## 2. Day19 与 Day20 的区别

Day19 实现了：

```text
机械按键软件消抖
稳定按下事件
稳定释放事件
```

Day19 可以判断：

```text
按键刚刚按下
按键刚刚释放
```

但不能判断：

```text
按键一共按了多长时间
这次操作是短按还是长按
```

Day20 在此基础上新增：

```text
按下时间记录
按住时长计算
短按事件
长按事件
长按防重复触发
```

---

## 3. 工程结构

```text
day20
├── Makefile
├── README.md
├── include
│   ├── button.h
│   ├── config.h
│   ├── control.h
│   ├── debug.h
│   ├── event.h
│   ├── fault.h
│   ├── fault_code.h
│   ├── gpio.h
│   ├── ring_buffer.h
│   ├── sensor.h
│   ├── software_timer.h
│   ├── state_machine.h
│   ├── system.h
│   ├── system_type.h
│   └── uart.h
├── src
│   ├── button.c
│   ├── control.c
│   ├── event.c
│   ├── fault.c
│   ├── gpio.c
│   ├── main.c
│   ├── ring_buffer.c
│   ├── sensor.c
│   ├── software_timer.c
│   ├── state_machine.c
│   ├── system.c
│   └── uart.c
└── build
    └── day20_test
```

Day20 主要修改：

```text
include/button.h
src/button.c
src/main.c
README.md
```

Makefile 目标程序：

```makefile
TARGET = build/day20_test
```

---

## 4. 时间参数

`button.h` 中定义：

```c
#define BUTTON_DEBOUNCE_MS   30U
#define BUTTON_LONG_PRESS_MS 1000U
```

两个宏的作用不同。

### BUTTON_DEBOUNCE_MS

```text
30 ms
```

表示原始输入连续稳定 30 ms 后，才确认按键状态发生变化。

它用于过滤机械抖动。

### BUTTON_LONG_PRESS_MS

```text
1000 ms
```

表示按键稳定按下后，持续按住达到 1000 ms，产生长按事件。

它用于区分短按和长按。

---

## 5. 按键状态

```c
typedef enum
{
    BUTTON_STATE_RELEASED = 0,
    BUTTON_STATE_PRESSED = 1
} ButtonState;
```

按键有两种逻辑状态：

```text
BUTTON_STATE_RELEASED
按键处于释放状态。

BUTTON_STATE_PRESSED
按键处于按下状态。
```

这里的状态是程序内部的逻辑状态，不一定直接等于真实开发板的 GPIO 电平。

---

## 6. Day20 的按键事件

```c
typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS
} ButtonEvent;
```

事件含义：

```text
BUTTON_EVENT_NONE
没有新事件。

BUTTON_EVENT_PRESSED
按键经过消抖后，刚刚确认稳定按下。

BUTTON_EVENT_RELEASED
长按操作结束后，按键刚刚稳定释放。

BUTTON_EVENT_SHORT_PRESS
按键没有达到长按阈值就被释放。

BUTTON_EVENT_LONG_PRESS
按键持续按住达到长按阈值。
```

---

## 7. 状态和事件的区别

状态表示：

```text
按键现在是什么情况
```

例如按键持续按住期间：

```text
PRESSED
PRESSED
PRESSED
PRESSED
```

事件表示：

```text
刚刚发生了什么操作
```

例如长按事件只出现一次：

```text
NONE
LONG_PRESS
NONE
NONE
```

因此：

```text
状态可以持续存在
事件一般只在某个时刻产生一次
```

---

## 8. Button 结构体

```c
typedef struct
{
    ButtonState raw_state;
    ButtonState last_raw_state;
    ButtonState stable_state;

    unsigned int last_change_tick;
    unsigned int press_tick;

    int long_press_triggered;

    ButtonEvent event;
} Button;
```

成员作用如下：

```text
raw_state
本次读取到的原始按键状态，可能包含抖动。

last_raw_state
上一次读取到的原始状态，用于检测原始输入变化。

stable_state
经过消抖确认后的可信状态。

last_change_tick
原始状态最后一次变化的时间。

press_tick
本次稳定按下被确认的时间。

long_press_triggered
本次按住期间是否已经触发过长按。

event
当前产生的按键事件。
```

---

## 9. press_tick

```c
unsigned int press_tick;
```

用于记录本次稳定按下发生的时间。

例如：

```text
770 ms：确认稳定按下
```

则：

```text
press_tick = 770
```

后续通过：

```c
current_tick - button->press_tick
```

计算按住时长。

例如：

```text
current_tick = 1770
press_tick = 770
```

则：

```text
held_time = 1770 - 770
          = 1000 ms
```

达到长按阈值。

---

## 10. long_press_triggered

```c
int long_press_triggered;
```

用于防止长按事件重复触发。

约定：

```text
0：本次按键还没有触发长按
1：本次按键已经触发过长按
```

假设主循环每 10 ms 检查一次：

```text
1770 ms：按住 1000 ms
1780 ms：按住 1010 ms
1790 ms：按住 1020 ms
```

这些时间都满足长按条件。

如果没有标志位，可能每 10 ms 产生一次长按事件。

第一次产生长按事件后执行：

```c
button->long_press_triggered = 1;
```

后续继续按住时，不再产生新的长按事件。

---

## 11. 初始化按键

```c
void button_init(Button *button,
                 ButtonState initial_state,
                 unsigned int current_tick)
{
    button->raw_state = initial_state;
    button->last_raw_state = initial_state;
    button->stable_state = initial_state;

    button->last_change_tick = current_tick;
    button->press_tick = current_tick;

    button->long_press_triggered = 0;

    button->event = BUTTON_EVENT_NONE;
}
```

初始化后：

```text
raw_state = RELEASED
last_raw_state = RELEASED
stable_state = RELEASED

last_change_tick = 0
press_tick = 0

long_press_triggered = 0
event = NONE
```

`press_tick` 在真正稳定按下时会被重新赋值。

---

## 12. 两个时间变量

`button_update()` 中使用：

```c
unsigned int stable_time_ms;
unsigned int held_time_ms;
```

### stable_time_ms

```c
stable_time_ms =
    current_tick - button->last_change_tick;
```

表示原始输入已经连续稳定多久。

用途：

```text
判断是否完成 30 ms 软件消抖
```

### held_time_ms

```c
held_time_ms =
    current_tick - button->press_tick;
```

表示按键已经稳定按住多久。

用途：

```text
判断短按和长按
```

两者不能混淆：

| 变量 | 起始时间 | 作用 |
|---|---|---|
| `stable_time_ms` | 原始状态最后变化时间 | 消抖 |
| `held_time_ms` | 稳定按下确认时间 | 短按、长按 |

---

## 13. 原始状态变化检测

```c
if(button->raw_state != button->last_raw_state)
{
    button->last_raw_state = button->raw_state;
    button->last_change_tick = current_tick;
}
```

只要原始状态发生变化，就重新记录：

```text
last_change_tick
```

机械抖动期间，时间基准会不断重新开始。

因此只有最后一次变化后连续稳定 30 ms，稳定状态才会更新。

---

## 14. 消抖判断

```c
stable_time_ms =
    current_tick - button->last_change_tick;

if(stable_time_ms >= BUTTON_DEBOUNCE_MS)
{
    if(button->stable_state != button->raw_state)
    {
        button->stable_state = button->raw_state;
    }
}
```

消抖核心公式：

```c
current_tick - last_change_tick >= BUTTON_DEBOUNCE_MS
```

含义：

```text
原始状态从最后一次变化到现在，
是否已经连续稳定达到 30 ms。
```

---

## 15. 稳定按下处理

```c
if(button->stable_state ==
   BUTTON_STATE_PRESSED)
{
    button->press_tick = current_tick;
    button->long_press_triggered = 0;

    button->event = BUTTON_EVENT_PRESSED;
}
```

确认稳定按下时完成：

```text
记录本次按下时间
清除长按触发标志
产生稳定按下事件
```

例如：

```text
770 ms：稳定按下
```

则：

```text
press_tick = 770
long_press_triggered = 0
event = PRESSED
```

---

## 16. 持续按住检测

```c
if(button->stable_state == BUTTON_STATE_PRESSED &&
   button->long_press_triggered == 0)
{
    held_time_ms =
        current_tick - button->press_tick;

    if(held_time_ms >= BUTTON_LONG_PRESS_MS)
    {
        button->long_press_triggered = 1;
        button->event = BUTTON_EVENT_LONG_PRESS;
    }
}
```

必须同时满足：

```text
稳定状态为按下
本次尚未触发长按
```

才继续判断长按时间。

---

## 17. 长按判断

长按核心公式：

```c
current_tick - press_tick >= BUTTON_LONG_PRESS_MS
```

例如：

```text
press_tick = 770
current_tick = 1770
```

则：

```text
1770 - 770 = 1000 ms
```

达到长按阈值，产生：

```text
BUTTON_EVENT_LONG_PRESS
```

同时设置：

```text
long_press_triggered = 1
```

防止后续重复触发。

---

## 18. 为什么长按不等待释放

长按的定义是：

```text
持续按住达到指定时间
```

所以在达到 1000 ms 时就已经可以确认长按。

事件顺序：

```text
770 ms：PRESSED
1770 ms：LONG_PRESS
2070 ms：RELEASED
```

不需要等到 2070 ms 释放后才产生长按事件。

---

## 19. 稳定释放处理

稳定释放后，首先计算：

```c
held_time_ms =
    current_tick - button->press_tick;
```

然后判断：

```c
if(button->long_press_triggered == 0)
{
    button->event = BUTTON_EVENT_SHORT_PRESS;
}
else
{
    button->event = BUTTON_EVENT_RELEASED;
}
```

如果没有触发过长按，释放时识别为短按。

如果已经触发过长按，释放时只产生释放事件。

最后执行：

```c
button->long_press_triggered = 0;
```

为下一次按键操作做准备。

---

## 20. 为什么短按只能在释放时确定

稳定按下时，程序还不知道用户接下来会：

```text
很快释放
或者
继续按住达到长按时间
```

所以按下时只能记录：

```text
press_tick
```

如果在达到 1000 ms 之前释放，才能确认：

```text
本次属于短按
```

因此短按事件在稳定释放时产生。

---

## 21. 短按识别过程

第一次按键：

```text
140 ms：原始状态最后一次变成按下
170 ms：完成 30 ms 消抖，确认稳定按下

440 ms：原始状态最后一次变成释放
470 ms：完成 30 ms 消抖，确认稳定释放
```

按住时间：

```text
470 - 170 = 300 ms
```

因为：

```text
300 < 1000
```

所以产生：

```text
BUTTON_EVENT_SHORT_PRESS
```

事件输出：

```text
[EVENT] Button pressed at 170 ms
[EVENT] Short press at 470 ms
```

---

## 22. 长按识别过程

第二次按键：

```text
740 ms：原始状态最后一次变成按下
770 ms：完成消抖，确认稳定按下
```

记录：

```text
press_tick = 770
```

到 1770 ms：

```text
1770 - 770 = 1000 ms
```

产生：

```text
BUTTON_EVENT_LONG_PRESS
```

之后继续按住，不再重复产生长按。

---

## 23. 长按后的释放

第二次释放：

```text
2040 ms：原始状态最后一次变成释放
2070 ms：完成消抖，确认稳定释放
```

因为：

```text
long_press_triggered = 1
```

所以不会产生短按，而是产生：

```text
BUTTON_EVENT_RELEASED
```

事件输出：

```text
[EVENT] Button released after long press at 2070 ms
```

---

## 24. 模拟按键信号

主程序模拟了两次按键操作。

第一次短按：

```text
100～140 ms：按下抖动
140～400 ms：稳定按下
400～440 ms：释放抖动
440 ms 后：稳定释放
```

第二次长按：

```text
700～740 ms：按下抖动
740～2000 ms：稳定按下
2000～2040 ms：释放抖动
2040 ms 后：稳定释放
```

---

## 25. 使用 switch 处理事件

```c
switch(event)
{
    case BUTTON_EVENT_PRESSED:
        break;

    case BUTTON_EVENT_SHORT_PRESS:
        break;

    case BUTTON_EVENT_LONG_PRESS:
        break;

    case BUTTON_EVENT_RELEASED:
        break;

    case BUTTON_EVENT_NONE:
    default:
        break;
}
```

`switch` 根据不同事件执行不同功能。

每个 `case` 后使用：

```c
break;
```

避免程序继续进入后面的分支。

---

## 26. 短按控制 LED

短按事件中执行：

```c
gpio_led_toggle(sys);
led_sync_state(sys);
```

LED 初始状态：

```text
0
```

短按后：

```text
0 → 1
```

输出：

```text
[ACTION] Short press toggled LED, level = 1
```

---

## 27. 长按动作

长按事件中目前只模拟执行：

```text
[ACTION] Long-press action executed
```

没有修改 LED。

真实项目中，长按可以用于：

```text
恢复默认设置
进入配置模式
启动设备
关闭设备
执行复位
进入配网模式
```

---

## 28. 事件读取后清除

```c
ButtonEvent button_get_event(Button *button)
{
    ButtonEvent event;

    event = button->event;
    button->event = BUTTON_EVENT_NONE;

    return event;
}
```

事件被读取以后立即恢复为：

```text
BUTTON_EVENT_NONE
```

保证同一个事件不会被主程序反复处理。

---

## 29. 实际运行结果

```text
[EVENT] Button pressed at 170 ms
[DEBUG] button short press, held = 300 ms
[EVENT] Short press at 470 ms
[ACTION] Short press toggled LED, level = 1

[EVENT] Button pressed at 770 ms
[DEBUG] button long press at 1770 ms, held = 1000 ms
[EVENT] Long press at 1770 ms
[ACTION] Long-press action executed

[DEBUG] button released after long press at 2070 ms
[EVENT] Button released after long press at 2070 ms
```

事件顺序完全符合预期。

---

## 30. 自动验证事件次数

验证短按只出现一次：

```bash
make run | grep -c "\[EVENT\] Short press"
```

预期：

```text
1
```

验证长按只出现一次：

```bash
make run | grep -c "\[EVENT\] Long press"
```

预期：

```text
1
```

验证进行了两次稳定按下：

```bash
make run | grep -c "\[EVENT\] Button pressed"
```

预期：

```text
2
```

---

## 31. 最终 LED 状态

LED 初始状态为：

```text
0
```

第一次短按翻转一次：

```text
0 → 1
```

第二次长按没有修改 LED。

所以最终状态为：

```text
LED state: 1
GPIO output reg: 0x00000001
LED pin level: 1
```

---

## 32. 编译运行

进入 Day20：

```bash
cd /root/Embedded_14Days/day20
```

清理：

```bash
make clean
```

编译：

```bash
make
```

运行：

```bash
make run
```

筛选关键结果：

```bash
make run | grep -E "\[EVENT\]|\[ACTION\]|short press|long press"
```

---

## 33. 本次遇到的错误

最初编译时出现：

```text
multiple definition of `button_init'
multiple definition of `button_update'
multiple definition of `button_get_event'
multiple definition of `button_get_state'
undefined reference to `main'
```

原因是误把 `button.c` 的内容写入了：

```text
src/main.c
```

导致：

```text
button.c 中定义了一套 button_* 函数
main.c 中又定义了一套 button_* 函数
```

链接器发现相同函数存在多个定义，因此报告：

```text
multiple definition
```

同时错误的 `main.c` 中没有：

```c
int main(void)
```

因此报告：

```text
undefined reference to `main'
```

修正文件职责后：

```text
button.c：定义 button_* 模块函数
main.c：定义 main() 并调用按键模块
```

程序即可正常链接。

---

## 34. 编译和链接的区别

下面步骤属于编译：

```text
button.c → button.o
main.c → main.o
```

每个源文件单独检查语法并生成目标文件。

下面步骤属于链接：

```text
button.o
main.o
gpio.o
system.o
...
      ↓
day20_test
```

链接器负责把所有目标文件组合成最终程序。

本次错误发生在链接阶段，而不是单个 `.c` 文件编译阶段。

---

## 35. VS Code 文件冲突

使用终端命令：

```bash
cat > src/main.c
```

修改磁盘文件时，如果 VS Code 中仍打开着未保存的旧版本，可能出现：

```text
无法保存文件，文件内容较新
```

此时不能直接用旧内容覆盖磁盘新文件。

正确处理方法：

```text
关闭旧标签
选择不保存
重新打开文件
```

这样 VS Code 会重新加载终端写入的正确版本。

---

## 36. Day20 核心公式

消抖判断：

```c
current_tick - last_change_tick >= BUTTON_DEBOUNCE_MS
```

长按判断：

```c
current_tick - press_tick >= BUTTON_LONG_PRESS_MS
```

两者含义不同：

```text
第一个公式：
原始状态是否稳定达到消抖时间。

第二个公式：
按键是否持续按住达到长按时间。
```

---

## 37. Day20 核心流程

短按流程：

```text
稳定按下
    ↓
记录 press_tick
    ↓
没有达到 1000 ms
    ↓
稳定释放
    ↓
产生 SHORT_PRESS
```

长按流程：

```text
稳定按下
    ↓
记录 press_tick
    ↓
持续按住达到 1000 ms
    ↓
产生一次 LONG_PRESS
    ↓
long_press_triggered = 1
    ↓
继续按住不再重复
    ↓
释放时产生 RELEASED
```

---

## 38. 当前实现的局限

当前结构体中只有一个：

```c
ButtonEvent event;
```

同一时刻只能保存一个事件。

例如短按释放时，当前设计优先产生：

```text
SHORT_PRESS
```

不会同时保存：

```text
RELEASED
```

更复杂的系统可以使用：

```text
事件位图
事件队列
Ring Buffer
RTOS 消息队列
```

保存多个事件。

---

## 39. Day20 今日收获

通过 Day20 学习，掌握了：

1. 记录稳定按下时间；
2. 计算按键持续按住时长；
3. 区分消抖时间和长按时间；
4. 在释放时确认短按；
5. 在达到阈值时确认长按；
6. 长按不需要等待释放；
7. 使用标志位防止长按重复触发；
8. 长按后释放不再误判为短按；
9. 使用 `switch-case` 处理多种事件；
10. 使用短按事件控制 LED；
11. 读取事件后将事件清除；
12. 使用命令自动统计事件次数；
13. 理解编译和链接的区别；
14. 理解 `multiple definition` 错误；
15. 理解 `undefined reference to main` 错误；
16. 理解 VS Code 编辑器内容和磁盘文件冲突；
17. 为真实开发板按键功能开发打下基础。

学习路线：

```text
Day15：GPIO 寄存器与位操作
Day16：UART 完整命令解析
Day17：Ring Buffer 与 UART 字节流
Day18：系统 Tick 与软件定时器
Day19：按键边沿检测与软件消抖
Day20：按键短按、长按与持续按住检测
```
