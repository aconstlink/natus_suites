
#include "main.h"

#include <natus/gpu/async.h>
#include <natus/gpu/backend/id.hpp>
#include <natus/gpu/configuration/render_configuration.h>
#include <natus/gpu/backend/null/null.h>

// simply tests the async backend in a serial manner.
// it should check the flow of the id through the system
int main( int argc, char ** argv )
{
    natus::gpu::null_backend_res_t null_ = natus::gpu::null_backend_res_t() ;
    natus::gpu::async_res_t async_ = natus::gpu::async_res_t( natus::gpu::async_t( null_ ) ) ;
    
    natus::gpu::async_id_res_t aid ;

    // init 
    {
        natus::gpu::async_id_res_t aid0 ;
        natus::gpu::render_configuration_t rc ;
        async_->prepare( aid0, rc ) ;

        aid = ::std::move( aid0 ) ;
    }
    
    // runs in the render thread
    async_->update() ;

    {
        async_->render( aid ) ;
    }



    return 0 ;
}
