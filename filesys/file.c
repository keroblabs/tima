#include <string.h>
#include "system.h"
#include "file.h"
#include "efs.h"
#include "tima_libc.h"
#include "heap.h"

///////////////////////////////////////////////////////////////////////////////

#define CLUSTER_PREALLOC_FILE	2

///////////////////////////////////////////////////////////////////////////////

char volume[ VOLUME_MAX_SIZE ];

///////////////////////////////////////////////////////////////////////////////

static uint8_t dir_read_buffer[32];

int fs_last_error = FILE_OK;

///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////

uint32_t	_file_required_cluster(File *file,uint32_t offset, uint32_t size);
void		_file_initFile(File *file, FileSystem *fs, FileLocation *loc);
uint8_t		_file_validateChar(uint8_t c);

///////////////////////////////////////////////////////////////////////////////

char file_locatePath( const char * full_filename )
{
    //char fatfilename[11];
    FileLocation loc;
    FileSystem *fs;
	char * filename;
    
	// identify the device driver
	filename = file_getVolume( full_filename, volume );

    fs = efs_get_filesystem( volume );
	if( fs == NULL ) return( -6 );
    
    if( filename == NULL ) return FS_IS_ROOT_DIR;
    
    if( !strcmp( filename, "/" ) ) return FS_IS_ROOT_DIR;

    //dir_getFatFileName(filename,fatfilename);
    //dir_getFatFileName_Long( filename, long_filename );    
    
    return fs_findFile(fs,filename,&loc,0);  
}

void _file_set_pos(File *file,uint32_t pos)
{
    if( file == NULL ) return;
    
	if(pos<=file->FileSize)
    {
		file->FilePtr=pos;
	}
    else
    {
		file->FilePtr=file->FileSize;
    }
}

uint32_t _file_required_cluster(File *file,uint32_t offset, uint32_t size)
{
	uint32_t clusters_required,clustersize;
	uint32_t hc;

	if((offset+size)>file->FileSize)
	{
		if(file->Cache.ClusterCount==0)
		{ 
			/* Number of cluster unknown */
			hc = fat_countClustersInChain(file->fs,file->Cache.FirstCluster);
			file->Cache.ClusterCount = hc;
		}
		else
		{
			hc = file->Cache.ClusterCount; /* This better be right */
		}
		clustersize = file->fs->volumeId.BytesPerSector * file->fs->volumeId.SectorsPerCluster;
		if((size-file->FileSize+offset)>((hc-((file->FileSize+clustersize-1)/clustersize))*clustersize))
		{
			clusters_required = (((offset+size)-(hc*clustersize))+clustersize-1)/clustersize;
		}
		else
		{
			clusters_required = 0;
		}
	}
	else
	{
		clusters_required = 0;
	}
	return(clusters_required);
}

///////////////////////////////////////////////////////////////////////////////

int file_ferror( void )
{
	return fs_last_error;
}

uint32_t file_fread(uint8_t *buf,uint32_t size, uint32_t count, File *file)
{
	uint32_t r;
    
    if( file == NULL ) return 0;

	r=file_read(buf, size * count, file->FilePtr,file);
	file->FilePtr+=r;
	return(r);
}

int file_fgetc(File *file)
{
	int ret;
    int size;
    
    if( file == NULL ) return -1;
    
    ret = 0;
	size=file_read( ( uint8_t * )&ret, 1, file->FilePtr, file );
	file->FilePtr += size;
    
    if( size == 0 ) return -1;
    
	return(ret);
}

uint32_t file_fwrite(uint8_t *buf, uint32_t size, uint32_t count, File *file)
{
	uint32_t r;
    
    if( file == NULL ) return 0;

	r=file_write(buf,size*count,file->FilePtr,file);
	file->FilePtr+=r;
	return(r);
}

int file_fseek( File * file, int64_t offset, int origin )
{
	int64_t new_pos = 0;

	if( file == NULL ) FILE_INTERNAL_ERROR;

	switch( origin )
	{
		case FS_SEEK_SET:
			new_pos = (int64_t)offset;
			break;

		case FS_SEEK_CUR:
			new_pos = (int64_t)file->FilePtr + offset;
			break;

		case FS_SEEK_END:
			new_pos = (int64_t)file->FileSize - offset;
			break;
	}

	if( new_pos < 0 ) return FILE_OUT_OF_RANGE;
	else if( new_pos > ( int64_t )file->FileSize ) return FILE_OUT_OF_RANGE;

	file->FilePtr =  (uint32_t)new_pos;
	return FILE_OK;
}

