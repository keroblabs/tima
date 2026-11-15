#include "system.h"
#include "debug.h"
//#include "tima_libc.h"
#include "device.h"
//#include "commands.h"
#include "timer.h"
//#include "graphics.h"
//#include "draw.h"
//#include "efs.h"

/////////////////////////////////////////////////////////////////////////////////////////

#ifdef _USE_LINUX
#define DEV_FS_IA6B 				"/dev/tty31"
#define DEV_MDDS30					"/dev/tty32"
#else
#define DEV_FS_IA6B 				"/dev/tty9"
#define DEV_MDDS30					"/dev/tty8"
#endif

#define FS_IA6B_CHANNEL_COUNT		14

#define CHANNEL_INIT_VALUE			0
#define CHANNEL_SAME_VALUE			1
#define CHANNEL_UPDATED_VALUE		2

#define THROTTLE_MIN_VALUE			320
#define THROTTLE_MAX_VALUE			2000
#define THROTTLE_STEPS				( ( THROTTLE_MAX_VALUE - THROTTLE_MIN_VALUE ) / 100 )
#define THROTTLE_CHANGE_MS			100

enum
{
	DRIVER_MODE_AUTO,
	DRIVER_MODE_MANUAL,
};

enum
{
	IBUS_CHANNEL1,
	IBUS_CHANNEL2,
	IBUS_CHANNEL3,
	IBUS_CHANNEL4,
	IBUS_CHANNEL5,
	IBUS_CHANNEL6,
	IBUS_CHANNEL7,
	IBUS_CHANNEL8,
	IBUS_CHANNEL9,
	IBUS_CHANNEL10,
	IBUS_CHANNEL11,
	IBUS_CHANNEL12,
	IBUS_CHANNEL13,
	IBUS_CHANNEL14,
};

/////////////////////////////////////////////////////////////////////////////////////////

typedef struct channel_data_t_
{
	uint16_t value;
	uint8_t is_update;

} channel_data_t;

/////////////////////////////////////////////////////////////////////////////////////////

static device_t * fs_ia6b_dev;
static device_t * mdds_30_dev;
static device_t * blade_left_dev;
static device_t * blade_right_dev;
static device_t * speed_low_dev;
static device_t * speed_high_dev;

static device_t * blade_sw_dev;
static device_t * brake_sw_dev;
static device_t * main_sw_dev;

static device_t * dac_cs;
static device_t * dac_sclk;
static device_t * dac_data;
static device_t * dac_load;

static uint8_t fs_ia6b_state;
static uint8_t channel_cnt;
static uint16_t channel_chk;
static uint16_t temp_channel_value;
static bool_t fs_ia6b_rx_done;
static bool_t enable_dac;
static uint16_t throttle_value;

static timer_data_t throttle_delay;

static bool_t prev_blade_sw;
static bool_t prev_brake_sw;
static bool_t prev_main_sw;

static bool_t start_blade_sw;
static bool_t start_brake_sw;
static bool_t start_main_sw;

static bool_t safe_start;
static bool_t main_enabled;
static bool_t brake_enabled;
static bool_t blade_enabled;

static int driver_value_left;
static int driver_value_right;
static int driver_offset;
static int driver_output;

static uint8_t driver_mode;

static channel_data_t channel_list[ FS_IA6B_CHANNEL_COUNT ];

/////////////////////////////////////////////////////////////////////////////////////////

static device_t * uart_chain_init( uint32_t baud, const char * dev_name )
{
	device_t * dev_data;

    dev_data = device_open( dev_name );

    if( dev_data != NULL )
	{
    	uint32_t ret = device_ioctrl( dev_data, DEVICE_SET_CONFIG_DATA, &baud );
		// DEBUG_PRINTK( "open %s (%d)\r\n", dev_name, ret );
	}

	return dev_data;
}

