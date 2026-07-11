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

    button->event = BUTTON_EVENT_NONE;

    DEBUG_PRINT("button init, state = %d, tick = %u ms\n",
                initial_state,
                current_tick);
}

void button_update(Button *button,
                   ButtonState raw_state,
                   unsigned int current_tick)
{
    unsigned int stable_time_ms;

    button->raw_state = raw_state;

    if(button->raw_state != button->last_raw_state)
    {
        button->last_raw_state = button->raw_state;
        button->last_change_tick = current_tick;

        DEBUG_PRINT("button raw changed to %d at %u ms\n",
                    button->raw_state,
                    current_tick);
    }

    stable_time_ms =
        current_tick - button->last_change_tick;

    if(stable_time_ms >= BUTTON_DEBOUNCE_MS)
    {
        if(button->stable_state != button->raw_state)
        {
            button->stable_state = button->raw_state;

            if(button->stable_state ==
               BUTTON_STATE_PRESSED)
            {
                button->event = BUTTON_EVENT_PRESSED;

                DEBUG_PRINT(
                    "button stable pressed at %u ms\n",
                    current_tick);
            }
            else
            {
                button->event = BUTTON_EVENT_RELEASED;

                DEBUG_PRINT(
                    "button stable released at %u ms\n",
                    current_tick);
            }
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