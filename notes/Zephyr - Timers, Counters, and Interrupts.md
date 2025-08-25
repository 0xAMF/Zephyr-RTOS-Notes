In Zephyr there are two types of timers, software timers and hardware timers (counters).
The software timers are called just timers, and hardware timers are called counters.
### Timers
Since timers are software components we don't need an overlay file, we can just have a `main.c` file and other files necessary to build the application.
```
.
├── CMakeLists.txt
├── prj.conf # left empty
└── src
    └── main.c
```
`CMakeLists.txt`
```CMake
cmake_minimum_required(VERSION 3.20.0)

set(BOARD stm32f4_disco)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(timer_demo)

target_sources(app PRIVATE src/main.c)

```
`src/main.c`
```C
#include <zephyr/kernel.h>

#define TIMER_MS 1000

static struct k_timer my_timer;
```
- The zephyr kernel defines a timer struct `k_timer` which holds the data necessary for defining a timer..
```C

// timer callback
void timer_callback(struct k_timer *timer)
{
    // make sure that the callback is triggered by our timer
    if (timer == &my_timer) {
		printk("Timer!\r\n");
    }
}
```
- The callback is what executes when the timer reaches the time specified in the `k_timer_start()` function.
```C

int main(void)
{
    k_timer_init(&my_timer, timer_callback, NULL);
    // wait for 1 sec and start calling our callback every second
    k_timer_start(&my_timer, K_MSEC(TIMER_MS), K_MSEC(TIMER_MS));

    while(1) {
		k_msleep(TIMER_MS);
    }
}
```
- `k_timer_init()`: Initializing the timer struct and linking the callback to it using.
	- The first argument is the address of the timer struct.
	- The second argument is the expiry function or what function is called when the timer expires.
	- The third argument is what function to invoke if the timer is stopped while running, we don't need that for that example.
