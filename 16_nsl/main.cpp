
#include "main.h"

#include <natus/nsl/symbol.hpp>
#include <natus/nsl/database.hpp>
#include <natus/nsl/generator.hpp>
#include <natus/nsl/dependency_resolver.hpp>

#include <natus/format/global.h>
#include <natus/format/nsl/nsl_module.h>

#include <natus/io/database.h>
#include <natus/log/global.h>

using namespace natus::core::types ;

int main( int argc, char ** argv )
{
    natus::io::database_res_t db = natus::io::database_t( natus::io::path_t( DATAPATH ), "./working", "data" ) ;
    natus::nsl::database_res_t ndb = natus::nsl::database_t() ;

    natus::ntd::vector< natus::io::location_t > shader_locations = {
        natus::io::location_t( "shaders.lib_a.nsl" ),
        natus::io::location_t( "shaders.lib_b.nsl" ),
        natus::io::location_t( "shaders.effect.nsl" ),
    };

    natus::ntd::vector< natus::nsl::symbol_t > config_symbols ;

    for( auto const& l : shader_locations )
    {
        natus::format::module_registry_res_t mod_reg = natus::format::global_t::registry() ;
        auto fitem2 = mod_reg->import_from( l, db ) ;

        natus::format::nsl_item_res_t ii = fitem2.get() ;
        if( ii.is_valid() ) ndb->insert( std::move( std::move( ii->doc ) ), config_symbols ) ;
    }
    
    for( auto const & c : config_symbols )
    {
        natus::nsl::generatable_t res = natus::nsl::dependency_resolver_t().resolve( ndb, c ) ;
        if( res.missing.size() != 0 )
        {
            natus::log::global_t::warning( "We have missing symbols." ) ;
            for( auto const& s : res.missing )
            {
                natus::log::global_t::status( s.expand() ) ;
            }
        }

        {
            auto code = natus::nsl::generator_t( std::move( res ) ).generate() ;
            int const bp = 0 ;
        }
    }
    
    

    return 0 ;
}
