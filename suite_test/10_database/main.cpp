
#include "main.h"

#include <natus/io/database.h>
#include <natus/log/global.h>

using namespace natus::core::types ;

//
// this test packs a natus file from a file system into same directory
//
int main( int argc, char ** argv )
{
    natus::io::monitor_res_t mon = natus::io::monitor_t() ;

    //natus::io::database db ;
    natus::io::database db( natus::io::path_t( DATAPATH ) / "data" ) ;

#if 0
    {
        db.load( "data" ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, natus::io::result const res ) 
        { 
            if( res != natus::io::result::ok ) 
            {
                natus::log::global_t::error( "file not loaded with " + natus::io::to_string( res ) ) ;
                return ;
            }

            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( natus::ntd::string_t( data, sib ) ) ;
        } ) ;
    }

    {
        db.load( "noexist" ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib, natus::io::result const res )
        {
            if( res != natus::io::result::ok )
            {
                natus::log::global_t::error( "file not loaded with " + natus::io::to_string( res ) ) ;
                return ;
            }

            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( natus::ntd::string_t( data, sib ) ) ;
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

    db.attach( mon ) ;

    // attach individually.
    //db.attach( "data", mon ) ;
    //db.attach( "subfolder.data", mon ) ;

    db.pack() ;

    // test file monitoring
    // increase max_iter here and just save some of the txt files.
    size_t const max_iter = 0 ;
    for( size_t i=0; i<max_iter; ++i )
    {
        mon->for_each_and_swap( [&]( natus::ntd::string_cref_t loc, natus::io::monitor_t::notify const n )
        {
            natus::log::global_t::status( "[monitor] : Got " + natus::io::monitor_t::to_string(n) + " for " + loc ) ;
        }) ;

        ::std::this_thread::sleep_for( ::std::chrono::milliseconds(1000) ) ;
    }

    db.detach( mon ) ;

    // dump content
    db.dump_to_std() ;
#endif
    return 0 ;
}