static void fs_ia6b_process( uint16_t rx_byte )
{
	uint8_t update_flag = CHANNEL_SAME_VALUE;

	switch( fs_ia6b_state )
	{
		case 0:
			fs_ia6b_state = ( rx_byte == 0x20 ) ? 1 : 0;
			break;

		case 1:
			fs_ia6b_state = ( rx_byte == 0x40 ) ? 2 : 0;
			channel_cnt = 0;
			break;

		case 2:
			temp_channel_value = rx_byte;
			fs_ia6b_state = 3;
			break;

		case 3:
			temp_channel_value |= ( rx_byte << 8 );

			if( temp_channel_value < 1000 ) temp_channel_value = 0;
			else temp_channel_value = ( temp_channel_value - 1000 ) >> 4;

			if( channel_list[ channel_cnt ].is_update == CHANNEL_INIT_VALUE )
			{
				update_flag = CHANNEL_SAME_VALUE;
			}
			else if( temp_channel_value != channel_list[ channel_cnt ].value )
			{
				update_flag = CHANNEL_UPDATED_VALUE;
			}

			channel_list[ channel_cnt ].value     = temp_channel_value;
			channel_list[ channel_cnt ].is_update = update_flag;

			channel_cnt++;

			fs_ia6b_state = ( channel_cnt >= FS_IA6B_CHANNEL_COUNT ) ? 4 : 2;
			break;

		case 4:
			channel_chk = rx_byte;
			fs_ia6b_state = 5;
			break;

		case 5:
			channel_chk |= ( rx_byte << 8 );
			fs_ia6b_state = 0;
			fs_ia6b_rx_done = TRUE;
			break;
	}
}

static void mdds_30_update( int left, int right )
{
	uint8_t mdds_buffer[2];

	if( mdds_30_dev != NULL )
	{
		uint8_t mdds_left = ABS(left);
		uint8_t mdds_right = ABS(right);

		mdds_buffer[0] = mdds_left & 0x03F;
		mdds_buffer[0] |= ( left < 0 ) ? 0x40 : 0x00;

		mdds_buffer[1] = mdds_right & 0x03F;
		mdds_buffer[1] |= 0x80;
		mdds_buffer[1] |= ( right < 0 ) ? 0x40 : 0x00;

		device_write_buffer( mdds_30_dev, mdds_buffer, 2 );
	}
}

static void channel_data_debug( void )
{
	int cnt;

	for( cnt = 0; cnt < FS_IA6B_CHANNEL_COUNT; cnt++ )
	{
		if( channel_list[ cnt ].is_update == CHANNEL_UPDATED_VALUE )
		{
			channel_list[ cnt ].is_update = CHANNEL_SAME_VALUE;
			DEBUG_PRINTK( "Channel %d = %d\n", cnt + 1, channel_list[ cnt ].value );
		}
	}
}

