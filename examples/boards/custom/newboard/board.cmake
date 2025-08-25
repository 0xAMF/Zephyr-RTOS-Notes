board_runner_args(openocd "--config" "board/stm32f4discovery.cfg")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

