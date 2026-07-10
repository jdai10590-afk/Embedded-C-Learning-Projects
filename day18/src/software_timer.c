#include "software_timer.h"
#include "debug.h"

void software_timer_init(SoftwareTimer *timer,
                         unsigned int period_ms)
{
    timer->start_tick = 0;
    timer->period_ms = period_ms;
    timer->enabled = 0;

    DEBUG_PRINT("software timer init, period = %u ms\n",
                period_ms);
}

void software_timer_start(SoftwareTimer *timer,
                          unsigned int current_tick)
{
    timer->start_tick = current_tick;
    timer->enabled = 1;

    DEBUG_PRINT("software timer start at %u ms\n",
                current_tick);
}

void software_timer_stop(SoftwareTimer *timer)
{
    timer->enabled = 0;

    DEBUG_PRINT("software timer stopped\n");
}

int software_timer_is_expired(SoftwareTimer *timer,
                              unsigned int current_tick)
{
    unsigned int elapsed_ms;

    if(timer->enabled == 0)
    {
        return 0;
    }

    if(timer->period_ms == 0)
    {
        DEBUG_PRINT("software timer period is zero\n");
        return 0;
    }

    elapsed_ms = current_tick - timer->start_tick;

    if(elapsed_ms >= timer->period_ms)
    {
        timer->start_tick += timer->period_ms;

        return 1;
    }

    return 0;
}