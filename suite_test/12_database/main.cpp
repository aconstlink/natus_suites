
#include "main.h"

#include <natus/io/database.h>
#include <natus/log/global.h>

using namespace natus::core::types ;

//
// this test packs a natus database file from a file system working directory
// and for a few seconds, the data monitor can be tested by saving files in the working dir.
//
int main( int argc, char ** argv )
{
    natus::io::monitor_res_t mon = natus::io::monitor_t() ;

    natus::io::database db( natus::io::path_t( DATAPATH ), "./working", "data" ) ;

    {
        db.load( natus::io::location_t( "images.checker.png" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib ) 
        { 

            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( "loaded images.checker with " + ::std::to_string(sib) + " bytes" ) ;
        } ) ;
    }

    {
        db.load( natus::io::location_t( "some_info.txt" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib )
        {
            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( natus::ntd::string_t( data, sib ) ) ;
        } ) ;
    }

    {
        db.load( natus::io::location_t( "meshes.text.obj" ) ).wait_for_operation( [&] ( char_cptr_t data, size_t const sib )
        {
            natus::log::global_t::status( "********************************" ) ;
            natus::log::global_t::status( natus::ntd::string_t( data, sib ) ) ;
        } ) ;
    }

    db.attach( mon ) ;
    //db.attach( "data", mon ) ;
    //db.attach( "subfolder.data", mon ) ;

    db.pack() ;

    for( size_t i=0; i<10; ++i )
    {
        mon->for_each_and_swap( [&]( natus::io::location_cref_t loc, natus::io::monitor_t::notify const n )
        {
            natus::log::global_t::status( "[monitor] : Got " + natus::io::monitor_t::to_string(n) + " for " + loc.as_string() ) ;
        }) ;

        ::std::this_thread::sleep_for( ::std::chrono::milliseconds(1000) ) ;
    }

    db.detach( mon ) ;

    //db.dump_to_std() ;

    return 0 ;
}
