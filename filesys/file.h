#ifndef __FILE_H_
#define __FILE_H_

///////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "fs_time.h"
#include "fs.h"
#include "dir.h"
#include "fat.h"

///////////////////////////////////////////////////////////////////////////////

#define FS_MODE_READ    0x72
#define FS_MODE_WRITE   0x77
#define FS_MODE_APPEND  0x61
#define FS_MODE_RANDOM  0x62
#define FS_MODE_DEVICE  0x30

#define FS_SEEK_SET		0
#define FS_SEEK_CUR		1
#define FS_SEEK_END		2

#define FILE_STATUS_OPEN    0
#define FILE_STATUS_WRITE   1
#define FILE_STATUS_HEAP    2

#define SEEK_CUR    1
#define SEEK_END    2
#define SEEK_SET    0

#define PARTITION_NAME_LENGTH			15
#define VOLUME_MAX_SIZE					128

#define IO_CONTROL_FILE_SIZE			1
#define IO_CONTROL_FILE_POSITION		2

///////////////////////////////////////////////////////////////////////////////

#define FILE_OK                     0
#define FILE_EOF                    -1
#define FILE_NOT_FOUND              -2
#define FILE_ALREADY_EXIST          -3
#define FILE_WRITE_ERROR            -4
#define FILE_WRITE_INVALID          -5
#define FILE_INVALID_MODE           -6
#define FILE_INVALID_FS             -7
#define FILE_INVALID_DEVICE         -8
#define FILE_UNABLE_TO_WRITE        -9
#define FILE_INTERNAL_ERROR         -10
#define FILE_DIR_NOT_EMPTY			-11
#define FILE_OUT_OF_RANGE			-12
#define FILE_INVALID_SYSTEM_FS      -13


#define DIR_MODE_ROOT               0
#define DIR_MODE_DIR                1
#define DIR_MODE_SYSTEM             2
#define DIR_MODE_DEVICE             3

///////////////////////////////////////////////////////////////////////////////

typedef struct _File
{
	FileRecord DirEntry;
	FileLocation Location; /* Location in directory!! */
    // DevLocation DevInfo;   /* used for device */
	FileSystem *fs;
	ClusterChain Cache;
	uint8_t	FileStatus;
	uint32_t FilePtr;
	uint32_t FileSize;

} File;

///////////////////////////////////////////////////////////////////////////////

File* 		file_fopen(char* full_filename,char mode);
void		file_fclose(File *file);
void        file_flush( File * file );
int			file_ferror( void );

uint32_t	file_fread(uint8_t *buf,uint32_t size, uint32_t count, File *file);
uint32_t	file_fwrite (uint8_t* buf,uint32_t size, uint32_t count, File* file); 

char *	    file_fgets(char *buf,uint32_t size,File *file);
int			file_fputs(const char * text, File *file );

uint8_t     file_fputc( uint8_t data, File *file );
int         file_fgetc(File *file);

int         file_fprintk( File *file, const char *fmt, ... );

char		file_feof(File *file);

char		file_remove( char * filename);

uint32_t	file_fioctrl( File * file, uint32_t param, void * data );
int			file_fseek( File * file, int64_t offset, int origin );
int64_t		file_ftell(File *file);
void		file_rewind( File * file );

///////////////////////////////////////////////////////////////////////////////

bool_t      file_IsDirectory( const char * full_filename );
bool_t      file_Exists( const char * full_filename );
bool_t      file_isRoot( const char * full_filename );

uint32_t	file_read(uint8_t *buf, uint32_t size, uint32_t offset, File *file);
uint32_t	file_write(uint8_t* buf,uint32_t size,uint32_t offset,File* file);

char*		file_normalToFatName(char* filename,char* fatfilename);
char*		file_normalToFatName2(char* filename,char* fatfilename);

void		file_setAttr(File* file,uint8_t attribute,uint8_t val);
uint8_t		file_getAttr(File* file,uint8_t attribute);
char *		file_getVolume( const char * full_filename, char * volume );

char        file_locatePath( const char * full_filename );

bool_t      file_IsOpen( File * file );
bool_t		file_IsTXBusy( File * file );

///////////////////////////////////////////////////////////////////////////////


#endif

