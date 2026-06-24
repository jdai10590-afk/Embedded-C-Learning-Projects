#ifndef SYSTEM_TYPE_H
#define SYSTEM_TYPE_H

#include <stdint.h>

typedef enum{
    SYS_STATE_INIT = 0,
    SYS_STATE_IDLE,
    SYS_STATE_RUNNING,
    SYS_STATE_FAULT,
    SYS_STATE_TEST
}SystemState;

typedef struct{
    int16_t x;
    int16_t y;
    int16_t speed;
    uint8_t valid;
}RadarTarget;

typedef struct{
    uint8_t led_zone;
    uint8_t led_state;
}LedStatus;

typedef struct{
    float voltage;
    float current;
    float temperature;
}SensoeData;

typedef struct{
    SystemState state;
    RadarTarget radar;
    LedStatus led;
    SensoeData sensor;
    uint16_t fault_code;
}SystemData;

#endif