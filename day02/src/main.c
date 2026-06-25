#include <stdio.h>
#include"system_type.h"
#include"fault_code.h"

void system_init(SystemStatus *sys)
{
    sys->led_state = 0;
    sys->voltage = 12.5f;
    sys->current = 1.2f;
    sys->temperature = 35.6f;
    sys->fault_code = FAULT_NONE;
}

void sensor_update(SystemStatus *sys)
{
    sys->voltage = 9.5f;
    sys->current = 2.5f;
    sys->temperature = 72.0f;
}

void fault_check(SystemStatus *sys)
{
    sys->fault_code = FAULT_NONE;
    if(sys->voltage < 10.0f)
    {
        sys->fault_code |= FAULT_LOW_VOLTAGE;
    }

    if(sys->current >2.0f)
    {
        sys->fault_code |= FAULT_OVER_CURRENT;
    }
    
    if(sys->temperature > 60.0f)
    {
        sys->fault_code |= FAULT_OVER_TEMP;
    }
}
void system_print(const SystemStatus *sys)
 {
    printf("LED state: %d\n",sys->led_state);
    printf("Voltage: %.2f V\n",sys->voltage);
    printf("current: %.2f A\n",sys->current);
    printf("Temperature: %.2f C\n",sys->temperature);
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

int main(void)
{
    SystemStatus sys;

    system_init(&sys);
    sensor_update(&sys);
    fault_check(&sys);
    system_print(&sys);

    return 0;
}