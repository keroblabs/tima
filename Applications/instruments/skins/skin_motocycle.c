#include "system.h"
#include "timer.h"
#include "graphics.h"
#include "draw.h"
#include "pipe.h"
#include "tima_libc.h"
#include "syscomm.h"

#include "obd_control.h"
#include "warning_lights.h"
#include "instruments.h"
#include "gui_setup.h"
#include "icons.h"

///////////////////////////////////////////////////////////////

#define UPDATE_INFO_FAST_MS             200
#define UPDATE_INFO_SLOW_MS             1000
#define OBD_WATCHDOG_MS                 2000

#define TEMP_FLASHING_MS				800
#define BRAKE_FLASHING_MS				300

#define INSTRUMENTS_FONT_TYPE			FONT_12x16	

#define INSTRUMENT_LINE_LEN				26
#define INSTRUMENT_RATIO				200
#define SMALL_GAUGE_RATIO				175

#define INSTRUMENTS_OFFSET_Y			100

#define RPM_INSTRUMENT_POSX				250
#define RPM_INSTRUMENT_POSY				( 140 + INSTRUMENTS_OFFSET_Y )
#define SPEED_INSTRUMENT_POSX			220
#define SPEED_INSTRUMENT_POSY			( 270 + INSTRUMENTS_OFFSET_Y )

#define FUEL_INSTRUMENT_POSX			( 300 + RPM_INSTRUMENT_POSX )
#define FUEL_INSTRUMENT_POSY			RPM_INSTRUMENT_POSY

#define TEMP_INSTRUMENT_POSX			( 300 + RPM_INSTRUMENT_POSX )
#define TEMP_INSTRUMENT_POSY			RPM_INSTRUMENT_POSY

#define RPM_MARK_FACTOR					17
#define RPM_GREEN_UPPER_VALUE			6
#define RPM_GREEN_ANGLE_START			-1024
#define RPM_GREEN_ANGLE_LENGTH			((16*RPM_GREEN_UPPER_VALUE)*RPM_MARK_FACTOR)

#define RPM_GREEN_ANGLE_RANGE			RPM_GREEN_ANGLE_START, RPM_GREEN_ANGLE_START+RPM_GREEN_ANGLE_LENGTH
#define RPM_GREEN_LONG_MARK_STEP		(RPM_GREEN_ANGLE_LENGTH/(4*RPM_GREEN_UPPER_VALUE))
#define RPM_GREEN_SHORT_MARK_STEP		(RPM_GREEN_ANGLE_LENGTH/(16*RPM_GREEN_UPPER_VALUE))
#define RPM_GREEN_NUMBER_STEP			(RPM_GREEN_ANGLE_LENGTH/RPM_GREEN_UPPER_VALUE)

#define RPM_RED_ANGLE_RANGE				RPM_GREEN_ANGLE_START+RPM_GREEN_ANGLE_LENGTH, RPM_GREEN_ANGLE_START+2048

#define WARNING_POSX					RPM_INSTRUMENT_POSX+145
#define WARNING_POSY					RPM_INSTRUMENT_POSY-70

#define WARNING_TOP_POSX				RPM_INSTRUMENT_POSX+150
#define WARNING_TOP_POSY				RPM_INSTRUMENT_POSY-100

#define SLOT_SPACE_WIDTH				80
#define SLOT_SPACE_HEIGHT				50

#define SLOT_WARNING_POS(x,y)			( WARNING_POSX + ( x * SLOT_SPACE_WIDTH ) ), ( WARNING_POSY + ( y * SLOT_SPACE_HEIGHT ) )

#define SLOT0_WARNING_POS				SLOT_WARNING_POS(0,0)
#define SLOT1_WARNING_POS				SLOT_WARNING_POS(0,1)
#define SLOT2_WARNING_POS				SLOT_WARNING_POS(0,2)

#define SLOT3_WARNING_POS				SLOT_WARNING_POS(1,0)
#define SLOT4_WARNING_POS				SLOT_WARNING_POS(1,1)
#define SLOT5_WARNING_POS				SLOT_WARNING_POS(1,2)

#define SLOT6_WARNING_POS				SLOT_WARNING_POS(2,0)
#define SLOT7_WARNING_POS				SLOT_WARNING_POS(2,1)
#define SLOT8_WARNING_POS				SLOT_WARNING_POS(2,2)

#define SLOT_WARNING_POS_MAX			0xFFFF, 0xFFFF

#define INFOCENTRE_POS_SIZE				270, 10, 260, 100, 10

#define INSTRUMENT_CONTROL_WARNING_HIDE( slot, icon ) 			graphics_fill_box( slot, SLOT_SPACE_WIDTH, SLOT_SPACE_HEIGHT, APP_RGB(0,0,0) )

