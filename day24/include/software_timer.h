#ifndef SOFTWARE_TIMER_H
#define SOFTWARE_TIMER_H

typedef struct
{
    unsigned int start_tick;
    unsigned int period_ms;
    int enabled;
} SoftwareTimer;

void software_timer_init(SoftwareTimer *timer,
                         unsigned int period_ms);

void software_timer_start(SoftwareTimer *timer,
                          unsigned int current_tick);

void software_timer_stop(SoftwareTimer *timer);

int software_timer_is_expired(SoftwareTimer *timer,
                              unsigned int current_tick);

#endif