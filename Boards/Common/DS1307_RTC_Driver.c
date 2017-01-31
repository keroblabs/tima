#include "system.h"
#include "hardware.h"
#include "stm32f4xx_i2c.h"
#include "tima_libc.h"

/////////////////////////////////////////////////////////////////////

#define I2C_RTC_ADDR		0x68
#define I2C_RTC_NVM_BASE	0x08
#define I2C_RTC_NVM_MAX		( 0x3F - I2C_RTC_NVM_BASE )

/////////////////////////////////////////////////////////////////////

void rtc_init( void )
{
}

time_t rtc_read( void )
{
	date_time_t tm;
        
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day_week;
	uint8_t date;
	uint8_t month;
	uint8_t year; 
    
    // set addr 0
    I2C_Bus_Start( I2C_RTC_ADDR, I2C_TRANSMIT );
    I2C_Write_Data( 0x00 );
    I2C_Bus_Stop();
    
    // read data
    I2C_Bus_Start( I2C_RTC_ADDR, I2C_RECEIVE );
    
    seconds = I2C_Read_Data( TRUE );
    minutes = I2C_Read_Data( TRUE );
    hours = I2C_Read_Data( TRUE );
    day_week = I2C_Read_Data( TRUE );
    date = I2C_Read_Data( TRUE );
    month = I2C_Read_Data( TRUE );
    year = I2C_Read_Data( FALSE );   
    
    I2C_Bus_Stop();
    
    tm.tm_sec = ( (seconds >> 4) * 10 ) + (seconds & 0x0F);
    tm.tm_min = ( (minutes >> 4) * 10 ) + (minutes & 0x0F);
    tm.tm_hour = ( (hours >> 4) * 10 ) + (hours & 0x0F);
    tm.tm_wday = day_week;
    tm.tm_mday = ( (date >> 4) * 10 ) + (date & 0x0F);
    tm.tm_mon = ( (month >> 4) * 10 ) + (month & 0x0F);	
    tm.tm_year = ( (year >> 4) * 10 ) + (year & 0x0F);
    
    return tima_mktime( &tm );
}

void rtc_write( time_t time )
{
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day_week;
	uint8_t date;
	uint8_t month;
	uint8_t year;

    date_time_t * tm;
    
    tm = tima_localtime( time );
    
	seconds = ( ( tm->tm_sec / 10 ) << 4 ) | ( tm->tm_sec % 10 ); 
	minutes = ( ( tm->tm_min / 10 ) << 4 ) | ( tm->tm_min % 10 ); 
	hours =   ( ( tm->tm_hour / 10 ) << 4 ) | ( tm->tm_hour % 10 ); 
	
	day_week = tm->tm_wday;
	
	date =  ( ( tm->tm_mday / 10 ) << 4 ) | ( tm->tm_mday % 10 ); 
	month = ( ( tm->tm_mon / 10 ) << 4 )  | ( tm->tm_mon % 10 ); 
	year =  ( ( tm->tm_year / 10 ) << 4 ) | ( tm->tm_year % 10 );     
    
    // set addr 0 and write
    I2C_Bus_Start( I2C_RTC_ADDR, I2C_TRANSMIT );
    I2C_Write_Data( 0x00 );
	I2C_Write_Data( seconds );
	I2C_Write_Data( minutes );
	I2C_Write_Data( hours );
	I2C_Write_Data( day_week );
	I2C_Write_Data( date );
	I2C_Write_Data( month );
	I2C_Write_Data( year );    
    I2C_Bus_Stop();    
}

void _rtc_calendar_init( uint32_t index )
{
    uint8_t reg0;

    // set addr 0
    I2C_Bus_Start( I2C_RTC_ADDR, I2C_TRANSMIT );
    I2C_Write_Data( 0x00 );
    I2C_Bus_Stop();
    
    // read data
    I2C_Bus_Start( I2C_RTC_ADDR, I2C_RECEIVE );
    reg0 = I2C_Read_Data( FALSE );
    I2C_Bus_Stop();
    
    if( reg0 & 0x80 )
    {
        I2C_Bus_Start( I2C_RTC_ADDR, I2C_TRANSMIT );
        I2C_Write_Data( 0x00 );
        I2C_Write_Data( 0x00 );
        I2C_Bus_Stop();        
    }
}

uint32_t _rtc_calendar_get( uint32_t index, time_t * value, uint32_t size )
{
	// obsolete, dont use it
    return 0;
}

uint32_t _rtc_calendar_ioctrl( uint32_t index, uint32_t param, void * data )
{
    time_t * p_value = ( time_t * )data;
    
    if( param == DEVICE_GET_RUNTIME_VALUE )
    {
        if( p_value != NULL ) *p_value = rtc_read();  
    }
	else if( param == DEVICE_SET_RUNTIME_VALUE )
    {
        if( p_value != NULL ) rtc_write( *p_value );
    }

    return( 0 );
}

///////////////////////////////////////////////////////////////////////////

static bool_t _in_use;

const static device_data_t calendar_device = 
{
    "rtc",
    &_in_use,
    
    _rtc_calendar_init,
    NULL,
    NULL,
    NULL,
    NULL,
    _rtc_calendar_ioctrl,
    NULL,
    
    DEV_MODE_CHANNEL,
    
    _rtc_calendar_get,
    NULL
};

DECLARE_DEVICE_SECTION( calendar_device )
