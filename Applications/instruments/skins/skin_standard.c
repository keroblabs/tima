#include "system.h"
#include "timer.h"
#include "graphics.h"
#include "draw.h"
#include "pipe.h"
#include "tima_libc.h"

#include "warning_lights.h"
#include "instruments.h"

#include "icons.h"

///////////////////////////////////////////////////////////////

#define TEMP_FLASHING_MS		800
#define BRAKE_FLASHING_MS		300

#define WARNING_POSX            25
#define WARNING_POSY            ( _USE_SCREEN_HEIGHT - SLOT_SPACE_HEIGHT - 10 )

#define INSTRUMENTS_FONT_TYPE		FONT_12x16	

#define INSTRUMENT_LINE_LEN			26
#define INSTRUMENT_RATIO			220
#define SMALL_GAUGE_RATIO			120

#define INSTRUMENTS_OFFSET_Y		100

#define RPM_INSTRUMENT_POSX			570
#define RPM_INSTRUMENT_POSY			( 270 + INSTRUMENTS_OFFSET_Y )
#define SPEED_INSTRUMENT_POSX		270
#define SPEED_INSTRUMENT_POSY		( 270 + INSTRUMENTS_OFFSET_Y )

#define FUEL_INSTRUMENT_POSX		650
#define FUEL_INSTRUMENT_POSY		( 150 + INSTRUMENTS_OFFSET_Y )
#define TEMP_INSTRUMENT_POSX		650
#define TEMP_INSTRUMENT_POSY		( 340 + INSTRUMENTS_OFFSET_Y )

#define WARNING_TOP_POSX        ( ( _USE_SCREEN_WIDTH >> 1 ) - ( SLOT_SPACE_WIDTH >> 1 ) )
#define WARNING_TOP_POSY        20

#define SLOT_SPACE_WIDTH		80
#define SLOT_SPACE_HEIGHT		50

#define SLOT_WARNING_POS(x)		( WARNING_POSX + ( x * SLOT_SPACE_WIDTH ) ), WARNING_POSY

#define SLOT0_WARNING_POS		( WARNING_TOP_POSX - SLOT_SPACE_WIDTH ), WARNING_TOP_POSY - 10
#define SLOT1_WARNING_POS		( WARNING_TOP_POSX - ( SLOT_SPACE_WIDTH * 3 ) ), WARNING_TOP_POSY
#define SLOT2_WARNING_POS		( WARNING_TOP_POSX - ( SLOT_SPACE_WIDTH * 2 ) ), WARNING_TOP_POSY
#define SLOT3_WARNING_POS		( WARNING_TOP_POSX + ( SLOT_SPACE_WIDTH * 2 ) ), WARNING_TOP_POSY
#define SLOT4_WARNING_POS		( WARNING_TOP_POSX + ( SLOT_SPACE_WIDTH * 3 ) ), WARNING_TOP_POSY

#define SLOT5_WARNING_POS		SLOT_WARNING_POS(0)
#define SLOT6_WARNING_POS		SLOT_WARNING_POS(1)
#define SLOT7_WARNING_POS		SLOT_WARNING_POS(2)
#define SLOT8_WARNING_POS		SLOT_WARNING_POS(3)
#define SLOT9_WARNING_POS		SLOT_WARNING_POS(4)

#define SLOT10_WARNING_POS		SLOT_WARNING_POS(5)
#define SLOT11_WARNING_POS		SLOT_WARNING_POS(6)

#define SLOT_WARNING_POS_MAX	0xFFFF, 0xFFFF

///////////////////////////////////////////////////////////////

