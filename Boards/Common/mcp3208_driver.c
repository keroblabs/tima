#include "system.h"
#include "debug.h"

///////////////////////////////////////////////////////////////////////////

#define SPI_INDEX_START         80
#define SPI_INDEX_COUNT         8

///////////////////////////////////////////////////////////////////////////

static device_t * spi_device = NULL;

///////////////////////////////////////////////////////////////////////////

static uint16_t spi_adc_read_data( uint8_t index )
{
    uint32_t cnt;
    uint32_t cmd[3];
    uint16_t data;
    uint32_t enable = (uint32_t)TRUE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &enable );

    cmd[0] =  ( 1 << 2 ); // start
    cmd[0] |= ( 1 << 1 ); // single ended
    cmd[0] |= ( index >> 2 ) & 0x01;
    cmd[1] = ( index << 6 ) & 0xF0;
    cmd[2] = 0;

    for( cnt = 0; cnt < 3; cnt++ )
    {
        device_ioctrl( spi_device, DEVICE_GET_RUNTIME_VALUE, &cmd[cnt] );
    }

    data = cmd[1] & 0x0F;
    data = ( data << 8 ) | cmd[2];

    enable = (uint32_t)FALSE;
    device_ioctrl( spi_device, DEVICE_SET_RUNTIME_VALUE, &enable );

    return data;
}

////////////////////////////////////////////////////////////////////////

static void adc_drv_init( uint32_t index )
{
    if( spi_device == NULL ) spi_device = device_open( DEV_SPI_ADC );
    if( spi_device != NULL )
    {
        //DEBUG_PRINTK( "Open ADC(%d)\n", index );
    }
}

static void adc_drv_close( uint32_t index )
{
}

static uint32_t adc_drv_ioctl( uint32_t index, uint32_t param, void * value )
{
    return 0;
}

static uint32_t adc_drv_output( uint32_t index, uint8_t * buffer, uint32_t size )
{
    return 0;
}

static uint32_t adc_drv_input( uint32_t index, uint8_t * buffer, uint32_t size )
{
    uint16_t * value = ( uint16_t * )buffer;

    if( spi_device == NULL ) return 0;
    if( size != 2 ) return 0;

    index -= SPI_INDEX_START;
    *value = spi_adc_read_data( index );

    return size;
}

////////////////////////////////////////////////////////////////////////

static bool_t adc_validate( uint32_t index )
{
    if( ( index >= SPI_INDEX_START ) && ( index < ( SPI_INDEX_START + SPI_INDEX_COUNT ) ) ) return TRUE;
    return FALSE;
}

static bool_t _in_use;

const static device_data_t adc_spi_device =
{
    "adc*",
    &_in_use,

    adc_drv_init,
    adc_drv_close,
    NULL,
    adc_validate,
    NULL,
    adc_drv_ioctl,
    NULL,

    DEV_MODE_CHANNEL,

    adc_drv_input,
    adc_drv_output,

};

DECLARE_DEVICE_SECTION( adc_spi_device );
