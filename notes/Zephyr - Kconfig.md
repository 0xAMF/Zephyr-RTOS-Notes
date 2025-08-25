> Kconfig is a configuration system that uses a hierarchy of configuration _files_ (with their own syntax), which in turn define a hierarchy of configuration _options_, also called _symbols_. The _build system uses these symbols_ to include or exclude files from the build process and as configuration options in the _source code_ itself. All in all, this allows you to modify your application without modifying the source code.

**Note**
- Kconfig controls the firmware not the hardware, it determines which drivers we include in the firmware we flash on the board, it does not enable or disable hardware, it ignores its existence.
- Devicetrees controls the hardware and can enable or disable a hardware feature.
## Configuring Symbols
Each subsystem in zephyr defines some kconfig symbols that allows you to enable or disable the feature that is related to symbol.
*Example:*
The debugging subsystem defines the `PRINTK` symbol, which we can enable or disable, the `PRINTK` symbol is defined in the `Kconfig` file in `zephyr/subsys/debug/Kconfig`.
```kconfig
config PRINTK
	bool "Send printk() to console"
	default y
```
The `PRINTK` symbol is enabled by default, we can configure this to our needs in the application configuration file `prj.conf`.
Symbols are assigned their values using the following syntax:
```
CONFIG_<symbol name>=<value>
```
*Note:* There should be no spaces around the equals sign.
`PRINTK` is a `bool` symbol that can be enabled or disabled using `y` or `n`, so if we want to disable the `PRINTK` symbol we use the following syntax.
```
CONFIG_PRINTK=n
```
We can also disable a symbol by commenting it out.
```yml
# CONFIG_PRINTK=n
```
Setting this symbol to `n` or commenting it out controls whether we include the code of this feature or not through `#ifdef` directive.
`zephyr/lib/os/printk.h`
```C
#ifdef CONFIG_PRINTK
extern void printk(const char *fmt, ...);
#else
static inline void printk(const char *fmt, ...)
{
    ARG_UNUSED(fmt);
}
```
`zephyr/lib/os/printk.c`
```C
#ifdef CONFIG_PRINTK
void printk(const char *fmt, ...)
{
    // ...
}
#endif /* defined(CONFIG_PRINTK) */
```
After finishing our configurations we can use `west` to build and run using the following command:
```bash
west build --board <board name> --build-dir ../build --pristine
west flash --build-dir ../build
```

> [!NOTE] Note
> the `--pristine` option to enforce a complete rebuild of your application. Typically, this is needed when adding new build or configuration files.

Knowing that  _Kconfig_ uses an entire _hierarchy_ of `Kconfig` files. The initial configuration is merged from several `Kconfig` files, including the `Kconfig` file of the specified board.
- If we build and flash we see that `printk` still works and our changes didn't affect the application, this is because enabling one option can enable another one and `Kconfig` does not warn about conflicting types, so somewhere in a `Kconfig` file a symbol that depends on `printk` enabled it.
 - All _Kconfig_ symbols are merged into a single `zephyr/.config` file located in the build directory.
