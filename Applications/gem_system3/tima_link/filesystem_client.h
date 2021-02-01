#ifndef APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_FILESYSTEM_CLIENT_H_
#define APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_FILESYSTEM_CLIENT_H_

////////////////////////////////////////////////////////////////////

#include "types.h"

////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////

void tima_link_filesystem_process( void );
void tima_link_filesystem_client_init( void );
void tima_link_filesystem_transaction_open_file( const char * filename, const char * mode );

////////////////////////////////////////////////////////////////////

#endif /* APPLICATIONS_GEM_SYSTEM3_TIMA_LINK_FILESYSTEM_CLIENT_H_ */
