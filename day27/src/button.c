#include "button.h"
#include "debug.h"

void button_init(Button *button,
                 ButtonState initial_state,
                 unsigned int current_tick)
{
    button->raw_state = initial_state;
    button->last_raw_state = initial_state;
    button->stable_state = initial_state;

    button->last_change_tick = current_tick;
    button->press_tick = current_tick;

    button->long_press_triggered = 0;

    button->event = BUTTON_EVENT_NONE;

    DEBUG_PRINT(
        "button init, state = %d, tick = %u ms\n",
        initial_state,
        current_tick);
}

void button_update(Button *button,
                   ButtonState raw_state,
                   unsigned int current_tick)
{
    unsigned int stable_time_ms;
    unsigned int held_time_ms;

    button->raw_state = raw_state;

    /*
     * 原始输入发生变化时，重新开始消抖计时。
     */
    if(button->raw_state != button->last_raw_state)
    {
        button->last_raw_state = button->raw_state;
        button->last_change_tick = current_tick;

        DEBUG_PRINT(
            "button raw changed to %d at %u ms\n",
            button->raw_state,
            current_tick);
    }

    /*
     * 计算当前原始状态已经连续稳定了多长时间。
     */
    stable_time_ms =
        current_tick - button->last_change_tick;

    /*
     * 原始状态稳定达到消抖时间后，
     * 再判断可信的稳定状态是否需要更新。
     */
    if(stable_time_ms >= BUTTON_DEBOUNCE_MS)
    {
        if(button->stable_state != button->raw_state)
        {
            button->stable_state = button->raw_state;

            /*
             * 确认稳定按下。
             */
            if(button->stable_state ==
               BUTTON_STATE_PRESSED)
            {
                button->press_tick = current_tick;
                button->long_press_triggered = 0;

                button->event = BUTTON_EVENT_PRESSED;

                DEBUG_PRINT(
                    "button stable pressed at %u ms\n",
                    current_tick);
            }
            /*
             * 确认稳定释放。
             */
            else
            {
                held_time_ms =
                    current_tick - button->press_tick;

                /*
                 * 如果本次按住期间没有触发长按，
                 * 那么释放时认定为短按。
                 */
                if(button->long_press_triggered == 0)
                {
                    button->event =
                        BUTTON_EVENT_SHORT_PRESS;

                    DEBUG_PRINT(
                        "button short press, held = %u ms\n",
                        held_time_ms);
                }
                /*
                 * 如果已经触发过长按，
                 * 释放时只产生释放事件。
                 */
                else
                {
                    button->event =
                        BUTTON_EVENT_RELEASED;

                    DEBUG_PRINT(
                        "button released after long press "
                        "at %u ms\n",
                        current_tick);
                }

                /*
                 * 本次按键操作结束，
                 * 为下一次按键重新准备。
                 */
                button->long_press_triggered = 0;
            }
        }
    }

    /*
     * 按键保持稳定按下时，持续检查按住时间。
     */
    if(button->stable_state == BUTTON_STATE_PRESSED &&
       button->long_press_triggered == 0)
    {
        held_time_ms =
            current_tick - button->press_tick;

        if(held_time_ms >= BUTTON_LONG_PRESS_MS)
        {
            button->long_press_triggered = 1;
            button->event = BUTTON_EVENT_LONG_PRESS;

            DEBUG_PRINT(
                "button long press at %u ms, held = %u ms\n",
                current_tick,
                held_time_ms);
        }
    }
}

ButtonEvent button_get_event(Button *button)
{
    ButtonEvent event;

    event = button->event;

    button->event = BUTTON_EVENT_NONE;

    return event;
}

ButtonState button_get_state(const Button *button)
{
    return button->stable_state;
}