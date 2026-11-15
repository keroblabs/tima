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
PROJECT = bcm_001

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
DEFS += -D_USE_SCREEN_WIDTH=240
DEFS += -D_USE_SCREEN_HEIGHT=320

# List all default ASM defines here, like -D_DEBUG=1
ADEFS =

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
CINCDIR += $(APP_PATH)/source
CINCDIR += $(APP_PATH)/library

# End of include section
##############################################################################################

##############################################################################################
# Start of source section
#
SRC  =

# List C source files here
SRC += $(APP_PATH)/source/app_main.c
SRC += $(APP_PATH)/source/can_comms.c
SRC += $(APP_PATH)/source/output_ctrl.c
SRC += $(APP_PATH)/library/pio_check.c

# List ASM source files here
ASRC  =

#
# End of source section
##############################################################################################
# kb2016mgc
# http://www.theadulthub.com/Member/Home.aspx#/d
# ./st-flash --reset --serial 303030303030303030303031 --format 'ihex'  write ~/Projects/GIT/timaos_v42/Applications/bcm_001/make_mini_m4/bcm_001.hex
##############################################################################################
# Start of Modules section
#

include $(DEFAULT_PATH)/make/make_console.rules
include $(DEFAULT_PATH)/make/make_filesys.rules
#include $(DEFAULT_PATH)/make/make_speex.rules
#include $(DEFAULT_PATH)/make/make_graphics.rules
include $(DEFAULT_PATH)/make/make_framework.rules
include $(DEFAULT_PATH)/make/make_interface.rules
include $(DEFAULT_PATH)/make/make_libc.rules
include $(DEFAULT_PATH)/make/make_system.rules

include $(DEFAULT_PATH)/Boards/Nucleo_446/make/make_mini_m4.rules

#
# End of Modules section
##############################################################################################

##############################################################################################
# Start of additional modules
#

#include $(DEFAULT_PATH)/make/make_4Ddisplay.rules


#
# End of source section
##############################################################################################


# this is always the last include
include $(DEFAULT_PATH)/make/make_rules.rules

# *** EOF ***

