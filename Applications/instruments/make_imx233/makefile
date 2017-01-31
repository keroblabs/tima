##############################################################################################
#
# On command line:
#
# make all = Create project
#
# make clean = Clean project files.
#
# To rebuild project do "make clean" and "make all".
#
#
##############################################################################################

##############################################################################################
# Start of main variables
#

# Define project name here
PROJECT = instruments

# Tima OS paths
DEFAULT_PATH = ../../..

include $(DEFAULT_PATH)/make/make_initial.rules

#
# End of main variables
##############################################################################################

##############################################################################################
# Start of define section
#

# List all default C defines here, like -D_DEBUG=1
DEFS =
DEFS += -D_USE_SCREEN_WIDTH=800
DEFS += -D_USE_SCREEN_HEIGHT=480

# List all default ASM defines here, like -D_DEBUG=1
ADEFS =

#
# End of define section
##############################################################################################

##############################################################################################
# Start of include section
#

# List all default directories to look for include files here
CINCDIR =
CINCDIR += $(APP_PATH)/fonts
CINCDIR += $(APP_PATH)/source
CINCDIR += $(APP_PATH)/icons
CINCDIR += $(APP_PATH)/skins

# End of include section
##############################################################################################

##############################################################################################
# Start of source section
#
SRC  =

# List C source files here
SRC += $(APP_PATH)/source/app_main.c
SRC += $(APP_PATH)/source/instruments.c
SRC += $(APP_PATH)/source/pio_check.c
SRC += $(APP_PATH)/source/gui_setup.c
#SRC += $(APP_PATH)/source/setup_data.c
SRC += $(APP_PATH)/source/fuel_check.c
SRC += $(APP_PATH)/source/syscomm.c
SRC += $(APP_PATH)/source/odometer_control.c
SRC += $(APP_PATH)/source/obd_control.c
#SRC += $(APP_PATH)/source/obd_engine.c
#SRC += $(APP_PATH)/source/battery_check.c
#SRC += $(APP_PATH)/source/warning_lights.c
#SRC += $(APP_PATH)/source/obd_send_module.c


SRC += $(APP_PATH)/skins/skin_motocycle.c
SRC += $(APP_PATH)/skins/skin_standard.c

SRC += $(APP_PATH)/icons/battery_icon.c
SRC += $(APP_PATH)/icons/beam_icon.c
SRC += $(APP_PATH)/icons/bonnet_icon.c
SRC += $(APP_PATH)/icons/boot_icon.c
SRC += $(APP_PATH)/icons/brake_icon.c
SRC += $(APP_PATH)/icons/engine_icon.c
SRC += $(APP_PATH)/icons/epc_icon.c
SRC += $(APP_PATH)/icons/fog_icon.c
SRC += $(APP_PATH)/icons/fuel_blue_icon.c
SRC += $(APP_PATH)/icons/fuel_icon.c
SRC += $(APP_PATH)/icons/full_icon.c
SRC += $(APP_PATH)/icons/hazard_icon.c
SRC += $(APP_PATH)/icons/ind_left_icon.c
SRC += $(APP_PATH)/icons/ind_right_icon.c
SRC += $(APP_PATH)/icons/indicator_icon.c
SRC += $(APP_PATH)/icons/lamp_icon.c
SRC += $(APP_PATH)/icons/lane_icon.c
SRC += $(APP_PATH)/icons/oil_icon.c
SRC += $(APP_PATH)/icons/temp_blue_icon.c
SRC += $(APP_PATH)/icons/temp_icon.c
SRC += $(APP_PATH)/icons/wash_icon.c

# List ASM source files here
ASRC  =

#
# End of source section
##############################################################################################

##############################################################################################
# Start of Modules section
#

#include $(DEFAULT_PATH)/make/make_console.rules
#include $(DEFAULT_PATH)/make/make_filesys.rules
#include $(DEFAULT_PATH)/make/make_speex.rules
include $(DEFAULT_PATH)/make/make_graphics.rules
include $(DEFAULT_PATH)/make/make_framework.rules
include $(DEFAULT_PATH)/make/make_interface.rules
include $(DEFAULT_PATH)/make/make_libc.rules
include $(DEFAULT_PATH)/make/make_system.rules

include $(DEFAULT_PATH)/Boards/imx233/make/make_imx233.rules


#
# End of Modules section
##############################################################################################

##############################################################################################
# Start of additional modules
#

##include $(DEFAULT_PATH)/make/make_4Ddisplay.rules


#
# End of source section
##############################################################################################


# this is always the last include
include $(DEFAULT_PATH)/make/make_rules.rules

# *** EOF ***

