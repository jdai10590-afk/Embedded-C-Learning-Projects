#include <stdio.h>
#include "system.h"
#include "config.h"
#include "fault_code.h"
#include "debug.h"

void system_init(SystemStatus *sys)
{
    sys->led_state = SYS_INIT_LED_STATE;
    sys->gpio_output_reg = 0;

    sys->mode = SYS_MODE_INIT;
    sys->last_mode = SYS_MODE_INIT;

    sys->voltage = SYS_INIT_VOLTAGE;
    sys->current = SYS_INIT_CURRENT;
    sys->temperature = SYS_INIT_TEMPERATURE;

    sys->fault_code = FAULT_NONE;

    sys->cycle_count = 0;
    sys->fault_cycle_count = 0;
    sys->fault_enter_count = 0;

    DEBUG_PRINT("system init done\n");
}

void system_print(const SystemStatus *sys)
{
    printf("LED state: %d\n", sys->led_state);
    printf("GPIO output reg: 0x%08X\n",sys->gpio_output_reg);
    printf("LED pin level: %u\n",
            (sys->gpio_output_reg & BIT(LED_GPIO_PIN)) ? 1U : 0U);

    printf("System mode: ");
    switch(sys->mode)
    {
        case SYS_MODE_INIT:
            printf("INIT\n");
            break;

        case SYS_MODE_RUN:
            printf("RUN\n");
            break;

        case SYS_MODE_FAULT:
            printf("FAULT\n");
            break;

        default:
            printf("UNKNOWN\n");
            break;
    }

    printf("Voltage: %.2f V\n", sys->voltage);
    printf("Current: %.2f A\n", sys->current);
    printf("temperature: %.2f C\n", sys->temperature);
    printf("Fault code: 0x%04X\n", sys->fault_code);

    if(sys->fault_code == FAULT_NONE)
    {
        printf("System normal\n");
    }
    else
    {
        if(sys->fault_code & FAULT_LOW_VOLTAGE)
        {
            printf("Fault: Low voltage\n");
        }

        if(sys->fault_code & FAULT_OVER_CURRENT)
        {
            printf("Fault: Over current\n");
        }

        if(sys->fault_code & FAULT_OVER_TEMP)
        {
            printf("Fault: Over temperature\n");
        }
    }

    printf("Cycle count: %u\n", sys->cycle_count);
    printf("Fault cycle count: %u\n", sys->fault_cycle_count);
    printf("Fault enter count: %u\n", sys->fault_enter_count);
}