static const warning_data_t warning_data[] = 
{
	{ SLOT0_WARNING_POS, NULL /* car1 */,			( 1 << WARNING_DOORLEFT ),		FALSE },
	{ SLOT0_WARNING_POS, NULL /* car2 */,			( 1 << WARNING_DOORRIGHT ),		FALSE },
	{ SLOT0_WARNING_POS, NULL /* car3 */,			( 1 << WARNING_BONNET ),		FALSE },
	{ SLOT0_WARNING_POS, NULL /* car4 */,			( 1 << WARNING_BOOT ),			FALSE },

	{ SLOT1_WARNING_POS, &lane_icon_bitmap,			( 1 << WARNING_CRUISE ),		FALSE },
	{ SLOT2_WARNING_POS, &engine_icon_bitmap,		( 1 << WARNING_ENGINE ),		FALSE },
	{ SLOT3_WARNING_POS, &epc_icon_bitmap,			( 1 << WARNING_SYSFAIL ),		FALSE },
	{ SLOT4_WARNING_POS, &wash_icon_bitmap,			( 1 << WARNING_LOWWASHER ),		FALSE },

	///////////////////////////////////////////////////////////////

	{ SLOT5_WARNING_POS, &lamp_icon_bitmap,				( 1 << WARNING_HEADLAMP ),	FALSE },
	//{ SLOT5_WARNING_POS, &beam_icon_bitmap,				( 1 << WARNING_MAINBEAM ),	FALSE },
	{ SLOT5_WARNING_POS-4,&fog_icon_bitmap,				( 1 << WARNING_FOGLIGHT ),	FALSE },
	{ SLOT5_WARNING_POS, &full_icon_bitmap,				( 1 << WARNING_FULLBEAM ),	FALSE },

	{ SLOT6_WARNING_POS, &brake_icon_bitmap,	( 1 << WARNING_HANDBRAKE_MOVING ),	TRUE  },
	{ SLOT6_WARNING_POS, &brake_icon_bitmap,	( 1 << WARNING_HANDBRAKE_STATE ),	FALSE },
	{ SLOT6_WARNING_POS, &brake_icon_bitmap,	( 1 << WARNING_BRAKELEVEL ),		FALSE },

	{ SLOT7_WARNING_POS, &indicator_left_icon_bitmap,	( 1 << WARNING_INDLEFT ),	FALSE },
	{ SLOT7_WARNING_POS, &indicator_right_icon_bitmap,	( 1 << WARNING_INDRIGHT ),	FALSE },
	{ SLOT7_WARNING_POS, &hazard_icon_bitmap,			( 1 << WARNING_HAZARD ),	FALSE },

	{ SLOT9_WARNING_POS, &temp_icon_bitmap,			( 1 << WARNING_TEMPERATURE ),	TRUE  },

	{ SLOT11_WARNING_POS, &battery_icon_bitmap,			( 1 << WARNING_BATTERY ),	FALSE },

	{ SLOT10_WARNING_POS, &oil_icon_bitmap,			( 1 << WARNING_OILPRESSURE ),	FALSE },

	{ SLOT_WARNING_POS_MAX, NULL, 0, FALSE }
};

///////////////////////////////////////////////////////////////

static void skin_standard_draw_instrument_speed( uint16_t posx, uint16_t posy, uint16_t colour, uint16_t niddle_colour, int angle );
static void skin_standard_draw_instrument_rpm( uint16_t posx, uint16_t posy, uint16_t colour1, uint16_t colour2, uint16_t niddle_colour, int angle );
static void skin_standard_draw_fuel_gauge( uint16_t posx, uint16_t posy, uint16_t colour1, uint16_t colour2, uint16_t niddle_colour, int angle, bool_t low );
static void skin_standard_draw_temperature_gauge( uint16_t posx, uint16_t posy, uint16_t colour1, uint16_t colour2, uint16_t niddle_colour, int angle );

static void skin_standard_process( void );

///////////////////////////////////////////////////////////////

static uint16_t gauge_colour;
static uint16_t niddle_colour;
static uint16_t red_colour;
static uint16_t back_colour;

static bool_t low_fuel_flag;
static bool_t is_mph;

static int curr_speed_angle = -1;
static bool_t update_speed = FALSE;

static int curr_rpm_angle = -1;
static bool_t update_rpm = FALSE;
static bool_t update_rpm_arc = FALSE;

static uint32_t curr_main_odometer = -1;
static uint32_t curr_trip_odometer = -1;

static int curr_fuel_angle = -1;
static bool_t update_fuel = FALSE;
static bool_t prev_low_fuel = TRUE+1;

static int curr_temp_angle = -1;
static bool_t update_temp = FALSE;

