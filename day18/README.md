# Day18：系统 Tick、软件定时器与非阻塞任务调度

## 1. 学习目标

Day18 在前面模块化工程的基础上，引入系统 Tick 和软件定时器，实现多个不同周期任务的非阻塞调度。

本次实现三个周期任务：

```text
传感器任务：每 200 ms 执行一次
LED 任务：每 500 ms 执行一次
状态任务：每 1000 ms 执行一次
```

同时测试：

```text
定时器初始化
定时器启动
定时器到期判断
定时器停止
定时器重新启动
多个定时器独立运行
同一 Tick 执行多个任务
```

Day18 的核心目标是：

```text
主循环不阻塞等待，
而是不断检查任务是否到期，
到期后才执行对应任务。
```

---

## 2. 为什么需要软件定时器

如果使用阻塞式延时：

```c
delay_ms(1000);
system_print(&sys);
```

程序在等待的 1000 ms 内无法及时处理其他任务。

例如：

```text
无法及时读取传感器
无法及时处理 UART 数据
无法及时检查故障
无法同时控制多个不同周期任务
```

软件定时器采用非阻塞方式：

```text
主循环持续运行
        ↓
读取当前系统 Tick
        ↓
分别检查每个任务是否到期
        ↓
到期就执行
未到期就继续处理其他任务
```

这种方式适合裸机嵌入式系统中的周期任务调度。

---

## 3. 工程结构

```text
day18
├── Makefile
├── include
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
    └── day18_test
```

Day18 新增：

```text
include/software_timer.h
src/software_timer.c
```

并重新编写：

```text
src/main.c
```

Makefile 编译目标修改为：

```makefile
TARGET = build/day18_test
```

---

## 4. 系统 Tick

系统 Tick 可以理解为系统运行时间计数。

本次使用：

```c
unsigned int system_tick_ms;
```

表示当前系统时间，单位为毫秒。

在主循环中：

```c
for(system_tick_ms = 0;
    system_tick_ms <= 3000;
    system_tick_ms += 100)
```

Tick 依次变化为：

```text
0 ms
100 ms
200 ms
300 ms
...
3000 ms
```

需要注意：

```text
这里的 Tick 只是数值模拟，
程序并没有真的每次等待 100 ms。
```

在真实单片机中，系统 Tick 通常由 SysTick 或硬件定时器中断增加，例如：

```c
volatile unsigned int system_tick_ms = 0;

void SysTick_Handler(void)
{
    system_tick_ms++;
}
```

这表示每进入一次 1 ms 中断，系统时间增加 1 ms。

---

## 5. 软件定时器结构体

在 `software_timer.h` 中定义：

```c
typedef struct
{
    unsigned int start_tick;
    unsigned int period_ms;
    int enabled;
} SoftwareTimer;
```

三个成员含义如下。

### start_tick

```c
unsigned int start_tick;
```

表示本轮计时从哪个 Tick 开始。

例如：

```text
start_tick = 500
```

表示定时器当前从 500 ms 开始计时。

---

### period_ms

```c
unsigned int period_ms;
```

表示定时器周期，单位为毫秒。

例如：

```text
period_ms = 200
```

表示每经过 200 ms 到期一次。

---

### enabled

```c
int enabled;
```

表示定时器是否启用：

```text
enabled = 1：定时器正在运行
enabled = 0：定时器已经停止
```

---

## 6. 软件定时器函数接口

`software_timer.h` 中声明：

```c
void software_timer_init(SoftwareTimer *timer,
                         unsigned int period_ms);

void software_timer_start(SoftwareTimer *timer,
                          unsigned int current_tick);

void software_timer_stop(SoftwareTimer *timer);

int software_timer_is_expired(SoftwareTimer *timer,
                              unsigned int current_tick);
```

对应功能：

```text
software_timer_init()
初始化定时器并设置周期。

software_timer_start()
从当前 Tick 开始计时。

software_timer_stop()
停止定时器。

software_timer_is_expired()
判断定时器是否到期。
```

---

## 7. 初始化定时器

实现代码：

```c
void software_timer_init(SoftwareTimer *timer,
                         unsigned int period_ms)
{
    timer->start_tick = 0;
    timer->period_ms = period_ms;
    timer->enabled = 0;

    DEBUG_PRINT("software timer init, period = %u ms\n",
                period_ms);
}
```

例如：

```c
SoftwareTimer led_timer;

software_timer_init(&led_timer, 500);
```

初始化后：

```text
start_tick = 0
period_ms = 500
enabled = 0
```