#define INSTRUMENT_CONTROL_WARNING_SHOW( slot, icon ) 			graphics_show_bitmap( slot, icon, TRUE )

#define INSTRUMENT_CONTROL_WARNING_STATE( slot, icon, state ) 	if( state ) \
																	graphics_show_bitmap( slot, icon, TRUE );\
																else\
																	graphics_fill_box( slot, SLOT_SPACE_WIDTH, SLOT_SPACE_HEIGHT, APP_RGB(0,0,0) )

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////

static uint16_t gauge_colour;
static uint16_t niddle_colour;
static uint16_t red_colour;
static uint16_t back_colour;

static int curr_speed_angle;
static bool_t update_speed;

static int curr_rpm_angle;
static bool_t update_rpm;
static bool_t update_rpm_arc;
static bool_t update_dashboard;

static bool_t update_infocentre;

static uint32_t raw_odometer_value;
static uint32_t main_odometer;
static uint32_t trip_odometer;

static int curr_fuel_angle;
static bool_t update_fuel;
static bool_t prev_low_fuel;

static int curr_temp_angle;
static bool_t update_temp;

static mark_data_t fuel_mark_data;
static mark_data_t temp_mark_data;
static mark_data_t rpm_mark_data;

static uint8_t trip_index;

//static bool_t is_fast_fuel;

static timer_data_t update_rtc_ms;
static timer_data_t update_info_fast;
static timer_data_t update_info_slow;
static timer_data_t obd_watchdog_timer;

static char temp_string[30];

///////////////////////////////////////////////////////////////

void skin_motocycle_draw_instrument_speed( int angle )
{
	int calc_angle;
	uint16_t posx = RPM_INSTRUMENT_POSX-50;
	uint16_t posy = RPM_INSTRUMENT_POSY-(INSTRUMENT_RATIO-80)+35;

	if( angle == curr_speed_angle ) return;
	curr_speed_angle = angle;
    
	// adjust speed according to the wheel size, increase the factor
	// to increase the speed reading
	calc_angle = ( 200 * angle ) >> 8;

    draw_font_init( INSTRUMENTS_FONT_TYPE );

	if( instruments_show_mph )
		show_large_number( posx, posy, calc_angle, "MPH", gauge_colour );
	else
		show_large_number( posx, posy, ( 410 * calc_angle ) >> 8, "KM/H", gauge_colour );

	update_speed = TRUE;
}

void skin_motocycle_draw_instrument_rpm( int rpm_value )
{
	#define RPM_START_ANGLE -1024
    #define FAST_RPM_UPDATE 1
    
#ifndef FAST_RPM_UPDATE
	int angle_offset;
#endif
    
	int angle;
	uint16_t num;

    angle = ( ( rpm_value / 50 ) * rpm_mark_data.angle_step ) - 1024;
    if( angle < -1024 ) angle = -1024;
    if( angle > 936 ) angle = 936;

	if( angle == curr_rpm_angle ) return;
	draw_font_init( INSTRUMENTS_FONT_TYPE );

#ifdef FAST_RPM_UPDATE
    curr_rpm_angle = angle;
#else
	angle_offset = ABS( angle - curr_rpm_angle );
	if( angle_offset > 30 ) angle_offset = 30;

	if( angle_offset < 5 )
	{
		curr_rpm_angle = angle;
	}
	else
	{
		if( angle > curr_rpm_angle ) curr_rpm_angle += angle_offset;
		else curr_rpm_angle -= angle_offset;
	}
#endif

    rpm_mark_data.angle3 = curr_rpm_angle;
    draw_instrument_marks_ex( RPM_INSTRUMENT_POSX, RPM_INSTRUMENT_POSY,
                              INSTRUMENT_RATIO+(INSTRUMENT_NUM_DIST*3)-20, &rpm_mark_data );
    
	if( update_rpm == FALSE )
	{
		num = 0;
		draw_font_init( FONT_12x16 );

		num = draw_instrument_numbers( RPM_INSTRUMENT_POSX, RPM_INSTRUMENT_POSY,
                                       INSTRUMENT_RATIO-(INSTRUMENT_NUM_DIST*4)-20,
                                       rpm_mark_data.colour1,
                                       RPM_GREEN_ANGLE_RANGE,
                                       RPM_GREEN_NUMBER_STEP+10, num, 1 );

		draw_instrument_numbers( RPM_INSTRUMENT_POSX,
                                 RPM_INSTRUMENT_POSY,
                                 INSTRUMENT_RATIO-(INSTRUMENT_NUM_DIST*4)-20,
                                 rpm_mark_data.colour2,
                                 RPM_RED_ANGLE_RANGE,
                                 RPM_GREEN_NUMBER_STEP+10, num, 1 );
	}

	update_rpm = TRUE;
}

