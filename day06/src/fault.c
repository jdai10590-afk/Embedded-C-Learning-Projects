#include "fault.h"
#include "fault_code.h"

void fault_check(SystemStatus *sys)
{
    sys->fault_code = FAULT_NONE;
    if(sys->voltage <  10.0f)
    {
        sys->fault_code |=FAULT_LOW_VOLTAGE;
    }
    if(sys->current > 2.0f)
    {
        sys->fault_code |= FAULT_OVER_CURRENT;
    }
    if(sys->temperature >60.0f)
    {
        sys->fault_code |= FAULT_OVER_TEMP;
    }
}