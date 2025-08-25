#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>

#define COUNTER_DELAY_MS 1000000
#define ALARM_CH_ID 0

// counter callback
// we must setup our callback like this in zephyr
void counter_isr(const struct device *dev,
		 uint8_t chan_id,
		 uint32_t ticks,
		 void *user_data)
{
    // we cast our user data as alarm_cfg
    struct counter_alarm_cfg *alarm_cfg = user_data;

    // reset alarm: this function sets up the timer for us using the delay we pass as a parameter
    alarm_cfg->ticks = counter_us_to_ticks(dev, COUNTER_DELAY_MS);
    counter_set_channel_alarm(dev, ALARM_CH_ID, alarm_cfg);

    // extra code to prevent the alarm from reseting before exiting the routine
    printk("Counter!\r\n"); // bad idea to print in ISR
}

int main(void)
{
    int ret;
    const struct device *counter_dev = DEVICE_DT_GET(DT_ALIAS(my_counter));

    if (!device_is_ready(counter_dev)) {
	printk("ERR: timer is not ready\r\n");
	return 0;
    }

    struct counter_alarm_cfg alarm_cfg = {
	.callback = counter_isr,
	.ticks = counter_us_to_ticks(counter_dev, COUNTER_DELAY_MS),
	.user_data = &alarm_cfg,
	.flags = 0
    };

    // start the counter
    ret = counter_start(counter_dev);
    if (ret < 0) {
	printk("ERR (%d): failed to start the counter\r\n", ret);
	return 0;
    }

    // set alarm
    ret = counter_set_channel_alarm(counter_dev, ALARM_CH_ID, &alarm_cfg);
    if (ret < 0) {
	printk("ERR (%d): failed to configure timer\r\n", ret);
	return 0;
    }

    printk("Counter alarm is set to %u ticks\r\n", alarm_cfg.ticks);

    while(1) {

    }

    return 0;
}
