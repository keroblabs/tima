#ifndef __instruments_h__
#define __instruments_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

enum
{
	SKIN_MOTOCYCLE_MPH,
	SKIN_MOTOCYCLE_KPH,
	SKIN_STANDARD_MPH,
	SKIN_STANDARD_KPH,
};

enum
{
	INDICATOR_NONE_FLAG,
	INDICATOR_LEFT_FLAG,
	INDICATOR_RIGHT_FLAG,
	INDICATOR_HAZARD_FLAG = 0x10,
};

///////////////////////////////////////////////////////////////

#define ITEM_TYPE_LIST_SKIN				1000
#define INSTRUMENT_NUM_DIST				10
#define ODOMETER_CALIBRATION            3240
#define ODOMETER_TRIP_COUNT				3

/*
enum
{
	GAUGE_SPEED,
	GAUGE_RPM,
	GAUGE_TEMP,
	GAUGE_FUEL,

	LIGHT_BATTERY,
	LIGHT_OIL,
	LIGHT_BRAKE,
	LIGHT_LOWBEAM,
	LIGHT_FULLBEAM,
	LIGHT_SIDES,
	LIGHT_FOG,

	LIGHT_ECU,

	LIGHT_FUEL,
	LIGHT_TEMP,
	LIGHT_INDICATOR,
	LIGHT_HAZARD,
};
*/

///////////////////////////////////////////////////////////////

typedef struct _skin_data_t_
{
	uint32_t item_type;
	uint32_t skin_type;

	void ( *skin_init )( void );
	void ( *skin_process )( void );
	void ( *update_warning_update )( uint32_t index, uint32_t mask, bool_t use_timed );

    uint8_t ( *tts_event )( uint16_t posx, uint16_t posy, uint16_t posz );
	void ( *display_event )( void * p_data, uint32_t mode );

} skin_data_t;

typedef struct _draw_niddle_data_t
{
	uint16_t posx;
	uint16_t posy;

	bool_t draw_circle;
	uint16_t circle_ratio;
	uint16_t circle_colour;

	bool_t end_niddle;
	uint16_t niddle_len;
	uint16_t niddle_width;
	uint16_t niddle_colour;

	uint16_t * backup;

} niddle_data_t;

typedef struct _mark_data_t_
{
	int start_angle;
	int end_angle;
	int angle_step;
	int angle1;
	int angle3;

	uint16_t len1;
    uint16_t len2;
    uint16_t marks;

	uint16_t colour1;
	uint16_t colour2;
	uint16_t colour3;

} mark_data_t;

///////////////////////////////////////////////////////////////

extern uint16_t niddle_speed_backup[];
extern uint16_t niddle_rpm_backup[];

extern uint16_t niddle_temp_backup[];
extern uint16_t niddle_fuel_backup[];

extern bool_t instruments_show_mph;

extern int speed_value;
extern int rpm_value;
extern int temperature_value;
extern int fuel_value;
extern uint32_t odometer_value;
extern uint32_t trip_list[];

///////////////////////////////////////////////////////////////

void instruments_init( void );
void instruments_process( void );

///////////////////////////////////////////////////////////////

void draw_instrument_marks( uint16_t posx, uint16_t posy,
						    uint16_t ratio, uint16_t len, uint16_t colour,
							int start_angle, int end_angle, int angle_step );

void draw_instrument_marks_ex( uint16_t posx, uint16_t posy, uint16_t ratio, mark_data_t * data );

void draw_instrument_value( uint16_t posx, uint16_t posy, uint32_t value, uint16_t fore_colour, uint16_t back_colour );

uint16_t draw_instrument_numbers( uint16_t posx, uint16_t posy,
								  uint16_t ratio, uint16_t colour,
								  int start_angle, int end_angle, int angle_step,
								  uint16_t start_num, uint16_t num_step );

void draw_instrument_print( uint16_t posx, uint16_t posy,
							uint16_t ratio, uint16_t colour,
							char * info, int angle );

uint16_t draw_7seg_digits( uint16_t posx, uint16_t posy, char * value, uint16_t colour, bool_t update, void * data );

void draw_instruments_arc( uint16_t posx, uint16_t posy, uint16_t ratio, int start_angle, int end_angle, uint16_t colour );
void draw_erase_niddle( uint16_t posx, uint16_t posy, uint16_t width, uint16_t len, uint16_t rad, uint16_t * backup );
void draw_erase_niddle_ex( niddle_data_t * data, uint16_t rad );
void draw_niddle( uint16_t posx, uint16_t posy, uint16_t width, uint16_t len, uint16_t rad, uint16_t colour, uint16_t * backup );
void draw_niddle_ex( niddle_data_t * data, uint16_t rad );
void show_small_number( uint16_t posx, uint16_t posy, uint16_t value, char * str_unit, uint16_t colour );
void show_large_number( uint16_t posx, uint16_t posy, uint16_t value, char * str_unit, uint16_t colour );
void show_odometer_total_number( uint16_t posx, uint16_t posy, uint32_t value, uint32_t div, uint16_t colour );
void draw_instrument_infocentre_box( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t corner_ratio, uint16_t colour );

void instruments_set_mph_speed( bool_t state );

uint8_t instruments_get_button( uint16_t posx, uint16_t posy, uint16_t posz );

bool_t instruments_get_mph_speed( void );

uint16_t instruments_get_back_colour( void );

void instruments_select_skin( uint32_t index );

void instruments_show_information( void * p_data, uint32_t param );

void instruments_reset_trip_odometer( uint8_t index );

///////////////////////////////////////////////////////////////

void skin_motocycle_draw_instrument_speed( int angle );
void skin_motocycle_draw_instrument_rpm( int rpm_value );
void skin_motocycle_draw_fuel_gauge( int fuel_level, bool_t low );
void skin_motocycle_draw_temperature_gauge( int input_temperature );
void skin_motocycle_update_odometer( void );
void skin_motocycle_show_odometer_main( uint32_t main );
void skin_motocycle_show_brake_warning( bool_t state );
void skin_motocycle_show_fullbeam_warning( bool_t state );
void skin_motocycle_show_heaplamp_warning( bool_t state );
void skin_motocycle_show_foglight_warning( bool_t state );
void skin_motocycle_show_engine_warning( bool_t state );
void skin_motocycle_show_battery_warning( bool_t state );
void skin_motocycle_show_oil_warning( bool_t state );
void skin_motocycle_show_indicator_warning( uint8_t state );
void skin_motocycle_show_temperature_warning( bool_t state );

///////////////////////////////////////////////////////////////

#endif // __instruments_h__
