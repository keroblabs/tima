
#define TIMA_OS

#define DEV_PRINTK				"/dev/tty10"
#define DEV_SYSCOMM             "/dev/tty5"
#define DEV_UNUSED              "/dev/tty18"

////////////////////////////////////////////////////////

#define DEV_SPI_EEPROM			"/dev/spi1"
#define DEV_SPI_FLASH			"/dev/spi1"
#define DEV_SPI_SDCARD			"/dev/spi1"
#define DEV_SPI_GPIO			"/dev/spi2"
#define DEV_SPI_24GHZ			"/dev/spi3"

////////////////////////////////////////////////////////

#define DEV_SOCKET_CLIENT		"/dev/tty7"
#define DEV_SOCKET_SERVER		"/dev/tty6"

#define DEV_AUDIO				"/dev/audio0"

#define DEV_REMOTE				"/dev/tty10"

#define DEV_4DLCD				"/dev/tty8"
#define DEV_SYSTEM				"/dev/tty10"

#define __DEV_OBD_TTY			"/dev/tty10"
#define __DEV_KEYPAD			"/dev/tty3"
#define __DEV_PANEL				"/dev/tty15"

#define DEV_DATALINK			"/dev/tty7"
#define DEV_WIFI_CLICK			"/dev/tty10"
#define DEV_VM_SERIAL			"/dev/tty7"

//#define DEV_TTY_COMMAND			DEV_PRINTK
//#define DEV_TTY_COMMAND			DEV_SOCKET_CLIENT
#define DEV_TTY_COMMAND			DEV_SOCKET_SERVER

#define DONT_USE_MULTITHREAD

#define DONT_WANT_FTP_SERVER
#define DONT_WANT_SOCKET_OVER_TTY
#define DONT_WANT_FTM_OVER_WEB_SERVER
#define DONT_WANT_INPUT_CONSOLE
#define DONT_WANT_WEB_SERVER
#define DONT_WANT_DATALINK
#define DONT_WANT_FTM_LIBRARY

#define DONT_WANT_OBJECT_HANDLER
#define DONT_WANT_GRAPHICS
#define DONT_WAIT_1BPP_FONTS
#define DONT_WANT_RUFFUS_VM

#define WANT_GRAPHICS
#define WANT_AUDIO_INTERFACE
#define WANT_SPEEX
#define WANT_GPIO

#if 0 // def __MACH__
#define _USE_FRONTEND
#endif