void skin_motocycle_draw_fuel_gauge( int fuel_level, bool_t low )
{
	int angle;
	int draw_angle;

#define FUEL_ICON_GAUGE_POS		FUEL_INSTRUMENT_POSX+145, FUEL_INSTRUMENT_POSY+160
#define FUEL_GRAPH_POS
    
    if( ( prev_low_fuel != TRUE ) && ( low == TRUE ) )
    {
        graphics_fill_box( FUEL_ICON_GAUGE_POS, fuel_blue_icon_bitmap.width, fuel_blue_icon_bitmap.width, back_colour );
        graphics_show_bitmap( FUEL_ICON_GAUGE_POS, &fuel_icon_bitmap, TRUE );
        prev_low_fuel = TRUE;
    }
    else if( ( prev_low_fuel != FALSE ) && ( low == FALSE ) )
    {
        graphics_fill_box( FUEL_ICON_GAUGE_POS, fuel_icon_bitmap.width, fuel_icon_bitmap.width, back_colour );
        graphics_show_bitmap( FUEL_ICON_GAUGE_POS, &fuel_blue_icon_bitmap, TRUE );
        prev_low_fuel = FALSE;
    }
    
    if( fuel_level == -1 ) return;
    
    if( fuel_level > 100 ) angle = 896;
	else if( fuel_level < 0 ) angle = 0;
    else angle = ( ( uint32_t )fuel_level * 896 ) / 100;

	if( angle == curr_fuel_angle ) return;

	if( angle > 890 ) angle = 890;
	if( angle < 0 ) angle = 0;
    curr_fuel_angle = angle;

	draw_font_init( INSTRUMENTS_FONT_TYPE );

    draw_angle = 3072-curr_fuel_angle;

	fuel_mark_data.angle3 = draw_angle;
	draw_instrument_marks_ex( FUEL_INSTRUMENT_POSX, FUEL_INSTRUMENT_POSY, SMALL_GAUGE_RATIO+(INSTRUMENT_NUM_DIST*3), &fuel_mark_data );

	if( update_fuel == FALSE )
	{
		draw_font_init( FONT_12x16 );
		draw_instrument_print( FUEL_INSTRUMENT_POSX, FUEL_INSTRUMENT_POSY, SMALL_GAUGE_RATIO-(INSTRUMENT_NUM_DIST*3), gauge_colour, "F", 2176 );
		draw_instrument_print( FUEL_INSTRUMENT_POSX, FUEL_INSTRUMENT_POSY, SMALL_GAUGE_RATIO-(INSTRUMENT_NUM_DIST*3), gauge_colour, "1/2", 2624 );
		draw_instrument_print( FUEL_INSTRUMENT_POSX, FUEL_INSTRUMENT_POSY, SMALL_GAUGE_RATIO-(INSTRUMENT_NUM_DIST*3), red_colour, "E", 3072 );
	}

	update_fuel = TRUE;
}

void skin_motocycle_draw_temperature_gauge( int input_temperature )
{
	int angle;

	if( input_temperature < 35 ) angle = temp_mark_data.start_angle;
	else if( input_temperature > 160 ) angle = temp_mark_data.end_angle;
	else angle = temp_mark_data.start_angle - ( ( input_temperature - 28 ) * 7 );

	if( angle == curr_temp_angle ) return;

	draw_font_init( INSTRUMENTS_FONT_TYPE );

	curr_temp_angle = angle;

	temp_mark_data.angle3 = curr_temp_angle;
	draw_instrument_marks_ex( TEMP_INSTRUMENT_POSX, TEMP_INSTRUMENT_POSY, SMALL_GAUGE_RATIO+(INSTRUMENT_NUM_DIST*3), &temp_mark_data );

	if( update_temp == FALSE )
	{
		draw_font_init( FONT_12x16 );
		draw_instrument_print( TEMP_INSTRUMENT_POSX, TEMP_INSTRUMENT_POSY, SMALL_GAUGE_RATIO-(INSTRUMENT_NUM_DIST*3), temp_mark_data.colour1, "C", temp_mark_data.start_angle );
		draw_instrument_print( TEMP_INSTRUMENT_POSX, TEMP_INSTRUMENT_POSY, SMALL_GAUGE_RATIO-(INSTRUMENT_NUM_DIST*3), temp_mark_data.colour2, "H", temp_mark_data.end_angle );

		graphics_show_bitmap( TEMP_INSTRUMENT_POSX+150, TEMP_INSTRUMENT_POSY-200, &temp_blue_icon_bitmap, TRUE );
	} 

	update_temp = TRUE;
}

