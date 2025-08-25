#include <zephyr/kernel.h>

#define TIMER_MS 1000

static struct k_timer my_timer;

// timer callback
void timer_callback(struct k_timer *timer)
{
    // make sure that the callback is triggered by our timer
    if (timer == &my_timer) {
	printk("Timer!\r\n");
    }
}

int main(void)
{
    k_timer_init(&my_timer, timer_callback, NULL);
    // wait for 1 sec and start calling our callback every second
    k_timer_start(&my_timer, K_MSEC(TIMER_MS), K_MSEC(TIMER_MS));

    while(1) {
	k_msleep(TIMER_MS);
    }
}
