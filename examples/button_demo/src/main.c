#include "zephyr/kernel/thread_stack.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <inttypes.h>

#define SLEEP_TIME_MS	1

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(my_button), gpios);
// Struct for holding GPIO-related callback functions
static struct gpio_callback btn_cb_data;


void button_work_isr(struct k_work *work)
{
    printk("Button Pressed: pin %d\r\n", button.pin);
}

// define the button_work_isr as work object
K_WORK_DEFINE(button_work, button_work_isr);

// GPIO Callback
void button_isr(const struct device *dev,
		struct gpio_callback *cb,
		uint32_t pins)
{
    // check if the correct button was pressed
    if (BIT(button.pin) & pins) {
	k_work_submit(&button_work);
    }
}


int main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&button)) {
	printk("Error: button device %s is not ready\n",
	button.port->name);
	return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
	printk("Error %d: failed to configure %s pin %d\n",
	ret, button.port->name, button.pin);
	return 0;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
	printk("ERR: Couldn't configure button as interrupt source \r\n");
	return 0;
    }

    // connect ISR to interrupt source
    gpio_init_callback(&btn_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &btn_cb_data);

    while(1) {
	k_msleep(1000);
    }

    return 0;
}