- `k_timer_start()`: Start the timer. Takes the address of the timer struct, the initial duration and the timer period.
### Counters 
The counter API is provided by zephyr: [Zephyr API Documentation: Counter Interface](https://docs.zephyrproject.org/latest/doxygen/html/group__counter__interface.html), but how to set up the hardware in the overlay file can change a SoC to another, so i suggest to check out the board's `.dts` file in zephyr to see how things work, for this example i'm using the `stm32f4_disco` board.
Since we are using hardware, our application files should look like this.
```
.
├── boards
│   └── stm32f4_disco.overlay
├── CMakeLists.txt
├── prj.conf
└── src
    └── main.c
```
in `stm32f4_disco.dts` there is only one timer enabled by default in the `dts` file which is `timers2`.
`boards/st/stm32f4_disco/stm32f4_disco.dts`
```C
&timers2 {
	status = "okay";

	pwm2: pwm {
		status = "okay";
		pinctrl-0 = <&tim2_ch1_pa0>;
		pinctrl-names = "default";
	};
};
```
We can see that the `timers2` enables only `pwm`, but we need to enable counter. 
Digging into the zephyr files for `stm32f4` we see in SoC's `.dtsi`, we can see how the timers are defined and which modes it supports.
`dts/arm/st/f4/stm32f4.dtsi`
```C
timers2: timers@40000000 {
			compatible = "st,stm32-timers";
			reg = <0x40000000 0x400>;
			clocks = <&rcc STM32_CLOCK(APB1, 0U)>;
			resets = <&rctl STM32_RESET(APB1, 0U)>;
			interrupts = <28 0>;
			interrupt-names = "global";
			st,prescaler = <0>;
			status = "disabled";

			pwm {
				compatible = "st,stm32-pwm";
				status = "disabled";
				#pwm-cells = <3>;
			};

			counter {
				compatible = "st,stm32-counter";
				status = "disabled";
			};

			qdec {
				compatible = "st,stm32-qdec";
				status = "disabled";
				st,input-filter-level = <NO_FILTER>;
			};
		};
```
- In the SoC the timer is disabled by default, and so is all of its modes.
- Now we have an idea how to enable the counter in our overlay file.
- The interrupt is set up also in the `.dtsi` file.
	- `interrupts = <28 0>`: in the `arm,v7m-nvic` file which defines the binding for ARM's NVIC (Nested Vectored Interrupt Controller), we can see how these values are understood by the kernel.
`dts/bindings/interrupt-controller/arm,v7m-nvic.yaml`
```C
interrupt-cells:
  - irq
  - priority	
```
- That means that in `timers2`, we have an `irq` of 28 and a priority of `0`.
In our `.overlay` file in our application we can start writing the timer configuration to make the counter work.
`boards/stm32f4_disco.overlay` (Inside our application directory)
```C
/ {
    aliases {
		my-counter = &my_counter;
    };
};

&timers2 {
    status = "okay";
    st,prescaler = <4000>;
    my_counter: counter {
        compatible = "st,stm32-counter";
        status = "okay";
    };
};
```
- Even though the status is set `okay` for the `stm32f4_disco` board, we set here again just to make everything explicit.
- We set a prescaler for our timer if needed, the default value of the prescaler is `0` in the `stm32f4.dtsi` file.
- Now we make sure that we make the counter compatible with the `st,stm32-counter` binding file and set it to `okay` to enable it.
- In the aliases we make an alias for our `counter`, not the timer, because we are using the counter API in the `C` file.
Now we can write the counter code.
`src/main.c`
```C
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>

#define COUNTER_DELAY_MS 1000000
#define ALARM_CH_ID 0
```
- We include the libraries necessary to interact with the counter.
- We set the counter delay in milliseconds.
- An `ALARM_CH_ID` is set for the various alarms that could be set while configuring the counter.
```C
// counter callback
// we must setup our callback like this in zephyr
void counter_isr(const struct device *dev,
		 uint8_t chan_id,
		 uint32_t ticks,
		 void *user_data)
{
    // we cast our user data as alarm_cfg
    struct counter_alarm_cfg *alarm_cfg = user_data;

    alarm_cfg->ticks = counter_us_to_ticks(dev, COUNTER_DELAY_MS);
    counter_set_channel_alarm(dev, ALARM_CH_ID, alarm_cfg);

    printk("Counter!\r\n"); // bad idea to print in ISR
}
```
- In our `counter_isr` we reset the top value of the counter to start counting again from the top, to do that we extract the ticks using `counter_us_to_ticks()`.
	- `counter_us_to_ticks()`: converts our delay in milliseconds to counter ticks, based on the device configuration (prescaler) defined in our overlay file.
	- We assign `ticks` to the alarm configuration.
	- We set the value using `counter_set_channel_alarm()`, taking the device, `ALARM_CH_ID` and the `alarm_cfg` configuration file.
- The `printk` provides extra code to prevent the alarm from resetting before exiting the routine, it is considered a bad idea to use `printk` in an ISR, you should use [Workqueue Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/workqueue.html) to schedule the `printk` for safer thread operation.
```C
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
```
- As usual we extract the device info using `DEVICE_DT_GET(DT_ALIAS())` from the device tree, we have to pass in the name of the device defined in the `/aliases` node which we defined in our overlay, and then check if the device is ready.
- We set our `counter_alarm_cfg` configuration struct.
	- `callback`: takes the the ISR of the counter.
	- `ticks`: takes the ticks which we calculate using `counter_us_to_ticks` using the device struct and the time period we defined.
	- `user_data`: takes the address of the alarm configuration.
	- `flags`: takes the flags if found [Zephyr API Documentation: Counter Interface](https://docs.zephyrproject.org/latest/doxygen/html/group__counter__interface.html#COUNTER_ALARM_FLAGS).
```C
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
```
- Finally we start the alarm using `counter_start(<counter_device>)` function and check for errors and set the alarm channel using the device, alarm channel ID, and the alarm configuration.
`prj.conf`
```conf
CONFIG_COUNTER=y
```
- Make sure that the counter is enabled.
`CMakeLists.txt`
```CMake
cmake_minimum_required(VERSION 3.20.0)

set(BOARD stm32f4_disco)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(counter_demo)

target_sources(app PRIVATE src/main.c)
```
### Pin Change Interrupts
For example we are going to use external interrupts to print on the console using a button.
The project structure should look like this:
```
.
├── boards
│   └── stm32f4_disco.overlay
├── CMakeLists.txt
├── prj.conf
└── src
    └── main.c
```
Even though we don't need to define an overlay file for our button, because the `stm32f4_disco` have a button defined in the `.dts` file as `sw0`, but we defined a button in the overlay anyway as a practice for devicetrees.
`boards/stm32f4_disco.overlay`
```C
/ {
    aliases {
		my-button = &button_1;
    };
    
    buttons {
	compatible = "gpio-keys";
		button_1: button {
		    gpios = <&gpioa 0 GPIO_ACTIVE_HIGH>;
		};
    };
};
```
As usual we have our `CMakeLists.txt` file:
```CMake
cmake_minimum_required(VERSION 3.13.0)

set(BOARD stm32f4_disco)
set(DTC_OVERLAY_FILE "boards/stm32f4_disco.overlay")


find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(button_demo)

target_sources(app PRIVATE src/main.c)
```
- **Note**: we don't need to specify the `DTC_OVERLAY_FILE` actually, because the build looks for overlay files matching the board name in the `boards` directory anyway.
`src/main.c`
```C
#include "zephyr/kernel/thread_stack.h"
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <inttypes.h>

#define SLEEP_TIME_MS	1

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(DT_ALIAS(my_button), gpios);
// Struct for holding GPIO-related callback functions
static struct gpio_callback btn_cb_data;
```
- As always we define our button as a gpio device using `GPIO_DT_SPEC_GET(DT_ALIAS())` macros, taking the `my_button` node from the aliases in our overlay file.
- Define a `gpio_callback` struct to hold our callback data.
We mentioned before that using `printk` inside an ISR is a bad idea, that's why in this example we are going to use [Workqueue Threads](https://docs.zephyrproject.org/latest/kernel/services/threads/workqueue.html).
The workqueue receives a work object related a specific ISR through `k_work_submit()` function and stores that object in the queue, then the queue pops out the work object and calls its ISR when possible, for more details check out the docs.
```C

void button_work_isr(struct k_work *work)
{
    printk("Button Pressed: pin %d\r\n", button.pin);
}

// define the button_work_isr as work object
K_WORK_DEFINE(button_work, button_work_isr);
```
- In this example we use the system's workqueue, but we can define our own queue if needed.
- First we write the work ISR, in our case we want to use `printk` indicate that the buttons is pressed.
- After writing the ISR we have to define the work ISR as a `work` object, so we can be able to submit it into the workqueue.
```C
void button_isr(const struct device *dev,
		struct gpio_callback *cb,
		uint32_t pins)
{
    // check if the correct button was pressed
    if (BIT(button.pin) & pins) {
		k_work_submit(&button_work);
    }
}
```
- Here we implement the ISR for the external interrupt (our button), the ISR signature should contain the parameters mentioned above.
- Check if the correct pin is set then we submit our work queue using `k_work_submit(<work_obj>)` with the object related to the `printk` function instead of just writing `printk` in the ISR.
**Note**: The `BIT()` macro takes the pin number and converts it to a bit mask.
```C
int main(void)
{
    int ret;

    if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",button.port->name);
		return 0;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		ret, button.port->name, button.pin);
		return 0;
    }
```
- Here we check if the button is ready and configure it as gpio input, and check on errors of course.
```C
    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret < 0) {
		printk("ERR: Couldn't configure button as interrupt source \r\n");
		return 0;
    }
```
- We configure the gpio pin interrupt, passing in the gpio struct (`&button`), and the interrupt flag for configuration: [Zephyr API Documentation: GPIO Driver APIs](https://docs.zephyrproject.org/latest/doxygen/html/group__gpio__interface.html#ga91657faac28f9b213105dd61a419dd5a), specifying and the interrupt flag for configuration, for the flags check out: [Zephyr API Documentation: GPIO Driver APIs](https://docs.zephyrproject.org/latest/doxygen/html/group__gpio__interface.html#ga91657faac28f9b213105dd61a419dd5a).
```C
    // connect ISR to interrupt source
    gpio_init_callback(&btn_cb_data, button_isr, BIT(button.pin));
    gpio_add_callback(button.port, &btn_cb_data);

    while(1) {
		k_msleep(1000);
    }

    return 0;
}
```
- Here we connect the ISR to the interrupt source, first we initialize the callback struct using the `gpio_init_callback()` function.
- Then we add the callback using `gpio_add_callback()` function.
---
- [Timers — Zephyr Project Documentation](https://docs.zephyrproject.org/latest/kernel/services/timing/timers.html)
- [Zephyr API Documentation: Counter Interface](https://docs.zephyrproject.org/latest/doxygen/html/group__counter__interface.html)

#embedded #zephyr