OpenOCD (Open On-Chip Debugger) is an open source tool that is used for programming and debugging microcontrollers.
It's widely used for:
- Programming microcontrollers.
- Debugging.
- Talking to CPU cores.
**How openOCD Works**
- It runs as a server on the host machine.
- It talks to the debugger through GDB remote serial protocol over TCP/IP (on port 3333 by default).
- It translates those commands into **JTAG/SWD operations** to interact with the MCU.
### Running openOCD
openOCD can be installed using either the binary package from [Getting OpenOCD](https://openocd.org/pages/getting-openocd.html), or through a package manager on linux.
```
sudo apt install openocd
```
After connecting your board to the computer now we can start openOCD.
- If the board used has a supported configuration, you can use its configuration directly:
- The configurations are stored under `/usr/share/openocd/scripts/`
```
openocd -f board/stm32f4discovery.cfg
```
- If there is no board configuration you have to specify two things:
	- Target CPU: the MCU family for your your board.
	- Interface: which is the adapter used to program the MCU.
```
openocd -f interface/stlink-v2.cfg -f target/stm32f4x.cfg
```
Now openOCD has a running server on port `:3333` for GDB, or `:4444` for telnet.
**Using GDB**
```bash
gdb-multiarch program.elf
>>> target remote localhost:3333 # connects to openOCD GDB server
>>> monitor reset halt
>>> load     # loads the binary
>>> continue # starts executing
```
**Using `telnet`**
```
telnet localhost 4444
reset halt
flash write_image erase myprogram.elf
reset run
```
---
[openocd.org/doc-release/README](https://openocd.org/doc-release/README)