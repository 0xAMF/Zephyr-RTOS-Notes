#include "zephyr/sys/printk.h"
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/console/console.h>


static const int32_t blink_max_ms = 2000;
static const int32_t blink_min_ms = 0;
static const int32_t printk_sleep_ms = 100;

// stack size settings: it is a good practice to statically determine the stack size, to avoid
// memory allocation that might take some time.
#define BLINK_THREAD_STACK_SIZE 512
#define INPUT_THREAD_STACK_SIZE 512
// define stack areas for the thread
K_THREAD_STACK_DEFINE(blink_stack, BLINK_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(input_stack, INPUT_THREAD_STACK_SIZE);

// declare thread data structure
static struct k_thread blink_thread;
static struct k_thread input_thread;

// Define Mutex
K_MUTEX_DEFINE(mtx);

// define shared blink sleep value
static int32_t blink_sleep_ms = 500;

// get LED struct from the device tree
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);

// console input thread entry point
void input_thread_start(void *arg1, void *arg2, void *arg3) {
    int8_t inc = 0;

    printk("Starting input thread\r\n");

    while(1) {
	const char *line = console_getline();

	if (line[0] == '+') {
	    inc += 1;
	} else if (line[0] == '+') { 
	    inc -= 1;
	} else {
	    continue;
	}

	k_mutex_lock(&mtx, K_FOREVER);
	blink_sleep_ms += (int32_t)inc * 100;
	if (blink_sleep_ms > blink_max_ms) {
	    blink_sleep_ms = blink_max_ms;
	} else if (blink_sleep_ms < blink_min_ms) {
	    blink_sleep_ms = blink_min_ms;
	} else {}
	k_mutex_unlock(&mtx);

	printk("Updating blink sleep to: %d\r\n", blink_sleep_ms);

    }
}


// blink thread entry point
void blink_thread_start(void *arg1, void *arg2, void *arg3) {
    int ret;
    int state = 0;
    int32_t sleep_ms;

    printk("Starting input thread\r\n");

    while(1) {
	k_mutex_lock(&mtx, K_FOREVER);
	sleep_ms = blink_sleep_ms;
	k_mutex_unlock(&mtx);

	state = !state;
	ret = gpio_pin_set_dt(&led, state);
	if (ret < 0) {
	    printk("ERR: Couldn't toggle pin\r\n");
	}

	k_msleep(sleep_ms);
    }
}

int main(void)
{
    int ret;
    k_tid_t blink_tid;
    k_tid_t input_tid;

    if (!gpio_is_ready_dt(&led)) {
	printk("ERR: GPIO Pin not ready\r\n");
	return 0;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT);
    if (ret < 0) {
	printk("ERR: Couldn't configure GPIO Pin\r\n");
	return 0;
    }

    console_getline_init();

    input_tid = k_thread_create(&input_thread,            // thread struct
				input_stack,              // Stack
				K_THREAD_STACK_SIZEOF(input_stack),
				input_thread_start,       // Entry point
				NULL,                     // arg1
				NULL,                     // arg2
				NULL,                     // arg3
				8,                        // priority
				0,                        // Options
				K_NO_WAIT);               // Delay
 

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
	k_msleep(1000);
    }

    return 0;

}
