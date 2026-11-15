cmake_minimum_required(VERSION 3.6)

project(make_kd_tima_v42_arm LANGUAGES C ASM)

# Point to our own cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

set(MCU         "cortex-m4")
set(THUMB       "-mthumb -mthumb-interwork")
set(OPT         "-O3")
set(DEBUG       "-g")
set(ASM_DEF     "-D__ASSEMBLY__")
set(LDDEF       "STM32F4xx_FLASH.ld")
set(MAPDEF      "${PROJECT_NAME}.map")

set(LD_LINKER   "-Wl,-T -Xlinker ../../../Boards/Nucleo_446/library/${LDDEF}")
set(LD_MAP      "-Wl,-Map -Xlinker ./${MAPDEF}")
set(LD_SECTION  "-Wl,--cref,--gc-sections")
set(LD_STATIC   "-Wl,-static")
set(LD_DYNAMIC  "-Wl,--export-dynamic")

set(CPU_FLAGS   "-mcpu=${MCU} ${THUMB}")

set(CMAKE_SYSTEM_NAME                   ARM)
set(CMAKE_SYSTEM_PROCESSOR              ARM)
set(CMAKE_CROSSCOMPILING                1)
set(CMAKE_TRY_COMPILE_TARGET_TYPE       STATIC_LIBRARY)
set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS   "")
set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

set(CMAKE_C_COMPILER        arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER      arm-none-eabi-gcc)
set(CMAKE_LINKER            arm-none-eabi-gcc)
set(CMAKE_OBJDUMP           arm-none-eabi-objdump)
set(CMAKE_OBJCOPY           arm-none-eabi-objcopy)

set(CMAKE_LINKER_FLAGS          "-nostartfiles ${LD_LINKER} ${LD_MAP} ${LD_SECTION} ${LD_STATIC} ${LD_DYNAMIC}")
set(CMAKE_CXX_LINK_EXECUTABLE   "<CMAKE_LINKER> ${CPU_FLAGS} ${CMAKE_LINKER_FLAGS} <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")
set(CMAKE_C_LINK_EXECUTABLE     "<CMAKE_LINKER> ${CPU_FLAGS} ${CMAKE_LINKER_FLAGS} <OBJECTS> -o <TARGET>.elf <LINK_LIBRARIES>")

set(CMAKE_C_FLAGS           "-c -fsigned-char -mlittle-endian ${CPU_FLAGS} ${OPT}" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS         "-c -x assembler-with-cpp ${ASM_DEF}" CACHE STRING "" FORCE)


add_definitions(-D_USE_SCREEN_WIDTH=240)
add_definitions(-D_USE_SCREEN_HEIGHT=320)

add_definitions(-DUSE_FILESYS)
add_definitions(-DTIMA_USE_CONFIG)
add_definitions(-DSTM32F446xx)
add_definitions(-DUSE_STDPERIPH_DRIVER)
add_definitions(-DUSE_ST_TYPES)
add_definitions(-DUSE_SECTIONS)
add_definitions(-DBOARD_NUCLEO_446)


add_definitions("-Wall")

link_directories(   ~/Projects/GIT_Dev/SDL/build)

include_directories(
                    ../source
                    ../../../console
                    ../../../graphics
                    ../../../filesys
                    ../../../Boards/Nucleo_446/CMSIS/Device
                    ../../../Boards/Nucleo_446/CMSIS/Include
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/inc
                    ../../../Boards/Nucleo_446/library
                    ../../../Boards/Nucleo_446
                    ../../../Boards/Common
                    ../../../system
                    ../../../c_lib
                    ../../../interface
                    ../../../framework
                    )

add_executable(     ${PROJECT_NAME}
                    ../source/app_main.c

                    ../../main.c

                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/misc.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_rcc.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_gpio.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_usart.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_dma.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_spi.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_adc.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_syscfg.c
                    ../../../Boards/Nucleo_446/STM32F4xx_StdPeriph_Driver/src/stm32f4xx_exti.c

                    ../../../Boards/Nucleo_446/library/adc.c
                    ../../../Boards/Nucleo_446/library/cpu_driver.c
                    ../../../Boards/Nucleo_446/library/ms_timer.c
                    ../../../Boards/Nucleo_446/library/usart_driver.c
                    ../../../Boards/Nucleo_446/library/system_stm32f446.c
                    ../../../Boards/Nucleo_446/library/gpio_driver.c
                    ../../../Boards/Nucleo_446/library/spi_driver.c
                    ../../../Boards/Nucleo_446/library/counter_driver.c
                    ../../../Boards/Nucleo_446/library/socket_library.c

                    ../../../Boards/Nucleo_446/library/startup_stm32f4xx.s

                    ../../../system/device.c
                    ../../../system/heap.c
                    ../../../system/lcd.c
                    ../../../system/mass_mal.c
                    ../../../system/system.c
                    ../../../system/t_threads.c

                    ../../../c_lib/net_lib.c
                    ../../../c_lib/atoi.c
                    ../../../c_lib/date_time.c
                    ../../../c_lib/itoa.c
                    ../../../c_lib/memcpy.c
                    ../../../c_lib/sine_table.c
                    ../../../c_lib/str_compare.c
                    ../../../c_lib/str_token.c
                    ../../../c_lib/vsnprintk.c

                    ../../../interface/epr_layer.c
                    ../../../interface/timer.c
                    ../../../interface/debug.c

                    ../../../framework/buffer.c
                    ../../../framework/random.c
                    ../../../framework/pipe.c
                    ../../../framework/message.c
                    ../../../framework/lp_filter.c
                )

target_link_libraries(  ${PROJECT_NAME} c m gcc)

set_property(   TARGET ${PROJECT_NAME}
                APPEND
                PROPERTY ADDITIONAL_CLEAN_FILES ${PROJECT_NAME}.elf ${PROJECT_NAME}map ${PROJECT_NAME}.lst ${PROJECT_NAME}.bin
            )

add_custom_command( TARGET ${PROJECT_NAME}
                    POST_BUILD
                    COMMAND ${CMAKE_OBJDUMP} -D -t ${PROJECT_NAME}.elf > ${PROJECT_NAME}.lst
                    COMMAND ${CMAKE_OBJCOPY} ${PROJECT_NAME}.elf --output-target=binary ${PROJECT_NAME}.bin
                    )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
