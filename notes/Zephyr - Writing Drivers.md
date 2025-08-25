When writing a driver we can consider drivers as out-of-tree modules. The difference between writing drivers and writing modules is file structure and hierarchy and how the code relates to the device tree.

*Example*: `modules/button/`: a simple button driver.
```
.
├── CMakeLists.txt
├── drivers
│   ├── button
│   │   ├── button.c
│   │   ├── button.h
│   │   ├── CMakeLists.txt
│   │   └── Kconfig
│   ├── CMakeLists.txt
│   └── Kconfig
├── dts
│   └── bindings
│       └── input
│           └── custom,button.yaml
├── Kconfig
└── zephyr
    └── module.yaml
```
### Driver Source Code
- The driver source code should be inside a `drivers/<driver_name>` folder.
- The `button.c` and `button.h` inside the `drivers/button` directory contains the implementation of the driver.
### Writing A Simple Button Wrapper
#### Header Code
`button.h`
```C
#ifndef ZEPHYR_DRIVERS_BUTTON_H_
#define ZEPHYR_DRIVERS_BUTTON_H_

#include <zephyr/drivers/gpio.h>

// we use this struct instead using the sensor_driver_api which is more complex
struct button_api {
    int (*get)(const struct device *dev, uint8_t *state);
};

struct button_config {
    struct gpio_dt_spec btn;
    uint32_t id;
};

#endif
```
- In the header file we have to `struct`, one is for the API, and the other for the configuration.
- Since we are implementing a simple button driver we don't need to use the `sensor_driver_api` provided by zephyr.
`zephyr/drivers/sensor.h`
```C
__subsystem struct sensor_driver_api {
	sensor_attr_set_t attr_set;
	sensor_attr_get_t attr_get;
	sensor_trigger_set_t trigger_set;
	sensor_sample_fetch_t sample_fetch;
	sensor_channel_get_t channel_get;
	sensor_get_decoder_t get_decoder;
	sensor_submit_t submit;
};
```
- The `sensor_driver_api` requires filling all the members above, which is not needed in our case.
#### Driver Implementation
The first thing we do in the driver implementation is to tie our driver to a compatible binding file using `DT_DRV_COMPAT` macro.
```C
// ties to compatible = "custom,button" node in the device tree
#define DT_DRV_COMPAT custom_button 
```
- Zephyr looks for this exact macro so we have to check that we are writing exactly how it is.
**Logging**
A driver should use the zephyr logger to enable verbose output of how things work inside the driver and inside zephyr which makes debugging errors much easier.
```C
#include <errno.h>
#include <zephyr/logging/log.h>

#include "button.h"

// enable logging
LOG_MODULE_REGISTER(button);
```
- `errno.h`: useful to debug errors with specific error number (`errno`).
- `log.h`: includes macros and functions necessary for logging.
- `LOG_MODULE_REGISTER(button)`: enables logs of how zephyr sets up the driver.
**Function Declaration**
The function are declared as a static (private) function, but the function needed by the API will be the exposed to the user through the `*get()` function pointer in the button API.
```C
static int button_init(const struct device *dev);
static int get_button_state(const struct device *dev, uint8_t *state);
```
**Function Definition**
The `init` function takes a `device` struct which defines a device in zephyr.
In the `device` struct the `config` pointer is defined as a `void` pointer, which makes it easier to pass in custom configuration defined by the user.
`zephyr/device.h`
```C
struct device {
	const char *name;
	/** Address of device instance config information */
	const void *config;
	const void *api;
	struct device_state *state;
	void *data;
	/*.....*/
};
```
---
`init` function implementation: `button.c`
```c
static int button_init(const struct device *dev) {
    int ret;

    // cast the device config to our button_config struct, check device struct
    const struct button_config *cfg = (const struct button_config *)dev->config;
    // get the button struct from the config
    const struct gpio_dt_spec *btn = &cfg->btn;
    
    LOG_DBG("Initializing button (ID = %u)\r\n", cfg->id);
    // check if the button is ready
    if (!gpio_is_ready_dt(btn)) {
		LOG_ERR("GPIO is not ready\r\n");
		return -ENODEV;
    }

    // setup button as input
    ret = gpio_pin_configure_dt(btn, GPIO_INPUT);
    if (ret < 0) {
		LOG_ERR("Couldn't configure GPIO as input\r\n");
		return -ENODEV;
    }

    return 0;
}
```
- We extract the configuration from the device struct and cast it to `button_config` structure which we defined earlier.
- We define a `gpio_dt_spec` using the configurations we extracted from the device.
- We set up the button gpio pin through the gpio API.
---
*`get_button_state` function implementation: `button.c`*
```C
static int get_button_state(const struct device *dev, uint8_t *state) {
    int ret;

    // cast the device config to our button_config struct, check device struct
    const struct button_config *cfg = (const struct button_config *)dev->config;
    // get the button struct from the config
    const struct gpio_dt_spec *btn = &cfg->btn;

    // poll the button's state
    ret = gpio_pin_get_dt(btn);
    if (ret < 0) {
	LOG_ERR("Error (%d): failed to read the button pin\r\n", ret);
	return ret;
    } else {
	*state = ret;
    }

    return 0;
}
```
- First we extract the configuration from the `device` struct.
- We define a `gpio_dt_spec` and cast the button configuration to it.
- we use the gpio API to get the state of the button using `gpio_pin_get_dt()` and assign it the `state` variable passed in the parameters.
**Exposing the `get_button_state` function to the API**
```C
// define public API fuctions for the driver
static const struct button_api button_api_funcs = {
    .get = get_button_state,
};
```
---
**Devicetree Handling**
```C
#define BUTTON_DEFINE(inst)                                                 \
                                                                            \
    /* Create an instance of the config struct, populate with DT values */  \
    static const struct button_config button_config_##inst = {              \
        .btn = GPIO_DT_SPEC_GET(                                            \
            DT_PHANDLE(DT_INST(inst, custom_button), pin), gpios),          \
        .id = inst                                                          \
    };                                                                      \
                                                                            \
    DEVICE_DT_INST_DEFINE(inst,                                             \
                          button_init,                                      \
                          NULL,                                             \
                          NULL,                                             \
                          &button_config_##inst,                            \
                          POST_KERNEL,                                      \
                          CONFIG_GPIO_INIT_PRIORITY,                        \
                          &button_api_funcs);                               \

DT_INST_FOREACH_STATUS_OKAY(BUTTON_DEFINE)
```
- The `BUTTON_DEFINE(inst)` expansion macro is used to define driver instances.
	- The `inst` parameter is going to be defined for us as a part of the zephyr build process.
	- We fill out the `button_config` struct giving each button unique name based on the `inst` using the `##inst` that gets replaced with the instance number assigned.
	- Inside the struct we define the `btn` using `GPIO_DT_SPEC_GET()` which acquires the button configuration through the instance number of the `custom_button` through the `DT_INSTANCE()` and `DT_PHANDLE()` macro, the property `pin` must match the property defined in the binding file.
	- The `id` is then assigned based on the instance number.
	- `DEVICE_DT_INST_DEFINE()`: Creates a device instance from a devicetree node identifier and registers the `init` function to run during boot, then passing the priority and our driver API.
		- The documentation for this macro can be found in `zephyr/device.h`
