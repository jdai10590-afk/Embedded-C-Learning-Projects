#include "state_machine.h"
#include "fault_code.h"
#include "debug.h"

void state_machine_update(SystemStatus *sys)
{
    if(sys->fault_code == FAULT_NONE)
    {
        sys->mode = SYS_MODE_RUN;
    }
    else
    {
        sys->mode = SYS_MODE_FAULT;
    }
    DEBUG_PRINT("state machine update done\n");
}