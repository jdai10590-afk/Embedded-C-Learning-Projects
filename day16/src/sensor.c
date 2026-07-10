#include "sensor.h"
#include "debug.h"
#include "config.h"

void sensor_update(SystemStatus *sys, int cycle)
{
    switch(cycle)
    {
        case 0:
        case 1:
        case 4:
            sys->voltage = SENSOR_NORMAL_VOLTAGE;
            sys->current = SENSOR_NORMAL_CURRENT;
            sys->temperature = SENSOR_NORMAL_TEMPERATURE;
            break;

        case 2:
        case 3:
            sys->voltage = SENSOR_FAULT_VOLTAGE;
            sys->current = SENSOR_FAULT_CURRENT;
            sys->temperature = SENSOR_FAULT_TEMPERATURE;
            break;
        default:
            sys->voltage = SENSOR_NORMAL_VOLTAGE;
            sys->current = SENSOR_NORMAL_CURRENT;
            sys->temperature = SENSOR_NORMAL_TEMPERATURE;
            break;
    }
    DEBUG_PRINT("sensor update done\n");
}