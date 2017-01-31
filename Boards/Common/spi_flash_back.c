#include "system.h"
#include "debug.h"
//#include "commands.h"
#include "timer.h"

///////////////////////////////////////////////////////////////////////////

#define SPI_READ_50MHZ          0x0B
#define SPI_BYTE_WRITE          0x02
#define SPI_AUTO_INC            0xAD
#define SPI_ERASE_4K            0x20
#define SPI_FULL_ERASE          0x60
#define SPI_READ_ID             0x9F
#define SPI_ENABLE_RYBY         0x70
#define SPI_DISABLE_RYBY        0x80
#define SPI_WRITE_EN            0x06
#define SPI_WRITE_DIS           0x04
#define SPI_GET_STATUS          0x05
#define SPI_WREN_STATUS         0x50
#define SPI_WRITE_STATUS        0x01

#define SPI_STATUS_BUSY         0x01
#define SPI_STATUS_WEN          0x02

#define SPI_WRITE_PROTECT       ( 0x3C | 0x80 )

#define SPI_HW_BUSY_STATE       0

#define SPI_CACHE_MASK          0x00FFF
#define SPI_AAI_SIZE            2

#define SPI_FLASH_WRITE_MS		200

#define SPI_STATUS_OK			1
#define SPI_STATUS_FAILED		2

#undef CONSOLE_PRINTK
#define CONSOLE_PRINTK DEBUG_PRINTK

///////////////////////////////////////////////////////////////////////////

static uint8_t      sector_cache[ SPI_CACHE_MASK + 1 ];
static uint32_t     sector_start;
static device_t *   spi_device;
static timer_data_t write_timer;
static bool_t       is_flush;
static bool_t 		is_enabled;

///////////////////////////////////////////////////////////////////////////

static uint8_t spi_flash_read_status( void );

///////////////////////////////////////////////////////////////////////////

static uint8_t spi_flash_wait_no_busy( void )
{
	uint8_t status;
	uint32_t safe_cnt;
	__IO uint32_t wait_cnt;

	safe_cnt = 10000;

    // wait for erase complete here
    do
    {
    	safe_cnt--;
    	if( !safe_cnt ) return SPI_STATUS_FAILED;

    	for( wait_cnt = 0; wait_cnt < 100; wait_cnt++ )
    	{
    	}

        status = spi_flash_read_status();
    } while( status & SPI_STATUS_BUSY );

    return SPI_STATUS_OK;
}

static uint32_t spi_flash_read_id( void )
{
    uint32_t ret_id;
    ret_id = 0;
    uint32_t state;
    
    // select memory
    state = TRUE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // send command and get response
    device_write( spi_device, SPI_READ_ID );
    device_read_buffer( spi_device, ( uint8_t * )&ret_id, 3 );

    // deselect memory
    state = FALSE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // return ID
    return( ret_id );
}

static void spi_flash_send_command( uint8_t command )
{
    uint32_t state;

    // select memory
    state = TRUE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // send command
    device_write( spi_device, command );
    
    // deselect memory
    state = FALSE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );
}

static uint8_t spi_flash_read_status( void )
{
    uint8_t status;
    uint32_t state;

    // select memory
    state = TRUE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // send command
    device_write( spi_device, SPI_GET_STATUS );
    status = device_read( spi_device );
    
    // deselect memory
    state = FALSE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    return( status );
}

static void spi_flash_write_status( uint8_t status )
{
    uint32_t state;

    // write enable command
    spi_flash_send_command( SPI_WREN_STATUS );
    
    // select memory
    state = TRUE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // send command
    device_write( spi_device, SPI_WRITE_STATUS );
    device_write( spi_device, status );
    
    // deselect memory
    state = FALSE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );
}

static void spi_flash_erase_sector( uint32_t addr )
{
	uint32_t state;

    // write enable command
    spi_flash_send_command( SPI_WRITE_EN );
    
    // select memory
    state = TRUE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // send command
    device_write( spi_device, SPI_ERASE_4K );

    // send address
    device_write( spi_device, ( addr >> 16 ) & 0x0FF );
    device_write( spi_device, ( addr >> 8 ) & 0x0FF );
    device_write( spi_device, addr & 0x0FF );
    
    // deselect memory
    state = FALSE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // wait for erase complete here
    if( spi_flash_wait_no_busy() == SPI_STATUS_FAILED )
    {
        CONSOLE_PRINTK( "Erase failed\r\n" );
    }
}

static void spi_flash_read_internal( uint32_t addr, uint8_t * readbuff, uint16_t BlockSize)
{
    uint16_t counter;
	uint32_t state;

    // select memory
    state = TRUE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

    // send command
    device_write( spi_device, SPI_READ_50MHZ );

    // send address
    device_write( spi_device, ( addr >> 16 ) & 0x0FF );
    device_write( spi_device, ( addr >> 8 ) & 0x0FF );
    device_write( spi_device, addr & 0x0FF );

    // send dummy byte
    device_write( spi_device, 0xFF );

    // read block
    for( counter = 0; counter < BlockSize; counter++ )
    {
        readbuff[ counter ] = device_read( spi_device );
    }
    
    // deselect memory
    state = FALSE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );
}

