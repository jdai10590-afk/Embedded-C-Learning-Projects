# Day19：按键边沿检测与软件消抖

## 1. 学习目标

Day19 在前面 GPIO、系统 Tick 和模块化工程的基础上，实现机械按键的软件消抖与边沿事件检测。

本次主要完成：

```text
模拟机械按键抖动信号
记录原始按键状态
检测原始状态变化
使用系统 Tick 进行 30 ms 软件消抖
确认稳定按下状态
确认稳定释放状态
产生一次性按下事件
产生一次性释放事件
按下事件触发 LED 翻转
```

最终运行结果：

```text
170 ms：确认一次稳定按下
570 ms：确认一次稳定释放
```

虽然原始信号在按下和释放过程中多次变化，但程序最终只产生：

```text
一次按下事件
一次释放事件
```

---

## 2. 为什么机械按键需要消抖

理想按键信号：

```text
未按下：0
按下：  1
```

理想按下过程：

```text
0 0 0 0 1 1 1 1 1 1
```

真实机械按键内部由金属触点接触。在按下或释放瞬间，触点可能在几毫秒内反复接通和断开。

真实信号可能是：

```text
0 0 0 1 0 1 0 1 1 1
```

中间的：

```text
1 → 0 → 1 → 0 → 1
```

就是机械抖动。

如果程序直接根据原始电平判断按键，可能把一次按键操作识别成：

```text
按下
释放
按下
释放
按下
```

因此必须进行消抖。

---

## 3. 软件消抖基本思想

本次采用时间稳定法进行消抖：

```text
读取原始按键状态
        ↓
原始状态发生变化
        ↓
记录最后一次变化时间
        ↓
等待原始状态持续稳定
        ↓
稳定时间达到 30 ms
        ↓
更新可信的稳定状态
        ↓
产生按下或释放事件
```

核心规则：

```text
原始状态必须连续保持相同值至少 30 ms，
程序才确认按键状态真正发生变化。
```

---

## 4. 工程结构

```text
day19
├── Makefile
├── README.md
├── include
│   ├── button.h
│   ├── config.h
│   ├── control.h
│   ├── debug.h
│   ├── event.h
│   ├── fault_code.h
│   ├── fault.h
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
    └── day19_test
```

Day19 新增：

```text
include/button.h
src/button.c
```

并重新编写：

```text
src/main.c
```

Makefile 编译目标：

```makefile
TARGET = build/day19_test
```

---

## 5. 消抖时间

在 `button.h` 中定义：

```c
#define BUTTON_DEBOUNCE_MS 30U
```

表示按键原始状态需要连续稳定：

```text
30 ms
```

才确认状态变化。

其中：

```text
U 表示 unsigned，无符号整数
```

因为系统 Tick 和时间参数使用 `unsigned int`，所以时间常量也使用无符号类型。

---

## 6. 按键逻辑状态

定义：

```c
typedef enum
{
    BUTTON_STATE_RELEASED = 0,
    BUTTON_STATE_PRESSED = 1
} ButtonState;
```

两种状态：

```text
BUTTON_STATE_RELEASED
按键未按下。

BUTTON_STATE_PRESSED
按键已经按下。
```

当前电脑模拟中约定：

```text
0 = 释放
1 = 按下
```

这表示程序内部的逻辑状态，不一定等同于真实开发板的 GPIO 电平。

一些开发板按键为低电平有效：

```text
GPIO = 1：释放
GPIO = 0：按下
```

真实项目中，可以先把 GPIO 电平转换为逻辑状态，再交给按键模块处理。

---

## 7. 按键事件

定义：

```c
typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED
} ButtonEvent;
```

三种事件：

```text
BUTTON_EVENT_NONE
没有新的按键事件。

BUTTON_EVENT_PRESSED
稳定状态刚从释放变成按下。

BUTTON_EVENT_RELEASED
稳定状态刚从按下变成释放。
```

---

## 8. 状态与事件的区别

状态表示：

```text
按键现在处于什么状态
```

例如按键一直按住：

```text
PRESSED
PRESSED
PRESSED
PRESSED
```

事件表示：

```text
按键刚刚发生了什么变化
```

例如按键被确认按下时：

```text
NONE
PRESSED
NONE
NONE
```

所以：

```text
状态可以持续存在
事件通常只产生一次
```

如果要实现“一次按下翻转一次 LED”，应该使用：

```c
BUTTON_EVENT_PRESSED
```

