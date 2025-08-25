## Using a Custom Board
If your board does not live inside the zephyr directory you have to specify the `BOARD_ROOT` variable.
In this example its defined in the `CMakeLists.txt` file.
```CMake
set(BOARD_ROOT <board-root-directory>)
```
- replace `<board-root-directory>` with the parent directory where the `boards` directory live.
- **IMPORTANT**: your custom board configurations has to be inside a `boards` directory.