void skin_motocycle_reset_trip_odometer( void )
{
    trip_list[trip_index] = raw_odometer_value;
}

void skin_motocycle_show_odometer_main( uint32_t main )
{
    char trip_index_str[10];
    
	uint16_t posx = RPM_INSTRUMENT_POSX-40;
	uint16_t posy = RPM_INSTRUMENT_POSY+30;
    
    uint32_t main_value;
    uint32_t trip_value;

    if( main != NEG_U32 )
    {
        raw_odometer_value = main;
    }
    
    main_value = raw_odometer_value / ODOMETER_CALIBRATION;
    trip_value = ( ( ( raw_odometer_value - trip_list[trip_index] ) * 10 ) / ODOMETER_CALIBRATION ); // % 10000;
    
    while( trip_value >= 10000 ) trip_value -= 10000;

	if( ( main_value != main_odometer ) || ( main == NEG_U32 ) )
	{
		main_odometer = main_value;
		show_odometer_total_number( posx, posy+40, (uint32_t)(main_odometer), 1, gauge_colour );
	}
    
    if( ( trip_value != trip_odometer ) || ( main == NEG_U32 ) )
    {
        draw_font_init( FONT_8x8 );
        sprintk( trip_index_str, "trip %d", trip_index+1 );
        draw_text( posx+90, posy-16, trip_index_str, gauge_colour, back_colour );

        trip_odometer = trip_value;
        show_odometer_total_number( posx, posy, (trip_odometer), 10, gauge_colour );
    }
}

void skin_motocycle_show_brake_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT3_WARNING_POS, &brake_icon_bitmap, state );
}

void skin_motocycle_show_fullbeam_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT2_WARNING_POS, &full_icon_bitmap, state );
}

void skin_motocycle_show_heaplamp_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT2_WARNING_POS, &lamp_icon_bitmap, state );
}

void skin_motocycle_show_foglight_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT2_WARNING_POS-4, &fog_icon_bitmap, state );
}

void skin_motocycle_show_engine_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT1_WARNING_POS, &engine_icon_bitmap, state );
}

void skin_motocycle_show_battery_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT5_WARNING_POS, &battery_icon_bitmap, state );
}

void skin_motocycle_show_oil_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT6_WARNING_POS, &oil_icon_bitmap, state );
}

void skin_motocycle_show_temperature_warning( bool_t state )
{
	INSTRUMENT_CONTROL_WARNING_STATE( SLOT8_WARNING_POS, &temp_icon_bitmap, state );
}

void skin_motocycle_show_indicator_warning( uint8_t state )
{
	uint8_t local_state;
	local_state = state & 0x0F;

	if( local_state == 0 )
	{
		INSTRUMENT_CONTROL_WARNING_HIDE( SLOT4_WARNING_POS, &hazard_icon_bitmap );
	}
	else if( state & INDICATOR_HAZARD_FLAG )
	{
		INSTRUMENT_CONTROL_WARNING_SHOW( SLOT4_WARNING_POS, &hazard_icon_bitmap );
	}
	else if( ( state & 0x07 ) == INDICATOR_LEFT_FLAG )
	{
		INSTRUMENT_CONTROL_WARNING_SHOW( SLOT4_WARNING_POS, &indicator_left_icon_bitmap );
	}
	else if( ( state & 0x07 ) == INDICATOR_RIGHT_FLAG )
	{
		INSTRUMENT_CONTROL_WARNING_SHOW( SLOT4_WARNING_POS, &indicator_right_icon_bitmap );
	}
}

void skin_motocycle_infocentre2_show_text( uint16_t posx, uint16_t posy,
                                           uint16_t width, uint16_t height, uint16_t corner_ratio,
                                           uint16_t line, const char * text,
                                           uint16_t colour )
{
	if( line < 1 ) line = 1;
	posy += ((INSTRUMENT_RATIO-80)*2)+15+height;
	draw_font_init( FONT_12x16 );
	draw_text( posx+corner_ratio-2, posy+9+((line-1)*22), ( char * )text, colour, back_colour );
}

void skin_motocycle_infocentre_show_text( uint16_t posx, uint16_t posy,
                                          uint16_t width, uint16_t height, uint16_t corner_ratio,
										  uint16_t line, const char * text,
										  uint16_t colour )
{
	if( line < 1 ) line = 1;
	draw_font_init( FONT_12x16 );
	draw_text( posx+corner_ratio-2, posy+9+((line-1)*22), ( char * )text, colour, back_colour );
}

