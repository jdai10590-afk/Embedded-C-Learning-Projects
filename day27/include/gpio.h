#ifndef GPIO_H
#define GPIO_H

#include "system_type.h"

void gpio_init(SystemStatus *sys);
void gpio_led_on(SystemStatus *sys);
void gpio_led_off(SystemStatus *sys);
void gpio_led_toggle(SystemStatus *sys);
int gpio_led_read(const SystemStatus *sys);

#endif