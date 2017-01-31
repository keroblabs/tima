#include "system.h"
#include "timer.h"
#include "graphics.h"
#include "draw.h"
#include "pipe.h"
#include "tima_libc.h"
#include "gui_setup.h"
#include "instruments.h"
//#include "setup_data.h"

#include "debug.h"

///////////////////////////////////////////////////////////////

#define INSTRUMENT_TANK_SIZE			25

///////////////////////////////////////////////////////////////////////////

typedef struct _number_7seg_t_
{
	uint16_t width;
	uint16_t height;

	uint16_t vert_width;
	uint16_t vert_height;
	uint16_t horz_width;
	uint16_t horz_height;

	uint16_t dp_offsetx;
	uint16_t dp_offsety;

	const uint8_t * vert_7seg;
	const uint8_t * horz_7seg;

} number_7seg_t;

#define FUEL_LOW_LEVEL_ON_FLAG		0x80
#define FUEL_LOW_LEVEL_OFF_FLAG		0x90

#define FUEL_ADC_FILTER_SIZE		64

#define SEG_A	(1<<0)
#define SEG_B	(1<<1)
#define SEG_C	(1<<2)
#define SEG_D	(1<<3)
#define SEG_E	(1<<4)
#define SEG_F	(1<<5)
#define SEG_G	(1<<6)

#define SEG_C1	(1<<7)
#define SEG_D1	(1<<8)

///////////////////////////////////////////////////////////////////////////


