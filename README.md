# Zephyr RTOS Notes

## Contents

### Fundamentals
- [Basics](./notes/Zephyr%20-%20Basics.md)
  - **Example**: [Hello World](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/hello_world) 
- [Kconfig](./notes/Zephyr%20-%20Kconfig.md)
- [Device Tree Basics](./notes/Zephyr%20-%20Device%20Tree%20Basics.md)
- [Device Tree Semantics](./notes/Zephyr%20-%20Device%20Tree%20Semantics.md)
- [Device Tree Practice](./notes/Zephyr%20-%20Device%20Tree%20Practice.md)

### Core Features
- [ADC](./notes/Zephyr%20-%20ADC.md)
  - **Example**: [ADC Demo](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/adc_demo) 
- [Modules](./notes/Zephyr%20-%20Modules.md)
  - **Example**: [Custom Module Demo](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/modules/say_hello)  
- [Writing Device Drivers](./notes/Zephyr%20-%20Writing%20Drivers.md)
  - **Example**: [Simple Button Driver](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/modules/button) 
- [Multithreading](./notes/Zephyr%20-%20Multithreading.md)
  - **Example 1**: [Threads Demo](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/threads_demo)
  - **Example 2**: [Mutex Demo](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/mutex_demo)
- [Timers, Counters, and Interrupts](./notes/Zephyr%20-%20Timers%2C%20Counters%2C%20and%20Interrupts.md)
  - **Example 1**: [Timer Demo](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/timer_demo)
  - **Example 2**: [Counter Demo](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/counter_interrupt_demo) 

### Advanced Topics
- [Custom Board Support](./notes/Zephyr%20-%20Custom%20Board%20Support.md)
  - **Example**: [Custom Support For STM32F4 Discovery Board](https://github.com/0xAMF/Zephyr-RTOS-Notes/tree/main/examples/boards) 
- [West Workspaces](./notes/Zephyr%20-%20West%20Workspaces.md)

#### Running Freestanding Applications (outside the zephyr directory)
To make things easier, `cd` into the zephyr repo in your local machine and run the following commands:
```bash
west build /path/to/app -b <board_name> -p always
west flash /path/to/app/build
```
- This works because the environment variables required to build a zephyr application already exist inside the zephyr project, you can add these variables to your `.bashrc` but it didn't work for me.
- The other way is to create your application as a west workspace, check notes on west workspaces for more details.

If you are working with qemu, run using the following command:
```bash
west build /path/to/app -b <board_name> -p always
west build /path/to/build -t run
```
- You don't have to pass in the build directory when running if you are inside the application where the build lives.

---

## Resources
- [Zephyr RTOS Official Documentation](https://docs.zephyrproject.org/latest/)  
- [Zephyr GitHub Repository](https://github.com/zephyrproject-rtos/zephyr)  
- [Practical Zephyr Series](https://interrupt.memfault.com/tags#practical-zephyr-series)
- [Introduction To Zephry: DigiKey](https://www.youtube.com/playlist?list=PLEBQazB0HUyTmK2zdwhaf8bLwuEaDH-52)
