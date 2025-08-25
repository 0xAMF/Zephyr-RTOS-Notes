#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define BLINK_SLEEP_MS 500
#define PRINTK_SLEEP_MS 700

// stack size settings: it is a good practice to statically determine the stack size, to avoid
// memory allocation that might take some time.
#define BLINK_THREAD_STACK_SIZE 256
// define stack areas for the thread
K_THREAD_STACK_DEFINE(blink_stack, BLINK_THREAD_STACK_SIZE);

// declare thread data structure
static struct k_thread blink_thread;

// get LED struct from the device tree
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);

// blink thread entry point
void blink_thread_start(void *arg1, void *arg2, void *arg3) {
    int ret;
    int state = 0;

    while(1) {
	state = !state;
	ret = gpio_pin_set_dt(&led, state);
	if (ret < 0) {
	    printk("ERR: Couldn't toggle pin\r\n");
	}

	k_msleep(BLINK_SLEEP_MS);
    }
}

int main(void)
{
    int ret;
    k_tid_t blink_tid;

    if (!gpio_is_ready_dt(&led)) {
	printk("ERR: GPIO Pin not ready\r\n");
	return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret < 0) {
	printk("ERR: Couldn't configure GPIO Pin\r\n");
	return 0;
    }

    // start the blink thread
    blink_tid = k_thread_create(&blink_thread,            // thread struct
				blink_stack,              // Stack
				K_THREAD_STACK_SIZEOF(blink_stack),
				blink_thread_start,       // Entry point
				NULL,                     // arg1
				NULL,                     // arg2
				NULL,                     // arg3
				7,                        // priority
				0,                        // Options
				K_NO_WAIT);               // Delay
    
    while(1) {
	printk("Hello\r\n");
	k_msleep(PRINTK_SLEEP_MS);
    }

    return 0;

}