static niddle_data_t speed_niddle_data;
static niddle_data_t rpm_niddle_data;

static uint32_t rpm_angle;
static uint32_t speed_angle;
static uint32_t temperature_angle;
static uint32_t fuel_angle;

///////////////////////////////////////////////////////////////

static void skin_standard_draw_instrument_speed( uint16_t posx, uint16_t posy, uint16_t colour, uint16_t niddle_colour, int angle )
{
	int angle_offset;

	if( angle == curr_speed_angle ) return;

	draw_font_init( INSTRUMENTS_FONT_TYPE );
	if( update_speed == TRUE )
	{
		// draw_erase_niddle( posx, posy, 5, INSTRUMENT_RATIO+20, curr_speed_angle, niddle_speed_backup );
		draw_erase_niddle_ex( &speed_niddle_data, curr_speed_angle );
	}

	angle_offset = ABS( angle - curr_speed_angle ) >> 3;
	if( angle_offset > 40 ) angle_offset = 40;
	if( angle_offset == 0 ) angle_offset = 1;

	if( angle > curr_speed_angle ) curr_speed_angle += angle_offset;
	else curr_speed_angle -= angle_offset;

	if( update_speed == FALSE )
	{
		draw_instrument_marks( posx, posy, INSTRUMENT_RATIO+(INSTRUMENT_NUM_DIST*3), 15, colour, 0, 1200, 80/5 );
		draw_instrument_marks( posx, posy, INSTRUMENT_RATIO+(INSTRUMENT_NUM_DIST*3), 30, colour, 0, 1200, 80 );

		draw_instrument_numbers( posx, posy, INSTRUMENT_RATIO-40, colour, 0, 1200, 80*2, 0, 20 );
	}

	if( is_mph )
		show_small_number( posx-100, posy-75, curr_speed_angle / 8, "MPH", colour );
	else
		show_small_number( posx-100, posy-75, curr_speed_angle / 5, "KM/H", colour );

	draw_instruments_arc( posx, posy, rpm_niddle_data.circle_ratio-5, 0, 1200, speed_niddle_data.niddle_colour );

	//draw_niddle( posx, posy, 5, INSTRUMENT_RATIO+20, curr_speed_angle, niddle_colour, niddle_speed_backup );
	draw_niddle_ex( &speed_niddle_data, curr_speed_angle );
	update_speed = TRUE;
}

static void skin_standard_draw_instrument_rpm( uint16_t posx, uint16_t posy, uint16_t colour1, uint16_t colour2, uint16_t niddle_colour, int angle )
{
	int angle_offset;

	if( angle == curr_rpm_angle ) return;

	draw_font_init( INSTRUMENTS_FONT_TYPE );
	if( update_rpm == TRUE )
	{
		//draw_erase_niddle( posx, posy, 5, INSTRUMENT_RATIO+20, curr_rpm_angle, niddle_rpm_backup );
		draw_erase_niddle_ex( &rpm_niddle_data, curr_rpm_angle );
	}

	angle_offset = ABS( angle - curr_rpm_angle ) >> 3;
	if( angle_offset > 40 ) angle_offset = 40;
	if( angle_offset == 0 ) angle_offset = 1;

	if( angle > curr_rpm_angle ) curr_rpm_angle += angle_offset;
	else curr_rpm_angle -= angle_offset;

	if( update_rpm == FALSE )
	{
		draw_instrument_marks( posx, posy, INSTRUMENT_RATIO+(INSTRUMENT_NUM_DIST*3), 15, colour1, 0, 800, 80/2 );
		draw_instrument_marks( posx, posy, INSTRUMENT_RATIO+(INSTRUMENT_NUM_DIST*3), 30, colour1, 0, 800, 80 );
		draw_instrument_numbers( posx, posy, INSTRUMENT_RATIO-(INSTRUMENT_NUM_DIST*4), colour1, 0, 800, 80*2, 0, 1 );

		draw_instrument_marks( posx, posy, INSTRUMENT_RATIO+(INSTRUMENT_NUM_DIST*3), 15, colour2, 800, 1200, 80/2 );
		draw_instrument_marks( posx, posy, INSTRUMENT_RATIO+(INSTRUMENT_NUM_DIST*3), 30, colour2, 800, 1200, 80 );
		draw_instrument_numbers( posx, posy, INSTRUMENT_RATIO-(INSTRUMENT_NUM_DIST*4), colour2, 800, 1200, 80*2, 5, 1 );

		draw_instruments_arc( posx, posy, rpm_niddle_data.circle_ratio-5, 0, 1200, rpm_niddle_data.niddle_colour );
	}

	//draw_niddle( posx, posy, 5, INSTRUMENT_RATIO+20, curr_rpm_angle, niddle_colour, niddle_rpm_backup );
	draw_niddle_ex( &rpm_niddle_data, curr_rpm_angle );

	update_rpm = TRUE;
}