而不是在按键保持按下时反复检查状态并翻转 LED。

---

## 9. 按键结构体

定义：

```c
typedef struct
{
    ButtonState raw_state;
    ButtonState last_raw_state;
    ButtonState stable_state;

    unsigned int last_change_tick;

    ButtonEvent event;
} Button;
```

一个 `Button` 对象保存完整的按键处理信息。

例如：

```c
Button key1;
```

表示创建一个按键对象。

---

## 10. raw_state

```c
ButtonState raw_state;
```

表示当前这一次读取到的原始按键状态。

它可能包含机械抖动，例如：

```text
PRESSED
RELEASED
PRESSED
RELEASED
PRESSED
```

因此不能直接把 `raw_state` 当作可信按键状态。

---

## 11. last_raw_state

```c
ButtonState last_raw_state;
```

表示上一次读取到的原始按键状态。

通过比较：

```c
raw_state != last_raw_state
```

可以判断原始输入是否刚刚发生变化。

如果发生变化，就需要：

```text
保存新的原始状态
重新记录变化时间
重新开始消抖计时
```

---

## 12. stable_state

```c
ButtonState stable_state;
```

表示已经经过消抖确认的稳定状态。

它才是可以交给业务逻辑使用的可信状态。

例如按下抖动期间：

```text
raw_state：
1 0 1 0 1 1 1 1

stable_state：
0 0 0 0 0 0 0 1
```

原始信号多次变化，但稳定状态只在连续稳定 30 ms 后才改变。

---

## 13. last_change_tick

```c
unsigned int last_change_tick;
```

记录原始状态最后一次变化的系统 Tick。

例如：

```text
140 ms：原始状态最后一次从释放变为按下
```

则：

```text
last_change_tick = 140
```

当前时间为 170 ms 时：

```text
170 - 140 = 30 ms
```

达到消抖要求，可以确认稳定按下。

---

## 14. event

```c
ButtonEvent event;
```

用于保存当前产生的按键事件。

可能是：

```text
BUTTON_EVENT_NONE
BUTTON_EVENT_PRESSED
BUTTON_EVENT_RELEASED
```

稳定状态改变时产生事件。

主程序读取事件后，会将它清除为：

```text
BUTTON_EVENT_NONE
```

保证同一个事件只被处理一次。

---

## 15. 按键初始化

函数声明：

```c
void button_init(Button *button,
                 ButtonState initial_state,
                 unsigned int current_tick);
```

实现：

```c
void button_init(Button *button,
                 ButtonState initial_state,
                 unsigned int current_tick)
{
    button->raw_state = initial_state;
    button->last_raw_state = initial_state;
    button->stable_state = initial_state;

    button->last_change_tick = current_tick;

    button->event = BUTTON_EVENT_NONE;

    DEBUG_PRINT("button init, state = %d, tick = %u ms\n",
                initial_state,
                current_tick);
}
```

调用：

```c
Button key1;

button_init(&key1,
            BUTTON_STATE_RELEASED,
            0U);
```

初始化后：

```text
raw_state = RELEASED
last_raw_state = RELEASED
stable_state = RELEASED
last_change_tick = 0
event = NONE
```

---

## 16. 为什么三个状态需要初始化为相同值

初始化时执行：

```c
button->raw_state = initial_state;
button->last_raw_state = initial_state;
button->stable_state = initial_state;
```

如果系统启动时按键处于释放状态，那么：

```text
当前原始状态 = 释放
上次原始状态 = 释放
稳定状态 = 释放
```

三者保持一致，可以避免第一次更新时误判状态变化。

---

## 17. 按键更新函数

函数声明：

```c
void button_update(Button *button,
                   ButtonState raw_state,
                   unsigned int current_tick);
```

每次采样按键后调用：

```c
button_update(&key1,
              raw_state,
              system_tick_ms);
```

该函数负责：

```text
保存当前原始状态
检测原始状态变化
记录最后变化时间
计算稳定时间
判断是否达到 30 ms
更新稳定状态
产生按下或释放事件
```

---

## 18. 保存原始状态

```c
button->raw_state = raw_state;
```

这里只是保存当前读取值。

它尚未经过消抖，因此不能立即确认按键已经按下或释放。

---

## 19. 检测原始状态变化

```c
if(button->raw_state != button->last_raw_state)
{
    button->last_raw_state = button->raw_state;
    button->last_change_tick = current_tick;
}
```

