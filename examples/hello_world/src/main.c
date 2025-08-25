#include <zephyr/kernel.h>

int main(void)
{
    int n = 0;

    while (1)
    {
	printk("Test %d\r\n", n++);
        k_msleep(1000);
    }
    return 0;
}

