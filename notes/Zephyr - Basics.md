## Zephyr Application Types
- **Freestanding Applications**: independent of zephyr location and does not have zephyr in the application file tree.
- **Workspace Applications**: uses `west` workspace, `west` is used to initialize our workspace based on a `west.yml` manifest file. `west` uses the manifest file to create complex workspaces: The location and revision are specified in the manifest file for each external dependency used in your project.
- **Zephyr Repository Applications**: the application is a part of the zephyr repository.
## A Skeleton Freestanding Application
File Structure: 
```
.
├── src
│   └── main.c
├── CMakeLists.txt
└── prj.conf
```
Zephyr has its own meta tool (`west`) that uses another meta tool (`CMake`).
## West vs. CMake
[[CMake]] is used for building applications only, while `west` is used for building, flashing, and debugging applications using the [Zephyr Extension Commands](https://docs.zephyrproject.org/latest/develop/west/zephyr-cmds.html), and its mainly used for managing multiple repositories.
## Kconfig
Our application skeleton has a `prj.conf` file which must exist in any application *even if empty*, and is used by the [Kconfig Configuration System](https://docs.zephyrproject.org/latest/build/kconfig/index.html).
 we’ll be using this configuration file to tell the build system which modules and which features we want to use in our application. Anything we don’t plan on using will be deactivated, decreasing the size of our application and reducing compilation times.
## Writing a Dummy Application
### Source Code
`main.c` 
```C
#include <zephyr/kernel.h>

void main(void)
{
    while (1)
    {
        k_msleep(100U); // Sleep for 100 ms.
    }
}
```
### Build Configuration
For this project we create a simple `CMakeLists.txt` file specifying the, project details, the target, and the board we are using.
```CMake
cmake_minimum_required(VERSION 3.20.0)

set(BOARD nrf52dk/nrf52832)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project( EmptyApp VERSION 0.1 DESCRIPTION "Empty Zephyr application." LANGUAGES C)

target_sources(app PRIVATE src/main.c)
```
Setting the `BOARD` variable can also be done using environment variable and we don't have to set it in CMake.
- Notice that we didn't specify an executable, because this is done within the zephyr CMake package. The package already defines your executable and also provides this `app` library target which is intended to be used for all application code.
### Building The Application
Using CMake
```bash
cmake -B ./build
```
Within the `./build` folder contains the application built for the `BOARD` specified in the `CMakeLists.txt` file. The following command allows passing the `BOARD` to CMake directly and thereby either overrides a given `set(BOARD ...)` instruction or provides the `BOARD` in case it hasn’t been specified at all:
```bash
cmake -B ./build -DBOARD=nrf52dk/nrf52832
```
Building with `west`
```bash
rm -fr ./build
west build -d ./build
```
If we run this we will get unknown board error, we have to specify the board using `--board=<board_name>`.
```bash
west build --board nrf52840dk/nrf52840 -d ./build
```
We can also set the `BOARD` as an environment variable and just run:
```bash
export BOARD=nrf52840dk/nrf52840
west build -d ./build
```
Once we have a build directory built using one of the previous commands, we can run `west` without passing the board name.
We can change the `west` configuration options to include a board and build without having to pass in the board in the command line.
We can list the current configuration using the following command:
```bash
west config -l
```
We can add a default board configuration to our settings as follows:
```bash
west config build.board=nrf52dk/nrf52832
```
*NOTE*: instead of deleting the `build` folder, you can also use the `--pristine` to generate a new build:
```bash
west build -d ../build --pristine # --pristine = -p
```
## Flashing and Debugging
We flash using `west flash` command specifying the `build` directory using `-d` option.
```bash
west flash -d ./build
```
- We can neglect the `-d ./build` since the `build` directory already exists in the directory we are executing `west` from.
The debugging works the same way as flashing:
```bash
west debug -d ./build
```
---
## Resources
- [Practical Zephyr - Zephyr Basics](https://interrupt.memfault.com/blog/practical_zephyr_basics)
- [Installing the nRF Connect SDK](https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/installation/install_ncs.html#)
- [Creating an application by hand - zephyr documentation](https://docs.zephyrproject.org/latest/develop/application/index.html#creating-an-application-by-hand)

#embedded #zephyr 
