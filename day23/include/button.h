#ifndef BUTTON_H
#define BUTTON_H

#define BUTTON_DEBOUNCE_MS   30U
#define BUTTON_LONG_PRESS_MS 1000U

typedef enum
{
    BUTTON_STATE_RELEASED = 0,
    BUTTON_STATE_PRESSED = 1
} ButtonState;

typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_PRESSED,
    BUTTON_EVENT_RELEASED,
    BUTTON_EVENT_SHORT_PRESS,
    BUTTON_EVENT_LONG_PRESS
} ButtonEvent;

typedef struct
{
    ButtonState raw_state;
    ButtonState last_raw_state;
    ButtonState stable_state;

    unsigned int last_change_tick;
    unsigned int press_tick;

    int long_press_triggered;

    ButtonEvent event;
} Button;

void button_init(Button *button,
                 ButtonState initial_state,
                 unsigned int current_tick);

void button_update(Button *button,
                   ButtonState raw_state,
                   unsigned int current_tick);

ButtonEvent button_get_event(Button *button);

ButtonState button_get_state(const Button *button);

#endif