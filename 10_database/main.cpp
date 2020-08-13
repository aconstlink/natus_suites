
#include "main.h"

#include <natus/io/database.h>
#include <natus/log/global.h>

//
// this test packs a natus file from a file system
//
int main( int argc, char ** argv )
{
    //natus::log::global_t::status( natus::std::string_t(DATAPATH) ) ;

    natus::io::database db( natus::io::path_t( DATAPATH ) / "data" ) ;

    // show content


    //db.pack() ;

    return 0 ;
}
