# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(bbb C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(bbb
    # bbb.c
    bbb.cpp
    WS2812.cpp
    )

# pull in common dependencies
target_link_libraries(bbb 
    pico_stdlib
    hardware_pio
    hardware_adc
    )

if (PICO_CYW43_SUPPORTED)
    target_link_libraries(bbb pico_cyw43_arch_none)
endif()

# create map/bin/hex file etc.
pico_add_extra_outputs(bbb)

# add url via pico_set_program_url
pico_generate_pio_header(bbb ${CMAKE_CURRENT_LIST_DIR}/WS2812.pio)

pico_set_program_name(bbb "WS2812 Example")
pico_set_program_version(bbb "1.0")

pico_enable_stdio_uart(bbb 1)
pico_enable_stdio_usb(bbb 1)

# Add the standard library to the build
#target_link_libraries(bbb 
 #       pico_stdlib
#)

# Add any user requested libraries
#target_link_libraries(bbb
 #       hardware_pio
#)

pico_add_extra_outputs(bbb)