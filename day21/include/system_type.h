#ifndef SYSTEM_TYPE_H
#define SYSTEM_TYPE_H

typedef enum
{
    SYS_MODE_INIT = 0,
    SYS_MODE_RUN,
    SYS_MODE_FAULT
} SystemMode;

typedef struct{
    int led_state;

    unsigned int gpio_output_reg;

    SystemMode mode;
    SystemMode last_mode;


    float voltage;
    float current;
    float temperature;

    unsigned int fault_code;
    
    unsigned int cycle_count;
    unsigned int fault_cycle_count;
    unsigned int fault_enter_count;
}SystemStatus;

#endif