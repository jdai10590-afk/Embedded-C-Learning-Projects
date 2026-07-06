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
    SystemMode mode;
    float voltage;
    float current;
    float temperature;
    unsigned int fault_code;
}SystemStatus;

#endif