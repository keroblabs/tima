#include "system.h"

////////////////////////////////////////////////////////////////////

#define GO_IDLE_STATE				0x00
#define SEND_OP_CONDITION			0x01
#define SEND_CSD					0x09
#define SEND_CID					0x0A
#define SET_BLOCK_LEN_512			0x10
#define READ_SINGLE_BLOCK			0x11
#define WRITE_BLOCK					0x18
#define ERASE_WR_BLK_START_ADDR		0x20
#define ERASE_WR_BLK_END_ADDR		0x21
#define ERASE_SELECTED_BLOCKS		0x26
#define ACMD_SEND_OP_CONDITION		0x29
#define SEND_APPLICATION_COMMAND	0x37

#define SEND_COMMAND_MASK			0x40

#define SD_SEND_COMMAND_TRY			256

#define R1_IN_IDLE_STATE    		(1<<0)
#define R1_ERASE_RESET      		(1<<1)
#define R1_ILLEGAL_COMMAND  		(1<<2)
#define R1_COM_CRC_ERROR    		(1<<3)
#define R1_ERASE_SEQ_ERROR  		(1<<4)
#define R1_ADDRESS_ERROR    		(1<<5)
#define R1_PARAMETER_ERROR  		(1<<6)

#define ERROR_CMD0					1
#define ERROR_CMD1					2
#define ERROR_CMD16					3
#define ERROR_READ_BLOCK			4
#define ERROR_WRITE_BLOCK			5
#define ERROR_READ_TOKEN			6

#define SD_SELECT()					sdcard_enable( TRUE )
#define SD_CLEAR()					sdcard_enable( FALSE )

////////////////////////////////////////////////////////////////////

static device_t * spi_device;
static uint8_t sd_cmd[8];
static bool_t high_cap_sd;

////////////////////////////////////////////////////////////////////

static void sdcard_enable( bool_t mode )
{
	uint32_t state = ( uint32_t )mode;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &state );
}

static uint8_t sd_calculate_crc( uint8_t * data, uint16_t size )
{
    uint8_t Data;
    uint16_t a;
    int i;
    uint8_t crc = 0;
    
   for (a=0;a<size;a++) //Loop through the packet array starting at packet[0] 
      { 
         Data=data[a]; 
// Loop through the bits in the current array index starting at 0 
         for (i=0;i<8;i++) 
            { 
               crc <<= 1; 
               if ((((Data & 0x80)^(crc & 0x80)) != 0)) 
               { 
                  crc ^= 0x09; 
               } 
               Data <<= 1; 
            }   // End Inner for loop 
       }// End outer for loop 

   crc = (crc<<1) | 1; 

    return(crc);     
}

static bool_t sd_card_response( uint8_t resp )
{
	uint16_t count = 0xFFF;  
	uint8_t result;  

	while(count > 0)  
	{    
		result = device_read( spi_device );
		
		if (result == resp)      
			break;    

		count--;  
	}  

	/* Failure, loop was exited due to timeout */  
	if (count == 0) return TRUE; 

	return FALSE; /* Normal, loop was exited before timeout */	
}

static uint8_t spi_send_sd_command( uint8_t sd_command, uint32_t address )
{
	uint8_t counter;
	uint16_t response;
	uint8_t local_spi_command[ 6 ];

	// spi_exchange_byte( 0xFF );	
    SD_SELECT();
	
	local_spi_command[ 0 ] = SEND_COMMAND_MASK | sd_command;
	local_spi_command[ 1 ] = ( uint8_t )( ( address >> 24 ) & 0x0FF );
	local_spi_command[ 2 ] = ( uint8_t )( ( address >> 16 ) & 0x0FF );
	local_spi_command[ 3 ] = ( uint8_t )( ( address >> 8 ) & 0x0FF );
	local_spi_command[ 4 ] = ( uint8_t )( address & 0x0FF );

 	if( sd_command == GO_IDLE_STATE ) local_spi_command[ 5 ] = 0x95;
    else if( sd_command == 8 ) local_spi_command[ 5 ] = 0x87;
	else local_spi_command[ 5 ] = sd_calculate_crc( local_spi_command, 5 );
    
    device_write_buffer( spi_device, local_spi_command, 6 );

	counter = 0;
	do
	{
		response = device_read( spi_device );
	}
	while( ( ++counter < SD_SEND_COMMAND_TRY ) && ( response == 0xFF ) );
	
    SD_CLEAR();
    
	return( response );    
}

static uint8_t spi_send_sd_CMD8( void )
{
	uint8_t counter;
	uint16_t response;
    uint8_t resp_r1;
	uint8_t local_spi_command[ 6 ];
    uint8_t sd_command = 8;
    uint32_t address = 0x1AA;
    
	// spi_exchange_byte( 0xFF );	
    SD_SELECT();
	
	local_spi_command[ 0 ] = SEND_COMMAND_MASK | sd_command;
	local_spi_command[ 1 ] = ( uint8_t )( ( address >> 24 ) & 0x0FF );
	local_spi_command[ 2 ] = ( uint8_t )( ( address >> 16 ) & 0x0FF );
	local_spi_command[ 3 ] = ( uint8_t )( ( address >> 8 ) & 0x0FF );
	local_spi_command[ 4 ] = ( uint8_t )( address & 0x0FF );
 	local_spi_command[ 5 ] = 0x87;
    
    device_write_buffer( spi_device, local_spi_command, 6 );

	counter = 0;
	do
	{
		resp_r1 = device_read( spi_device );
	}
	while( ( ++counter < SD_SEND_COMMAND_TRY ) && ( resp_r1 == 0xFF ) );
	
    response = device_read( spi_device );
    response = device_read( spi_device );
    response = device_read( spi_device );
    response = device_read( spi_device );
    
    SD_CLEAR();
    
	return( resp_r1 );    
}


