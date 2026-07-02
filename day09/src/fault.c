#include "fault.h"
#include "fault_code.h"
#include "debug.h"
#include "config.h"

void fault_check(SystemStatus *sys)
{
    sys->fault_code = FAULT_NONE;
    if(sys->voltage <  FAULT_LOW_VOLTAGE_TH)
    {
        sys->fault_code |=FAULT_LOW_VOLTAGE;
    }
    if(sys->current > FAULT_OVER_CURRENT_TH)
    {
        sys->fault_code |= FAULT_OVER_CURRENT;
    }
    if(sys->temperature >FAULT_OVER_TEMP_TH)
    {
        sys->fault_code |= FAULT_OVER_TEMP;
    }
    if(sys->fault_code == FAULT_NONE)
    {
        sys->mode = SYS_MODE_RUN;
    }
    else
    {
        sys->mode = SYS_MODE_FAULT;
    }
    DEBUG_PRINT("fault check done\n");
}