表示：

```text
LED 定时器周期已经设置为 500 ms，
但定时器还没有启动。
```

---

## 8. 启动定时器

实现代码：

```c
void software_timer_start(SoftwareTimer *timer,
                          unsigned int current_tick)
{
    timer->start_tick = current_tick;
    timer->enabled = 1;

    DEBUG_PRINT("software timer start at %u ms\n",
                current_tick);
}
```

例如：

```c
software_timer_start(&led_timer, 300);
```

执行后：

```text
start_tick = 300
period_ms = 500
enabled = 1
```

表示：

```text
定时器从 300 ms 开始计时，
经过 500 ms 后到期。
```

预计到期时间为：

```text
300 + 500 = 800 ms
```

---

## 9. 停止定时器

实现代码：

```c
void software_timer_stop(SoftwareTimer *timer)
{
    timer->enabled = 0;

    DEBUG_PRINT("software timer stopped\n");
}
```

调用：

```c
software_timer_stop(&led_timer);
```

停止后：

```text
enabled = 0
```

需要注意：

```text
停止 LED 定时器不等于关闭 LED。
```

停止定时器只表示不再周期性执行 LED 翻转任务。

LED 当前电平会保持在停止前的状态。

---

## 10. 判断定时器是否到期

核心代码：

```c
int software_timer_is_expired(SoftwareTimer *timer,
                              unsigned int current_tick)
{
    unsigned int elapsed_ms;

    if(timer->enabled == 0)
    {
        return 0;
    }

    if(timer->period_ms == 0)
    {
        DEBUG_PRINT("software timer period is zero\n");
        return 0;
    }

    elapsed_ms = current_tick - timer->start_tick;

    if(elapsed_ms >= timer->period_ms)
    {
        timer->start_tick += timer->period_ms;

        return 1;
    }

    return 0;
}
```

返回规则：

```text
返回 1：定时器到期
返回 0：定时器没有到期或没有启用
```

---

## 11. 到期判断公式

软件定时器最核心的判断是：

```c
current_tick - start_tick >= period_ms
```

可以理解为：

```text
当前时间 - 本轮开始时间
是否已经达到定时器周期
```

例如：

```text
current_tick = 800
start_tick = 500
period_ms = 500
```

已经经过：

```text
800 - 500 = 300 ms
```

因为：

```text
300 < 500
```

所以没有到期。

当：

```text
current_tick = 1000
```

已经经过：

```text
1000 - 500 = 500 ms
```

因为：

```text
500 >= 500
```

所以定时器到期。

---

## 12. 为什么使用 >= 而不是 ==

不应只判断：

```c
elapsed_ms == period_ms
```

因为主循环不一定刚好在精确时间检查定时器。

例如周期是 500 ms，但程序可能从 400 ms 直接检查到 600 ms。

这时：

```text
600 >= 500
```

说明任务已经到期，应当执行。

如果使用：

```text
600 == 500
```

条件不成立，就会错过这次任务。

因此使用：

```c
elapsed_ms >= timer->period_ms
```

更可靠。

---

## 13. 到期后更新时间基准

定时器到期后执行：

```c
timer->start_tick += timer->period_ms;
```

例如：

```text
start_tick = 500
period_ms = 500
```

到期后：

```text
start_tick = 500 + 500
           = 1000
```

下一次到期时间是：

```text
1500 ms
```

因此周期任务可以按照固定时间表运行：

```text
500
1000
1500
2000
2500
3000 ms
```

---

## 14. 为什么不直接使用 current_tick

另一种写法可能是：

```c
timer->start_tick = current_tick;
```

但如果任务本应在 500 ms 执行，实际在 520 ms 才被处理，那么下一次时间就会变成：

```text
520 + 500 = 1020 ms
```

后续周期会逐渐发生漂移。

当前采用：

```c
timer->start_tick += timer->period_ms;
```

即使任务稍晚执行，下一次仍然按照原有周期基准计算。

---

## 15. 创建三个定时器

`main.c` 中创建：

```c
SoftwareTimer sensor_timer;
SoftwareTimer led_timer;
SoftwareTimer status_timer;
```

它们虽然类型相同，但内部状态相互独立。

分别设置周期：

```c
software_timer_init(&sensor_timer, 200);
software_timer_init(&led_timer, 500);
software_timer_init(&status_timer, 1000);
```

对应关系：

```text
sensor_timer：200 ms
led_timer：500 ms
status_timer：1000 ms
```

---

