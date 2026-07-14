#include <stdio.h>

#include "interrupt.h"

/*
 * 主循环处理软件事件时使用的统计信息。
 *
 * 这些数据只在主循环中修改，
 * 因此本次不需要使用 volatile。
 */
typedef struct
{
    uint32_t button_event_count;
    uint32_t uart_event_count;
    uint8_t last_uart_byte;
} ApplicationEventStats;

/*
 * 模拟主循环的软件事件处理。
 *
 * ISR 只设置 event_flags，
 * 真正的业务处理放在这里完成。
 *
 * 返回本次处理的软件事件数量。
 */
static int process_pending_events(
    InterruptController *controller,
    ApplicationEventStats *stats)
{
    int processed_count;
    uint8_t received_byte;

    if((controller == NULL) ||
       (stats == NULL))
    {
        return 0;
    }

    processed_count = 0;

    /*
     * 处理按键软件事件。
     */
    if(interrupt_event_is_pending(
           controller,
           INTERRUPT_EVENT_BUTTON) != 0)
    {
        printf(
            "[MAIN] Processing button event\n");

        /*
         * 主循环完成业务后，
         * 清除软件事件标志。
         */
        if(interrupt_clear_event(
               controller,
               INTERRUPT_EVENT_BUTTON) == 0)
        {
            printf(
                "[MAIN ERROR] "
                "Failed to clear button event\n");
        }
        else
        {
            stats->button_event_count++;
            processed_count++;
        }
    }

    /*
     * 处理 UART 接收软件事件。
     */
    if(interrupt_event_is_pending(
           controller,
           INTERRUPT_EVENT_UART_RX) != 0)
    {
        /*
         * ISR 已经把硬件接收寄存器中的数据
         * 保存到了 uart_rx_event_data。
         */
        received_byte =
            controller->uart_rx_event_data;

        printf(
            "[MAIN] Processing UART byte "
            "'%c' (0x%02X)\n",
            (int)received_byte,
            (unsigned int)received_byte);

        if(interrupt_clear_event(
               controller,
               INTERRUPT_EVENT_UART_RX) == 0)
        {
            printf(
                "[MAIN ERROR] "
                "Failed to clear UART event\n");
        }
        else
        {
            stats->last_uart_byte =
                received_byte;

            stats->uart_event_count++;
            processed_count++;
        }
    }

    return processed_count;
}

/*
 * 测试中断控制器初始化状态。
 */
static void test_initial_state(
    const InterruptController *controller,
    const ApplicationEventStats *stats)
{
    printf("\n");
    printf(
        "----- Test 1: Initial State -----\n");

    printf(
        "[STATE] Enabled flags = 0x%08lX\n",
        (unsigned long)
        controller->enabled_flags);

    printf(
        "[STATE] Pending flags = 0x%08lX\n",
        (unsigned long)
        controller->pending_flags);

    printf(
        "[STATE] Event flags   = 0x%08lX\n",
        (unsigned long)
        controller->event_flags);

    if((controller->enabled_flags ==
        INTERRUPT_SOURCE_NONE) &&
       (controller->pending_flags ==
        INTERRUPT_SOURCE_NONE) &&
       (controller->event_flags ==
        INTERRUPT_EVENT_NONE) &&
       (controller->irq_request_count == 0U) &&
       (controller->handled_irq_count == 0U) &&
       (controller->button_isr_count == 0U) &&
       (controller->uart_rx_isr_count == 0U) &&
       (stats->button_event_count == 0U) &&
       (stats->uart_event_count == 0U))
    {
        printf(
            "[CHECK] Initial interrupt state is correct\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Initial interrupt state mismatch\n");
    }
}

/*
 * 测试中断未使能时：
 *
 * 1. IRQ 请求仍然能够设置 pending；
 * 2. 调度器不会执行 ISR；
 * 3. 使能中断后，原来的 pending 可以继续处理。
 */
