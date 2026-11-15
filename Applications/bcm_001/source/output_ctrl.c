#include "output_ctrl.h"
#include "can_comms.h"
#include "pio_check.h"
#include "timer.h"
#include "debug.h"

////////////////////////////////////////////////////////////////////

#define BCM_CAN_ID                              0x7f3

/**********/

#define CAN_SWITCH_BARREL_ID					0x01

#define CAN_ACC_MASK							0x01
#define CAN_IGNITION_MASK                       0x02
#define CAN_STARTER_MASK                        0x04

/**********/

#define CAN_AC_CONTROLLER_ID					0x02

#define CAN_AC_CLUTCH_MASK                      0x01
#define CAN_AC_FAN2_MASK                      	0x02

/**********/

#define CAN_CONSOLE_ID							0x03

#define CAN_WS_HEATER_MASK                      0x01
#define CAN_REAR_FOG_MASK                       0x02
#define CAN_REAR_OPENER_MASK                    0x04

/**********/

#define CAN_REAR_BCM_ID							0x04

#define CAN_REVERSE_MASK                        0x01
#define CAN_BOOT_OPEN_MASK                      0x02

/**********/

#define CAN_FRONT_MAIN_BCM_ID					0x05

#define CAN_MESSAGE_GEM_BITMAP					(CAN_MESSAGE_BCM_ID+1)

#define CAN_MESSAGE3_STATUS_FRONTFOG            0x01
#define CAN_MESSAGE3_STATUS_FULLBEAM            0x02
#define CAN_MESSAGE3_STATUS_MAINBEAM            0x04
#define CAN_MESSAGE3_STATUS_HEADLAMPS           0x08

/**********/

#define CAN_MESSAGE_BCM_ID                      0
#define CAN_MESSAGE_GPIO_INDEX					1
#define CAN_MESSAGE_VALUE1_INDEX				4
#define CAN_MESSAGE_VALUE2_INDEX				6

/**********/

#define BCM_KEEP_ALIVE_ID						0x80

////////////////////////////////////////////////////////////////////

#define READ_INTERVAL_MS						200

#define UPDATE_GPIO_PIN(curr, new, mask, gpio, debug)       if( (curr ^ new) & mask ) \
                                                            { \
                                                                bool_t new_state = ( new & mask ) ? TRUE : FALSE; \
                                                                if( gpio != NULL ) pio_check_event_set_output( gpio, new_state ); \
                                                                DEBUG_PRINTK( debug, new_state ); \
                                                            }

////////////////////////////////////////////////////////////////////

typedef struct bcm_can_data_t_
{
	uint32_t curr_gpio;
	uint32_t set_gpio;

	uint16_t curr_value1;
	uint16_t curr_value2;

	uint16_t set_value1;
	uint16_t set_value2;

	uint8_t  tx_count;

	on_received_t on_received;

	bool_t   is_present;
	bool_t   watchdog_set;

} bcm_can_data_t;

////////////////////////////////////////////////////////////////////

static void * outp_mcsel_drv;
static void * outp_pwm0_drv;

static void * outp4_drv;
static void * outp5_drv;

static void * outl4_drv;
static void * outl5_drv;
static void * outl6_drv;
static void * outl7_drv;
static void * dummy_drv;

static device_t * boot_sw;
static device_t * reverse_sw;
static device_t * adc_fuel;
static device_t * adc_temp;

static uint8_t curr_gpio;
static timer_data_t read_timeout;
static hook_data_t hook_data;
static bcm_can_data_t bcm_data;

////////////////////////////////////////////////////////////////////

static void check_request_to_send( uint8_t bcm_id )
{
	uint8_t bcm_message[CAN_DATA_MAX_SIZE];

	if( bcm_id == CAN_REAR_BCM_ID )
	{
		bcm_message[0] = CAN_REAR_BCM_ID;

        bcm_message[1] = ( bcm_data.curr_gpio >> 16 ) & 0x0FF;
        bcm_message[2] = ( bcm_data.curr_gpio >> 8 )  & 0x0FF;
        bcm_message[3] =   bcm_data.curr_gpio & 0x0FF;

        bcm_message[4] = ( bcm_data.curr_value1 >> 8 )  & 0x0FF;
        bcm_message[5] =   bcm_data.curr_value1 & 0x0FF;

        bcm_message[6] = ( bcm_data.curr_value2 >> 8 )  & 0x0FF;
        bcm_message[7] =   bcm_data.curr_value2 & 0x0FF;

        can_comms_send_data( BCM_CAN_ID, bcm_message );
	}
}

