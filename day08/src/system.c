#include <stdio.h>
#include "system.h"
#include "fault_code.h"
#include "debug.h"
#include "config.h"

void system_init(SystemStatus *sys)
{
    sys->led_state = SYS_INIT_LED_STATE;
    sys->voltage = SYS_INIT_VOLTAGE;
    sys->current = SYS_INIT_CURRENT;
    sys->temperature = SYS_INIT_TEMPERATURE;
    sys->fault_code = FAULT_NONE;

    DEBUG_PRINT("system init done\n");
}

void system_print(const SystemStatus *sys)
{
    printf("LED state: %d\n",sys->led_state);
    printf("Voltage: %.2f V\n",sys->voltage);
    printf("Current: %.2f A\n",sys->current);
    printf("temperature: %.2f C\n",sys->temperature);
    printf("Fault code: 0x%04X\n",sys->fault_code);

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

    if(sys->fault_code == FAULT_NONE)
    {
        printf("System normal\n");
    }
}