## Device Trees
>  the _Devicetree_ is a tree data structure you provide to describe your hardware. Each _node_ describes one _device_, e.g., the UART peripheral that we used for logging via `printk` . Except for the root note, each node has exactly one parent, thus the term _device tree_.

We we build our project using `west`, zephyr detects the device tree of our target board automatically, we can see that in the logs:
![[Pasted image 20250731175406.png]]
When we take a look at our board device tree we see something like:
```c
#include <mem.h>

#ifndef DT_DRAM_BASE
#define DT_DRAM_BASE		0
#endif
#ifndef DT_DRAM_SIZE
#define DT_DRAM_SIZE		DT_SIZE_K(4096)
#endif
#define DT_FLASH_SIZE		DT_SIZE_K(4096)

#include <ia32.dtsi>

/ {
	model = "QEMU X86 emulator";
	compatible = "qemu,x86_emulator";

	flash0: flash@500000 {
		compatible = "soc-nv-flash";
		reg = <0x00500000 DT_FLASH_SIZE>;
	};
<----/snippet/----->
```
Zephyr uses the `C/C++` preprocessor to resolve includes - and for resolving actual `C` macros that are used within DTS files. This happens in the call to the CMake function `zephyr_dt_preprocess` in the mentioned Devicetree CMake module.
**Note:** The file extensions `.dts` and `.dtsi`. Both file extensions are Devicetree source files, but by convention the `.dtsi` extension is used for DTS files that are intended to be _included_ by other files.

>  Devicetrees in Zephyr: You start with your _board_, which uses a specific _MCU_, which has a certain _architecture_ and vendor-specific peripherals.

Each Devicetree source file can _reference_ nodes to add, remove, or modify properties for each included file. Example for our `qemu_x86.dts` virtual board device tree:
```C
/ {
	chosen {
		zephyr,console = &uart0;
		zephyr,shell-uart = &uart0;
		// -- and other peripherals --//
	};

	&uart0 {
		status = "okay";
		current-speed = <115200>;
	};
```
- With `zephyr,console` we seem to be able to tell Zephyr that we want it to use the node `uart0` for the console output and therefore `printk` statements.
- We’re also modifying the `&uart0` reference’s properties, e.g., we set the baud rate to `115200` and enable it by setting its status to “okay”.
- The `uart0` node is originally defined in the included Devicetree source file of the SoC `ia32.dtsi` , and seems to be disabled by default
```C
uart0: uart@3f8 {
	compatible = "ns16550";
	reg = <0x000003f8 0x100>;
	label = "UART_0";
	clock-frequency = <1843200>;
	interrupts = <4 IRQ_TYPE_LOWEST_EDGE_RISING 3>;
	interrupt-parent = <&intc>;

	status = "disabled";
};
```
## Compiling The Devicetree
 While the official Devicetree compiler `dtc` is definitely invoked during the build process, it is not used to generate any source code. Instead, Zephyr feeds the generated `build/zephyr/zephyr.dts.pre` into its own `GEN_DEFINES_SCRIPT` Python script, located at `zephyr/scripts/dts/gen_defines.py`.
> The Devicetree compiler `dtc` is typically used to compile Devicetree sources into a _binary_ format called the Devicetree blob `dtb`. The Linux kernel parses the DTB and uses the information to configure and initialize the hardware components described in the DTB. This allows the kernel to know how to communicate with the hardware without hardcoding this information in the kernel code. Thus, in Linux, the Devicetree is parsed and loaded during _runtime_ and thus can be _changed_ without modifying the application. 
> Zephyr, however, is designed to run on resource-constrained, embedded systems. It is simply not feasible to load a Devicetree blob during runtime: Any such structure would take up too many resources in both the Zephyr drivers and storing the Devicetree blob. Instead, the Devicetree is resolved during _compile time_.

