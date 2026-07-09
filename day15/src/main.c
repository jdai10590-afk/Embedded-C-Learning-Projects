#include <stdio.h>
#include "system_type.h"
#include "system.h"
#include "sensor.h"
#include "fault.h"
#include "state_machine.h"
#include "control.h"
#include "event.h"
#include "gpio.h"

int main(void)
{
    SystemStatus sys;

    system_init(&sys);
    gpio_init(&sys);

    for(int cycle = 0; cycle < 5; cycle++)
    {
        printf("\n===== Cycle %d =====\n", cycle);

        sensor_update(&sys, cycle);
        fault_check(&sys);
        state_machine_update(&sys);
        control_update(&sys);
        event_update(&sys);
        system_print(&sys);
    }

    return 0;
}
