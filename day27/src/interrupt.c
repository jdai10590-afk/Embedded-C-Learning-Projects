#include <stddef.h>

#include "interrupt.h"

int interrupt_source_is_valid(
    uint32_t source)
{
    /*
     * 当前接口一次只操作一个中断源。
     *
     * INTERRUPT_SOURCE_ALL 虽然是合法位组合，
     * 但不允许作为单个中断源传入。
     */
    if((source ==
        INTERRUPT_SOURCE_BUTTON) ||
       (source ==
        INTERRUPT_SOURCE_UART_RX))
    {
        return 1;
    }

    return 0;
}

int interrupt_event_is_valid(
    uint32_t event)
{
    /*
     * 当前接口一次只操作一个软件事件。
     */
    if((event ==
        INTERRUPT_EVENT_BUTTON) ||
       (event ==
        INTERRUPT_EVENT_UART_RX))
    {
        return 1;
    }

    return 0;
}

int interrupt_init(
    InterruptController *controller)
{
    if(controller == NULL)
    {
        return 0;
    }

    /*
     * 初始化时关闭所有中断。
     */
    controller->enabled_flags =
        INTERRUPT_SOURCE_NONE;

    /*
     * 当前没有等待处理的硬件中断。
     */
    controller->pending_flags =
        INTERRUPT_SOURCE_NONE;

    /*
     * 当前没有等待主循环处理的软件事件。
     */
    controller->event_flags =
        INTERRUPT_EVENT_NONE;

    /*
     * 清零 UART 模拟寄存器和事件数据。
     */
    controller->uart_rx_data_register = 0U;
    controller->uart_rx_event_data = 0U;

    /*
     * 清零中断统计。
     */
    controller->irq_request_count = 0U;
    controller->handled_irq_count = 0U;
    controller->button_isr_count = 0U;
    controller->uart_rx_isr_count = 0U;

    return 1;
}

int interrupt_enable(
    InterruptController *controller,
    uint32_t source)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_source_is_valid(
           source) == 0)
    {
        return 0;
    }

    /*
     * 使用按位或设置对应中断使能位。
     *
     * 其他中断源的状态不会受到影响。
     */
    controller->enabled_flags |= source;

    return 1;
}

int interrupt_disable(
    InterruptController *controller,
    uint32_t source)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_source_is_valid(
           source) == 0)
    {
        return 0;
    }

    /*
     * 使用按位与和按位取反，
     * 清除指定中断使能位。
     *
     * 注意：
     * 禁止中断不会清除 pending。
     */
    controller->enabled_flags &=
        ~source;

    return 1;
}

int interrupt_is_enabled(
    const InterruptController *controller,
    uint32_t source)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_source_is_valid(
           source) == 0)
    {
        return 0;
    }

    if((controller->enabled_flags &
        source) != 0U)
    {
        return 1;
    }

    return 0;
}

int interrupt_is_pending(
    const InterruptController *controller,
    uint32_t source)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_source_is_valid(
           source) == 0)
    {
        return 0;
    }

    if((controller->pending_flags &
        source) != 0U)
    {
        return 1;
    }

    return 0;
}

int interrupt_request(
    InterruptController *controller,
    uint32_t source)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_source_is_valid(
           source) == 0)
    {
        return 0;
    }

    /*
     * 外设事件发生后，设置 pending。
     *
     * 即使中断还没有使能，
     * pending 仍然可以被记录。
     */
    controller->pending_flags |= source;

    /*
     * 统计外设提交中断请求的次数。
     */
    controller->irq_request_count++;

    return 1;
}

int interrupt_uart_receive_byte(
    InterruptController *controller,
    uint8_t received_byte)
{
    if(controller == NULL)
    {
        return 0;
    }

    /*
     * 模拟 UART 硬件把接收到的字节
     * 保存到数据寄存器。
     */
    controller->uart_rx_data_register =
        received_byte;

    /*
     * 产生 UART RX 中断请求。
     *
     * interrupt_request() 内部会：
     *
     * 设置 UART pending
     * 增加 irq_request_count
     */
    return interrupt_request(
        controller,
        INTERRUPT_SOURCE_UART_RX);
}