static void spi_flash_write_internal( uint32_t addr, uint8_t * buff, uint16_t size )
{
    uint16_t byte_cnt;
	uint32_t state;

    // write enable command
    spi_flash_send_command( SPI_WRITE_EN );

    //is_first = TRUE;
    for( byte_cnt = 0; byte_cnt < size; byte_cnt += SPI_AAI_SIZE )
    {
        // select memory
        state = TRUE;
        device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

        // send command
        device_write( spi_device, SPI_AUTO_INC );
        
        if( byte_cnt == 0 )
        {
        	device_write( spi_device, ( addr >> 16 ) & 0x0FF );
        	device_write( spi_device, ( addr >> 8 ) & 0x0FF );
        	device_write( spi_device, addr & 0x0FF );
        }
        
        device_write( spi_device, ( uint8_t )buff[ byte_cnt ] );
        device_write( spi_device, ( uint8_t )buff[ byte_cnt + 1 ] );

        // deselect memory
        state = FALSE;
        device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );

        // wait for hw status ready
        if( spi_flash_wait_no_busy() == SPI_STATUS_FAILED )
        {
        	CONSOLE_PRINTK( "Write AAI failed\r\n" );
        	return;
        }
    }
    
    // write disable command
	spi_flash_send_command( SPI_WRITE_DIS );

    if( spi_flash_wait_no_busy() == SPI_STATUS_FAILED )
    {
        CONSOLE_PRINTK( "Write failed\r\n" );
    	return;
    }
}

///////////////////////////////////////////////////////////////////////////

static void spi_flash_global_init( void )
{
    is_flush = FALSE;
	is_enabled = FALSE;

	sector_start = NEG_U32;
}

static int spi_flash_init( void )
{
    uint8_t status;

    timer_Start( &write_timer, SPI_FLASH_WRITE_MS );
    timer_Stop( &write_timer );

    spi_device = device_open( "/dev/spi2" );
    if( spi_device == NULL ) return -1;

#ifdef DEBUG_ENABLED
    DEBUG_PRINTK( "Flash ID: %08x\r\n", spi_flash_read_id() );
#endif

    // get status
    status = spi_flash_read_status();
	CONSOLE_PRINTK( "Status = 0x%02x\r\n", status );

    // check if write is enabled
    if( status & SPI_WRITE_PROTECT )
    {
        // disable protection
        status = 0;

        // write status back
        spi_flash_write_status( status );

        // get status
        status = spi_flash_read_status();
    	CONSOLE_PRINTK( "Status updated = 0x%02x\r\n", status );
    }
    
    return( 0 );
}

static int spi_flash_read(uint64_t addr64, uint8_t *readbuff, uint32_t BlockSize)
{
	__IO uint32_t addr32 = addr64;

	CONSOLE_PRINTK( "Flash read 0x%08x (0x%08x)\r\n", addr32, sector_start );

    if( sector_start != ( addr32 & ( ~SPI_CACHE_MASK ) ) )
    {
    	// save cache
    	if( ( is_flush == FALSE ) && ( is_enabled == TRUE ) )
    	{
            spi_flash_erase_sector( sector_start );
            spi_flash_write_internal( sector_start, sector_cache, SPI_CACHE_MASK + 1 );
    	}

    	is_flush = TRUE;
        timer_Stop( &write_timer );

        // load new cache
        sector_start = addr32 & ( ~SPI_CACHE_MASK );
        spi_flash_read_internal( sector_start, sector_cache, SPI_CACHE_MASK + 1 );
    }

    memcpy( ( uint8_t * )readbuff, &sector_cache[ addr32 & SPI_CACHE_MASK ], BlockSize );
    return( 0 );
}

static int spi_flash_write( uint64_t addr64, uint8_t *writebuff, uint32_t BlockSize )
{
    uint8_t status;
	__IO uint32_t addr32 = addr64;

	CONSOLE_PRINTK( "Flash Write 0x%08x (0x%08x)\r\n", addr32, sector_start );

    if( sector_start != ( addr32 & ( ~SPI_CACHE_MASK ) ) )
    {
    	// save cache
    	if( ( is_flush == FALSE ) && ( is_enabled == TRUE ) )
    	{
            spi_flash_erase_sector( sector_start );
            spi_flash_write_internal( sector_start, sector_cache, SPI_CACHE_MASK + 1 );
    	}

        timer_Stop( &write_timer );

        // load new cache
        sector_start = addr32 & ( ~SPI_CACHE_MASK );
        spi_flash_read_internal( sector_start, sector_cache, SPI_CACHE_MASK + 1 );
    }

    // update cache area
    memcpy( &sector_cache[ addr32 & SPI_CACHE_MASK ], ( uint8_t * )writebuff, BlockSize );
    is_flush = FALSE;
	is_enabled = TRUE;

    // start timer
    timer_Reload( &write_timer );

    return( 0 );
}

static void spi_flash_process( void )
{
	if( is_enabled == FALSE ) return;

	if( timer_Check( &write_timer ) )
	{
		timer_Stop( &write_timer );

		CONSOLE_PRINTK( "Flash flush 0x%08x\r\n", sector_start );

		is_flush = TRUE;
        spi_flash_erase_sector( sector_start );
        spi_flash_write_internal( sector_start, sector_cache, SPI_CACHE_MASK + 1 );
	}
}

static uint32_t spi_flash_block_count( void )
{
    return( 4096 );
}
 
static uint32_t spi_flash_block_size( void )
{
    return( 512 );
}

//////////////////////////////////////////////////////////////////

static const char flash_mass_name[ 8 ] = "/flash";

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
	0
};

DECLARE_CRITICAL_SECTION( spi_flash_global_init );
DECLARE_DISC_SECTION( flash_mount );
DECLARE_PROCESS_SECTION( spi_flash_process );