- `DT_INST_FOREACH_STATUS_OKAY()`: The device tree build process calls this to create an instance of structs for each device defined in the devicetree.
	- This macro takes our expansion macro (`BUTTON_DEFINE`) and passes an instance number to it for each node defined in the devicetree.
*Note:* inside the expansion macro we can only use comments in the `/**/` format, if use `//` it would fail.
#### Driver CMake Files
The first CMake file should be inside the driver source code directory .
(`modules/button/drivers/button/CMakeLists.txt`)
```CMake
zephyr_library()
zephyr_library_sources(button.c)
zephyr_include_directories(.)
```
- `zephyr_library()`: declares the current directory as a zephyr library, if no name is given the name of the library is derived from the directory.
- `zephyr_library_sources()`: lists the library's source code.
- `zephyr_include_directories()`: add header files to the CMake search directories.
The second CMake file should be inside the `drivers` directory.
(`modules/button/drivers/CMakeLists.txt`)
```CMake
add_subdirectory_ifdef(CONFIG_CUSTOM_BUTTON button)
```
- imports the `button` directory if the symbol associated with the driver is set.
The final CMake file should be in the parent directory.
(`modules/button/CMakeLists.txt`)
```CMake
add_subdirectory(drivers)
zephyr_include_directories(drivers)
```
- `add_subdirectory()`: include required subdirectories.
- `zephyr_include_directories()`: add subdirectories to the compiler's search path.
#### Driver Kconfig Files
Similar to the CMake files, each subdirectory level has a `Kconfig` file.
The first one inside the the driver's source code directory.
(`modules/button/drivers/button/Kconfig`)
```Kconfig
config CUSTOM_BUTTON
    bool "Custom Button"
    default n
    depends on GPIO
    help
	Enable custom button driver
```
- Defines the symbol associated with our driver and declares the default value and which driver it depends on.
The second and third `Kconfig` include the `Kconfig` in the subdirectory using `rsource`.
(`modules/button/drivers/Kconfig`)
```Kconfig
rsource "button/Kconfig"
```
(`modules/button/Kconfig`)
```Kconfig
rsource "drivers/Kconfig"
```
#### Defining The Bindings File
The bindings file has to match the compatibility string we defined the `DT_DRV_COMPAT` with in the driver source code.
The file should be under the `dts/bindings/` directory, its optional to group bindings in a subdirectory under those directories.
`dts/bindings/input/cutom,button.yaml`
```YAML
description: custom button
compatible: "custom,button"
properties:
  pin:
    type: phandle
    required: true
```
#### Defining The Driver As A Module
In the top directory we have to create a `zephyr` directory with one file that must be named `module.yaml`. This file defines the metadata necessary for to recognize the driver.
`zephyr/module.yaml`
```YAML
name: button
build:
  cmake: .
  kconfig: Kconfig
  settings: 
    dts_root: .
```
### Writing Application Using The Driver
#### The Overlay File
First we define our button inside of the `.overlay` file in the application directory.
`boards/stm32f4_disco.overlay`
```C
/ {
    aliases {
		my-button = &button_1;
    };

    custom-buttons {
		button_1: custom_button_1 {
		    compatible = "custom,button";
		    pin = <&a0>;
		};
    };

    gpio-keys {
		compatible = "gpio-keys";

		a0: gpioa {
		    gpios = <&gpioa 0 GPIO_ACTIVE_HIGH>;
		};
    };
};
```
- The compatible property must should be assigned to `"custom,button"` to match our driver compatible string.
#### The CMake File
To add make CMake see our driver we have to set the `ZEPHYR_EXTRA_MODULES` variable and make it point to the location of the driver.
`CMakeLists.txt`
```CMake
cmake_minimum_required(VERSION 3.20.0)

set(BOARD stm32f4_disco)
set(DTC_OVERLAY_FILE "boards/stm32f4_disco.overlay")
## include our driver
set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../modules/button")
##
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(custom_button)

target_sources(app PRIVATE src/main.c)
```
#### `prj.conf` File
In this file we make sure to enable the symbol related to the driver, and other symbols necessary required to make things work.
`prj.conf`
```conf
CONFIG_CUSTOM_BUTTON=y
CONFIG_LOG=y
CONFIG_LOG_DEFAULT_LEVEL=4
CONFIG_GPIO=y
```
#### Application Code 
`main.c`
```C
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
```
- We include `button.h` and CMake figures out where the file is.
- We fetch the device from the `.overlay` file using `DEVICE_DT_GET()` macro.
- We define a `button_api` and cast the `api` value from the device to it.
- Now we can access the custom APIs we defined in the driver source code and print out the state of the button.

---
# Sources
- [Introduction to Zephyr Part 6: How to Write a Device Driver | DigiKey](https://www.youtube.com/watch?v=vXAg_UbEurc&list=PLEBQazB0HUyTmK2zdwhaf8bLwuEaDH-52&index=6&ab_channel=DigiKey)
- [How to Build Drivers for Zephyr | Interrupt](https://interrupt.memfault.com/blog/building-drivers-on-zephyr)

#embedded #zephyr 