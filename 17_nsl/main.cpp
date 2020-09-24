
#include "main.h"

#include <natus/nsl/parser.hpp>
#include <natus/nsl/database.hpp>
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
    natus::nsl::database_res_t nsl_db = natus::nsl::database_t() ;

    // test lib
    {
        db->load( natus::io::location_t( "shader.mylib.nsl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib )
        {
            natus::ntd::string_t file = natus::ntd::string_t( d, sib ) ;
            natus::nsl::post_parse::document_t doc = natus::nsl::parser_t("mylib.nsl").process( std::move( file ) ) ;

            nsl_db->insert( std::move( doc ) ) ;
        } ) ;
    }

    // test myshader
    {
        db->load( natus::io::location_t( "shader.myshader.nsl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib )
        {
            natus::ntd::string_t file = natus::ntd::string_t( d, sib ) ;
            natus::nsl::post_parse::document_t doc = natus::nsl::parser_t( "myshader.nsl" ).process( std::move( file ) ) ;

            nsl_db->insert( std::move( doc ) ) ;
        } ) ;
    }

    {
        natus::nsl::dependency_resolver_t dep_res( nsl_db ) ;

        dep_res.resolve( natus::nsl::symbol("myconfig") ) ;
    }

    return 0 ;
}