void skin_motocycle_show_infocentre( uint16_t posx, uint16_t posy,
                                     uint16_t width, uint16_t height,
                                     uint16_t corner_ratio, uint16_t colour )
{
	//char ts_pos_str[18];

	if( update_infocentre == FALSE )
	{
		update_infocentre = TRUE;
		draw_instrument_infocentre_box( posx, posy, width, height, corner_ratio, colour );
		draw_instrument_infocentre_box( posx, posy+((INSTRUMENT_RATIO-80)*2)+15+height, width, height, corner_ratio, colour );

		/*

		12345678901234567890

		NO MESSAGE
		00 MESSAGES
		LOW WASH LEVEL
		ENGINE FAILURE
		SYSTEM FAILURE
		FAILURE CODE 0000
		LEFT DOOR OPEN
		RIGHT DOOR OPEN
		BONNET OPEN
		BOOT OPEN
		BOTH DOORS OPEN
		DOOR AND BOOT OPEN
		DOORS AND BOOT OPEN
		DOOR AND BONNET OPEN
		DOORS / BONNET OPEN
		BONNET AND BOOT OPEN
		EVERYTHING OPEN!!

		00.00 L USED
		00.00 L IN TANK
		000 KM TO EMPTY TANK
		00.00 KM/L INSTANT
		00.00 KM/L AVERAGE
		000.00 KM TRIP 1
		000.00 KM TRIP 5

		00:00  cruise   00.C

		*/

		/*
		NVRAM:
		odometer counter
		fuel flow counter
		distance 1...5

		*/

		//draw_font_init( FONT_12x16 ); //            12345678901234567890

		//draw_text( posx+corner_ratio-2, posy+9,    "InfoCentre line 0001", colour, back_colour );
		//draw_text( posx+corner_ratio-2, posy+9+22, "InfoCentre line 0002", colour, back_colour );
		//draw_text( posx+corner_ratio-2, posy+9+44, "InfoCentre line 0003", colour, back_colour );
		//draw_text( posx+corner_ratio-2, posy+9+66, "00:00      cruise on", colour, back_colour );
		//draw_text( posx+corner_ratio-2, posy+9+66, "00:00      ", colour, back_colour );
	}
}

static void skin_motocycle_show_dashboard( void )
{
	uint16_t posx = RPM_INSTRUMENT_POSX;
	uint16_t posy = RPM_INSTRUMENT_POSY;

	if( update_dashboard == FALSE )
	{
		update_dashboard = TRUE;

		draw_instruments_arc( posx, posy, (INSTRUMENT_RATIO-80)-5, -1024, 1024, niddle_colour );
		draw_instruments_arc( posx+300, posy, (INSTRUMENT_RATIO-80)-5, 1024, 3072, niddle_colour );
		draw_line( posx, posy-(INSTRUMENT_RATIO-80)+5, posx+300, posy-(INSTRUMENT_RATIO-80)+5, niddle_colour );
		draw_line( posx, posy+(INSTRUMENT_RATIO-80)-5, posx+300, posy+(INSTRUMENT_RATIO-80)-5, niddle_colour );
	}
}

static void skin_motocycle_update_warning_index( uint32_t index, uint32_t mask, bool_t use_timed )
{
}

