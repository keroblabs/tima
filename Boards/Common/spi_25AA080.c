#include "system.h"

//#include "debug.h"

///////////////////////////////////////////////////////////////////////////

#define SPI_READ_DATA           0x03
#define SPI_WRITE_DATA          0x02
#define SPI_WRITE_EN            0x06
#define SPI_WRITE_DIS           0x04
#define SPI_WRITE_STATUS        0x01
#define SPI_READ_STATUS         0x05

#define EPR_STATUS_WPEN			0x80
#define EPR_STATUS_WEL			0x02
#define EPR_STATUS_WIP			0x01

#define EPR_BP_NONE				0x00
#define EPR_BP_UPPER_Q			0x04
#define EPR_BP_UPPER_H			0x08
#define EPR_BP_ALL				0x0C

///////////////////////////////////////////////////////////////////////////

static device_t * spi_device;

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////

static void spi_flash_control_cs( bool_t state )
{
	uint32_t enable = (uint32_t)state;
	device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &enable );
}

static uint32_t spi_flash_read_id( void )
{
    uint32_t ret_id;
    ret_id = 0;
    
    // return ID
    return( ret_id );
}

static void spi_flash_send_command( uint8_t command )
{
    if( spi_device != NULL )
    {
    	spi_flash_control_cs( TRUE );
    	device_write( spi_device, command );
    	spi_flash_control_cs( FALSE );
    }
}

static uint8_t spi_flash_read_status( void )
{
    uint8_t status = 0;

    if( spi_device != NULL )
    {
    	spi_flash_control_cs( TRUE );

    	device_write( spi_device, SPI_READ_STATUS );
    	status = device_read( spi_device );

    	spi_flash_control_cs( FALSE );
    }

    return( status );
}


static void spi_flash_write_status( uint8_t status )
{
    if( spi_device != NULL )
    {
    	spi_flash_control_cs( TRUE );

    	device_write( spi_device, SPI_WRITE_STATUS );
    	device_write( spi_device, status );

    	spi_flash_control_cs( FALSE );
    }
}

static void spi_flash_erase_sector( uint32_t addr )
{
}

static void spi_flash_read_internal( uint32_t addr, uint8_t * buff, uint16_t size)
{
	uint16_t cnt_byte;

    if( spi_device == NULL ) return;

    for( cnt_byte = 0; cnt_byte < size; cnt_byte++ )
    {
        spi_flash_control_cs( TRUE );
        device_write( spi_device, SPI_READ_DATA );
        device_write( spi_device, ( addr & 0x0FF00 ) >> 8 );
        device_write( spi_device, ( addr & 0x000FF ) );

        buff[cnt_byte] = device_read( spi_device );
        spi_flash_control_cs( FALSE );

        addr++;
    }
}

static void spi_flash_write_internal( uint32_t addr, uint8_t * buff, uint16_t size )
{
	uint16_t cnt_byte;
	uint8_t status;

    if( spi_device == NULL ) return;

	spi_flash_send_command( SPI_WRITE_EN );

    for( cnt_byte = 0; cnt_byte < size; cnt_byte++ )
    {
        spi_flash_control_cs( TRUE );
        device_write( spi_device, SPI_WRITE_DATA );
        device_write( spi_device, ( addr & 0x0FF00 ) >> 8 );
        device_write( spi_device, ( addr & 0x000FF ) );
        device_write( spi_device, buff[cnt_byte] );
        spi_flash_control_cs( FALSE );

        addr++;

        DEBUG_PRINTK( "Flash status: 0x%x\r\n", spi_flash_read_status() );
        while( spi_flash_read_status() & EPR_STATUS_WIP ) {}
        DEBUG_PRINTK( "Flash status: 0x%x\r\n", spi_flash_read_status() );
    }

    spi_flash_send_command( SPI_WRITE_DIS );
}

///////////////////////////////////////////////////////////////////////////

static int spi_flash_init( void )
{
    //uint8_t status;
    //uint8_t wr_data[3] = { 0x01, 0x02, 0x03 };
    //uint8_t rd_data[3];

    spi_device = device_open( "/dev/spi1" );
    if( spi_device == NULL ) return MAL_FAIL;

    return( 0 );
}

static int spi_flash_read(uint64_t block_num, uint8_t *readbuff, uint32_t BlockSize)
{
	spi_flash_read_internal( ( uint32_t )block_num, readbuff, BlockSize );
    return( 0 );
}

static int spi_flash_write(uint64_t block_num, uint8_t *writebuff, uint32_t BlockSize)
{
	spi_flash_write_internal( ( uint32_t )block_num, writebuff, BlockSize );
    return( 0 );
}


static uint32_t spi_flash_block_count( void )
{
    return( 2 );
}
 
static uint32_t spi_flash_block_size( void )
{
    return( 512 );
}

static int spi_flash_format( void )
{
    return 0;
}

//////////////////////////////////////////////////////////////////

static const char flash_mass_name[ 8 ] = "/eeprom";

///////////////////////////////////////////////////////////////////////////

static const msd_driver_t flash_mount =
{
	flash_mass_name,

    spi_flash_init,
    spi_flash_write,
    spi_flash_read,
    spi_flash_block_count,
    spi_flash_block_size,
	NULL,
	NULL,
	0,
	0,
	FALSE
};

DECLARE_STORAGE_DEVICE_SECTION( flash_mount );