void file_rewind( File * file )
{
    if( file != NULL ) file->FilePtr = 0;
}

int64_t file_ftell(File *file)
{
    if( file == NULL ) return 0;
	return((int64_t)file->FilePtr);
}

char file_feof(File *file)
{
    if( file == NULL ) return 1;
    if( file->fs == NULL ) 
	{
		return 1;
	}

    if( file->FilePtr >= file->FileSize ) return( 1 );
    
	return( 0 );
}

uint32_t file_read(uint8_t *buf, uint32_t size, uint32_t offset, File *file)
{
	uint32_t bytes_read=0,size_left=size,coffset=offset;
	uint32_t cclus,csec,cbyte;
	uint32_t rclus,rsec;
	uint32_t btr;
	uint8_t *tbuf;
    
	if(!file_getAttr(file,FILE_STATUS_OPEN))return(0);
    
    if( file_feof( file ) == TRUE ) return 0;
    
    if( buf == NULL )
    {
        return( file->FileSize - file->FilePtr );
    }
    
	if(offset>=file->FileSize)
		size_left=0; /* Offset check */

	if( (offset+size > file->FileSize) && size_left!=0)
		size_left=file->FileSize-offset;

	while(size_left>0)
	{
		cclus = coffset/(512*file->fs->volumeId.SectorsPerCluster);
		csec = (coffset/(512))%file->fs->volumeId.SectorsPerCluster;
		cbyte = coffset%512;

		if(cbyte!=0 || size_left<512)
		{
			btr = 512-(coffset%512)>=size_left?size_left:512-(coffset%512);
		}
		else
		{
			btr = 512;
		}

		if((fat_LogicToDiscCluster(file->fs,&(file->Cache),cclus))!=0)
		{
			return(0);
		}
		rclus=file->Cache.DiscCluster;
		rsec=fs_clusterToSector(file->fs,rclus);


		if(btr==512)
		{
			/*part_readBuf(file->fs.part,rsec+csec,buf+bytes_read);*/
			part_getSect(file->fs->partition,rsec+csec,buf+bytes_read);
		}
		else
		{
			/*part_readBuf(file->fs->part,rsec+csec,tbuf);*/
			tbuf = part_getSect(file->fs->partition,rsec+csec,0);
			// memcpy(tbuf+(coffset%512),buf+bytes_read,btr);
			memcpy(buf+bytes_read,tbuf+(coffset%512),btr);
		}

		coffset+=btr;
		bytes_read+=btr;
		size_left-=btr;
	}

	return(bytes_read);
}

char * file_fgets(char *buf,uint32_t size,File *file)
{
	static uint8_t buffer[ 51 ];
	uint32_t read_size = 0;
	uint32_t xfer_size = 0;
	uint32_t counter;

	while( ( file->FilePtr < file->FileSize ) || ( file->fs->type == SYS_DEVICE ) )
	{
		read_size = file_read( buffer, sizeof( buffer ), file->FilePtr, file ); // - 1;

		for( counter = 0; counter < read_size; counter++ )
		{
			if( ( buffer[ counter ] == 0x0A ) ||
				( ( buffer[ counter ] == 0x0D ) && ( buffer[ counter + 1 ] == 0x0A ) ) )
			{
				file->FilePtr++;
				if( buffer[ counter ] == 0x0D ) file->FilePtr++;

				return( buf );
			}

			buf[ xfer_size++ ] = buffer[ counter ];
			buf[ xfer_size ] = 0;

			file->FilePtr++;

			if( xfer_size >= ( size - 1 ) ) return( buf );
		}
        
        if( file->fs->type == SYS_DEVICE ) break; // return read_size;
	}

	return( buf );
}

uint8_t file_fputc( uint8_t data, File *file )
{
    file_fwrite( ( uint8_t * )&data, 1, 1, file );
	return data;
}

