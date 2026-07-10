# Day17：UART 环形缓冲区与分段命令接收

## 1. 学习目标

Day17 在 Day16 UART 字符串命令解析的基础上，引入 Ring Buffer 环形缓冲区。

Day16 中，主程序直接把完整命令交给 UART 模块：

```c
uart_process_command(&sys, "LED ON");
```

但真实 UART 通常是一个字节一个字节接收，例如：

```text
'L'
'E'
'D'
' '
'O'
'N'
'\r'
'\n'
```

因此 Day17 实现以下流程：

```text
UART 字节逐个到达
        ↓
写入 Ring Buffer
        ↓
主程序逐个取出字符
        ↓
拼接 command_buffer
        ↓
遇到 '\n'
        ↓
得到完整命令
        ↓
调用 uart_process_command()
```

---

## 2. 工程结构

```text
day17
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
│   ├── state_machine.c
│   ├── system.c
│   └── uart.c
└── build
    └── day17_test
```

Day17 新增：

```text
include/ring_buffer.h
src/ring_buffer.c
```

并修改：

```text
include/uart.h
src/uart.c
src/main.c
Makefile
```

---

## 3. Ring Buffer 数据结构

`ring_buffer.h` 中定义：

```c
#define RING_BUFFER_SIZE 64

typedef struct
{
    char data[RING_BUFFER_SIZE];

    unsigned int head;
    unsigned int tail;
    unsigned int count;
} RingBuffer;
```

各成员含义：

```text
data
实际保存 UART 字符的数组。

head
下一次写入数据的位置。

tail
下一次读取数据的位置。

count
当前缓冲区内尚未读取的数据数量。
```

初始化状态：

```text
head = 0
tail = 0
count = 0
```

---

## 4. 为什么需要 count

仅看：

```text
head == tail
```

无法判断缓冲区是空还是满。

缓冲区为空时：

```text
head = 0
tail = 0
count = 0
```

缓冲区写满并绕回后：

```text
head = 0
tail = 0
count = 64
```

因此使用：

```text
count == 0
```

判断缓冲区为空。

使用：

```text
count == RING_BUFFER_SIZE
```

判断缓冲区已满。

---

## 5. 初始化函数

```c
void ring_buffer_init(RingBuffer *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;

    DEBUG_PRINT("ring buffer init done\n");
}
```

调用方法：

```c
RingBuffer uart_rx_buffer;

ring_buffer_init(&uart_rx_buffer);
```

初始化后，缓冲区可以安全进行写入和读取。

---

## 6. 判空和判满

判断缓冲区为空：

```c
int ring_buffer_is_empty(const RingBuffer *rb)
{
    return rb->count == 0;
}
```

返回规则：

```text
返回 1：缓冲区为空
返回 0：缓冲区不为空
```

判断缓冲区已满：

```c
int ring_buffer_is_full(const RingBuffer *rb)
{
    return rb->count == RING_BUFFER_SIZE;
}
```

返回规则：

```text
返回 1：缓冲区已满
返回 0：缓冲区未满
```

---

## 7. 写入数据 push

```c
int ring_buffer_push(RingBuffer *rb, char byte)
{
    if(ring_buffer_is_full(rb))
    {
        DEBUG_PRINT("ring buffer full, push failed\n");
        return 0;
    }

    rb->data[rb->head] = byte;

    rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
    rb->count++;

    return 1;
}
```

核心步骤：

```text
1. 判断缓冲区是否已满；
2. 将 byte 写入 data[head]；
3. head 移动到下一个位置；
4. count 增加 1；
5. 返回写入成功。
```

调用示例：

```c
ring_buffer_push(&uart_rx_buffer, 'L');
```

返回规则：

```text
返回 1：写入成功
返回 0：缓冲区已满
```

---

## 8. 读取数据 pop

```c
int ring_buffer_pop(RingBuffer *rb, char *byte)
{
    if(ring_buffer_is_empty(rb))
    {
        DEBUG_PRINT("ring buffer empty, pop failed\n");
        return 0;
    }

    *byte = rb->data[rb->tail];

    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
    rb->count--;

    return 1;
}
```

核心步骤：

```text
1. 判断缓冲区是否为空；
2. 读取 data[tail]；
3. 将结果写入调用者的字符变量；
4. tail 移动到下一个位置；
5. count 减少 1；
6. 返回读取成功。
```

