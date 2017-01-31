#ifndef __DDIR_H__
#define __DDIR_H__

///////////////////////////////////////////////////////////////////////////////

#include "types.h"
#include "fat.h"
#include "fs_time.h"
#include "fs.h"

///////////////////////////////////////////////////////////////////////////////

#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_DEVICE     ( 0x80 | ATTR_ARCHIVE | ATTR_DIRECTORY | ATTR_VOLUME_ID )

#define OFFSET_DE_FILENAME            0
#define OFFSET_DE_ATTRIBUTE          11
#define OFFSET_DE_NTRESERVED         12
#define OFFSET_DE_CRTIMETNT          13
#define OFFSET_DE_CREATETIME         14
#define OFFSET_DE_CREATEDATE         16
#define OFFSET_DE_LASTACCESSDATE     18
#define OFFSET_DE_FIRSTCLUSTERHIGH   20
#define OFFSET_DE_WRITETIME          22
#define OFFSET_DE_WRITEDATE          24
#define OFFSET_DE_FIRSTCLUSTERLOW    26
#define OFFSET_DE_FILESIZE           28

#define DIRFIND_FILE 0
#define DIRFIND_FREE 1

///////////////////////////////////////////////////////////////////////////////

typedef struct
{
	char name[ MAX_LONG_FILENAME_LENGHT ];
	// char fat_name[ 12 ];
	char extension[ 5 ];
	uint8_t attribute;
    
    uint16_t date;
    uint16_t time;
    
    uint32_t size;

} tFileInformation;

///////////////////////////////////////////////////////////////////////////////

void dir_getFileStructure(FileSystem *fs,FileRecord *filerec,FileLocation *loc);
void dir_createDirectoryEntry(FileSystem *fs,FileRecord *filerec,FileLocation *loc, char * long_filename, char * fullname );
void dir_createDefaultEntry(FileSystem *fs,FileRecord *filerec,char* fatfilename);
void dir_setFirstCluster(FileSystem *fs,FileLocation *loc,uint32_t cluster_addr);
void dir_setFileSize(FileSystem *fs,FileLocation *loc,uint32_t numbytes);

uint32_t dir_findinRoot(FileSystem *fs,char * fatname, FileLocation *loc);
uint32_t dir_findinDir(FileSystem *fs, char * fatname, uint32_t startCluster, FileLocation *loc, uint8_t mode);
uint32_t dir_findinBuf(uint8_t *buf,char *fatname, FileLocation *loc, uint8_t mode);
uint32_t dir_findinCluster(FileSystem *fs,uint32_t cluster,char *fatname, FileLocation *loc, uint8_t mode);
uint32_t dir_findinRootArea(FileSystem *fs,char* fatname, FileLocation *loc, uint8_t mode);

char dir_getFatFileName(char* filename, char* fatfilename);
char dir_getFatFileName_Long(char* filename, char* fatfilename);
char dir_addCluster(FileSystem *fs,uint32_t firstCluster);

char dir_getVolumeName( FileSystem * fs, char * name );
char dir_updateDirectoryEntry(FileSystem *fs,FileRecord *entry,FileLocation *loc);
char dir_updateVolumeName( FileSystem * fs, uint8_t curr_part, char * name );

///////////////////////////////////////////////////////////////////////////////

void *      dir_openFileList( char * path );
int         dir_readFileList( void * p_dir, tFileInformation * file_info );
void        dir_closeFileList( void * p_dir );

char        dir_createDirectory( char * full_dirname );

bool_t		dir_isEmpty( char * path );
int			dir_getFileCount( char * path );

///////////////////////////////////////////////////////////////////////////////

#endif
