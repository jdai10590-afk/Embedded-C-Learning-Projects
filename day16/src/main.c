#include <stdio.h>

#include "system_type.h"
#include "system.h"
#include "gpio.h"
#include "uart.h"

int main(void)
{
    SystemStatus sys;

    system_init(&sys);
    gpio_init(&sys);

    printf("\n===== UART Command Test =====\n");

    uart_process_command(&sys, "STATUS");

    uart_process_command(&sys, "LED ON");
    uart_process_command(&sys, "STATUS");

    uart_process_command(&sys, "LED OFF");
    uart_process_command(&sys, "STATUS");

    uart_process_command(&sys, "LED TOGGLE");
    uart_process_command(&sys, "STATUS");

    uart_process_command(&sys, "RESET");
    uart_process_command(&sys, "STATUS");

    uart_process_command(&sys, "HELLO");

    return 0;
}