static void skin_standard_draw_fuel_gauge( uint16_t posx, uint16_t posy, uint16_t colour1, uint16_t colour2, uint16_t niddle_colour, int angle, bool_t low )
{
	int angle_offset;

	if( angle == curr_fuel_angle ) return;

	if( angle > 1024 ) angle = 1024;
	if( angle < 0 ) angle = 0;

	draw_font_init( INSTRUMENTS_FONT_TYPE );
	if( update_fuel == TRUE )
	{
		draw_erase_niddle( posx, posy, 5, SMALL_GAUGE_RATIO-5, 2048-curr_fuel_angle, niddle_fuel_backup );
	}

	angle_offset = ABS( angle - curr_fuel_angle ) >> 3;
	if( angle_offset > 64 ) angle_offset = 64;
	if( angle_offset == 0 ) angle_offset = 1;

	if( angle > curr_fuel_angle ) curr_fuel_angle += angle_offset;
	else curr_fuel_angle -= angle_offset;

	//if( angle > curr_angle ) curr_angle++;
	//else curr_angle--;

	if( ( prev_low_fuel != TRUE ) && ( low == TRUE ) )
	{
		graphics_fill_box( posx+35, posy-50, fuel_blue_icon_bitmap.width, fuel_blue_icon_bitmap.width, back_colour );
		graphics_show_bitmap( posx+25, posy-50, &fuel_icon_bitmap, TRUE );
		prev_low_fuel = TRUE;
	}
	else if( ( prev_low_fuel != FALSE ) && ( low == FALSE ) )
	{
		graphics_fill_box( posx+25, posy-50, fuel_icon_bitmap.width, fuel_icon_bitmap.width, back_colour );
		graphics_show_bitmap( posx+35, posy-50, &fuel_blue_icon_bitmap, TRUE );
		prev_low_fuel = FALSE;
	}

	if( update_fuel == FALSE )
	{
		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 10, colour1, 1024, 1920, 64 );
		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 10, colour2, 1920, 2049, 64 );

		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 20, colour1, 1024, 1920, 256 );
		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 20, colour2, 1920, 2049, 256 );

		draw_instrument_print( posx, posy, SMALL_GAUGE_RATIO, colour2, "E", 2048 );
		draw_instrument_print( posx, posy, SMALL_GAUGE_RATIO, colour1, "1/2", 1536 );
		draw_instrument_print( posx, posy, SMALL_GAUGE_RATIO, colour1, "F", 1024 );
	}

	draw_niddle( posx, posy, 5, SMALL_GAUGE_RATIO-5, 2048-curr_fuel_angle, niddle_colour, niddle_fuel_backup );

	update_fuel = TRUE;
}

