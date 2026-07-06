#include "state_machine.h"
#include "fault_code.h"
#include "debug.h"

void state_machine_update(SystemStatus *sys)
{
    switch(sys->mode)
    {
        case SYS_MODE_INIT:
            if(sys->fault_code == FAULT_NONE)
            {
                sys->mode = SYS_MODE_RUN;
                DEBUG_PRINT("state: INIT -> RUN\n");
            }
            else
            {
                sys->mode = SYS_MODE_FAULT;
                DEBUG_PRINT("state: INIT -> FAULT\n");
            }
            break;

        case SYS_MODE_RUN:
            if(sys->fault_code != FAULT_NONE)
            {
                sys->mode = SYS_MODE_FAULT;
                DEBUG_PRINT("state: RUN -> FAULT\n");
            }
            else
            {
                DEBUG_PRINT("state: RUN keep\n");
            }
            break;

        case SYS_MODE_FAULT:
            if(sys->fault_code == FAULT_NONE)
            {
                sys->mode = SYS_MODE_RUN;
                DEBUG_PRINT("state: FAULT -> RUN\n");
            }
            else
            {
                DEBUG_PRINT("state: FAULT keep\n");
            }
            break;

        default:
            sys->mode = SYS_MODE_FAULT;
            DEBUG_PRINT("state: UNKNOWN -> FAULT\n");
            break;
    }

    DEBUG_PRINT("state machine update done\n");
}