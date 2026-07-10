#include <stdio.h>

#include "system_type.h"
#include "system.h"
#include "gpio.h"
#include "uart.h"
#include "ring_buffer.h"

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

static void ring_buffer_boundary_test(void)
{
    RingBuffer test_buffer;
    char byte;
    unsigned int index;
    int result;

    ring_buffer_init(&test_buffer);

    printf("\n===== Ring Buffer Boundary Test =====\n");

    for(index = 0; index < RING_BUFFER_SIZE; index++)
    {
        result = ring_buffer_push(
            &test_buffer,
            (char)('A' + index % 26)
        );

        if(result == 0)
        {
            printf("[TEST] Unexpected push failure at index %u\n",
                   index);
            return;
        }
    }

    printf("[TEST] After filling:\n");
    printf("head  = %u\n", test_buffer.head);
    printf("tail  = %u\n", test_buffer.tail);
    printf("count = %u\n", test_buffer.count);
    printf("full  = %d\n",
           ring_buffer_is_full(&test_buffer));

    result = ring_buffer_push(&test_buffer, 'Z');

    printf("[TEST] Extra push result = %d\n", result);

    for(index = 0; index < RING_BUFFER_SIZE; index++)
    {
        result = ring_buffer_pop(&test_buffer, &byte);

        if(result == 0)
        {
            printf("[TEST] Unexpected pop failure at index %u\n",
                   index);
            return;
        }
    }

    printf("[TEST] After draining:\n");
    printf("head  = %u\n", test_buffer.head);
    printf("tail  = %u\n", test_buffer.tail);
    printf("count = %u\n", test_buffer.count);
    printf("empty = %d\n",
           ring_buffer_is_empty(&test_buffer));

    result = ring_buffer_pop(&test_buffer, &byte);

    printf("[TEST] Extra pop result = %d\n", result);
}

int main(void)
{
    SystemStatus sys;
    RingBuffer uart_rx_buffer;

    system_init(&sys);
    gpio_init(&sys);
    ring_buffer_init(&uart_rx_buffer);

    printf("\n===== UART Ring Buffer Test =====\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "STATUS\r\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "LED ON\r\n");
    uart_simulate_input(&sys, &uart_rx_buffer, "STATUS\r\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "LED OFF\r\n");
    uart_simulate_input(&sys, &uart_rx_buffer, "STATUS\r\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "LED TOGGLE\r\n");
    uart_simulate_input(&sys, &uart_rx_buffer, "STATUS\r\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "RESET\r\n");
    uart_simulate_input(&sys, &uart_rx_buffer, "STATUS\r\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "HELLO\r\n");

    printf("\n===== Fragmented UART Command Test =====\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "LED ");
    uart_simulate_input(&sys, &uart_rx_buffer, "ON\r\n");

    uart_simulate_input(&sys, &uart_rx_buffer, "STATUS\r\n");
    
    printf("\n===== UART Command Overflow Test =====\n");

    uart_simulate_input(&sys,
                        &uart_rx_buffer,
                        "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890\r\n");

    uart_simulate_input(&sys,
                        &uart_rx_buffer,
                        "STATUS\r\n");
    
    ring_buffer_boundary_test();
    return 0;
}