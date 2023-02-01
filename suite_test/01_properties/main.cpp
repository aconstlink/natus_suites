
#include "main.h"

#include <natus/property/property_sheet.hpp>
#include <natus/log/global.h>
#include <natus/ntd/vector.hpp>

using namespace natus::core::types ;

//
int main( int argc, char ** argv )
{
    natus::property::property_sheet_t ps ;

    ps.set_value< int_t >( "my.int", 5 ) ;
    ps.set_value< float_t >( "my.float", 5.0f ) ;
    ps.set_value< natus::ntd::vector< uint32_t > >( "my.vec", { 5, 43, 9348, 2, 29 } ) ;



    return 0 ;
}