static void skin_standard_draw_temperature_gauge( uint16_t posx, uint16_t posy, uint16_t colour1, uint16_t colour2, uint16_t niddle_colour, int angle )
{
	int angle_offset;

	if( angle == curr_temp_angle ) return;

	draw_font_init( INSTRUMENTS_FONT_TYPE );
	if( update_temp == TRUE )
	{
		draw_erase_niddle( posx, posy, 5, SMALL_GAUGE_RATIO-5, 2048-curr_temp_angle, niddle_temp_backup );
	}

	angle_offset = ABS( angle - curr_temp_angle ) >> 3;
	if( angle_offset > 64 ) angle_offset = 64;
	if( angle_offset == 0 ) angle_offset = 1;

	if( angle > curr_temp_angle ) curr_temp_angle += angle_offset;
	else curr_temp_angle -= angle_offset;

	//if( angle > curr_angle ) curr_angle++;
	//else curr_angle--;

	if( update_temp == FALSE )
	{
		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 10, colour2, 1024, 1024+256, 64 );
		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 10, colour1, 1024+256, 2049, 64 );

		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 20, colour2, 1024, 1024+256, 256 );
		draw_instrument_marks( posx, posy, SMALL_GAUGE_RATIO, 20, colour1, 1024+256, 2049, 256 );

		draw_instrument_print( posx, posy, SMALL_GAUGE_RATIO, colour1, "C", 2048 );
		//draw_instrument_print( posx, posy, SMALL_GAUGE_RATIO, colour1, "ok", 1536 );
		draw_instrument_print( posx, posy, SMALL_GAUGE_RATIO, colour2, "H", 1024 );

		graphics_show_bitmap( posx+35, posy-50, &temp_blue_icon_bitmap, TRUE );
	}

	draw_niddle( posx, posy, 5, SMALL_GAUGE_RATIO-5, 2048-curr_temp_angle, niddle_colour, niddle_temp_backup );
	update_temp = TRUE;
}

static void skin_standard_show_odometer( uint16_t posx, uint16_t posy, uint64_t main, uint64_t trip )
{
#define ODOMETER_OFFSET_X		120
#define ODOMETER_OFFSET_Y		95

	bool_t update_arc = FALSE;

	if( main != curr_main_odometer )
	{
		curr_main_odometer = main;
		show_odometer_total_number( posx-ODOMETER_OFFSET_X, posy-ODOMETER_OFFSET_Y+28, (uint32_t)main, 1, gauge_colour );
		update_arc = TRUE;
	}

	if( trip != curr_trip_odometer )
	{
		curr_trip_odometer = trip;
		show_odometer_total_number( posx-ODOMETER_OFFSET_X, posy-ODOMETER_OFFSET_Y, (uint32_t)trip, 10, gauge_colour );
		update_arc = TRUE;
	}

	if( update_arc == TRUE )
	{
		draw_instruments_arc( posx, posy, rpm_niddle_data.circle_ratio-5, 0, 1200, rpm_niddle_data.niddle_colour );
	}
}

static void skin_standard_update_warning_index( uint32_t index, uint32_t mask, bool_t use_timed )
{
	uint32_t cnt;

	cnt = 0;
	while( warning_data[cnt].mask != 0 )
	{
		if( warning_data[cnt].mask & ( 1 << index ) )
		{
			graphics_fill_box( warning_data[cnt].posx, warning_data[cnt].posy, SLOT_SPACE_WIDTH, SLOT_SPACE_HEIGHT, APP_RGB(0,0,0) );

			if( warning_data[cnt].mask & mask )
			{
				if( ( warning_data[cnt].p_icon != NULL ) && ( warning_data[cnt].timed == use_timed ) )
					graphics_show_bitmap( warning_data[cnt].posx, warning_data[cnt].posy, warning_data[cnt].p_icon, TRUE );
			}

			break;
		}

		cnt++;
	}
}

