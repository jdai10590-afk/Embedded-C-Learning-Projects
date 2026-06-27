#include "sensor.h"

void sensor_update(SystemStatus *sys)
{
    sys->voltage = 9.5f;
    sys->current = 2.5f;
    sys->temperature = 75.0f;
}