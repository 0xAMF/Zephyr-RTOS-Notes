#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

static const int sleep_time_ms = 1000;
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int main(void)
{
    int ret;
    int state = 0;


    if (!gpio_is_ready_dt(&led)) {
	printk("Failed to set up gpio\r\n");
	return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret < 0) {
	return 0;
    }

    while(1) {
	state = !state;
	printk("LED state = %d\r\n", state);
	ret = gpio_pin_set_dt(&led, state);
	if (ret < 0) {
	    return 0;
	}

	k_msleep(sleep_time_ms);
    }
    
    return 0;
}
