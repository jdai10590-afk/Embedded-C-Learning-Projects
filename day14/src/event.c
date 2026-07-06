#include "event.h"
#include "fault_code.h"
#include "debug.h"

void event_update(SystemStatus *sys)
{
    sys->cycle_count++;

    if(sys->mode == SYS_MODE_FAULT)
    {
        sys->fault_cycle_count++;
    }

    if((sys->last_mode != SYS_MODE_FAULT) && (sys->mode == SYS_MODE_FAULT))
    {
        sys->fault_enter_count++;
        DEBUG_PRINT("event: enter FAULT count +1\n");
    }

    sys->last_mode = sys->mode;

    DEBUG_PRINT("event update done\n");
}