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

void uart_process_rx_buffer(SystemStatus *sys,
                            RingBuffer *rx_buffer)
{
    static char command_buffer[UART_COMMAND_MAX_LENGTH];
    static unsigned int command_length = 0;
    static int command_overflow = 0;

    char byte;

    while(ring_buffer_is_empty(rx_buffer) == 0)
    {
        if(ring_buffer_pop(rx_buffer, &byte) == 0)
        {
            break;
        }

        if(byte == '\r')
        {
            continue;
        }

        if(byte == '\n')
        {
            if(command_overflow == 1)
            {
                printf("[UART] Command discarded after overflow\n");
            }
            else if(command_length > 0)
            {
                command_buffer[command_length] = '\0';

                uart_process_command(sys, command_buffer);
            }

            command_length = 0;
            command_overflow = 0;

            continue;
        }

        if(command_overflow == 1)
        {
            continue;
        }

        if(command_length < UART_COMMAND_MAX_LENGTH - 1)
        {
            command_buffer[command_length] = byte;
            command_length++;
        }
        else
        {
            command_overflow = 1;

            printf("[UART] Command too long\n");
        }
    }
}