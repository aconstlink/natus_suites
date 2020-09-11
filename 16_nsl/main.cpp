
#include "main.h"

#include <natus/nsl/parser.hpp>
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

    // test lib
    {
        db->load( natus::io::location_t( "shader.mylib.nsl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib )
        {
            natus::ntd::string_t const file = natus::ntd::string_t( d, sib ) ;
            natus::nsl::ast_t ast = natus::nsl::parser_t("mylib.nsl").process( file ) ;
        } ) ;
    }

    // test myshader
    {
        db->load( natus::io::location_t( "shader.myshader.nsl" ) ).wait_for_operation( [&] ( char_cptr_t d, size_t const sib )
        {
            natus::ntd::string_t const file = natus::ntd::string_t( d, sib ) ;
            natus::nsl::ast_t ast = natus::nsl::parser_t("myshader.nsl").process( file ) ;
        } ) ;
    }

    return 0 ;
}
