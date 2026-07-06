#include "sensor.h"
#include "debug.h"
#include "config.h"

void sensor_update(SystemStatus *sys)
{
    sys->voltage = SENSOR_SIM_VOLTAGE;
    sys->current = SENSOR_SIM_CURRENT;
    sys->temperature = SENSOR_SIM_TEMPERATURE;

    DEBUG_PRINT("sensor update done\n");
}