static int sd_card_init( void )
{
    uint16_t loop_cnt;
	uint16_t counter;
    uint8_t cmd_retcode;

	high_cap_sd = FALSE;
    loop_cnt = 100;

    spi_device = device_open( "/dev/spi1" );
    if( spi_device == NULL ) return -1;

    while( loop_cnt )
    {
        loop_cnt--;
        
        // send 80 clks
        for( counter = 0; counter < 32; counter++ )
        {
            device_write( spi_device, 0xFF );
        }

        // send CMD0
        counter = 0;
        while( ( spi_send_sd_command( GO_IDLE_STATE, 0 ) != 0x01 ) && ( counter < SD_SEND_COMMAND_TRY ) ) counter++;
        if( counter == SD_SEND_COMMAND_TRY ) continue;
        
        cmd_retcode = spi_send_sd_CMD8();      
		// wait for initialization
        /*
		counter = 0;
		do
		{
		    cmd_retcode = spi_send_sd_command( SEND_OP_CONDITION, 0 );
		    counter++;
		}
		while( ( ( cmd_retcode & R1_IN_IDLE_STATE  ) != 0 ) && ( counter < SD_SEND_COMMAND_TRY ) );
		if( counter == SD_SEND_COMMAND_TRY ) continue;    
        */
            
        // send app command
        cmd_retcode = spi_send_sd_command( SEND_APPLICATION_COMMAND, 0 );

		// init SD mode
		counter = 0;
		do
		{
		    cmd_retcode = spi_send_sd_command( ACMD_SEND_OP_CONDITION, 0 );
		    counter++;
		}
		while( ( cmd_retcode  != 0 ) && ( counter < SD_SEND_COMMAND_TRY ) );
		if( counter < SD_SEND_COMMAND_TRY ) break;        
    }
    
    if( loop_cnt == 0 ) return 1;

	return 0;
}

static int sd_read_block( uint32_t addr, uint8_t *buff, uint16_t BlockSize )
{
	uint32_t index;
    uint8_t ret_code;

	if( high_cap_sd == FALSE )
	{
		index = addr * 512;
	}

    ret_code = spi_send_sd_command( READ_SINGLE_BLOCK, index );
	if( ret_code != 0x00 )
	{
	    return( 1 );
	}    

	// wait for cmd response
	while( ( ret_code = device_read( spi_device ) ) != 0xFE );

	// read block
	device_read_buffer( spi_device, buff, BlockSize );

	// read CRC
	device_read( spi_device );
	device_read( spi_device );
	
	SD_CLEAR();
	device_read( spi_device );

	return( 0 );
}

static int sd_write_block( uint32_t addr, uint8_t *buff, uint16_t BlockSize )
{
	uint32_t index;

	if( high_cap_sd == FALSE )
	{
		index = addr * 512;
	}

	SD_SELECT();

	sd_cmd[0] = WRITE_BLOCK | SEND_COMMAND_MASK;
	sd_cmd[1] = ( index >> 24 ) & 0x0FF;
	sd_cmd[2] = ( index >> 16 ) & 0x0FF;	
	sd_cmd[3] = ( index >> 8 ) & 0x0FF;	
	sd_cmd[4] = ( index ) & 0x0FF;	
	sd_cmd[5] = 0xFF;	

	// send read command
	device_write_buffer( spi_device, sd_cmd, 6 );
    
	// wait for cmd response
	if( sd_card_response( 0 ) == TRUE ) return ERROR_WRITE_BLOCK;

    // write token
	device_write( spi_device, 0xFE );
    
    // write block + dummy CRC
    device_write_buffer( spi_device, buff, BlockSize );
    device_write( spi_device, 0xFE );
    device_write( spi_device, 0xFE );
    
	SD_CLEAR();
	device_read( spi_device );

	return( 0 );    
}

static uint32_t sd_card_block_cnt( void )
{
	return( 4096 );
}

static uint32_t sd_card_block_size( void )
{
	return( 512 );
}

//////////////////////////////////////////////////////////////////

static const char sd_mass_name[ 8 ] = "/sdcard";

///////////////////////////////////////////////////////////////////////////

static const msd_driver_t sd_disc_mount =
{
	sd_mass_name,

    sd_card_init,
    sd_write_block,
    sd_read_block,
    sd_card_block_cnt,
    sd_card_block_size,
	NULL,
	NULL,
	0,
	0
};

DECLARE_STORAGE_DEVICE_SECTION( sd_disc_mount );

	
