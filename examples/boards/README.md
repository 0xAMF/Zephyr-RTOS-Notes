## Creating Custom Board For Different Architectures
How you write the board porting files, especially the `.dts` files depends on what the SoC requires, so if you want to port a board different than the one i used in this example (stm32f407 discovery), search for boards that use the same SoC as the board you want to support.

### Notes on Porting ARM and STM32 Boards
- Since the stm32f4 is based on ARM Cortex-M4, we have to look for boards that use the same architecture to get an idea on how to set things up.
- What's different in ARM is that you need to set up clocks and the RCC peripheral before doing anything.
- In pinctrl `.dtsi` file, we use the `STM32_PINMUX` macro to define pins for our peripherals, which is a unique macro used for STM32 boards SoCs only.
- In `Kconfig.<board_name>` you have to check if the SoC macro already exists, if not, you have to port the SoC first (don't do that, the SoC probably exists).
- If you want to add flash and debug support check for similar boards' `board.cmake` file, but if you are using openocd, make sure to pass the `.cfg` configuration files required to debug your board.
    - You can either pass the `.cfg` files for your board directly if that's possible or you can pass the debugger (stlink-v2 for example) and the board's SoC.
    - The configuration for openocd is added inside the `board.cmake` using the `--config` option.
    ```cmake
    board_runner_args(openocd "--config" "board/stm32f4discovery.cfg")
    ```
    - here i'm passing the `stm32f4discovery.cfg` configuration since i'm using the stm32f407 board.
    - If openocd doesn't have a configuration for your board, the set up will be something like this:
    ```cmake
    board_runner_args(openocd "--config" "interface/your_interface.cfg" "--config" "target/your_target.cfg")
    ```
    - The target is the SoC you are targeting.
    - The interface is the hardware debugger your are using.