static void output_on_can( uint16_t id, uint8_t * buffer )
{
    uint8_t  bcm_id   = CAN_VALUE_TO_8BIT(buffer, CAN_MESSAGE_BCM_ID);
    uint32_t new_gpio = CAN_VALUE_TO_24BIT(buffer, CAN_MESSAGE_GPIO_INDEX);

    if( id != BCM_CAN_ID ) return;

    if( bcm_id & BCM_KEEP_ALIVE_ID )
    {
    	bcm_id &= ~BCM_KEEP_ALIVE_ID;

    	check_request_to_send( bcm_id );
    }
    else
    {
        switch( bcm_id )
        {
    		case CAN_SWITCH_BARREL_ID:
    	        UPDATE_GPIO_PIN( curr_gpio, new_gpio, CAN_ACC_MASK,         dummy_drv, "Acc         = %d\n" );
    	        UPDATE_GPIO_PIN( curr_gpio, new_gpio, CAN_IGNITION_MASK,    outl7_drv, "Ignition    = %d\n" );
    	        UPDATE_GPIO_PIN( curr_gpio, new_gpio, CAN_STARTER_MASK,     outl6_drv, "Starter     = %d\n" );
    			break;

    		case CAN_AC_CONTROLLER_ID:
    	        UPDATE_GPIO_PIN( curr_gpio, new_gpio, CAN_AC_CLUTCH_MASK,   outl4_drv, "AC clutch   = %d\n" );
    			break;

    		case CAN_CONSOLE_ID:
    	        UPDATE_GPIO_PIN( curr_gpio, new_gpio, CAN_WS_HEATER_MASK,   outl5_drv, "WS heater   = %d\n" );

    	        UPDATE_GPIO_PIN( curr_gpio, new_gpio, CAN_REAR_OPENER_MASK, outp4_drv, "Rear opener = %d\n" );
    	        UPDATE_GPIO_PIN( curr_gpio, new_gpio, CAN_REAR_FOG_MASK,    outp5_drv, "Rear fog    = %d\n" );
    			break;
        }

        curr_gpio = new_gpio;

    }
}

void output_ctrl_process( void )
{
	uint32_t new_gpio;

	if( timer_Check( &read_timeout ) == TRUE )
	{
		timer_Reload( &read_timeout );

		device_read_buffer( adc_fuel, ( uint8_t * )&bcm_data.curr_value1, 2 );
		device_read_buffer( adc_temp, ( uint8_t * )&bcm_data.curr_value2, 2 );

		new_gpio = 0;
		new_gpio |= ( pio_check_read_input( reverse_sw, TRUE ) ) ? CAN_REVERSE_MASK : 0x00;
		new_gpio |= ( pio_check_read_input( boot_sw, TRUE ) ) ? CAN_BOOT_OPEN_MASK  : 0x00;

		if( new_gpio != bcm_data.curr_gpio )
		{
			bcm_data.curr_gpio = new_gpio;
			check_request_to_send( CAN_REAR_BCM_ID );
		}
	}
}

void output_ctrl_init( void )
{
    outp_mcsel_drv =  pio_check_event_open_output( "/dev/gpio9",  FALSE, TRUE );
    outp_pwm0_drv =   pio_check_event_open_output( "/dev/gpio10", FALSE, TRUE );

    outp4_drv =       pio_check_event_open_output( "/dev/gpio11", FALSE, FALSE );
    outp5_drv =       pio_check_event_open_output( "/dev/gpio12", FALSE, FALSE );

    /* SPI expansion GPIO covers from gpio70 to gpio77 for input, gpio78 to gpio85 for output */
    reverse_sw =      pio_check_open_input( "/dev/gpio70", NULL );
    boot_sw    =      pio_check_open_input( "/dev/gpio71", NULL );

    outl4_drv =       pio_check_event_open_output( "/dev/gpio82", FALSE, FALSE );
    outl5_drv =       pio_check_event_open_output( "/dev/gpio83", FALSE, FALSE );
    outl6_drv =       pio_check_event_open_output( "/dev/gpio84", FALSE, FALSE );
    outl7_drv =       pio_check_event_open_output( "/dev/gpio85", FALSE, FALSE );

    /* SPI ADC covers from adc80 to adc87 */
    adc_fuel  =       device_open( "/dev/adc80" );
    adc_temp  =       device_open( "/dev/adc81" );

    dummy_drv =       NULL;

    memset( &bcm_data, 0x00, sizeof( bcm_data ) );

    timer_Start( &read_timeout, READ_INTERVAL_MS );

    can_comms_hook_id( &hook_data, BCM_CAN_ID, output_on_can );

    curr_gpio =     FALSE;
}
