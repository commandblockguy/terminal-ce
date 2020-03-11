cmake_minimum_required(VERSION 3.15)
project(terminal_ce C)

set(CMAKE_C_STANDARD 90)

include_directories($ENV{CEDEV}/include)

add_custom_target(Make ALL $(MAKE) WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# A dummy target to make CLion do stuff
add_executable(dummy
        src/gfx/gfx.c
        src/gfx/gfx.h
        src/escape.c
        src/escape.h
        src/graphics.c
        src/graphics.h
        src/input.c
        src/input.h
        src/main.c
        src/menu.c
        src/menu.h
        src/settings.c
        src/settings.h
        src/terminal.c
        src/terminal.h)