static void move_controller_steering( void )
{
	bool_t mdds_needs_update = FALSE;
	int manual_drive = 0;
	int auto_drive = 0;

	if( channel_list[ IBUS_CHANNEL3 ].is_update == CHANNEL_UPDATED_VALUE )
	{
		channel_list[ IBUS_CHANNEL3 ].is_update = CHANNEL_SAME_VALUE;
		mdds_needs_update = TRUE;
	}

	if( channel_list[ IBUS_CHANNEL5 ].is_update == CHANNEL_UPDATED_VALUE )
	{
		channel_list[ IBUS_CHANNEL5 ].is_update = CHANNEL_SAME_VALUE;
		mdds_needs_update = TRUE;
	}

	if( channel_list[ IBUS_CHANNEL7 ].is_update == CHANNEL_UPDATED_VALUE )
	{
		channel_list[ IBUS_CHANNEL7 ].is_update = CHANNEL_SAME_VALUE;
		mdds_needs_update = TRUE;
	}

	if( channel_list[ IBUS_CHANNEL8 ].is_update == CHANNEL_UPDATED_VALUE )
	{
		channel_list[ IBUS_CHANNEL8 ].is_update = CHANNEL_SAME_VALUE;
		mdds_needs_update = TRUE;
	}

	if( channel_list[ IBUS_CHANNEL1 ].is_update == CHANNEL_UPDATED_VALUE )
	{
		channel_list[ IBUS_CHANNEL1 ].is_update = CHANNEL_SAME_VALUE;
		mdds_needs_update = TRUE;
	}

	if( channel_list[ IBUS_CHANNEL6 ].is_update == CHANNEL_UPDATED_VALUE )
	{
		channel_list[ IBUS_CHANNEL6 ].is_update = CHANNEL_SAME_VALUE;
		mdds_needs_update = TRUE;
	}

	if( mdds_needs_update == TRUE )
	{
		mdds_needs_update = FALSE;

		driver_offset = (int)channel_list[ IBUS_CHANNEL1 ].value;
		driver_offset -= 31;

		int factor = driver_offset * (int)channel_list[ IBUS_CHANNEL6 ].value;
		driver_offset = factor / 63;

		driver_mode = ( channel_list[ IBUS_CHANNEL8 ].value > 31 ) ? DRIVER_MODE_AUTO : DRIVER_MODE_MANUAL;
		manual_drive = (int)channel_list[ IBUS_CHANNEL3 ].value;
		auto_drive = (int)channel_list[ IBUS_CHANNEL5 ].value;

		driver_output = ( driver_mode == DRIVER_MODE_AUTO ) ? auto_drive : manual_drive;

		driver_value_left  = driver_output;
		driver_value_right = driver_output;

		if( driver_offset > 0 )
		{
			// turn right
			driver_value_right -= driver_offset;

			if( driver_output < 8 )
			{
				driver_value_left += driver_offset;
			}
		}
		else if( driver_offset < 0 )
		{
			// turn left
			driver_value_left += driver_offset;

			if( driver_output < 8 )
			{
				 driver_value_right -= driver_offset;
			}
		}

		if( driver_output > 10 )
		{
			if( driver_value_right < 0 ) driver_value_right = 0;
			if( driver_value_left < 0 ) driver_value_left = 0;
		}

		if( driver_value_right < -63 ) driver_value_right = -63;
		if( driver_value_left < -63 ) driver_value_left = -63;
		if( driver_value_right > 63 ) driver_value_right = 63;
		if( driver_value_left > 63 ) driver_value_left = 63;

		if( channel_list[ IBUS_CHANNEL7 ].value > 31 )
		{
			driver_value_left = 0 - driver_value_left;
			driver_value_right = 0 - driver_value_right;
		}

		DEBUG_PRINTK( "%4d <- left ... right -> %d\n", driver_value_left, driver_value_right );

		mdds_30_update( driver_value_left, driver_value_right );
	}
}

static void send_byte_to_dac( uint8_t byte_data )
{
	uint8_t counter;
	for( counter = 8; counter != 0; counter-- )
	{
		bool_t bit_out = ( byte_data & ( 1 << ( counter - 1 ) ) ) ? TRUE : FALSE;
		device_write( dac_sclk, 0x00 );
		device_write( dac_data, bit_out );
		device_write( dac_sclk, 0x01 );
	}
}

static void set_throttle_output( uint16_t value )
{
	uint8_t dac_data_value[2];

	dac_data_value[0]  = ( value >> 8 ) & 0x0F;
	dac_data_value[0] |= 0x10;

	dac_data_value[1] = value & 0x0FF;

	device_write( dac_load, 0x01 );
	device_write( dac_cs, 0x00 );
	send_byte_to_dac( dac_data_value[0] );
	send_byte_to_dac( dac_data_value[1] );
	device_write( dac_cs, 0x01 );
	device_write( dac_load, 0x00 );
	device_write( dac_load, 0x01 );

	dac_data_value[0] |= 0x80;

	device_write( dac_load, 0x01 );
	device_write( dac_cs, 0x00 );
	send_byte_to_dac( dac_data_value[0] );
	send_byte_to_dac( dac_data_value[1] );
	device_write( dac_cs, 0x01 );
	device_write( dac_load, 0x00 );
	device_write( dac_load, 0x01 );
}

static bool_t is_blade_enabled;

static void controller_manual_process( void )
{
	if( enable_dac && timer_Check( &throttle_delay ) )
	{
		if( throttle_value < THROTTLE_MAX_VALUE )
		{
			throttle_value += THROTTLE_STEPS;
			set_throttle_output( throttle_value );
			timer_Reload( &throttle_delay );	
		}
		else
		{
			enable_dac = FALSE;
			DEBUG_PRINTK( "Throttle MAX\n" );
		}
	}

	//if (!is_blade_enabled && )

}