static void skin_motocycle_init( void )
{
#if defined __WIN32
	gauge_colour = APP_RGB( 32,32,32 );
	niddle_colour = APP_RGB( 32,16,0 );
	red_colour = APP_RGB( 32, 0, 0 );
#else
	gauge_colour = APP_RGB( 200,200,200 );
	niddle_colour = APP_RGB( 255,192,0 );
	red_colour = APP_RGB( 255, 0, 0 );
#endif

	back_colour = instruments_get_back_colour();

	timer_Start( &update_rtc_ms, 2000 );
    timer_Start( &update_info_fast, UPDATE_INFO_FAST_MS );
    timer_Start( &update_info_slow, UPDATE_INFO_SLOW_MS );
    timer_Start( &obd_watchdog_timer, OBD_WATCHDOG_MS );

	trip_index = 0;
	curr_speed_angle = -1;
	update_speed = FALSE;

	update_rpm = FALSE;
	update_rpm_arc = FALSE;
	update_dashboard = FALSE;

	update_infocentre = FALSE;

    raw_odometer_value = 0;
	main_odometer = 0;
	trip_odometer = 0;

	curr_fuel_angle = -1;
	update_fuel = FALSE;
	prev_low_fuel = TRUE+1;

	curr_temp_angle = -1;
	update_temp = FALSE;

	fuel_mark_data.start_angle = 3073;
	fuel_mark_data.end_angle = 2176;
	fuel_mark_data.angle_step = -14;
	fuel_mark_data.angle1 = 2960;
	fuel_mark_data.angle3 = fuel_mark_data.end_angle;
    fuel_mark_data.len1 = 20;
    fuel_mark_data.len2 = 35;
    fuel_mark_data.marks = 5;
	fuel_mark_data.colour1 = red_colour;
	fuel_mark_data.colour2 = gauge_colour;
	fuel_mark_data.colour3 = APP_RGB(64,64,64);

	temp_mark_data.start_angle = 1921;
	temp_mark_data.end_angle = 1024;
	temp_mark_data.angle_step = -14;
	temp_mark_data.angle1 = 1024+224;
	temp_mark_data.angle3 = temp_mark_data.end_angle;
	temp_mark_data.len1 = 20;
    temp_mark_data.len2 = 35;
    temp_mark_data.marks = 5;
    temp_mark_data.colour1 = gauge_colour;
	temp_mark_data.colour2 = red_colour;
	temp_mark_data.colour3 = APP_RGB(64,64,64);
    
    rpm_mark_data.start_angle = -1024;
    rpm_mark_data.end_angle = 1024;
    rpm_mark_data.angle_step = 14;
    rpm_mark_data.angle1 = 1024-512;
    rpm_mark_data.angle3 = temp_mark_data.end_angle;
    rpm_mark_data.len1 = 20;
    rpm_mark_data.len2 = 35;
    rpm_mark_data.marks = 8;
    rpm_mark_data.colour1 = gauge_colour;
    rpm_mark_data.colour2 = red_colour;
    rpm_mark_data.colour3 = APP_RGB(64,64,64);
    
	curr_rpm_angle = rpm_mark_data.start_angle-1;

	skin_motocycle_draw_instrument_speed( 0 );
    skin_motocycle_draw_temperature_gauge( 0 );
    skin_motocycle_draw_instrument_rpm( 0 );
    skin_motocycle_draw_fuel_gauge( 0, FALSE );
    skin_motocycle_show_dashboard();
    
    skin_motocycle_show_odometer_main(NEG_U32);

    skin_motocycle_show_infocentre( INFOCENTRE_POS_SIZE, gauge_colour );
}

static void skin_motocycle_process( void )
{
	date_time_t * time_now;
	char time_now_str[30];
    obd_resp_t * p_resp;
    uint32_t mem;

	if( timer_Check( &update_rtc_ms ) )
	{
		timer_Reload( &update_rtc_ms );
		time_now = tima_localtime( timer_Now() );
        mem = sys_heap_used();
        //                         12345678901234567890
		//sprintk( time_now_str, "%2d:%02d mem=%d  ", time_now->tm_hour, time_now->tm_min, mem );
		sprintk( time_now_str, "%2d:%02d", time_now->tm_hour, time_now->tm_min );
		skin_motocycle_infocentre_show_text( INFOCENTRE_POS_SIZE, 4, ( const char * )time_now_str, gauge_colour );
	}
    
    if( timer_Check( &obd_watchdog_timer ) )
    {
        timer_Expire( &update_info_slow );
        timer_Expire( &update_info_fast );
        timer_Reload( &obd_watchdog_timer );
    }
    
    if( timer_Check( &update_info_fast ) )
    {
        // on fast timeout, request speed
        timer_Stop( &update_info_fast );
        obd_control_send_request( OBD_REQUEST_SPEED );
    }

    if( timer_Check( &update_info_slow ) )
    {
        // on slow timeout, request temperature
        timer_Stop( &update_info_slow );
        obd_control_send_request( OBD_REQUEST_TEMPERATURE );
    }
    
    if( ( p_resp = obd_control_get_response() ) != NULL )
    {
        switch( p_resp->request )
        {
            case OBD_REQUEST_SPEED:
                // on speed response, request RPM
            	if( p_resp->response == OBD_RESULT_OK )
                    skin_motocycle_draw_instrument_speed( p_resp->value );
                obd_control_remove_response();
                obd_control_send_request( OBD_REQUEST_RPM );
                timer_Reload( &obd_watchdog_timer );
                break;

            case OBD_REQUEST_RPM:
                // on RPM response, restart fast timer
                if( p_resp->response == OBD_RESULT_OK )
                    skin_motocycle_draw_instrument_rpm( p_resp->value );
                obd_control_remove_response();
                timer_Reload( &update_info_fast );
                timer_Reload( &obd_watchdog_timer );
                break;

            case OBD_REQUEST_TEMPERATURE:
                // on temperature response, restart slow timer
                if( p_resp->response == OBD_RESULT_OK )
                    skin_motocycle_draw_temperature_gauge( p_resp->value );
                obd_control_remove_response();
                timer_Reload( &update_info_slow );
                
                sprintk( temp_string, "Temp   %d     ", p_resp->value );
                instruments_show_information( temp_string, SETUP_PARAM_DISPLAY2_LINE4 );
                timer_Reload( &obd_watchdog_timer );
                break;
        }
    }
}