`build/zephyr/.config`
```
# CONFIG_DEBUG is not set
# CONFIG_STACK_USAGE is not set
# CONFIG_STACK_SENTINEL is not set
CONFIG_PRINTK=y
```
Zephyr contains hundreds of `Kconfig` fragments, and it is impossible to navigate all of them.
This where `west` integrates *`menuconfig`* and *`guiconfig`*.
## Loading `menuconfig` and `guiconfig`
For the `build` command, _West_ has some builtin _targets_, two of which are used for _Kconfig_:
The targets `menuconfig` and `guiconfig` can be used to start graphical _Kconfig_ tools which allow modifying the `.config` file in your build directory.
```bash
west build --build-dir ../build -t menuconfig
west build --build-dir ../build -t guiconfig
```
if we run the `menuconfig` command we will get a TUI that looks like this:
![[Pasted image 20250730141929.png]]
to search for a symbol we use `/`.
![[Pasted image 20250730142013.png]]
after searching for `PRINTK` we can find it with `-*-`, that means we can't change because it depends on other symbol.
If we press `?` we can see the details of the symbol.
![[Pasted image 20250730142138.png]]
We can see that the symbol `NCS_BOOT_BANNER` is `y` selecting this symbol, that is why we when we set our `PRINTK` to `n` nothing happened.
**Notice:** If you did not build your project or the build system cannot build the required targets due to CMake/Kconfig processing issues, the tool will not load since the `zephyr/.config` file is missing.
## Build Types and Alternative Kconfig Files
Different build types in Zephyr are supported using [alternative `Kconfig` files](https://docs.zephyrproject.org/latest/develop/application/index.html#basics), specified using the [`CONF_FILE`](https://docs.zephyrproject.org/latest/develop/application/index.html#important-build-system-variables) build system variable
For example: if we want to a release build we create a release kconfig file which defines the symbols for a release build type.
```
.
├── src
│   └── main.c
├── CMakeLists.txt
├── prj.conf
└── prj_release.conf
```
inside the `prj_release.conf` we define the symbols for the release build type, we pass this file as parameter in the build command.
```bash
rm -rf ../build
west build --board nrf52840dk/nrf52840 -d ./build -- -DCONF_FILE=prj_release.conf
```
Zephyr accepts build types by specifying an alternative `Kconfig` file that typically uses the file name format `prj_<build>.conf`, using the `CONF_FILE` build system variable.
## Board Specific Kconfig Fragments
 Board fragments are placed in the `boards` directory in the project root (next to the `CMakeLists.txt` file) and use the `<board>.conf` name format.
directory hierarchy:
```
.
├── boards
│   └── <board>.conf
├── src
│   └── main.c
├── CMakeLists.txt
├── prj.conf
└── prj_release.conf
```
`--pristine` build is required at this point because otherwise, the build system does not pick up our newly added `.conf` file. This is one of the very few occasions where a pristine build is actually required.
```bash
west build --board <board> -d ./build --pristine
```
If we try to build using `prj_release.conf` configuration the `<board>.conf` will be ignored, so if we want our board configurations in the release build we have to create a `<board>_<build>.conf` file, for example `nrf52840dk_nrf52840_release.conf`.
Now, when we rebuild the project, we see that our board fragment is merged into the `Kconfig` configuration, and no warning is issued by `west build`:
```bash
rm -rf ../build
west build --board nrf52840dk_nrf52840 -d ../build -- -DCONF_FILE=prj_release.conf
```
## Extra Configuration Files
`EXTRA_CONF_FILE`. This build system variable accepts one or more additional _Kconfig_ fragments. This option can be useful, e.g., to specify additional configuration options used by multiple build types (normal builds, “release” builds, “debug” builds) in a separate fragment. An example could be adding TLS configs with a `tls.conf` or adding external NOR configs with `qspi_nor.conf`
We can now pass the two extra configuration fragments to the build system using the EXTRA_CONF_FILE variable. The paths are relative to the project root and can either be separated using semicolons or spaces:
```bash
rm -rf ../build
west build --board <board> -d ../build -- \
  -DEXTRA_CONF_FILE=<extra_configs>.conf
```
The fragments specified using the `EXTRA_CONF_FILE` variable are merged into the final configuration in the given order. E.g., we can create a `release` build and reverse the order of the fragments:
```bash
rm -rf ../build
west build --board <board> -d ../build -- \
  -DCONF_FILE="prj_release.conf" \
  -DEXTRA_CONF_FILE=<extra_configs>
```
## Kconfig Hardening
The kconfig hardening tool is one of the targets of `west` just like `menuconfig` and `guiconfig`. It is executed using the `-t hardenconfig` option when using `west build`.
The hardening tool checks the _Kconfig_ symbols against a set of known configuration options that should be used for a secure Zephyr application. It then lists all differences found in the application.
```bash
# testing normal build
west build --board <board> \
  -d ./build \
  --pristine \
  -t hardenconfig
```
Output:
![[Pasted image 20250730205742.png]]
## Custom Kconfig Symbols
CMake automatically detects a `Kconfig` file if it is placed in the same directory of the application’s `CMakeLists.txt`, and that is what we’ll use for our own configuration file.
Basic `Kconfig` file for a custom symbol:
```Kconfig
mainmenu "Customized Menu Name"
source "Kconfig.zephyr"

menu "Application Options"
config USR_FUN
	bool "Enable usr_fun"
	default n
	help
	  Enables the usr_fun function.
endmenu
```
`mainmenu` defines a custom text to be the main menu of our `Kconfig`.
`source` includes the top level zephyr `Kconfig` file `zephyr/Kconfig.zephyr` and all of its symbols.
This is necessary since we’re effectively replacing the `zephyr/Kconfig` file of the Zephyr base that is usually parsed as the first file by _Kconfig_.
## Configuring Application Build Using Kconfig
Under the `src` directory we add two new files `src/usr_func.c` and `src/usr_func.h`, we define in them a simple function that prints out a message.
```C
// Content of usr_fun.h
#pragma once

void usr_fun(void);
```

```C
// Content of usr_fun.c
#include "usr_fun.h"
#include <zephyr/kernel.h>

void usr_fun(void)
{
    printk("Message in a user function.\n");
}
```
Now we let CMake know about these files, and instead of including these files with each build, we can use our Kconfig symbol `USR_FUN` to only include the files only when the symbol is enabled.
```CMake
# in CMakeLists.txt
target_sources_ifdef(
    CONFIG_USR_FUN
    app
    PRIVATE
    src/usr_fun.c
)
```
`target_sources_ifdef` conditionally includes our files in the build in case `CONFIG_USR_FUN` is defined. Since the symbol `USR_FUN` is _disabled_ by default, our build currently does not include `usr_fun`.
### Using The Sources in `main.c`
If we include the the `usr_fun.h` library and use its functions while having `CONFIG_USR_FUN` set to `n`, this could result in a linker error, because the files are not included in the build, so we have to surround the code in which we use `usr_fun` in with `#ifdefs`, for example:
```C
#include <zephyr/kernel.h>

#ifdef CONFIG_USR_FUN
#include "usr_fun.h"
#endif

#define SLEEP_TIME_MS 100U

void main(void)
{
    printk("Message in a bottle.\n");

#ifdef CONFIG_USR_FUN
    usr_fun();
#endif

    while (1)
    {
        k_msleep(SLEEP_TIME_MS);
    }
}
```
Now our build succeeds whether we include the symbol or not.
### Why Using `#ifdef` Works ?
by default, Zephyr enables the _CMake_ variable [`CMAKE_EXPORT_COMPILE_COMMANDS`](https://cmake.org/cmake/help/latest/variable/CMAKE_EXPORT_COMPILE_COMMANDS.html). The compiler command for `main.c` is thus captured by the `compile_commands.json` in our build directory:
```json
{
  "directory": "/path/to/build",
  "command": "/path/to/bin/arm-zephyr-eabi-gcc --SNIP-- -o CMakeFiles/app.dir/src/main.c.obj -c /path/to/main.c",
  "file": "/path/to/main.c"
},
```
Within the large list of parameters passed to the compiler, there is also the `-imacros` option specifying the `autoconf.h` Kconfig header file:
```bash
-imacros /path/to/build/zephyr/include/generated/autoconf.h
```
check `-imacros`: who generates autoconf.h
This header file contains the configured value of the `USR_FUN` symbol as a macro:
```C
// --snip---
#define CONFIG_USR_FUN 1
```

---
- [zephyr project - kconfig](https://docs.zephyrproject.org/latest/build/kconfig/index.html#configuration-system-kconfig)
- [Practical Zephyr - Kconfig (Part 2) | Interrupt](https://interrupt.memfault.com/blog/practical_zephyr_kconfig)
- [linux kernel - kconfig language](https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html)

#embedded #zephyr #linux 