#include "setup_data.h"
#include "epr_layer.h"
#include "instruments.h"
#include "debug.h"

///////////////////////////////////////////////////////////////

#define SETUP_DATA_HEADER		0x438D
#define SETUP_DEFAULT_SPEED		2844

///////////////////////////////////////////////////////////////

static epr_data_t * epr_data;

///////////////////////////////////////////////////////////////

void setup_data_init( void )
{
	setup_data_t * ret;
	epr_data = epr_layer_init( "/eeprom1", sizeof( setup_data_t ), 0 );

	if( epr_data == NULL )
	{
		DEBUG_PRINTK( "NVM error\r\n" );
        epr_data = NULL;
		return;
	}

	ret = setup_data_get();
	if( ret->header != SETUP_DATA_HEADER )
	{
		DEBUG_PRINTK( "Init NVM\r\n" );

		memset( ret, 0x00, sizeof( setup_data_t ) );
		ret->header = SETUP_DATA_HEADER;
		ret->speed_setup = SETUP_DEFAULT_SPEED;
		ret->is_mph = TRUE;

		// preset odometer
		//ret->odometer_main = (2844 * 5);

		epr_layer_flush( epr_data );
	}
}

void setup_data_write( void )
{
	epr_layer_flush( epr_data );
}

setup_data_t * setup_data_get( void )
{
	setup_data_t * ret;

	if( epr_data == NULL ) return NULL;
	ret = ( setup_data_t * )epr_layer_read( epr_data );
	return ret;
}
