## Devicetree Overlays
The build system combines the board’s `.dts` file and any `.overlay` files by concatenating them, with the overlays put last. Thus, the contents of the `.overlay` file have priority over any definitions in the board’s `.dts` file or its includes.
### Automatic Overlays
The Zephyr build system automatically picks up additional _overlays_ based on their location and file name.
The search performed by the CMake module `zephyr/cmake/modules/dts.cmake`:
- In case the CMake variable `DTC_OVERLAY_FILE` is set, the build system uses the specified file(s) and stops the search.
- If the file `boards/<BOARD>.overlay` exists in the application’s root directory, the build system selects the provided file as overlay and proceeds with the following step.
- If a specific revision has been specified for the `BOARD` in the format `<board>@<revision>` and `boards/<BOARD>_<revision>.overlay` exists in the application’s root directory, this file is used in _addition_ to `boards/<BOARD>.overlay`, if both exist.
- If _overlays_ have been encountered in any of the previous steps, the search stops.
- If no files have been found and `<BOARD>.overlay` exists in the application’s root directory, the build system uses the overlay and stops the search.
- Finally, if none of the above overlay files exist but `app.overlay` exists in the application’s root directory, the build system uses the overlay.
On top of the _overlay_ files that have or haven’t been discovered by the build process, the CMake variable `EXTRA_DTC_OVERLAY_FILE` allows to specify additional _overlay_ files that are added regardless of the outcome of the overlay search.
**NOTE**: Devicetree overlay files that were detected _last_ have the _highest_ precedence, since they may overwrite anything in the previously added overlay files.

> If you provide the CMake variable `DTC_OVERLAY_FILE` in your build, the board overlays will no longer be picked up automatically. For quick builds that might be helpful, but in case you’re building an application that should run on multiple boards, you should **not** use `DTC_OVERLAY_FILE` but maybe rather list additional overlays using `EXTRA_DTC_OVERLAY_FILE`

- `.overlay` files can be found in the zephyr core commonly as shields, but sometimes they can be used for board-specific modification.
## Matching `compatible` Bindings
One of the differences between `/soc/uart@40002000` and our `/node_with_props` is that `/soc/uart@40002000` has a **compatible** property.
> The `compatible` property value consists of one or more strings that define the specific programming model for the device. This list of strings should be used by a client program for device driver selection. The property value consists of a concatenated list of null-terminated strings, from most specific to most general. They allow a device to express its compatibility with a family of similar devices