static void controller_process( void )
{
	uint16_t rx_byte;

	if( fs_ia6b_dev != NULL )
	{
		if( device_is_rx_pending( fs_ia6b_dev ) )
		{
			rx_byte = device_read( fs_ia6b_dev );
			fs_ia6b_process( rx_byte );
		}
	}

	if( enable_dac && timer_Check( &throttle_delay ) )
	{
		if( throttle_value < THROTTLE_MAX_VALUE )
		{
			throttle_value += THROTTLE_STEPS;
			set_throttle_output( throttle_value );
			timer_Reload( &throttle_delay );	
		}
		else
		{
			enable_dac = FALSE;
			DEBUG_PRINTK( "Throttle MAX\n" );
		}
	}

	if( fs_ia6b_rx_done == TRUE )
	{
		fs_ia6b_rx_done = FALSE;

		if( channel_list[ IBUS_CHANNEL10 ].is_update == CHANNEL_UPDATED_VALUE )
		{
			channel_list[ IBUS_CHANNEL10 ].is_update = CHANNEL_SAME_VALUE;

			if( channel_list[ IBUS_CHANNEL10 ].value > 50 )
			{
				DEBUG_PRINTK( "Blades enabled\n" );
				device_write( blade_left_dev, 0x00 );
				device_write( blade_right_dev, 0x00 );
				enable_dac = TRUE;
				throttle_value = THROTTLE_MIN_VALUE;
				timer_Start( &throttle_delay, THROTTLE_CHANGE_MS );
			}
			else if( channel_list[ IBUS_CHANNEL10 ].value < 20 )
			{
				DEBUG_PRINTK( "Blades disabled\n" );
				device_write( blade_left_dev, 0x01 );
				device_write( blade_right_dev, 0x01 );
				enable_dac = FALSE;
				set_throttle_output( THROTTLE_MIN_VALUE );
			}
		}

		if( channel_list[ IBUS_CHANNEL9 ].is_update == CHANNEL_UPDATED_VALUE )
		{
			channel_list[ IBUS_CHANNEL9 ].is_update = CHANNEL_SAME_VALUE;

			if( channel_list[ IBUS_CHANNEL9 ].value > 50 )
			{
				DEBUG_PRINTK( "Sleep low\n" );
				device_write( speed_low_dev, 0x00 );
				device_write( speed_high_dev, 0x00 );
			}
			else if( channel_list[ IBUS_CHANNEL9 ].value < 20 )
			{
				DEBUG_PRINTK( "Speed high\n" );
				device_write( speed_low_dev, 0x01 );
				device_write( speed_high_dev, 0x01 );
			}
			else
			{
				DEBUG_PRINTK( "Speed half\n" );
				device_write( speed_low_dev, 0x01 );
				device_write( speed_high_dev, 0x00 );
			}
		}
		move_controller_steering();
		//channel_data_debug();
	}
}

static void controller_init( void )
{
	int cnt;

	for( cnt = 0; cnt < FS_IA6B_CHANNEL_COUNT; cnt++ )
	{
		channel_list[ cnt ].is_update = FALSE;
		channel_list[ cnt ].value = 0;
	}

	fs_ia6b_state = 0;
	fs_ia6b_rx_done = FALSE;

	driver_value_left = 0;
	driver_value_right = 0;
	driver_offset = 0;

	driver_mode = DRIVER_MODE_MANUAL;

	driver_offset = 0;
	driver_output = 0;
	driver_value_left = 0;
	driver_value_right = 0;

	enable_dac = FALSE;

	blade_left_dev = device_open( "/dev/gpio1" );
	blade_right_dev = device_open( "/dev/gpio2" );
	speed_high_dev = device_open( "/dev/gpio3" );
	speed_low_dev = device_open( "/dev/gpio4" );

	dac_cs = device_open( "/dev/gpio6" );
	dac_sclk = device_open( "/dev/gpio7" );
	dac_data = device_open( "/dev/gpio8" );
	dac_load = device_open( "/dev/gpio9" );

	device_write( blade_left_dev, 0x01 );
	device_write( blade_right_dev, 0x01 );

	device_write( speed_high_dev, 0x01 );
	device_write( speed_low_dev, 0x01 );

	device_write( dac_cs, 0x01 );
	device_write( dac_sclk, 0x00 );
	device_write( dac_data, 0x00 );
	device_write( dac_load, 0x01 );

	fs_ia6b_dev = uart_chain_init( 115200, DEV_FS_IA6B );
	mdds_30_dev = uart_chain_init( 115200, DEV_MDDS30 );

	set_throttle_output( THROTTLE_MIN_VALUE );
	mdds_30_update( 0, 0 );
}

