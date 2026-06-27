#include "system_type.h"
#include "system.h"
#include "sensor.h"
#include "fault.h"

int main(void)
{
    SystemStatus sys;

    system_init(&sys);
    sensor_update(&sys);
    fault_check(&sys);
    system_print(&sys);

    return 0;
}