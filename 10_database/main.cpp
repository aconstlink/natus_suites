
#include "main.h"

#include <natus/io/database.h>
#include <natus/log/global.h>

using namespace natus::core::types ;

//
// this test packs a natus file from a file system
//
int main( int argc, char ** argv )
{
    //natus::log::global_t::status( natus::std::string_t(DATAPATH) ) ;

    natus::io::database db( natus::io::path_t( DATAPATH ) / "data" ) ;

    {
        db.load( "data" ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, natus::io::result const res ) 
        { 
            if( res != natus::io::result::ok ) 
            {
                natus::log::global_t::error( "file not loaded with " + natus::io::to_string( res ) ) ;
                return ;
            }

            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( natus::std::string_t( data, sib ) ) ;
        } ) ;
    }

    {
        db.load( "data2" ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, natus::io::result const res )
        {
            if( res != natus::io::result::ok )
            {
                natus::log::global_t::error( "file not loaded with " + natus::io::to_string( res ) ) ;
                return ;
            }

            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( natus::std::string_t( data, sib ) ) ;
        } ) ;
    }

    {
        db.load( "subfolder.data" ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, natus::io::result const res )
        {
            if( res != natus::io::result::ok )
            {
                natus::log::global_t::error( "file not loaded with " + natus::io::to_string( res ) ) ;
                return ;
            }

            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( "resource loaded with " + ::std::to_string(sib) + " bytes" ) ;
        } ) ;
    }

    //db.pack() ;
    ::std::this_thread::sleep_for( ::std::chrono::milliseconds(10000) ) ;

    //db.dump_to_std() ;

    return 0 ;
}
