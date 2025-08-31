## Running Simulation
1. Creating the machine
```bash
mach create
```
- if no name is give, the name `machine-0` is given by default.
2. Load platform description for a specific device.
```bash
machine LoadPlatformDescription @platforms/cpus/<platform>.repl
```
3. Load the binary used
```bash
sysbus LoadELF @/path/to/build/elf
```
4. Display the uart where the messages are shown.
```bash
showAnalyzer sysbus.<shell_uart_device>
```
5. Start the simulation
```bash
start
```