static void test_pending_while_disabled(
    InterruptController *controller,
    ApplicationEventStats *stats)
{
    int dispatched_count;
    int processed_count;

    dispatched_count = 0;
    processed_count = 0;

    printf("\n");
    printf(
        "----- Test 2: Pending While Disabled -----\n");

    /*
     * 初始化后按键中断尚未使能。
     */
    printf(
        "[STATE] Button enabled = %s\n",
        interrupt_is_enabled(
            controller,
            INTERRUPT_SOURCE_BUTTON) ?
        "YES" :
        "NO");

    /*
     * 模拟按键外设提交 IRQ。
     */
    if(interrupt_request(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[ERROR] Failed to request button IRQ\n");

        return;
    }

    printf(
        "[IRQ] Button request generated\n");

    printf(
        "[STATE] Button pending = %s\n",
        interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_BUTTON) ?
        "YES" :
        "NO");

    /*
     * 中断未使能，调度器不应执行 ISR。
     */
    dispatched_count =
        interrupt_dispatch(controller);

    printf(
        "[DISPATCH] Handled in disabled state = %d\n",
        dispatched_count);

    if((dispatched_count == 0) &&
       (interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_BUTTON) != 0) &&
       (interrupt_event_is_pending(
            controller,
            INTERRUPT_EVENT_BUTTON) == 0))
    {
        printf(
            "[CHECK] Disabled IRQ remained pending\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Disabled IRQ state mismatch\n");
    }

    /*
     * 使能按键中断。
     */
    if(interrupt_enable(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[ERROR] Failed to enable button IRQ\n");

        return;
    }

    printf(
        "[IRQ] Button interrupt enabled\n");

    /*
     * 原来保留的 pending 现在应被处理。
     */
    dispatched_count =
        interrupt_dispatch(controller);

    printf(
        "[DISPATCH] Handled after enabling = %d\n",
        dispatched_count);

    if((dispatched_count == 1) &&
       (interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_BUTTON) == 0) &&
       (interrupt_event_is_pending(
            controller,
            INTERRUPT_EVENT_BUTTON) != 0))
    {
        printf(
            "[CHECK] Stored button IRQ entered ISR\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Stored button IRQ was not handled correctly\n");
    }

    /*
     * 模拟主循环处理按键软件事件。
     */
    processed_count =
        process_pending_events(
            controller,
            stats);

    printf(
        "[MAIN] Processed events = %d\n",
        processed_count);

    if((processed_count == 1) &&
       (stats->button_event_count == 1U) &&
       (controller->event_flags ==
        INTERRUPT_EVENT_NONE))
    {
        printf(
            "[CHECK] Button event processed by main loop\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Button event processing failed\n");
    }
}

/*
 * 测试 UART 接收中断。
 */
static void test_uart_rx_interrupt(
    InterruptController *controller,
    ApplicationEventStats *stats)
{
    int dispatched_count;
    int processed_count;

    dispatched_count = 0;
    processed_count = 0;

    printf("\n");
    printf(
        "----- Test 3: UART RX Interrupt -----\n");

    if(interrupt_enable(
           controller,
           INTERRUPT_SOURCE_UART_RX) == 0)
    {
        printf(
            "[ERROR] Failed to enable UART RX IRQ\n");

        return;
    }

    printf(
        "[IRQ] UART RX interrupt enabled\n");

    /*
     * 模拟 UART 硬件收到字符 A。
     */
    if(interrupt_uart_receive_byte(
           controller,
           (uint8_t)'A') == 0)
    {
        printf(
            "[ERROR] Failed to receive UART byte\n");

        return;
    }

    printf(
        "[UART HARDWARE] Received 'A'\n");

    printf(
        "[STATE] UART pending = %s\n",
        interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_UART_RX) ?
        "YES" :
        "NO");

    dispatched_count =
        interrupt_dispatch(controller);

    printf(
        "[DISPATCH] UART ISR count this time = %d\n",
        dispatched_count);

    if((dispatched_count == 1) &&
       (interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_UART_RX) == 0) &&
       (interrupt_event_is_pending(
            controller,
            INTERRUPT_EVENT_UART_RX) != 0) &&
       (controller->uart_rx_event_data ==
        (uint8_t)'A'))
    {
        printf(
            "[CHECK] UART ISR saved byte correctly\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] UART ISR result mismatch\n");
    }

    processed_count =
        process_pending_events(
            controller,
            stats);

    printf(
        "[MAIN] Processed events = %d\n",
        processed_count);

    if((processed_count == 1) &&
       (stats->uart_event_count == 1U) &&
       (stats->last_uart_byte ==
        (uint8_t)'A'))
    {
        printf(
            "[CHECK] UART event processed by main loop\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "UART main-loop processing failed\n");
    }
}