当本次原始状态与上一次不同时，说明输入刚发生变化。

程序完成：

```text
更新 last_raw_state
更新 last_change_tick
重新开始稳定时间计算
```

---

## 20. 为什么抖动会不断重新计时

按下时模拟的原始信号：

```text
100 ms：按下
110 ms：释放
120 ms：按下
130 ms：释放
140 ms：按下
```

每次状态变化都会执行：

```c
button->last_change_tick = current_tick;
```

所以变化时间依次更新为：

```text
100
110
120
130
140 ms
```

最后从 140 ms 开始保持按下。

因此真正的稳定计时从：

```text
140 ms
```

开始，而不是从 100 ms 开始。

---

## 21. 计算稳定时间

```c
stable_time_ms =
    current_tick - button->last_change_tick;
```

例如：

```text
current_tick = 160
last_change_tick = 140
```

则：

```text
stable_time_ms = 20 ms
```

还没有达到 30 ms。

到 170 ms：

```text
170 - 140 = 30 ms
```

满足消抖要求。

---

## 22. 判断是否达到消抖时间

```c
if(stable_time_ms >= BUTTON_DEBOUNCE_MS)
```

等价于：

```c
if(stable_time_ms >= 30U)
```

使用 `>=` 而不是 `==`，因为主循环不一定恰好在第 30 ms 检查。

例如下一次检查时已经稳定 40 ms：

```text
40 >= 30
```

仍然应该确认状态。

---

## 23. 为什么还要比较稳定状态

```c
if(button->stable_state != button->raw_state)
```

如果：

```text
stable_state = PRESSED
raw_state = PRESSED
```

说明稳定状态没有发生新变化。

即使原始状态已经稳定很久，也不能重复产生按下事件。

只有：

```text
stable_state != raw_state
```

时，才更新稳定状态并产生事件。

---

## 24. 产生按下事件

```c
if(button->stable_state ==
   BUTTON_STATE_PRESSED)
{
    button->event = BUTTON_EVENT_PRESSED;
}
```

表示稳定状态从：

```text
RELEASED → PRESSED
```

此时产生一次按下事件。

按键继续保持按下时，不会重复产生事件。

---

## 25. 产生释放事件

```c
else
{
    button->event = BUTTON_EVENT_RELEASED;
}
```

表示稳定状态从：

```text
PRESSED → RELEASED
```

此时产生一次释放事件。

---

## 26. 读取并清除事件

实现：

```c
ButtonEvent button_get_event(Button *button)
{
    ButtonEvent event;

    event = button->event;

    button->event = BUTTON_EVENT_NONE;

    return event;
}
```

处理过程：

```text
先把当前事件保存到局部变量
        ↓
将结构体中的事件清除
        ↓
返回刚才保存的事件
```

例如原事件是：

```text
BUTTON_EVENT_PRESSED
```

第一次读取返回：

```text
BUTTON_EVENT_PRESSED
```

内部事件被清除。

第二次读取返回：

```text
BUTTON_EVENT_NONE
```

因此同一个事件不会被重复处理。

---

## 27. 读取稳定状态

实现：

```c
ButtonState button_get_state(const Button *button)
{
    return button->stable_state;
}
```

该函数返回经过消抖确认的可信状态。

参数使用：

```c
const Button *button
```

表示函数只读取按键数据，不允许修改结构体成员。

---

## 28. 模拟按键原始信号

电脑环境中没有真实 GPIO 按键，因此通过时间模拟原始信号：

```c
static ButtonState simulate_button_raw_state(
    unsigned int current_tick)
```

模拟过程：

```text
0～90 ms：稳定释放

100 ms：按下
110 ms：释放
120 ms：按下
130 ms：释放
140 ms：按下

140～490 ms：稳定按下

500 ms：释放
510 ms：按下
520 ms：释放
530 ms：按下
540 ms：释放

540 ms 以后：稳定释放
```

---

## 29. 按下抖动过程

原始状态：

| Tick | 原始状态 |
|---:|:---:|
| 100 ms | PRESSED |
| 110 ms | RELEASED |
| 120 ms | PRESSED |
| 130 ms | RELEASED |
| 140 ms | PRESSED |

最后一次变化发生在：

```text
140 ms
```

之后持续保持按下。

---

## 30. 为什么在 170 ms 确认按下

消抖时间：

```text
30 ms
```

最后变化时间：

```text
140 ms
```

