#include <stdio.h>
#include "system_type.h"
#include "fault_code.h"

static void SystemData_Init(SystemData *sys)
{
    sys->state = SYS_STATE_INIT;

    sys->radar.x = 0;
    sys->radar.y = 0;
    sys->radar.speed = 0;
    sys->radar.valid = 0;

    sys->led.led_zone = 0;
    sys->led.led_state = 0;

    sys->sensor.voltage = 0.0f;
    sys->sensor.current = 0.0f;
    sys->sensor.temperature = 0.0f;

    sys->fault_code = FAULT_NONE;
}

static void SystemData_Print(const SystemData *sys)
{
    printf("System state: %d\n", sys->state);
    printf("Radar x: %d\n", sys->radar.x);
    printf("Radar y: %d\n", sys->radar.y);
    printf("Radar speed: %d\n", sys->radar.speed);
    printf("Radar valid: %d\n", sys->radar.valid);
    printf("LED zone: %d\n", sys->led.led_zone);
    printf("LED state: %d\n", sys->led.led_state);
    printf("Sensor voltage: %.2f V\n",sys->sensor.voltage);
    printf("Sensor current: %.2f A\n", sys->sensor.current);
    printf("Sensor temperature: %.2f C\n",sys->sensor.temperature);
    printf("Fault code: 0x%04X\n", sys->fault_code);
}

int main(void)
{
    SystemData sys;

    SystemData_Init(&sys);

    sys.state = SYS_STATE_RUNNING;

    sys.radar.x = 120;
    sys.radar.y = -50;
    sys.radar.speed = 30;
    sys.radar.valid = 1;

    sys.led.led_zone = 2;
    sys.led.led_state = 1;

    sys.sensor.voltage = 12.5f;
    sys.sensor.current = 1.2f;
    sys.sensor.temperature = 35.6f;

    sys.fault_code |= FAULT_UART_TIMEOUT;
    sys.fault_code |= FAULT_CAN_ERROR;
    sys.fault_code |= FAULT_OVER_TEMP;
    sys.fault_code |= FAULT_LOW_VOLTAGE;

    SystemData_Print(&sys);

    return 0;
}