`dtc --> zephyr.dts.pre --> gen_defines.py --> devicetree_generated.h`
[Zephyr - Device Tree Documentation](https://docs.zephyrproject.org/latest/build/dts/index.html)
![[Pasted image 20250817141632.png]]
The output of `gen_defines.py` generator:
```
-- Generated zephyr.dts: /path/to/build/zephyr/zephyr.dts
-- Generated devicetree_generated.h: /path/to/build/zephyr/include/generated/devicetree_generated.h
-- Including generated dts.cmake file: /path/to/build/zephyr/dts.cmake
```
- The build process creates three important output files: The `zephyr.dts` that has been generated out of the preprocessed `zephyr.dts.pre`, a `devicetree_generated.h` header file, and a CMake file `dts.cmake`.
- The `zephyr.dts` Devicetree source is passed to the `dtc` device tree compiler not generate warnings and errors not to generate actual binary output. This helps to reduce the complexity of the `gen_defines.py` script and ensures that the `.dts` is valid. 
- The `devicetree_generated.h` header file replaces the Devicetree blob `dtb`. It contains _macros_ for “all things Devicetree” and is included by the drivers and our application thereby strips all unnecessary or unused parts.
`build/zephyr/include/generated/devicetree_generated.h`
```
#define DT_CHOSEN_zephyr_console DT_N_S_soc_S_uart_40002000
// --snip---
#define DT_N_S_soc_S_uart_40002000_P_current_speed 115200
#define DT_N_S_soc_S_uart_40002000_P_status "okay"
```
- `DT_` is just the common prefix for Devicetree macros,
- `_S_` is a forward slash `/`,
- `_N_` refers to a _node_,
- `_P_` is a _property_.
`DT_N_S_soc_S_uart_40002000_P_current_speed` simply refers to the _property_ `current_speed` of the _node_ `/soc/uart_40002000`. This configuration value is set during _compile time_. You’ll need to recompile your application in case you want to change this property.

> The approach in Linux would be different: There, the (UART speed) property is read from the Devicetree blob `dtb` during runtime. You could change the property, recompile the Devicetree, exchange the Devicetree blob, and wouldn’t need to touch your application or the Kernel at all.

 The generated `dts.cmake` is a file that basically allows to access the entire Devicetree from within CMake, using CMake target properties, e.g., we’ll find the _current speed_ of our UART peripheral also within CMake:
`dts.cmake`
```CMake
set_target_properties(
    devicetree_target
    PROPERTIES
    "DT_PROP|/soc/uart@40002000|current-speed" "115200"
)
```
## Basic Source File Syntax
```c
/dts-v1/;
/ {/*empty*/};
```
`/dts-v1/;`: identifies the file as a version 1 Devicetree source file. 
- Without this tag, the Devicetree compiler would treat the file as being of the obsolete version _0_ - which is incompatible with the current major Devicetree version _1_ used by Zephyr.
- This is required when working with zephyr.
After the version flag we have empty device tree with only one node, which is the root node defined with the forward slash (`/`).
Within the root node we can define nodes in the form of a tree similar to a `JSON` file.
```c
/dts-v1/;

/ {
  node {
    subnode {
      /* name/value properties */
    };
  };
};
```
- A node in the Devicetree can be uniquely identified by specifying the full _path_ from the root node, through all subnodes, to the desired node, separated by forward slashes. E.g., our full path to our _subnode_ is `/node/subnode`.
Node names can also have an optional, hexadecimal _unit-address_, specified using an `@` and thus resulting in the full node name format `node-name@unit-address`. E.g., we could give our `subnode` the _unit-address_ `0123ABC` as follows:
```c
/dts-v1/;

/ {
  node {
    subnode@0123ABC {
      reg = <0x0123ABC>;
      /* properties */
    };
  };
};
```
The _unit-address_ can be used to distinguish between several subnodes of the same type. It can be a real register address, typically a base address, e.g., the base address of the register space of a specific UART interface, but also a plain instance number, e.g., when describing a multi-core MCU by using a `/cpus` node, with two instances `cpu@0` and `cpu@1` for each CPU core. Each node with a _unit-address_ **must** also have the property `reg` - and any node _without_ a _unit-address_ must _not_ have the property `reg`.
*Note*: A node name without a `unit-address` must be unique.
Sometimes in complex device trees the paths become so long, and if we want to reference a node with a long path that can be difficult. The solution for this issue is using *node labels*.
```c
/dts-v1/;

/ {
  node {
    subnode_label: subnode@0123ABC {
      reg = <0x0123ABC>;
      /* properties */
    };
  };
};
```
Now instead of using `subnode@0123ABC` we can use `subnode_label` to identify this node.
 This term is not explicitly defined in the Devicetree specification but is used extensively in Zephyr.
 *Example*: `zephyr/dts/x86/ia32.dts`
 ```c
uart0: uart@3f8 {
	compatible = "ns16550";
	reg = <0x000003f8 0x100>;
	label = "UART_0";
	clock-frequency = <1843200>;
	interrupts = <4 IRQ_TYPE_LOWEST_EDGE_RISING 3>;
	interrupt-parent = <&intc>;

	status = "disabled";
};
```
- The `unit-address` of `uart0` matches the actual base address in the datasheet for this SoC.
- The `reg` property has two values enclosed by `<..>`, the first value is the `unit-address` and the second value is the length of the address space reserved for this UART instance.
## Property Names and Basic Value Types

| Zephyr type    | DTSpec equivalent            | Example                                          |
| -------------- | ---------------------------- | ------------------------------------------------ |
| boolean        | property with no value       | interrupt-controller;                            |
| string         | string                       | status = "disabled";                             |
| array          | 32-bit integer cells         | reg = <0x40002000 0x1000>;                       |
| int            | A single 32-bit integer cell | current-speed = <115200>;                        |
| 64-bit integer | 32-bit integer cells         | value = <0xBAADF00D 0xDEADBEEF>;                 |
| uint8-array    | bytestring                   | mac-address = [ DE AD BE EF 12 34 ];             |
| string-array   | stringlist                   | compatible = "nordic,nrf-egu", "nordic,nrf-swi"; |
| compound       | “comma-separated components” | foo = <1 2>, [3 4], "five"                       |
> keep in mind that [in Zephyr all DTS files are fed into the preprocessor](read://https_interrupt.memfault.com/?url=https%3A%2F%2Finterrupt.memfault.com%2Fblog%2Fpractical_zephyr_dt#devicetree-includes-and-sources-in-zephyr) and therefore Zephyr allows the use of macros in DTS files. E.g., you might encounter properties like `max-frequency = <DT_FREQ_M(8)>;`, which do not match the Devicetree syntax at all. There, the preprocessor replaces the macro `DT_FREQ_M` with the corresponding literal before the source file is parsed.

Parentheses, arithmetic operators, and bitwise operators are allowed in property values, though the entire expression must be parenthesized.
```c
/ {
  foo {
    bar = <(2 * (1 << 5))>;
  };
};
```
## References and `phandle` Types
**Overlay Files**: additional DTS file on top of the hierarchy of files that is included starting with the board’s Devicetree source file. We can specify an extra Devicetree overlay file using the CMake variable `DTC_OVERLAY_FILE`.
*Note:* We don't have to write the version `/dts-v1/;` in overlay files.
### `phandle`
 Devicetree source files need some way to refer to nodes, something like pointers in `C`, and that’s what a `phandle` is.
- Property name `phandle`, value type _32-bit integer_.
- The `phandle` property specifies a numerical identifier for a node that is unique within the Devicetree. The `phandle` property value is used by other nodes that need to refer to the node associated with the property.
- **Note:** Most Devicetrees will not contain explicit phandle properties. The DTC tool automatically inserts the phandle properties when the DTS is compiled.
- We can use node references outside the root node to define _additional_ properties for a node or change a node’s existing properties.
```c
/ {
  label_a: node_a { /* Empty. */ };
  node_refs {
    phandle-by-path = <&{/node_a}>;
    phandle-by-label = <&label_a>;
  };
};
``` 
If we build our project with the `overlay` file, we will see something like this in the output `zephyr.dts` file:
`build/zephyr/zephyr.dts`
```c
/dts-v1/;

/ {
  label_a: node_a {
    phandle = < 0x1c >;
  };
  node_refs {
    phandle-by-path = < &{/node_a} >;
    phandle-by-label = < &label_a >;
  };
};
```
- The generator created a `phandle` property for the node we referenced `node_a`.
> “In a cell array, a reference to another node will be expanded to that node’s phandle.”
- the references `&{/node_a}` and `&label_a` in our properties `phandle-by-path` and `phandle-by-label` are essentially expanded to `node_a`’s `phandle` _0x1c_. Thus, **the reference is equivalent to its phandle**. Zephyr’s documentation is right to refer to `&{/node_a}` and `&label_a` as “`phandle`s”.
- We can manually set our phandle property in the overlay file.
```c
 label_a: node_a {
   phandle = <0xc0ffee>;
 };
```
Another common use of `phandle` is referencing a node defined somewhere else, in the board's SoC for example and modify its properties in the board `.dts` file.
`boards/st/stm32f4_disco/stm32f4_disco.dts`
```C
&i2c1 {
	clock-frequency = <I2C_BITRATE_FAST>;
	pinctrl-0 = <&i2c1_scl_pb6 &i2c1_sda_pb9>;
	pinctrl-names = "default";
	status = "okay";

	audio_codec: cs43l22@4a {
		compatible = "cirrus,cs43l22";
		reg = <0x4a>;
		reset-gpios = <&gpiod 4 0>;
	};
};
```
- Here we reference the I2C node originally defined in the SoC `.dtsi` file and we modify it for our board.
### `path`, `phandles`, and `phandle-array`

| Zephyr Type     | DTC Equivalent       | Example                                |
| --------------- | -------------------- | -------------------------------------- |
| `path`          | 32-bit integer cells | `zephyr,console = &uart0;`             |
| `phandle`<br>   | `phandle`            | `pinctrl-0 = <&uart0_default>;`        |
| `phandles`<br>  | 32-bit integer cells | `cpu-power-states = <&idle &suspend>;` |
| `phandle-array` | 32-bit integer cells | `gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;` |
*Note:* If we don’t place the reference within `<>`, the [DTSpec](https://www.devicetree.org/specifications/) defines the following behavior:
> “Outside a cell array, a reference to another node will be expanded to that node’s full path.”

So, if we remove the `<>` in the previous example and built the project, the property `phandle` in `node_a` will disappear, because we didn't reference `node_a`.

#### `phandle-array`
```c
/ {
  label_a: node_a {
    #phandle-array-of-ref-cells = <2>;
  };
  label_b: node_b {
    #phandle-array-of-ref-cells = <1>;
  };
  node_refs {
    phandle-array-of-refs = <&{/node_a} 1 2 &label_b 3>;
  };
};
```
`build/zephyr/zephyr.dts`
```c
/dts-v1/;

/ {
  /* Lots of other nodes ... */
  label_a: node_a {
    #phandle-array-of-ref-cells = < 0x2 >;
    phandle = < 0x1c >;
  };
  label_b: node_b {
    #phandle-array-of-ref-cells = < 0x1 >;
    phandle = < 0x1d >;
  };
  node_refs {
    phandle-array-of-refs = < &{/node_a} 0x1 0x2 &label_b 0x3 >;
  };
};
```

`phandle-array` is a list of phandles _with metadata_ for each phandle. This is how it works:
- By convention, a `phandle-array` property is plural and its name should thus end with “_s_”.
- The value of a `phandle-array` property is an array of phandles, but each phandle can be followed by cells (32-bit values), sometimes also called a phandle’s “metadata”. In the example above, the two values `1 2` are `&{/node_a}`’s _metadata_, whereas `3` is `&label_b`’s metadata.
- The new properties `#phandle-array-of-ref-cells` tell the compiler how many metadata _cells_ are supported by the corresponding node. Such properties are called [specifier cells](https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#specifier-cell-names-cells): In our example, `node_a` specifies that the node supports _two_ cells, `node_b`’s specifier cell only allows _one_ cell after its phandle.
_Specifier cells_ like `#phandle-array-of-ref-cells` have a defined naming convention: The name is formed by removing the plural ‘_s_’ and attaching ‘_-cells_’ to the name of the `phandle-array` property. For our property _phandle-array-of-refs_, we thus end up with _phandle-array-of-ref~~s~~**-cells**_.
- To reference exactly one node, use the `phandle` type.
- To reference _zero_ or more nodes, we use the `phandles` type.
- To reference _zero_ or more nodes **with** metadata, we use a `phandle-array`.
*Note:* if we don't follow the naming convention the `dtc` compiles with no errors, but have to define some _schema_ and add _meaning_ to all the properties and their values; we’ll have to define the Devicetree’s **semantics**.
Without semantics, the DTS generator can’t make sense of the provided Devicetree and therefore also won’t generate anything that you’d be able to use in your application.

## `phandles` vs. `phandle-array`

| Feature           | `phandles`                            | `phandle-array`                                                |
| ----------------- | ------------------------------------- | -------------------------------------------------------------- |
| **Definition**    | A list of **references (phandles)**   | A list of **phandles + arguments**                             |
| **Data**          | Only phandles                         | Phandles **with additional data** per entry                    |
| **Used When**     | You just need to refer to other nodes | You also need to pass arguments (e.g., GPIO pin number, flags) |
| **Property Type** | Simple array of phandles              | Array of phandle-argument tuples                               |
| **Example Prop**  | `clocks = <&clk1 &clk2>;`             | `gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;`                         |
| **Node Decl.**    | Requires `phandle`                    | Requires `#*-cells` (like `#gpio-cells`)                       |
#### `phandle-array` example
`zephyr/dts/arm/nordic/nrf52840.dtsi`
```c
/ {
  soc {
    gpio0: gpio@50000000 {
      compatible = "nordic,nrf-gpio";
      gpio-controller;
      reg = <0x50000000 0x200 0x50000500 0x300>;
      #gpio-cells = <2>;
      status = "disabled";
      port = <0>;
    };

    gpio1: gpio@50000300 {
      compatible = "nordic,nrf-gpio";
      gpio-controller;
      reg = <0x50000300 0x200 0x50000800 0x300>;
      #gpio-cells = <2>;
      ngpios = <16>;
      status = "disabled";
      port = <1>;
    };
  };
};
```
`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`

```c
/ {
  leds {
    compatible = "gpio-leds";
    led0: led_0 {
      gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
      label = "Green LED 0";
    };
  };
};
```
The nodes `gpio0` and `gpio1` both contain the _specifier cells_ `#gpio-cells` which indicate that we need to pass exactly two _cells_ to use the node in a `phandle-array`. In the `led0`’s property `gpios` of type `phandle-array` we can see that we do exactly that: We use the two cells to specify the _pin_ and _flags_ that we’re using for the LED.
## `/aliases`
`/aliases` node is a child node of the root node. The following is specified for its properties:

> Each property of the `/aliases` node defines an alias. The property _“name”_ specifies the _alias name_. The property _“value”_ specifies the _full **path** to a node in the Devicetree_. [DTSpec](https://www.devicetree.org/specifications/)

```c
/ {
  aliases {
    alias-by-label = &label_a;
    alias-by-path = &{/node_a};
    alias-as-string = "/node_a";
  };
  label_a: node_a {
    /* Empty. */
  };
};
```
`/aliases` are just yet another way to get the full path to nodes in your application.
**NOTE**: For the application, it does not matter whether you use an alias, label or a full path, but inside the device tree *we can't replace labels with aliases*, the device tree will not compile if we do that.
```c
/ {
  aliases {
    alias-by-label = &label_a;
  };
  label_a: node_a {
    /* Empty. */
  };
  node_ref {
    // This doesn't work. An alias cannot be used like labels.
    phandle-by-alias = <&alias-by-label>;
  };
};
```
An important use of aliases is making sure that different labels/properties in different boards can be understood by the `dtc` the same way, giving an alias for each different property ensuring flexibility.
`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```c
/ {
  aliases {
    led0 = &led0;
    /* ... */
    pwm-led0 = &pwm_led0;
    sw0 = &button0;
    /* ... */
  };
};
```

`zephyr/boards/arm/nucleo_c031c6/nucleo_c031c6.dts`
```c
/ {
  aliases {
    led0 = &green_led_4;
    pwm-led0 = &green_pwm_led;
    sw0 = &user_button;
    /* ... */
  };
};
```
**Note:** In case the aliases don’t exist in the board DTS files, you could use overlay files
## `/chosen`
`zephyr/boards/x86/qemu_x86/qemu_x86.dts`
```c
chosen {
	zephyr,sram = &dram0;
	zephyr,flash = &flash0;
	zephyr,console = &uart0;
	zephyr,shell-uart = &uart0;
	zephyr,bt-uart = &uart1;
	zephyr,uart-pipe = &uart1;
	zephyr,bt-mon-uart = &uart1;
	zephyr,code-partition = &slot0_partition;
	zephyr,flash-controller = &sim_flash;
};
```

> The `/chosen` node does not represent a real device in the system but describes parameters chosen or specified by the system firmware at run time. It shall be a child of the root node. [DTSpec](https://www.devicetree.org/specifications/)

The first sentence can be a bit misleading: It doesn’t mean that we cannot refer to “real devices” using `/chosen` properties, it simply means that a device defined as a `/chosen` property is always a _reference_. Thus, in short, `/chosen` contains a list of _system parameters_.
- **Note:** In Zephyr, `/chosen` is only used at build-time. There is no run-time feature.
-  Zephyr uses `/chosen` properties exclusively for references to other _nodes_ and therefore the `/chosen` node only contains properties of the type `path`. Also, the `DT_CHOSEN` macro in the Devicetree API is only used to retrieve node identifiers.
-  If you want to specify a _node_ that is independent of the nodes in a Devicetree, you should use an _alias_ rather than a chosen property.
-  `/chosen` is used to specify global configuration options and properties that affect the system as a whole.
-  In Zephyr, `/chosen` is typically only used for [Zephyr-specific parameters](https://docs.zephyrproject.org/latest/build/dts/api/api.html#devicetree-chosen-nodes) and not for application-specific configuration options.
## Zephyr DTS Skeleton and Addressing
Our DTS files include a a skeleton file `skeleton.dtsi`, which contain a minimal set of nodes and properties.
```
nrf52840dk_nrf52840.dts
└── nordic/nrf52840_qiaa.dtsi
    └── nordic/nrf52840.dtsi
        └── arm/armv7-m.dtsi
            └── skeleton.dtsi
```
`zephyr/dts/common/skeleton.dtsi`
```c
/ {
  #address-cells = <1>;
  #size-cells = <1>;
  chosen { };
  aliases { };
};
```
They provide the necessary _addressing_ information for nodes that use a _unit-addresses_.
For example:
```c
/ {
  soc {
    uart0: uart@40002000 { reg = <0x40002000 0x1000>; };
    uart1: uart@40028000 { reg = <0x40028000 0x1000>; };
  };
};
```
A 64-bit integer value is represented using two cells, thus `<0x40002000 0x1000>` could technically be a single 64-bit integer, and _length_ could be omitted. To be _really_ sure how `uart@40002000` is addressed, we need to look at the parent node’s `#address-cells` and `#size-cells` properties. So what are those properties?
 Property names `#address-cells`, `#size-cells`, value type _32-bit integer_.
 The `#address-cells` and `#size-cells` properties describe how child device nodes should be addressed.
 - The `#address-cells` property defines the number of _32-bit integers_ used to encode the **address** field in a child node’s `reg` property.
 - The `#size-cells` property defines the number of _32-bit integers_ used to encode the **size** field in a child node’s `reg` property.
-   Having `#address-cells` and `#size-cells` defined with `<1>`, for our `uart@40002000` node, the `reg`’s value `<0x40002000 0x1000>` indeed refers to the _address_ `0x40002000` and the _length/size_ `0x1000`. This is also not surprising since the nRF52840 uses **32-bit architecture**.
We can also find an example of a **64-bit architecture** in Zephyr’s DTS files:
`zephyr/dts/riscv/sifive/riscv64-fu740.dtsi`
```c
/ {
  soc {
    #address-cells = <2>;
    #size-cells = <2>;

    uart0: serial@10010000 {
      reg = <0x0 0x10010000 0x0 0x1000>;
    };
  };
};
```
---

- [Practical Zephyr - Devicetree basics (Part 3) | Interrupt](https://interrupt.memfault.com/blog/practical_zephyr_dt)
- [Device Tree Specifications](https://www.devicetree.org/specifications/)
- [Zephyr - Device Tree Documentation](https://docs.zephyrproject.org/latest/build/dts/index.html)
- [Zephyr Development Summit - Devicetrees](https://www.youtube.com/watch?v=w8GgP3h0M8M&list=PLzRQULb6-ipFDwFONbHu-Qb305hJR7ICe)

#embedded #zephyr #linux 