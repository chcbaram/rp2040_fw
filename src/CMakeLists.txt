
file(GLOB SRC_FILES 
  bsp/*.c
  hw/*.c
  hw/driver/*.c
  hw/driver/hangul/*.c
  hw/driver/lcd/*.c
  hw/driver/dxl/*.c

  lib/littlefs/*.c
)

file(GLOB_RECURSE SRC_FILES_RECURSE
  ap/*.cpp
  ap/*.c
  common/*.c 
)

add_executable(
  ${PRJ_NAME}
  
  main.cpp

  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
)

target_include_directories(${PRJ_NAME} PRIVATE 
  ap
  bsp
  common 
  common/core
  common/hw/include 
  hw
  lib
)

add_compile_definitions(${PRJ_NAME}  PUBLIC
  PICO_AUDIO_PIO=1
  PICO_AUDIO_DMA_IRQ=1
  )

target_link_libraries(${PRJ_NAME} 
  cmsis_core
  pico_stdlib
  pico_audio_i2s
  hardware_spi
  hardware_dma
  hardware_irq
  hardware_i2c
  hardware_flash
  hardware_pwm
  hardware_pio  
  )



pico_add_extra_outputs(${PRJ_NAME})
pico_enable_stdio_usb(${PRJ_NAME}  1)
pico_enable_stdio_uart(${PRJ_NAME} 0)

pico_set_linker_script(${PRJ_NAME} "${CMAKE_SOURCE_DIR}/src/bsp/ldscript/pico_flash.ld")

pico_generate_pio_header(${PRJ_NAME} ${CMAKE_CURRENT_LIST_DIR}/hw/driver/pio/ws2812.pio)

set_target_properties(${PRJ_NAME} PROPERTIES SUFFIX ".elf")
add_custom_command(TARGET ${PRJ_NAME} POST_BUILD
        COMMAND arm-none-eabi-size --format=berkeley --totals "$<TARGET_FILE:${PRJ_NAME}>"
        COMMENT "Invoking: Cross ARM GNU Print Size")