`compatible` is a list of strings, where each string is essentially a reference to some _model_. The [DTSpec](https://www.devicetree.org/specifications/) uses a specific term for such models: They are called **bindings**.
```c
i2c0: i2c@40003000 {
    compatible = "nordic,nrf-i2c";
    reg = <0x40003000 0x1000>;
};
```
`compatible = "nordic,nrf-i2c"` tells Zephyr:
- This node should be **handled by the `nordic,nrf-i2c` binding**.
- This binding maps to a **C driver** and provides **metadata** for code generation.
**NOTE**
- The Devicetree compiler won’t complain if we assign a `string` value to the property `int`.
- While the property names `int` or `string` are not very revealing as such, the same is also true for the `current-speed` property of the `/soc/uart@40002000` node.
- By providing _compatible bindings_, we’re telling the Devicetree compiler to check whether the given node really matches the properties and types defined in a _binding_. Bindings also add **semantics to a Devicetree** by giving properties a _meaning_.
_Bindings_ live outside the Devicetree. The [DTSpec](https://www.devicetree.org/specifications/) doesn’t specify any file format or syntax that is used for bindings. Zephyr, like [Linux](https://docs.kernel.org/devicetree/bindings/writing-schema.html), uses `.yaml` files for its bindings.
### Why Properties of a Node Without `compatible` Are Not in `devicetree_generated.h`
**Reason:**  
Zephyr's devicetree system only processes and generates C macros for nodes that have a valid `compatible` string and an associated `.yaml` binding file. If a node lacks `compatible`, Zephyr ignores it during the device tree code generation phase.
## Bindings in Zephyr
Zephyr recursively looks for Devicetree bindings (`.yaml` files) in `zephyr/dts/bindings`. Bindings are matched against the strings provided in the `compatible` property of a node.
`zephyr/dts/bindings/serial/nordic,nrf-uart-common.yaml`
**Note**
- If we have no bindings are specified for a `.dts` file its properties will not be present in the `devicetree_generated.h` macros.
- The one exception is that the build system will always generate macros for standard properties, like [reg](https://docs.zephyrproject.org/latest/build/dts/intro-syntax-structure.html#dt-important-props), whose meaning is defined by the devicetree specification. This happens regardless of whether the node has a matching binding or not.
```yaml
include: [uart-controller.yaml, pinctrl-device.yaml]

properties:
    reg:
      required: true

    interrupts:
      required: true

    disable-rx:
      type: boolean
      description: |
        Disable UART reception capabilities (only required to disable reception
        if CONFIG_PINCTRL is enabled).

    current-speed:
      description: |
        Initial baud rate setting for UART. Only a fixed set of baud
        rates are selectable on these devices.
      enum:
        - 1200
        - 2400
        - 4800
        - 9600
	## ---------- ##
```
Here, Nordic seems to restrict the allowed baud rates using an enumeration: We can only specify values from the given list.
To find out the property’s type, we need to step further down the include tree, into `uart-controller.yaml`. This is Zephyr’s base model for UART controllers, which is used regardless of the actual vendor:
`zephyr/dts/bindings/serial/uart-controller.yaml`
```yaml
include: base.yaml

bus: uart

properties:
  # --snip--
  current-speed:
    type: int
    description: Initial baud rate setting for UART
  # --snip--
```
Now, we finally know that `current-speed` is of type `int` and is used to configure the initial baud rate setting for UART (though the description fails to mention that the baud rate is specified in _bits per second_).
Combining this with the information in `nordic,nrf-uart-common.yaml`, we know that we can only select from a pre-defined list of baud rates and cannot specify our own custom baud rate - at least not in the Devicetree. Given this _binding_, the Devicetree compiler now rejects any but the allowed values, and it is therefore not possible to specify a _syntactically_ correct value that is not an _integer_ of the given list.
## Bindings Directory
Just like the `dts/bindings` in _Zephyr’s_ root directory, the [build process](https://docs.zephyrproject.org/latest/build/dts/intro-input-output.html) also picks up any bindings in `dts/bindings` in the _application’s_ root directory.
## Naming
> “The `compatible` string should consist only of lowercase letters, digits, and dashes, and should start with a letter. A single comma is typically only used following a vendor prefix. Underscores should not be used.” [DTSpec](https://www.devicetree.org/specifications/)

Example:
`dts/bindings/custom,props-basics.yaml`
```c
description: Custom properties
compatible: "custom,props-basic"

properties:
  int:
    type: int
  string:
    type: string
```
bindings define a node’s properties under the key _properties_. The template given below is the simplest form for a property in a binding:
```c
properties:
  <property-name>:
    type: <property-type>
    # required: false -> omitted by convention if false
```
`dts/playground/props-basics.overlay`
```c
/ {
  node_with_props {
    compatible = "custom,props-basic"
    int = <1>;
    string = "foo";
  };
};
```
After building, we see that our properties as `#defines` present in the `devicetree_generated.h` output file, which didn't happen before we added the `compatible` line.
## `phandle-array` in Zephyr
`zephyr/dts/arm/nordic/nrf52840.dtsi`
```c
/ {
  soc {
    gpio0: gpio@50000000 {
      compatible = "nordic,nrf-gpio";
      #gpio-cells = <2>;
      port = <0>;
    };

    gpio1: gpio@50000300 {
      compatible = "nordic,nrf-gpio";
      #gpio-cells = <2>;
      port = <1>;
    };
  };
};
```
When we need to specify a specific pin in gpio, we can't do that with `phandle`, we have to use `phanle-array` to be able to pass extra parameters like the pin number or the state of the pin.
`zephyr/boards/arm/nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts`
```c
/ {
  leds {
    led0: led_0 {
      gpios = <&gpio0 13 GPIO_ACTIVE_LOW>;
    };
  };
};
```
What we haven’t seen yet, is how we can know that the first cell _13_ is the _pin_ within `gpio0`, and the second cell _GPIO_ACTIVE_LOW_ is a configuration _flag_. This is part of the _semantics_ defined by the _binding_ of the _gpio_ nodes that are compatible with `nordic,nrf-gpio`:
`zephyr/dts/bindings/gpio/nordic,nrf-gpio.yaml`
```yaml
description: NRF5 GPIO node
compatible: "nordic,nrf-gpio"
# --snip--

gpio-cells:
  - pin
  - flags
```
Here we see that `gpio-cells` consists of two items `pin` and `flags`, which are matched exactly against the provided metadata in the given order. Thus, _13_ refers to the `pin` and `GPIO_ACTIVE_LOW` to its `flags`.
You may have noticed that the `gpio` nodes do not conform to the naming convention that we’ve seen in the last article. This makes it a bit awkward to explain what needs to be provided for a `phandle-array` and the referenced nodes. Therefore, we’ll build our own from scratch.
> **Note:** Have a look at [Zephyr’s documentation about specifier cells](https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#specifier-cell-names-cells) in case you want to know how `gpios` opts out of the mentioned naming convention.
### A `phandle-array` from scratch
- By convention, a `phandle-array` property is plural and its name must thus end in _s_.
- The value of a `phandle-array` property is an array of phandles, but each phandle is followed by the pre-defined number of cells for each referenced node.
- The number of cells that can follow a node’s reference is specified by the node’s _specifier cells_ property
- The _specifier cells_ property has a defined naming convention: The name is formed by removing the plural ‘_s_’ and attaching ‘_-cells_’ to the name of the `phandle-array` property: `#<name-without-s>-cells`
`dts/playground/props-phandles.overlay`
```c
/ {
  label_a: node_a { /* Empty. */ };
  label_b: node_b { /* Empty. */ };
  node_refs {
    compatible = "custom-props-phandles";
    /* --snip-- */
    phandle-array-of-refs = <&{/node_a} 1 2 &label_b 1>;
  };
};
```
Within our binding for `custom-props-phandles`, we can now add the property `phandle-array-of-refs` of type `phandle-array`:
`dts/bindings/custom-props-phandles.yaml`
```yaml
description: Custom properties
compatible: "custom-props-phandles"

properties:
  # --snip--
  phandle-array-of-refs:
    type: phandle-array
```
With that, we fulfill our first rule, namely that our property name ends with an ‘s’. If we violate this rule, e.g., by using `phandle-array-of-ref` as the property name, the Devicetree compiler would reject the binding with the following error:
```
-- Found devicetree overlay: dts/playground/props-phandles.overlay
devicetree error: 'phandle-array-of-ref' in 'properties:'
in /path/to/dts/bindings/custom-props-phandles.yaml has type 'phandle-array'
and its name does not end in 's', but no 'specifier-space' was provided.
```
## Deleting Properties
We’ve learned that a boolean is set to _true_ if it exists in the Devicetree, otherwise, it is _false_. Let’s assume that a boolean property exists in some include file, how could you set this property to _false_ in your overlay? You’d somehow need to be able to tell the compiler to _remove_ the property since that is the only way to set it to _false_.
, we can _delete_ the property after the node’s declaration using `/delete-property/`. We’ll do this for both, `existent-boolean` and `string`, as follows:
`dts/playground/props-basics.overlay`
```c
/ {
  label_with_props: node_with_props {
    compatible = "custom-props-basics";
    existent-boolean;
    /* --snip-- */
  };
};

&label_with_props {
  /delete-property/ existent-boolean;
  /delete-property/ string;
}
```
## Zephyr's Device Tree API
### Token Pasting
 In its simplest form, tokens are pasted or concatenated using the token-pasting operator `##`
 **Token pasting** (also called token concatenation) is a technique used in C and C++ macros via the `##` operator in the preprocessor. It allows you to **combine (paste) two tokens into one** during macro expansion.
##### Basic Syntax
```C
#define PASTE(a, b) a##b
```
This pastes `a` and `b` into a single token.
**Example**:
```C
#define MAKE_VAR(name) int my_##name = 0;
MAKE_VAR(counter)
```
After preprocessing, this expands to:
```C
int my_counter = 0;
```
### Node Identifiers
Zephyr provides the `DT_ALIAS` macro to get a node’s identifier via its alias:
```c
// Node identifier by /aliases node.
// DT_ALIAS(alias) = DT_N_ALIAS_ ## alias
#define NODE_PROPS_ALIAS_BY_LABEL   DT_ALIAS(alias_by_label)
#define NODE_PROPS_ALIAS_BY_PATH    DT_ALIAS(alias_by_path)
#define NODE_PROPS_ALIAS_BY_STRING  DT_ALIAS(alias_as_string)
```
![[Pasted image 20250802143237.png]]
This leaves us with node labels and retrieving a node’s identifier by its path. Zephyr provides the two macros `DT_NODELABEL` and `DT_PATH` for this:
```C
// Node identifier by label.
// DT_NODELABEL(label) = DT_N_NODELABEL_ ## label
#define NODE_PROPS_BY_LABEL DT_NODELABEL(label_with_props)

// Node identifier by path.
// DT_PATH(...) = DT_N_ pasted with S_<node> for all nodes in the path.
#define NODE_PROPS_BY_PATH  DT_PATH(node_with_props)
```
The `DT_NODELABEL` is again a simple token pasting of `DT_N_NODELABEL_` and the provided _label_, resulting in the macro `DT_N_NODELABEL_label_with_props` that we’ve seen in the section about [labels and paths](read://https_interrupt.memfault.com/?url=https%3A%2F%2Finterrupt.memfault.com%2Fblog%2Fpractical_zephyr_dt_semantics#labels-and-paths), which again resolves to the node identifier `DT_N_S_node_with_props`.
The macro `DT_PATH` is a _variadic_ macro that allows retrieving a node’s identifier using its full path. The path to a node is specified by the sequence of nodes, starting at the root node `/`. Each argument is thus a _node identifier_; the root node `/` is omitted. The Devicetree API simply pastes `DT_N` and `_S_<node>` for each node (in the “lowercase-and-underscores” form) in the path, resulting in the _node identifier_ `DT_N_S_node_with_props`.
**Note:** The path `/soc/uart@40002000` is an example of a node that is two levels deep, obtained using `DT_PATH(soc, uart_40002000)`. Notice that it is **not** possible to use node labels in paths.
### Property Values
Using the [node identifiers](read://https_interrupt.memfault.com/?url=https%3A%2F%2Finterrupt.memfault.com%2Fblog%2Fpractical_zephyr_dt_semantics#node-identifiers) and the property’s name in the “lowercase-and-underscores” form, we can access the property’s value using the macro `DT_PROP`. The macro is very straight-forward since all it needs to do is paste the node ID, separated by `_P_` with the provided property name.

---
- [Practical Zephyr - Devicetree semantics (Part 4) | Interrupt](https://interrupt.memfault.com/blog/practical_zephyr_dt_semantics)

#embedded #zephyr 