所以确认时间：

```text
140 + 30 = 170 ms
```

运行结果：

```text
[DEBUG] button stable pressed at 170 ms
[EVENT] Button pressed at 170 ms
```

---

## 31. 按下事件控制 LED

事件处理代码：

```c
if(event == BUTTON_EVENT_PRESSED)
{
    gpio_led_toggle(sys);
    led_sync_state(sys);
}
```

按下事件产生时：

```text
LED 从 0 翻转为 1
```

输出：

```text
[ACTION] LED toggled, level = 1
```

按键保持按下期间，没有重复按下事件，因此 LED 只翻转一次。

---

## 32. 释放抖动过程

原始状态：

| Tick | 原始状态 |
|---:|:---:|
| 500 ms | RELEASED |
| 510 ms | PRESSED |
| 520 ms | RELEASED |
| 530 ms | PRESSED |
| 540 ms | RELEASED |

最后一次变化发生在：

```text
540 ms
```

之后持续保持释放。

---

## 33. 为什么在 570 ms 确认释放

最后变化时间：

```text
540 ms
```

消抖时间：

```text
30 ms
```

所以：

```text
540 + 30 = 570 ms
```

运行结果：

```text
[DEBUG] button stable released at 570 ms
[EVENT] Button released at 570 ms
```

释放事件只打印信息，没有翻转 LED。

---

## 34. 每 10 ms 采样一次

主循环：

```c
for(system_tick_ms = 0U;
    system_tick_ms <= 700U;
    system_tick_ms += 10U)
```

Tick 依次为：

```text
0
10
20
30
...
700 ms
```

每 10 ms 执行一次：

```c
button_update();
```

这表示按键采样周期为 10 ms。

采样周期应明显小于消抖时间，才能较准确地观察原始状态变化。

---

## 35. SAMPLE 输出

输出格式：

```c
printf("[SAMPLE] tick = %3u ms, raw = %d, stable = %d\n",
       system_tick_ms,
       raw_state,
       button_get_state(&key1));
```

例如：

```text
[SAMPLE] tick = 120 ms, raw = 1, stable = 0
```

表示：

```text
当前 Tick：120 ms
原始输入：按下
稳定状态：仍然是释放
```

原始状态虽然为按下，但还没有连续稳定 30 ms，所以稳定状态没有立即改变。

---

## 36. 按下阶段关键状态

| 时间 | raw_state | stable_state | event |
|---:|:---:|:---:|:---:|
| 90 ms | 0 | 0 | NONE |
| 100 ms | 1 | 0 | NONE |
| 110 ms | 0 | 0 | NONE |
| 120 ms | 1 | 0 | NONE |
| 130 ms | 0 | 0 | NONE |
| 140 ms | 1 | 0 | NONE |
| 150 ms | 1 | 0 | NONE |
| 160 ms | 1 | 0 | NONE |
| 170 ms | 1 | 1 | PRESSED |

---

## 37. 释放阶段关键状态

| 时间 | raw_state | stable_state | event |
|---:|:---:|:---:|:---:|
| 490 ms | 1 | 1 | NONE |
| 500 ms | 0 | 1 | NONE |
| 510 ms | 1 | 1 | NONE |
| 520 ms | 0 | 1 | NONE |
| 530 ms | 1 | 1 | NONE |
| 540 ms | 0 | 1 | NONE |
| 550 ms | 0 | 1 | NONE |
| 560 ms | 0 | 1 | NONE |
| 570 ms | 0 | 0 | RELEASED |

---

## 38. 核心运行结果

按下原始抖动：

```text
[DEBUG] button raw changed to 1 at 100 ms
[DEBUG] button raw changed to 0 at 110 ms
[DEBUG] button raw changed to 1 at 120 ms
[DEBUG] button raw changed to 0 at 130 ms
[DEBUG] button raw changed to 1 at 140 ms
```

稳定按下：

```text
[DEBUG] button stable pressed at 170 ms
[EVENT] Button pressed at 170 ms
[ACTION] LED toggled, level = 1
```

释放原始抖动：

```text
[DEBUG] button raw changed to 0 at 500 ms
[DEBUG] button raw changed to 1 at 510 ms
[DEBUG] button raw changed to 0 at 520 ms
[DEBUG] button raw changed to 1 at 530 ms
[DEBUG] button raw changed to 0 at 540 ms
```

稳定释放：

```text
[DEBUG] button stable released at 570 ms
[EVENT] Button released at 570 ms
```