int file_fputs(const char * text, File *file )
{
    int total_bytes = file_fwrite( ( uint8_t * )text, (uint32_t)strlen( text ), 1, file );
    
    if( total_bytes == -1 ) return FILE_EOF;

    file->FilePtr--;

	return( total_bytes - 1 );
}

uint32_t file_write(uint8_t* buf,uint32_t size,uint32_t offset,File* file)
{
	uint32_t need_cluster;
	uint32_t cclus,csec,cbyte;
	uint32_t size_left=size,bytes_written=0;
	uint32_t rclus,rsec;
	uint32_t coffset=offset;
	uint16_t btr;
	uint8_t *tbuf;
    
	if(!file_getAttr(file,FILE_STATUS_OPEN) || !file_getAttr(file,FILE_STATUS_WRITE))return(0);
    
	if(offset>file->FileSize)
	{
		offset=file->FileSize;
	}

	need_cluster = _file_required_cluster(file,offset,size);

	if(need_cluster)
	{
		if(fat_allocClusterChain(file->fs,&(file->Cache),need_cluster+CLUSTER_PREALLOC_FILE)!=0)
		{
			return(0);
		}
	}

	while(size_left>0){

		cclus = coffset/(512*file->fs->volumeId.SectorsPerCluster);
		csec = (coffset/(512))%file->fs->volumeId.SectorsPerCluster;
		cbyte = coffset%512;

		if(cbyte!=0 || size_left<512)
		{
			btr = ( uint16_t )( ( 512-(coffset%512)>=size_left?size_left:512-(coffset%512) ) & 0x01FF );
		}
		else
		{
			btr = 512;
		}

		if((fat_LogicToDiscCluster(file->fs,&(file->Cache),cclus))!=0)
		{
			file->FileSize+=bytes_written;
			dir_setFileSize(file->fs,&(file->Location),file->FileSize);
			return(bytes_written);
		}
		rclus=file->Cache.DiscCluster;
		rsec=fs_clusterToSector(file->fs,rclus);

		if(btr==512)
		{
			part_writeBuf(file->fs->partition,rsec+csec,buf+bytes_written);
		}
		else
		{
			tbuf = part_getSect(file->fs->partition,rsec+csec,0);
			memcpy(tbuf+(coffset%512),buf+bytes_written,btr);
			part_writeBuf(file->fs->partition,rsec+csec,tbuf);
		}

		coffset+=btr;
		bytes_written+=btr;
		size_left-=btr;
	}

	if(bytes_written>file->FileSize-offset){
		file->FileSize+=bytes_written-(file->FileSize-offset);
    }

	return(bytes_written);
}

int file_fprintk( File *file, const char *fmt, ... )
{
	va_list args;	
	int printed_len;
	
    if( file == NULL ) return 0;
    if(!file_getAttr(file,FILE_STATUS_OPEN) || !file_getAttr(file,FILE_STATUS_WRITE))return(0);
    
	/* Emit the output into the temporary buffer */ 
	va_start(args, fmt);	
	printed_len = vsnprintk(vsf_printk_buf, VSF_PRINTK_BUF_SIZE, fmt, args); 
	va_end(args);	

	/*	 * Copy the output into log_buf.  
	       If the caller didn't provide	 
	     * appropriate log level tags, we insert them here	 */ 

    return file_fputs( ( const char * )vsf_printk_buf, file );  
}

bool_t file_isRoot( const char * full_filename )
{
    char result = file_locatePath( full_filename );
    if( result == FS_IS_ROOT_DIR ) return TRUE;
    return FALSE;
}

bool_t file_IsDirectory( const char * full_filename )
{
    char result = file_locatePath( full_filename );
    
    if( ( result == FS_IS_DIRECTORY ) || ( result == FS_IS_ROOT_DIR ) ) return TRUE;
    return FALSE;
}

bool_t file_Exists( const char * full_filename )
{
    if( file_locatePath( full_filename ) == FS_IS_FILE ) return TRUE;
    return FALSE;
}

char * file_getVolume( const char * full_filename, char * volume )
{
	char * stop_ptr;
	//int counter;
	//int size;

    strcpy( volume, full_filename );
	stop_ptr = strchr( &volume[ 1 ], '/' );
    
    if( stop_ptr != NULL )
        *stop_ptr = 0;

	stop_ptr = ( char * )strchr( &full_filename[ 1 ], '/' );
    return( stop_ptr );
}

