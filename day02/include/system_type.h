#ifndef SYSTEM_TYPE_H
#define SYSTEM_TYPE_H

typedef struct{
    int led_state;
    float voltage;
    float current;
    float temperature;
    unsigned int fault_code;
}SystemStatus;

#endif