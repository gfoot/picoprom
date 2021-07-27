cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(picoprom C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

add_executable(picoprom
	configs.c
	eeprom.c
	picoprom.c
	xmodem.c
	i2c.c
)

pico_add_extra_outputs(picoprom)

target_link_libraries(picoprom pico_stdlib hardware_i2c)

pico_enable_stdio_usb(picoprom 1)
pico_enable_stdio_uart(picoprom 0)