## 16. 启动三个定时器

```c
software_timer_start(&sensor_timer, 0);
software_timer_start(&led_timer, 0);
software_timer_start(&status_timer, 0);
```

三个定时器都从 0 ms 开始计时。

启动后：

```text
sensor_timer
start_tick = 0
period_ms = 200
enabled = 1

led_timer
start_tick = 0
period_ms = 500
enabled = 1

status_timer
start_tick = 0
period_ms = 1000
enabled = 1
```

---

## 17. 传感器任务

定义：

```c
static void sensor_task(unsigned int current_tick)
{
    printf("[TASK][SENSOR] Executed at %u ms\n",
           current_tick);
}
```

目前只打印任务执行时间，用于模拟：

```text
读取温度
读取电压
读取电流
采集 ADC
读取其他传感器
```

执行时间：

```text
200
400
600
800
1000
...
3000 ms
```

---

## 18. LED 任务

定义：

```c
static void led_task(SystemStatus *sys,
                     unsigned int current_tick)
{
    gpio_led_toggle(sys);

    if(gpio_led_read(sys) == 1)
    {
        sys->led_state = LED_STATE_ON;
    }
    else
    {
        sys->led_state = LED_STATE_OFF;
    }

    printf("[TASK][LED] Executed at %u ms, level = %d\n",
           current_tick,
           gpio_led_read(sys));
}
```

任务执行内容：

```text
1. 翻转模拟 GPIO；
2. 读取 GPIO 电平；
3. 同步软件 LED 状态；
4. 打印任务执行时间。
```

初始 LED 为 0。

正常周期运行时：

```text
500 ms：0 → 1
1000 ms：1 → 0
1500 ms：0 → 1
2000 ms：1 → 0
```

---

## 19. 状态打印任务

定义：

```c
static void status_task(SystemStatus *sys,
                        unsigned int current_tick)
{
    printf("[TASK][STATUS] Executed at %u ms\n",
           current_tick);

    system_print(sys);
}
```

状态任务周期是 1000 ms，所以执行时间为：

```text
1000
2000
3000 ms
```

打印内容包括：

```text
LED 软件状态
GPIO 输出寄存器
LED 引脚电平
系统模式
传感器数值
故障码
运行统计
```

---

## 20. 主循环中的定时器检查

核心结构：

```c
if(software_timer_is_expired(&sensor_timer,
                             system_tick_ms))
{
    sensor_task(system_tick_ms);
}

if(software_timer_is_expired(&led_timer,
                             system_tick_ms))
{
    led_task(&sys, system_tick_ms);
}

if(software_timer_is_expired(&status_timer,
                             system_tick_ms))
{
    status_task(&sys, system_tick_ms);
}
```

可以理解为：

```text
传感器到时间了吗？
到时间就执行传感器任务。

LED 到时间了吗？
到时间就执行 LED 任务。

状态打印到时间了吗？
到时间就打印状态。
```

每个定时器独立判断，不会互相覆盖。

---

## 21. 同一 Tick 执行多个任务

在 1000 ms 时：

```text
1000 是 200 的整数倍
1000 是 500 的整数倍
1000 是 1000 的整数倍
```

所以三个任务同时到期。

程序执行顺序为：

```text
sensor_task()
led_task()
status_task()
```

运行结果：

```text
[TICK] 1000 ms
[TASK][SENSOR] Executed at 1000 ms
[TASK][LED] Executed at 1000 ms, level = 0
[TASK][STATUS] Executed at 1000 ms
```

状态任务最后执行，所以它看到的是 LED 翻转后的最新状态。

---

## 22. LED 定时器停止测试

在主循环中加入：

```c
if(system_tick_ms == 1600)
{
    software_timer_stop(&led_timer);

    printf("[CONTROL] LED timer stopped at %u ms\n",
           system_tick_ms);
}
```

1600 ms 时：

```text
enabled = 0
```

LED 定时器停止。

此后即使到达原计划中的：

```text
2000 ms
2500 ms
```

LED 任务也不会执行。

---

## 23. 停止定时器不改变 LED 状态

1500 ms 时 LED 从 0 翻转为 1。

1600 ms 停止定时器后，LED 仍然保持为 1。

所以 2000 ms 状态打印为：

```text
LED state: 1
GPIO output reg: 0x00000001
LED pin level: 1
```

这说明：

```text
停止定时器只停止周期任务，
不会主动改变被控制对象的当前状态。
```

---

## 24. LED 定时器重新启动测试

在 2300 ms 时执行：

