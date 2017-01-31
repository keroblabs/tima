#include "system.h"
#include "hardware.h"

////////////////////////////////////////////////////////////////////////

#define I2C_DS1631_ADDR         0x48

#define DS1631_CMD_CONFIG       0xAC
#define DS1632_CMD_TH           0xA1
#define DS1631_CMD_TL           0xA2
#define DS1631_CMD_READ         0xAA
#define DS1631_CMD_START_T      0x51
#define DS1631_CMD_STOP_T       0x22
#define DS1631_CMD_RESET        0x54

////////////////////////////////////////////////////////////////////////

void _thermo_init( uint32_t index )
{
    // doing a power reset is enough
    //I2C_Bus_Start( I2C_DS1631_ADDR, I2C_TRANSMIT );
    //I2C_Write_Data( DS1631_CMD_RESET );
    //I2C_Bus_Stop();
    
    // start continous conversion
    I2C_Bus_Start( I2C_DS1631_ADDR, I2C_TRANSMIT );
    I2C_Write_Data( DS1631_CMD_START_T );
    I2C_Bus_Stop();
}

uint32_t _thermo_read( void )
{
    uint32_t temp;
    
    // this is the read command
    I2C_Bus_Start( I2C_DS1631_ADDR, I2C_TRANSMIT );
    I2C_Write_Data( DS1631_CMD_READ );
    I2C_Bus_Stop();
    
    // read the temperature
    I2C_Bus_Start( I2C_DS1631_ADDR, I2C_RECEIVE );
    temp = I2C_Read_Data( TRUE );
    temp = ( temp << 8 ) + I2C_Read_Data( FALSE );   
    I2C_Bus_Stop();
    
    return temp;
}

uint32_t _thermo_ioctrl( uint32_t index, uint32_t param, void * data )
{
    uint32_t * p_value = ( uint32_t * )data;
    
    if( param == DEVICE_GET_RUNTIME_VALUE )
    {
        if( p_value != NULL ) *p_value = _thermo_read();
    }

    return( 0 );    
}

uint16_t _thermo_get( uint32_t index, uint32_t * value, uint16_t size )
{
	uint32_t ret;
	
	ret = _thermo_read();

	*value = ret;
	return sizeof( uint32_t );
}

////////////////////////////////////////////////////////////////////////

static bool_t _in_use = FALSE;

const static device_data_t therm_device = 
{
    "thermo",
    &_in_use,
    
    _thermo_init,
    NULL,
    NULL,
    NULL,
    NULL,
    _thermo_ioctrl,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    _thermo_get,
    NULL
    
};

DECLARE_DEVICE_SECTION( therm_device );

