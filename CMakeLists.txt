# Find Zephyr. This also links to Zephyr's build system 
cmake_minimum_required(VERSION 3.13.1)

# set our board 
set(BOARD nrf52840dk_nrf52840)

find_package(Zephyr)

#define project name
project(lab2_part2)

# Add source files
target_include_directories(app PRIVATE src/inc)
FILE(GLOB SRC_FILES "src/*.c")
target_sources(app PRIVATE src/main.c ${SRC_FILES})

