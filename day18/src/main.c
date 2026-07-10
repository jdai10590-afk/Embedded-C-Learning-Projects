#include <stdio.h>

#include "config.h"
#include "gpio.h"
#include "software_timer.h"
#include "system.h"
#include "system_type.h"

static void sensor_task(unsigned int current_tick)
{
    printf("[TASK][SENSOR] Executed at %u ms\n",
           current_tick);
}

static void led_task(SystemStatus *sys,
                     unsigned int current_tick)
{
    gpio_led_toggle(sys);

    if(gpio_led_read(sys) == 1)
    {
        sys->led_state = LED_STATE_ON;
    }
    else
    {
        sys->led_state = LED_STATE_OFF;
    }

    printf("[TASK][LED] Executed at %u ms, level = %d\n",
           current_tick,
           gpio_led_read(sys));
}

static void status_task(SystemStatus *sys,
                        unsigned int current_tick)
{
    printf("[TASK][STATUS] Executed at %u ms\n",
           current_tick);

    system_print(sys);
}

int main(void)
{
    SystemStatus sys;

    SoftwareTimer sensor_timer;
    SoftwareTimer led_timer;
    SoftwareTimer status_timer;

    unsigned int system_tick_ms;

    system_init(&sys);
    gpio_init(&sys);

    software_timer_init(&sensor_timer, 200);
    software_timer_init(&led_timer, 500);
    software_timer_init(&status_timer, 1000);

    software_timer_start(&sensor_timer, 0);
    software_timer_start(&led_timer, 0);
    software_timer_start(&status_timer, 0);

    printf("\n===== Software Timer Test =====\n");

    for(system_tick_ms = 0;
        system_tick_ms <= 3000;
        system_tick_ms += 100)
    {
        printf("\n[TICK] %u ms\n", system_tick_ms);

        if(system_tick_ms == 1600)
        {
            software_timer_stop(&led_timer);

            printf("[CONTROL] LED timer stopped at %u ms\n",
                system_tick_ms);
        }

        if(system_tick_ms == 2300)
        {
            software_timer_start(&led_timer, system_tick_ms);

            printf("[CONTROL] LED timer restarted at %u ms\n",
                system_tick_ms);
        }

        if(software_timer_is_expired(&sensor_timer,
                                     system_tick_ms))
        {
            sensor_task(system_tick_ms);
        }

        if(software_timer_is_expired(&led_timer,
                                     system_tick_ms))
        {
            led_task(&sys, system_tick_ms);
        }

        if(software_timer_is_expired(&status_timer,
                                     system_tick_ms))
        {
            status_task(&sys, system_tick_ms);
        }
    }

    software_timer_stop(&sensor_timer);
    software_timer_stop(&led_timer);
    software_timer_stop(&status_timer);

    printf("\n===== Software Timer Test Finished =====\n");

    return 0;
}