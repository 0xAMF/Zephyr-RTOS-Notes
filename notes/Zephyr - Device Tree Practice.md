## Resolving Device Objects With `DEVICE_DT_GET`
Before resolving macros manually, it is always worth looking into the documentation. We have two nested macros, so it makes sense to check the inner `DT_GPIO_CTLR_BY_IDX` first. The API documentation claims that it can be used to “_get the node identifier for the controller phandle from a gpio phandle-array property_”. For the assignment `gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;` in our LED node, we’d thus expect to get the node identifier for the phandle `&gpio0`.
Let’s have a look at the macro and its expansion:
```c
// We want to know the expansion of the following macro:
#define DT_GPIO_CTLR_BY_IDX(node_id, gpio_pha, idx) \
  DT_PHANDLE_BY_IDX(node_id, gpio_pha, idx)

// Knowing that DT_PHANDLE_BY_IDX is defined as follows:
#define DT_PHANDLE_BY_IDX(node_id, prop, idx) \
  DT_CAT6(node_id, _P_, prop, _IDX_, idx, _PH)

// Given:
// node_id = DT_N_S_leds_S_led_0
// prop    = gpios
// idx     = 0

// DT_GPIO_CTLR_BY_IDX
//    = DT_N_S_leds_S_led_0 ## _P_ ## gpios ## _IDX_ ## 0 ## _PH
//    = DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH
```
Thus, `DT_GPIO_CTLR_BY_IDX` resolves to `DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH`. We can find a macro for that token in `devicetree_generated.h`, which indeed resolves to the GPIO node’s identifier `DT_N_S_soc_S_gpio_50000000`:
```bash
grep DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH ../build/zephyr/include/generated/devicetree_generated.h
#define DT_N_S_leds_S_led_0_P_gpios_IDX_0_PH DT_N_S_soc_S_gpio_50000000
```
`DEVICE_DT_GET` takes this node identifier as its parameter `node_id`.
```c
// Original macro from zephyr/include/zephyr/device.h
#define DEVICE_DT_GET(node_id) (&DEVICE_DT_NAME_GET(node_id))
```
What can the documentation tell us about this macro?
> Returns a pointer to a device object created from a Devicetree node, if any device was allocated by a driver. If no such device was allocated, this will fail at linker time [with an error like `undefined reference to __device_dts_ord_<N>`]

## `status` Property
While our LED node has no such property, the referenced GPIO node in its `gpios` property does. The nodes are first defined in the MCU’s DTS file with the `status` property set to `"disabled"`:
`zephyr/dts/arm/nordic/nrf52840.dtsi`
```c
/ {
  soc {
    gpio0: gpio@50000000 { /* --snip-- */ status = "disabled"; port = <0>; };
    gpio1: gpio@50000300 { /* --snip-- */ status = "disabled"; port = <1>; };
  };
};
```
> “A node is considered enabled if its `status` property is either `"okay"` or not defined (i.e., does not exist in the Devicetree source). Nodes with `status` `"disabled"` are explicitly disabled. […] Devicetree nodes which correspond to physical devices must be enabled for the corresponding `struct` device in the Zephyr driver model to be allocated and initialized.” ---

We can disable the node by setting the `status` to `"disabled"` in our application’s board overlay file:
```c
&gpio0 {
    status = "disabled";
};
```
Trying to recompile the project leads to the linker error that we’ve seen in the documentation of `DEVICE_NAME_GET`:
```
/opt/nordic/ncs/v2.4.0/zephyr/include/zephyr/device.h:84:41: error:
'__device_dts_ord_11' undeclared here (not in a function);
did you mean '__device_dts_ord_15'?
   84 | #define DEVICE_NAME_GET(dev_id) _CONCAT(__device_, dev_id)
      |                                         ^~~~~~~~~
```
The documentation also mentions, that the `status` property is implicitly added with the value `"okay"` for nodes that do not define the property in the Devicetree. As we can see in the merged `zephyr.dts` file in the build folder, our `/leds/led_0` node doesn’t have the `status` property:
`build/zephyr/zephyr.dts`
```c
/ {
  leds {
    compatible = "gpio-leds";
    led0: led_0 {
      gpios = < &gpio0 0xd 0x1 >;
      label = "Green LED 0";
    };
  };
};
```
## Pin Control 
To see how pins are assigned to our UART peripheral, we need to look into our board’s DTS file. There, we find `pinctrl-<x>` properties for the `&uart0` node:
`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```c
&uart0 {
  compatible = "nordic,nrf-uarte";
  status = "okay";
  current-speed = <115200>;
  pinctrl-0 = <&uart0_default>;
  pinctrl-1 = <&uart0_sleep>;
  pinctrl-names = "default", "sleep";
};
```
**Distributed pin control**
Some MCUs, such as the nRF series from Nordic, do not restrict pin assignments, and the pin assignment is defined by each _peripheral_. E.g., the UART node selects whichever pins it uses for RXD, TXD, CTS, and RTS. The pin control is thus _distributed_ across all peripherals; there is no centralized pin multiplexer.
**Centralized pin control**
Other MCUs, such as the STM32, do not allow an arbitrary pin assignment. Instead, pins support alternative functions, managed by a centralized pin multiplexer which assigns the pin to a certain _peripheral_.
**Devicetree approach**
Whether an MCU uses centralized or distributed pin control does not necessarily have an impact on how the pin control is reflected in the Devicetree. Zephyr supports two approaches in _Devicetree_ for pin multiplexing: **Grouped** and **node** approach.
- In the **node approach** the vendor provides a DTS file containing dedicated nodes for **all** pins and the supported alternative functions. The provided nodes are referenced **unmodified** by the `pinctrl` properties of the peripheral. This is mostly used for MCUs with centralized pin control and fixed alternative functions and we’ll see a practical example when browsing the [STM32 Devicetree source files](read://https_interrupt.memfault.com/?url=https%3A%2F%2Finterrupt.memfault.com%2Fblog%2Fpractical_zephyr_05_dt_practice#node-approach-with-the-stm32).
- In the **grouped approach** the vendor’s Devicetree sources do not provide nodes for all possible combinations. Instead, nodes containing the pin configuration are created either for the board or by the application, and pins are grouped by their configuration (e.g., pull resistors), thus the name “grouped”. The pin multiplexing may or may not be restricted and thus this approach is for both, distributed and centralized pin multiplexing. We’ll see this when we browse the [Nordic Devicetree source files in the next section](read://https_interrupt.memfault.com/?url=https%3A%2F%2Finterrupt.memfault.com%2Fblog%2Fpractical_zephyr_05_dt_practice#node-approach-with-the-stm32).

---
# References
- [Practical Zephyr - Devicetree practice (Part 5) | Interrupt](https://interrupt.memfault.com/blog/practical_zephyr_05_dt_practice) 
- [Zephyr - pinctrl](https://docs.zephyrproject.org/latest/hardware/pinctrl/index.html)

#embedded #zephyr 