---

## 39. 最终系统状态

LED 初始为：

```text
0
```

170 ms 产生一次稳定按下事件，LED 翻转：

```text
0 → 1
```

570 ms 释放事件没有控制 LED。

因此最终状态：

```text
LED state: 1
GPIO output reg: 0x00000001
LED pin level: 1
```

---

## 40. 编译运行

进入 Day19：

```bash
cd /root/Embedded_14Days/day19
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
make run | grep -E "button raw|button stable|\[EVENT\]|\[ACTION\]"
```

---

## 41. 统计事件次数

统计按下事件：

```bash
make run | grep -c "\[EVENT\] Button pressed"
```

结果：

```text
1
```

统计释放事件：

```bash
make run | grep -c "\[EVENT\] Button released"
```

结果：

```text
1
```

说明：

```text
一次带抖动的按下只产生一次按下事件
一次带抖动的释放只产生一次释放事件
```

---

## 42. 修改消抖时间对照实验

将：

```c
#define BUTTON_DEBOUNCE_MS 30U
```

临时修改为：

```c
#define BUTTON_DEBOUNCE_MS 40U
```

最后一次按下变化：

```text
140 ms
```

新的确认时间：

```text
140 + 40 = 180 ms
```

最后一次释放变化：

```text
540 ms
```

新的确认时间：

```text
540 + 40 = 580 ms
```

这证明：

```text
事件确认时间
=
最后一次原始状态变化时间
+
消抖时间
```

正式版本恢复为：

```c
#define BUTTON_DEBOUNCE_MS 30U
```

---

## 43. 当前实现的局限

当前结构体只保存一个事件：

```c
ButtonEvent event;
```

如果主程序长时间不调用：

```c
button_get_event();
```

而期间又产生了第二个事件，后面的事件可能覆盖前面的事件。

当前主循环每次更新后立即读取事件，因此不会出现问题。

更复杂的项目可以使用：

```text
事件队列
Ring Buffer
消息队列
RTOS Queue
```

保存多个未处理事件。

---

## 44. 当前模拟与真实硬件的区别

当前程序通过：

```c
simulate_button_raw_state();
```

模拟按键输入。

真实单片机中通常是：

```c
raw_state = gpio_button_read();
```

完整结构可以是：

```text
GPIO 读取物理电平
        ↓
转换为逻辑 PRESSED/RELEASED
        ↓
button_update()
        ↓
软件消抖
        ↓
产生按键事件
        ↓
业务逻辑处理
```

当前按键模块可以继续移植到真实开发板。

---

## 45. Day19 核心理解

三个数据不能混淆：

```text
raw_state
刚读取到的原始输入，可能抖动。

stable_state
经过时间确认后的可信状态。

event
稳定状态刚刚变化时产生的一次性通知。
```

消抖核心公式：

```c
current_tick - last_change_tick >= BUTTON_DEBOUNCE_MS
```

含义是：

```text
从原始状态最后一次变化到现在，
是否已经连续稳定达到消抖时间。
```

完整流程：

```text
原始输入变化
      ↓
重新记录变化时间
      ↓
连续稳定 30 ms
      ↓
更新稳定状态
      ↓
产生一次按下或释放事件
```

---

## 46. Day19 今日收获

通过 Day19 学习，掌握了：

1. 机械按键产生抖动的原因；
2. 原始按键状态和稳定状态的区别；
3. 按键状态和按键事件的区别；
4. 按下边沿和释放边沿；
5. 使用系统 Tick 进行软件消抖；
6. 记录原始状态最后变化时间；
7. 原始状态变化后重新开始消抖计时；
8. 使用 30 ms 判断稳定状态；
9. 使用 `>=` 避免错过确认时间；
10. 只在稳定状态变化时产生事件；
11. 按下事件只产生一次；
12. 释放事件只产生一次；
13. 读取并清除事件；
14. 使用按下事件翻转 LED；
15. 同步 LED 软件状态和 GPIO 状态；
16. 使用模拟信号测试机械抖动；
17. 使用 `grep -c` 自动统计事件数量；
18. 为以后连接真实 GPIO 按键打基础。

学习阶段关系：

```text
Day15：GPIO 寄存器与位操作
Day16：UART 完整命令解析
Day17：Ring Buffer 与 UART 字节流
Day18：系统 Tick 与软件定时器
Day19：按键边沿检测与软件消抖
```