uint8_t skin_motocycle_tts_event( uint16_t posx, uint16_t posy, uint16_t posz )
{
    uint8_t index;
    
    if( ( posx < 80 ) && ( posy < 80 ) )
    {
        index = EDIT_BUTTON_B1;
    }
    else if( ( posx < 80 ) && ( posy > 400 ) )
    {
        index = EDIT_BUTTON_B2;
    }
    else if( ( posx > 720 ) && ( posy < 80 ) )
    {
        index = EDIT_BUTTON_B3;
    }
    else if( ( posx > 720 ) && ( posy > 400 ) )
    {
        index = EDIT_BUTTON_B4;
    }
    else
    {
        index = 0;
    }
    
    return index;
}

void skin_motocycle_show_button( uint16_t posx, uint16_t posy, uint16_t width, uint16_t heigh, uint16_t colour, const char * text )
{
	uint16_t text_width;
	uint16_t text_height;
	uint16_t text_posx;
	uint16_t text_posy;

	width--;
	heigh--;

	draw_line( posx, posy, posx+width, posy, colour );
	draw_line( posx, posy+heigh, posx+width, posy+heigh, colour );
	draw_line( posx, posy, posx, posy+heigh, colour );
	draw_line( posx+width, posy, posx+width, posy+heigh, colour );

	draw_font_init(FONT_8x8);
	text_width = draw_get_text_width( (char*)text );
	text_height = draw_get_text_height( (char*)text );

	text_posx = ( ( width - text_width ) >> 1 ) + posx;
	text_posy = ( ( heigh - text_height ) >> 1 ) + posy;
	draw_text( text_posx, text_posy, ( char * )text, colour, APP_RGB(0,0,0) );
}

void skin_motocycle_erase_button( uint16_t posx, uint16_t posy, uint16_t width, uint16_t heigh )
{
	graphics_fill_box( posx, posy, width, heigh, APP_RGB(0,0,0) );
}

