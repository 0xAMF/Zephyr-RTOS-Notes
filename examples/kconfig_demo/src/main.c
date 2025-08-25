#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <zephyr/toolchain.h>
#include <stdarg.h>
#include <stddef.h>

#include "say_hello.h"

static const int32_t sleep_time_ms = 1000;

int main(void)
{
    uint32_t rnd;
    double rnd_f;

    while(1) {
	rnd = sys_rand32_get();	
	rnd_f = (double)rnd/(UINT32_MAX + 1.0);

	printf("Random Value: %.3f\r\n", rnd_f);

	say_hello();

	k_msleep(sleep_time_ms);
    }

    return 0;
}