static void blade_sw_event( bool_t state )
{

}

static void brake_sw_event( bool_t state )
{
	
}

static void main_sw_event( bool_t state )
{
	if( state == FALSE )
	{
		safe_start = FALSE;
		set_throttle_output( THROTTLE_MIN_VALUE );
		mdds_30_update( 0, 0 );
	}
}

static bool_t controller_manual_gpio_process( void )
{
	/* make sure the inputs have the correct polarity */
	bool_t local_blade_sw = device_read( blade_sw_dev ) ? FALSE : TRUE; // FALSE = disabled
	bool_t local_brake_sw = device_read( brake_sw_dev ) ? FALSE : TRUE; // FALSE = depressed 
	bool_t local_main_sw = device_read( main_sw_dev ) ? FALSE : TRUE;   // FALSE = turned off

	if( ( local_blade_sw == start_blade_sw ) &&
	    ( local_brake_sw == start_brake_sw ) &&
	    ( local_main_sw == start_main_sw ) &&
	    ( safe_start == FALSE ) )
	{
		safe_start = TRUE;		
	}
	
	if( safe_start == FALSE )
	{
		local_blade_sw = start_blade_sw;
		local_brake_sw = start_brake_sw;
		local_main_sw = start_main_sw;
	}

	if( prev_blade_sw != local_blade_sw )
	{
		prev_blade_sw = local_blade_sw;
		blade_sw_event( local_blade_sw );
	}

	if( prev_brake_sw != local_brake_sw )
	{
		prev_brake_sw = local_brake_sw;
		brake_sw_event( local_brake_sw );
	}

	if( prev_main_sw != local_main_sw )
	{
		prev_main_sw = local_main_sw;
		main_sw_event( local_main_sw );
	}

	return TRUE;
}

static void controller_manual_init( void )
{
	start_blade_sw = FALSE;
	start_brake_sw = FALSE;
	start_main_sw = FALSE;
	safe_start = FALSE;

	blade_left_dev = device_open( "/dev/gpio1" );
	blade_right_dev = device_open( "/dev/gpio2" );
	speed_high_dev = device_open( "/dev/gpio3" );
	speed_low_dev = device_open( "/dev/gpio4" );

	dac_cs = device_open( "/dev/gpio6" );
	dac_sclk = device_open( "/dev/gpio7" );
	dac_data = device_open( "/dev/gpio8" );
	dac_load = device_open( "/dev/gpio9" );

	blade_sw_dev = device_open( "/dev/gpio10" );
	brake_sw_dev = device_open( "/dev/gpio11" );
	main_sw_dev = device_open( "/dev/gpio12" );

	mdds_30_dev = uart_chain_init( 115200, DEV_MDDS30 );

	device_write( blade_left_dev, 0x01 );
	device_write( blade_right_dev, 0x01 );

	device_write( speed_high_dev, 0x01 );
	device_write( speed_low_dev, 0x01 );

	device_write( dac_cs, 0x01 );
	device_write( dac_sclk, 0x00 );
	device_write( dac_data, 0x00 );
	device_write( dac_load, 0x01 );

	set_throttle_output( THROTTLE_MIN_VALUE );
	mdds_30_update( 0, 0 );
}

void app_main( void )
{
	controller_init();

	DEBUG_PRINTK( "Lawnmower controller\r\n" );

    while( 1 )
    {
		controller_process();
		SYSTEM_EVENTS();
    }
}

DECLARE_APPLICATION_SECTION( app_main );