调用示例：

```c
char received_byte;

ring_buffer_pop(&uart_rx_buffer, &received_byte);
```

---

## 9. 环形回绕

head 和 tail 的移动方式为：

```c
(rb->head + 1) % RING_BUFFER_SIZE
```

以及：

```c
(rb->tail + 1) % RING_BUFFER_SIZE
```

假设：

```text
RING_BUFFER_SIZE = 64
head = 63
```

执行：

```text
(63 + 1) % 64
= 64 % 64
= 0
```

head 会从数组末尾重新回到下标 0。

移动过程为：

```text
0 → 1 → 2 → ... → 62 → 63 → 0 → 1
```

这就是环形缓冲区名称的来源。

---

## 10. FIFO 先进先出

环形缓冲区遵循 FIFO：

```text
First In, First Out
先进先出
```

例如依次写入：

```text
A
B
C
```

读取顺序仍然是：

```text
A
B
C
```

最早写入的数据最先被读取。

---

## 11. UART 字节接收

Day17 新增：

```c
int uart_receive_byte(RingBuffer *rx_buffer, char byte)
{
    if(ring_buffer_push(rx_buffer, byte) == 0)
    {
        printf("[UART] RX buffer full, byte lost: 0x%02X\n",
               (unsigned char)byte);

        return 0;
    }

    DEBUG_PRINT("uart received byte: 0x%02X\n",
                (unsigned char)byte);

    return 1;
}
```

该函数用于模拟 UART 每次收到一个字节。

例如：

```c
uart_receive_byte(&uart_rx_buffer, 'L');
uart_receive_byte(&uart_rx_buffer, 'E');
uart_receive_byte(&uart_rx_buffer, 'D');
```

字符会逐个进入 Ring Buffer。

---

## 12. UART 命令拼接

Day17 使用以下静态变量保存正在接收的命令：

```c
static char command_buffer[UART_COMMAND_MAX_LENGTH];
static unsigned int command_length = 0;
static int command_overflow = 0;
```

其中：

```text
command_buffer
保存正在拼接的命令字符。

command_length
记录当前已经接收了多少个命令字符。

command_overflow
记录当前命令是否超过最大长度。
```

使用 `static` 后，即使函数退出，这些变量的内容仍然保留。

因此一条命令可以分多次到达。

---

## 13. 回车和换行处理

串口终端常使用：

```text
\r\n
```

作为一行结束符。

其中：

```text
\r：回车，十六进制 0x0D
\n：换行，十六进制 0x0A
```

程序遇到 `\r` 时忽略：

```c
if(byte == '\r')
{
    continue;
}
```

遇到 `\n` 时认为一条命令接收完成：

```c
if(byte == '\n')
{
    command_buffer[command_length] = '\0';

    uart_process_command(sys, command_buffer);
}
```

例如接收到：

```text
'L' 'E' 'D' ' ' 'O' 'N' '\r' '\n'
```

最终拼成：

```text
LED ON
```

---

## 14. 字符串结束符

C 字符串必须以：

```c
'\0'
```

结尾。

因此一条命令接收完成后，需要执行：

```c
command_buffer[command_length] = '\0';
```

例如：

```text
'L' 'E' 'D' ' ' 'O' 'N' '\0'
```

才能作为合法字符串传给：

```c
strcmp()
printf("%s")
uart_process_command()
```

---

## 15. 模拟 UART 字节逐个到达

`main.c` 中新增：

```c
static void uart_simulate_input(SystemStatus *sys,
                                RingBuffer *rx_buffer,
                                const char *text)
{
    unsigned int index = 0;

    printf("\n[SIM] Sending: %s", text);

    while(text[index] != '\0')
    {
        uart_receive_byte(rx_buffer, text[index]);
        index++;
    }

    uart_process_rx_buffer(sys, rx_buffer);
}
```

该函数把完整测试字符串拆成单个字符。

例如：

```c
uart_simulate_input(
    &sys,
    &uart_rx_buffer,
    "LED ON\r\n"
);
```

实际处理顺序是：

```text
'L'
'E'
'D'
' '
'O'
'N'
'\r'
'\n'
```

每个字符分别进入 Ring Buffer，然后再被取出并拼接。

---

## 16. 分段命令测试

测试代码：