static void skin_standard_init( void )
{
#if defined WIN32
	gauge_colour = APP_RGB( 32,32,32 );
	niddle_colour = APP_RGB( 32,16,0 );
	red_colour = APP_RGB( 32, 0, 0 );
#else
	gauge_colour = APP_RGB( 200,200,200 );
	niddle_colour = APP_RGB( 255,192,0 );
	red_colour = APP_RGB( 255, 0, 0 );
#endif

	back_colour = instruments_get_back_colour();
	low_fuel_flag = FALSE;

	curr_speed_angle = -1;
	update_speed = FALSE;

	curr_rpm_angle = -1;
	update_rpm = FALSE;

	curr_fuel_angle = -1;
	update_fuel = FALSE;
	prev_low_fuel = TRUE+1;

	curr_temp_angle = -1;
	update_temp = FALSE;

	speed_niddle_data.backup = &niddle_speed_backup[0];
	speed_niddle_data.posx = SPEED_INSTRUMENT_POSX;
	speed_niddle_data.posy = SPEED_INSTRUMENT_POSY;
	speed_niddle_data.niddle_colour = niddle_colour;
	speed_niddle_data.niddle_width = 5;
	speed_niddle_data.niddle_len = INSTRUMENT_RATIO+20;
	speed_niddle_data.end_niddle = TRUE;
	speed_niddle_data.draw_circle = FALSE;
	speed_niddle_data.circle_ratio = INSTRUMENT_RATIO-80;
	speed_niddle_data.circle_colour = niddle_colour;

	rpm_niddle_data.backup = &niddle_rpm_backup[0];
	rpm_niddle_data.posx = RPM_INSTRUMENT_POSX;
	rpm_niddle_data.posy = RPM_INSTRUMENT_POSY;
	rpm_niddle_data.niddle_colour = niddle_colour;
	rpm_niddle_data.niddle_width = 5;
	rpm_niddle_data.niddle_len = INSTRUMENT_RATIO+20;
	rpm_niddle_data.end_niddle = TRUE;
	rpm_niddle_data.draw_circle = FALSE;
	rpm_niddle_data.circle_ratio = INSTRUMENT_RATIO-80;
	rpm_niddle_data.circle_colour = niddle_colour;
}

static void skin_standard_mph_init( void )
{
	skin_standard_init();
	is_mph = TRUE;
}

static void skin_standard_kph_init( void )
{
	skin_standard_init();
	is_mph = FALSE;
}

static void skin_standard_process( void )
{
	skin_standard_show_odometer( RPM_INSTRUMENT_POSX, RPM_INSTRUMENT_POSY, odometer_value, 0 );

	skin_standard_draw_instrument_speed( SPEED_INSTRUMENT_POSX, SPEED_INSTRUMENT_POSY, gauge_colour, niddle_colour, speed_angle );
	skin_standard_draw_instrument_rpm( RPM_INSTRUMENT_POSX, RPM_INSTRUMENT_POSY, gauge_colour, red_colour, niddle_colour, rpm_angle );
	skin_standard_draw_temperature_gauge( TEMP_INSTRUMENT_POSX, TEMP_INSTRUMENT_POSY, gauge_colour, red_colour, niddle_colour, temperature_angle );
	skin_standard_draw_fuel_gauge( FUEL_INSTRUMENT_POSX, FUEL_INSTRUMENT_POSY, gauge_colour, red_colour, niddle_colour, fuel_angle, low_fuel_flag );
}

static void skin_standard_update_data( void )
{
    if( speed_value <= 150 ) speed_angle = speed_value * 8;
    if( rpm_value <= 7250 ) rpm_angle = ( rpm_value * 128 ) / 800;
    
    if( temperature_value > 300 ) temperature_angle = ( 300 * 5 ) + 37;
	if( temperature_value < 20 ) temperature_angle = 0;
	else temperature_angle = ( temperature_value * 5 ) + 37;
    
    if( fuel_value > 100 ) fuel_angle = ( ( uint32_t )100 * 1024 ) / 100;
	else if( fuel_value < 0 ) fuel_angle = ( ( uint32_t )0 * 1024 ) / 100;
    else fuel_angle = ( ( uint32_t )fuel_value * 1024 ) / 100;

}

///////////////////////////////////////////////////////////////

static const skin_data_t skin_standard_mph_data = 
{
	ITEM_TYPE_LIST_SKIN,
	SKIN_STANDARD_MPH,
	skin_standard_mph_init,
	skin_standard_process,
	skin_standard_update_warning_index,
    NULL,
	NULL,
};

static const skin_data_t skin_standard_kph_data = 
{
	ITEM_TYPE_LIST_SKIN,
	SKIN_STANDARD_KPH,
	skin_standard_kph_init,
	skin_standard_process,
	skin_standard_update_warning_index,
    NULL,
	NULL,
};

///////////////////////////////////////////////////////////////

DECLARE_LIST_SECTION( skin_standard_mph_data );
DECLARE_LIST_SECTION( skin_standard_kph_data );