uint32_t file_fioctrl( File * file, uint32_t param, void * data )
{
    if( file == NULL ) return FILE_INVALID_MODE;

	if( param == IO_CONTROL_FILE_SIZE ) return file->FileSize;
	else if( param == IO_CONTROL_FILE_POSITION ) return file->FilePtr;

    return FILE_INVALID_MODE;
}

File * file_fopen(char* full_filename,char mode)
{
    char fatfilename[11];
    FileRecord wtmp;
    FileLocation loc;
    uint32_t sec;
    FileSystem *fs;
	char find_file_ret;
	char * filename;
    char * long_filename;
	File * file;
    
	fs_last_error = FILE_OK;

	file = ( File * )MMALLOC( sizeof( File ) );
	if( file == NULL ) 
	{
		fs_last_error = FILE_INTERNAL_ERROR;
		return NULL;
	}

    memset( file, 0x00, sizeof( File ) );

	// identify the device driver
	filename = file_getVolume( full_filename, volume );

    fs = efs_get_filesystem( volume );
	if( fs == NULL ) 
	{ 
		MFREE( file ); 
		fs_last_error = FILE_INVALID_FS; 
		return NULL; 
	}

    dir_getFatFileName(filename,fatfilename);

    switch(mode)
	{
        case FS_MODE_READ:
            //DEBUG_PRINTK( "fopen filename: %s = ", filename );
			find_file_ret = fs_findFile(fs,filename,&loc,0);
            //DEBUG_PRINTK( "%d\r\n", find_file_ret );
            if( ( find_file_ret == FS_IS_DIRECTORY ) || ( find_file_ret == FS_IS_FILE ) )
			{
                dir_getFileStructure(fs,&(file->DirEntry), &loc);
                _file_initFile(file,fs,&loc);
				file_setAttr(file,FILE_STATUS_OPEN, TRUE);
				file_setAttr(file,FILE_STATUS_WRITE, FALSE);
                
                if( find_file_ret == FS_IS_DIRECTORY )
                {
                    // for directory, determine the data size
                    file->FileSize = ( 1024 * 1024 );
                    
                    while( !file_feof( file ) )
                    {
                        file_fread( dir_read_buffer, 32, 1, file );
                        if( dir_read_buffer[0] == 0 )
                        {
                            // one of the directory entries is NULL, this is the end
                            file->FileSize = file->FilePtr - 32;
                            file->FilePtr = 0;
                            break;
                        }
                    }
                }
				fs_last_error = FILE_OK;
				return(file);
            }
			MFREE( file );
			fs_last_error = FILE_NOT_FOUND;
            return(NULL);

        case FS_MODE_WRITE:
            if(fs_findFile(fs,filename,&loc,&sec)) /* File may NOT exist, but parent HAS to exist */
			{
				fs_fileDelete( fs, ( uint8_t * )filename );
                // return(FILE_OPEN_ALREADY_EXIST);
			}

			if(sec==0)
            { 
                /* Parent dir does not exist */
				MFREE( file );
				fs_last_error = FILE_WRITE_INVALID;
 				return(NULL);
			}
            
            if(fs_findFreeFile(fs,filename,&loc,0))
			{
                long_filename = ( char * )MMALLOC( VOLUME_MAX_SIZE );
                if( long_filename == NULL ) 
				{ 
					MFREE( file );
					fs_last_error = FILE_UNABLE_TO_WRITE; 
					return NULL; 
				}
                
                dir_getFatFileName_Long( filename, long_filename );
                
                dir_createDefaultEntry(fs,&wtmp,fatfilename);
                dir_createDirectoryEntry(fs,&wtmp,&loc, long_filename, filename );
                memcpy(&(file->DirEntry),&wtmp,sizeof(wtmp));
				_file_initFile(file,fs,&loc);
                sec=fs_getNextFreeCluster(file->fs,fs_giveFreeClusterHint(file->fs));
                dir_setFirstCluster(file->fs,&(file->Location),sec);
                fs_setFirstClusterInDirEntry(&(file->DirEntry),sec);
                fs_initClusterChain(fs,&(file->Cache),sec);
                fat_setNextClusterAddress(fs,sec,fat_giveEocMarker(fs));
				file_setAttr(file,FILE_STATUS_OPEN, TRUE);
				file_setAttr(file,FILE_STATUS_WRITE, TRUE);
                
				MFREE( long_filename );
				fs_last_error = FILE_OK;
            	return(file);
			}
            else
			{
				MFREE( file );
				fs_last_error = FILE_WRITE_ERROR;
                return(NULL);
			}
            break;
            
        case FS_MODE_APPEND:
        case FS_MODE_RANDOM:
			if(fs_findFile(fs,filename,&loc,0)==FS_IS_FILE) /* File exists */
			{
				dir_getFileStructure(fs,&(file->DirEntry), &loc);
				_file_initFile(file,fs,&loc);
				if(file->Cache.FirstCluster==0)
                {
					sec=fs_getNextFreeCluster(file->fs,fs_giveFreeClusterHint(file->fs));
					dir_setFirstCluster(file->fs,&(file->Location),sec);
					fs_setFirstClusterInDirEntry(&(file->DirEntry),sec);
					fat_setNextClusterAddress(fs,sec,fat_giveEocMarker(fs));
					_file_initFile(file,fs,&loc);
				}
                if( mode == FS_MODE_RANDOM ) _file_set_pos(file, 0);
				else _file_set_pos(file,file->FileSize);
				file_setAttr(file,FILE_STATUS_OPEN, TRUE);
				file_setAttr(file,FILE_STATUS_WRITE, TRUE);
			}
			else /* File does not exist */
			{
				MFREE( file );
				fs_last_error = FILE_NOT_FOUND;
				return(NULL);
			}
#if 0
			{
				if(fs_findFreeFile(fs,filename,&loc,0))
				{
					dir_createDefaultEntry(fs,&wtmp,fatfilename);
					dir_createDirectoryEntry(fs,&wtmp,&loc, filename, filename );
					memcpy(&(file->DirEntry),&wtmp,sizeof(wtmp));
					_file_initFile(file,fs,&loc);
					sec=fs_getNextFreeCluster(file->fs,fs_giveFreeClusterHint(file->fs));
					dir_setFirstCluster(file->fs,&(file->Location),sec);
	                fs_setFirstClusterInDirEntry(&(file->DirEntry),sec);
    	            fs_initClusterChain(fs,&(file->Cache),sec);
					fat_setNextClusterAddress(fs,sec,fat_giveEocMarker(fs));
					file_setAttr(file,FILE_STATUS_OPEN, TRUE);
					file_setAttr(file,FILE_STATUS_WRITE, TRUE);
				}
				else
				{
					MFREE( file );
					fs_last_error = FILE_WRITE_ERROR; 
					return NULL;
				}
			}
#endif
			fs_last_error = FILE_OK; 
			return file;
    }

	MFREE( file );
	fs_last_error = FILE_INVALID_MODE; 
    return(NULL);
}