```c
uart_simulate_input(&sys, &uart_rx_buffer, "LED ");
uart_simulate_input(&sys, &uart_rx_buffer, "ON\r\n");
```

第一次只收到：

```text
LED 
```

因为没有换行符，所以程序不会立即执行命令。

此时静态变量保存：

```text
command_buffer = "LED "
command_length = 4
```

第二次收到：

```text
ON\r\n
```

最终拼接为：

```text
LED ON
```

随后执行：

```text
[UART] Received command: LED ON
[UART] LED turned ON
```

这说明程序支持一条命令分多次接收。

---

## 17. 命令长度溢出保护

定义：

```c
#define UART_COMMAND_MAX_LENGTH 32
```

数组总长度是 32，但最后一个位置必须留给：

```c
'\0'
```

因此最多只能保存 31 个有效命令字符。

如果命令超过长度，程序执行：

```c
command_overflow = 1;
printf("[UART] Command too long\n");
```

在遇到换行符后，整条命令被丢弃：

```text
[UART] Command discarded after overflow
```

然后恢复：

```c
command_length = 0;
command_overflow = 0;
```

后续正常命令仍然可以继续处理。

测试输出：

```text
[UART] Command too long
[UART] Command discarded after overflow
```

随后 `STATUS` 命令仍能正常执行。

---

## 18. Ring Buffer 写满测试

连续写入 64 个字符后：

```text
head  = 0
tail  = 0
count = 64
full  = 1
```

继续写入一个字符时：

```text
[DEBUG] ring buffer full, push failed
[TEST] Extra push result = 0
```

说明程序不会覆盖尚未读取的数据。

---

## 19. Ring Buffer 读空测试

读取全部 64 个字符后：

```text
head  = 0
tail  = 0
count = 0
empty = 1
```

继续读取时：

```text
[DEBUG] ring buffer empty, pop failed
[TEST] Extra pop result = 0
```

说明程序不会从空缓冲区读取无效数据。

---

## 20. 编译运行

进入 Day17：

```bash
cd /root/Embedded_14Days/day17
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

查看 Ring Buffer 边界测试：

```bash
make run | sed -n '/Ring Buffer Boundary Test/,$p'
```

---

## 21. 运行结果总结

Day17 已成功实现：

```text
STATUS
LED ON
LED OFF
LED TOGGLE
RESET
未知命令
UART 字节逐个接收
分段命令接收
命令长度溢出保护
Ring Buffer 写满保护
Ring Buffer 读空保护
head 和 tail 环形回绕
```

关键边界测试结果：

```text
===== Ring Buffer Boundary Test =====
[TEST] After filling:
head  = 0
tail  = 0
count = 64
full  = 1
[DEBUG] ring buffer full, push failed
[TEST] Extra push result = 0

[TEST] After draining:
head  = 0
tail  = 0
count = 0
empty = 1
[DEBUG] ring buffer empty, pop failed
[TEST] Extra pop result = 0
```

---

## 22. Day17 核心理解

Day16 的处理方式：

```text
一次得到完整命令
        ↓
直接进行字符串比较
```

Day17 的处理方式：

```text
字符逐个到达
        ↓
Ring Buffer 暂存
        ↓
逐个读取字符
        ↓
拼成完整字符串
        ↓
解析命令
```

Day17 的核心可以总结为：

```text
Ring Buffer 解决数据接收速度和处理速度不完全一致的问题。
```

当 UART 字节不断到达时，可以先将数据存入缓冲区，再由主程序逐步处理。

---

## 23. 今日收获

通过 Day17 学习，掌握了：

1. Ring Buffer 环形缓冲区的基本结构；
2. `head`、`tail` 和 `count` 的作用；
3. FIFO 先进先出机制；
4. 环形下标回绕；
5. 缓冲区判空和判满；
6. Ring Buffer 字符写入和读取；
7. UART 字节逐个接收；
8. 使用 `\r\n` 判断命令结束；
9. C 字符串结束符 `\0`；
10. `static` 局部变量保存跨函数调用状态；
11. 分段命令拼接；
12. 命令长度溢出保护；
13. 缓冲区满和空的边界保护。

阶段关系：

```text
Day15：GPIO 寄存器与位操作
Day16：UART 完整字符串命令解析
Day17：Ring Buffer 与 UART 字节流接收
```