```c
if(system_tick_ms == 2300)
{
    software_timer_start(&led_timer, system_tick_ms);

    printf("[CONTROL] LED timer restarted at %u ms\n",
           system_tick_ms);
}
```

重新启动后：

```text
start_tick = 2300
period_ms = 500
enabled = 1
```

下一次到期时间：

```text
2300 + 500 = 2800 ms
```

因此 LED 在 2800 ms 再次执行。

---

## 25. 为什么 3000 ms 不再执行 LED

2800 ms 到期后：

```text
start_tick = 2800
```

到 3000 ms 已经过：

```text
3000 - 2800 = 200 ms
```

因为：

```text
200 < 500
```

所以 LED 定时器尚未再次到期。

3000 ms 只执行：

```text
传感器任务
状态打印任务
```

---

## 26. 关键运行结果

初始化：

```text
[DEBUG] software timer init, period = 200 ms
[DEBUG] software timer init, period = 500 ms
[DEBUG] software timer init, period = 1000 ms
```

500 ms：

```text
[TICK] 500 ms
[TASK][LED] Executed at 500 ms, level = 1
```

1000 ms：

```text
[TICK] 1000 ms
[TASK][SENSOR] Executed at 1000 ms
[TASK][LED] Executed at 1000 ms, level = 0
[TASK][STATUS] Executed at 1000 ms
```

停止定时器：

```text
[TICK] 1600 ms
[DEBUG] software timer stopped
[CONTROL] LED timer stopped at 1600 ms
```

2000 ms 时 LED 没有执行：

```text
[TICK] 2000 ms
[TASK][SENSOR] Executed at 2000 ms
[TASK][STATUS] Executed at 2000 ms
```

重新启动：

```text
[TICK] 2300 ms
[DEBUG] software timer start at 2300 ms
[CONTROL] LED timer restarted at 2300 ms
```

重新启动后第一次到期：

```text
[TICK] 2800 ms
[TASK][SENSOR] Executed at 2800 ms
[TASK][LED] Executed at 2800 ms, level = 0
```

---

## 27. 编译运行

进入 Day18：

```bash
cd /root/Embedded_14Days/day18
```

清理旧文件：

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

筛选任务输出：

```bash
make run | grep -E "\[TICK\]|\[TASK\]|\[CONTROL\]"
```

---

## 28. Day18 的核心理解

Day18 最重要的公式是：

```c
current_tick - start_tick >= period_ms
```

它表示：

```text
从本轮开始计时到现在，
经过的时间是否已经达到任务周期。
```

整个调度流程为：

```text
Tick 不断增加
      ↓
检查 sensor_timer
      ↓
检查 led_timer
      ↓
检查 status_timer
      ↓
哪个到期就执行哪个任务
      ↓
主循环继续运行
```

这是一种基础的：

```text
非阻塞轮询调度
```

---

## 29. 当前模拟和真实硬件的区别

当前程序使用：

```c
system_tick_ms += 100;
```

人为模拟时间增加。

它并不代表程序真的经过了 100 ms。

真实单片机通常使用：

```text
SysTick 中断
硬件定时器中断
RTOS 系统 Tick
```

来维护真实时间。

真实结构通常是：

```text
定时器中断每 1 ms 发生一次
        ↓
system_tick_ms 加 1
        ↓
主循环读取 Tick
        ↓
软件定时器判断任务是否到期
```

因此当前软件定时器代码以后可以继续迁移到真实开发板。

---

## 30. Day18 今日收获

通过 Day18 学习，掌握了：

1. 系统 Tick 的基本概念；
2. 阻塞式延时和非阻塞调度的区别；
3. 软件定时器结构体设计；
4. `start_tick` 的作用；
5. `period_ms` 的作用；
6. `enabled` 的作用；
7. 软件定时器初始化；
8. 软件定时器启动；
9. 软件定时器停止；
10. 软件定时器到期判断；
11. 使用时间差判断任务周期；
12. 为什么使用 `>=` 而不是 `==`；
13. 多个定时器独立运行；
14. 同一 Tick 执行多个任务；
15. 定时器停止后任务不再执行；
16. 定时器重新启动后从新 Tick 计时；
17. LED 软件状态与 GPIO 状态同步；
18. 非阻塞周期任务调度的基本结构。

阶段关系：

```text
Day15：GPIO 寄存器和位操作
Day16：UART 完整命令解析
Day17：Ring Buffer 和 UART 字节流
Day18：系统 Tick 和软件定时器调度
```
