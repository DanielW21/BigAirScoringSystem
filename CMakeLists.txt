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
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(BigAirScoringSystem C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# 1. Add the library subdirectories
add_subdirectory(BNO08x_Pico_Library)
add_subdirectory(pico_ssd1306)
add_subdirectory(modules)

# 2. Define your main executable
add_executable(BigAirScoringSystem 
    BigAirScoringSystem.cpp 
)

# 3. Link everything together
target_link_libraries(BigAirScoringSystem
    pico_stdlib
    hardware_i2c
    BNO08x_Pico_Library
    pico_ssd1306
    BigAirModules
)

# Enable USB output so you can see "Yaw" in the Serial Monitor
pico_enable_stdio_usb(BigAirScoringSystem 1)
pico_enable_stdio_uart(BigAirScoringSystem 0)

pico_add_extra_outputs(BigAirScoringSystem)