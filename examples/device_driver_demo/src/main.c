#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>

#include "button.h"

#define SLEEP_TIME_MS 50
static const struct device *btn = DEVICE_DT_GET(DT_ALIAS(my_button));

int main(void)
{
    int ret;
    int8_t state;

    if (!device_is_ready(btn)) {
	printk("Error: buttons are not ready\r\n");
	return 0;
    }

    const struct button_api *btn_api = (const struct button_api *)btn->api;

    while(1) {
	ret = btn_api->get(btn, &state);
	if (ret < 0) {
	    printk("Error (%d): failed to read button", ret);
	    continue;
	}
	printk("state: %u\r\n", state);

	k_msleep(SLEEP_TIME_MS);
    }

    return 0;
}
