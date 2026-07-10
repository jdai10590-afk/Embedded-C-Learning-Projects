#include "gpio.h"
#include "config.h"
#include "debug.h"

void gpio_init(SystemStatus *sys)
{
    sys->gpio_output_reg = 0;
    DEBUG_PRINT("gpio init done\n");
}

void gpio_led_on(SystemStatus *sys)
{
    sys->gpio_output_reg |= BIT(LED_GPIO_PIN);
    DEBUG_PRINT("gpio: LED bit set\n");
}

void gpio_led_off(SystemStatus *sys)
{
    sys->gpio_output_reg &= ~BIT(LED_GPIO_PIN);
    DEBUG_PRINT("gpio: LED bit clear\n");
}

void gpio_led_toggle(SystemStatus *sys)
{
    sys->gpio_output_reg ^= BIT(LED_GPIO_PIN);
    DEBUG_PRINT("gpio: LED bit toggle\n");
}

int gpio_led_read(const SystemStatus *sys)
{
    if(sys->gpio_output_reg & BIT(LED_GPIO_PIN))
    {
        return 1;
    }

    return 0;
}
