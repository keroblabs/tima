#include "system.h"
#include "timer.h"
#include "random.h"
#include "tima_libc.h"

#include "elm327_simulator.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static const char response_pid[] = { 'S','E','A','R','C','H','I','N','G','.','.','.',0x0d,
    'U','N','A','B','L','E',' ','T','O',' ','C','O','N','N','E','C','T',0x0d,0x0d,'>',0x00 };

static const char response_cmd[] = { 'O','K',0x0d,'>',0x00 };

static const char elm317_init[] =
{
    0x0d,0x0d,'E','L','M','3','2','7',' ','V','1','.','3','a',0x00
};

static const char response_echo_ok[] =
{
    'a','t','e','0',0x0d,'O','K',0x00
};

static const char response_ok[] =
{
    'O','K',0x00
};

static bool_t echo_state;

static char command_input[1024];
static uint32_t command_index;

///////////////////////////////////////////////////////////////

static int obd_rpm;
static int obd_speed;
static int obd_temp_coolant;

static bool_t is_start;

static device_t * obd_dev;
static timer_data_t update_timer;

static int64_t obd_seed;

///////////////////////////////////////////////////////////////

static void elm327_simulator_send( const char * resp )
{
    char new_line[50];
    strcpy( new_line, resp );
    strcat( new_line, "\r>" );
    
    //DEBUG_PRINTK( "%s\r\n", resp );
    
    device_write_buffer( obd_dev, ( uint8_t * )new_line, ( uint32_t )strlen( new_line ) );
}

static void elm327_simulator_start( void )
{
    char new_line[50];

    strcpy( new_line, elm317_init );
    strcat( new_line, "\r>" );
    
    device_write_buffer( obd_dev, ( uint8_t * )new_line, ( uint32_t )strlen( new_line ) );
}

static bool_t elm327_simulator_validade_command( char * command )
{
    int i;
    bool_t ret = FALSE;
    
    // the only AT command recognized
    if( !strncmp( command, "ate0\r", 5 ) )
    {
        return TRUE;
    }
    
    for( i = 0; i < 5; i++ )
    {
        if( i == 4 )
        {
            if( command[i] != '\r' ) ret = FALSE;
            break;
        }
        
        if( ( ( command[i] >= 0x30 ) && ( command[i] <= 0x39 ) ) ||
            ( ( command[i] >= 'A' ) && ( command[i] <= 'F' ) ) ||
            ( ( command[i] >= 'a' ) && ( command[i] <= 'f' ) ) )
        {
            ret = TRUE;
        }
        else
        {
            ret = FALSE;
            break;
        }
    }
    
    return ret;
}

static void elm327_simulator_send_response( char * command )
{
    char output[50];
    int val;
    
    command[4] = 0;
    //DEBUG_PRINTK( "Command = %s\r\n", command );
    
    if( !strcmp( command, "010c" ) )
    {
        val = obd_rpm << 2;
        sprintk( output, "41 0c %02x %02x", ( val >> 8 ), ( val & 0x0FF ) );
        elm327_simulator_send( output );
    }
    else if( !strcmp( command, "0110" ) )
    {
        elm327_simulator_send("41 10 00 00");
    }
    else if( !strcmp( command, "010d" ) )
    {
        val = obd_speed;
        sprintk( output, "41 0d %02x", ( val & 0x0FF ) );
        elm327_simulator_send( output );
    }
    else if( !strcmp( command, "0105" ) )
    {
        val = obd_temp_coolant + 40;
        sprintk( output, "41 05 %02X", ( val & 0x0FF ) );
        elm327_simulator_send( output );
    }
    else if( !strcmp( command, "0104" ) )
    {
        elm327_simulator_send("41 04 00");
    }
    else if( !strcmp( command, "010f" ) )
    {
        elm327_simulator_send("41 0F 33");
    }
    else if( !strcmp( command, "ate0" ) )
    {
        if( echo_state )
        {
            echo_state = FALSE;
            DEBUG_PRINTK( "Echo OFF" );
            elm327_simulator_send(response_echo_ok);
        }
        else
        {
            DEBUG_PRINTK( "Echo On" );
            elm327_simulator_send(response_ok);
        }
    }
    else
    {
        elm327_simulator_send( "NO DATA" );
    }
}

void elm327_simulator_init( void )
{
    uint32_t baud;
    rand_get_seed( &obd_seed );
    
    obd_rpm = 0;
    obd_speed = 0;
    obd_temp_coolant = 0;
    
    echo_state = FALSE;
    
    obd_dev = device_open( DEV_OBD_TTY );
    if( obd_dev == NULL ) return;
    
    baud = 115200;
    device_ioctrl( obd_dev, DEVICE_SET_CONFIG_DATA, &baud );
    
    timer_Start( &update_timer, 2000 );
    
    is_start = FALSE;
}

void elm327_simulator_process( void )
{
    uint32_t val;
    //char command[10];
    
    if( obd_dev == NULL ) return;
    
    if( is_start == FALSE )
    {
        // flush serial
        device_read_buffer( obd_dev, ( uint8_t * )command_input, 50 );

        is_start = TRUE;
        elm327_simulator_start();
    }
    
    if( device_is_rx_pending( obd_dev ) )
    {
        device_read_buffer( obd_dev, ( uint8_t * )&command_input[command_index], 1 );
        command_input[command_index+1] = 0;
        
        if( echo_state == TRUE )
        {
            device_write_buffer( obd_dev, ( uint8_t * )&command_input[command_index], 1 );
        }
        
        if( command_input[command_index] == 0x0d )
        {
            if( elm327_simulator_validade_command( command_input ) == TRUE )
            {
                elm327_simulator_send_response( command_input );
                command_index = 0;
            }
        }
        else
        {
            command_index++;
        }
        
    }
    
    if( timer_Check( &update_timer ) )
    {
        val = rand_get_next( &obd_seed ) & 0xFFF;
        obd_rpm = 800 + ( int )val;
        //DEBUG_PRINTK( "RPM = %d\r\n", obd_rpm );
        
        val = rand_get_next( &obd_seed ) & 0x07F;
        obd_speed = 5 + ( int )val;
        
        val = rand_get_next( &obd_seed ) & 0x03F;
        obd_temp_coolant = 30 + ( int )val;
        
        timer_Reload( &update_timer );
        //if( obd_temp_coolant < 100 ) obd_temp_coolant += 5;
    }
    
}
