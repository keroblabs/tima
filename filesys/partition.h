
#ifndef __PARTITION_H__
#define __PARTITION_H__

///////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "mass_mal.h"

///////////////////////////////////////////////////////////////////////////////

#define LBA_ADDR_MBR                0
#define PARTITION_TABLE_OFFSET      0x1BE
#define SIZE_PARTITION_FIELD        16
#define TOTAL_PARTITIONS			4

#define disc_ReadSector(l,a,b)		MAL_Read(l,a,b,512)
#define disc_WriteSector(l,a,b)		MAL_Write(l,a,b,512)
#define disc_GetSectorCount(l)		MAL_GetBlockCount(l)

#define disc_GetLun(v)				MAL_GetLun(v)
#define disc_InitLun(l)				MAL_Init(l)
#define disc_DevName(l)				MAL_RetrieveVolumeName(l)
#define disc_Count()				MAL_GetTotalLun()

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

#define PT_FAT12  0x01
#define PT_FAT16A 0x04
#define PT_FAT16  0x06
#define PT_FAT32  0x0B
#define PT_FAT32A 0x5C
#define PT_FAT16B 0x5E

///////////////////////////////////////////////////////////////////////////////

typedef struct _PartitionField
{
	uint8_t lun;
	uint8_t bootFlag;
	uint8_t CHS_begin[3];
	uint8_t type;
	uint8_t CHS_end[3];
	uint32_t LBA_begin;
	uint32_t numSectors;
	void * fs;

} PartitionField;

typedef struct _Partition
{
	uint8_t disc;
	PartitionField partitions[TOTAL_PARTITIONS];

} Partition;

///////////////////////////////////////////////////////////////////////////////

static INLINE int disc_InitDevice( uint8_t * lun, char * volume )
{
    // 0 = success
    *lun = disc_GetLun( volume );
	if( *lun == LUN_NOT_FOUND ) return MAL_FAIL;
	if( !disc_InitLun( *lun ) ) return MAL_OK;
    return MAL_FAIL;
}

///////////////////////////////////////////////////////////////////////////////

int 		part_initPartition(Partition *part, uint8_t refDisc);
int			part_isFatPart(uint8_t type);
int         part_isExtPart(uint8_t type);

int			part_writeBuf(PartitionField *part,uint32_t address,uint8_t* buf);
uint8_t*	part_getSect(PartitionField *part, uint32_t address,uint8_t * buf);

int			part_CreateDefaultMBR( Partition * part );
int			part_UpdatePartitionTable( Partition * part );
int			part_FormatPartition( PartitionField *part, uint8_t part_index, char * volume );

char		part_getVolumeName( PartitionField * part, uint32_t loc, char * name );

///////////////////////////////////////////////////////////////////////////////

#include "extract.h"

///////////////////////////////////////////////////////////////////////////////

#endif
