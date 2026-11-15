#include "system.h"
#include "timer.h"

#include "debug.h"

///////////////////////////////////////////////////////////////////////////

#define SPI_DEVICE_OPCODE_WR	0x40
#define SPI_DEVICE_OPCODE_RD	0x41

#define SPI_ADDR_IODIR			0x00
#define SPI_ADDR_IPOL			0x02
#define SPI_ADDR_GPINTEN		0x04
#define SPI_ADDR_DEFVAL			0x06
#define SPI_ADDR_INTCON			0x08
#define SPI_ADDR_IOCON			0x0A
#define SPI_ADDR_GPPU			0x0C
#define SPI_ADDR_INTF			0x0E
#define SPI_ADDR_INTCAP			0x10
#define SPI_ADDR_GPIO			0x12
#define SPI_ADDR_OLAT			0x14

#define PORTA_OFFSET            0x00
#define PORTB_OFFSET            0x01

#define SPI_INDEX_START			70
#define SPI_INDEX_COUNT			16

#define READ_TIMER_MS			50

///////////////////////////////////////////////////////////////////////////

static device_t * spi_device;
static timer_data_t read_timer;
static uint8_t curr_gpio;
static uint8_t curr_output;
static uint8_t req_output;

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

static void spi_expio_control_cs( bool_t state )
{
	uint32_t enable = (uint32_t)state;
	device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &enable );
}

static void spi_expio_write_data( uint8_t address, uint8_t data )
{
	spi_expio_control_cs( TRUE );

	device_write( spi_device, SPI_DEVICE_OPCODE_WR );
	device_write( spi_device, address );
	device_write( spi_device, data );

	spi_expio_control_cs( FALSE );
}

static uint8_t spi_expio_read_data( uint8_t address )
{
	uint8_t data;

	spi_expio_control_cs( TRUE );

	device_write( spi_device, SPI_DEVICE_OPCODE_RD );
	device_write( spi_device, address );

	data = device_read( spi_device );

	spi_expio_control_cs( FALSE );

	return data;
}

static uint8_t spi_expio_read_port( void )
{
	uint8_t value;

	value = spi_expio_read_data( SPI_ADDR_GPIO + PORTA_OFFSET );
	return value;
}

static void spi_expio_set_output( uint8_t portb )
{
    spi_expio_write_data( SPI_ADDR_GPIO + PORTB_OFFSET, portb );
}

static void spi_expio_init( void )
{
    if( spi_device == NULL ) spi_device = device_open( DEV_SPI_GPIO );
    if( spi_device == NULL ) return;

	// default values
	spi_expio_write_data( SPI_ADDR_IOCON + PORTA_OFFSET, 0x00 );
    spi_expio_write_data( SPI_ADDR_IOCON + PORTB_OFFSET, 0x00 );

	// disable all interrupts
	spi_expio_write_data( SPI_ADDR_GPINTEN + PORTA_OFFSET, 0x00 );
	spi_expio_write_data( SPI_ADDR_DEFVAL + PORTA_OFFSET, 0x00 );
	spi_expio_write_data( SPI_ADDR_INTCON + PORTA_OFFSET, 0x00 );

	spi_expio_write_data( SPI_ADDR_GPINTEN + PORTB_OFFSET, 0x00 );
    spi_expio_write_data( SPI_ADDR_DEFVAL + PORTB_OFFSET, 0x00 );
    spi_expio_write_data( SPI_ADDR_INTCON + PORTB_OFFSET, 0x00 );

    // set PORT A as input, pull_up and inverted
	spi_expio_write_data( SPI_ADDR_IODIR + PORTA_OFFSET, 0xFF );
	spi_expio_write_data( SPI_ADDR_GPPU + PORTA_OFFSET, 0xFF );
	spi_expio_write_data( SPI_ADDR_IPOL + PORTA_OFFSET, 0xFF );

	// set PORT B as output
    spi_expio_write_data( SPI_ADDR_IODIR + PORTB_OFFSET, 0x00 );

	timer_Start( &read_timer, READ_TIMER_MS );

	curr_output = req_output = 0x00;
	spi_expio_set_output( req_output );

	curr_gpio = spi_expio_read_port();
    //DEBUG_PRINTK( "PIO = %02x\r\n", curr_gpio );
}

static void spi_expio_read_task( void )
{
	if( spi_device == NULL ) return;

	if( timer_Check( &read_timer ) )
	{
		timer_Reload( &read_timer );

		curr_gpio = spi_expio_read_port();

		if( curr_output != req_output )
		{
		    spi_expio_set_output( req_output );
		    curr_output = req_output;
		}
	}
}

////////////////////////////////////////////////////////////////////////

static void gpio_spi_global_init( void )
{
	spi_device = NULL;
}

static void gpio_drv_init( uint32_t index )
{
	if( spi_device != NULL ) return;
	spi_expio_init();
}

static void gpio_drv_close( uint32_t index )
{
}

static uint32_t gpio_drv_ioctl( uint32_t index, uint32_t param, void * value )
{
	return 0;
}

static uint32_t gpio_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
    if( spi_device == NULL ) return 0;

    index -= SPI_INDEX_START;
    index -= 8; // PORT_B
    if( buffer[0] != 0 ) req_output |= ( 1 << index );
    else req_output &= ~( 1 << index );

    return size;
}

static uint32_t gpio_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
	if( spi_device == NULL ) return 0;

	index -= SPI_INDEX_START;
	buffer[0] = ( curr_gpio & ( 1 << index ) ) ? TRUE : FALSE;
	return size;
}

////////////////////////////////////////////////////////////////////////

static bool_t gpio_validate( uint32_t index )
{
    if( ( index >= SPI_INDEX_START ) && ( index < ( SPI_INDEX_START + SPI_INDEX_COUNT ) ) ) return TRUE;
    return FALSE;
}

static bool_t _in_use;

const static device_data_t gpio_spi_device =
{
    "gpio*",
    &_in_use,

    gpio_drv_init,
    gpio_drv_close,
    NULL,
    gpio_validate,
	NULL,
	gpio_drv_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

    gpio_drv_input,
    gpio_drv_output,

};

DECLARE_DEVICE_SECTION( gpio_spi_device );
DECLARE_INIT_POWERUP_SECTION( gpio_spi_global_init );
DECLARE_PROCESS_SECTION( spi_expio_read_task );


