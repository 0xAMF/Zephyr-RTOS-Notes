Whenever we work with zephyr application there is a common workflow.
## Creating the Application Directory Structure
The common directory structure includes, `src/main.c`, `prj.conf` for the Kconfig configurations, and a `CMakeLists.txt` for the build.
If we are customizing the hardware we might need a `.overlay` file, most likely will be inside a `boards` directory.
So for our ADC example, of course we will need hardware customization, so the structure will be something like:
```
.
├── boards
│   └── stm32f4_disco.overlay
├── CMakeLists.txt
├── prj.conf
└── src
    └── main.c
```
## Setting Up the Hardware
If we need to configure the hardware we need to know how the hardware is defined in the `.dts`, `.dtsi` and `.yaml` binding files of the board and SoC inside the zephyr project files, in our case, we need to figure how the hardware should be set up.
`dts/arm/st/f4/stm32f4.dtsi`
```C

adc1: adc@40012000 {
	compatible = "st,stm32f4-adc", "st,stm32-adc";
	reg = <0x40012000 0x400>;
	clocks = <&rcc STM32_CLOCK(APB2, 8U)>;
	interrupts = <18 0>;
	status = "disabled";
	#io-channel-cells = <1>;
	resolutions = <STM32_ADC_RES(12, 0x00)
			       STM32_ADC_RES(10, 0x01)
				   STM32_ADC_RES(8, 0x02)
			       STM32_ADC_RES(6, 0x03)>;
	sampling-times = <3 15 28 56 84 112 144 480>;
	st,adc-clock-source = <SYNC>;
	st,adc-sequencer = <FULLY_CONFIGURABLE>;
};
```
In the `.dtsi` for our SoC, we see that multiple properties are defined, but not all of them, if we take a look in the `st,stm32-adc` binding file we will have a better idea on how to set up the ADC in our `.overlay` file.
`dts/bindings/adc/st,stm32-adc.yaml`
```YAML

description: ST STM32 family ADC

compatible: "st,stm32-adc"

include: [adc-controller.yaml, pinctrl-device.yaml]

properties:
  reg:
    required: true
	...
...
```
- Here we see that the `YAML` file include a common `adc-controller.yaml` file that defines how all ADCs should be defined regardless of the SoC.
In the `adc-controller.yaml` under `dts/bindings/adc/` directory, we can see some required properties, some of which are defined in the SoC's `.dtsi` some of it are not, in our case, there is no ADC defined in our board's `.dts` (`stm32f4_disco.dts`), so we have to do a lot of work ourselves.
`dts/bindings/adc/adc-controller.yaml`
```YAML
include: base.yaml
properties:
  "#io-channel-cells":
    type: int
    required: true

  "#address-cells":
    const: 1

  "#size-cells":
    const: 0

child-binding:
  description: |
    Channel configuration.

    All nodes using this binding must be named "channel", otherwise their
    data will not be accessible for the ADC API macros.

    This is based on Linux, documentation:
      https://www.kernel.org/doc/Documentation/devicetree/bindings/iio/adc/adc.yaml
  ...
...
```
- The first thing we can take from the binding file is that all nodes using the `adc-controller.yaml` binding has to be named channel, the channel will be a sub-node of our ADC.
- The channel requires some properties, we can find them in the `adc-controller.yaml` file.
- For our example we defined 5 channel properties that were required for our board.
	- `zephyr,gain`: determines the gain for the ADC, the gain is set as one of the values of the enum defined in the `.yaml` file.
	- `zephyr,reference`
	- `zephyr,acquisition-time`
