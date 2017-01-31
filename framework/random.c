#include "random.h"

////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////

static uint32_t rand_seed = 27854263;

////////////////////////////////////////////////////////////////////////////////////////////////

void rand_generate( void )
{
    rand_seed = (rand_seed * 32719 + 3) % 32768;
}

void rand_get_seed( int64_t * seed )
{
    int cnt;
    uint8_t * seed_8 = ( uint8_t * )seed;
    
    for( cnt = 0; cnt < 8; cnt++ )
    {
        rand_generate(); 
        seed_8[cnt] = rand_seed % 0xff;
        
        if( seed_8[cnt] == 0 ) seed_8[cnt]++;
    }
}

uint32_t rand_get_next( int64_t * seed )
{
    uint32_t * seed_32 = ( uint32_t * )seed;
    
    if( ( seed_32[0] == 0 ) || ( seed_32[1] == 0 ) )
    {
        rand_get_seed( seed );
        
        if( seed_32[0] == 0 ) seed_32[0]++;
        if( seed_32[1] == 0 ) seed_32[1]++;
    }
    
    seed_32[0] = 36969 * (seed_32[0] & 65535) + (seed_32[0] >> 16);
    seed_32[1] = 18000 * (seed_32[1] & 65535) + (seed_32[1] >> 16);
    
    return( ( seed_32[0] << 16 ) + ( seed_32[1] & 0x0FFFF ) );
}
