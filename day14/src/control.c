#include "control.h"
#include "config.h"
#include "debug.h"

void control_update(SystemStatus *sys)
{
    switch(sys->mode)
    {
        case SYS_MODE_INIT:
            sys->led_state = LED_STATE_OFF;
            DEBUG_PRINT("control: LED OFF in INIT mode\n");
            break;

        case SYS_MODE_RUN:
            sys->led_state = LED_STATE_ON;
            DEBUG_PRINT("control: LED ON in RUN mode\n");
            break;

        case SYS_MODE_FAULT:
            sys->led_state = LED_STATE_BLINK;
            DEBUG_PRINT("control: LED BLINK in FAULT mode\n");
            break;

        default:
            sys->led_state = LED_STATE_OFF;
            DEBUG_PRINT("control: LED OFF in UNKNOWN mode\n");
            break;
    }
}