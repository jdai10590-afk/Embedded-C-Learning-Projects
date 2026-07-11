#include <stdio.h>

#include "button.h"
#include "config.h"
#include "gpio.h"
#include "system.h"
#include "system_type.h"

/*
 * 根据系统 Tick 模拟机械按键原始信号。
 *
 * 0   ~  90 ms：稳定释放
 * 100 ~ 140 ms：按下抖动
 * 140 ~ 490 ms：稳定按下
 * 500 ~ 540 ms：释放抖动
 * 540 ms 以后：稳定释放
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

    if(current_tick < 500U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 500U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 510U)
    {
        return BUTTON_STATE_PRESSED;
    }

    if(current_tick == 520U)
    {
        return BUTTON_STATE_RELEASED;
    }

    if(current_tick == 530U)
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
    if(event == BUTTON_EVENT_PRESSED)
    {
        printf("[EVENT] Button pressed at %u ms\n",
               current_tick);

        gpio_led_toggle(sys);
        led_sync_state(sys);

        printf("[ACTION] LED toggled, level = %d\n",
               gpio_led_read(sys));
    }
    else if(event == BUTTON_EVENT_RELEASED)
    {
        printf("[EVENT] Button released at %u ms\n",
               current_tick);
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

    printf("\n===== Button Debounce Test =====\n");

    for(system_tick_ms = 0U;
        system_tick_ms <= 700U;
        system_tick_ms += 10U)
    {
        raw_state =
            simulate_button_raw_state(system_tick_ms);

        button_update(&key1,
                      raw_state,
                      system_tick_ms);

        event = button_get_event(&key1);

        printf("[SAMPLE] tick = %3u ms, raw = %d, stable = %d\n",
               system_tick_ms,
               raw_state,
               button_get_state(&key1));

        handle_button_event(&sys,
                            event,
                            system_tick_ms);
    }

    printf("\n===== Final System State =====\n");
    system_print(&sys);

    printf("\n===== Button Debounce Test Finished =====\n");

    return 0;
}