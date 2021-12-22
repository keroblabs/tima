#include "types.h"
#include "tima_libc.h"

int64_t tima_atoi_n( const char * str, uint32_t len )
{
	int64_t ret;
    bool_t is_neg;

    uint32_t base = 10;
    uint32_t cnt = 0;

    is_neg = FALSE;
	ret = 0;

    if( ( str[0] == '0' ) && ( str[1] == 'x' ) )
    {
        base = 16;
        cnt = 2;
    }
    else if( str[0] == 'h' )
    {
        base = 16;
        cnt = 1;
    }
    else if( str[0] == 'b' )
    {
        base = 2;
        cnt = 1;
    }
    else if( ( ( str[ cnt ] >= 'A' ) && ( str[ cnt ] <= 'F' ) ) ||
	         ( ( str[ cnt ] >= 'a' ) && ( str[ cnt ] <= 'f' ) ) )
	{
	    base = 16;
        cnt = 0;
	}

	for( ; cnt < len; cnt++ )
	{
        if( str[cnt] <= 0x20 ) continue;
        if( str[cnt] == '-' )
        {
            is_neg = TRUE;
            continue;
        }
        
		ret *= base;

		if( ( base == 16 ) && ( str[ cnt ] >= 'A' ) && ( str[ cnt ] <= 'F' ) )
		{
			ret += ( str[ cnt ] - (int32_t)'A' + 10 );
		}
		else if( ( base == 16 ) && ( str[ cnt ] >= 'a' ) && ( str[ cnt ] <= 'f' ) )
		{
			ret += ( str[ cnt ] - (int32_t)'a' + 10 );
		}
		else if( ( str[ cnt ] >= '0' ) && ( str[ cnt ] <= '9' ) )
		{
			ret += ( str[ cnt ] - (int32_t)'0' );
		}
		else
		{
			return 0;
		}
	}
    
    if( is_neg == TRUE )
    {
        ret = 0 - ret;
    }

	return ret;
}

int64_t tima_atoi( const char * str )
{
	return ( tima_atoi_n( str, (uint32_t)strlen( str ) ) );
}

bool_t tima_is_numeric( const char * str )
{
	int i;
	for( i = 0; i < (int)strlen( str ); i++ )
	{
		if( !tima_isdigit( ( *str ) ) ) return FALSE;
		str++;
	}

	return TRUE;
}