int interrupt_clear_pending(
    InterruptController *controller,
    uint32_t source)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_source_is_valid(
           source) == 0)
    {
        return 0;
    }

    /*
     * 清除指定中断源的 pending 位。
     */
    controller->pending_flags &=
        ~source;

    return 1;
}

int interrupt_event_is_pending(
    const InterruptController *controller,
    uint32_t event)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_event_is_valid(
           event) == 0)
    {
        return 0;
    }

    if((controller->event_flags &
        event) != 0U)
    {
        return 1;
    }

    return 0;
}

int interrupt_clear_event(
    InterruptController *controller,
    uint32_t event)
{
    if(controller == NULL)
    {
        return 0;
    }

    if(interrupt_event_is_valid(
           event) == 0)
    {
        return 0;
    }

    /*
     * 主循环处理完事件后，
     * 清除对应软件事件位。
     */
    controller->event_flags &=
        ~event;

    return 1;
}

void interrupt_button_isr(
    InterruptController *controller)
{
    if(controller == NULL)
    {
        return;
    }

    /*
     * 没有按键 pending 时，
     * 不执行中断处理。
     */
    if(interrupt_is_pending(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        return;
    }

    /*
     * ISR 第一步：
     * 清除硬件中断挂起标志。
     *
     * 在真实 MCU 中通常对应：
     *
     * 清除 EXTI 中断标志
     * 清除 GPIO 边沿标志
     */
    (void)interrupt_clear_pending(
        controller,
        INTERRUPT_SOURCE_BUTTON);

    /*
     * ISR 第二步：
     * 设置软件事件。
     *
     * 按键的消抖、短按、长按和业务处理，
     * 后续放到主循环中完成。
     */
    controller->event_flags |=
        INTERRUPT_EVENT_BUTTON;

    /*
     * 更新中断统计。
     */
    controller->button_isr_count++;
    controller->handled_irq_count++;
}

void interrupt_uart_rx_isr(
    InterruptController *controller)
{
    if(controller == NULL)
    {
        return;
    }

    /*
     * 没有 UART RX pending 时，
     * 不执行中断处理。
     */
    if(interrupt_is_pending(
           controller,
           INTERRUPT_SOURCE_UART_RX) == 0)
    {
        return;
    }

    /*
     * ISR 快速读取模拟 UART 数据寄存器。
     *
     * 真实 MCU 中通常需要及时读取接收寄存器，
     * 避免下一个字节到来后覆盖当前数据。
     */
    controller->uart_rx_event_data =
        controller->uart_rx_data_register;

    /*
     * 清除 UART RX pending。
     */
    (void)interrupt_clear_pending(
        controller,
        INTERRUPT_SOURCE_UART_RX);

    /*
     * 通知主循环：
     * 有一个 UART 字节等待处理。
     */
    controller->event_flags |=
        INTERRUPT_EVENT_UART_RX;

    /*
     * 更新中断统计。
     */
    controller->uart_rx_isr_count++;
    controller->handled_irq_count++;
}

int interrupt_dispatch(
    InterruptController *controller)
{
    int handled_count;

    if(controller == NULL)
    {
        return 0;
    }

    handled_count = 0;

    /*
     * Day27 暂时规定按键中断先检查。
     *
     * 这里只是固定的软件检查顺序，
     * 还不是真正的硬件中断优先级。
     */
    if((interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_BUTTON) != 0) &&
       (interrupt_is_enabled(
            controller,
            INTERRUPT_SOURCE_BUTTON) != 0))
    {
        interrupt_button_isr(
            controller);

        handled_count++;
    }

    /*
     * 再检查 UART RX 中断。
     */
    if((interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_UART_RX) != 0) &&
       (interrupt_is_enabled(
            controller,
            INTERRUPT_SOURCE_UART_RX) != 0))
    {
        interrupt_uart_rx_isr(
            controller);

        handled_count++;
    }

    /*
     * 返回本次 dispatch 实际执行的 ISR 数量。
     */
    return handled_count;
}