#ifndef __string_lib_h__
#define __string_lib_h__

///////////////////////////////////////////////////////////////

#include "types.h"

///////////////////////////////////////////////////////////////

typedef struct _string_t_
{
	char * buffer;
	
	uint32_t size;
	// uint32_t curr_size;
	uint32_t step_size;

} string_t;

///////////////////////////////////////////////////////////////

string_t * string_create( void );
string_t * string_create_ex( const char * text );
string_t * string_create_size( uint32_t curr_size );

void string_destroy( string_t * input );

uint32_t string_append_text( string_t * input, const char * text );
uint32_t string_append( string_t * input1, string_t * input2 );
uint32_t string_copy_text( string_t * input, const char * text );
uint32_t string_copy( string_t * input, string_t * input2 );

void string_replace_char ( string_t * input, char old_char, char new_char );

char * string_get_char_buffer( string_t * input );

string_t * string_get_substring( string_t * input, uint32_t start, uint32_t len );

int string_locate_content( string_t * input, const char * content );
int string_locate_content_ex( string_t * input, uint32_t start_index, const char * content );
int string_compare( string_t * input1, string_t * input2 );
int string_length( string_t * input );

char string_char_at( string_t * input, uint32_t pos );

void string_clear( string_t * intput );
void string_delete( string_t * object, uint32_t position, uint32_t len );

///////////////////////////////////////////////////////////////

#endif // __string_lib_h__
