## Creating Threads
To create a thread we need to do the following:
1. Define the thread's stack
2. Declare the thread struct `k_thread`
3. Implement the thread entry point
4. Start the thread using `k_thread_craete()` with a return value of type `k_tid_t`
### Example: Blink LED Thread
**Project Structure**
- The board `.overlay` file is optional though, the `stm32f4_disco` board has LEDs defined in their `.dts` files, so we can use them immediately without defining our own LEDs.
- The `prj.conf` remains empty
```
.
├── boards
│   └── stm32f4_disco.overlay
├── CMakeLists.txt
├── prj.conf
└── src
    └── main.c
```
**`CMakeLists.txt`**
```CMake
cmake_minimum_required(VERSION 3.13.0)

set(BOARD stm32f4_disco) # chose your own board here

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(threads_demo)

target_sources(app PRIVATE src/main.c)
```
**`src/main.c`**
Initializing the the thread stack, thread struct, and other variables/macros needed for the code
```C
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define BLINK_SLEEP_MS 500
#define PRINTK_SLEEP_MS 700

// define the thread's stack
#define BLINK_THREAD_STACK_SIZE 256
K_THREAD_STACK_DEFINE(blink_stack, BLINK_THREAD_STACK_SIZE);

// declare thread data structure
static struct k_thread blink_thread;

// get LED struct from the device tree
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(my_led), gpios);
```
- `K_THREAD_STACK_DEFINE`: defines the stack for our thread, takes the size and initializes a variable `blink_stack` based on that size.
	- Alternatively, the stack area may be dynamically allocated using [`k_thread_stack_alloc()`](https://docs.zephyrproject.org/latest/doxygen/html/group__thread__apis.html#gafe00cc70bac8a47ba6dda21bde508614) and freed using [`k_thread_stack_free()`](https://docs.zephyrproject.org/latest/doxygen/html/group__thread__apis.html#ga95560cb85f6656b981a9a50ff2cd70b7).
- `blink_thread`: contains the thread information 
Implementing the thread's entry point
```C
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
```
- The thread can have up to 3 arguments, however if we provide no argument in the signature the compiler will not complain, but it is considered good practice to provide the 3 argument even if we don't use them.
- The thread code toggles an LED using `gpio_pin_set_dt()` function every `BLINK_SLEEP_MS` time.
Creating and running the thread in the main function
```C
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
```
- After checking on the LED pin, we create our thread using `k_thread_create()` function providing the arguments that defines our thread as mentioned in the code above.
- Higher thread priority number = Lower priority.
- Specifying a start delay of [`K_NO_WAIT`](https://docs.zephyrproject.org/latest/doxygen/html/group__clock__apis.html#ga3d9541cfe2e8395af66d186efa77362f) instructs the kernel to start thread execution immediately.
- Thread options can be found in the documentation: [Threads — Zephyr Project Documentation](https://docs.zephyrproject.org/latest/kernel/services/threads/index.html#thread-options), but we decided to provide no option for this example.
After building and running the code we can see that the `printk` and the LED blink operate separately in their own threads at their own pace with different delays. 
## Synchronization and Mutex
For this example we have to threads, an input thread which takes user input to control the frequency with which the LED blinks, and an blink thread that blinks an LED every time period.
The file structure is the same as before, we only modify the `src/main.c` file and enable console related symbols for the user input.
`prj.conf`
```conf
CONFIG_CONSOLE_SUBSYS=y
CONFIG_CONSOLE_GETLINE=y
```
`src/main.c`
Including required libraries and defining the the variables needed.
```C
#include <stdint.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/console/console.h>


static const int32_t blink_max_ms = 2000;
static const int32_t blink_min_ms = 0;
static const int32_t printk_sleep_ms = 100;
static int32_t blink_sleep_ms = 500;
```
Thread stack size definition.
```C

#define BLINK_THREAD_STACK_SIZE 512
#define INPUT_THREAD_STACK_SIZE 512
// define stack areas for the thread
K_THREAD_STACK_DEFINE(blink_stack, BLINK_THREAD_STACK_SIZE);
K_THREAD_STACK_DEFINE(input_stack, INPUT_THREAD_STACK_SIZE);

```
Declaring the thread data structures and defining the Mutex to protect our shared resource.
```C
// declare thread data structure
static struct k_thread blink_thread;
static struct k_thread input_thread;
// Define Mutex
K_MUTEX_DEFINE(mtx);
```
Create led gpio structure, getting the led defined in the `stm32f4_disco.dts` file, for a specific board check out the board's `.dts` in the zephyr source code, or create your own `.overlay` file with a custom led pin.
```C
const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(blue_led_6), gpios);
```
**Console Input Thread Entry Point**
```C
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
```
- Get user input from `console_getline()` function, returns a line string.
	- Take the first character and check whether it is a `+` or `-`, based on that increase or decrease the LED frequency.
- Enter critical section, lock the mutex using `k_mutex_lock()`, takes the mutex object and a lock duration.
- Exit critical section and unlock the mutex using the `k_mutex_unlock()` function which takes the mutex only.
**Blink Thread Entry Point**
```C
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
```
- Before assigning `sleep_ms` to `blink_sleep_ms` we lock the mutex using `k_mutex_lock()` to protect our shared resource (`blink_sleep_ms`), after assignment we unlock using `k_mutex_unlock()`.
- After that we blink our LED using the `gpio_pin_set_dt()` gpio API function.
**Initializing the Thread in the main Function**
```C
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

	// start the console input thread
    input_tid = k_thread_create(&input_thread,            // thread struct
				input_stack,              // Stack
				K_THREAD_STACK_SIZEOF(input_stack),
				input_thread_start,       // Entry point
				NULL,                     // arg1
				NULL,                     // arg2
				NULL,                     // arg3
				7,                        // priority
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
```
- After checking on the LED gpio pin, we initialize the console using `console_getline_init()`.
- We start both the blink thread and the console input thread.
- Both threads should have the same priority, because if we give the blink thread the higher priority, we might face issues taking input.
	- Having two thread with the same priority makes zephyr switch to the thread that waits for longer time.
---
#embedded #zephyr 