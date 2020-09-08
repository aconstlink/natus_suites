
#include "main.h"

#include <natus/format/global.h>
#include <natus/io/database.h>
#include <natus/log/global.h>

using namespace natus::core::types ;

//
// 
//
int main( int argc, char ** argv )
{
    // create a database for the project
    natus::io::database_res_t db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;

    natus::io::monitor_res_t mon = natus::io::monitor_t() ;
    
    // track file changes for re-import
    {
        db->attach( mon ) ;
    }

    // the ctor will register some default factories...
    natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;

    // ... so this import will work by using the default module for png which might be the stb module.
    auto fitem = mod_reg->import_from( natus::io::location_t( "images.checker.png" ), db ) ;

    // wait for the import to be finished which is async.
    fitem.wait() ;

    natus::format::image_item_res_t iir = fitem.get() ;
    if( iir.is_valid() )
    {
        int bp = 0 ;
    }
    

    for( size_t i = 0; i < 10; ++i )
    {
        mon->for_each_and_swap( [&] ( natus::io::location_cref_t loc, natus::io::monitor_t::notify const n )
        {
            
        } ) ;

        ::std::this_thread::sleep_for( ::std::chrono::milliseconds( 1000 ) ) ;
    }

    return 0 ;
}