*Note*: I set these properties using trial and error, because first i didn't have any properties set, then I got an error, went back to the `.yaml` files, added another property, got another error, went back again, and so on. so I recommend that you go through the the bindings file before writing your overlay.
Before starting to write our own overlay we should first look for board overlay files in the ADC sample code with a similar architecture to have an idea how to set up our own ADC.
`samples/drivers/adc/adc_dt/boards/stm32h745g_disco.overlay`
```C
&adc1 {
	#address-cells = <1>;
	#size-cells = <0>;

	channel@0 {
		reg = <0>;
		zephyr,gain = "ADC_GAIN_1";
		zephyr,reference = "ADC_REF_INTERNAL";
		zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
		zephyr,resolution = <16>;
	};
};
```
- As we saw in the `adc-controller.yaml` binding file, we had to define `channel` child node that contains the channel properties required for the ADC.
- Even though the `#address-cells` and `#size-cells` are set in the `adc-controller.yaml` file, we should be explicitly define  them in our overlay file to avoid errors that might happen due to conflicted values.
Now we start writing our `.overlay` file.
`boards/stm32f4_disco.overlay`
```C
/ {
    aliases {
		my-adc = &adc1;
		my-adc-channel = &adc1_ch0;
    };
};
```
- Here we created an alias of the ADC and the ADC channel.
```C

&pinctrl {
    adc1_ch0: adc1_ch0 {
        pinmux = <STM32_PINMUX('A', 2, ANALOG)>; /* PA2 as ADC input */
    };
};
```
- Define ADC channel pins, this is specific for the `STM32` SoCs, for other SoCs check their `pinctrl` files to see how you can set up pins.
```C
&adc1 {
    #address-cells = <1>;
    #size-cells = <0>;
    status = "okay";
    pinctrl-0 = <&adc1_ch0>;
    pinctrl-names = "default";
    st,adc-prescaler = <4>;

    adc1_ch0: channel@0 {
        reg = <0>; /* channel number */
        zephyr,gain = "ADC_GAIN_1_4";
        zephyr,reference = "ADC_REF_INTERNAL";
		zephyr,vref-mv = <3894>; /* optional but useful to calculate the voltage in the source file */
        zephyr,acquisition-time = <ADC_ACQ_TIME_DEFAULT>;
        zephyr,resolution = <12>;
    };
};
```
- Since the ADC is not enabled by default in the board's `.dts` file, we have to set the status to `okay`, 
- We set up our pins be referencing `adc_ch0` node created in our `pinctrl` node.
- Set the prescaler since it is a required value that is not set in our board `.dts`, the values we can set are in an enum in the `st,stm32-adc.yaml` binding file.
- As we saw earlier in the ADC sample, we define a channel child node, with a channel number stored in the `reg` required property, after that we set our required properties as we saw in the binding file.
Now our overlay is ready and we can write the C source file.
`src/main.c`
```C
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>

// Settings
static const int32_t sleep_time_ms = 100;

// Get Devicetree configurations
#define MY_ADC_CH DT_ALIAS(my_adc_channel)
static const struct device *adc = DEVICE_DT_GET(DT_ALIAS(my_adc));
static const struct adc_channel_cfg adc_ch = ADC_CHANNEL_CFG_DT(MY_ADC_CH);
```
- Including the required libraries and defining necessary variables.
- Creating the device struct for the ADC, and creating the channel configuration `adc_channel_cfg` using `ADC_CHANNEL_CFG_DT()` macro.
```C
int main(void)
{
    int ret;
    uint16_t buf;
    uint16_t val_mv;
    int32_t vref_mv;

    // Get Vref (mV) from Devicetree property
    vref_mv = DT_PROP(MY_ADC_CH, zephyr_vref_mv);
```
- declare required variables and acquire `vref_mv` which represents the reference voltage property from the channel node.	
```C
    // Buffer and options for ADC (defined in adc.h)
    struct adc_sequence seq = {
        .channels = BIT(adc_ch.channel_id),
        .buffer = &buf,
        .buffer_size = sizeof(buf),
        .resolution = DT_PROP(MY_ADC_CH, zephyr_resolution)
    };

    // Make sure that the ADC was initialized
    if (!device_is_ready(adc)) {
        printk("ADC peripheral is not ready\r\n");
        return 0;
    }

    // Configure ADC channel
    ret = adc_channel_setup(adc, &adc_ch);
    if (ret < 0) {
        printk("Could not set up ADC\r\n");
        return 0;
    }
```
- Here we create the ADC sampling sequence struct:
	- `channels`: takes a bit mask indicating the channel to be included in each sampling of this sequence.
	- `buffer`: takes a pointer to a buffer where the samples are to be written.
	- `buffer_size`: takes the size of the buffer.
	- `resolution`: the ADC resolution which is defined in the ADC channel node.
- Check if the ADC device is ready, and setup the ADC channel.
```C
    // Do forever
    while (1) {
        // Sample ADC
        ret = adc_read(adc, &seq);
        if (ret < 0) {
            printk("Could not read ADC: %d\r\n", ret);
            continue;
        }
        // Calculate ADC value (mV)
        val_mv = buf * vref_mv / (1 << seq.resolution);
        // Print ADC value
        printk("Raw: %u, mV: %u\r\n", buf, val_mv);
        // Sleep
        k_msleep(sleep_time_ms);
    }
}
```
- Read from the ADC using the `adc_read()` function, store the output in the `seq` struct.
- Calculate the ADC value in millivolts.
Finally we can set up our `prj.conf` and `CMakeLists.txt` files
`prj.conf`
```conf
CONFIG_ADC=y
CONFIG_ADC_STM32=y
```
`CMakeLists.txt`
```conf
cmake_minimum_required(VERSION 3.20.0)

set(BOARD stm32f4_disco)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(adc_demo)

target_sources(app PRIVATE src/main.c)
```
---
[Analog-to-Digital Converter (ADC) — Zephyr Project Documentation](https://docs.zephyrproject.org/latest/hardware/peripherals/adc.html)
[Introduction to Zephyr Part 5: Devicetree Bindings | DigiKey](https://www.youtube.com/watch?v=nL1kZ3kPyo0&list=PLEBQazB0HUyTmK2zdwhaf8bLwuEaDH-52&index=5&ab_channel=DigiKey)
#embedded #zephyr 