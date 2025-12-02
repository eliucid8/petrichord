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
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)
include(pico_fft_import.cmake)

# FreeRTOS setup
include(FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(petrichord C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(petrichord 
    petrichord.cpp 
    io/fake_strum_irq.cpp
    # io/cap_strum_irq.cpp
    controllers/chord_controller.cpp
    io/mic_input.cpp
    controllers/imu_controller.cpp
    controllers/key_matrix_controller.cpp
)

pico_set_program_name(petrichord "petrichord")
pico_set_program_version(petrichord "0.1")

# Generate PIO header
pico_generate_pio_header(petrichord ${CMAKE_CURRENT_LIST_DIR}/blink.pio)

# Add the standard library to the build
target_link_libraries(petrichord
        pico_stdlib
        pico_audio_i2s
        )

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(petrichord 0)
pico_enable_stdio_usb(petrichord 1)


# Add the standard include files to the build
target_include_directories(petrichord PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/input
        ${CMAKE_CURRENT_LIST_DIR}/io
        ${CMAKE_CURRENT_LIST_DIR}/pico_fft/src
)

# Add any user requested libraries
target_link_libraries(petrichord 
        hardware_pio
        hardware_timer
        hardware_gpio
        hardware_spi
        hardware_adc
        pico_fft
        hardware_i2c
        hardware_irq
        )

pico_add_extra_outputs(petrichord)

set_target_properties(petrichord 
PROPERTIES CXX_STANDARD 17 
CXX_STANDARD_REQUIRED YES
)

add_subdirectory(controllers/BNO055_driver)
target_link_libraries(petrichord
    pico_bno055
)

target_link_libraries(petrichord
  FreeRTOS-Kernel 
  FreeRTOS-Kernel-Heap4 
)