project(make_kd_tima_v42)
cmake_minimum_required(VERSION 3.0)

# Point to our own cmake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

# Find SDL2
find_package(SDL2 REQUIRED)

# Add global definitions

add_definitions(-D_USE_SCREEN_WIDTH=800)
add_definitions(-D_USE_SCREEN_HEIGHT=480)
add_definitions(-DTIMA_USE_CONFIG)
add_definitions(-D_USE_FRONTEND)
add_definitions(-DMAC_OS_EMULATOR)

add_definitions("-Wall")

link_directories(   ~/Projects/GIT_Dev/SDL/build)

include_directories(
                    ../source
                    ../../../console
                    ../../../graphics
                    ../../../filesys
                    ../../../Boards/MacOS
                    ../../../Boards/Common
                    ../../../system
                    ../../../c_lib
                    ../../../interface
                    ../../../framework
                    )

add_executable(     ${PROJECT_NAME}
                    ../source/app_main.c

                    ../../../main.c

                    ../../../console/cmd_cat.c
                    ../../../console/cmd_cd.c
                    ../../../console/cmd_dump.c
                    ../../../console/cmd_heap.c
                    ../../../console/cmd_ls.c
                    ../../../console/cmd_mkdir.c
                    ../../../console/cmd_obd2.c
                    ../../../console/cmd_part.c
                    ../../../console/cmd_rm.c
                    ../../../console/cmd_rtc.c
                    ../../../console/cmd_speex.c
                    ../../../console/cmd_ymodem.c
                    ../../../console/commands.c
                    ../../../console/ymodem.c

                    ../../../graphics/draw.c
                    ../../../graphics/graphics.c
                    ../../../graphics/font_8x8.c
                    ../../../graphics/font_12x16.c

                    ../../../filesys/efs.c
                    ../../../filesys/extract.c
                    ../../../filesys/partition.c
                    ../../../filesys/fs_time.c
                    ../../../filesys/dir.c
                    ../../../filesys/fat.c
                    ../../../filesys/file.c
                    ../../../filesys/fs.c

                    ../../../Boards/MacOS/library/socket_library.c
                    ../../../Boards/MacOS/library/cpu_driver.c
                    ../../../Boards/MacOS/library/ms_timer.c
                    ../../../Boards/MacOS/library/printf_driver.c
                    ../../../Boards/MacOS/library/rtc_driver.c
                    ../../../Boards/MacOS/library/ttyusb_driver.c
                    ../../../Boards/MacOS/library/eeprom_driver.c
                    ../../../Boards/MacOS/library/file_disc.c
                    ../../../Boards/MacOS/library/adc.c

                    ../../../Boards/Common/SDL_LCD_Driver.c
                    ../../../Boards/Common/socket_driver.c
                    ../../../Boards/Common/vhd_library.c

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

                    ../../../framework/random.c
                    ../../../framework/pipe.c
                    ../../../framework/message.c
                    ../../../framework/lp_filter.c
                )


target_link_libraries(${PROJECT_NAME} SDL2 dl m )

install(TARGETS ${PROJECT_NAME} DESTINATION bin)