void skin_motocycle_display_event( void * p_data, uint32_t param )
{
	//char * p_str;

#define BUTTON1_WIDTH	80
#define BUTTON1_HEIGHT	80
#define BUTTON2_WIDTH	70
#define BUTTON2_HEIGHT	70

#define BUTTON1_SIZE	BUTTON1_WIDTH,BUTTON1_HEIGHT
#define BUTTON2_SIZE	BUTTON2_WIDTH,BUTTON2_HEIGHT
#define BUTTON_B1_POS	0,0
#define BUTTON_B2_POS	0,(480-BUTTON1_HEIGHT)
#define BUTTON_B3_POS	(800-BUTTON2_WIDTH),0
#define BUTTON_B4_POS	(800-BUTTON2_WIDTH),(480-BUTTON2_HEIGHT)
#define BUTTON_B1		BUTTON_B1_POS, BUTTON1_SIZE
#define BUTTON_B2		BUTTON_B2_POS, BUTTON1_SIZE
#define BUTTON_B3		BUTTON_B3_POS, BUTTON2_SIZE
#define BUTTON_B4		BUTTON_B4_POS, BUTTON2_SIZE

#define BUTTON_SHOW_COLOUR		APP_RGB( 100, 100, 100 )
#define BUTTON_SELECT_COLOUR	APP_RGB( 255, 255, 0 )

	switch( param )
	{
		case SETUP_PARAM_UPDATE_DATETIME: timer_Expire( &update_rtc_ms ); break;
		case SETUP_PARAM_SET_KPH: instruments_show_mph = FALSE; break;
		case SETUP_PARAM_SET_MPH: instruments_show_mph = TRUE; break;

		case SETUP_PARAM_INCR_TRIP_INDEX:
			if( ++trip_index >= ODOMETER_TRIP_COUNT ) trip_index = 0;
            skin_motocycle_show_odometer_main(NEG_U32);
			break;

		case SETUP_PARAM_RESET_TRIP:
            skin_motocycle_reset_trip_odometer();
			break;
            
        case SETUP_PARAM_LOAD_TRIP:
        {
            uint32_t local_trip_value[3];
            memcpy( local_trip_value, p_data, sizeof( local_trip_value ) );
            trip_list[0] = local_trip_value[0];
            trip_list[1] = local_trip_value[1];
            trip_list[2] = local_trip_value[2];
            break;
        }
            
        case SETUP_PARAM_SAVE_TRIP:
        {
            uint32_t local_trip_value[3];
            local_trip_value[0] = trip_list[0];
            local_trip_value[1] = trip_list[1];
            local_trip_value[2] = trip_list[2];
            memcpy( p_data, local_trip_value, sizeof( local_trip_value ) );
            break;
        }
            
        case SETUP_PARAM_SHOW_BUTTON_B1: skin_motocycle_show_button( BUTTON_B1, BUTTON_SHOW_COLOUR, ( char * )p_data ); break;
		case SETUP_PARAM_SHOW_BUTTON_B2: skin_motocycle_show_button( BUTTON_B2, BUTTON_SHOW_COLOUR, ( char * )p_data ); break;
		case SETUP_PARAM_SHOW_BUTTON_B3: skin_motocycle_show_button( BUTTON_B3, BUTTON_SHOW_COLOUR, ( char * )p_data ); break;
		case SETUP_PARAM_SHOW_BUTTON_B4: skin_motocycle_show_button( BUTTON_B4, BUTTON_SHOW_COLOUR, ( char * )p_data ); break;

		case SETUP_PARAM_SHOW_SELECT_B1: skin_motocycle_show_button( BUTTON_B1, BUTTON_SELECT_COLOUR, ( char * )p_data ); break;
		case SETUP_PARAM_SHOW_SELECT_B2: skin_motocycle_show_button( BUTTON_B2, BUTTON_SELECT_COLOUR, ( char * )p_data ); break;
		case SETUP_PARAM_SHOW_SELECT_B3: skin_motocycle_show_button( BUTTON_B3, BUTTON_SELECT_COLOUR, ( char * )p_data ); break;
		case SETUP_PARAM_SHOW_SELECT_B4: skin_motocycle_show_button( BUTTON_B4, BUTTON_SELECT_COLOUR, ( char * )p_data ); break;

		case SETUP_PARAM_ERASE_B1: skin_motocycle_erase_button( BUTTON_B1 ); break;
		case SETUP_PARAM_ERASE_B2: skin_motocycle_erase_button( BUTTON_B2 ); break;
		case SETUP_PARAM_ERASE_B3: skin_motocycle_erase_button( BUTTON_B3 ); break;
		case SETUP_PARAM_ERASE_B4: skin_motocycle_erase_button( BUTTON_B4 ); break;

		case SETUP_PARAM_DISPLAY1_LINE1: skin_motocycle_infocentre_show_text( INFOCENTRE_POS_SIZE, 1, ( const char * )p_data, gauge_colour ); break;
		case SETUP_PARAM_DISPLAY1_LINE2: skin_motocycle_infocentre_show_text( INFOCENTRE_POS_SIZE, 2, ( const char * )p_data, gauge_colour ); break;
		case SETUP_PARAM_DISPLAY1_LINE3: skin_motocycle_infocentre_show_text( INFOCENTRE_POS_SIZE, 3, ( const char * )p_data, gauge_colour ); break;
		case SETUP_PARAM_DISPLAY1_LINE4: skin_motocycle_infocentre_show_text( INFOCENTRE_POS_SIZE, 4, ( const char * )p_data, gauge_colour ); break;

		case SETUP_PARAM_DISPLAY2_LINE1: skin_motocycle_infocentre2_show_text( INFOCENTRE_POS_SIZE, 1, ( const char * )p_data, gauge_colour ); break;
		case SETUP_PARAM_DISPLAY2_LINE2: skin_motocycle_infocentre2_show_text( INFOCENTRE_POS_SIZE, 2, ( const char * )p_data, gauge_colour ); break;
		case SETUP_PARAM_DISPLAY2_LINE3: skin_motocycle_infocentre2_show_text( INFOCENTRE_POS_SIZE, 3, ( const char * )p_data, gauge_colour ); break;
		case SETUP_PARAM_DISPLAY2_LINE4: skin_motocycle_infocentre2_show_text( INFOCENTRE_POS_SIZE, 4, ( const char * )p_data, gauge_colour ); break;
	}
}

///////////////////////////////////////////////////////////////

static const skin_data_t skin_motocycle_data =
{
	ITEM_TYPE_LIST_SKIN,
	SKIN_MOTOCYCLE_MPH,
	skin_motocycle_init,
	skin_motocycle_process,
	skin_motocycle_update_warning_index,
    skin_motocycle_tts_event,
	skin_motocycle_display_event
};

///////////////////////////////////////////////////////////////

DECLARE_LIST_SECTION( skin_motocycle_data );