void file_fclose(File *file)
{
    if(fs_hasTimeSupport()){
        if(file_getAttr(file,FILE_STATUS_WRITE))
        {
            file->DirEntry.AccessDate = fs_makeDate();
            file->DirEntry.FileSize = file->FileSize;
            file->DirEntry.WriteDate = file->DirEntry.AccessDate;
            file->DirEntry.WriteTime = fs_makeTime();
            dir_updateDirectoryEntry(file->fs,&(file->DirEntry),&(file->Location));
        }
    }
    else
    {
        if(file_getAttr(file,FILE_STATUS_WRITE))
        {
            dir_setFileSize(file->fs,&(file->Location),file->FileSize);
			fs_updateFreeClusters(file->fs);
        }
    }
    
	MFREE( file );
}

void _file_initFile(File *file, FileSystem *fs, FileLocation *loc)
{
	file->fs=fs;

	file->FileSize=file->DirEntry.FileSize;
	file->FilePtr=0;
    file->Location.firstSector=loc->firstSector;
	file->Location.Sector=loc->Sector;
	file->Location.Offset=loc->Offset;
	file->Cache.Linear=0;
	file->Cache.FirstCluster=(((uint32_t)file->DirEntry.FirstClusterHigh)<<16)+
	                                    file->DirEntry.FirstClusterLow;
	file->Cache.LastCluster=0;
	file->Cache.LogicCluster=0;
	file->Cache.DiscCluster=file->Cache.FirstCluster;
}

