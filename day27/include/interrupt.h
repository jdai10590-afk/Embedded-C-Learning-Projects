#ifndef INTERRUPT_H
#define INTERRUPT_H

#include <stdint.h>

/*
 * Day27 模拟两个中断源：
 *
 * bit0：按键外部中断
 * bit1：UART 接收中断
 */
#define INTERRUPT_SOURCE_NONE       0x00000000UL
#define INTERRUPT_SOURCE_BUTTON     (1UL << 0)
#define INTERRUPT_SOURCE_UART_RX    (1UL << 1)

#define INTERRUPT_SOURCE_ALL        \
    (INTERRUPT_SOURCE_BUTTON |      \
     INTERRUPT_SOURCE_UART_RX)

/*
 * ISR 产生的软件事件。
 *
 * 中断源表示：
 * 硬件侧发生了什么。
 *
 * 事件标志表示：
 * ISR 已经记录了什么，
 * 等待主循环继续处理。
 */
#define INTERRUPT_EVENT_NONE        0x00000000UL
#define INTERRUPT_EVENT_BUTTON      (1UL << 0)
#define INTERRUPT_EVENT_UART_RX     (1UL << 1)

#define INTERRUPT_EVENT_ALL         \
    (INTERRUPT_EVENT_BUTTON |       \
     INTERRUPT_EVENT_UART_RX)

/*
 * 模拟中断控制器。
 */
typedef struct
{
    /*
     * 中断使能标志。
     *
     * 某一位为 1：
     * 对应中断允许进入 ISR。
     *
     * 某一位为 0：
     * 对应中断暂时禁止处理。
     */
    volatile uint32_t enabled_flags;

    /*
     * 中断挂起标志。
     *
     * 外设事件发生后设置。
     *
     * ISR 处理完成后清除。
     */
    volatile uint32_t pending_flags;

    /*
     * 软件事件标志。
     *
     * ISR 不执行复杂业务，
     * 只设置对应事件位。
     *
     * 主循环发现事件后再完成处理。
     */
    volatile uint32_t event_flags;

    /*
     * 模拟 UART 硬件接收数据寄存器。
     *
     * UART 收到一个字节后，
     * 先把数据保存到这里，
     * 再产生 UART RX 中断。
     */
    volatile uint8_t uart_rx_data_register;

    /*
     * UART ISR 保存给主循环的数据。
     *
     * ISR 从 uart_rx_data_register
     * 读取字节并保存到这里。
     */
    volatile uint8_t uart_rx_event_data;

    /*
     * 外设提交中断请求的总次数。
     *
     * 即使中断没有使能，
     * 请求仍然可以被记录。
     */
    volatile uint32_t irq_request_count;

    /*
     * 已经进入 ISR 并完成处理的
     * 中断总次数。
     */
    volatile uint32_t handled_irq_count;

    /*
     * 按键 ISR 执行次数。
     */
    volatile uint32_t button_isr_count;

    /*
     * UART RX ISR 执行次数。
     */
    volatile uint32_t uart_rx_isr_count;
} InterruptController;

/*
 * 判断是否为合法的单个中断源。
 *
 * 当前合法值：
 *
 * INTERRUPT_SOURCE_BUTTON
 * INTERRUPT_SOURCE_UART_RX
 *
 * 合法返回 1，非法返回 0。
 */
int interrupt_source_is_valid(
    uint32_t source);

/*
 * 判断是否为合法的单个软件事件。
 *
 * 当前合法值：
 *
 * INTERRUPT_EVENT_BUTTON
 * INTERRUPT_EVENT_UART_RX
 *
 * 合法返回 1，非法返回 0。
 */
int interrupt_event_is_valid(
    uint32_t event);

/*
 * 初始化中断控制器。
 *
 * 初始化后：
 *
 * 所有中断关闭
 * 所有 pending 清零
 * 所有事件清零
 * UART 数据清零
 * 所有计数清零
 *
 * 成功返回 1，失败返回 0。
 */
int interrupt_init(
    InterruptController *controller);

/*
 * 使能一个中断源。
 *
 * 例如：
 *
 * interrupt_enable(
 *     &controller,
 *     INTERRUPT_SOURCE_BUTTON);
 *
 * 成功返回 1，失败返回 0。
 */
int interrupt_enable(
    InterruptController *controller,
    uint32_t source);

/*
 * 禁止一个中断源。
 *
 * 禁止中断不会自动清除已经存在的
 * pending 标志。
 *
 * 成功返回 1，失败返回 0。
 */
int interrupt_disable(
    InterruptController *controller,
    uint32_t source);

/*
 * 查询中断源是否已经使能。
 *
 * 已使能返回 1，
 * 未使能或参数错误返回 0。
 */
int interrupt_is_enabled(
    const InterruptController *controller,
    uint32_t source);

/*
 * 查询中断是否处于挂起状态。
 *
 * pending 为 1 返回 1，
 * 否则返回 0。
 */
int interrupt_is_pending(
    const InterruptController *controller,
    uint32_t source);

/*
 * 提交一个通用中断请求。
 *
 * 操作：
 *
 * pending_flags 设置对应位
 * irq_request_count 加 1
 *
 * 注意：
 *
 * 提交请求不代表 ISR 会立即执行。
 * 只有对应中断已经使能，
 * 调度器才会进入 ISR。
 *
 * 成功返回 1，失败返回 0。
 */
int interrupt_request(
    InterruptController *controller,
    uint32_t source);

/*
 * 模拟 UART 收到一个字节。
 *
 * 操作：
 *
 * 1. 保存 received_byte
 * 2. 设置 UART RX pending
 * 3. irq_request_count 加 1
 *
 * 成功返回 1，失败返回 0。
 */
int interrupt_uart_receive_byte(
    InterruptController *controller,
    uint8_t received_byte);

/*
 * 清除指定中断的 pending 标志。
 *
 * ISR 通常需要在完成必要处理后
 * 清除中断标志。
 *
 * 成功返回 1，失败返回 0。
 */
int interrupt_clear_pending(
    InterruptController *controller,
    uint32_t source);

/*
 * 查询软件事件是否正在等待处理。
 *
 * 事件存在返回 1，
 * 否则返回 0。
 */
int interrupt_event_is_pending(
    const InterruptController *controller,
    uint32_t event);

/*
 * 清除主循环已经处理完成的软件事件。
 *
 * 成功返回 1，失败返回 0。
 */
int interrupt_clear_event(
    InterruptController *controller,
    uint32_t event);

/*
 * 按键中断服务函数。
 *
 * ISR 只完成：
 *
 * 清除按键 pending
 * 设置按键软件事件
 * 增加中断计数
 *
 * 不执行耗时业务。
 */
void interrupt_button_isr(
    InterruptController *controller);

/*
 * UART 接收中断服务函数。
 *
 * ISR 只完成：
 *
 * 读取 UART 数据寄存器
 * 保存接收字节
 * 清除 UART pending
 * 设置 UART 软件事件
 * 增加中断计数
 */
void interrupt_uart_rx_isr(
    InterruptController *controller);

/*
 * 模拟 CPU 的中断调度过程。
 *
 * 调度器检查：
 *
 * pending_flags
 * enabled_flags
 *
 * 只有同时满足：
 *
 * pending = 1
 * enabled = 1
 *
 * 才会调用对应 ISR。
 *
 * 返回本次实际处理的中断数量。
 */
int interrupt_dispatch(
    InterruptController *controller);

#endif