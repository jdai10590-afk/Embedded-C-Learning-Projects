#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "system.h"
#include "gpio.h"
#include "config.h"
#include "debug.h"

void uart_process_command(SystemStatus *sys, const char *command)
{
    printf("\n[UART] Received command: %s\n", command);

    if(strcmp(command, "STATUS") == 0)
    {
        printf("[UART] System status:\n");
        system_print(sys);
    }
    else if(strcmp(command, "LED ON") == 0)
    {
        sys->led_state = LED_STATE_ON;
        gpio_led_on(sys);

        printf("[UART] LED turned ON\n");
    }
    else if(strcmp(command, "LED OFF") == 0)
    {
        sys->led_state = LED_STATE_OFF;
        gpio_led_off(sys);

        printf("[UART] LED turned OFF\n");
    }
    else if(strcmp(command, "LED TOGGLE") == 0)
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

        printf("[UART] LED toggled\n");
    }
    else if(strcmp(command, "RESET") == 0)
    {
        system_init(sys);
        gpio_init(sys);

        printf("[UART] System reset completed\n");
    }
    else
    {
        printf("[UART] Unknown command: %s\n", command);
    }

    DEBUG_PRINT("uart command process done\n");
}