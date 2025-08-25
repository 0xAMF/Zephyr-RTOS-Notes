## Writing out-of-tree Modules
### `say_hello` Basic Module

```
say_hello/
├── CMakeLists.txt
├── Kconfig
├── include/
│   └── say_hello.h
├── src/
│   └── say_hello.c
└── zephyr/
    └── module.yml
```
The first thing we have to implement is the actual code of the module, for this example we implement a module that prints `hello` to the console, the code is something like:
`say_hello.h`
```C
#ifndef SAY_HELLO_H_
#define SAY_HELLO_H_

void say_hello(void);

#endif
```
`say_hello.c`
```C
#include <zephyr/kernel.h>
#include "say_hello.h"

void say_hello(void) {
    printk("Hello!\r\n");
}
```
### Defining the Module's `Kconfig` Symbol
After writing the module, now we need to define the module's `Kconfig` symbol to be able to include the module in our application.
```Kconfig
config SAY_HELLO
    bool "Print test to console"
    default n
    depends on PRINTK
    help
	adds say_hello() function to print "hello"
```
### The Module's `CMakeLists.txt`
The `CMakeLists.txt` file defines from which we include the module's header files, and define the module's source files, assuming that the module's symbol is enabled, if not we don't include anything.
```CMake
# check if the configuration is set in Kconfig
if (CONFIG_SAY_HELLO)
    # include the say_hello header for zephyr
    zephyr_include_directories(include)
    zephyr_library_sources(src/say_hello.c)

endif()
```
### `module.yaml` File
The `module.yaml` file lives under the `zephyr` subdirectory, this file is used to tell the zephyr's build system about our module.
```yaml
name: say_hello
build:
  cmake: .
  kconfig: Kconfig
```
- `name`: specify the module's name.
- `cmake`: take the path to the `CMakeLists.txt` file, `.` points to the directory where the `zephyr` directory lives, which is the module's directory.
- `kconfig`: points to our `Kconfig`.
## Using the Module
### In The app's `prj.conf` 
First we enable the module using the symbol we defined earlier:
```conf
CONFIG_SAY_HELLO=y
```
### Making `CMake` Use Our Module
Before defining the project in our `CMakeLists.txt` we include the extra module using the following syntax:
```CMake
set(ZEPHYR_EXTRA_MODULES "${CMAKE_SOURCE_DIR}/../modules/say_hello")
```
### In our `main.c` File
Now we can include the module as a header and use it.
```C
#include <zephyr/kernel.h>

#include "say_hello.h"

static const int32_t sleep_time_ms = 1000;

int main(void)
{
    while(1) {
		say_hello();
		k_msleep(sleep_time_ms);
    }
    return 0;
}
```
---
#embedded #zephyr 