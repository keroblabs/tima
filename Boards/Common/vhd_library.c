#include "system.h"

////////////////////////////////////////////////////////////////////////

#define VHD_DEFAULT_VHD_NAME		"conectix"
#define VHD_DEFAULT_FEATURES		0x00000002
#define VHD_DEFAULT_VERSION			0x00010000
#define VHD_DEFAULT_OFFSET			((uint64_t)-1)
#define VHD_DEFAULT_DISCTYPE		0x00000002
#define VHD_DEFAULT_TIMESTAMP		0x1bfc83bc
#define VHD_DEFAULT_CREATOR_APP		0x77696e20
#define VHD_DEFAULT_CREATOR_VER		0x00060001
#define VHD_DEFAULT_CREATOR_OS		0x5769326b

////////////////////////////////////////////////////////////////////////

typedef struct _vhd_footer
{
	char name[8];
	uint32_t features;
	uint32_t format_ver;
	uint64_t offset;
	uint32_t time_stap;
	uint32_t creator_app;
	uint32_t creator_ver;
	uint32_t creator_os;
	uint64_t size_org;
	uint64_t curr_size;
	uint32_t disc_geometry;
	uint32_t disc_type;
	uint32_t checksum;
	uint8_t unique_id[16];
	uint8_t reserved[428];

} vhd_footer_t;

////////////////////////////////////////////////////////////////////////

//static vhd_footer_t footer;
static uint8_t buffer_vhd[512];

////////////////////////////////////////////////////////////////////////

void memcpy_nce( void * target, void * input, uint32_t size )
{
	memcpy( target, input, size );
}

void memcpy_cce( void * target, void * input, uint32_t size )
{
	uint8_t * p_input = ( uint8_t * )input;
	uint8_t * p_target = &( ( uint8_t * )target )[size-1];
	while( size-- )
	{
		*( p_target ) = *( p_input );
		p_target--;
		p_input++;
	}
}

uint32_t get_disc_geometry( uint64_t disc_size )
{
	uint32_t ret;
	uint32_t totalSectors = ( uint32_t )( disc_size / 512 );
	uint32_t sectorsPerTrack;
	uint32_t cylinderTimesHead;
	uint32_t cylinders;
	uint32_t heads;

	if (totalSectors > 65535 * 16 * 255)
	{
		totalSectors = 65535 * 16 * 255;
	}

	if (totalSectors >= 65535 * 16 * 63)
	{
		sectorsPerTrack = 255;
		heads = 16;
		cylinderTimesHead = totalSectors / sectorsPerTrack;
	}
	else
	{
		sectorsPerTrack = 17; 
		cylinderTimesHead = totalSectors / sectorsPerTrack;

		heads = (cylinderTimesHead + 1023) / 1024;
      
		if (heads < 4)
		{
			heads = 4;
		}
		if (cylinderTimesHead >= (heads * 1024) || heads > 16)
		{
			sectorsPerTrack = 31;
			heads = 16;
			cylinderTimesHead = totalSectors / sectorsPerTrack;	
		}
		if (cylinderTimesHead >= (heads * 1024))
		{
			sectorsPerTrack = 63;
			heads = 16;
			cylinderTimesHead = totalSectors / sectorsPerTrack;
		}
	}
	cylinders = cylinderTimesHead / heads;

	ret = ( cylinders << 16 ) + ( heads << 8 ) + sectorsPerTrack;
	return ret;
}

uint32_t get_checksum( uint8_t * buffer, uint32_t size )
{
	uint32_t ret = 0;
	uint32_t cnt;

	for( cnt = 0; cnt < size; cnt++ )
	{
		ret += buffer[cnt];
	}

	return ~ret;
}

uint64_t set_val64( uint64_t val )
{
	uint64_t ret;
	memcpy_cce( &ret, &val, sizeof( val ) );
	return ret;
}

uint32_t set_val32( uint32_t val )
{
	uint32_t ret;
	memcpy_cce( &ret, &val, sizeof( val ) );
	return ret;
}

uint8_t * create_vhd_footer( uint64_t size, uint8_t * unique_id )
{
	vhd_footer_t * p_footer;

	memset( buffer_vhd, 0x00, sizeof( buffer_vhd ) );
	p_footer = ( vhd_footer_t * )&buffer_vhd[0];

	memcpy( p_footer->name, VHD_DEFAULT_VHD_NAME, sizeof( p_footer->name ) );
	p_footer->features		= set_val32( VHD_DEFAULT_FEATURES );
	p_footer->format_ver	= set_val32( VHD_DEFAULT_VERSION );
	p_footer->offset		= set_val64( VHD_DEFAULT_OFFSET );

	p_footer->time_stap		= set_val32( VHD_DEFAULT_TIMESTAMP );
	p_footer->creator_app	= set_val32( VHD_DEFAULT_CREATOR_APP );
	p_footer->creator_ver	= set_val32( VHD_DEFAULT_CREATOR_VER );
	p_footer->creator_os	= set_val32( VHD_DEFAULT_CREATOR_OS );

	p_footer->size_org      = set_val64( size );
	p_footer->curr_size     = set_val64( size );
	p_footer->disc_geometry = set_val32( get_disc_geometry( size ) );
	p_footer->disc_type		= set_val32( VHD_DEFAULT_DISCTYPE );

	memcpy( p_footer->unique_id, unique_id, sizeof( p_footer->unique_id ) );
	p_footer->checksum		= set_val32( get_checksum( buffer_vhd, sizeof( buffer_vhd ) ) );

	return ( uint8_t * )buffer_vhd;
}