/*
 * 测试按键和 UART 中断同时 pending。
 *
 * 当前软件调度顺序：
 *
 * 1. BUTTON
 * 2. UART RX
 */
static void test_multiple_interrupts(
    InterruptController *controller,
    ApplicationEventStats *stats)
{
    int dispatched_count;
    int processed_count;

    dispatched_count = 0;
    processed_count = 0;

    printf("\n");
    printf(
        "----- Test 4: Multiple Interrupts -----\n");

    /*
     * 同时产生按键请求和 UART 字节 Z。
     */
    if(interrupt_request(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[ERROR] Failed to request button IRQ\n");

        return;
    }

    if(interrupt_uart_receive_byte(
           controller,
           (uint8_t)'Z') == 0)
    {
        printf(
            "[ERROR] Failed to request UART IRQ\n");

        return;
    }

    printf(
        "[IRQ] Button and UART requests generated\n");

    printf(
        "[STATE] Pending flags = 0x%08lX\n",
        (unsigned long)
        controller->pending_flags);

    /*
     * 两个中断均已使能，
     * 因此本次应执行两个 ISR。
     */
    dispatched_count =
        interrupt_dispatch(controller);

    printf(
        "[DISPATCH] Handled interrupts = %d\n",
        dispatched_count);

    printf(
        "[STATE] Event flags = 0x%08lX\n",
        (unsigned long)
        controller->event_flags);

    if((dispatched_count == 2) &&
       (controller->pending_flags ==
        INTERRUPT_SOURCE_NONE) &&
       ((controller->event_flags &
         INTERRUPT_EVENT_ALL) ==
        INTERRUPT_EVENT_ALL))
    {
        printf(
            "[CHECK] Both ISRs completed\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Multiple interrupt dispatch failed\n");
    }

    /*
     * 主循环一次处理两个软件事件。
     */
    processed_count =
        process_pending_events(
            controller,
            stats);

    printf(
        "[MAIN] Processed events = %d\n",
        processed_count);

    if((processed_count == 2) &&
       (stats->button_event_count == 2U) &&
       (stats->uart_event_count == 2U) &&
       (stats->last_uart_byte ==
        (uint8_t)'Z') &&
       (controller->event_flags ==
        INTERRUPT_EVENT_NONE))
    {
        printf(
            "[CHECK] Both events processed by main loop\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Multiple event processing failed\n");
    }
}

/*
 * 测试禁止中断后：
 *
 * pending 仍然可以产生，
 * 但 ISR 不执行。
 *
 * 本测试最后手动清除该请求。
 */
static void test_interrupt_disable(
    InterruptController *controller)
{
    uint32_t handled_before;
    uint32_t button_isr_before;
    int dispatched_count;

    handled_before =
        controller->handled_irq_count;

    button_isr_before =
        controller->button_isr_count;

    dispatched_count = 0;

    printf("\n");
    printf(
        "----- Test 5: Interrupt Disable -----\n");

    if(interrupt_disable(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[ERROR] Failed to disable button IRQ\n");

        return;
    }

    printf(
        "[IRQ] Button interrupt disabled\n");

    if(interrupt_request(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[ERROR] Failed to request disabled IRQ\n");

        return;
    }

    printf(
        "[IRQ] Button request generated while disabled\n");

    dispatched_count =
        interrupt_dispatch(controller);

    printf(
        "[DISPATCH] Handled while disabled = %d\n",
        dispatched_count);

    if((dispatched_count == 0) &&
       (interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_BUTTON) != 0) &&
       (controller->handled_irq_count ==
        handled_before) &&
       (controller->button_isr_count ==
        button_isr_before))
    {
        printf(
            "[CHECK] Disabled interrupt did not enter ISR\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Disabled interrupt was handled unexpectedly\n");
    }

    /*
     * 本次决定丢弃这个未处理请求，
     * 因此手动清除 pending。
     */
    if(interrupt_clear_pending(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[ERROR] Failed to clear disabled pending\n");

        return;
    }

    printf(
        "[IRQ] Disabled button pending cleared manually\n");

    /*
     * 重新使能按键中断。
     */
    if(interrupt_enable(
           controller,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[ERROR] Failed to re-enable button IRQ\n");

        return;
    }

    dispatched_count =
        interrupt_dispatch(controller);

    if((dispatched_count == 0) &&
       (interrupt_is_pending(
            controller,
            INTERRUPT_SOURCE_BUTTON) == 0))
    {
        printf(
            "[CHECK] Cleared IRQ was not processed later\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Cleared IRQ unexpectedly entered ISR\n");
    }
}

/*
 * 测试非法参数以及 ISR 防御性检查。
 *
 * 非法操作不应改变任何中断计数。
 */
static void test_invalid_operations(
    InterruptController *controller)
{
    uint32_t request_before;
    uint32_t handled_before;
    uint32_t button_before;
    uint32_t uart_before;

    request_before =
        controller->irq_request_count;

    handled_before =
        controller->handled_irq_count;

    button_before =
        controller->button_isr_count;

    uart_before =
        controller->uart_rx_isr_count;

    printf("\n");
    printf(
        "----- Test 6: Invalid Operations -----\n");

    if(interrupt_init(NULL) == 0)
    {
        printf(
            "[EXPECTED ERROR] NULL init rejected\n");
    }

    if(interrupt_enable(
           NULL,
           INTERRUPT_SOURCE_BUTTON) == 0)
    {
        printf(
            "[EXPECTED ERROR] NULL enable rejected\n");
    }

    if(interrupt_enable(
           controller,
           INTERRUPT_SOURCE_NONE) == 0)
    {
        printf(
            "[EXPECTED ERROR] NONE source rejected\n");
    }

    /*
     * 当前接口一次只允许操作一个中断源，
     * 因此 ALL 位组合应被拒绝。
     */
    if(interrupt_enable(
           controller,
           INTERRUPT_SOURCE_ALL) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Combined source rejected\n");
    }

    if(interrupt_request(
           controller,
           0x00000080UL) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Invalid IRQ source rejected\n");
    }

    if(interrupt_uart_receive_byte(
           NULL,
           (uint8_t)'X') == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "NULL UART controller rejected\n");
    }

    if(interrupt_clear_event(
           controller,
           INTERRUPT_EVENT_ALL) == 0)
    {
        printf(
            "[EXPECTED ERROR] "
            "Combined event rejected\n");
    }

    /*
     * 当前没有 pending，
     * 直接调用 ISR 不应增加计数或产生事件。
     */
    interrupt_button_isr(controller);
    interrupt_uart_rx_isr(controller);

    /*
     * NULL ISR 调用也不应导致程序崩溃。
     */
    interrupt_button_isr(NULL);
    interrupt_uart_rx_isr(NULL);

    if((controller->irq_request_count ==
        request_before) &&
       (controller->handled_irq_count ==
        handled_before) &&
       (controller->button_isr_count ==
        button_before) &&
       (controller->uart_rx_isr_count ==
        uart_before) &&
       (controller->pending_flags ==
        INTERRUPT_SOURCE_NONE) &&
       (controller->event_flags ==
        INTERRUPT_EVENT_NONE))
    {
        printf(
            "[CHECK] Invalid operations changed no state\n");
    }
    else
    {
        printf(
            "[CHECK ERROR] "
            "Invalid operation changed interrupt state\n");
    }
}

int main(void)
{
    InterruptController controller;
    ApplicationEventStats app_stats;

    /*
     * 初始化应用层统计。
     */
    app_stats.button_event_count = 0U;
    app_stats.uart_event_count = 0U;
    app_stats.last_uart_byte = 0U;

    printf("\n");
    printf(
        "===== Day27 Interrupt and Event Test =====\n");

    if(interrupt_init(
           &controller) == 0)
    {
        printf(
            "[FATAL] Interrupt initialization failed\n");

        return 1;
    }

    test_initial_state(
        &controller,
        &app_stats);

    test_pending_while_disabled(
        &controller,
        &app_stats);

    test_uart_rx_interrupt(
        &controller,
        &app_stats);

    test_multiple_interrupts(
        &controller,
        &app_stats);

    test_interrupt_disable(
        &controller);

    test_invalid_operations(
        &controller);

    printf("\n");
    printf(
        "----- Final Interrupt Status -----\n");

    printf(
        "[FINAL] Enabled flags       = 0x%08lX\n",
        (unsigned long)
        controller.enabled_flags);

    printf(
        "[FINAL] Pending flags       = 0x%08lX\n",
        (unsigned long)
        controller.pending_flags);

    printf(
        "[FINAL] Event flags         = 0x%08lX\n",
        (unsigned long)
        controller.event_flags);

    printf(
        "[FINAL] IRQ requests        = %lu\n",
        (unsigned long)
        controller.irq_request_count);

    printf(
        "[FINAL] Handled IRQs        = %lu\n",
        (unsigned long)
        controller.handled_irq_count);

    printf(
        "[FINAL] Button ISR count    = %lu\n",
        (unsigned long)
        controller.button_isr_count);

    printf(
        "[FINAL] UART RX ISR count   = %lu\n",
        (unsigned long)
        controller.uart_rx_isr_count);

    printf(
        "[FINAL] Button events       = %lu\n",
        (unsigned long)
        app_stats.button_event_count);

    printf(
        "[FINAL] UART events         = %lu\n",
        (unsigned long)
        app_stats.uart_event_count);

    printf(
        "[FINAL] Last UART byte      = '%c' (0x%02X)\n",
        (int)app_stats.last_uart_byte,
        (unsigned int)
        app_stats.last_uart_byte);

    /*
     * 中断请求共 5 次：
     *
     * 1. 中断未使能时的 BUTTON 请求
     * 2. UART 字节 A
     * 3. 多中断测试中的 BUTTON 请求
     * 4. 多中断测试中的 UART 字节 Z
     * 5. BUTTON 被禁止时的请求
     *
     * 实际处理 4 次：
     *
     * BUTTON ISR：2 次
     * UART ISR：2 次
     *
     * 第 5 次请求因为中断被禁止，
     * 最后由主循环手动清除 pending，
     * 没有进入 ISR。
     */
    if((controller.enabled_flags ==
        INTERRUPT_SOURCE_ALL) &&
       (controller.pending_flags ==
        INTERRUPT_SOURCE_NONE) &&
       (controller.event_flags ==
        INTERRUPT_EVENT_NONE) &&
       (controller.irq_request_count == 5U) &&
       (controller.handled_irq_count == 4U) &&
       (controller.button_isr_count == 2U) &&
       (controller.uart_rx_isr_count == 2U) &&
       (app_stats.button_event_count == 2U) &&
       (app_stats.uart_event_count == 2U) &&
       (app_stats.last_uart_byte ==
        (uint8_t)'Z') &&
       (controller.uart_rx_event_data ==
        (uint8_t)'Z'))
    {
        printf(
            "[FINAL CHECK] "
            "All interrupt tests passed\n");
    }
    else
    {
        printf(
            "[FINAL CHECK ERROR] "
            "Interrupt result mismatch\n");
    }

    printf("\n");
    printf(
        "===== Interrupt Test Finished =====\n");

    return 0;
}