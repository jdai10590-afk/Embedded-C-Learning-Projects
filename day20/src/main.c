#include <stdio.h>

#include "button.h"
#include "config.h"
#include "gpio.h"
#include "system.h"
#include "system_type.h"

/*
 * 模拟两次机械按键操作：
 *
 * 第一次：短按
 * 100～140 ms：按下抖动
 * 140～400 ms：稳定按下
 * 400～440 ms：释放抖动
 * 440 ms 以后：稳定释放
 *
 * 第二次：长按
 * 700～740 ms：按下抖动
 * 740～2000 ms：稳定按下
 * 2000～2040 ms：释放抖动
 * 2040 ms 以后：稳定释放
 */
static ButtonState simulate_button_raw_state(
    unsigned int current_tick)
{
    if(current_tick < 100U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 100U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 110U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 120U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 130U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick < 400U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 400U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 410U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 420U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 430U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick < 700U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 700U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 710U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 720U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 730U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick < 2000U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 2000U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 2010U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 2020U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 2030U)
    {
        return BUTTON_STATE_PRESSED;
    }

    return BUTTON_STATE_RELEASED;
}

static void led_sync_state(SystemStatus *sys)
{
    if(gpio_led_read(sys) == 1)
    {
        sys->led_state = LED_STATE_ON;
    }
    else
    {
        sys->led_state = LED_STATE_OFF;
    }
}

static void handle_button_event(SystemStatus *sys,
                                ButtonEvent event,
                                unsigned int current_tick)
{
    switch(event)
    {
        case BUTTON_EVENT_PRESSED:
            printf("[EVENT] Button pressed at %u ms\n",
                   current_tick);
            break;

        case BUTTON_EVENT_SHORT_PRESS:
            printf("[EVENT] Short press at %u ms\n",
                   current_tick);

            gpio_led_toggle(sys);
            led_sync_state(sys);

            printf("[ACTION] Short press toggled LED, "
                   "level = %d\n",
                   gpio_led_read(sys));
            break;

        case BUTTON_EVENT_LONG_PRESS:
            printf("[EVENT] Long press at %u ms\n",
                   current_tick);

            printf("[ACTION] Long-press action executed\n");
            break;

        case BUTTON_EVENT_RELEASED:
            printf("[EVENT] Button released after long press "
                   "at %u ms\n",
                   current_tick);
            break;

        case BUTTON_EVENT_NONE:
        default:
            break;
    }
}

int main(void)
{
    SystemStatus sys;
    Button key1;

    ButtonState raw_state;
    ButtonEvent event;

    unsigned int system_tick_ms;

    system_init(&sys);
    gpio_init(&sys);

    button_init(&key1,
                BUTTON_STATE_RELEASED,
                0U);

    printf("\n===== Short and Long Press Test =====\n");

    for(system_tick_ms = 0U;
        system_tick_ms <= 2200U;
        system_tick_ms += 10U)
    {
        raw_state =
            simulate_button_raw_state(system_tick_ms);

        button_update(&key1,
                      raw_state,
                      system_tick_ms);

        event = button_get_event(&key1);

        printf("[SAMPLE] tick = %4u ms, raw = %d, "
               "stable = %d\n",
               system_tick_ms,
               raw_state,
               button_get_state(&key1));

        handle_button_event(&sys,
                            event,
                            system_tick_ms);
    }

    printf("\n===== Final System State =====\n");
    system_print(&sys);

    printf("\n===== Test Finished =====\n");

    return 0;
}