static const uint8_t vert_7seg_xbig[] = 
{
	0x00,0x00,0x00,0xf0,0xf0,0xf0,0xf0,0xff,0xff,0xff,0x00,0x00,0x00,
	0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,
	0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,
	0xff,0xff,0xff,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

static const uint8_t horz_7seg_xbig[] = 
{
	0x00,0x0E,0x1E,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x1F,0x0E,0x06,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};


static const uint8_t vert_7seg_big[] = 
{
	0x00,0x00,0xe0,0xfe,0xff,0xff,0xff,0x1e,
	0xe0,0xfe,0xff,0xff,0xff,0x1f,0x01,0x00,
	0x01,0x03,0x03,0x03,0x01,0x00,0x00,0x00,
};

static const uint8_t horz_7seg_big[] = 
{
	0x00, 0x0E, 0x1E, 0x3F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x1E, 0x0e, 0x00
};

static const uint8_t vert_7seg[] = 
{
	0xF0,
	0xf8,
	0x7f,
	0x0F,
};

static const uint8_t horz_7seg[] = 
{
	0x06, 0x0E, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x07, 0x06,
};

static const number_7seg_t data_7seg_big = 
{
	34, 46,
	8, 18,
	14, 5, 
	2, 36,

	vert_7seg_big,
	horz_7seg_big
};

static const number_7seg_t data_7seg_xbig = 
{
	40, 85,
	13, 34,
	22, 12, 
	2, 36,

	vert_7seg_xbig,
	horz_7seg_xbig
};

static const number_7seg_t data_7seg_small = 
{
	20, 24,
	4, 8,
	11, 4, 
	2, 20,

	vert_7seg,
	horz_7seg
};

static const uint16_t seg_list[] =
{
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_G,
	SEG_C1 | SEG_D1,
	SEG_E | SEG_F | SEG_G | SEG_C | SEG_B,
	SEG_E | SEG_F | SEG_G | SEG_C | SEG_D,
	SEG_C | SEG_D | SEG_A | SEG_F,
	SEG_E | SEG_F | SEG_G | SEG_A | SEG_D,
	SEG_A | SEG_B | SEG_D | SEG_E | SEG_F | SEG_G,
	SEG_C | SEG_D | SEG_E,
	SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
	SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
};

#define SEG_A_POS		posx+(p_data->vert_width/2),						posy+(p_data->horz_height/2)
#define SEG_B_POS		posx,												posy+p_data->horz_height+p_data->vert_height
#define SEG_C_POS		posx+p_data->horz_width+p_data->vert_width,			posy+(p_data->horz_height/2)
#define SEG_D_POS		posx+p_data->horz_width+(p_data->vert_width/2),		posy+p_data->horz_height+p_data->vert_height

#define SEG_C1_POS		posx+(p_data->horz_width+p_data->vert_width)/2,		posy+(p_data->horz_height/2)
#define SEG_D1_POS		posx+(p_data->horz_width/2),						posy+p_data->horz_height+p_data->vert_height

#define SEG_E_POS		posx+(p_data->vert_width+(p_data->vert_width/2)),	posy
#define SEG_F_POS		posx+(p_data->vert_width),							posy+(p_data->horz_height/2)+p_data->vert_height
#define SEG_G_POS		posx+(p_data->vert_width/2),						posy+p_data->horz_height+(p_data->vert_height)*2

#define HORZ_SEGMENT	p_data->horz_width, p_data->horz_height, ( uint8_t * )p_data->horz_7seg
#define VERT_SEGMENT	p_data->vert_width, p_data->vert_height, ( uint8_t * )p_data->vert_7seg

///////////////////////////////////////////////////////////////////////////

//static char str_instruments[100];

static skin_data_t * selected_skin;

#if (defined MAC_OS_EMULATOR || defined WIN32 )
static timer_data_t update_timer;
#endif

//static timer_data_t adc_read_delay;

static uint16_t back_colour;

//static device_t * adc_device;

//static bool_t low_fuel_flag;

///////////////////////////////////////////////////////////////////////////

uint16_t niddle_speed_backup[800];
uint16_t niddle_rpm_backup[800];

uint16_t niddle_temp_backup[300];
uint16_t niddle_fuel_backup[300];

bool_t instruments_show_mph;

int speed_value;
int rpm_value;
int temperature_value;
int fuel_value;

uint32_t odometer_value;
uint32_t trip_list[ODOMETER_TRIP_COUNT];

///////////////////////////////////////////////////////////////////////////

void draw_instruments_arc( uint16_t posx, uint16_t posy, uint16_t ratio, int start_angle, int end_angle, uint16_t colour )
{
    uint16_t x1 = 0, y1 = 0, x2, y2;
    int i;
	bool_t is_start;

	is_start = TRUE;

    for( i = start_angle; i < end_angle; i += 1 )
    {
        x2 = posx - ( uint16_t )( ( cos_table( i ) * ratio ) >> 15 );
        y2 = posy - ( uint16_t )( ( sin_table( i ) * ratio ) >> 15 );
        
		if( is_start == FALSE )
		{
	        draw_line( x1, y1, x2, y2, colour );
		}

		x1 = x2;
		y1 = y2;

		is_start = FALSE;
    }
}

void draw_instrument_infocentre_box( uint16_t posx, uint16_t posy, uint16_t width, uint16_t height, uint16_t corner_ratio, uint16_t colour )
{
	draw_instruments_arc( posx+corner_ratio, posy+corner_ratio, corner_ratio, 0, 1024, colour );
	draw_instruments_arc( posx+width-corner_ratio, posy+corner_ratio, corner_ratio, 1024, 2048, colour );

	draw_instruments_arc( posx+corner_ratio, posy+height-corner_ratio, corner_ratio, -1024, 0, colour );
	draw_instruments_arc( posx+width-corner_ratio, posy+height-corner_ratio, corner_ratio, 2048, 3072, colour );

	draw_line( posx+corner_ratio, posy, posx+width-corner_ratio, posy, colour );
	draw_line( posx+corner_ratio, posy+height, posx+width-corner_ratio, posy+height, colour );

	draw_line( posx, posy+corner_ratio, posx, posy+height-corner_ratio, colour );
	draw_line( posx+width, posy+corner_ratio, posx+width, posy+height-corner_ratio, colour );
}

void draw_instrument_marks_ex( uint16_t posx, uint16_t posy, uint16_t ratio, mark_data_t * data )
{
    int x1, y1, x2, y2;
    int i;
    int cnt;
    int incr;
    uint16_t draw_colour;

    incr = ( ( data->end_angle - data->start_angle ) / data->angle_step ) / ( data->marks - 1 );

    if( data->angle_step >= 0 )
    {
        for( i = data->start_angle, cnt = 0; i < data->end_angle; i += data->angle_step, cnt++ )
        {
            x1 = posx - ( ( cos_table( i ) * ratio ) >> 15 );
            y1 = posy - ( ( sin_table( i ) * ratio ) >> 15 );

            if( ( cnt == incr ) || ( cnt == 0 ) )
            {
                cnt = 0;
                x2 = posx - ( ( cos_table( i ) * (ratio-data->len2) ) >> 15 );
                y2 = posy - ( ( sin_table( i ) * (ratio-data->len2) ) >> 15 );
            }
            else
            {
                x2 = posx - ( ( cos_table( i ) * (ratio-data->len1) ) >> 15 );
                y2 = posy - ( ( sin_table( i ) * (ratio-data->len1) ) >> 15 );
            }

            if( i < data->angle1 ) draw_colour = data->colour1;
            else draw_colour = data->colour2;

            if( i > data->angle3 ) draw_colour = data->colour3;

            draw_line( (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, draw_colour );
        }
    }
    else
    {
        for( i = data->start_angle, cnt = 0; i > data->end_angle; i += data->angle_step, cnt++ )
        {
            x1 = posx - ( ( cos_table( i ) * ratio ) >> 15 );
            y1 = posy - ( ( sin_table( i ) * ratio ) >> 15 );

            if( ( cnt == incr ) || ( cnt == 0 ) )
            {
                cnt = 0;
                x2 = posx - ( ( cos_table( i ) * (ratio-data->len2) ) >> 15 );
                y2 = posy - ( ( sin_table( i ) * (ratio-data->len2) ) >> 15 );
            }
            else
            {
                x2 = posx - ( ( cos_table( i ) * (ratio-data->len1) ) >> 15 );
                y2 = posy - ( ( sin_table( i ) * (ratio-data->len1) ) >> 15 );
            }

            if( i > data->angle1 ) draw_colour = data->colour1;
            else draw_colour = data->colour2;

            if( i < data->angle3 ) draw_colour = data->colour3;

            draw_line( (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, draw_colour );
        }
    }
}

void draw_instrument_marks( uint16_t posx, uint16_t posy,
						    uint16_t ratio, uint16_t len, uint16_t colour,
							int start_angle, int end_angle, int angle_step )
{
    int x1, y1, x2, y2;
    int i;

    if( angle_step >= 0 )
    {
        for( i = start_angle; i < end_angle; i += angle_step )
        {
            x1 = posx - ( ( cos_table( i ) * ratio ) >> 15 );
            y1 = posy - ( ( sin_table( i ) * ratio ) >> 15 );

            x2 = posx - ( ( cos_table( i ) * (ratio-len) ) >> 15 );
            y2 = posy - ( ( sin_table( i ) * (ratio-len) ) >> 15 );

            draw_line( (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, colour );
        }
    }
    else
    {
        for( i = start_angle; i > end_angle; i += angle_step )
        {
            x1 = posx - ( ( cos_table( i ) * ratio ) >> 15 );
            y1 = posy - ( ( sin_table( i ) * ratio ) >> 15 );

            x2 = posx - ( ( cos_table( i ) * (ratio-len) ) >> 15 );
            y2 = posy - ( ( sin_table( i ) * (ratio-len) ) >> 15 );

            draw_line( (uint16_t)x1, (uint16_t)y1, (uint16_t)x2, (uint16_t)y2, colour );
        }
    }

}

void draw_instrument_value( uint16_t posx, uint16_t posy, uint32_t value, uint16_t fore_colour, uint16_t back_colour )
{
	char num_str[10];
    sprintk( num_str, "%d   ", value );

    draw_text(posx, posy, num_str, fore_colour, back_colour);
}

uint16_t draw_instrument_numbers( uint16_t posx, uint16_t posy,
								  uint16_t ratio, uint16_t colour,
								  int start_angle, int end_angle, int angle_step,
								  uint16_t start_num, uint16_t num_step )
{
    uint16_t x1, y1;
    int i;
    uint16_t curr_num;
    char num_str[5];
	int shift_num;
	int offset;
	int offsetx;

    curr_num = start_num;
	shift_num = 0;
	offset = 8;
    offsetx = 0;

    for( i = start_angle; i < end_angle; i += angle_step )
    {
        x1 = posx - ( uint16_t )( ( cos_table( i ) * (ratio+INSTRUMENT_NUM_DIST) ) >> 15 );
        y1 = posy - ( uint16_t )( ( sin_table( i ) * (ratio+INSTRUMENT_NUM_DIST) ) >> 15 );
        
		offsetx = 0;

		if( i > 512 ) shift_num = 1;
		if( i > 720 ) offset = 13;
		if( ( ( i < ( 32-1024 ) ) && ( i > ( 0-32-1024 ) ) ) || ( ( i > ( 1024-32 ) ) && ( i < ( 1024+32 ) ) ) ) offsetx = 5;

        sprintk( num_str, "%d", curr_num );
        curr_num += num_step;
        
        draw_text(x1-((strlen(num_str)*12)>>shift_num)+offsetx, y1-offset, num_str, colour, back_colour);
    }

	return curr_num;
}

void draw_instrument_print( uint16_t posx, uint16_t posy,
							uint16_t ratio, uint16_t colour,
							char * info, int angle )
{
    uint16_t x1, y1;
	int shift_num;
	int offset;

	shift_num = 0;
	offset = 8;
    
    x1 = posx - ( ( cos_table( angle ) * (ratio+INSTRUMENT_NUM_DIST) ) >> 15 );
    y1 = posy - ( ( sin_table( angle ) * (ratio+INSTRUMENT_NUM_DIST) ) >> 15 );
        
	if( angle > 512 ) shift_num = 1;
	if( angle > 720 ) offset = 13;

    draw_text(x1-((strlen(info)*12)>>shift_num), y1-offset, info, colour, back_colour);
}

void draw_erase_niddle_ex( niddle_data_t * data, uint16_t rad )
{
	uint32_t index = 0;

	uint16_t posx_left, posy_left;
	uint16_t posx_right, posy_right;
	uint16_t posx_flood, posy_flood;
	uint16_t posx_end, posy_end;
	uint16_t posx_end_left, posy_end_left;
    uint16_t posx_end_right, posy_end_right;
	uint16_t posx_start, posy_start;
	uint16_t posx_start_left, posy_start_left;
    uint16_t posx_start_right, posy_start_right;
    
	posx_end = data->posx - ( ( cos_table( rad ) * data->niddle_len ) >> 15 );
	posy_end = data->posy - ( ( sin_table( rad ) * data->niddle_len ) >> 15 );
    
    posx_start = data->posx - ( ( cos_table( rad ) * ( data->circle_ratio ) ) >> 15 );
	posy_start = data->posy - ( ( sin_table( rad ) * ( data->circle_ratio ) ) >> 15 );
    
    posx_start_left = posx_start - ( ( cos_table( rad + 1024 ) * data->niddle_width ) >> 15 );
    posy_start_left = posy_start - ( ( sin_table( rad + 1024 ) * data->niddle_width ) >> 15 );
    
    posx_start_right = posx_start - ( ( cos_table( rad - 1024 ) * data->niddle_width ) >> 15 );
    posy_start_right = posy_start - ( ( sin_table( rad - 1024 ) * data->niddle_width ) >> 15 );

    posx_end_left = posx_end - ( ( cos_table( rad + 1024 ) * (data->niddle_width>>1) ) >> 15 );
    posy_end_left = posy_end - ( ( sin_table( rad + 1024 ) * (data->niddle_width>>1) ) >> 15 );

    posx_end_right = posx_end - ( ( cos_table( rad - 1024 ) * (data->niddle_width>>1) ) >> 15 );
    posy_end_right = posy_end - ( ( sin_table( rad - 1024 ) * (data->niddle_width>>1) ) >> 15 );

    posx_flood = data->posx - ( ( cos_table( rad ) * ( data->niddle_len/3 ) ) >> 15 );
	posy_flood = data->posy - ( ( sin_table( rad ) * ( data->niddle_len/3 ) ) >> 15 );

	posx_left = data->posx - ( ( cos_table( rad + 1024 ) * data->niddle_width ) >> 15 );
	posy_left = data->posy - ( ( sin_table( rad + 1024 ) * data->niddle_width ) >> 15 );

	posx_right = data->posx - ( ( cos_table( rad - 1024 ) * data->niddle_width ) >> 15 );
	posy_right = data->posy - ( ( sin_table( rad - 1024 ) * data->niddle_width ) >> 15 );

	if( data->end_niddle )
	{
		index += draw_restore_line_data( posx_start_left, posy_start_left, posx_start_right, posy_start_right, &data->backup[index], 4096 );
	}

	index += draw_restore_line_data( posx_start_left, posy_start_left, posx_end_left, posy_end_left, &data->backup[index], 4096 );
	index += draw_restore_line_data( posx_start_right, posy_start_right, posx_end_right, posy_end_right, &data->backup[index], 4096 );
	index += draw_restore_line_data( posx_end_left, posy_end_left, posx_end_right, posy_end_right, &data->backup[index], 4096 );
}

void draw_erase_niddle( uint16_t posx, uint16_t posy, uint16_t width, uint16_t len, uint16_t rad, uint16_t * backup )
{
	uint32_t index = 0;

	uint16_t posx_left, posy_left;
	uint16_t posx_right, posy_right;
	uint16_t posx_flood, posy_flood;
	uint16_t posx_end, posy_end;
	uint16_t posx_end_left, posy_end_left;
    uint16_t posx_end_right, posy_end_right;
	uint16_t posx_start, posy_start;
	uint16_t posx_start_left, posy_start_left;
    uint16_t posx_start_right, posy_start_right;
    
	posx_end = posx - ( ( cos_table( rad ) * len ) >> 15 );
	posy_end = posy - ( ( sin_table( rad ) * len ) >> 15 );
    
    posx_start = posx - ( ( cos_table( rad ) * ( width << 2 ) ) >> 15 );
	posy_start = posy - ( ( sin_table( rad ) * ( width << 2 ) ) >> 15 );
    
    posx_start_left = posx_start - ( ( cos_table( rad + 1024 ) * width ) >> 15 );
    posy_start_left = posy_start - ( ( sin_table( rad + 1024 ) * width ) >> 15 );
    
    posx_start_right = posx_start - ( ( cos_table( rad - 1024 ) * width ) >> 15 );
    posy_start_right = posy_start - ( ( sin_table( rad - 1024 ) * width ) >> 15 );

    posx_end_left = posx_end - ( ( cos_table( rad + 1024 ) * (width>>1) ) >> 15 );
    posy_end_left = posy_end - ( ( sin_table( rad + 1024 ) * (width>>1) ) >> 15 );

    posx_end_right = posx_end - ( ( cos_table( rad - 1024 ) * (width>>1) ) >> 15 );
    posy_end_right = posy_end - ( ( sin_table( rad - 1024 ) * (width>>1) ) >> 15 );

    posx_flood = posx - ( ( cos_table( rad ) * ( len/3 ) ) >> 15 );
	posy_flood = posy - ( ( sin_table( rad ) * ( len/3 ) ) >> 15 );

	posx_left = posx - ( ( cos_table( rad + 1024 ) * width ) >> 15 );
	posy_left = posy - ( ( sin_table( rad + 1024 ) * width ) >> 15 );

	posx_right = posx - ( ( cos_table( rad - 1024 ) * width ) >> 15 );
	posy_right = posy - ( ( sin_table( rad - 1024 ) * width ) >> 15 );

	index += draw_restore_line_data( posx_start_left, posy_start_left, posx_end_left, posy_end_left, &backup[index], 4096 );
	index += draw_restore_line_data( posx_start_right, posy_start_right, posx_end_right, posy_end_right, &backup[index], 4096 );
	index += draw_restore_line_data( posx_end_left, posy_end_left, posx_end_right, posy_end_right, &backup[index], 4096 );
}

void draw_niddle_ex( niddle_data_t * data, uint16_t rad )
{
	uint32_t index = 0;
	uint16_t posx_left, posy_left;
	uint16_t posx_right, posy_right;
	uint16_t posx_end, posy_end;
	uint16_t posx_end_left, posy_end_left;
    uint16_t posx_end_right, posy_end_right;

	uint16_t posx_start, posy_start;
	uint16_t posx_start_left, posy_start_left;
    uint16_t posx_start_right, posy_start_right;

	if( data->draw_circle )
	{
		draw_circle( data->posx, data->posy, data->circle_ratio, data->circle_colour );
	}
    
	posx_start = data->posx - ( ( cos_table( rad ) * ( data->circle_ratio ) ) >> 15 );
	posy_start = data->posy - ( ( sin_table( rad ) * ( data->circle_ratio ) ) >> 15 );
    
    posx_start_left = posx_start - ( ( cos_table( rad + 1024 ) * data->niddle_width ) >> 15 );
    posy_start_left = posy_start - ( ( sin_table( rad + 1024 ) * data->niddle_width ) >> 15 );
    
    posx_start_right = posx_start - ( ( cos_table( rad - 1024 ) * data->niddle_width ) >> 15 );
    posy_start_right = posy_start - ( ( sin_table( rad - 1024 ) * data->niddle_width ) >> 15 );
    
	posx_end = data->posx - ( ( cos_table( rad ) * data->niddle_len ) >> 15 );
	posy_end = data->posy - ( ( sin_table( rad ) * data->niddle_len ) >> 15 );

    posx_end_left = posx_end - ( ( cos_table( rad + 1024 ) * (data->niddle_width>>1) ) >> 15 );
    posy_end_left = posy_end - ( ( sin_table( rad + 1024 ) * (data->niddle_width>>1) ) >> 15 );
    
    posx_end_right = posx_end - ( ( cos_table( rad - 1024 ) * (data->niddle_width>>1) ) >> 15 );
    posy_end_right = posy_end - ( ( sin_table( rad - 1024 ) * (data->niddle_width>>1) ) >> 15 );
    
	posx_left = data->posx - ( ( cos_table( rad + 1024 ) * data->niddle_width ) >> 15 );
	posy_left = data->posy - ( ( sin_table( rad + 1024 ) * data->niddle_width ) >> 15 );

	posx_right = data->posx - ( ( cos_table( rad - 1024 ) * data->niddle_width ) >> 15 );
	posy_right = data->posy - ( ( sin_table( rad - 1024 ) * data->niddle_width ) >> 15 );

	if( data->backup != NULL )
	{
		if( data->end_niddle )
		{
			index += draw_get_line_data( posx_start_left, posy_start_left, posx_start_right, posy_start_right, &data->backup[index], 4096 );
		}

        index += draw_get_line_data( posx_start_left, posy_start_left, posx_end_left, posy_end_left, &data->backup[index], 4096 );
		index += draw_get_line_data( posx_start_right, posy_start_right, posx_end_right, posy_end_right, &data->backup[index], 4096 );
		index += draw_get_line_data( posx_end_left, posy_end_left, posx_end_right, posy_end_right, &data->backup[index], 4096 );
	}

    // comments below refer to looking at the niddle at vertical position
	// bottom
	if( data->end_niddle )
	{
		draw_line( posx_start_left, posy_start_left, posx_start_right, posy_start_right, data->niddle_colour );
	}

    // right line
    draw_line( posx_start_left, posy_start_left, posx_end_left, posy_end_left, data->niddle_colour );
	
    // left line
    draw_line( posx_start_right, posy_start_right, posx_end_right, posy_end_right, data->niddle_colour );
    
    // top line
	draw_line( posx_end_left, posy_end_left, posx_end_right, posy_end_right, data->niddle_colour );
}

void draw_niddle( uint16_t posx, uint16_t posy, uint16_t width, uint16_t len, uint16_t rad, uint16_t colour, uint16_t * backup )
{
	uint32_t index = 0;
	uint16_t posx_left, posy_left;
	uint16_t posx_right, posy_right;
	uint16_t posx_end, posy_end;
	uint16_t posx_end_left, posy_end_left;
    uint16_t posx_end_right, posy_end_right;

	uint16_t posx_start, posy_start;
	uint16_t posx_start_left, posy_start_left;
    uint16_t posx_start_right, posy_start_right;

	draw_circle( posx, posy, width << 2, colour );
    
	posx_start = posx - ( ( cos_table( rad ) * ( width << 2 ) ) >> 15 );
	posy_start = posy - ( ( sin_table( rad ) * ( width << 2 ) ) >> 15 );
    
    posx_start_left = posx_start - ( ( cos_table( rad + 1024 ) * width ) >> 15 );
    posy_start_left = posy_start - ( ( sin_table( rad + 1024 ) * width ) >> 15 );
    
    posx_start_right = posx_start - ( ( cos_table( rad - 1024 ) * width ) >> 15 );
    posy_start_right = posy_start - ( ( sin_table( rad - 1024 ) * width ) >> 15 );
    
	posx_end = posx - ( ( cos_table( rad ) * len ) >> 15 );
	posy_end = posy - ( ( sin_table( rad ) * len ) >> 15 );

    posx_end_left = posx_end - ( ( cos_table( rad + 1024 ) * (width>>1) ) >> 15 );
    posy_end_left = posy_end - ( ( sin_table( rad + 1024 ) * (width>>1) ) >> 15 );
    
    posx_end_right = posx_end - ( ( cos_table( rad - 1024 ) * (width>>1) ) >> 15 );
    posy_end_right = posy_end - ( ( sin_table( rad - 1024 ) * (width>>1) ) >> 15 );
    
	posx_left = posx - ( ( cos_table( rad + 1024 ) * width ) >> 15 );
	posy_left = posy - ( ( sin_table( rad + 1024 ) * width ) >> 15 );

	posx_right = posx - ( ( cos_table( rad - 1024 ) * width ) >> 15 );
	posy_right = posy - ( ( sin_table( rad - 1024 ) * width ) >> 15 );

	if( backup != NULL )
	{
        index += draw_get_line_data( posx_start_left, posy_start_left, posx_end_left, posy_end_left, &backup[index], 4096 );
		index += draw_get_line_data( posx_start_right, posy_start_right, posx_end_right, posy_end_right, &backup[index], 4096 );
		index += draw_get_line_data( posx_end_left, posy_end_left, posx_end_right, posy_end_right, &backup[index], 4096 );
	}

    // comments below refer to looking at the niddle at vertical position
    // right line
    draw_line( posx_start_left, posy_start_left, posx_end_left, posy_end_left, colour );
	
    // left line
    draw_line( posx_start_right, posy_start_right, posx_end_right, posy_end_right, colour );
    
    // top line
	draw_line( posx_end_left, posy_end_left, posx_end_right, posy_end_right, colour );
}

uint16_t draw_7seg_digits( uint16_t posx, uint16_t posy, char * value, uint16_t colour, bool_t update, void * data )
{
	uint16_t curr_segs;
	int i;
	int j;
	number_7seg_t * p_data = ( number_7seg_t * )data;

	for( i = 0; i < (int)strlen(value); i++ )
	{
		if( ( value[i] < 0x30 ) || ( value[i] > 0x39 ) ) continue;
		curr_segs = seg_list[value[i] & 0x0F];

		if( curr_segs & SEG_A ) graphics_bitmap_1bb( SEG_A_POS, VERT_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_B ) graphics_bitmap_1bb( SEG_B_POS, VERT_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_C ) graphics_bitmap_1bb( SEG_C_POS, VERT_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_D ) graphics_bitmap_1bb( SEG_D_POS, VERT_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_C1 ) graphics_bitmap_1bb( SEG_C1_POS, VERT_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_D1 ) graphics_bitmap_1bb( SEG_D1_POS, VERT_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_E ) graphics_bitmap_1bb( SEG_E_POS, HORZ_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_F ) graphics_bitmap_1bb( SEG_F_POS, HORZ_SEGMENT, back_colour, colour );
		if( curr_segs & SEG_G ) graphics_bitmap_1bb( SEG_G_POS, HORZ_SEGMENT, back_colour, colour );

		if( ( i > 0 ) && ( value[i-1] == '.' ) )
		{
			for( j = 0; j < 5; j++ )
			{
				graphics_set_pixel( posx-6+p_data->dp_offsetx, posy+j+p_data->dp_offsety, colour );
				graphics_set_pixel( posx-5+p_data->dp_offsetx, posy+j+p_data->dp_offsety, colour );
				graphics_set_pixel( posx-4+p_data->dp_offsetx, posy+j+p_data->dp_offsety, colour );
			}
		}

		posx += p_data->width;
	}

	return posx;
}

void show_small_number( uint16_t posx, uint16_t posy, uint16_t value, char * str_unit, uint16_t colour )
{
	char str_angle[20];
	uint16_t new_posx;
    
    graphics_fill_box( posx-5, posy-2, (data_7seg_small.width*3) + 10, data_7seg_small.height+5, back_colour );
    
    //if( value <= 9999 ) posx += 22;
    //if( value <= 999 ) posx += 22;
    if( value <= 99 ) posx += data_7seg_small.width;
    if( value <= 9 ) posx += data_7seg_small.width;

	sprintk( str_angle, "%d", value );
	new_posx = draw_7seg_digits( posx, posy, str_angle, colour, FALSE, ( void * )&data_7seg_small );
	draw_text( new_posx+10, posy+4, str_unit, colour, back_colour );
}

void show_large_number( uint16_t posx, uint16_t posy, uint16_t value, char * str_unit, uint16_t colour )
{
	char str_angle[20];
	uint16_t new_posx;
    
    graphics_fill_box( posx-5, posy-2, (data_7seg_xbig.width*3) + 10, data_7seg_xbig.height + 5, back_colour );
    
    //if( value <= 9999 ) posx += 22;
    //if( value <= 999 ) posx += 22;
    if( value <= 99 ) posx += data_7seg_xbig.width;
    if( value <= 9 ) posx += data_7seg_xbig.width;

	sprintk( str_angle, "%d", value );
	new_posx = draw_7seg_digits( posx, posy, str_angle, colour, FALSE, ( void * )&data_7seg_xbig );
	draw_text( new_posx + 10, posy + 20, str_unit, colour, back_colour );
}

void show_odometer_total_number( uint16_t posx, uint16_t posy, uint32_t value, uint32_t div, uint16_t colour )
{
	char str_angle[20];
	uint16_t new_posx;
    uint16_t xdec = 0;
    
    if( div == 10 ) xdec += data_7seg_small.width;
    if( div == 100 ) xdec += data_7seg_small.width;
    if( div == 1000 ) xdec += data_7seg_small.width;

    graphics_fill_box( posx-5, posy-2, (data_7seg_small.width*8)+10+xdec, data_7seg_small.height+5, back_colour );
    
    if( value <= 999999 ) posx += data_7seg_small.width;
    if( value <= 99999 ) posx += data_7seg_small.width;
    if( value <= 9999 ) posx += data_7seg_small.width;
    if( value <= 999 ) posx += data_7seg_small.width;
    if( value <= 99 ) posx += data_7seg_small.width;
    if( ( value <= 9 ) && ( div == 1 ) ) posx += data_7seg_small.width;
    posx += data_7seg_small.width;
    
	if( div == 1 ) sprintk( str_angle, "%d", value );
    else if( div == 10 ) sprintk( str_angle, "%d.%d", value / div, value % div );
    else if( div == 100 ) sprintk( str_angle, "%d.%02d", value / div, value % div );
    else if( div == 1000 ) sprintk( str_angle, "%d.%03d", value / div, value % div );
	new_posx = draw_7seg_digits( posx, posy, str_angle, colour, FALSE, ( void * )&data_7seg_small );
}


void instruments_reset_trip_odometer( uint8_t index )
{
	if( index >= ODOMETER_TRIP_COUNT ) return;
	trip_list[index] = 0;
}

void instruments_set_mph_speed( bool_t state )
{
	instruments_show_mph = state;
}

bool_t instruments_get_mph_speed( void )
{
	return instruments_show_mph;
}

uint16_t instruments_get_back_colour( void )
{
	return back_colour;
}

void instruments_process( void )
{
	if( selected_skin != NULL )
	{
		selected_skin->skin_process();
	}
}

void instruments_show_information( void * p_data, uint32_t param )
{
    if( ( selected_skin != NULL ) && ( selected_skin->display_event != NULL ) )
	{
		selected_skin->display_event( p_data, param );
	}
}

uint8_t instruments_get_button( uint16_t posx, uint16_t posy, uint16_t posz )
{
    uint8_t ret = 0;
    if( ( selected_skin != NULL ) && ( selected_skin->tts_event != NULL ) )
	{
		ret = selected_skin->tts_event( posx, posy, posz );
	}
    
    return ret;
}

void instruments_select_skin( uint32_t index )
{
	selected_skin = ( skin_data_t * )list_section_get_item( ITEM_TYPE_LIST_SKIN, index );
	if( selected_skin != NULL ) selected_skin->skin_init();
}

void instruments_init( void )
{
	//setup_data_t * setup_data;
	back_colour = 0x0000;

	memset( niddle_speed_backup, 0x00, sizeof( niddle_speed_backup ) );
	memset( niddle_rpm_backup, 0x00, sizeof( niddle_rpm_backup ) );
	memset( niddle_fuel_backup, 0x00, sizeof( niddle_fuel_backup ) );
	memset( niddle_temp_backup, 0x00, sizeof( niddle_temp_backup ) );

	instruments_show_mph = FALSE;

#if defined MAC_OS_EMULATOR
	instruments_select_skin(2);
#else
	instruments_select_skin(0);
#endif
    
	speed_value = 0;
	rpm_value = 0;
	temperature_value = 0;
	fuel_value = 0;
    
#if (defined MAC_OS_EMULATOR || defined WIN32 )
    timer_Start( &update_timer, 100 );
#endif
}