char* file_normalToFatName(char* filename,char* fatfilename)
{
	uint8_t c,dot=0,vc=0;

	for(c=0;c<11;c++)fatfilename[c]=' ';

	c=0;

	if(*filename == '.')
	{
		fatfilename[0]='.';
		vc++;
		if(*(filename+1) == '.')
		{
			fatfilename[1]='.';
			filename+=2;
		}
		else
		{
			filename++;
		}
	}
	else
	{
		while(*filename != '\0' && *filename != '/')
		{
		// while(*filename != '\0' && *filename != ' ' && *filename != '/'){
			if(*filename=='.' && !dot)
			{
				dot=1;
				c=8;
			}
			else
			{
				if(dot)
				{
					if(c<=10)
					{
						fatfilename[c]=_file_validateChar(*filename);
						c++;
					}
				}
				else
				{
					if(c<=7)
					{
						fatfilename[c]=_file_validateChar(*filename);
						c++; vc++;
					}
				}
			}
			filename++;
		}
	}

	if(vc>0)
	{
		if(*filename=='\0')
		{
			return(filename);
		}
		else
		{
			return(filename+1);
		}
	}
	else
	{
		return(0);
	}
}

char* file_normalToFatName2(char* filename,char* fatfilename)
{
	uint8_t c;
    // uint8_t dot=0;
    uint8_t vc=0;

	// for(c=0;c<11;c++)fatfilename[c]=' ';

	c=0;

	if(*filename == '.')
    {
		fatfilename[0]='.';
		vc++;
		if(*(filename+1) == '.')
        {
			fatfilename[1]='.';
			filename+=2;
		}
        else
        {
			filename++;
		}
	}
    else
    {
		while(*filename != '\0' && *filename != '/')
        {
            #if 0
		    // while(*filename != '\0' && *filename != ' ' && *filename != '/'){
			if(*filename=='.' && !dot)
            {
				dot=1;
				c=8;
			}
            else
            {
				if(dot)
                {
					if(c<=10)
                    {
						fatfilename[c]=_file_validateChar(*filename);
						c++;
					}
				}
                else
                {
					if(c<=7)
                    {
						fatfilename[c]=_file_validateChar(*filename);
						c++;
                        vc++;
					}
				}
			}
            #endif

    		fatfilename[c]=(*filename);
			vc++;
    		c++;
    		fatfilename[c]=0;

			filename++;
		}
	}

	if(vc>0)
    {
		if(*filename=='\0')
        {
			return(filename);
		}
        else
        {
			return(filename+1);
		}
	}
    else
    {
		return(0);
	}
}

uint8_t _file_validateChar(uint8_t c)
{
    if( (c<0x20) || (c>0x20&&c<0x30&&c!='-') || (c>0x39&&c<0x41) || (c>0x5A&&c<0x61&&c!='_') ||	(c>0x7A&&c!='~') )
		return(0x58);
    if( c>=0x61 && c<=0x7A )
		return(c-32);

	return(c);
}

void file_setAttr(File* file,uint8_t attribute,uint8_t val)
{
	if(val)
	{
		file->FileStatus|=1<<attribute;
	}
	else
	{
		file->FileStatus&=~(1<<attribute);
	}
}

uint8_t file_getAttr(File* file,uint8_t attribute)
{
	return((file->FileStatus&(1<<attribute))!=0);
}

char file_remove( char* full_filename )
{
    //char fatfilename[11];
    FileSystem *fs = NULL;
	char * filename;
    
	// identify the device driver
	filename = file_getVolume( full_filename, volume );

    fs = efs_get_filesystem( volume );
	if( fs == NULL ) return( FILE_INVALID_FS );
    
    if( filename == NULL ) return FILE_NOT_FOUND;
    
    //dir_getFatFileName(filename,fatfilename);
    //dir_getFatFileName_Long( filename, long_filename );   

    return fs_fileDelete( fs, ( uint8_t * )filename );
}

void file_flush( File * file )
{
}

