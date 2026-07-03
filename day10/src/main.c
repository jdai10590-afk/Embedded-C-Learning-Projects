#include "system_type.h"
#include "system.h"
#include "sensor.h"
#include "fault.h"
#include "state_machine.h"

int main(void)
{
    SystemStatus sys;

    system_init(&sys);
    sensor_update(&sys);
    fault_check(&sys);
    state_machine_update(&sys);
    system_print(&sys);

    return 0;
}